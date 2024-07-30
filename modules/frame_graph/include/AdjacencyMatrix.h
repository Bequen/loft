#pragma once

#include <cstdint>

#include <vector>
#include "modules/common/src/Assert.h"

/**
 * Represents a graph as an adjacency matrix.
 */
struct AdjacencyMatrix {
private:
    std::vector<std::vector<bool>> m_matrix;

    void find_dft(uint32_t node, uint32_t target, uint32_t maxDepth);

public:
    explicit AdjacencyMatrix(uint32_t width);

    [[nodiscard]] bool get(uint32_t from, uint32_t to) const {
        ASSERT(from < m_matrix.size() && to < m_matrix.size());

        return m_matrix[from][to];
    }

    inline AdjacencyMatrix& set(uint32_t from, uint32_t to) {
        ASSERT(from < m_matrix.size() && to < m_matrix.size());

        m_matrix[from][to] = true;
        return *this;
    }

    inline AdjacencyMatrix& unset(uint32_t from, uint32_t to) {
        ASSERT(from < m_matrix.size() && to < m_matrix.size());

        m_matrix[from][to] = false;
        return *this;
    }

    /**
     * Counts and returns number of dependencies of item at 'to' index
     * @param to Index of the dependant
     * @return number representing count of dependencies
     */
    [[nodiscard]] uint32_t num_dependencies(uint32_t to) const {
        ASSERT(to < m_matrix.size());

        uint32_t numDependencies = 0;
        for(uint32_t x = 0; x < m_matrix.size(); x++) {
            if(get(x, to)) {
                numDependencies++;
            }
        }

        return numDependencies;
    }

    /**
     * Gets all the dependencies of 'to' item as vector
     * @param to Index of the dependant
     * @return vector of indices of dependencies
     */
    [[nodiscard]] std::vector<uint32_t> get_dependencies(uint32_t to) const {
        ASSERT(to < m_matrix.size());

        uint32_t numDependencies = num_dependencies(to);
        std::vector<uint32_t> dependencies(numDependencies);
        uint32_t i = 0;

        for(uint32_t x = 0; x < m_matrix.size() && i < numDependencies; x++) {
            if(get(x, to)) {
                dependencies[i++] = x;
            }
        }

        return dependencies;
    }

    /**
     * Does a transitive reduction on the matrix.
     */
    void transitive_reduction();

    void print();
};
