#pragma once

#ifndef MOCK_DEFINED
#define MOCK_DEFINED 1

#include <memory>

#include "Gpu.hpp"
#include "ImageChain.hpp"
#include "RenderGraphBuilder.hpp"

std::unique_ptr<Gpu> create_mock_gpu();

ImageChain create_mock_image_chain(
    const Gpu* gpu,
    uint32_t num_images,
    VkExtent2D extent,
    VkFormat format
);

struct EmptyContext { };

lft::rg::RenderTaskBuilder create_empty_task(const std::string& name);

#endif
