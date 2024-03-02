#include <cstdint>
#include <string>

namespace io::file {
	/**
	 * Reads file on path in as binary.
	 * 
	 * @param path Path of the file
	 * @param pOutSize Pointer to the size_t variable to which the number of 
	 * bytes of the file will be set.
	 */
	uint32_t *read_binary(const std::string& path, size_t *pOutSize);
}
