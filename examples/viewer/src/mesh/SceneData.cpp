#include "SceneData.hpp"

SceneData::SceneData(uint32_t numNodes, uint32_t numMaterials, uint32_t numVertices,
					 uint32_t numIndices, uint32_t numMeshes, uint32_t numPrimitives) {
	this->m_materials.resize(numMaterials);
	this->m_vertices.resize(numVertices);
	this->m_indices.resize(numIndices);
	this->m_meshes.resize(numMeshes);
	this->m_primitives.resize(numPrimitives);
	this->m_nodes.resize(numNodes);

	m_numVertices = 0;
	m_numIndices = 0;
	m_numMeshes = 0;
}
