#pragma once

#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <memory>
#include <iostream>

#include "Gpu.hpp"
#include "props.hpp"
#include "Resource.hpp"

#include <volk/volk.h>
#include <map>

namespace lft::rg {

class ImageResourceDefinition {
private:
	std::string m_name;
	VkFormat m_format;
	VkExtent2D m_extent;
	VkClearValue m_clear_value;

public:
	REF(m_name, name);
	GET(m_format, format);
	GET(m_extent, extent);
	GET(m_clear_value, clear_value);

	ImageResourceDefinition(
			const std::string& name,
			VkFormat format,
			VkExtent2D extent,
			VkClearValue clear_value
		) :
		m_name(name),
		m_format(format),
		m_extent(extent),
		m_clear_value(clear_value) {
	}
};

class RenderPassRecordInfo {
	std::shared_ptr<const Gpu> m_gpu;
	VkCommandBuffer m_command_buffer;
	uint32_t m_buffer_idx;
	uint32_t m_image_idx;

public:
	GET(m_gpu, gpu);
	GET(m_command_buffer, command_buffer);
	GET(m_image_idx, image_idx);
	GET(m_buffer_idx, buffer_idx);

	RenderPassRecordInfo(
			std::shared_ptr<const Gpu> gpu,
			VkCommandBuffer command_buffer,
			uint32_t buffer_idx,
			uint32_t image_in_flight_idx) :
		m_command_buffer(command_buffer),
		m_image_idx(image_in_flight_idx),
		m_buffer_idx(buffer_idx),
		m_gpu(gpu) {
	}
};

class RenderPassBuildInfo {
	std::shared_ptr<const Gpu> m_gpu;
	uint32_t m_num_buffers;

	VkViewport m_viewport;
	VkRenderPass m_renderpass;

	std::vector<std::map<std::string, ImageResource>> m_resources;

public:
	GET(m_gpu, gpu);
	GET(m_num_buffers, num_buffers);
	GET(m_viewport, viewport);
	GET(m_renderpass, renderpass);

	inline ImageResource get_resource(const std::string& name, uint32_t buffer_idx) const {
		std::cout << "Getting resource " << name << " in buffer " << buffer_idx << std::endl;
		return m_resources[buffer_idx].find(name)->second;
	}

	RenderPassBuildInfo(
			std::shared_ptr<const Gpu> gpu,
			uint32_t num_buffers,
			VkViewport viewport,
			VkRenderPass renderpass,
			std::vector<std::map<std::string, ImageResource>> resources) :
		m_gpu(gpu),
		m_num_buffers(num_buffers),
		m_viewport(viewport),
		m_renderpass(renderpass),
		m_resources(resources) {
	}
};


class GpuTask {

private:
	std::string m_name;
	std::vector<std::string> m_dependencies;
	std::vector<std::string> m_recording_dependencies;

	std::vector<ImageResourceDefinition> m_outputs;
	std::optional<ImageResourceDefinition> m_depth_output;

	VkExtent2D m_extent;


public:
	REF(m_name, name);
	REF(m_dependencies, dependencies);
	REF(m_recording_dependencies, recording_dependencies);

	REF(m_outputs, outputs);
	REF(m_depth_output, depth_output);
	GET(m_extent, extent);

	void set_extent(VkExtent2D extent) {
		m_extent = extent;
	}

	virtual void record(const RenderPassRecordInfo& info) const = 0;
	virtual void build(const RenderPassBuildInfo& info) const = 0;

	void add_color_output(const std::string& name,
			VkFormat format,
			VkExtent2D extent, 
			VkClearColorValue clear_value) {
		m_outputs.emplace_back(name, format, extent,
				VkClearValue {
						.color = clear_value
				});
	}

	void add_dependency(const std::string& dependency) {
		m_dependencies.push_back(dependency);
	}

	void add_recording_dependency(const std::string& dependency) {
		m_recording_dependencies.push_back(dependency);
	}

	void set_depth_output(
			const std::string& name,
			VkFormat format,
			VkExtent2D extent,
			VkClearDepthStencilValue clear_value
	) {
		m_depth_output = ImageResourceDefinition(name, format, extent,
				VkClearValue {
						.depthStencil = clear_value
				});
	}


protected:
	GpuTask(const std::string& name) :
		m_name(name),
		m_depth_output({}){
	}
};

template<typename T>
class LambdaGpuTask : public GpuTask {
	typedef std::function<void(const RenderPassRecordInfo&, T*)> RenderPassRecordFunc;
	typedef std::function<void(const RenderPassBuildInfo&, T*)> RenderPassBuildFunc;

	T* m_context;

	RenderPassRecordFunc m_record_func;
	RenderPassBuildFunc m_build_func;


public:
	void record(const RenderPassRecordInfo& info) const override {
		m_record_func(info, m_context);
	}

	void build(const RenderPassBuildInfo& info) const override {
		m_build_func(info, m_context);
	}

	LambdaGpuTask(const std::string& name, T* context,
			RenderPassBuildFunc build_func,
			RenderPassRecordFunc record_func) :
		GpuTask(name),
		m_context(context),
		m_record_func(record_func),
		m_build_func(build_func) {
	}
};

}
