#pragma once

#include "Vertex.h"
#include "DepthImage.h"
#include "VulkanUtils.h"
#include <vector>

class Pipeline
{
private:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo;
    VkViewport viewPort;
    VkRect2D scissor;
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo;
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = DepthImage::getDepthStencilStateCreateInfoOpaque();
    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
    VkPushConstantRange pushConstantRange;

    VkVertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

    bool wasInitialized = false;
    bool wasCreated = false;

public:
    Pipeline()
    {
    }

    void ini(VkShaderModule vertexShader, VkShaderModule fragmentShader, uint32_t width, uint32_t height)
    {
        shaderStageCreateInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfoVert.pNext = nullptr;
        shaderStageCreateInfoVert.flags = 0;
        shaderStageCreateInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageCreateInfoVert.module = vertexShader;
        shaderStageCreateInfoVert.pName = "main";
        shaderStageCreateInfoVert.pSpecializationInfo = nullptr;

        shaderStageCreateInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfoFrag.pNext = nullptr;
        shaderStageCreateInfoFrag.flags = 0;
        shaderStageCreateInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageCreateInfoFrag.module = fragmentShader;
        shaderStageCreateInfoFrag.pName = "main";
        shaderStageCreateInfoFrag.pSpecializationInfo = nullptr;

        vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputCreateInfo.pNext = nullptr;
        vertexInputCreateInfo.flags = 0;
        vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
        vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
        vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
        vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

        inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.pNext = nullptr;
        inputAssemblyCreateInfo.flags = 0;
        inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

        viewPort.x = 0.0f;
        viewPort.y = 0.0f;
        viewPort.width = width;
        viewPort.height = height;
        viewPort.minDepth = 0.0f;
        viewPort.maxDepth = 1.0f;

        scissor.offset = {0, 0};
        scissor.extent = {width, height};

        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.pNext = nullptr;
        viewportStateCreateInfo.flags = 0;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewPort;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationCreateInfo.pNext = nullptr;
        rasterizationCreateInfo.flags = 0;
        rasterizationCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationCreateInfo.depthBiasClamp = 0.0f;
        rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
        rasterizationCreateInfo.lineWidth = 1.0f;

        multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleCreateInfo.pNext = nullptr;
        multisampleCreateInfo.flags = 0;
        multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleCreateInfo.minSampleShading = 1.0f;
        multisampleCreateInfo.pSampleMask = nullptr;
        multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

        /* if (blendEnable) {
            realColor.rgb = (srcColorBlendFactor * currentColor.rgb) [[[colorBlendOp]]] (dstColorBlendFactor * previousColor.rgb);
            realColor.a   = (srcAlphaBlendFactor * currentColor.a)   [[[alphaBlendOp]]] (dstAlphaBlendFactor * previousColor.a);
        }
        else {
            realColor = currentColor;
        }
        realColor = realColor & colorWriteMask;
        */
        /* Alpha blending:
        realColor.rgb = currentColor.a * currentColor.rgb + (1 - currentColor.a) * previousColor.rgb;
        realColor.a = currentColor.a;
        */
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendCreateInfo.pNext = nullptr;
        colorBlendCreateInfo.flags = 0;
        colorBlendCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
        colorBlendCreateInfo.attachmentCount = 1;
        colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
        colorBlendCreateInfo.blendConstants[0] = 0.0f; // r
        colorBlendCreateInfo.blendConstants[1] = 0.0f; // g
        colorBlendCreateInfo.blendConstants[2] = 0.0f; // b
        colorBlendCreateInfo.blendConstants[3] = 0.0f; // a

        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.pNext = nullptr;
        dynamicStateCreateInfo.flags = 0;
        dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(VkBool32);

        wasInitialized = true;
    }

    void create(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout)
    {
        if (!wasInitialized)
        {
            throw std::logic_error("Pipeline was not initialized!");
        }

        if (wasCreated)
        {
            throw std::logic_error("Pipeline was already created!");
        }

        this->device = device;

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.push_back(shaderStageCreateInfoVert);
        shaderStages.push_back(shaderStageCreateInfoFrag);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext = nullptr;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

        VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
        ASSERT_VULKAN(result);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext = nullptr;
        pipelineCreateInfo.flags = 0;
        pipelineCreateInfo.stageCount = shaderStages.size();
        pipelineCreateInfo.pStages = shaderStages.data();
        pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        pipelineCreateInfo.pTessellationState = nullptr;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineCreateInfo.layout = pipelineLayout;
        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
        ASSERT_VULKAN(result);

        wasCreated = true;
    }

    void destroy()
    {
        if ( wasCreated)
        {
            vkDestroyPipeline(device, pipeline, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            wasCreated = false;
        }
    }

    VkPipeline getPipeline()
    {
        if (!wasCreated)
        {
            throw std::logic_error("Pipeline was not created!");
        }
        return pipeline;
    }

    VkPipelineLayout getLayout()
    {
        if (!wasCreated)
        {
            throw std::logic_error("Pipeline was not created!");
        }
        return pipelineLayout;
    }

    void setPolygonMode(VkPolygonMode polygonMode)
    {
        rasterizationCreateInfo.polygonMode = polygonMode;
    }
};