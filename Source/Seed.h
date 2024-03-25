#pragma once

#include "Common.h"

struct Seed final
{
    Seed() = default;

    uint32 seed = std::random_device{}();

    // these both are stored as 4-bit values, they should not exceed 16:
    uint8 questGeneratorVersion = 1;
    uint8 difficultyLevel = 1;

    // http://philzimmermann.com/docs/human-oriented-base-32-encoding.txt
    static constexpr auto base32symbols = "ybndrfg8ejkmcpqxot1uwisza345h769";

    explicit Seed(const String &encoded)
    {
        uint64 data = 0;
        const String symbols(base32symbols);

        for (int i = 0; i < 8; i++)
        {
            data <<= 5;
            data |= symbols.find(encoded[i]);
        }

        const uint8 smolNumbers = (data >> 32) & 0xffffffff;
        this->seed = data & 0xffffffff;

        this->questGeneratorVersion = (smolNumbers & 0b11110000) >> 4;
        this->difficultyLevel = smolNumbers & 0b00001111;
    }

    String encode() const
    {
        // this is encoding only 40 bits out of 64, and they a are packed like this:
        // [ unused (24) | quest generator version (4) | difficulty level (4) | seed (32) ]
        const uint8 smolNumbers = this->questGeneratorVersion << 4 | this->difficultyLevel;
        uint64 data = ((uint64)smolNumbers << 32) | ((uint64)this->seed);

        String encoded(8, ' ');
        for (int i = 8; i --> 0 ;)
        {
            encoded[i] = base32symbols[data & 0x1f];
            data >>= 5;
        }

        return encoded;
    }
};
