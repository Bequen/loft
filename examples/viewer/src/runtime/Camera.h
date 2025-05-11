#pragma once

#include <cstring>
#include "Gpu.hpp"
#include "cglm/mat4.h"
#include "cglm/cglm.h"

class Camera {
private:
    std::shared_ptr<const Gpu> m_gpu;

    struct {
        mat4 projection;
        mat4 view;
        vec4 position;
    } data;

    Buffer m_buffer;

    vec3 m_position;
    vec4 m_rotation;

public:
    [[nodiscard]] inline const Buffer& buffer() const {
        return m_buffer;
    }

    // BufferResourceLayout m_resource;

    Camera(const std::shared_ptr<const Gpu>& gpu, float aspect);

    void update();

    void move(vec3 velocity);

    void rotate(float dx, float dy);
};
