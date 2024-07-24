//
// Created by martin on 7/21/24.
//

#include <iostream>
#include <volk/volk.h>

#include "FrameGraphBuilder.h"

int main() {
    std::cout << "Hello world" << std::endl;

    auto shading_render_graph = lft::fg::RenderGraph("shading")
        .set_recording_dependency(meshOcclusionDependency)
        .add_color_attachment("gbuf_normal", VK_FORMAT_R8G8B8A8_SRGB)
        .set_depth_attachment("gbuf_depth", VK_FORMAT_R8G8B8A8_SSCALED);

    auto frame_graph = lft::fg::FrameGraphBuilder()
        .add_render_graph(shading_render_graph)
        .add_render_graph(shadow_render_graph)
        .build();

    while(true) {
        frame_graph.invalidate_resource("gbuf_pos");

        frame_graph.invalidate_dependency("gbuf");
    }

    return 0;
}