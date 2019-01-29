// Copyright 2017-2018 utz. See LICENSE for details.

#ifndef BINTRACKER_HELPERS__H__
#define BINTRACKER_HELPERS__H__

#include <random>
#include <vector>
#include <string>

enum ParameterType {MD_BOOL, MD_BYTE, MD_WORD, MD_DEC, MD_HEX, MD_STRING, MD_INVALID};

bool isNumber(const std::string& str);
int64_t strToNum(std::string str);
std::string numToStr(const int64_t& num, const int& padding, const bool& hexFormat);
template<typename T> void generate_random_data(T begin, T end);

// from MDAL
std::string trimChars(const std::string& inputString, const char* chars);
int getType(const std::string& parameter);
std::string getArgument(const std::string& token, const std::vector<std::string>& moduleLines);

// generate random data via el cheapo xorshift* implementation
template<typename T>
void generate_random_data(T begin, T end)
{
    uint64_t state = std::random_device()();
    uint64_t x;

    for (; begin != end; ++begin)
    {
        x = state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        state = x;
        x *= 0x2545F4914F6CDD1D;
        *begin = static_cast<unsigned>(x >> 32);
    }
}

#endif  // BINTRACKER_HELPERS__H__
