//
// Created by martin on 7/1/24.
//

#ifndef LOFT_SHADERBINARY_H
#define LOFT_SHADERBINARY_H

#include <utility>
#include <vector>
#include <cstdint>

struct ShaderBinary {
private:
    std::vector<uint32_t> m_data;

public:
    inline const std::vector<uint32_t>& data() const {
        return m_data;
    }

    inline const uint32_t code_size() const {
        // last integer is '\0' => should not be included in code size
        return m_data.size() - 1;
    }

    explicit ShaderBinary(std::vector<uint32_t> data) :
    m_data(std::move(data)) {

    }
};

#endif //LOFT_SHADERBINARY_H
