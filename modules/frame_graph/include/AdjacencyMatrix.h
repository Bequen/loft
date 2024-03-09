#include <cstdint>

#include <vector>

struct AdjacencyMatrix {
private:
    std::vector<std::vector<bool>> m_matrix;

    void find_dft(uint32_t node, uint32_t target);

public:
    AdjacencyMatrix(uint32_t width);

    bool get(uint32_t from, uint32_t to);

    AdjacencyMatrix& set(uint32_t from, uint32_t to);

    AdjacencyMatrix& unset(uint32_t from, uint32_t to);

    void transitive_reduction();

    void print();
};
