// Settings (minetest.conf / world.mt / map_meta.txt) text parser, including multiline
// values and groups, plus reading the parsed values back through the typed getters.
#include "harness_common.h"
#include "settings.h"

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
	Settings settings;
	if (!settings.parseConfigLines(is))
		return -1;
	for (const std::string &name : settings.getNames()) {
		std::string value;
		float num;
		s32 integer;
		u64 big;
		v2f vec2;
		std::optional<v3f> vec;
		Settings *group = nullptr;
		settings.getNoEx(name, value);
		settings.getS32NoEx(name, integer);
		settings.getU64NoEx(name, big);
		settings.getV2FNoEx(name, vec2);
		settings.getFloatNoEx(name, num);
		settings.getV3FNoEx(name, vec);
		settings.getGroupNoEx(name, group);
	}
	return 0;
}
