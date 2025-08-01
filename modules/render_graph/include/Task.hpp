#pragma once

#include <cassert>
#include <vector>
#include <iostream>

#include <volk.h>

#include "RenderPass.hpp"

namespace lft::rg {

#define MAX_ATTACHMENT_COUNT 9
#define MAX_COLOR_ATTACHMENT_COUNT MAX_ATTACHMENT_COUNT - 1

struct TaskRenderPassState {
    uint32_t num_attachments : 4;
    uint32_t resource_flags : 18;

    TaskRenderPassState() {

    }

    TaskRenderPassState(
        uint32_t num_color_attachments,
        bool has_depth_attachment
    ) {
        assert(num_color_attachments <= MAX_COLOR_ATTACHMENT_COUNT);
        num_attachments = num_color_attachments + has_depth_attachment;
    }

    inline void set_resource_is_first(uint32_t resource_idx) {
        assert(resource_idx < MAX_ATTACHMENT_COUNT);
        resource_flags |= (1 << resource_idx);
    }

    inline void set_resource_is_last(uint32_t resource_idx) {
        assert(resource_idx < MAX_ATTACHMENT_COUNT);
        resource_flags |= (1 << (resource_idx + 9));
    }

    inline bool is_resource_first(uint32_t resource_idx) const {
        assert(resource_idx < MAX_ATTACHMENT_COUNT);
        return resource_flags & (1 << resource_idx);
    }

    inline bool is_resource_last(uint32_t resource_idx) const {
        assert(resource_idx < MAX_ATTACHMENT_COUNT);
        return resource_flags & (1 << (resource_idx + 9));
    }
};

struct TaskRenderPass {
    VkRenderPass render_pass;
    TaskRenderPassState state;

    TaskRenderPass() : render_pass(VK_NULL_HANDLE) {

    }



    TaskRenderPass(const VkRenderPass rp, const TaskRenderPassState state) :
    render_pass(rp),
    state(state) {

    }
};

struct Task {
   	TaskInfo pDefinition;

   	TaskRenderPass render_pass;
    std::vector<uint32_t> rp_attachment_states;
   	std::vector<VkFramebuffer> framebuffer;

   	VkExtent2D extent;

    bool equals(const Task& other) const {
        if(!pDefinition.equals(other.pDefinition)) {
            return false;
        }

        if(framebuffer.size() != other.framebuffer.size()) {
            std::cout << "Number of framebuffers is not equal" << std::endl;
            return false;
        }

        return true;
    }
};

}
