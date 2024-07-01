#pragma once

#include <string>
#include <utility>
#include <vector>
#include <cstdint>
#include <optional>
#include <cstring>
#include "props.hpp"

#include "MaterialData.h"
#include "TextureData.h"
#include "../Vertex.hpp"
#include "mesh/Transform.hpp"

struct Mesh {
	unsigned int primitiveIdx;
	unsigned int numPrimitives;
};

struct Primitive {
    unsigned int objectIdx;
	unsigned int offset;
	unsigned int count;
	unsigned int baseVertex;
};

struct SceneNode {
    std::string name;
	uint32_t transformIdx;
    int32_t meshIdx;

    SceneNode() {

    }

    SceneNode(std::string name, uint32_t transformIdx, int32_t meshIdx) {

    }
};

class SceneData {
	std::string m_dir;
	std::string m_filename;

	std::vector<MaterialData> m_materials;
    std::vector<TextureData> m_textures;

	std::vector<Vertex> m_vertices;
	size_t m_numVertices;

	std::vector<Index> m_indices;
	size_t m_numIndices;

	std::vector<SceneNode> m_nodes;
	std::vector<Mesh> m_meshes;
    std::vector<Primitive> m_primitives;

    std::vector<Transform> m_transforms;
	uint32_t m_numMeshes;

public:
	REF(m_vertices, vertices);
	GET(m_numVertices, num_vertices);

	REF(m_indices, indices);
	GET(m_numIndices, num_indices);

	GET(m_nodes, nodes);
	GET(m_nodes.size(), num_nodes);

	GET(m_meshes, meshes);

    REF(m_materials, materials);
    GET(m_materials.size(), num_materials);

    REF(m_textures, textures);
    GET(m_textures.size(), num_textures);

    REF(m_primitives, primitives);
    REF(m_transforms, transforms);

	SceneData(uint32_t numNodes, uint32_t numMaterials, uint32_t numVertices,
			  uint32_t numIndices, uint32_t numMeshes, uint32_t numPrimitives);

    inline uint32_t push_transform(Transform transform) {
        m_transforms.push_back(transform);
        return m_transforms.size() - 1;
    }

	inline Vertex* suballoc_vertices(size_t numVertices) {
		auto result = m_vertices.data() + m_numVertices;
		m_numVertices += numVertices;
		return result;
	}

	inline Index* suballoc_indices(size_t numIndices) {
		auto result = m_indices.data() + m_numIndices;
		m_numIndices += numIndices;
		return result;
	}

	inline SceneData& set_path(std::string path) {
		unsigned split = path.find_last_of("/\\");
		this->m_dir = path.substr(0, split + 1);
		this->m_filename = path.substr(split);
		return *this;
	}

    inline SceneData& set_material(uint32_t idx, MaterialData materialData) {
        m_materials[idx] = std::move(materialData);
        return *this;
    }

    /**
     * When loading up textures, first the loader must call this.
     * Resize to appropriate size and then set individual indexes.
     * It is because we use indexing to retrieve the texture, so it makes no sense to suddenly push it
     * one by one
     * @param size Number of textures to be holded by scene data
     * @return returns reference to current SceneData for chaining
     */
    SceneData& resize_textures(size_t size) {
        m_textures.resize(size);
        return *this;
    }

    SceneData& set_texture(uint32_t idx, TextureData texture) {
        m_textures[idx] = texture;
        return *this;
    }


    SceneData& resize_nodes(size_t size) {
        m_nodes.resize(size);
        return *this;
    }

    SceneData& set_node(uint32_t idx, SceneNode node) {
        m_nodes[idx] = node;
        return *this;
    }


    SceneData& resize_primitives(size_t size) {
        m_primitives.resize(size);
        return *this;
    }

    SceneData& set_primitive(uint32_t idx, Primitive& primitive) {
        m_primitives[idx] = primitive;
        return *this;
    }

    SceneData& set_mesh(uint32_t idx, Mesh& mesh) {
        m_meshes[idx] = mesh;
        return *this;
    }


    std::string get_absolute_path_of(std::string relativePath) {
        return m_dir + relativePath;
    }

    void remap_materials(std::vector<uint32_t> materialIds) {

    }
};
