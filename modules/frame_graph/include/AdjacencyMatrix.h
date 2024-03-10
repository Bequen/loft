#pragma once

#include <cstdint>

#include <vector>

struct AdjacencyMatrix {
private:
    std::vector<std::vector<bool>> m_matrix;

    void find_dft(uint32_t node, uint32_t target);

public:
    explicit AdjacencyMatrix(uint32_t width);

    [[nodiscard]] constexpr bool get(uint32_t from, uint32_t to) const {
        return m_matrix[from][to];
    }

    inline AdjacencyMatrix& set(uint32_t from, uint32_t to) {
        m_matrix[from][to] = true;
        return *this;
    }

    inline AdjacencyMatrix& unset(uint32_t from, uint32_t to) {
        m_matrix[from][to] = false;
        return *this;
    }

    [[nodiscard]] constexpr uint32_t num_dependencies(uint32_t to) const {
        uint32_t numDependencies = 0;
        for(uint32_t x = 0; x < m_matrix.size(); x++) {
            if(get(x, to)) {
                numDependencies++;
            }
        }

        return numDependencies;
    }

    void transitive_reduction();

    void print();
};
