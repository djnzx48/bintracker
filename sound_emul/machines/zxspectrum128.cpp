// Copyright 2017-2018 utz. See LICENSE for details.

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <samplerate.h>
// #include <chrono>

#include "zxspectrum128.h"

using std::vector;
using std::ifstream;
using std::cout;

Virtual_ZX128::Virtual_ZX128()
    :memory {Z80Memory::Configuration::Spectrum128}, cpu {&memory, Z80Type::NMOS},
    ay {std::make_unique<ayumi>()}
{
    ayumi_configure(ay.get(), 0, 1773400, 45600); // change 0 to 1 for YM

    ayumi_set_pan(ay.get(), 0, 0.5, 0);
    ayumi_set_pan(ay.get(), 1, 0.5, 0);
    ayumi_set_pan(ay.get(), 2, 0.5, 0);

    cpu.inputPortsShort[0xfe] = 0xff;   // return no_key_down on keyboard checks

    ifstream ROMFILE("resources/roms/zxspectrum128.rom", ios::binary);
    if (ROMFILE.is_open()) {
        std::array<uint8_t, 0x4000> rom1;
        std::array<uint8_t, 0x4000> rom2;
        ROMFILE.seekg(0, ios::beg);
        ROMFILE.read(reinterpret_cast<char*>(rom1.data()), 0x4000);
        ROMFILE.read(reinterpret_cast<char*>(rom2.data()), 0x4000);

        memory.bank_switch(0b00000);
        memory.fill_bank(0, rom1);

        memory.bank_switch(0b10000);
        memory.fill_bank(1, rom2);

        memory.bank_switch(0b00000);
    } else {
        cout << "Warning: Could not load zxspectrum128.rom\n";
        std::array<uint8_t, 0x4000> rom1;
        std::array<uint8_t, 0x4000> rom2;

        generate_random_data(rom1.begin(), rom1.end());
        generate_random_data(rom2.begin(), rom2.end());

        memory.bank_switch(0b00000);
        memory.fill_bank(0, rom1);

        memory.bank_switch(0b10000);
        memory.fill_bank(1, rom2);

        memory.bank_switch(0b00000);
    }

    prgmIsInitialized = false;
    prgmHasFinished = true;
    bpInit = 0;
    bpExit = 0;
    bpReload = 0;
    currentTune = nullptr;
}


bool Virtual_ZX128::has_stopped() {
    return prgmHasFinished;
}


void Virtual_ZX128::init(Work_Tune *tune) {
    currentTune = tune;
}


void Virtual_ZX128::init_prgm() {
    cpu.reset();
    cpu.setPC(bpInit);

    prgmIsInitialized = true;
    prgmHasFinished = false;
}


void Virtual_ZX128::set_breakpoints(const int64_t &initBP, const int64_t &exitBP, const int64_t &reloadBP) {
    bpInit = initBP;
    bpExit = exitBP;
    bpReload = reloadBP;
}


void Virtual_ZX128::load_binary(char *code, const int &codeSize, const int &startAddress) {
    for (int i = 0; i < codeSize; i++) memory.write(i + startAddress, code[i] & 0xff);
    prgmIsInitialized = false;
}


void Virtual_ZX128::load_raw_data(const vector<char> &data, const int &orgAddress) {
    for (int i = 0; static_cast<unsigned>(i) < data.size(); i++) memory.write(i + orgAddress, data[i] & 0xff);
}


float Virtual_ZX128::get_audio_sample_rate() {
    return 3546900.0 / 77.0;
}


void Virtual_ZX128::generate_audio_chunk(ostringstream &AUDIOSTREAM, const uint64_t &audioChunkSize,
                                        const unsigned &playMode) {
    if (!prgmIsInitialized) init_prgm();

    unsigned internalSampleCount = 0;
    uint64_t externalSampleCount = 0;
    int prevPC = cpu.getPC();

    // beeper disabled for now

    while ((cpu.getPC() != bpExit) && (externalSampleCount < audioChunkSize)) {
        for (int i = 0; (i < 8) && (cpu.getPC() != bpExit); ++i) {
            if (cpu.getPC() == bpReload && cpu.getPC() != prevPC) {
                vector<char> data = currentTune->reload_data(playMode);
                for (int j = 0; static_cast<unsigned>(j) < data.size(); j++)
                    memory.write(j + currentTune->engineSize + currentTune->orgAddress, data[j] & 0xff);

                if (playMode > 2 && currentTune->musicdataBinary.symbols.count("loop_point_patch")) {
                    int64_t lp = currentTune->musicdataBinary.symbols.at(currentTune->config.seqLoopLabel);
                    int64_t lpPatchAddr = currentTune->musicdataBinary.symbols.at("loop_point_patch");
                    memory.write(lpPatchAddr, lp & 0xff);
                    memory.write(lpPatchAddr + 1, (lp>>8) & 0xff);
                }
            }

            prevPC = cpu.getPC();
            cpu.execute_cycle();
        }

        // TODO this should be in a separate AY sourcefile.
        enum class AyReg
        {
            ChanAFinePitch,
            ChanACoarsePitch,
            ChanBFinePitch,
            ChanBCoarsePitch,
            ChanCFinePitch,
            ChanCCoarsePitch,
            NoisePitch,
            Mixer,
            ChanAVolume,
            ChanBVolume,
            ChanCVolume,
            EnvFinePitch,
            EnvCoarsePitch,
            EnvShape,
            IoPortA,
            IoPortB
        };

        if (cpu.justWroteToAy)
        {
            cpu.justWroteToAy = false;

            int portval = cpu.outputPorts[0xbffd];
            auto currentReg = static_cast<AyReg>(cpu.outputPorts[0xfffd] & 0x0f); // actually high 4 bits are ignored but don't worry for now.

            switch (currentReg)
            {
                case AyReg::Mixer:
                    ayumi_set_mixer(ay.get(), 0, portval & 1, (portval >> 3) & 1, 0); // why does this lib make you specify envelope???
                    ayumi_set_mixer(ay.get(), 1, (portval >> 1) & 1, (portval >> 4) & 1, 0); // oh well, we're not using it anyway
                    ayumi_set_mixer(ay.get(), 2, (portval >> 2) & 1, (portval >> 5) & 1, 0);

                    break;
                case AyReg::ChanAVolume:
                    ayumi_set_volume(ay.get(), 0, portval & 0xf);

                    break;
                case AyReg::ChanBVolume:
                    ayumi_set_volume(ay.get(), 1, portval & 0xf);

                    break;
                case AyReg::ChanCVolume:
                    ayumi_set_volume(ay.get(), 2, portval & 0xf);

                    break;
                // don't care about others right now.
            }
        }

        internalSampleCount++;

        if (internalSampleCount == 11) {
            // render the sample

            ayumi_process(ay.get());
            ayumi_remove_dc(ay.get());

            int16_t internalSample = ay.get()->left * 65535; // scale it up, assume value in [0, 1]

            // duplicate to stereo
            AUDIOSTREAM << static_cast<char>(internalSample & 0xff) << static_cast<char>((internalSample >> 8) & 0xff)
                        << static_cast<char>(internalSample & 0xff) << static_cast<char>((internalSample >> 8) & 0xff);

            internalSampleCount = 0;
            externalSampleCount++;
        }
    }

    if (cpu.getPC() == bpExit) {
        prgmHasFinished = true;
        prgmIsInitialized = false;
        for (; externalSampleCount < audioChunkSize; externalSampleCount++) AUDIOSTREAM << 0 << 0 << 0 << 0;
    }
}
