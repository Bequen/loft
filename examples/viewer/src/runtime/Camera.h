#pragma once

#include <cstring>
#include "Gpu.hpp"
#include "cglm/mat4.h"
#include "cglm/cglm.h"
#include "ResourceLayout.hpp"

class Camera {
private:
    Gpu *m_pGpu;

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

    BufferResourceLayout m_resource;

    Camera(Gpu *pGpu);

    void update();

    void move(vec3 velocity);

    void rotate(float dx, float dy);
};
