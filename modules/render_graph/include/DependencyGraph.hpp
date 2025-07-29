#pragma once

#include "RenderPass.hpp"

namespace lft::rg {

/*
 * DependencyGraph is a class that represents a dynamic dependency graph for tasks in a render graph.
 */
class DependencyGraph {
private:
    std::vector<TaskInfo> m_queue;

public:
	DependencyGraph();

	uint32_t add_task(const TaskInfo& task);

	void remove_task(const TaskInfo& task);

	std::vector<TaskInfo>& build_queue();
};

}
