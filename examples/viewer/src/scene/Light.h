#pragma once

#include "cglm/mat4.h"
#include "cglm/cam.h"

struct Light {
    mat4 view;
    vec4 color;
    vec4 position;

    static Light directional(float near, float far, vec3 position, vec3 direction, vec4 color) {
        mat4 projection = {};
        glm_mat4_identity(projection);
        glm_perspective(glm_rad(90.0f), 1.0f, 1.0f, 1000.0f, projection);

        mat4 view = {};
        glm_mat4_identity(view);

        vec3 center = {};
        glm_vec3_add(position, direction, center);

        vec3 up = {0.0f, 0.0f, 1.0f};
        glm_lookat(position, center, up, view);

        Light light = {};
        glm_mat4_mul(projection, view, light.view);
        glm_vec4_copy(color, light.color);
        glm_vec3_copy(position, light.position);
        light.position[3] = 1.0f;

        return light;
    }
};
