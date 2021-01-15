#include "stb/stb_image.h"
#include <framework/Resource.h>

namespace frm
{
    bool Resource::loadBinary(const std::string& filepath, std::vector<uint8_t>& blob)
    {
        std::ifstream file(filepath, std::ios::binary);
        size_t size = 0;

        if (!file.is_open()) {
            return false;
        }

        file.seekg(0, std::ios::end);
        size = file.tellg();
        blob.resize(size);
        file.seekg(0, std::ios::beg);
        file.read((char*)blob.data(), size);
        file.close();

        return true;
    }

    bool Resource::loadImage(const std::string& filepath, ImageData& blob, int channelCount)
    {
        uint8_t* data = stbi_load(filepath.c_str(), &blob.width, &blob.height, &blob.channelCount, channelCount);
        size_t size = (size_t)blob.width * blob.height * blob.channelCount;

        if (data == nullptr) {
            return false;
        }

        blob.data.resize(size);
        std::memcpy(blob.data.data(), data, size);
        stbi_image_free(data);

        return true;
    }
}
