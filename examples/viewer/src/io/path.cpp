#include "path.hpp"
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

void io::path::setup_exe_path(char *pArg) {
	char *pBuffer = (char*)malloc(PATH_MAX);
#ifdef __linux__
	size_t bytes = readlink("/proc/self/exe", pBuffer, PATH_MAX);
    pBuffer[bytes] = '\0';
#elif _WIN32
    int bytes = GetModuleFileName(NULL, pBuffer, PATH_MAX);
    if(bytes >= 0)
        pBuffer[bytes] = '\0';
#else
#error "Unsupported platform"
#endif
	unsigned int lastSlash = 0;
	for(size_t i = bytes; i > 0; i--) {
		if(pBuffer[i] == '/') {
			lastSlash = i + 1;
			break;
		}
	}

	pBuffer[lastSlash] = '\0';

	pExePath = pBuffer;
}

std::string io::path::exec_dir() {
	return pExePath;
}

std::string io::path::shader(const std::string& name) {
	return exec_dir().append("shaders/").append(name);
}
std::string io::path::asset(const std::string& name) {
    return exec_dir().append("assets/").append(name);
}
