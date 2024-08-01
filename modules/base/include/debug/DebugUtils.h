#pragma once

#ifndef _DEBUG_UTILS_H
#define _DEBUG_UTILS_H

#include <volk/volk.h>

/* extern PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectName;

extern PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabel;
extern PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabel;
extern PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabel;

extern PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabel;
extern PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabel;
extern PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabel; */

void load_debug_utils(VkInstance instance);

#endif
