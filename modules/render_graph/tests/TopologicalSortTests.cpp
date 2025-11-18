#pragma once

#include <catch2/catch_test_macros.hpp>

#define private public
#include "Mock.hpp"

const VkFormat FMT = VK_FORMAT_R8G8B8A8_UNORM;
const VkExtent2D EXTENT = {
    .width = 1024,
    .height = 1024
};

/**
 * Test single task
 */
TEST_CASE("SingleTask", "[rg]") {
	auto gpu = create_mock_gpu();

	auto image_chain = create_mock_image_chain(gpu.get(), 1, EXTENT, FMT);
	lft::rg::Builder builder(
		gpu.get(), image_chain, "output"
	);

    EmptyContext ctx;

	auto task1 = create_empty_task("task1")
        .set_output_to_final()
        .build();

	builder.add_task(task1);

	auto sorted = topology_sort(builder.m_tasks, "output");

	REQUIRE(sorted.size() == 1);
	REQUIRE(sorted[0].name() == "task1");
}

/**
 * Test two tasks where second depends on the first
 */
void test_topological_sort_01() {
	auto gpu = create_mock_gpu();

	auto image_chain = create_mock_image_chain(gpu.get(), 1, EXTENT, FMT);
	lft::rg::Builder builder(
		gpu.get(), image_chain, "output"
	);

    EmptyContext ctx;

	auto task1 = lft::rg::render_task<EmptyContext>(
		"task1", &ctx,
		[](const lft::rg::TaskBuildInfo& info, EmptyContext* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, EmptyContext* ctx) {}
	).add_color_output("resource1", FMT, EXTENT, {})
	 .build();

	auto task2 = lft::rg::render_task<EmptyContext>(
		"task2", &ctx,
		[](const lft::rg::TaskBuildInfo& info, EmptyContext* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, EmptyContext* ctx) {}
	).add_dependency("resource1")
	 .add_color_output("output", FMT, EXTENT, {})
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

	auto image_chain = create_mock_image_chain(gpu.get(), 1, EXTENT, FMT);
	lft::rg::Builder builder(
		gpu.get(), image_chain, "output"
	);

	EmptyContext ctx;

	auto task1 = lft::rg::render_task<EmptyContext>(
		"task1", &ctx,
		[](const lft::rg::TaskBuildInfo& info, EmptyContext* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, EmptyContext* ctx) {}
	).add_color_output("resource1", FMT, EXTENT, {})
	 .build();

	auto task2 = lft::rg::render_task<EmptyContext>(
		"task2", &ctx,
		[](const lft::rg::TaskBuildInfo& info, EmptyContext* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, EmptyContext* ctx) {}
	).add_dependency("resource1")
	 .add_color_output("output", FMT, EXTENT, {})
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

	auto image_chain = create_mock_image_chain(gpu.get(), 1, EXTENT, FMT);

	EmptyContext ctx;

	auto task1 = lft::rg::render_task<EmptyContext>(
		"task1", &ctx,
		[](const lft::rg::TaskBuildInfo& info, EmptyContext* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, EmptyContext* ctx) {}
	).add_color_output("resource1", FMT, EXTENT, {})
	 .build();

	auto task2 = lft::rg::render_task<EmptyContext>(
		"task2", &ctx,
		[](const lft::rg::TaskBuildInfo& info, EmptyContext* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, EmptyContext* ctx) {}
	).add_color_output("resource2", FMT, EXTENT, {})
	 .build();

	auto task3 = lft::rg::render_task<EmptyContext>(
		"task3", &ctx,
		[](const lft::rg::TaskBuildInfo& info, EmptyContext* ctx) {},
		[](const lft::rg::TaskRecordInfo& info, EmptyContext* ctx) {}
	).add_dependency("resource1")
	 .add_dependency("resource2")
	 .add_color_output("output", FMT, EXTENT, {})
	 .build();

	lft::rg::Builder builder1(
		gpu.get(), image_chain, "output"
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

// int main() {
// 	test_topological_sort_01();
// 	test_topological_sort_02();
// 	test_topological_sort_03();
//
// 	return 0;
// }
