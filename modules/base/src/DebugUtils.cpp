#include "debug/DebugUtils.h"

/* PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectName;

PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabel;
PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabel;
PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabel;

PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabel;
PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabel;
PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabel;

#define LOAD_VULKAN_FUNC(instance, name) (PFN_##name##EXT)(vkGetInstanceProcAddr(instance, #name "EXT")); */

void load_debug_utils(VkInstance instance) {
    /* vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));

    vkCmdBeginDebugUtilsLabel = LOAD_VULKAN_FUNC(instance, vkCmdBeginDebugUtilsLabel);
    vkCmdEndDebugUtilsLabel = LOAD_VULKAN_FUNC(instance, vkCmdEndDebugUtilsLabel);
    vkCmdInsertDebugUtilsLabel = LOAD_VULKAN_FUNC(instance, vkCmdInsertDebugUtilsLabel);

    vkQueueBeginDebugUtilsLabel = LOAD_VULKAN_FUNC(instance, vkQueueBeginDebugUtilsLabel);
    vkQueueEndDebugUtilsLabel = LOAD_VULKAN_FUNC(instance, vkQueueEndDebugUtilsLabel);
    vkQueueInsertDebugUtilsLabel = LOAD_VULKAN_FUNC(instance, vkQueueInsertDebugUtilsLabel); */
}