#include "resources/Buffer.hpp"
#include "cglm/vec4.h"
#include "resources/Image.hpp"
#include "Gpu.hpp"
#include "SceneData.hpp"


struct Material {
    vec4 albedo;
    vec4 bsdf;

    int32_t colorTexture;
    float colorTextureBlend;

    int32_t normalTexture;
    float normalTextureBlend;

    int32_t pbrTexture;
    float pbrTextureBlend;
};

class MaterialBuffer {
public:
    Gpu *m_pGpu;

    Buffer m_materials;

    Image m_colorTexture;
    Image m_normalTexture;
    Image m_pbrTexture;

    ImageView m_colorTextureView;
    ImageView m_normalTextureView;
    ImageView m_pbrTextureView;

    VkSampler m_sampler;

    size_t m_materialBufferSize;

	void create_sampler(float numMipmapLevels);

    void allocate_color_texture_array(Gpu *pGpu, VkExtent2D extent, uint32_t count);

public:
    GET(m_colorTextureView, color_texture);
    GET(m_sampler, sampler);
    GET(m_normalTextureView, normal_texture);
    GET(m_pbrTextureView, pbr_texture);

    const size_t material_buffer_size() const {
        return m_materialBufferSize;
    }

    [[nodiscard]] const Buffer* material_buffer() const {
        return &m_materials;
    }

    MaterialBuffer(Gpu *pGpu, VkExtent2D textureLayerExtent, SceneData *pSceneData);
};
