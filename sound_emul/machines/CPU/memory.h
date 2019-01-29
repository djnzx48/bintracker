#ifndef SOUND_EMUL_MACHINES_CPU_MEMORY_H_
#define SOUND_EMUL_MACHINES_CPU_MEMORY_H_

#include <array>
#include <vector>

class Z80Memory
{
public:
    enum class Configuration
    {
        Spectrum48, Spectrum128
    };

    explicit Z80Memory(Configuration config);

    Z80Memory(const Z80Memory&) = delete;
    Z80Memory& operator=(const Z80Memory&) = delete;
    Z80Memory(Z80Memory&&) = delete;
    Z80Memory& operator=(Z80Memory&&) = delete;
    ~Z80Memory() = default;

    void fill_bank(int bank, const std::array<uint8_t, 0x4000>& bytes);
    inline void bank_switch(uint8_t bank);
    inline unsigned read(int index) const;
    inline void write(int index, unsigned byte);

private:
    Configuration config;
    std::vector<unsigned> memory;
    std::array<unsigned, 4> banks;
    std::array<bool, 4> write_allowed;
};

void Z80Memory::bank_switch(uint8_t bank)
{
    switch (config)
    {
        case Configuration::Spectrum128:
            int ram_bank {bank & 0x07};
            int rom_bank {(bank >> 4) & 0x01};

            std::get<0>(banks) = 0x4000 * ram_bank;
            std::get<3>(banks) = 0x4000 * rom_bank + 0x20000;

            break;
    }
}

unsigned Z80Memory::read(int index) const
{
    index &= 0xffff;
    int bank {index >> 14};

    return memory.at(banks.at(bank) + (index & 0x3fff));
}

void Z80Memory::write(int index, unsigned byte)
{
    index &= 0xffff;
    int bank {index >> 14};

    if (write_allowed.at(bank))
        memory.at(banks.at(bank) + (index & 0x3fff)) = byte;
}

#endif  // SOUND_EMUL_MACHINES_CPU_MEMORY_H_
