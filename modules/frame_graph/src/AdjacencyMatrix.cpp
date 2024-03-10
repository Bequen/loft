#include <iostream>
#include "AdjacencyMatrix.h"

AdjacencyMatrix::AdjacencyMatrix(uint32_t width) :
m_matrix(width, std::vector<bool>(width)) {

}

void AdjacencyMatrix::find_dft(uint32_t node, uint32_t target) {
    for(uint32_t x = 0; x < m_matrix.size(); x++) {
        if(get(x, node)) {
            unset(x, target);
            find_dft(x, target);
        }
    }
}

void AdjacencyMatrix::transitive_reduction() {
    for(uint32_t x = 0; x < m_matrix.size(); x++) {
        for(uint32_t y = 0; y < m_matrix.size(); y++) {
            if(get(y, x)) {
                find_dft(y, x);
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