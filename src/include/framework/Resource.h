#pragma once

#include <framework/Common.h>

namespace frm
{
    struct Resource
    {
        static bool loadBinary(const std::string& filepath, std::vector<uint8_t>& blob);
    };
}
