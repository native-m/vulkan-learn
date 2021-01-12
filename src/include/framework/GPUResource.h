#pragma once

#include <vulkan/vulkan.h>
#include <framework/Common.h>

namespace frm
{
    template<class T>
    class GPUResource
    {
    public:
        GPUResource(VmaAllocator allocator, T resource, VmaAllocation allocation) :
            m_allocator(allocator),
            m_resource(resource),
            m_allocation(allocation)
        {
        }

        ~GPUResource()
        {
            destroy();
        }

        void map(void** data);
        void unmap();

        template<class T>
        void map(T** data)
        {
            map(reinterpret_cast<void**>(data));
        }

        T get() const
        {
            return m_resource;
        }

    private:
        VmaAllocator m_allocator;
        T m_resource;
        VmaAllocation m_allocation;
        
        void destroy();
    };

    using BufferResource = GPUResource<VkBuffer>;
    using ImageResource = GPUResource<VkImage>;
    using BufferResourceRef = std::shared_ptr<BufferResource>;
    using ImageResourceRef = std::shared_ptr<ImageResource>;
}
