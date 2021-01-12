#include <framework/App.h>
#include <framework/Resource.h>
#include <framework/GPUResource.h>
#include <framework/ShapeGen.h>

struct IndexBufferExample : public frm::App
{
    frm::BufferResourceRef stagingBuffer; // BufferResourceRef is a wrapper for VkBuffer, object deletion is done automatically :)
    frm::BufferResourceRef vertexBuffer;
    frm::BufferResourceRef indexBuffer;
    VkCommandPool cmdPool;
    VkCommandBuffer renderCmd;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> fb;
    VkShaderModule vsModule;
    VkShaderModule fsModule;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    void onInit(frm::VulkanContext& context) override
    {
        context.createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &cmdPool);
        
        initBuffer(context);
        initRenderPass(context);
        initFramebuffer(context);
        loadResources(context);
        initPipeline(context);
        recordCmd(context);
    }

    void initBuffer(frm::VulkanContext& context)
    {
        frm::VertexPosCol* vertices = nullptr;
        uint32_t* indices = nullptr;
        size_t numVertices;
        size_t numIndices;
        VkBufferCreateInfo bufferInfo{};
        VkCommandBuffer copyCmd;

        numVertices = frm::ShapeGen::makeColorPlane(0.5f, indices, vertices, numIndices); // make a flat plane

        // create a temporary command buffer to copy buffer
        context.createCommandBuffer(cmdPool, &copyCmd);

        // create staging buffer
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(frm::VertexPosCol) * numVertices;
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
        frm::VertexPosCol* mapped = nullptr;
        stagingBuffer->map(&mapped);
        std::memcpy(mapped, vertices, sizeof(frm::VertexPosCol) * numVertices);
        stagingBuffer->unmap();

        // copy vertex data which currently inside the staging buffer to the vertex buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkBufferCopy region{};
        region.size = sizeof(frm::VertexPosCol) * numVertices;

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
            VkRect2D imgSize{};
            VkFramebuffer framebuffer;

            getClientSizeRect(imgSize); // get our window client size

            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = renderPass;
            fbInfo.attachmentCount = 1;
            fbInfo.pAttachments = &imgView;
            fbInfo.width = imgSize.extent.width;
            fbInfo.height = imgSize.extent.height;
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
        VkPushConstantRange pconstRange = {};
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
        VkRect2D clientRect{};

        getClientSizeRect(clientRect);

        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

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
        inputBinding.stride = sizeof(frm::VertexPosCol);

        inputAttribs[0].location = 0;
        inputAttribs[0].binding = 0;
        inputAttribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        inputAttribs[0].offset = 0;

        inputAttribs[1].location = 1;
        inputAttribs[1].binding = 0;
        inputAttribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
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
        viewport.width = static_cast<float>(clientRect.extent.width);
        viewport.height = static_cast<float>(clientRect.extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &clientRect;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;

        rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterState.lineWidth = 1.0f;
        rasterState.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;

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

    void recordCmd(frm::VulkanContext& context)
    {
        VkBuffer buf = vertexBuffer->get();
        VkDeviceSize ofs = 0;

        context.createCommandBuffer(cmdPool, &renderCmd);

        for (uint32_t i = 0; i < context.getSwapbufferCount(); i++) {
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
            rpBegin.framebuffer = fb[i];
            rpBegin.clearValueCount = 1;
            rpBegin.pClearValues = &clearValue;

            getClientSizeRect(rpBegin.renderArea);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            vkResetCommandBuffer(renderCmd, 0);
            vkBeginCommandBuffer(renderCmd, &beginInfo);
            vkCmdBeginRenderPass(renderCmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindVertexBuffers(renderCmd, 0, 1, &buf, &ofs); // bind our vertex buffer
            vkCmdBindIndexBuffer(renderCmd, indexBuffer->get(), 0, VK_INDEX_TYPE_UINT32);
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
    }

    void onUpdate(frm::VulkanContext& context, double dt) override
    {

    }

    void onRender(frm::VulkanContext& context, double dt) override
    {
        
    }

    void onDestroy(frm::VulkanContext& context) override
    {
        VkDevice device = context.getDevice();

        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
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
    return frm::App::run<IndexBufferExample>(640, 480);
}
