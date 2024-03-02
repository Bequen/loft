#include "io/file.hpp"

uint32_t *io::file::read_binary(const std::string& path, size_t *pOutSize) {
	FILE *f = fopen(path.c_str(), "rb");

	if(!f) {
		return nullptr;
	}

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	uint32_t *buf = new uint32_t[size + 1];
	fseek(f, 0, SEEK_SET);

	fread(buf, 4, size, f);
	fclose(f);

	buf[size] = '\0';

	if(pOutSize) *pOutSize = size;

	return buf;
}
