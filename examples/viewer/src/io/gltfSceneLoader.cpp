#include "gltfSceneLoader.hpp"

#include <stdio.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include <stdexcept>

inline void memcpy_stride(char *pSource, uint32_t sourceStride,
                          char *pTarget, uint32_t targetStride,
                          uint32_t singleSize, uint32_t count) {
	for(int i = 0; i < count; i++) {
		memcpy(pTarget + i * targetStride, pSource + i * sourceStride, singleSize);
	}
}

struct SceneEntityCounts {
	uint32_t numVertices;
	uint32_t numIndices;
	uint32_t numPrimitives;
};

bool count_elements(cgltf_data* pData, SceneEntityCounts* pOutCounts) {
	pOutCounts->numVertices = 0;
	pOutCounts->numIndices = 0;
	pOutCounts->numPrimitives = 0;

	for(int i = 0; i < pData->meshes_count; i++) {
		pOutCounts->numPrimitives += pData->meshes[i].primitives_count;
		for(int p = 0; p < pData->meshes[i].primitives_count; p++) {
			pOutCounts->numIndices += pData->meshes[i].primitives[p].indices->count;
			for(int a = 0; a < pData->meshes[i].primitives[p].attributes_count; a++) {
				auto attr = pData->meshes[i].primitives[p].attributes[a];
				
				pOutCounts->numVertices += attr.data->count;
			}
		}
	}

	return true;
}

bool load_indices(cgltf_primitive *pPrimitive, Index *pOut) {
	char *pData = (char*)pPrimitive->indices->buffer_view->buffer->data +
						 pPrimitive->indices->buffer_view->offset +
						 pPrimitive->indices->offset;

    uint32_t fileIndexSize = 0;
    switch(pPrimitive->indices->component_type) {
        case cgltf_component_type_r_16u:
            fileIndexSize = 2;
            break;
        case cgltf_component_type_r_32u:
            fileIndexSize = 4;
            break;
        default:
            fileIndexSize = 4;
    }

    for(uint32_t i = 0; i < pPrimitive->indices->count; i++) {
        Index index = 0;
        memcpy(&index, pData + i * pPrimitive->indices->stride, fileIndexSize);
        pOut[i] = index;
    }

	/* memcpy_stride(pData, pPrimitive->indices->stride,
                  (char*)pOut, sizeof(Index),
                  sizeof(Index), pPrimitive->indices->count); */
	return true;
}

bool load_primitive(cgltf_primitive *pPrimitive, Vertex *pOut) {
	
	for(uint32_t a = 0; a < pPrimitive->attributes_count; a++) {
		auto attr = pPrimitive->attributes[a];

		char *pData = (char*)((char*)attr.data->buffer_view->buffer->data +
								attr.data->buffer_view->offset +
								attr.data->offset);

		uint32_t singleSize = 0;
		uint32_t offset = 0;

		switch(attr.type) {
			case cgltf_attribute_type_position:
				singleSize = sizeof(float) * 3;
				offset = offsetof(Vertex, pos);
				break;
			case cgltf_attribute_type_normal:
				singleSize = sizeof(float) * 3;
				offset = offsetof(Vertex, norm);
				break;
			case cgltf_attribute_type_texcoord:
				singleSize = sizeof(float) * 2;
				offset = offsetof(Vertex, uv);
				break;
            case cgltf_attribute_type_tangent:
                singleSize = sizeof(float) * 3;
                offset = offsetof(Vertex, tangent);
			default:
				break;
		}

		memcpy_stride(pData, attr.data->stride,
                      (char*)pOut + offset, sizeof(Vertex),
                      singleSize, attr.data->count);


	}

	return true;
}

const bool load_mesh_data(cgltf_data *pData, SceneData *pOutData) {
    uint32_t primitiveIdx = 0;
    for(uint32_t m = 0; m < pData->meshes_count; m++) {
        Mesh mesh = Mesh {
            .primitiveIdx = primitiveIdx,
            .numPrimitives = (uint32_t)pData->meshes[m].primitives_count
        };

        pOutData->set_mesh(m, mesh);

		for(uint32_t p = 0; p < pData->meshes[m].primitives_count; p++) {
			cgltf_primitive *pPrimitive = &pData->meshes[m].primitives[p];

            auto indices = pOutData->suballoc_indices(pPrimitive->indices->count);
			load_indices(pPrimitive, indices);

            auto vertices = pOutData->suballoc_vertices(pPrimitive->attributes[0].data->count);
			load_primitive(pPrimitive, vertices);

            Primitive primitive = {
                    .offset = (uint32_t)(pOutData->num_indices() - pPrimitive->indices->count),
                    .count = (uint32_t)pPrimitive->indices->count,
                    .baseVertex = (uint32_t)(pOutData->num_vertices() - pPrimitive->attributes[0].data->count),
                    .objectIdx = (uint32_t)cgltf_material_index(pData, pPrimitive->material),
            };
            pOutData->set_primitive(primitiveIdx++, primitive);
		}
	}

	return true;
}

bool load_pbr_metallic_roughness(cgltf_data* pData, cgltf_pbr_metallic_roughness *pInfo, MaterialData *pOut) {
    if(pInfo->base_color_texture.texture != nullptr) {
        pOut->set_color_texture(cgltf_texture_index(pData, pInfo->base_color_texture.texture));

    }

    if(pInfo->metallic_roughness_texture.texture != nullptr) {
        pOut->set_metallic_roughness_texture(cgltf_texture_index(pData, pInfo->metallic_roughness_texture.texture));
    }

    return true;
}

bool load_textures(cgltf_data *pData, SceneData *pOutData) {
    pOutData->resize_textures(pData->textures_count);

    for(uint32_t ti = 0; ti < pData->textures_count; ti++) {
        std::string path = pOutData->get_absolute_path_of(pData->textures[ti].image->uri);
        TextureData textureData = TextureData("test", path);
        pOutData->set_texture(ti, textureData);
    }

    return true;
}

bool load_materials(cgltf_data *pData, SceneData *pOutData) {
    for(uint32_t m = 0; m < pData->materials_count; m++) {
        cgltf_material *material = &pData->materials[m];
        auto materialData = MaterialData(new float[]{0.0f, 0.0f, 0.0f, 0.0f});

        if(material->has_pbr_metallic_roughness) {
            load_pbr_metallic_roughness(pData, &material->pbr_metallic_roughness, &materialData);
        }

        if(material->normal_texture.texture != nullptr) {
            materialData.set_normal_texture(cgltf_texture_index(pData, material->normal_texture.texture));
        }

        if (material->alpha_mode == cgltf_alpha_mode_mask ||
            material->alpha_mode == cgltf_alpha_mode_blend) {
            materialData.set_alpha_blend(true);
        }

        pOutData->set_material(cgltf_material_index(pData, material), materialData);
    }
	return true;
}

void load_nodes(cgltf_data *pData, SceneData *pOutData) {
    pOutData->resize_nodes(pData->nodes_count);
    for(uint32_t ni = 0; ni < pData->nodes_count; ni++) {
        const auto node = &pData->nodes[ni];
        SceneNode nodeData = SceneNode();

        nodeData.transformIdx = 0;
        if(node->mesh != nullptr) {
            nodeData.meshIdx = cgltf_mesh_index(pData, node->mesh);
        } else {
            nodeData.meshIdx = -1;
        }

        pOutData->set_node(ni, nodeData);
    }
}

const SceneData GltfSceneLoader::from_file(std::string path) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result status = cgltf_parse_file(&options, path.c_str(), &data);

    if (status != cgltf_result_success) {
        cgltf_free(data);
        throw std::runtime_error("Failed to find file");
    }

	cgltf_load_buffers(&options, data, path.c_str());

	SceneEntityCounts counts = {};
	if(!count_elements(data, &counts)) {
        throw std::runtime_error("Failed to count elements");
    }

	SceneData scene(data->nodes_count + 1, data->materials_count,
					counts.numVertices, counts.numIndices,
					data->meshes_count, counts.numPrimitives);

    scene.set_path(path);

	if(!load_mesh_data(data, &scene)) {
	}

    if(!load_textures(data, &scene)) {
        throw std::runtime_error("Failed to load textures");
    }

	if(!load_materials(data, &scene)) {
	}

    load_nodes(data, &scene);

	return scene;
}
