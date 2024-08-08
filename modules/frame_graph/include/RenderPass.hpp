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

/**
 * Represents a render pass in the render graph
 */
class RenderPass {
private:
    /**
     * Name of the render pass
     */
	std::string m_name;

    /**
     * Outputs
     */
	std::vector<ResourceLayout*> m_outputs;
	ImageResourceLayout *m_pDepthOutput;

    /**
     * Dependencies
     */
	std::vector<std::string> m_inputs;

    /**
     * Output image extent
     */
    VkExtent2D m_extent;

    /**
     * Output dependencies
     */
    std::vector<std::string> m_passDependencies;

    /**
     * Recording dependencies
     */
    std::optional<std::string> m_recordingDependency;

public:
	/**
	 * Name of the renderpass - for debugging purposes
	 */
	REF(m_name, name);
    GET(m_extent, extent);
    GET(m_passDependencies, pass_dependencies);

    /**
     * Sets extent of output images
     * @param extent extent of the images
     * @return current instance of render pass
     */
    inline RenderPass& set_extent(const VkExtent2D extent) {
        m_extent = extent;
        return *this;
    }

    explicit RenderPass(std::string name) :
    m_name(std::move(name)), m_pDepthOutput(nullptr), m_extent({0, 0}) {

    }

    /**
     * Adds an output dependency. If the dependency is invalidated, this render pass and all those depending on it will be ru-run
     * @param name Name of the dependency
     * @return Current instance
     */
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

    /**
     * Adds color image output
     * @param name name of the resource
     * @param format format of the image
     * @param clearValue clear value for the image
     * @return Current instance
     */
	inline RenderPass& add_color_output(std::string name, VkFormat format, VkClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f }) {
        std::cout << "Adding color output" << name << std::endl;
        auto resource = new ImageResourceLayout(std::move(name), format);
        resource->set_clear_color_value(clearValue);
        m_outputs.push_back(resource);
		return *this;
	}

    /**
     * Adds depth image output
     * @param name name of the resource
     * @param format format of the image. Format must be of depth.
     * @param clearValue clear value for the image
     * @return Current instance
     */
    inline RenderPass& set_depth_output(std::string name, VkFormat format, VkClearDepthStencilValue clearValue = { 1.0f, 0 }) {
        std::cout << "Adding depth output" << name << std::endl;
        auto resource = new ImageResourceLayout(std::move(name), format);
        resource->set_clear_depth_value(clearValue);
        m_pDepthOutput = resource;
        return *this;
    }

    /**
     * Gets a reference to a vector of outputs
     * @return Returns a vector reference
     */
	inline const std::vector<ResourceLayout*>& outputs() const {
		return m_outputs;
	}

    /**
     * Gets depth resource output, if any
     * @return Optional type with depth resource. If depth output does not exist, returns none.
     */
    inline const std::optional<ImageResourceLayout*> depth_output() const {
        if(m_pDepthOutput) {
            return m_pDepthOutput;
        }
        return {};
    }

    inline bool has_pass_dependency(const std::string& name) {
        return std::count(m_passDependencies.begin(), m_passDependencies.end(), name) > 0;
    }

    /**
     * @return Returns number of outputs without depth output
     */
	inline uint32_t num_outputs() const {
		return m_outputs.size();
	}

    /**
     * @return Returns number of inputs
     */
    inline uint32_t num_inputs() const {
        return m_inputs.size();
    }

    /**
     * Adds dependency
     * @param name name of the dependency
     * @return current instance
     */
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

    /**
     * Checks if output resource with name exists and returns it if it does
     * @param name Name of the resource
     * @return Optional type. None means the resource does not exists.
     */
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

    inline const std::optional<std::string>& recording_dependency() const {
        return m_recordingDependency;
    }

    /**
     * Adds a recording dependency for the render pass
     * @param dependency Name of the dependency
     * @return Current instance
     */
    inline RenderPass& set_recording_dependency(const std::string& dependency) {
        m_recordingDependency = dependency;
        return *this;
    }
};

/**
 * Abstracts away prepare and record function to simplify coding new render passes.
 * @tparam T
 */
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