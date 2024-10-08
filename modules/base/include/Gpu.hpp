#pragma once

#include "result.hpp"
#include "props.hpp"
#include "Instance.hpp"
#include "GpuScheduler.h"
#include "resources/GpuAllocator.h"

#include <string>
#include <vector>
#include <memory>

/**
 * Abstract interface for a GPU
 */
class Gpu {
private:
    std::shared_ptr<const Instance> m_instance;
	VkPhysicalDevice m_gpu = VK_NULL_HANDLE;
	VkDevice m_dev = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool{};

    // Queue for a general commands
    VkQueue m_graphicsQueue{};
    uint32_t m_graphicsQueueIdx{};
    VkCommandPool m_graphicsCommandPool{};

    // Queue for a transfer commands
    VkQueue m_transferQueue{};
    uint32_t m_transferQueueIdx{};
    VkCommandPool m_transferCommandPool{};

    // Queue for present commands
    VkQueue m_presentQueue{};
    uint32_t m_presentQueueIdx{};
    VkCommandPool m_presentCommandPool{};

    VkCommandBuffer m_tracyCommandBuffer;

    std::vector<int32_t> get_queues(VkSurfaceKHR surface);

    Result create_logical_device(VkSurfaceKHR surface);

	Result choose_gpu(VkPhysicalDevice *pOut);

    Result create_descriptor_pool();

    std::unique_ptr<GpuAllocator> m_pAllocator;

public:
	GET(m_gpu, gpu);
	GET(m_dev, dev);

    [[nodiscard]] std::shared_ptr<const Instance> instance() const {
        return m_instance;
    }
    
	GET(m_descriptorPool, descriptor_pool);
    
	GET(m_graphicsCommandPool, graphics_command_pool);
    GET(m_graphicsQueue, graphics_queue);

	GET(m_transferCommandPool, transfer_command_pool);
	GET(m_transferQueue, transfer_queue);

    GET(m_tracyCommandBuffer, tracy_cmd_buf);

    GET(m_pAllocator.get(), memory);

	explicit 
	Gpu(std::shared_ptr<const Instance> instance, VkSurfaceKHR surface);

    ~Gpu();

    // Forbid copy
    Gpu(const Gpu&) = delete;

    static result<Gpu, Result> create(std::shared_ptr<const Instance> instance, VkSurfaceKHR surface);

    inline std::vector<uint32_t> present_queue_ids() const {
        return {m_presentQueueIdx};
    }

	inline uint32_t transfer_queue_idx() const {
		return m_transferQueueIdx;
	}


    void enqueue_present(VkPresentInfoKHR *pPresentInfo) const;
    void enqueue_graphics(VkSubmitInfo2 *pSubmitInfo, VkFence fence) const;
    void enqueue_transfer(VkSubmitInfo *pSubmitInfo, VkFence fence) const;
};
