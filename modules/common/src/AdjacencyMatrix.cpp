#include <iostream>

#include "AdjacencyMatrix.hpp"

AdjacencyMatrixNodeHandle& AdjacencyMatrixNodeHandle::add_dependency(uint32_t on) {
    m_graph->set(m_idx, on);
    return *this;
}

std::vector<uint32_t> AdjacencyMatrixNodeHandle::dependencies() {
    return m_graph->get_dependencies(m_idx);
}

bool AdjacencyMatrixNodeHandle::depends_on(uint32_t what) {
    return m_graph->get(what, m_idx);
}

void AdjacencyMatrix::find_dft(uint32_t node, uint32_t target, uint32_t maxDepth) {
    if(maxDepth == 0) {
        throw std::runtime_error("Contains loop");
    }

    for(uint32_t x = 0; x < m_matrix.size(); x++) {
        if(get(x, node)) {
            unset(x, target);
            find_dft(x, target, maxDepth - 1);
        }
    }
}

void AdjacencyMatrix::transitive_reduction() {
    for(uint32_t x = 0; x < m_matrix.size(); x++) {
        for(uint32_t y = 0; y < m_matrix.size(); y++) {
            if(get(y, x)) {
                find_dft(y, x, m_matrix.size());
            }
        }
    }
}

void AdjacencyMatrix::print() {
    for(uint32_t x = 0; x < m_matrix.size(); x++) {
        for(uint32_t y = 0; y < m_matrix.size(); y++) {
            std::cout << get(x, y) << " ";
        }
        std::cout << std::endl;
    }
}
