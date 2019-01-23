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

Virtual_ZX128::Virtual_ZX128() : cpu((memory.fill(0), &memory), Z80Type::NMOS) {
    cpu.inputPortsShort[0xfe] = 0xff;   // return no_key_down on keyboard checks

    ifstream ROMFILE("resources/roms/zxspectrum128.rom", ios::binary);
    if (ROMFILE.is_open()) {
        char buffer[0x4000];
        ROMFILE.seekg(0, ios::beg);
        ROMFILE.read(buffer, 0x4000);
        for (int i = 0; i < 0x4000; i++) memory[i] = buffer[i] & 0xff;
    } else {
        cout << "Warning: Could not load zxspectrum128.rom\n";
        vector<unsigned> randomData = generate_random_data(0x4000);
        for (int i = 0; i < 0x4000; i++) memory[i] = randomData[i] & 0xff;
    }

    prgmIsInitialized = false;
    prgmHasFinished = true;
    previousSample = 0;
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

    previousSample = 0;
    prgmIsInitialized = true;
    prgmHasFinished = false;
}


void Virtual_ZX128::set_breakpoints(const int64_t &initBP, const int64_t &exitBP, const int64_t &reloadBP) {
    bpInit = initBP;
    bpExit = exitBP;
    bpReload = reloadBP;
}


void Virtual_ZX128::load_binary(char *code, const int &codeSize, const int &startAddress) {
    for (int i = 0; i < codeSize; i++) memory[i + startAddress] = code[i] & 0xff;
    prgmIsInitialized = false;
}


void Virtual_ZX128::load_raw_data(const vector<char> &data, const int &orgAddress) {
    for (int i = 0; static_cast<unsigned>(i) < data.size(); i++) memory[i + orgAddress] = data[i] & 0xff;
}


float Virtual_ZX128::get_audio_sample_rate() {
    return 3546900.0 / 77.0;
}


void Virtual_ZX128::generate_audio_chunk(ostringstream &AUDIOSTREAM, const uint64_t &audioChunkSize,
                                        const unsigned &playMode) {
    if (!prgmIsInitialized) init_prgm();

    int64_t internalSample = 0;
    unsigned internalSampleCount = 0;
    uint64_t externalSampleCount = 0;
    int prevPC = cpu.getPC();

    while ((cpu.getPC() != bpExit) && (externalSampleCount < audioChunkSize)) {
        for (int i = 0; (i < 8) && (cpu.getPC() != bpExit); ++i) {
            if (cpu.getPC() == bpReload && cpu.getPC() != prevPC) {
                vector<char> data = currentTune->reload_data(playMode);
                for (int j = 0; static_cast<unsigned>(j) < data.size(); j++)
                    memory[j + currentTune->engineSize + currentTune->orgAddress] = data[j] & 0xff;

                if (playMode > 2 && currentTune->musicdataBinary.symbols.count("loop_point_patch")) {
                    int64_t lp = currentTune->musicdataBinary.symbols.at(currentTune->config.seqLoopLabel);
                    int64_t lpPatchAddr = currentTune->musicdataBinary.symbols.at("loop_point_patch");
                    memory[lpPatchAddr] = lp & 0xff;
                    memory[lpPatchAddr + 1] = (lp>>8) & 0xff;
                }
            }

            prevPC = cpu.getPC();
            cpu.execute_cycle();
        }

        internalSample += ((cpu.outputPortsShort[0xfe] & 0x10) + ((cpu.outputPortsShort[0xfe] & 0x8) >> 3));
        internalSampleCount++;

        if (internalSampleCount == 11) {
            internalSample = static_cast<int64_t>((((internalSample - 0x0f) << 4) / 11) * 8 * 15);

            // simple LP filter
            internalSample = previousSample + static_cast<int64_t>(((internalSample - previousSample) << 3) / 10);
            previousSample = internalSample;

            // duplicate to stereo
            AUDIOSTREAM << static_cast<char>(internalSample & 0xff) << static_cast<char>((internalSample >> 8) & 0xff)
                        << static_cast<char>(internalSample & 0xff) << static_cast<char>((internalSample >> 8) & 0xff);

            internalSampleCount = 0;
            internalSample = 0;
            externalSampleCount++;
        }
    }

    if (cpu.getPC() == bpExit) {
        prgmHasFinished = true;
        prgmIsInitialized = false;
        for (; externalSampleCount < audioChunkSize; externalSampleCount++) AUDIOSTREAM << 0 << 0 << 0 << 0;
    }
}
