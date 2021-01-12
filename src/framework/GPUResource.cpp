#include <framework/GPUResource.h>

namespace frm
{
    template<>
    void GPUResource<VkBuffer>::map(void** data)
    {
        vmaMapMemory(m_allocator, m_allocation, data);
    }

    template<>
    void GPUResource<VkBuffer>::unmap()
    {
        vmaUnmapMemory(m_allocator, m_allocation);
    }

    template<>
    void GPUResource<VkBuffer>::destroy()
    {
        vmaDestroyBuffer(m_allocator, m_resource, m_allocation);
    }

    template<>
    void GPUResource<VkImage>::destroy()
    {
        vmaDestroyImage(m_allocator, m_resource, m_allocation);
    }
}
