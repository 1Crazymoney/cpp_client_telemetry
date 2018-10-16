#ifndef AriaDecoderV3_HPP
#define AriaDecoderV3_HPP

#include <vector>
#include <cinttypes>
#include <iostream>

#include "zlib.h"
#undef compress

class AriaDecoderV3
{
public:

    static void decode(std::vector<uint8_t> &in, std::vector<uint8_t> &out, bool compressed = true);

    static void ExpandVector(std::vector<uint8_t> &in, std::vector<uint8_t> &out);
    static void InflateVector(std::vector<uint8_t> &in, std::vector<uint8_t> &out);
};

#endif
