// MapBlock deserialization: the map database / on-disk block format (version byte, then
// zstd/zlib compressed node data, metadata, static objects, node timers) and, with the
// "network" selector, the block data a server sends to clients.
#include "harness_common.h"
#include "exceptions.h"
#include "mapblock.h"
#include "serialization.h"

#include <sstream>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	harness_gamedef();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 2)
		return -1;
	// The first byte selects the on-disk vs. network entry point (a harness parameter);
	// the second is the serialization version byte every stored block starts with.
	const bool disk = data[0] & 1;
	const u8 version = data[1];
	if (!ser_ver_supported_read(version))
		return -1;

	std::istringstream is(std::string(reinterpret_cast<const char *>(data) + 2, size - 2),
			std::ios::binary);
	MapBlock block(v3s16(0, 0, 0), harness_gamedef());
	try {
		block.deSerialize(is, version, disk);
	} catch (const SerializationError &) {
		return -1;
	}
	return 0;
}
