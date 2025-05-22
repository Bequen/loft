#pragma once

#include <string>
#include <vector>

#include "AdjacencyMatrix.hpp"

#include "RenderGraph.hpp"
#include "ImageChain.hpp"
#include "RenderPass.hpp"

namespace lft::rg {

struct RenderGraphResource {
	VkImage image;
	VkImageView image_view;
};

/**
 * Optimization structures for graph definition
 */
struct BuilderContext {
	AdjacencyMatrix adjacency_matrix;

	struct {
		std::vector<ImageResourceDefinition> definitions;
		std::vector<uint32_t> counters;
		std::map<std::string, uint32_t> names;
	} resources;


	std::map<std::string, uint32_t> solved;

	std::map<std::string, uint32_t> named_dependencies;
	uint32_t num_dependencies = 0;
	
	std::optional<uint32_t> get_signal_idx(const std::string& dependency) {
		auto it = named_dependencies.find(dependency);
		if(it == named_dependencies.end()) {
			return {};
		}

		return it->second;
	}

	uint32_t create_signal_idx(const std::string& name) {
		if(named_dependencies.find(name) == named_dependencies.end()) {
			named_dependencies.insert({name, num_dependencies});
			return num_dependencies++;
		}

		return named_dependencies[name];
	}
};


class Builder {
private:
	std::string m_output_name;
	std::map<std::string, uint32_t> m_name_to_task_idx;
	std::vector<std::shared_ptr<GpuTask>> m_tasks;

	const std::shared_ptr<GpuTask> get_task_by_name(const std::string& name) {
		if(m_name_to_task_idx.find(name) == m_name_to_task_idx.end()) {
			throw std::runtime_error("Task does not exist");
		}

		return m_tasks[m_name_to_task_idx[name]];
	}

	AdjacencyMatrix build_adjacency_matrix() const;
	BuilderContext build_context() const;
	
	/**
	 * Creates an vector of task definitions in context.
	 * Definitions are used during allocation phase.
	 */
	std::vector<std::shared_ptr<GpuTask>> topology_sort(
			std::vector<std::shared_ptr<GpuTask>>& tasks,
			const BuilderContext& context
	);



	/**
	 * Builds a successors of render_passes. Adds it to command buffer and
	 * tries to expand it.
	 */
	void build_tree(std::vector<std::string>& render_passes, 
			uint32_t command_buffer,
			BuilderContext& context);

public:
	Builder(const std::string& output_name) :
		m_output_name(output_name) {
	}

	void add_render_pass(const std::shared_ptr<GpuTask> task) {
		m_tasks.push_back(task);
		m_name_to_task_idx[task->name()] = m_tasks.size() - 1;
	}

	void remove_render_pass(const std::string& name);

	RenderGraph build(std::shared_ptr<Gpu> gpu,
					  const ImageChain& output_chain,
					  uint32_t num_buffers);
};

}
