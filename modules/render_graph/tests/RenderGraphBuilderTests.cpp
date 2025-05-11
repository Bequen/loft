#include "Assert.h"
#include "RenderGraphBuilder.hpp"
#include <vulkan/vulkan_core.h>
#include <iostream>

void test_basic_render_graph() {
	/* auto builder = lft::rg::Builder("swapchain");

	auto pass1 = lft::rg::GpuTask("pass1");
	pass1.define_output("resource1", VK_FORMAT_R8G8B8A8_UNORM, {1024, 1024});
	pass1.define_output("resource2", VK_FORMAT_R8G8B8A8_UNORM, {1024, 1024});

	auto pass2 = lft::rg::RenderPass("pass2");
	pass2.add_dependency("resource1");
	pass2.define_output("swapchain", VK_FORMAT_R8G8B8A8_UNORM, {1024, 1024});

	builder.add_render_pass(pass1);
	builder.add_render_pass(pass2);

	auto render_graph_def = builder.build(nullptr, ImageChain(VK_FORMAT_R8G8B8A8_UNORM, {}, {}), 1);
	ASSERT(render_graph_def.command_buffers().size() == 2);

	auto cmd_buf_1 = render_graph_def.command_buffers()[0];
	// yet the number of render passes will be 1
	ASSERT(cmd_buf_1.render_passes.size() == 1);
	auto render_pass_1 = render_graph_def.render_pass(cmd_buf_1.render_passes[0]);
	ASSERT(cmd_buf_1.dependencies.size() == 1);
	ASSERT(cmd_buf_1.render_passes[0] == "pass1");
	ASSERT(render_pass_1.name() == "pass1");

	auto cmd_buf_2 = render_graph_def.command_buffers()[1];
	ASSERT(cmd_buf_2.render_passes.size() == 1);
	auto render_pass_2 = render_graph_def.render_pass(cmd_buf_2.render_passes[0]);
	ASSERT(cmd_buf_2.dependencies.size() == 1);
	ASSERT(cmd_buf_2.render_passes[0] == "pass2");
	ASSERT(render_pass_2.name() == "pass2"); */
}


void test_render_graph() {
	/* auto builder = lft::rg::Builder("swapchain");

	auto pass1 = lft::rg::RenderPass("pass1");
	pass1.define_output("resource1", VK_FORMAT_R8G8B8A8_UNORM, {1024, 1024});
	pass1.define_output("resource2", VK_FORMAT_R8G8B8A8_UNORM, {1024, 1024});

	auto pass2 = lft::rg::RenderPass("pass2");
	pass2.add_dependency("resource1");
	pass2.define_output("resource3", VK_FORMAT_R8G8B8A8_UNORM, {1024, 1024});

	auto pass3 = lft::rg::RenderPass("pass3");
	pass3.add_dependency("resource1");
	pass3.define_output("resource4", VK_FORMAT_R8G8B8A8_UNORM, {1024, 1024});

	auto pass4 = lft::rg::RenderPass("pass4");
	pass4.add_dependency("resource3");
	pass4.add_dependency("resource4");
	pass4.define_output("swapchain", VK_FORMAT_R8G8B8A8_UNORM, {1024, 1024});

	builder.add_render_pass(pass1);
	builder.add_render_pass(pass2);
	builder.add_render_pass(pass3);
	builder.add_render_pass(pass4);

	auto render_graph_def = builder.build(nullptr, ImageChain(VK_FORMAT_R8G8B8A8_UNORM, {}, {}), 1);
	ASSERT(render_graph_def.command_buffers().size() == 4);

	auto cmd_buf_1 = render_graph_def.command_buffers()[0];
	// yet the number of render passes will be 1
	ASSERT(cmd_buf_1.render_passes.size() == 1);
	auto render_pass_1 = render_graph_def.render_pass(cmd_buf_1.render_passes[0]);
	ASSERT(cmd_buf_1.dependencies.size() == 1);
	ASSERT(cmd_buf_1.render_passes[0] == "pass1");
	ASSERT(render_pass_1.name() == "pass1");

	auto cmd_buf_2 = render_graph_def.command_buffers()[1];
	ASSERT(cmd_buf_2.render_passes.size() == 1);
	auto render_pass_2 = render_graph_def.render_pass(cmd_buf_2.render_passes[0]);
	ASSERT(cmd_buf_2.dependencies.size() == 1);
	ASSERT(cmd_buf_2.render_passes[0] == "pass2");
	ASSERT(render_pass_2.name() == "pass2");

	auto cmd_buf_3 = render_graph_def.command_buffers()[2];
	// yet the number of render passes will be 1
	ASSERT(cmd_buf_3.render_passes.size() == 1);
	auto render_pass_3 = render_graph_def.render_pass(cmd_buf_3.render_passes[0]);
	ASSERT(cmd_buf_3.dependencies.size() == 1);
	ASSERT(cmd_buf_3.render_passes[0] == "pass3");
	ASSERT(render_pass_3.name() == "pass3"); */
}

int main() {
	test_basic_render_graph();

	std::cout << "------------------------" << std::endl;

	test_render_graph();
}
