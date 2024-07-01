//
// Created by martin on 6/10/24.
//

#ifndef LOFT_MATERIALDATA_H
#define LOFT_MATERIALDATA_H

#include <optional>
#include <cstdint>

struct MaterialData {
private:
    std::optional<uint32_t> m_colorTexture;
    std::optional<uint32_t> m_metallicRoughnessTexture;
    std::optional<uint32_t> m_normalTexture;

    float m_albedo[4]{};
    float m_bsdf[4]{};

    float m_colorTextureBlend;
    float m_normalTextureBlend;
    float m_pbrTextureBlend;

    bool m_isBlend;

public:
    [[nodiscard]] inline std::optional<uint32_t> color_texture() const {
        return m_colorTexture;
    }

    [[nodiscard]] inline std::optional<uint32_t> normal_texture() const {
        return m_normalTexture;
    }

    [[nodiscard]] inline std::optional<uint32_t> metallic_roughness_texture() const {
        return m_metallicRoughnessTexture;
    }

    [[nodiscard]] inline const float* albedo() const {
        return &m_albedo[0];
    }

    void set_albedo(float albedo[4]) {
        m_albedo[0] = albedo[0];
        m_albedo[1] = albedo[1];
        m_albedo[2] = albedo[2];
        m_albedo[3] = albedo[3];
    }

    [[nodiscard]] inline const float* bsdf() const {
        return &m_bsdf[0];
    }

    [[nodiscard]] inline const bool is_blended() const {
        return m_isBlend;
    }


    inline MaterialData& set_alpha_blend(bool value) {
        this->m_isBlend = value;
        return *this;
    }

    inline MaterialData& set_color_texture(const uint32_t idx) {
        m_colorTexture = idx;
        return *this;
    }

    inline MaterialData& set_normal_texture(const uint32_t idx) {
        m_normalTexture = idx;
        return *this;
    }

    inline MaterialData& set_metallic_roughness_texture(const uint32_t idx) {
        m_metallicRoughnessTexture = idx;
        return *this;
    }

    MaterialData() :
            m_isBlend(false) {

    }

    explicit MaterialData(float albedo[4]) : MaterialData() {
        set_albedo(albedo);
    }
};

#endif //LOFT_MATERIALDATA_H
