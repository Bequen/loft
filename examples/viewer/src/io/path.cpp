#include "path.hpp"

#include <iostream>
#if __linux__
#include <linux/limits.h>
#include <unistd.h>
#include <err.h>
#elif _WIN32
#include <windows.h>
#else
#error "Unsupported platform"
#endif
#include <string.h>

static char *pExePath;

#ifdef __linux__
const char DIRECTORY_SEPARATOR = '/';
#elif _WIN32
const char DIRECTORY_SEPARATOR = '\\';
#else
#error "Unsupported platform"
#endif

void io::path::setup_exe_path(char *pArg) {
	char *pBuffer = nullptr;
#ifdef __linux__
	pBuffer = malloc(PATH_MAX);
	size_t bytes = readlink("/proc/self/exe", pBuffer, PATH_MAX);
    pBuffer[bytes] = '\0';
#elif _WIN32
	pBuffer = (char*)malloc(_MAX_PATH);
    int bytes = GetModuleFileName(NULL, pBuffer, _MAX_PATH);
    if(bytes >= 0)
        pBuffer[bytes] = '\0';
#else
#error "Unsupported platform"
#endif
	unsigned int lastSlash = 0;
	for(size_t i = bytes; i > 0; i--) {
		if(pBuffer[i] == DIRECTORY_SEPARATOR) {
			lastSlash = i + 1;
			break;
		}
	}

	pBuffer[lastSlash] = '\0';
    std::cout << pBuffer << std::endl;

	pExePath = pBuffer;
}

std::string io::path::exec_dir() {
	return pExePath;
}

std::string io::path::shader(const std::string& name) {
	auto result = exec_dir() + std::string("shaders") + DIRECTORY_SEPARATOR + std::string(name);
    return result;
}
std::string io::path::asset(const std::string& name) {
    return exec_dir() + std::string("assets") + DIRECTORY_SEPARATOR + std::string(name);
}
