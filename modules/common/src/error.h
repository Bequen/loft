//
// Created by marti on 7/1/2024.
//

#ifndef LOFT_ERROR_H
#define LOFT_ERROR_H

#include <string>
#include <utility>
#include "Display.h"

namespace lft {
enum ErrorCode {

};

class Error : lft::Display {
private:
    ErrorCode m_code;
    std::string m_message;

public:
    Error(const ErrorCode code, std::string message) :
    m_code(code), m_message(std::move(message)) {

    }

    void display() const override {

    }
};
}

#endif //LOFT_ERROR_H
