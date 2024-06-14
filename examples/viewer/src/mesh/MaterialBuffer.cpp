#include <valarray>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include "MaterialBuffer.h"
#include "resources/GpuAllocator.h"
#include "resources/ImageBusWriter.h"


#include "resources/MipmapGenerator.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

unsigned char *load_image_data(const std::string& path, int32_t *pOutWidth, int32_t *pOutHeight, int32_t *pOutNumChannels, int32_t numChannels) {
    unsigned char *data = stbi_load(path.c_str(), pOutWidth, pOutHeight, pOutNumChannels, numChannels);

    return data;
}

unsigned short *load_image_data_16(const std::string& path, int32_t *pOutWidth, int32_t *pOutHeight, int32_t *pOutNumChannels, int32_t numChannels) {
    unsigned short *data = stbi_load_16(path.c_str(), pOutWidth, pOutHeight, pOutNumChannels, numChannels);
    return data;
}


/* uint32_t count_color_textures(SceneData *pSceneData) {
    uint32_t count = 0;
    for(uint32_t mi = 0; mi < pSceneData->num_materials(); mi++) {
        if(pSceneData->materials()[mi].color_texture().has_value()) {
           count++;
        }
    }

    return count < 2 ? 2 : count;
}

void MaterialBuffer::allocate_color_texture_array(Gpu *pGpu, VkExtent2D extent, uint32_t count) {
	MemoryAllocationInfo memoryAllocationInfo = {
		.usage = MEMORY_USAGE_AUTO_PREFER_DEVICE
	};

    uint32_t mipLevels = std::floor(std::log2(std::max(extent.width, extent.height)));

    ImageCreateInfo colorTextureInfo = {
            .extent = extent,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | 
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .arrayLayers = count <= 1 ? 2 : count,
            .mipLevels = mipLevels
    };

    pGpu->memory()->create_image(&colorTextureInfo, &memoryAllocationInfo,
								 &m_colorTexture);

    colorTextureInfo.format = VK_FORMAT_R16G16B16A16_UNORM;
    pGpu->memory()->create_image(&colorTextureInfo, &memoryAllocationInfo, 
								 &m_normalTexture);

    colorTextureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    pGpu->memory()->create_image(&colorTextureInfo, &memoryAllocationInfo, 
								 &m_pbrTexture);
}

void transfer_layout(Gpu *pGpu, const std::vector<Image*>& images,
                     VkImageSubresourceRange range,
                     VkCommandBuffer stagingCommandBuffer,
                     VkImageLayout oldLayout, 
					 VkImageLayout newLayout,
                     VkPipelineStageFlags srcStageMask,
					 VkPipelineStageFlags dstStageMask) {
    VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(stagingCommandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .subresourceRange = range
    };

    std::vector<VkImageMemoryBarrier> barriers(images.size(), barrier);
    for(int i = 0; i < images.size(); i++) {
        barriers[i].image = images[i]->img;
    }

    vkCmdPipelineBarrier(
            stagingCommandBuffer,
            srcStageMask, dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            barriers.size(), barriers.data()
    );

    vkEndCommandBuffer(stagingCommandBuffer);

    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &stagingCommandBuffer,
    };

    pGpu->enqueue_transfer(&submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(pGpu->transfer_queue());
} **/

void MaterialBuffer::allocate_for(const SceneData *pSceneData) {
    MemoryAllocationInfo memoryAllocationInfo = {
            .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    };

    BufferCreateInfo materialsBufferInfo = {
            .size = sizeof(Material) * pSceneData->num_materials(),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true,
    };

    m_pGpu->memory()->create_buffer(&materialsBufferInfo, &memoryAllocationInfo,
                                  &m_materialBuffer);
    m_materialBuffer.set_debug_name(m_pGpu, "material_buffer");
}

MaterialBuffer::MaterialBuffer(Gpu *pGpu, const SceneData *pSceneData) :
m_pGpu(pGpu),

m_materials(pSceneData->num_materials()),
m_materialsValidity(pSceneData->num_materials(), false),

m_textures(pSceneData->num_textures()),
m_textureValidity(pSceneData->num_textures(), false),
m_textureStorage(pGpu, pSceneData->num_textures()) {
    // allocate
    allocate_for(pSceneData);

    // push
    push_materials(pSceneData->materials(), pSceneData->textures());
}


/*
MaterialBuffer::MaterialBuffer(Gpu *pGpu, VkExtent2D textureLayerExtent, SceneData *pSceneData) :
    m_pGpu(pGpu) {

	/* VkExtent2D extent = textureLayerExtent;
	VkCommandBuffer cmdbuf = VK_NULL_HANDLE; //create_staging_command_buffer(pGpu);
    uint32_t mipLevels = std::floor(std::log2(std::max(extent.width, extent.height)));

    uint32_t numColorTextures = count_color_textures(pSceneData);
    allocate_color_texture_array(pGpu, extent, numColorTextures);

    std::vector<Image*> images = {
            &m_colorTexture,
            &m_normalTexture,
            &m_pbrTexture
    };

    VkImageSubresourceRange range = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = numColorTextures,
    };

    transfer_layout(pGpu, images, range,
        cmdbuf,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    std::vector<Material> materials(pSceneData->num_materials());

    uint32_t idx = 0;
    int32_t colorTextureIdx = 0;
    int32_t normalTextureIdx = 0;
    int32_t pbrTextureIdx = 0;

    VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = (uint32_t)colorTextureIdx,
                    .layerCount = 1,
            },

            .imageOffset = { 0, 0, 0 },
            .imageExtent = {
                    extent.width, extent.height, 1
            }
    };

	auto colorImageWriter = ImageBusWriter(m_pGpu, &m_colorTexture, extent,
										   4, 1);
	auto normalImageWriter = ImageBusWriter(m_pGpu, &m_normalTexture, extent,
											8, 1);
	auto pbrImageWriter = ImageBusWriter(m_pGpu, &m_pbrTexture, extent,
										 4, 1);

    int32_t width, height, numChannels = 4;
    for(const auto& materialData : pSceneData->materials()) {
        if(materialData.color_texture().has_value()) {
			uint32_t texture = materialData.color_texture().value();
			std::string path = pSceneData->textures()[texture].m_path;
			write_image(path, &colorImageWriter, colorTextureIdx++);
        }

        if(materialData.normal_texture().has_value()) {
            unsigned short* pImageData = nullptr; //load_image_data_16(pSceneData->textures()[materialData.normal_texture().value()].m_path, &width, &height, &numChannels, 4);
            region.imageExtent.width = width;
            region.imageExtent.height = height;
            region.imageSubresource.baseArrayLayer = normalTextureIdx;

            // normalImageWriter.write(region, pImageData);
            normalTextureIdx++;
        }

        if(materialData.metallic_roughness_texture().has_value()) {
            unsigned char* pImageData = nullptr;// load_image_data(pSceneData->textures()[materialData.metallic_roughness_texture().value()].m_path, &width, &height, &numChannels, 4);
            region.imageExtent.width = width;
            region.imageExtent.height = height;
            region.imageSubresource.baseArrayLayer = pbrTextureIdx;

            // pbrImageWriter.write(region, pImageData);

            pbrTextureIdx++;
        }

        materials[idx] = Material {
            .colorTexture = materialData.color_texture().has_value() ? colorTextureIdx - 1 : -1,
            .colorTextureBlend = materialData.color_texture().has_value() ? 1.0f : 0.0f,
            .normalTexture = materialData.normal_texture().has_value() ? normalTextureIdx - 1 : -1,
            .normalTextureBlend = materialData.normal_texture().has_value() ? 1.0f : 0.0f,
            .pbrTexture = materialData.metallic_roughness_texture().has_value() ? pbrTextureIdx - 1 : -1,
            .pbrTextureBlend = materialData.metallic_roughness_texture().has_value() ? 1.0f : 0.0f,
        };
        memcpy(materials[idx].albedo, materialData.albedo(), sizeof(vec4));
        memcpy(materials[idx].bsdf, materialData.bsdf(), sizeof(vec4));

        idx++;
    }

    colorImageWriter.flush();
    normalImageWriter.flush();
    pbrImageWriter.flush();



    auto mipmapGenerator = MipmapGenerator(pGpu);
    mipmapGenerator.generate(m_colorTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             extent, range);
    mipmapGenerator.generate(m_normalTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             extent, range);
    mipmapGenerator.generate(m_pbrTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             extent, range);

    /* transfer back to old layout */
    /* transfer_layout(pGpu, images, range, cmdbuf,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);

	MemoryAllocationInfo memoryAllocationInfo = {
			.usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
			.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	};

    BufferCreateInfo materialsBufferInfo = {
            .size = sizeof(Material) * 128,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true,
    };
    pGpu->memory()->create_buffer(&materialsBufferInfo, &memoryAllocationInfo,
								  &m_materials);

    m_materialBufferSize = materialsBufferInfo.size;

    void *pData = nullptr;
    pGpu->memory()->map(m_materials.allocation, &pData);
    memcpy(pData, reinterpret_cast<const void*>(materials.data()), materials.size() * sizeof(Material));
    pGpu->memory()->unmap(m_materials.allocation);

    VkImageSubresourceRange subresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = numColorTextures,
    };
    m_colorTextureView = m_colorTexture.create_view(pGpu, VK_FORMAT_R8G8B8A8_UNORM, subresource);
    m_normalTextureView = m_normalTexture.create_view(pGpu, VK_FORMAT_R16G16B16A16_UNORM, subresource);
    m_pbrTextureView = m_pbrTexture.create_view(pGpu, VK_FORMAT_R8G8B8A8_UNORM, subresource);

    VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .minLod = 0.0f,
            .maxLod = (float)mipLevels,
    };

    if(vkCreateSampler(pGpu->dev(), &samplerInfo, nullptr, &m_sampler)) {
        throw std::runtime_error("failed to create sampler");
    }
} */


uint32_t MaterialBuffer::upload_texture(const TextureData& texture, VkFormat format) {
    int32_t width, height, numChannels;

    unsigned char* pImageData = nullptr;
    uint32_t sizeScaler = 1;
    if(false) {
        pImageData = (unsigned char*)load_image_data_16(texture.m_path, &width, &height, &numChannels, 4);
        format = VK_FORMAT_R8G8B8A8_UNORM;
        sizeScaler = 2;
    } else {
        pImageData = load_image_data(texture.m_path, &width, &height, &numChannels, 4);
    }

    uint32_t idx = m_textureStorage.upload((unsigned char*)pImageData,
                                          width, height, width * height * 4 * sizeScaler,
                                          format);

    stbi_image_free(pImageData);

    /* m_textures.reserve(idx);
    m_textures[idx] = Texture(texture.m_name, m_textureStorage.view(idx).view); */

    return idx;
}



std::vector<uint32_t>
MaterialBuffer::upload_textures(const std::vector<MaterialData>& materials,
                                const std::vector<TextureData> &textures) {
    std::vector<uint32_t> mapping(textures.size());
    std::vector<bool> uploadFlags(textures.size(), false);

    for(auto& material : materials) {
        if (material.normal_texture().has_value() and
            not uploadFlags[material.normal_texture().value()]) {
            uint32_t textureIdx = material.normal_texture().value();

            mapping[textureIdx] = upload_texture(textures[textureIdx],
                                                 VK_FORMAT_R8G8B8A8_UNORM);
            uploadFlags[textureIdx] = true;
        }

        if (material.color_texture().has_value() and
            not uploadFlags[material.color_texture().value()]) {
            uint32_t textureIdx = material.color_texture().value();

            mapping[textureIdx] = upload_texture(textures[textureIdx],
                                                 VK_FORMAT_R8G8B8A8_SRGB);
            uploadFlags[textureIdx] = true;
        }

        if (material.metallic_roughness_texture().has_value() and
            not uploadFlags[material.metallic_roughness_texture().value()]) {
            uint32_t textureIdx = material.metallic_roughness_texture().value();

            mapping[textureIdx] = upload_texture(textures[textureIdx],
                                                 VK_FORMAT_R8G8B8A8_UNORM);
            uploadFlags[textureIdx] = true;
        }
    }

    return mapping;
}

uint32_t
MaterialBuffer::upload_material_data(const MaterialData& materialData,
                                     const std::vector<uint32_t>& textureMapping) {
    auto found = std::ranges::find(m_materialsValidity, false);
    auto distance = std::distance(m_materialsValidity.begin(), found);

    Material material = {};
    memcpy(material.albedo, materialData.albedo(), sizeof(vec4));
    memcpy(material.bsdf, materialData.bsdf(), sizeof(vec4));

    if(materialData.color_texture().has_value()) {
        material.colorTexture = textureMapping[materialData.color_texture().value()];
        material.colorTextureBlend = 1.0f;
    } if(materialData.normal_texture().has_value()) {
        material.normalTexture = textureMapping[materialData.normal_texture().value()];
        material.normalTextureBlend = 1.0f;
    } if(materialData.metallic_roughness_texture().has_value()) {
        material.pbrTexture = textureMapping[materialData.metallic_roughness_texture().value()];
        material.pbrTextureBlend = 1.0f;
    }

    set_material(distance, material);

    return distance;
}

std::vector<uint32_t>
MaterialBuffer::upload_materials_data(const std::vector<MaterialData>& materials,
                                      const std::vector<uint32_t>& textureMapping) {
    std::vector<uint32_t> mappings(materials.size());

    uint32_t i = 0;
    for(auto& material : materials) {
        mappings[i++] = upload_material_data(material, textureMapping);
    }

    uint32_t min = *std::min_element(mappings.begin(), mappings.end());
    uint32_t max = *std::max_element(mappings.begin(), mappings.end());

    char *pData;
    m_pGpu->memory()->map(m_materialBuffer.allocation, (void**)&pData);
    memcpy(pData + sizeof(Material) * min, &m_materials[min], sizeof(Material) * (max - min + 1));
    m_pGpu->memory()->unmap(m_materialBuffer.allocation);

    return mappings;
}

std::vector<uint32_t>
MaterialBuffer::push_materials(const std::vector<MaterialData> &materials,
                               const std::vector<TextureData> &textures) {
    auto textureMap = upload_textures(materials, textures);
    return upload_materials_data(materials, textureMap);
}
