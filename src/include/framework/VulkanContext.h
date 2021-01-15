#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <framework/Common.h>
#include <framework/GPUResource.h>

#define VK_FAILED(x) ((x) != VK_SUCCESS)

namespace frm
{
    class VulkanContext
    {
    public:
        VulkanContext();
        ~VulkanContext();

        void initDevice(SDL_Window* window);
        void prepareNextSwapbuffer(uint32_t& nextSwapbufferIndex);
        void present(uint32_t swapbufferIndex);
        void queueSubmit(const VkSubmitInfo& submitInfo);
        void waitIdle();

        // Wrapper for vkCreateX functions
        void createBuffer(const VkBufferCreateInfo& createInfo, VmaMemoryUsage usage, BufferResourceRef& buffer);
        void createImage(const VkImageCreateInfo& createInfo, VmaMemoryUsage usage, ImageResourceRef& image);
        void createImageView(const VkImageViewCreateInfo& createInfo, VkImageView* imageView);
        void createCommandPool(uint32_t flags, VkCommandPool* cmdPool);
        void createCommandBuffer(VkCommandPool cmdPool, VkCommandBuffer* cmdBuffer);
        void createShaderModule(const std::vector<uint8_t>& shaderBlob, VkShaderModule* shaderModule);
        void createDescriptorLayout(const VkDescriptorSetLayoutCreateInfo& createInfo, VkDescriptorSetLayout* setLayout);
        void createPipelineLayout(const VkPipelineLayoutCreateInfo& createInfo, VkPipelineLayout* pipelineLayout);
        void createGraphicsPipeline(const VkGraphicsPipelineCreateInfo& createInfo, VkPipeline* pipeline);
        void createFramebuffer(const VkFramebufferCreateInfo& createInfo, VkFramebuffer* framebuffer);
        void createRenderPass(const VkRenderPassCreateInfo& createInfo, VkRenderPass* renderpass);
        void createDescriptorPool(const VkDescriptorPoolCreateInfo& createInfo, VkDescriptorPool* descriptorPool);
        void createSampler(const VkSamplerCreateInfo& createInfo, VkSampler* sampler);

        void allocDescriptorSet(VkDescriptorSetLayout layout, VkDescriptorPool descPool, VkDescriptorSet* set);

        void destroyBuffer(VkBuffer buffer);

        VkDevice getDevice() const { return m_device; }
        VkQueue getQueue() const { return m_deviceQueue; }
        uint32_t getQueueIndex() const { return m_deviceQueueIndex; }
        size_t getSwapbufferCount() const { return m_swapchainImages.size(); }
        VkImage getSwapbuffer(size_t idx) const { return m_swapchainImages[idx]; }
        VkImageView getSwapbufferView(size_t idx) const { return m_swapchainImgViews[idx]; }
        VkFormat getSwapchainFormat() const { return VK_FORMAT_B8G8R8A8_SRGB; }

    private:
        VkInstance m_instance;
        VkPhysicalDevice m_physicalDevice;
        VkPhysicalDeviceFeatures m_pdFeatures;
        VkSurfaceKHR m_surface;
        VkSurfaceCapabilitiesKHR m_surfaceCaps;
        VkDevice m_device;
        VmaAllocator m_allocator;
        VkQueue m_deviceQueue;
        uint32_t m_deviceQueueIndex;
        VkSwapchainKHR m_swapchain;
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImgViews;
        std::vector<VkImageMemoryBarrier> m_swapchainInitialLayoutBarriers;
        VkFence m_swapbufferAvailable;
        VkFence m_submitFence;
        bool m_initialized;

        // instance
        static const char* g_instanceLayers[1];
        static const char* g_instanceExtensions[2];

        // device
        static const char* g_deviceLayers[1];
        static const char* g_deviceExtensions[1];

        void init();
        void shutdown();
        void createSwapchain();
    };
}
