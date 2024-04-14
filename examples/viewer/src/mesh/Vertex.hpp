#pragma once

#include "shaders/VertexBinding.h"
#include "shaders/VertexAttribute.h"
#include <vector>

typedef unsigned int Index;

struct VertexInput {

};

struct Vertex : VertexInput {
	float pos[3];
	float norm[3];
	float uv[2];

    float tangent[4];

    static inline std::vector<VertexBinding> bindings() {
        return {
                {
                        .binding = 0,
                        .stride = sizeof(Vertex),
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
        };
    }

    static inline std::vector<VertexAttribute> attributes() {
        return {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) },
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm) },
            { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) },
            { 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent) }
        };
    }
};
