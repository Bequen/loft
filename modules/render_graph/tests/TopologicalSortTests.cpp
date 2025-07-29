#define private public

#include "RenderGraphBuilder.hpp"

#include "Mock.hpp"

void test_topological_sort_01() {
	auto gpu = create_mock_gpu();

	VkExtent2D extent = {
		.width = 1024,
		.height = 1024
	};

	VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;

	auto image_chain = create_mock_image_chain(gpu, 1, extent, fmt);
	lft::rg::Builder builder(
		gpu, image_chain, "output"
	);

	struct Context {
	};
	Context ctx;

	auto task1 = lft::rg::render_task<Context>(
		"task1", &ctx,
		[](const lft::rg::TaskBuildInfo& info, Context* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, Context* ctx) {}
	).add_color_output("resource1", fmt, extent, {})
	 .build();

	auto task2 = lft::rg::render_task<Context>(
		"task2", &ctx,
		[](const lft::rg::TaskBuildInfo& info, Context* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, Context* ctx) {}
	).add_dependency("resource1")
	 .add_color_output("output", fmt, extent, {})
	 .build();

	builder.add_task(task1);
	builder.add_task(task2);

	auto sorted = topology_sort(builder.m_tasks, "output");

	ASSERT(sorted.size() == 2);
	ASSERT(sorted[0].name() == "task1");
	ASSERT(sorted[1].name() == "task2");
}

void test_topological_sort_02() {
	auto gpu = create_mock_gpu();

	VkExtent2D extent = {
		.width = 1024,
		.height = 1024
	};

	VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;

	auto image_chain = create_mock_image_chain(gpu, 1, extent, fmt);
	lft::rg::Builder builder(
		gpu, image_chain, "output"
	);

	struct Context {
	};
	Context ctx;

	auto task1 = lft::rg::render_task<Context>(
		"task1", &ctx,
		[](const lft::rg::TaskBuildInfo& info, Context* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, Context* ctx) {}
	).add_color_output("resource1", fmt, extent, {})
	 .build();

	auto task2 = lft::rg::render_task<Context>(
		"task2", &ctx,
		[](const lft::rg::TaskBuildInfo& info, Context* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, Context* ctx) {}
	).add_dependency("resource1")
	 .add_color_output("output", fmt, extent, {})
	 .build();

	// inverted - test if it actually sorts
	builder.add_task(task2);
	builder.add_task(task1);

	auto sorted = topology_sort(builder.m_tasks, "output");

	ASSERT(sorted.size() == 2);
	ASSERT(sorted[0].name() == "task1");
	ASSERT(sorted[1].name() == "task2");
}

void test_topological_sort_03() {
	auto gpu = create_mock_gpu();

	VkExtent2D extent = {
		.width = 1024,
		.height = 1024
	};

	VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;

	auto image_chain = create_mock_image_chain(gpu, 1, extent, fmt);

	struct Context {
	};

	Context ctx;

	auto task1 = lft::rg::render_task<Context>(
		"task1", &ctx,
		[](const lft::rg::TaskBuildInfo& info, Context* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, Context* ctx) {}
	).add_color_output("resource1", fmt, extent, {})
	 .build();

	auto task2 = lft::rg::render_task<Context>(
		"task2", &ctx,
		[](const lft::rg::TaskBuildInfo& info, Context* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, Context* ctx) {}
	).add_color_output("resource2", fmt, extent, {})
	 .build();

	auto task3 = lft::rg::render_task<Context>(
		"task3", &ctx,
		[](const lft::rg::TaskBuildInfo& info, Context* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, Context* ctx) {}
	).add_dependency("resource1")
	 .add_dependency("resource2")
	 .add_color_output("output", fmt, extent, {})
	 .build();

	lft::rg::Builder builder1(
		gpu, image_chain, "output"
	);

	builder1.add_task(task1);
	builder1.add_task(task2);
	builder1.add_task(task3);

	auto sorted = topology_sort(builder1.m_tasks, "output");

	ASSERT(sorted.size() == 3);
	ASSERT(sorted[0].name() == "task1" || sorted[0].name() == "task2");
	ASSERT(sorted[1].name() == "task1" || sorted[1].name() == "task2");
	ASSERT(sorted[0].name() != sorted[1].name());
	ASSERT(sorted[2].name() == "task3");
}

int main() {
	test_topological_sort_01();
	test_topological_sort_02();
	test_topological_sort_03();

	return 0;
}
