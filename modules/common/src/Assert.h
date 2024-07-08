//
// Created by martin on 7/5/24.
//

#ifndef LOFT_ASSERT_H
#define LOFT_ASSERT_H

#define ASSERT(expr) if(!(expr)) { throw std::runtime_error("Assertion failed: " #expr); }

#endif //LOFT_ASSERT_H
