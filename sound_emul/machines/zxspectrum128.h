// Copyright 2017-2018 utz. See LICENSE for details.

#ifndef SOUND_EMUL_MACHINES_ZXSPECTRUM128_H_
#define SOUND_EMUL_MACHINES_ZXSPECTRUM128_H_

#include <memory>
#include <vector>
#include "vm.h"
#include "CPU/z80.h"
#include "CPU/memory.h"

extern "C"
{
#include "AY/ayumi.h"
}

class Virtual_ZX128 : public Virtual_Machine {
 public:
    Z80Memory memory;

    void init(Work_Tune *tune = nullptr) override;
    void load_binary(char *code, const int &codeSize, const int &startAddress) override;
    void load_raw_data(const std::vector<char> &data, const int &orgAddress) override;
    void generate_audio_chunk(std::ostringstream &AUDIOSTREAM, const uint64_t &audioChunkSize,
                              const unsigned &playMode) override;
    float get_audio_sample_rate() override;
    void set_breakpoints(const int64_t &initBP, const int64_t &exitBP, const int64_t &reloadBP) override;
    bool has_stopped() override;

    Virtual_ZX128();
    ~Virtual_ZX128() override = default;

 private:
    z80cpu cpu;
    Work_Tune *currentTune;
    std::unique_ptr<ayumi> ay;
    bool prgmIsInitialized;
    bool prgmHasFinished;
    int bpInit;
    int bpExit;
    int bpReload;

    void insertVirtualOSReturn(int startAddress);
    void init_prgm();
};


#endif  // SOUND_EMUL_MACHINES_ZXSPECTRUM128_H_
