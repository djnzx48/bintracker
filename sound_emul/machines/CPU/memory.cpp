#include "memory.h"

Z80Memory::Z80Memory(Configuration config)
    :config {config}
{
    switch (config)
    {
        case Configuration::Spectrum48:
            memory.assign(0x10000, 0);

            std::get<0>(banks) = 0x0000;
            std::get<1>(banks) = 0x4000;
            std::get<2>(banks) = 0x8000;
            std::get<3>(banks) = 0xc000;

            std::get<0>(write_allowed) = false;
            std::get<1>(write_allowed) = true;
            std::get<2>(write_allowed) = true;
            std::get<3>(write_allowed) = true;

            break;
        case Configuration::Spectrum128:
            memory.assign(0x28000, 0);

            // 0x00000: bank 0
            // 0x04000: bank 1
            // 0x08000: bank 2
            // 0x0c000: bank 3
            // 0x10000: bank 4
            // 0x14000: bank 5
            // 0x18000: bank 6
            // 0x1c000: bank 7
            // 0x20000: rom 0
            // 0x24000: rom 1

            std::get<0>(banks) = 0x20000;
            std::get<1>(banks) = 0x14000;
            std::get<2>(banks) = 0x08000;
            std::get<3>(banks) = 0x00000;

            std::get<0>(write_allowed) = false;
            std::get<1>(write_allowed) = true;
            std::get<2>(write_allowed) = true;
            std::get<3>(write_allowed) = true;
    }
}

void Z80Memory::fill_bank(int bank, const std::array<uint8_t, 0x4000>& bytes)
{
    for (int i {0}; i != 0x4000; ++i)
    {
        memory.at(banks.at(bank) + i) = bytes.at(i);
    }
}
