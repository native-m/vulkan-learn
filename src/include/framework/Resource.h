#pragma once

#include <framework/Common.h>

namespace frm
{
    struct ImageData
    {
        int width = 0;
        int height = 0;
        int channelCount = 0;
        std::vector<uint8_t> data;
    };

    struct Resource
    {
        static bool loadBinary(const std::string& filepath, std::vector<uint8_t>& blob);
        static bool loadImage(const std::string& filepath, ImageData& data, int channelCount);
    };
}
