#include <framework/VulkanContext.h>

namespace frm
{
    VulkanContext::VulkanContext() :
        m_instance(nullptr),
        m_physicalDevice(nullptr),
        m_pdFeatures(),
        m_surface(nullptr),
        m_device(nullptr),
        m_deviceQueue(nullptr),
        m_deviceQueueIndex(0),
        m_swapchain(nullptr),
        m_swapbufferAvailable(nullptr),
        m_submitFence(nullptr),
        m_initialized(false)
    {
        init();
    }

    VulkanContext::~VulkanContext()
    {
        shutdown();
    }

    void VulkanContext::initDevice(SDL_Window* window)
    {
        static const float queuePriority = 1.0f;
        uint32_t queueFamilyCount;
        std::vector<VkQueueFamilyProperties> queueFamilyProps;
        uint32_t currentQueueIndex = 0;
        uint32_t graphicsQueueIndex = -1;
        std::vector<VkLayerProperties> layerExts;
        std::vector<VkExtensionProperties> deviceExts;
        VkDeviceQueueCreateInfo queueCreateInfo{};
        VkDeviceCreateInfo deviceInfo{};
        VkFenceCreateInfo fenceInfo{};

        if (m_initialized) {
            return;
        }

        // create surface
        if (!SDL_Vulkan_CreateSurface(window, m_instance, &m_surface)) {
            throw std::runtime_error("Cannot create surface");
        }

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &m_surfaceCaps);

        // query queue families
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
        queueFamilyProps.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilyProps.data());

        // find graphics queue
        for (auto& qf : queueFamilyProps) {
            VkBool32 presentQueueSupported;

            vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, currentQueueIndex, m_surface, &presentQueueSupported);

            if ((qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentQueueSupported) {
                graphicsQueueIndex = currentQueueIndex;
                break;
            }

            currentQueueIndex++;
        }

        if (graphicsQueueIndex == -1) {
            throw std::runtime_error("No graphics queue found");
        }

        m_deviceQueueIndex = graphicsQueueIndex;

        // prepare queue create info
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = m_deviceQueueIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // create device
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueCreateInfo; // the queue we want to create
        deviceInfo.enabledLayerCount = GET_ARRAY_SIZE(g_deviceLayers);
        deviceInfo.ppEnabledLayerNames = g_deviceLayers;
        deviceInfo.enabledExtensionCount = GET_ARRAY_SIZE(g_deviceExtensions);
        deviceInfo.ppEnabledExtensionNames = g_deviceExtensions;
        deviceInfo.pEnabledFeatures = &m_pdFeatures;

        if (VK_FAILED(vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device))) {
            throw std::runtime_error("Cannot create device");
        }

        vkGetDeviceQueue(m_device, graphicsQueueIndex, 0, &m_deviceQueue);

        createSwapchain();

        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        if (VK_FAILED(vkCreateFence(m_device, &fenceInfo, nullptr, &m_submitFence))) {
            throw std::runtime_error("Cannot create queue submit fence");
        }

        m_initialized = true;
    }

    void VulkanContext::prepareNextSwapbuffer(uint32_t& nextSwapbufferIndex)
    {
        vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, VK_NULL_HANDLE, m_swapbufferAvailable, &nextSwapbufferIndex);
        vkWaitForFences(m_device, 1, &m_swapbufferAvailable, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_swapbufferAvailable);
    }

    void VulkanContext::present(uint32_t swapbufferIndex)
    {
        VkPresentInfoKHR presentInfo = { };

        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.pImageIndices = &swapbufferIndex;

        if (VK_FAILED(vkQueuePresentKHR(m_deviceQueue, &presentInfo))) {
            throw std::runtime_error("Swapbuffer presentation failed");
        }
    }

    void VulkanContext::queueSubmit(const VkSubmitInfo& submitInfo)
    {
        // Synchronized queue submission
        if (VK_FAILED(vkQueueSubmit(m_deviceQueue, 1, &submitInfo, m_submitFence))) {
            throw std::runtime_error("Queue submission failed");
        }

        // Wait the submission to be done
        while (vkWaitForFences(m_device, 1, &m_submitFence, VK_TRUE, UINT64_MAX) == VK_TIMEOUT);
        vkResetFences(m_device, 1, &m_submitFence);
    }

    void VulkanContext::waitIdle()
    {
        vkDeviceWaitIdle(m_device);
    }

    void VulkanContext::createCommandPool(uint32_t flags, VkCommandPool* cmdPool)
    {
        VkCommandPoolCreateInfo cmdPoolInfo{};

        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = static_cast<VkCommandPoolCreateFlags>(flags);
        cmdPoolInfo.queueFamilyIndex = m_deviceQueueIndex;

        if (VK_FAILED(vkCreateCommandPool(m_device, &cmdPoolInfo, nullptr, cmdPool))) {
            throw std::runtime_error("Cannot create command pool");
        }
    }

    void VulkanContext::createCommandBuffer(VkCommandPool cmdPool, VkCommandBuffer* cmdBuffer)
    {
        VkCommandBufferAllocateInfo cmdBufferInfo{};

        cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferInfo.commandPool = cmdPool;
        cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferInfo.commandBufferCount = 1;

        if (VK_FAILED(vkAllocateCommandBuffers(m_device, &cmdBufferInfo, cmdBuffer))) {
            throw std::runtime_error("Cannot create command buffer");
        }
    }

    void VulkanContext::createShaderModule(const std::vector<uint8_t>& shaderBlob, VkShaderModule* shaderModule)
    {
        VkShaderModuleCreateInfo moduleInfo{};

        moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.codeSize = shaderBlob.size();
        moduleInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBlob.data());

        if (VK_FAILED(vkCreateShaderModule(m_device, &moduleInfo, nullptr, shaderModule))) {
            throw std::runtime_error("Cannot create shader module");
        }
    }

    void VulkanContext::createPipelineLayout(const VkPipelineLayoutCreateInfo& createInfo, VkPipelineLayout* pipelineLayout)
    {
        if (VK_FAILED(vkCreatePipelineLayout(m_device, &createInfo, nullptr, pipelineLayout))) {
            throw std::runtime_error("Cannot create pipeline layout");
        }
    }

    void VulkanContext::createGraphicsPipeline(const VkGraphicsPipelineCreateInfo& createInfo, VkPipeline* pipeline)
    {
        if (VK_FAILED(vkCreateGraphicsPipelines(m_device, nullptr, 1, &createInfo, nullptr, pipeline))) {
            throw std::runtime_error("Cannot create graphics pipeline");
        }
    }

    void VulkanContext::createFramebuffer(const VkFramebufferCreateInfo& createInfo, VkFramebuffer* framebuffer)
    {
        if (VK_FAILED(vkCreateFramebuffer(m_device, &createInfo, nullptr, framebuffer))) {
            throw std::runtime_error("Cannot create framebuffer");
        }
    }

    void VulkanContext::createRenderPass(const VkRenderPassCreateInfo& createInfo, VkRenderPass* renderpass)
    {
        if (VK_FAILED(vkCreateRenderPass(m_device, &createInfo, nullptr, renderpass))) {
            throw std::runtime_error("Cannot create render pass");
        }
    }

    void VulkanContext::init()
    {
        VkInstanceCreateInfo instanceInfo{};
        VkApplicationInfo appInfo{};
        uint32_t physicalDeviceCount;
        std::vector<VkPhysicalDevice> physicalDevices;
        bool discreteGpuFound = false;

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "vulkan-learn";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "vulkan-learn";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 0);

        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.ppEnabledLayerNames = g_instanceLayers;
        instanceInfo.enabledLayerCount = GET_ARRAY_SIZE(g_instanceLayers);
        instanceInfo.ppEnabledExtensionNames = g_instanceExtensions;
        instanceInfo.enabledExtensionCount = GET_ARRAY_SIZE(g_instanceExtensions);
        
        if (VK_FAILED(vkCreateInstance(&instanceInfo, nullptr, &m_instance))) {
            throw std::runtime_error("Cannot create instance");
        }

        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
        physicalDevices.resize(physicalDeviceCount);
        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

        // find discrete gpu (discrete AMD, NVIDIA, etc)
        for (auto p : physicalDevices) {
            VkPhysicalDeviceProperties prop{};

            vkGetPhysicalDeviceProperties(p, &prop);

            if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                m_physicalDevice = p;
                discreteGpuFound = true;

                break;
            }
        }

        if (!discreteGpuFound) {
            m_physicalDevice = physicalDevices[0]; // select default gpu
        }

        vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_pdFeatures);
    }

    void VulkanContext::shutdown()
    {
        vkDeviceWaitIdle(m_device);

        if (m_submitFence != nullptr) {
            vkDestroyFence(m_device, m_submitFence, nullptr);
        }

        if (m_swapbufferAvailable != nullptr) {
            vkDestroyFence(m_device, m_swapbufferAvailable, nullptr);
        }

        for (auto swapchainImgView : m_swapchainImgViews) {
            vkDestroyImageView(m_device, swapchainImgView, nullptr);
        }

        if (m_swapchain != nullptr) {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        }

        if (m_device != nullptr) {
            vkDestroyDevice(m_device, nullptr);
        }

        if (m_surface != nullptr) {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }

        if (m_instance != nullptr) {
            vkDestroyInstance(m_instance, nullptr);
        }
    }

    void VulkanContext::createSwapchain()
    {
        VkCommandPool cmdPool;
        VkCommandBuffer cmdBuffer;
        uint32_t swapchainImageCount;
        VkSwapchainCreateInfoKHR swapchainInfo{};
        VkFenceCreateInfo swapchainFenceInfo{};
        VkCommandPoolCreateInfo cmdPoolInfo{};
        VkCommandBufferAllocateInfo cmdBufferInfo{};
        VkCommandBufferBeginInfo cmdBufferBegin{};
        VkSubmitInfo submitInfo{};

        // create swapchain
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = m_surface;
        swapchainInfo.minImageCount = 2;
        swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
        swapchainInfo.imageExtent.width = m_surfaceCaps.maxImageExtent.width;
        swapchainInfo.imageExtent.height = m_surfaceCaps.maxImageExtent.height;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.preTransform = m_surfaceCaps.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

        if (VK_FAILED(vkCreateSwapchainKHR(m_device, &swapchainInfo, nullptr, &m_swapchain))) {
            throw std::runtime_error("Cannot create swapchain");
        }

        // fetch swapchain images and create views
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, nullptr);
        m_swapchainImages.resize(swapchainImageCount);
        m_swapchainImgViews.resize(swapchainImageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, m_swapchainImages.data());

        for (uint32_t i = 0; i < swapchainImageCount; i++) {
            VkImageViewCreateInfo imgViewInfo{};

            imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imgViewInfo.image = m_swapchainImages[i];
            imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imgViewInfo.format = swapchainInfo.imageFormat;
            imgViewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
            imgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imgViewInfo.subresourceRange.baseMipLevel = 0;
            imgViewInfo.subresourceRange.levelCount = 1;
            imgViewInfo.subresourceRange.baseArrayLayer = 0;
            imgViewInfo.subresourceRange.layerCount = 1;

            if (VK_FAILED(vkCreateImageView(m_device, &imgViewInfo, nullptr, &m_swapchainImgViews[i]))) {
                throw std::runtime_error("Cannot create swapchain image views");
            }
        }

        swapchainFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        if (VK_FAILED(vkCreateFence(m_device, &swapchainFenceInfo, nullptr, &m_swapbufferAvailable))) {
            throw std::runtime_error("Cannot create swapchain fence");
        }

        // ------------------------- OPTIONAL SECTION -------------------------
        // Pre-determine swapchain layout to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR

        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = m_deviceQueueIndex;

        if (VK_FAILED(vkCreateCommandPool(m_device, &cmdPoolInfo, nullptr, &cmdPool))) {
            throw std::runtime_error("Cannot create command pool for swapchain initial layout transition");
        }

        cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferInfo.commandPool = cmdPool;
        cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferInfo.commandBufferCount = 1;

        if (VK_FAILED(vkAllocateCommandBuffers(m_device, &cmdBufferInfo, &cmdBuffer))) {
            throw std::runtime_error("Cannot create command buffer for swapchain initial layout transition");
        }

        cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        for (auto swapbuffer : m_swapchainImages) {
            VkImageMemoryBarrier barrier = { };
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = m_deviceQueueIndex;
            barrier.dstQueueFamilyIndex = m_deviceQueueIndex;
            barrier.image = swapbuffer;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            m_swapchainInitialLayoutBarriers.push_back(barrier);
        }

        vkBeginCommandBuffer(cmdBuffer, &cmdBufferBegin);
        vkCmdPipelineBarrier(cmdBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            static_cast<uint32_t>(m_swapchainInitialLayoutBarriers.size()),
            m_swapchainInitialLayoutBarriers.data());
        vkEndCommandBuffer(cmdBuffer);

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        if (VK_FAILED(vkQueueSubmit(m_deviceQueue, 1, &submitInfo, nullptr))) {
            throw std::runtime_error("Cannot submit command buffer for swapchain initial layout transition");
        }

        vkDeviceWaitIdle(m_device);

        vkDestroyCommandPool(m_device, cmdPool, nullptr);
    }

    const char* VulkanContext::g_instanceLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    const char* VulkanContext::g_instanceExtensions[] = {
        "VK_KHR_surface",
#ifdef WIN32
        "VK_KHR_win32_surface"
#elif defined(__linux__)
        "VK_KHR_xcb_surface"
#endif
    };

    const char* VulkanContext::g_deviceLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    const char* VulkanContext::g_deviceExtensions[] = {
        "VK_KHR_swapchain"
    };
}
