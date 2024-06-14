//
// Created by martin on 6/19/24.
//

#ifndef LOFT_SCENETREE_H
#define LOFT_SCENETREE_H

#include "mesh/data/SceneData.hpp"
#include <memory>

class SceneTree {
private:
    struct SceneTreeNode {
        SceneNode node;
        bool isValid;
        int32_t childrenBegin;
        int32_t parent;
    };

    std::vector<SceneTreeNode> m_nodes;

public:
    inline const uint32_t num_nodes() const {
        return m_nodes.size();
    }

    SceneTree(uint32_t reserve) :
    m_nodes(reserve) {

    }
};


#endif //LOFT_SCENETREE_H
