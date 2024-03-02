#pragma once

#include <vulkan/vulkan.h>

struct BlendingInfo {
    VkBlendOp colorBlendOp;
    VkBlendFactor srcColorBlendFactor;
    VkBlendFactor dstColorBlendFactor;

    VkBlendOp alphaBlendOp;
    VkBlendFactor srcAlphaBlendFactor;
    VkBlendFactor dstAlphaBlendFactor;

    BlendingInfo() :
        colorBlendOp(VK_BLEND_OP_ADD),
        srcColorBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA),
        dstColorBlendFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA),
        alphaBlendOp(VK_BLEND_OP_ADD),
        srcAlphaBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA),
        dstAlphaBlendFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA) {
    }
};
