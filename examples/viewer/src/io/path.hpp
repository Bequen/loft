#pragma once

#include <string>

typedef unsigned int uint32_t;

namespace io::path {
void setup_exe_path(char *pArg);

std::string exec_dir();

std::string shader(const std::string& name);

std::string asset(const std::string& name);
}

