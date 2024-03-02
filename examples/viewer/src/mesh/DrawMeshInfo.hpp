//
// Created by martin on 10/24/23.
//

#ifndef LOFT_MESH_HPP
#define LOFT_MESH_HPP

typedef unsigned int uint32_t;

struct DrawMeshInfo {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    uint32_t vertexOffset;
    uint32_t firstInstance;

	DrawMeshInfo() :
	indexCount(0),
	instanceCount(0),
	firstIndex(0),
	vertexOffset(0),
	firstInstance(0) {
	
	}

	DrawMeshInfo(uint32_t indexCount,
				 uint32_t instanceCount,
				 uint32_t firstIndex,
				 uint32_t vertexOffset,
				 uint32_t firstInstance) :
	indexCount(indexCount), 
	instanceCount(instanceCount),
	firstIndex(firstIndex),
	vertexOffset(vertexOffset),
	firstInstance(firstInstance) {
		
	}
};

#endif //LOFT_MESH_HPP
