#pragma once

#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include "volk/volk.h"
#include <optional>
#include <functional>
#include <iostream>
#include "ResourceLayout.hpp"
#include "RenderPassBuildInfo.hpp"
#include "RecordingDependency.h"


class RenderPass {
private:
	std::string m_name;

	std::vector<ResourceLayout*> m_outputs;
	ImageResourceLayout *m_pDepthOutput;

	std::vector<std::string> m_inputs;

    VkExtent2D m_extent;

    std::vector<std::string> m_passDependencies;

    std::shared_ptr<const RecordingDependency> m_recordingDependency;

public:
	/**
	 * Name of the renderpass - for debugging purposes
	 */
	REF(m_name, name);
    GET(m_extent, extent);
    GET(m_passDependencies, pass_dependencies);

    inline RenderPass& set_extent(const VkExtent2D extent) {
        m_extent = extent;
        return *this;
    }

    explicit RenderPass(std::string name) :
    m_name(std::move(name)), m_pDepthOutput(nullptr), m_extent({0, 0}) {

    }

    inline RenderPass& add_pass_dependency(const std::string& name) {
        std::cout << "Adding pass dependency" << std::endl;
        m_passDependencies.push_back(name);
        return *this;
    }

    /**
     * Prepares the renderpass
     * Gets called after the frame graph is built
     */
    virtual void prepare(RenderPassBuildInfo info) = 0;

	/**
	 * Gets called when command buffer has to be re-recorded
	 */
    virtual void record(RenderPassRecordInfo info) = 0;

    inline RenderPass& set_recording_dependency(std::shared_ptr<const RecordingDependency> dependency) {
        m_recordingDependency = dependency;
        return *this;
    }
    
	inline RenderPass& add_color_output(std::string name, VkFormat format, VkClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f }) {
        std::cout << "Adding color output" << name << std::endl;
        auto resource = new ImageResourceLayout(std::move(name), format);
        resource->set_clear_color_value(clearValue);
        m_outputs.push_back(resource);
		return *this;
	}

    inline RenderPass& set_depth_output(std::string name, VkFormat format, VkClearDepthStencilValue clearValue = { 1.0f, 0 }) {
        std::cout << "Adding depth output" << name << std::endl;
        auto resource = new ImageResourceLayout(std::move(name), format);
        resource->set_clear_depth_value(clearValue);
        m_pDepthOutput = resource;
        return *this;
    }

	inline const std::vector<ResourceLayout*>& outputs() const {
		return m_outputs;
	}

    inline const std::optional<ImageResourceLayout*> depth_output() const {
        if(m_pDepthOutput) {
            return m_pDepthOutput;
        }
        return {};
    }

    inline bool has_pass_dependency(const std::string& name) {
        return std::count(m_passDependencies.begin(), m_passDependencies.end(), name) > 0;
    }

	inline uint32_t num_outputs() const {
		return m_outputs.size();
	}

    inline uint32_t num_inputs() const {
        return m_inputs.size();
    }

	inline RenderPass& add_input(const std::string& name) {
		m_inputs.push_back(name);
		return *this;
	}

    inline const std::vector<std::string>& inputs() const {
		return m_inputs; 
	}

    bool has_input(const std::string& name) {
        for(auto & input : m_inputs) {
            if(input == name) {
                return true;
            }
        }

        return false;
    }

    std::optional<ResourceLayout*> output(const std::string& name) {
		for(auto & output : m_outputs) {
			if(output->name() == name) {
				return output;
			}
		}

        if(m_pDepthOutput != nullptr && m_pDepthOutput->name() == name) {
            return m_pDepthOutput;
        }

		return {};
	}
};

template <typename T>
class LambdaRenderPass : public RenderPass {
    std::function<void(T*, RenderPassBuildInfo)> m_onBuild;
    std::function<void(T*, RenderPassRecordInfo)> m_onRecord;
    T* m_pContext;

public:
    LambdaRenderPass(std::string name,
                     T* pContext,
                     std::function<void(T*, RenderPassBuildInfo)> onBuild,
                     std::function<void(T*, RenderPassRecordInfo)> onRecord) :
                     m_onBuild(onBuild), m_onRecord(onRecord), m_pContext(pContext), RenderPass(name) {
        std::cout << "Creating Lambda Render Pass: " << name << std::endl;
    }

    void prepare(RenderPassBuildInfo info) override {
        m_onBuild(m_pContext, info);
    }

    void record(RenderPassRecordInfo info) override {
        m_onRecord(m_pContext, info);
    }
};