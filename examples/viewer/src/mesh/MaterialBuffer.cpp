#include <algorithm>
#include <volk.h>

#include <print>

#include "MaterialBuffer.h"
#include "resources/GpuAllocator.h"


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

    m_gpu->memory()->create_buffer(&materialsBufferInfo, &memoryAllocationInfo,
                                  &m_materialBuffer);
    m_materialBuffer.set_debug_name(m_gpu, "material_buffer");
}

MaterialBuffer::MaterialBuffer(const Gpu* gpu, const SceneData *pSceneData) :
    m_gpu(gpu),

    m_materials(pSceneData->num_materials()),
    m_materialsValidity(pSceneData->num_materials(), false),

    m_textures(pSceneData->num_textures()),
    m_textureValidity(pSceneData->num_textures(), false),
    m_textureStorage(gpu, pSceneData->num_textures())
{
    allocate_for(pSceneData);

    push_materials(pSceneData->materials(), pSceneData->textures());
}

MaterialBuffer::~MaterialBuffer() {
    vkDestroyBuffer(m_gpu->dev(), m_materialBuffer.buf, nullptr);
}

uint32_t MaterialBuffer::upload_texture(const TextureData& texture, VkFormat format) {
    int32_t width, height, numChannels;

    unsigned char* pImageData = nullptr;
    uint32_t sizeScaler = 1;
    pImageData = load_image_data(texture.m_path, &width, &height, &numChannels, 4);

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
    m_gpu->memory()->map(m_materialBuffer.allocation, (void**)&pData);
    memcpy(pData + sizeof(Material) * min,
           &m_materials[min],
           sizeof(Material) * (max - min + 1));
    m_gpu->memory()->unmap(m_materialBuffer.allocation);

    return mappings;
}

std::vector<uint32_t>
MaterialBuffer::push_materials(const std::vector<MaterialData> &materials,
                               const std::vector<TextureData> &textures) {
    auto textureMap = upload_textures(materials, textures);
    return upload_materials_data(materials, textureMap);
}
