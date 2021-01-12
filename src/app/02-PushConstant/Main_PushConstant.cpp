#include <framework/App.h>
#include <framework/Resource.h>

struct PushConstantExample : public frm::App
{
    VkCommandPool pool;
    VkCommandBuffer cmdBuffer;
    VkRenderPass renderPass;
    VkShaderModule vsModule;
    VkShaderModule fsModule;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkFramebuffer> fb;

    struct MyConstants
    {
        glm::vec4 color;
        float size;
    };

    MyConstants constants{};
    glm::vec4 tempColor{};
    float time = 0.f;

    void onInit(frm::VulkanContext& context) override
    {
        size_t swapbufferCount = context.getSwapbufferCount();

        initResource();

        context.createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &pool);
        context.createCommandBuffer(pool, &cmdBuffer);

        // Create render pass
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

        initPipeline();
        initFramebuffer();
    }

    void initResource()
    {
        // Initialize resources
        frm::VulkanContext& context = this->getContext();
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

    void initFramebuffer()
    {
        frm::VulkanContext& context = this->getContext();

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

    void initPipeline()
    {
        // Create our first pipeline
        frm::VulkanContext& context = this->getContext();
        VkPushConstantRange pconstRange = {};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        VkPipelineShaderStageCreateInfo shaderStages[2] = {};
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

        pconstRange.offset = 0;
        pconstRange.size = sizeof(MyConstants);
        pconstRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
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

        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

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

    void onUpdate(frm::VulkanContext& context, double dt) override
    {
        // generate rainbow color
        float s = std::sin(time * 1.13f) * 0.5f;
        float s1 = std::sin(time * 1.23f) * 0.5f;
        float s2 = std::sin(time * 1.33f) * 0.5f;
        
        tempColor.r += s;
        tempColor.g += s1;
        tempColor.b += s2;

        constants.size = std::abs(s) * 2.f;
        constants.color = tempColor / 255.f;

        time += static_cast<float>(dt);
    }

    void onRender(frm::VulkanContext& context, double dt) override
    {
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

        getClientSizeRect(rpBegin.renderArea);

        vkResetCommandBuffer(cmdBuffer, 0); // we use this command buffer twice, so we need to reset it

        // record command buffer
        vkBeginCommandBuffer(cmdBuffer, &cmdBegin);
        vkCmdBeginRenderPass(cmdBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MyConstants), &constants);
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0); // draw triangle to the framebuffer
        vkCmdEndRenderPass(cmdBuffer);
        vkEndCommandBuffer(cmdBuffer);

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        // execute the command buffer synchronously
        context.queueSubmit(submitInfo);
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
        vkDestroyCommandPool(device, pool, nullptr);
    }
};

int main()
{
    return frm::App::run<PushConstantExample>(640, 480);
}
