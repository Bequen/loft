#pragma once

#include <bitset>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <memory>
#include <iostream>

#include "Gpu.hpp"
#include "props.hpp"
#include "Resource.hpp"
#include "Recording.hpp"

#include <volk.h>
#include <map>
#include <vulkan/vulkan_core.h>

namespace lft::rg {

class ImageResourceDescription {
private:
	std::string m_name;
	VkFormat m_format;
	VkExtent2D m_extent;
	VkClearValue m_clear_value;
	bool m_is_color;

public:
	REF(m_name, name);
	GET(m_format, format);
	GET(m_extent, extent);
	GET(m_clear_value, clear_value);
	GET(m_is_color, is_color);

    inline void set_extent(VkExtent2D extent) {
        m_extent = extent;
    }

	ImageResourceDescription(
			const std::string& name,
			VkFormat format,
			VkExtent2D extent,
			VkClearValue clear_value,
			bool is_color
		) :
		m_name(name),
		m_format(format),
		m_extent(extent),
		m_clear_value(clear_value),
		m_is_color(is_color) {
	}

	bool equals(const ImageResourceDescription& other) const {
	    return m_name == other.m_name &&
			   m_format == other.m_format &&
			   m_extent.width == other.m_extent.width &&
			   m_extent.height == other.m_extent.height &&
			   m_clear_value.color.uint32[0] == other.m_clear_value.color.uint32[0] &&
			   m_clear_value.color.uint32[1] == other.m_clear_value.color.uint32[1] &&
			   m_clear_value.color.uint32[2] == other.m_clear_value.color.uint32[2] &&
			   m_clear_value.color.uint32[3] == other.m_clear_value.color.uint32[3] &&
			   m_clear_value.depthStencil.depth == other.m_clear_value.depthStencil.depth;
	}
};

struct BufferResourceDescription {
private:
	std::string m_name;
	VkDeviceSize m_size;

public:
	REF(m_name, name);
	GET(m_size, size);

	BufferResourceDescription(
			const std::string& name,
			VkDeviceSize size
		) :
		m_name(name),
		m_size(size) {
	}

	bool equals(const BufferResourceDescription& other) const {
	    return m_name == other.m_name && m_size == other.m_size;
	}
};

class TaskRecordInfo {
	const Gpu* m_gpu;
    lft::Recording m_recording;
	uint32_t m_buffer_idx;
	uint32_t m_image_idx;

public:
	GET(m_gpu, gpu);
	REF(m_recording, recording);
	GET(m_image_idx, image_idx);
	GET(m_buffer_idx, buffer_idx);

	TaskRecordInfo(
			const Gpu* gpu,
			lft::Recording recording,
			uint32_t buffer_idx,
			uint32_t image_in_flight_idx) :
		m_recording(recording),
		m_image_idx(image_in_flight_idx),
		m_buffer_idx(buffer_idx),
		m_gpu(gpu) {
	}
};

class TaskBuildInfo {
	const Gpu* m_gpu;
	uint32_t m_buffer_idx;
	uint32_t m_num_buffers;

	VkViewport m_viewport;
	VkRenderPass m_renderpass;

	std::unordered_map<std::string, ImageResource> m_resources;

public:
	GET(m_gpu, gpu);
	GET(m_num_buffers, num_buffers);
	GET(m_buffer_idx, buffer_idx);
	GET(m_viewport, viewport);
	GET(m_renderpass, renderpass);

	inline ImageResource get_resource(
			const std::string& name
	) const {
		return m_resources.find(name)->second;
	}

	TaskBuildInfo(
			const Gpu* gpu,
			uint32_t buffer_idx,
			uint32_t num_buffers,
			VkViewport viewport,
			VkRenderPass renderpass,
			std::unordered_map<std::string, ImageResource> resources) :
		m_gpu(gpu),
		m_buffer_idx(buffer_idx),
		m_num_buffers(num_buffers),
		m_viewport(viewport),
		m_renderpass(renderpass),
		m_resources(resources) {
	}
};

enum TaskType {
	GRAPHICS_TASK,
	COMPUTE_TASK,
	RAY_TRACING_TASK
};


struct TaskInfo {
	typedef std::function<void(const TaskBuildInfo&, void*)> TaskBuildFunc;
	typedef std::function<void(const TaskRecordInfo&, void*)> TaskRecordFunc;

	std::string m_name;
	TaskType m_type;

	void *m_pContext;
	TaskBuildFunc m_build_func;
	TaskRecordFunc m_record_func;

	std::vector<std::string> m_dependencies;
	std::vector<std::string> m_recording_dependencies;

	std::vector<BufferResourceDescription> m_buffer_outputs;
	std::vector<ImageResourceDescription> m_color_outputs;
	std::optional<ImageResourceDescription> m_depth_output;

	VkExtent2D m_extent;

	REF(m_name, name);
	GET(m_type, type);
	REF(m_build_func, build_func);
	REF(m_record_func, record_func);
	REF(m_dependencies, dependencies);
	REF(m_recording_dependencies, recording_dependencies);
	REF(m_buffer_outputs, buffer_outputs);
	REF(m_color_outputs, color_outputs);
	REF(m_depth_output, depth_output);

	TaskInfo() {
	}

	template<typename T>
	TaskInfo(const std::string& name,
			TaskType type,
			T *pContext,
			std::function<void(const TaskBuildInfo&, T*)> build_func,
			std::function<void(const TaskRecordInfo&, T*)> record_func
	) :
		m_name(name),
		m_type(type),
		m_pContext(pContext),
		m_build_func(build_func),
		m_record_func(record_func),
		m_extent(0, 0)
	{

	}

    bool has_output(const std::string& name) const {
        if(m_depth_output.has_value() && m_depth_output->name() == name) {
            return true;
        }

        if(std::any_of(m_buffer_outputs.begin(), m_buffer_outputs.end(),
                [name](const BufferResourceDescription& output) {
                    return output.name() == name;
                })) {
            return true;
        }    

        if(std::any_of(m_color_outputs.begin(), m_color_outputs.end(),
                [name](const ImageResourceDescription& output) {
                    return output.name() == name;
                })) {
            return true;
        }

        return false;
    }

	TaskInfo& add_color_output(const std::string& name,
			VkFormat format,
			VkExtent2D extent,
			VkClearColorValue clear_value) {
		m_color_outputs.emplace_back(name, format, extent,
				VkClearValue {
						.color = clear_value
				}, true);
		return *this;
	}

	TaskInfo& set_depth_output(
			const std::string& name,
			VkFormat format,
			VkExtent2D extent,
			VkClearDepthStencilValue clear_value
	) {
		m_depth_output = ImageResourceDescription(name, format, extent,
				VkClearValue {
						.depthStencil = clear_value
				}, false);
		return *this;
	}

	TaskInfo& add_dependency(const std::string& dependency) {
		m_dependencies.emplace_back(dependency);
		return *this;
	}

	TaskInfo& add_recording_dependency(const std::string& dependency) {
		m_recording_dependencies.emplace_back(dependency);
		return *this;
	}

	TaskInfo& set_extent(VkExtent2D extent) {
		m_extent = extent;
		return *this;
	}

	bool equals(const TaskInfo& other) const {
	    if(this->name() != other.name()) {
			std::cout << "Names are not the same: " << name() << " != " << other.name() << std::endl;
			return false;
		}

		if(this->m_type != other.m_type) {
		    std::cout << "Task types are not the same: " << m_type << " != " << other.m_type << std::endl;
		    return false;
		}

		if(m_dependencies != other.m_dependencies) {
		    std::cout << "Dependencies are different" << std::endl;
			return false;
		}

		if(m_buffer_outputs.size() != other.m_buffer_outputs.size()) {
		    return false;
		}

		for(uint32_t i = 0; i < m_buffer_outputs.size(); i++) {
		    auto output = other.m_buffer_outputs[i];
		    auto found = std::find_if(
						m_buffer_outputs.begin(),
						m_buffer_outputs.end(),
						[output](const BufferResourceDescription& desc) {
						    return output.equals(desc);
						});

			if(found == other.m_buffer_outputs.end()) {
			    std::cout << "Missing buffer output: " << output.name() << std::endl;
				return false;
			}
		}

		for(uint32_t i = 0; i < m_color_outputs.size(); i++) {
		    auto output = other.m_color_outputs[i];
		    auto found = std::find_if(
						m_color_outputs.begin(),
						m_color_outputs.end(),
						[output](const ImageResourceDescription& desc) {
						    return output.equals(desc);
						});

			if(found == other.m_color_outputs.end()) {
			    std::cout << "Missing color output: " << output.name() << std::endl;
				return false;
			}
		}

		if(m_depth_output.has_value() != other.m_depth_output.has_value()) {
		    std::cout << "Depth output differ" << std::endl;
		    return false;
		}

		if(m_depth_output.has_value() && !m_depth_output->equals(other.m_depth_output.value())) {
            std::cout << "Depth output differ" << std::endl;
		    return false;
		}

		if(m_extent.width != other.m_extent.width ||
		    m_extent.height != other.m_extent.height) {
			std::cout << std::format("Extent differ: [{},{}] != [{},{}]", m_extent.width, m_extent.height, other.m_extent.width, other.m_extent.height) << std::endl;
			return false;
		}

		return true;
	}
};

class ComputeTaskBuilder {
private:
	TaskInfo m_task_info;

public:
	ComputeTaskBuilder(const std::string& name,
			void *pContext,
			std::function<void(const TaskBuildInfo&, void*)> build_func,
			std::function<void(const TaskRecordInfo&, void*)> record_func
	) :
		m_task_info(name, COMPUTE_TASK, pContext, build_func, record_func) {
	}

	ComputeTaskBuilder& add_buffer_output(const std::string& name,
			VkDeviceSize size) {
		m_task_info.m_buffer_outputs.emplace_back(name, size);
		return *this;
	}

	ComputeTaskBuilder& add_dependency(const std::string& dependency) {
		m_task_info.m_dependencies.emplace_back(dependency);
		return *this;
	}

	ComputeTaskBuilder& add_recording_dependency(const std::string& dependency) {
		m_task_info.m_recording_dependencies.emplace_back(dependency);
		return *this;
	}

	TaskInfo build() {
		return m_task_info;
	}
};



class RenderTaskBuilder {
private:
	TaskInfo m_task_info;

public:
	RenderTaskBuilder(const std::string& name,
			void* pContext,
			std::function<void(const TaskBuildInfo&, void*)> build_func,
			std::function<void(const TaskRecordInfo&, void*)> record_func
	) :
		m_task_info(name, GRAPHICS_TASK, pContext, build_func, record_func) {
	}

	RenderTaskBuilder& add_color_output(const std::string& name,
			VkFormat format,
			VkExtent2D extent = VkExtent2D(0.0f, 0.0f),
			VkClearColorValue clear_value = {0.0f, 0.0f, 0.0f, 0.0f}) {
		m_task_info.m_color_outputs.emplace_back(name, format, extent,
				VkClearValue {
						.color = clear_value
				}, true);
		return *this;
	}

	RenderTaskBuilder& set_depth_output(
			const std::string& name,
			VkFormat format,
			VkExtent2D extent = VkExtent2D(0.0f, 0.0f),
			VkClearDepthStencilValue clear_value = {1.0f, 0}
	) {
		m_task_info.m_depth_output = ImageResourceDescription(name, format, extent,
				VkClearValue {
						.depthStencil = clear_value
				}, false);
		return *this;
	}

	RenderTaskBuilder& add_dependency(const std::string& dependency) {
		m_task_info.m_dependencies.emplace_back(dependency);
		return *this;
	}

	RenderTaskBuilder& add_recording_dependency(const std::string& dependency) {
		m_task_info.m_recording_dependencies.emplace_back(dependency);
		return *this;
	}

	RenderTaskBuilder& set_extent(VkExtent2D extent) {
		m_task_info.m_extent = extent;
		return *this;
	}

	TaskInfo build() {
		return m_task_info;
	}

};

template<typename T>
RenderTaskBuilder render_task(const std::string& name,
			T* pContext,
			std::function<void(const TaskBuildInfo&, T*)> build_func,
			std::function<void(const TaskRecordInfo&, T*)> record_func
) {
	return RenderTaskBuilder(name, (void*)pContext,
			[build_func, pContext](const TaskBuildInfo& info, void* ctx) {
				build_func(info, pContext);
			},
			[record_func, pContext](const TaskRecordInfo& info, void* ctx) {
				record_func(info, pContext);
			});
}


template<typename T>
ComputeTaskBuilder compute_task(const std::string& name,
			T* pContext,
			std::function<void(const TaskBuildInfo&, T*)> build_func,
			std::function<void(const TaskRecordInfo&, T*)> record_func
) {
	return ComputeTaskBuilder(name, (void*)pContext,
			[build_func, pContext](const TaskBuildInfo& info, void* ctx) {
				build_func(info, pContext);
			},
			[record_func, pContext](const TaskRecordInfo& info, void* ctx) {
				record_func(info, pContext);
			});
}

}
