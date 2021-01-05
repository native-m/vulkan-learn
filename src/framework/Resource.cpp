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
}
