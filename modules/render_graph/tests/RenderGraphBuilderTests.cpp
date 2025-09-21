#include "ImageChain.hpp"
#include "RenderPass.hpp"
#include <optional>
#include <vulkan/vulkan_core.h>

#define private public

#include "Assert.h"

#include "RenderGraphBuilder.hpp"
#include "Swapchain.hpp"
#include "SDLWindow.h"

#include "Mock.hpp"


struct Struct {
};

void test_render_graph_push() {
   	VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 1, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    auto task1 = lft::rg::render_task<Struct>(
            "task1", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_color_output("resource1", fmt, extent, {})
            .set_extent(extent)
            .build();
    builder.add_task(task1);

    std::cout << "First build" << std::endl;
    auto rg = builder.build();

    ASSERT(rg.m_buffers[0]->num_batches() == 1);
    ASSERT(rg.m_buffers[0]->batch(0).tasks.size() == 1);
    ASSERT(rg.m_buffers[0]->batch(0).tasks[0].pDefinition.equals(task1));

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource1")
            .set_extent(extent)
            .build();
    builder.add_task(task2);

    std::cout << "Second build" << std::endl;
    rg = builder.build();

    ASSERT(rg.m_buffers[0]->num_batches() == 2);
    ASSERT(rg.m_buffers[0]->batch(0).tasks.size() == 1);
    ASSERT(rg.m_buffers[0]->batch(0).tasks[0].pDefinition.equals(task1));

    ASSERT(rg.m_buffers[0]->batch(1).tasks.size() == 1);
    ASSERT(rg.m_buffers[0]->batch(1).tasks[0].pDefinition.equals(task2));

    auto deps1 = rg.m_dependency_matrix->get_dependencies(1);
    ASSERT(deps1.size() == 1);
    ASSERT(deps1[0] == 0);
    auto deps2 = rg.m_dependency_matrix->get_dependencies("task2");
    ASSERT(deps2.size() == 1);
    ASSERT(deps2[0] == "task1");

    lft::rg::Builder builder2(gpu.get(), image_chain, "output");
    builder2.add_task(task1);
    builder2.add_task(task2);
    std::cout << "Compare build" << std::endl;
    builder2.build();

    ASSERT(builder.m_allocator.equals(builder2.m_allocator));
    ASSERT(builder2.m_allocator.equals(builder.m_allocator));
}

void test_render_graph_insert_begin() {
   	VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 1, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    auto task1 = lft::rg::render_task<Struct>(
            "task1", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("resource1", fmt, extent, {});

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("resource2", fmt, extent, {});

    auto task3 = lft::rg::render_task<Struct>(
            "task3", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource2");

    builder.add_task(task2.build());
    builder.add_task(task3.build());

    builder.build();

    task2.add_dependency("resource1");
    builder.add_task(task2.build());
    builder.add_task(task1.build());

    lft::rg::RenderGraph rg = builder.build();

    ASSERT(rg.buffer(0).num_batches() == 3);
    ASSERT(rg.buffer(0).batch(0).tasks[0].pDefinition.name() == "task1");
    ASSERT(rg.buffer(0).batch(1).tasks[0].pDefinition.name() == "task2");
    ASSERT(rg.buffer(0).batch(2).tasks[0].pDefinition.name() == "task3");
}


void test_render_graph_insert_middle() {
   	VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 1, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    auto task1 = lft::rg::render_task<Struct>(
            "task1", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("resource1", fmt, extent, {});

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("resource2", fmt, extent, {})
            .add_dependency("resource1");

    auto task3 = lft::rg::render_task<Struct>(
            "task3", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource1");

    builder.add_task(task1.build());
    builder.add_task(task3.build());

    builder.build();

    task3.add_dependency("resource2");
    builder.add_task(task3.build());
    builder.add_task(task2.build());

    lft::rg::RenderGraph rg = builder.build();

    ASSERT(rg.buffer(0).num_batches() == 3);
    ASSERT(rg.buffer(0).batch(0).tasks[0].pDefinition.name() == "task1");
    ASSERT(rg.buffer(0).batch(1).tasks[0].pDefinition.name() == "task2");
    ASSERT(rg.buffer(0).batch(2).tasks[0].pDefinition.name() == "task3");
}

void test_render_graph_extent() {
   	VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 1, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    auto task1 = lft::rg::render_task<Struct>(
            "task1", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_color_output("resource1", fmt, extent, {})
            .build();
    builder.add_task(task1);

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource1")
            .build();
    builder.add_task(task2);

    lft::rg::RenderGraph rg = builder.build();

    ASSERT(rg.buffer(0).num_batches() == 2);
    // equals tests the extent
    ASSERT(!rg.buffer(0).batch(0).tasks[0].pDefinition.equals(task1));
    ASSERT(!rg.buffer(0).batch(1).tasks[0].pDefinition.equals(task2));

    ASSERT(rg.buffer(0).batch(0).tasks[0].extent.width == extent.width &&
        rg.buffer(0).batch(0).tasks[0].extent.height == extent.height);
    ASSERT(rg.buffer(0).batch(1).tasks[0].extent.width == extent.width &&
        rg.buffer(0).batch(1).tasks[0].extent.height == extent.height);
}

void test_render_graph_update_renderpass() {
    std::cout << "Running update test" << std::endl;
   	VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 1, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    auto task1 = lft::rg::render_task<Struct>(
            "task1", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_color_output("resource1", fmt, extent, {})
            .set_extent(extent)
            .build();
    builder.add_task(task1);

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource1")
            .set_extent(extent)
            .build();
    builder.add_task(task2);
    lft::rg::RenderGraph rg = builder.build();

    // update
    task1.add_color_output("resource2", fmt, extent, {});

    builder.add_task(task1);

    VkRenderPass previous_rp = rg.buffer(0).batch(0).tasks[0].render_pass.render_pass;
    auto previous_fb = rg.buffer(0).batch(0).tasks[0].framebuffer;
    builder.build();

    ASSERT(rg.buffer(0).num_batches() == 2);
    auto _task1 = rg.buffer(0).batch(0).tasks[0];
    auto _task2 = rg.buffer(0).batch(1).tasks[0];

    // equals tests the extent
    ASSERT(_task1.pDefinition.equals(task1));
    ASSERT(_task2.pDefinition.equals(task2));

    ASSERT(_task1.render_pass.render_pass != previous_rp);
    ASSERT(_task1.framebuffer != previous_fb);
}

void test_render_graph_remove() {
    std::cout << "Running update test" << std::endl;
   	VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 1, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    auto task1 = lft::rg::render_task<Struct>(
            "task1", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_color_output("resource1", fmt, extent, {})
            .add_color_output("resource2", fmt, extent, {})
            .set_extent(extent)
            .build();
    builder.add_task(task1);

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource1")
            .set_extent(extent)
            .build();

    builder.add_task(task1);
    builder.add_task(task2);
    auto rg = builder.build();

    ASSERT(rg.buffer(0).num_batches() == 2);
    ASSERT(rg.buffer(0).batch(0).tasks.size() == 1);
    ASSERT(rg.buffer(0).batch(1).tasks.size() == 1);

    builder.remove_task("task2");
    rg = builder.build();

    ASSERT(rg.buffer(0).num_batches() == 1);
    ASSERT(rg.buffer(0).batch(0).tasks.size() == 1);
}

/*
 * Attempts to add task, build the render graph, remove the task again and rebuild repeatedly.
 */
void test_render_graph_remove_and_add() {
    std::cout << "Running update test" << std::endl;
   	VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 4, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    auto task1 = lft::rg::render_task<Struct>(
            "task1", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_color_output("resource1", fmt, extent, {})
            .add_color_output("resource2", fmt, extent, {})
            .set_extent(extent)
            .build();
    builder.add_task(task1);

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource1")
            .set_extent(extent)
            .build();

    builder.add_task(task1);
    builder.add_task(task2);
    auto rg = builder.build();

    ASSERT(rg.buffer(0).num_batches() == 2);
    ASSERT(rg.buffer(0).batch(0).tasks.size() == 1);
    ASSERT(rg.buffer(0).batch(1).tasks.size() == 1);

    for(uint32_t i = 0; i < 10; i++) {
        builder.remove_task("task2");
        rg = builder.build();

        ASSERT(rg.buffer(0).num_batches() == 1);
        ASSERT(rg.buffer(0).batch(0).tasks.size() == 1);

        builder.add_task(task2);
        rg = builder.build();

        ASSERT(rg.buffer(0).num_batches() == 2);
        ASSERT(rg.buffer(0).batch(0).tasks.size() == 1);
        ASSERT(rg.buffer(0).batch(1).tasks.size() == 1);
    }
}

void test_buffer_idxs() {
   	VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 4, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    auto task1 = lft::rg::render_task<Struct>(
            "task1", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {
                ASSERT(info.buffer_idx() == 0);
            },
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_color_output("resource1", fmt, extent, {})
            .add_color_output("resource2", fmt, extent, {})
            .set_extent(extent)
            .build();

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {
                ASSERT(info.buffer_idx() == 0);
            },
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource1")
            .set_extent(extent)
            .build();

    builder.add_task(task1);
    builder.add_task(task2);
    auto rg = builder.build();

    ASSERT(rg.buffer(0).index() == 0);
}

void test_compute() {
    VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 4, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    bool is_build_func_called = false;
    auto task1 = lft::rg::compute_task<Struct>(
            "task1", &data,
            [&](const lft::rg::TaskBuildInfo& info, Struct* ctx) {
                ASSERT(info.buffer_idx() == 0);
                is_build_func_called = true;
            },
            [&](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_buffer_output("resource1", 1000)
            .build();

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {
                ASSERT(info.buffer_idx() == 0);
            },
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource1")
            .set_extent(extent)
            .build();

    builder.add_task(task1);
    builder.add_task(task2);
    auto rg = builder.build();

    ASSERT(is_build_func_called);
    ASSERT(rg.buffer(0).batch(0).tasks[0].pDefinition.name() == "task1");
    ASSERT(rg.buffer(0).batch(1).tasks[0].pDefinition.name() == "task2");
}

void test_render_graph2() {
    VkExtent2D extent = {
            .width = 1024,
            .height = 1024
    };

    auto gpu = create_mock_gpu();

    VkImageView a;
    VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
    ImageChain image_chain = create_mock_image_chain(gpu.get(), 4, extent, fmt);

    lft::rg::Builder builder(gpu.get(), image_chain, "output");

    Struct data = {};
    auto task1 = lft::rg::render_task<Struct>(
            "task1", &data,
            [&](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [&](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("resource1", VK_FORMAT_R8G8B8A8_UNORM, extent, {0.0, 0.0, 0.0, 0.0})
            .build();

    auto task2 = lft::rg::render_task<Struct>(
            "task2", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_dependency("resource1")
            .add_color_output("output", fmt, extent, {})
            .build();

    auto task3 = lft::rg::compute_task<Struct>(
            "task3", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_buffer_output("resource2", 1000)
            .build();

    auto task4 = lft::rg::render_task<Struct>(
            "task4", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("resource2")
            .add_dependency("task2")
            .build();

    auto task5 = lft::rg::render_task<Struct>(
            "task5", &data,
            [](const lft::rg::TaskBuildInfo& info, Struct* ctx) {},
            [](const lft::rg::TaskRecordInfo& info, Struct* ctx) {})
            .add_color_output("output", fmt, extent, {})
            .add_dependency("task2")
            .build();

    builder.add_task(task1);
    builder.add_task(task2);
    builder.add_task(task3);
    builder.add_task(task4);
    builder.add_task(task5);
    auto rg = builder.build();

    auto wait1 = rg.get_wait_semaphores_for(&rg.buffer(0), 0);
    ASSERT(rg.buffer(0).batch(0).tasks[0].pDefinition.name() == "task3");
    ASSERT(wait1.empty());

    auto wait2 = rg.get_wait_semaphores_for(&rg.buffer(0), 1);
    ASSERT(rg.buffer(0).batch(1).tasks[0].pDefinition.name() == "task1");
    ASSERT(wait2.empty());

    auto wait3 = rg.get_wait_semaphores_for(&rg.buffer(0), 2);
    ASSERT(rg.buffer(0).batch(2).tasks[0].pDefinition.name() == "task2");
    ASSERT(wait3.size() == 1);
    ASSERT(wait3[0].semaphore == rg.buffer(0).batch(1).signal);

    auto wait4 = rg.get_wait_semaphores_for(&rg.buffer(0), 3);
    ASSERT(rg.buffer(0).batch(3).tasks[0].pDefinition.name() == "task5");
    ASSERT(wait4.size() == 1);
    ASSERT(wait4[0].semaphore == rg.buffer(0).batch(2).signal);

    auto wait5 = rg.get_wait_semaphores_for(&rg.buffer(0), 4);
    ASSERT(rg.buffer(0).batch(4).tasks[0].pDefinition.name() == "task4");
    ASSERT(wait5.size() == 2);
    ASSERT(wait5[0].semaphore == rg.buffer(0).batch(2).signal ||
        wait5[0].semaphore == rg.buffer(0).batch(0).signal);
    ASSERT(wait5[1].semaphore == rg.buffer(0).batch(0).signal ||
        wait5[1].semaphore == rg.buffer(0).batch(2).signal);
}

int main() {
    /* test_render_graph_extent();
	test_render_graph_push();
	test_render_graph_insert_begin();
    test_render_graph_insert_middle();
	test_render_graph_update_renderpass();
	test_render_graph_remove();
	test_buffer_idxs();
	test_compute(); */
	test_render_graph2();
}
