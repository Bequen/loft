#include "resources/Buffer.hpp"
#include "resources/Image.hpp"
#include "Gpu.hpp"
#include "data/SceneData.hpp"
#include "data/TextureData.h"
#include "data/MaterialData.h"
#include "runtime/Material.h"
#include "runtime/Texture.h"
#include "TextureStorage.h"

/**
 * @brief MaterialBuffer class
 */
class MaterialBuffer {
public:
    std::shared_ptr<const Gpu> m_gpu;

    std::vector<Material> m_materials;
    std::vector<bool> m_materialsValidity;
    Buffer m_materialBuffer;

    std::vector<Texture> m_textures;
    std::vector<bool> m_textureValidity;
    TextureStorage m_textureStorage;

    void allocate_for(const SceneData *pSceneData);

public:
    /**
     * Size in bytes of the material data buffer
     * @return Returns the size in bytes
     */
    [[nodiscard]] size_t material_buffer_size() const {
        return m_materials.size() * sizeof(Material);
    }

    [[nodiscard]] const Buffer* material_buffer() const {
        return &m_materialBuffer;
    }

    inline const void set_material(uint32_t idx, Material material) {
        m_materials[idx] = material;
        m_materialsValidity[idx] = true;
    }

    /**
     * Create a new MaterialBuffer for the given Gpu and SceneData and stores the textures and material data.
     * @brief Constructor
     * @param pGpu
     * @param pSceneData
     */
    MaterialBuffer(const std::shared_ptr<const Gpu>& gpu, const SceneData *pSceneData);

    /**
     * @brief Copy constructor is disabled
     * @param other
     */
    MaterialBuffer(const MaterialBuffer& other) = delete;

    ~MaterialBuffer();

    uint32_t
    upload_texture(const TextureData& texture, VkFormat format);

    std::vector<uint32_t>
    upload_textures(const std::vector<MaterialData>& materials,
                    const std::vector<TextureData> &textures);

    uint32_t
    upload_material_data(const MaterialData& materialData,
                         const std::vector<uint32_t>& textureMapping);

    std::vector<uint32_t>
    upload_materials_data(const std::vector<MaterialData>& materials,
                          const std::vector<uint32_t>& textureMapping);

    std::vector<uint32_t>
    push_materials(const std::vector<MaterialData> &materials,
                   const std::vector<TextureData> &textures);
};
