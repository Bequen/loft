#include "RenderGraphBuilder.hpp"
#include "RenderPass.hpp"
#include <algorithm>
#include <ostream>
#include <iostream>
#include <queue>

#include "RenderGraphAllocator.hpp"

namespace lft::rg {

std::vector<std::string> get_task_names(std::vector<std::shared_ptr<GpuTask>> tasks) {
	std::vector<std::string> result(tasks.size());
	for(auto& task : tasks) {
		result.push_back(task->name());
	}

	return result;
}

bool is_depending_on(std::shared_ptr<GpuTask> task, const std::shared_ptr<GpuTask> depends_on) {
	return std::find_if(task->dependencies().begin(), task->dependencies().end(),
			[&](const auto& dependency) {
				if(dependency == depends_on->name()) {
					return true;
				}

				if(depends_on->depth_output().has_value() && 
					dependency == depends_on->depth_output()->name()) {
					return true;
				}

				return std::find_if(depends_on->outputs().begin(),
						depends_on->outputs().end(),
						[&](const auto& output) { return output.name() == dependency; }) 
					!= depends_on->outputs().end();
			}) != task->dependencies().end();
}

BuilderContext Builder::build_context() const {
	AdjacencyMatrix matrix(get_task_names(m_tasks));
	
	for(auto& task : m_tasks) {
		for(auto& dependency : m_tasks) {
			if(task->name() == dependency->name()) {
				continue;
			}

			if(is_depending_on(task, dependency)) {
				matrix.set(dependency->name(), task->name());
			}
		}
	}

	return BuilderContext {
		.adjacency_matrix = matrix
	};
}

std::map<std::string, uint32_t> get_depedencies_counts(
		const std::vector<std::shared_ptr<GpuTask>>& tasks,
		const BuilderContext& context
) {
	std::map<std::string, uint32_t> counts;
	for(auto& task : tasks) {
		counts[task->name()] = std::max(1u, context.adjacency_matrix.num_dependencies_of(task->name()));
	}

	return counts;
}

std::vector<std::shared_ptr<GpuTask>> get_begin_tasks(
		const std::vector<std::shared_ptr<GpuTask>>& tasks,
		const BuilderContext& context
) {
	std::vector<std::shared_ptr<GpuTask>> begin_tasks;
	for(auto& task : tasks) {
		if(context.adjacency_matrix.num_dependencies_of(task->name()) == 0) {
			begin_tasks.push_back(task);
		} else {
			std::cout << std::format("Task {} has {} dependencies", task->name(), context.adjacency_matrix.num_dependencies_of(task->name())) << std::endl;
		}
	}

	return begin_tasks;
}

std::vector<std::shared_ptr<GpuTask>> Builder::topology_sort(
		std::vector<std::shared_ptr<GpuTask>>& tasks,
		const BuilderContext& context
) {
	std::cout << std::format("Sorting tasks: {}", tasks.size()) << std::endl;
	auto counts = get_depedencies_counts(tasks, context);

	std::queue<std::shared_ptr<GpuTask>> queue;
	auto begin_tasks = get_begin_tasks(tasks, context);
	std::cout << std::format("Begin tasks: {}", begin_tasks.size()) << std::endl;
	for(auto& task : begin_tasks) {
		for(auto& output : task->outputs()) {
			std::cout << "Prd: " << output.name() << std::endl;
		}

		queue.push(task);
	}

	std::vector<std::shared_ptr<GpuTask>> sorted_tasks;

	while(!queue.empty()) {
		auto task = queue.front();
		queue.pop();

		counts[task->name()]--;
		if(counts[task->name()] > 0) {
			continue;
		}

		sorted_tasks.push_back(task);

		auto successors = context.adjacency_matrix.get_successors(task->name());
		for(auto succesor : successors) {
			queue.push(get_task_by_name(succesor));
		}
	}

	return sorted_tasks;
}

std::vector<uint32_t> collect_dependency_idxs_of_task(
		std::vector<std::shared_ptr<GpuTask>> tasks,
		uint32_t task_idx
) {
	std::shared_ptr<GpuTask> task = tasks[task_idx];

	std::vector<uint32_t> idxs;
	uint32_t idx = 0;
	for(auto dependency : tasks) {
		if(dependency->name() == task->name()) {
			continue;
		}

		if(is_depending_on(task, dependency)) {
			idxs.push_back(idx);
		}

		idx++;
	}

	return idxs;
}

RenderGraph Builder::build(
		std::shared_ptr<Gpu> gpu,
		const ImageChain& output_chain,
		uint32_t num_buffers
) {
	if(num_buffers == 0) {
		throw std::runtime_error("Number of buffers must be greater than 0");
	}

	if(gpu.get() == nullptr) {
		throw std::runtime_error("Invalid GPU");
	}

	if(m_tasks.empty()) {
		throw std::runtime_error("No tasks defined");
	}

	bool has_output = false;
	for(auto& task : m_tasks) {
		for(auto& output : task->outputs()) {
			if(output.name() == m_output_name) {
				has_output = true;
				break;
			}
		}

		if(has_output) {
			break;
		}
	}

	if(!has_output) {
		throw std::runtime_error(std::format("No task outputs to {} found", m_output_name));
	}

	auto context = build_context();

	std::vector<RenderGraphBuffer> buffers(num_buffers);

	auto sorted_tasks = topology_sort(m_tasks, context);
	for(auto& task : sorted_tasks) {
		task->set_extent(output_chain.extent());
	}

	std::vector<CommandBufferDefinition> command_buffers;

	for(uint32_t i = 0; i < sorted_tasks.size(); i++) { 
		command_buffers.push_back({
			.first_renderpass_idx = i,
			.num_renderpasses = 1,
			.signal_idx = i,
			.wait_signals_idx = collect_dependency_idxs_of_task(sorted_tasks, i)
		});
	}

	GraphAllocationInfo alloc_info = {
		.gpu = gpu,
		.output_chain = output_chain,
		.output_name = m_output_name,
		.render_passes = sorted_tasks,
		.command_buffers = command_buffers,
	};

	return Allocator(alloc_info, num_buffers).allocate();
}

}
