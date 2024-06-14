#ifndef LOFT_TRANSFORM_HPP
#define LOFT_TRANSFORM_HPP

#include <cglm/mat4.h>
#include <cglm/quat.h>
#include <cglm/affine.h>

struct Transform {
    mat4 model;

    static Transform identity() {
        Transform transform = {};
        glm_mat4_identity(transform.model);
        return transform;
    }

public:
    void transform(mat4 transformation) {
        glm_mat4_mul(model, transformation, model);
    }

    void translate(float translation[3]) {
        glm_translate(model, translation);
    }

    void rotate(float rotation[4]) {
        mat4 m;
        // glm_quat_rotate(model, rotation, m);
        // glm_mat4_copy(m, model);
    }

    void scale(float scale[3]) {
        glm_scale(model, scale);
    }
};

#endif //LOFT_TRANSFORM_HPP
