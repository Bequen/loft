//
// Created by martin on 7/21/24.
//

#ifndef LOFT_FRAMEGRAPH_H
#define LOFT_FRAMEGRAPH_H

#include <string>

namespace lft::fg {

class FrameGraph {
public:
    void record(int frequency);

    void run(const std::string &render_graph);
};

}
#endif //LOFT_FRAMEGRAPH_H
