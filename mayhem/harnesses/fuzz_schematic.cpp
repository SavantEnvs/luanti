// Schematic (.mts) file parser: signature, version, size, probability and node name tables,
// zlib-compressed node data.
#include "harness_common.h"
#include "exceptions.h"
#include "mapgen/mg_schematic.h"

#include <sstream>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	harness_gamedef();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	std::istringstream is(std::string(reinterpret_cast<const char *>(data), size),
			std::ios::binary);
	Schematic schem;
	try {
		if (!schem.deserializeFromMts(&is))
			return -1;
	} catch (const SerializationError &) {
		return -1;
	}
	return 0;
}
