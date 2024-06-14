#include <cstdint>
#include <string>
#include "ShaderBinary.h"

namespace io::file {
	/**
	 * Reads file on path in as binary.
	 * 
	 * @param path Path of the file
	 * @param pOutSize Pointer to the size_t variable to which the number of 
	 * bytes of the file will be set.
	 */
	ShaderBinary read_binary(const std::string& path);
}
