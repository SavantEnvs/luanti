// Compressed-data decoders used for map blocks, network packets and mod data: zlib, zstd
// and the versioned `decompress()` dispatcher, plus the string/JSON helpers built on them.
#include "harness_common.h"
#include "exceptions.h"
#include "serialization.h"
#include "util/serialize.h"

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
	const unsigned which = data[0] % 6;
	const u8 version = data[1];
	std::istringstream is(std::string(reinterpret_cast<const char *>(data) + 2, size - 2),
			std::ios::binary);
	std::ostringstream os(std::ios::binary);
	try {
		switch (which) {
		case 0:
			decompressZlib(is, os, 1 << 24, false);
			break;
		case 1:
			decompressZlib(is, os, 1 << 24, true);
			break;
		case 2:
			decompressZstd(is, os);
			break;
		case 3:
			if (!ser_ver_supported_read(version))
				return -1;
			decompress(is, os, version);
			break;
		case 4:
			deSerializeString32(is);
			deSerializeString16(is);
			deSerializeJsonStringIfNeeded(is);
			break;
		default:
			deSerializeJsonString(is);
			deSerializeString16(is);
			break;
		}
	} catch (const SerializationError &) {
		return -1;
	}
	return 0;
}
