#include <stdexcept>
#include "io/file.hpp"
#include "io/ShaderBinary.h"

ShaderBinary io::file::read_binary(const std::string& path) {
	FILE *f = fopen(path.c_str(), "rb");

	if(!f) {
		throw std::runtime_error("Failed to open file");
	}

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	std::vector<uint32_t> data(size + 1);
	fseek(f, 0, SEEK_SET);

	fread(data.data(), 4, size, f);
	fclose(f);

	data[size] = '\0';

	return ShaderBinary(data);
}
