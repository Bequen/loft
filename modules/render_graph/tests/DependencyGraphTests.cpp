#include "AdjacencyMatrix.hpp"

#include <set>

void test_color_dependency() {
    AdjacencyMatrix adj({"task1", "task2", "final"});
    adj.set("task1", "task2");
    adj.set("task2", "final");

    auto deps = adj.get_dependencies("final");
    ASSERT(deps == std::vector<std::string>({"task2"}));

    deps = adj.get_dependencies("task2");
    ASSERT(deps == std::vector<std::string>({"task1"}));

    deps = adj.get_dependencies("task1");
    ASSERT(deps == std::vector<std::string>({}));
}

void test_color_dependency2() {
    AdjacencyMatrix adj({"task1", "task2", "final"});
    adj.set("task1", "final");
    adj.set("task2", "final");

    auto deps = adj.get_dependencies("final");
    ASSERT(std::set<std::string>(deps.begin(), deps.end()) == std::set<std::string>({"task1", "task2"}));

    deps = adj.get_dependencies("task2");
    ASSERT(deps == std::vector<std::string>({}));

    deps = adj.get_dependencies("task1");
    ASSERT(deps == std::vector<std::string>({}));
}

int main() {
    test_color_dependency();
    test_color_dependency2();

    return 0;
}
