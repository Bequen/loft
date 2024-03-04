#pragma once

#include <vector>
#include <cstdint>

template<typename FromT, typename IntoT>
inline std::vector<IntoT> map(const std::vector<FromT>& original, std::unary_function<FromT, IntoT> func) {
    std::vector<IntoT> result(original.size());
    for(uint32_t i = 0; i < original; i++) {
        result[i] = func(original[i]);
    }
    return result;
}
