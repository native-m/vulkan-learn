#include <framework/App.h>
#include <framework/Resource.h>
#include <framework/GPUResource.h>
#include <framework/ShapeGen.h>

struct TransformExample : public frm::App
{
    frm::ImageResourceRef image; // ImageResourceRef & BufferResourceRef are wrapper for VkImage and VkBuffer, object deletion is done automatically :)
    frm::BufferResourceRef vertexBuffer; 
    frm::BufferResourceRef indexBuffer;
    VkCommandPool cmdPool;
    VkCommandBuffer renderCmd;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> fb;
    VkShaderModule vsModule;
    VkShaderModule fsModule;
    VkDescriptorSetLayout descSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descSet;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkImageView imageView;
    VkSampler sampler;
    VkRect2D viewRect;
    float aspect = 0.f;
    float time = 0.f;

    struct MyConstants
    {
        glm::mat4 wvpMatrix{};
    };

    MyConstants constants;

    void onInit(frm::VulkanContext& context) override
    {
        context.createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &cmdPool);
        
        initTexture(context);
        initSampler(context);
        initTransformation();
        initBuffer(context);
        initRenderPass(context);
        initFramebuffer(context);
        loadResources(context);
        initPipeline(context);
        initDescriptor(context);
        recordCmd(context);
    }

    void initTexture(frm::VulkanContext& context)
    {
        frm::BufferResourceRef stagingBuffer;
        frm::ImageData imageData;
        VkCommandBuffer copyCmd;
        VkBufferCreateInfo stagingBufInfo{};
        VkImageCreateInfo imageInfo{};
        VkImageViewCreateInfo imageViewInfo{};

        if (!frm::Resource::loadImage("shaderboi_fish.png", imageData, 4)) {
            throw std::runtime_error("Cannot load image");
        }
        
        // Create a staging buffer as a texture transfer source
        stagingBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufInfo.size = imageData.data.size();
        stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        context.createBuffer(stagingBufInfo, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);

        uint8_t* pixelData = nullptr;
        stagingBuffer->map(&pixelData);
        std::memcpy(pixelData, imageData.data.data(), imageData.data.size()); // copy image data to staging buffer
        stagingBuffer->unmap();

        // Create actual image on the GPU
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.extent.width = imageData.width;
        imageInfo.extent.height = imageData.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        context.createImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, image);

        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = image->get();
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = imageInfo.format;
        imageViewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        context.createImageView(imageViewInfo, &imageView);

        // create copy command
        context.createCommandBuffer(cmdPool, &copyCmd);

        VkImageMemoryBarrier imageTransferBarrier{};
        imageTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageTransferBarrier.srcAccessMask = 0;
        imageTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageTransferBarrier.srcQueueFamilyIndex = context.getQueueIndex();
        imageTransferBarrier.dstQueueFamilyIndex = context.getQueueIndex();
        imageTransferBarrier.image = image->get();
        imageTransferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageTransferBarrier.subresourceRange.baseMipLevel = 0;
        imageTransferBarrier.subresourceRange.levelCount = 1;
        imageTransferBarrier.subresourceRange.baseArrayLayer = 0;
        imageTransferBarrier.subresourceRange.layerCount = 1;

        VkImageMemoryBarrier imageFragReadBarrier{};
        imageFragReadBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageFragReadBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageFragReadBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageFragReadBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageFragReadBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageFragReadBarrier.srcQueueFamilyIndex = context.getQueueIndex();
        imageFragReadBarrier.dstQueueFamilyIndex = context.getQueueIndex();
        imageFragReadBarrier.image = image->get();
        imageFragReadBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageFragReadBarrier.subresourceRange.baseMipLevel = 0;
        imageFragReadBarrier.subresourceRange.levelCount = 1;
        imageFragReadBarrier.subresourceRange.baseArrayLayer = 0;
        imageFragReadBarrier.subresourceRange.layerCount = 1;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkBufferImageCopy imageCopy{};
        imageCopy.bufferOffset = 0;
        imageCopy.bufferRowLength = imageData.width;
        imageCopy.bufferImageHeight = imageData.height;
        imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageCopy.imageSubresource.mipLevel = 0;
        imageCopy.imageSubresource.baseArrayLayer = 0;
        imageCopy.imageSubresource.layerCount = 1;
        imageCopy.imageOffset.x = 0;
        imageCopy.imageOffset.y = 0;
        imageCopy.imageOffset.z = 0;
        imageCopy.imageExtent.width = imageData.width;
        imageCopy.imageExtent.height = imageData.height;
        imageCopy.imageExtent.depth = 1;

        vkBeginCommandBuffer(copyCmd, &beginInfo);
        vkCmdPipelineBarrier(copyCmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imageTransferBarrier);
        vkCmdCopyBufferToImage(copyCmd, stagingBuffer->get(), image->get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        vkCmdPipelineBarrier(copyCmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imageFragReadBarrier);
        vkEndCommandBuffer(copyCmd);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &copyCmd;

        context.queueSubmit(submit);

        vkFreeCommandBuffers(context.getDevice(), cmdPool, 1, &copyCmd);
    }

    void initSampler(frm::VulkanContext& context)
    {
        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 1;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        context.createSampler(samplerInfo, &sampler);
    }

    void initTransformation()
    {
        // viewport size
        getClientSizeRect(viewRect);

        // projection aspect ratio
        aspect = static_cast<float>(viewRect.extent.width) / static_cast<float>(viewRect.extent.height);
    }

    void initBuffer(frm::VulkanContext& context)
    {
        frm::BufferResourceRef stagingBuffer;
        frm::VertexPosTex* vertices = nullptr;
        uint32_t* indices = nullptr;
        size_t numVertices;
        size_t numIndices;
        VkBufferCreateInfo bufferInfo{};
        VkCommandBuffer copyCmd;

        numVertices = frm::ShapeGen::makePlane(0.5f, indices, vertices, numIndices); // make a flat plane

        // create a temporary command buffer to copy buffer
        context.createCommandBuffer(cmdPool, &copyCmd);

        // create staging buffer
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(frm::VertexPosTex) * numVertices;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        context.createBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);

        // create vertex buffer with the same size as the staging buffer
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        context.createBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer);

        // create index buffer
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.size = sizeof(uint32_t) * numIndices;
        context.createBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, indexBuffer);

        // copy vertex data to staging buffer
        frm::VertexPosTex* mapped = nullptr;
        stagingBuffer->map(&mapped);
        std::memcpy(mapped, vertices, sizeof(frm::VertexPosTex) * numVertices);
        stagingBuffer->unmap();

        // copy vertex data which currently inside the staging buffer to the vertex buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkBufferCopy region{};
        region.size = sizeof(frm::VertexPosTex) * numVertices;

        vkBeginCommandBuffer(copyCmd, &beginInfo);
        vkCmdCopyBuffer(copyCmd, stagingBuffer->get(), vertexBuffer->get(), 1, &region);
        vkEndCommandBuffer(copyCmd);

        // submit our copy command to GPU!!
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &copyCmd;

        context.queueSubmit(submit);

        // Now do the same thing with index data

        // copy index data to staging buffer
        uint32_t* mappedIndices;
        stagingBuffer->map(&mappedIndices);
        std::memcpy(mappedIndices, indices, sizeof(uint32_t) * numIndices);
        stagingBuffer->unmap();

        // copy index data which currently inside the staging buffer to the vertex buffer
        region.size = sizeof(uint32_t) * numIndices;

        vkResetCommandBuffer(copyCmd, 0);
        vkBeginCommandBuffer(copyCmd, &beginInfo);
        vkCmdCopyBuffer(copyCmd, stagingBuffer->get(), indexBuffer->get(), 1, &region);
        vkEndCommandBuffer(copyCmd);

        context.queueSubmit(submit);

        vkFreeCommandBuffers(context.getDevice(), cmdPool, 1, &copyCmd);

        delete indices;
        delete vertices;
    }

    void initRenderPass(frm::VulkanContext& context)
    {
        VkRenderPassCreateInfo renderPassInfo{};
        VkAttachmentDescription attachment{};
        VkAttachmentReference attRef{};
        VkSubpassDescription subpass{};

        attachment.format = context.getSwapchainFormat();
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        attRef.attachment = 0;
        attRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount = 0;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &attRef;

        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.dependencyCount = 0;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pAttachments = &attachment;
        renderPassInfo.pSubpasses = &subpass;

        context.createRenderPass(renderPassInfo, &renderPass);
    }

    void initFramebuffer(frm::VulkanContext& context)
    {
        // Create framebuffer for each swapbuffer
        for (size_t i = 0; i < context.getSwapbufferCount(); i++) {
            VkFramebufferCreateInfo fbInfo{};
            VkImageView imgView = context.getSwapbufferView(i);
            VkFramebuffer framebuffer;

            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = renderPass;
            fbInfo.attachmentCount = 1;
            fbInfo.pAttachments = &imgView;
            fbInfo.width = viewRect.extent.width;
            fbInfo.height = viewRect.extent.height;
            fbInfo.layers = 1;

            context.createFramebuffer(fbInfo, &framebuffer);
            fb.push_back(framebuffer);
        }
    }

    void loadResources(frm::VulkanContext& context)
    {
        // Initialize resources
        std::vector<uint8_t> vsBlob;
        std::vector<uint8_t> fsBlob;

        if (!frm::Resource::loadBinary("VertexShader.vs.spv", vsBlob)) {
            throw std::runtime_error("Cannot load vertex shader");
        }

        if (!frm::Resource::loadBinary("FragShader.fs.spv", fsBlob)) {
            throw std::runtime_error("Cannot load fragment shader");
        }

        context.createShaderModule(vsBlob, &vsModule);
        context.createShaderModule(fsBlob, &fsModule);
    }

    void initPipeline(frm::VulkanContext& context)
    {
        VkPushConstantRange pconstRange{};
        VkDescriptorSetLayoutBinding texBinding{}; // our texture binding information
        VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        VkPipelineShaderStageCreateInfo shaderStages[2] = {};
        VkVertexInputBindingDescription inputBinding{};
        VkVertexInputAttributeDescription inputAttribs[2] = {};
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        VkPipelineInputAssemblyStateCreateInfo inputAsm{};
        VkViewport viewport{};
        VkPipelineViewportStateCreateInfo viewportState{};
        VkPipelineRasterizationStateCreateInfo rasterState{};
        VkPipelineMultisampleStateCreateInfo multisample{};
        VkPipelineColorBlendStateCreateInfo colorBlend{};
        VkPipelineColorBlendAttachmentState blendAtt{};

        texBinding.binding = 0;
        texBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texBinding.descriptorCount = 1;
        texBinding.pImmutableSamplers = nullptr;
        texBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setLayoutInfo.bindingCount = 1;
        setLayoutInfo.pBindings = &texBinding;

        context.createDescriptorLayout(setLayoutInfo, &descSetLayout);

        pconstRange.offset = 0;
        pconstRange.size = sizeof(MyConstants);
        pconstRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pconstRange;

        context.createPipelineLayout(pipelineLayoutInfo, &pipelineLayout);

        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vsModule;
        shaderStages[0].pName = "main";

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fsModule;
        shaderStages[1].pName = "main";

        inputBinding.binding = 0;
        inputBinding.stride = sizeof(frm::VertexPosTex);

        inputAttribs[0].location = 0;
        inputAttribs[0].binding = 0;
        inputAttribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        inputAttribs[0].offset = 0;

        inputAttribs[1].location = 1;
        inputAttribs[1].binding = 0;
        inputAttribs[1].format = VK_FORMAT_R32G32_SFLOAT;
        inputAttribs[1].offset = 12;

        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &inputBinding;
        vertexInput.vertexAttributeDescriptionCount = 2;
        vertexInput.pVertexAttributeDescriptions = inputAttribs;

        inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAsm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(viewRect.extent.width);
        viewport.height = static_cast<float>(viewRect.extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &viewRect;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;

        rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterState.lineWidth = 1.0f;
        rasterState.cullMode = VK_CULL_MODE_NONE;
        rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        blendAtt.blendEnable = VK_FALSE;
        blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blendAtt;

        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAsm;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterState;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;

        context.createGraphicsPipeline(pipelineInfo, &pipeline);
    }

    void initDescriptor(frm::VulkanContext& context)
    {
        VkDescriptorPoolSize texBindingSize{};
        VkDescriptorPoolCreateInfo descPoolInfo{};
        VkDescriptorImageInfo imageDescInfo{};
        VkWriteDescriptorSet write{};

        texBindingSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texBindingSize.descriptorCount = 1;

        descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descPoolInfo.maxSets = 1;
        descPoolInfo.poolSizeCount = 1;
        descPoolInfo.pPoolSizes = &texBindingSize;

        context.createDescriptorPool(descPoolInfo, &descriptorPool);
        context.allocDescriptorSet(descSetLayout, descriptorPool, &descSet);

        imageDescInfo.sampler = sampler;
        imageDescInfo.imageView = imageView;
        imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descSet;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageDescInfo;

        vkUpdateDescriptorSets(context.getDevice(), 1, &write, 0, nullptr);
    }

    void recordCmd(frm::VulkanContext& context)
    {
        context.createCommandBuffer(cmdPool, &renderCmd);
    }

    void onUpdate(frm::VulkanContext& context, double dt) override
    {
        constants.wvpMatrix = glm::perspectiveLH(glm::radians(45.0f), aspect, 0.01f, 500.f) *
            glm::lookAtLH(glm::vec3(0.f, 0.f, -2.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f)) *
            glm::rotate(glm::identity<glm::mat4>(), time, glm::vec3(0.f, 1.f, 0.f));

        time += static_cast<float>(dt);
    }

    void onRender(frm::VulkanContext& context, double dt) override
    {
        VkBuffer buf = vertexBuffer->get();
        VkDeviceSize ofs = 0;
        VkCommandBufferBeginInfo cmdBegin{};
        VkRenderPassBeginInfo rpBegin{};
        VkClearValue clearValue{};
        VkSubmitInfo submitInfo{};

        clearValue.color.float32[0] = 0.0f;
        clearValue.color.float32[1] = 0.0f;
        clearValue.color.float32[2] = 0.0f;
        clearValue.color.float32[3] = 0.0f;

        cmdBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass = renderPass;
        rpBegin.framebuffer = fb[getCurrentSwapbuffer()];
        rpBegin.clearValueCount = 1;
        rpBegin.pClearValues = &clearValue;
        rpBegin.renderArea.offset.x = 0;
        rpBegin.renderArea.offset.y = 0;
        rpBegin.renderArea.extent.width = viewRect.extent.width;
        rpBegin.renderArea.extent.height = viewRect.extent.height;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkResetCommandBuffer(renderCmd, 0);
        vkBeginCommandBuffer(renderCmd, &beginInfo);
        vkCmdBeginRenderPass(renderCmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(renderCmd, 0, 1, &buf, &ofs); // bind vertex buffer
        vkCmdBindIndexBuffer(renderCmd, indexBuffer->get(), 0, VK_INDEX_TYPE_UINT32); // bind index buffer
        vkCmdPushConstants(renderCmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MyConstants), &constants); // set push constant values
        vkCmdBindDescriptorSets(renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr); // SET descriptor set to pipeline
        vkCmdDrawIndexed(renderCmd, 6, 1, 0, 0, 0); // draw triangle to the framebuffer
        vkCmdEndRenderPass(renderCmd);
        vkEndCommandBuffer(renderCmd);

        // submit our render command to GPU!!
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &renderCmd;

        context.queueSubmit(submit);
    }

    void onDestroy(frm::VulkanContext& context) override
    {
        VkDevice device = context.getDevice();

        vkDestroySampler(device, sampler, nullptr);
        vkDestroyImageView(device, imageView, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
        vkDestroyShaderModule(device, vsModule, nullptr);
        vkDestroyShaderModule(device, fsModule, nullptr);

        for (auto framebuffer : fb) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyCommandPool(device, cmdPool, nullptr);
    }
};

int main()
{
    return frm::App::run<TransformExample>(640, 480);
}
