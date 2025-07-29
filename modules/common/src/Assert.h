//
// Created by martin on 7/5/24.
//

#ifndef LOFT_ASSERT_H
#define LOFT_ASSERT_H

#include <stdexcept>
#include <format>
#include <source_location>

#define ASSERT(expr) if(!(expr)) { throw std::runtime_error(std::format("Assertion failed in {}: {}", std::source_location::current().function_name(), #expr)); }

#endif //LOFT_ASSERT_H
