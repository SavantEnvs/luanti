// Network-protocol definition deserializers: the structs a server sends to a client when
// it joins or spawns objects/particles (ObjectProperties, ItemDefinition, ContentFeatures,
// the whole node definition table, particles, tool capabilities, tile animations).
#include "harness_common.h"
#include "exceptions.h"
#include "itemdef.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
#include "object_properties.h"
#include "particles.h"
#include "tileanimation.h"
#include "tool.h"

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
	// Selector + negotiated protocol version are harness parameters, not format bytes.
	const unsigned which = (data[0] & 0x0f) % 9;
	const u16 proto = SERVER_PROTOCOL_VERSION_MIN +
			data[1] % (LATEST_PROTOCOL_VERSION - SERVER_PROTOCOL_VERSION_MIN + 1);
	std::istringstream is(std::string(reinterpret_cast<const char *>(data) + 2, size - 2),
			std::ios::binary);
	try {
		switch (which) {
		case 0: {
			ObjectProperties prop;
			prop.deSerialize(is);
			break;
		}
		case 1: {
			ItemDefinition def;
			def.deSerialize(is, proto);
			break;
		}
		case 2: {
			ContentFeatures f;
			f.deSerialize(is, proto);
			break;
		}
		case 3: {
			std::unique_ptr<NodeDefManager> mgr(createNodeDefManager());
			mgr->deSerialize(is, proto);
			break;
		}
		case 4: {
			ParticleParameters p;
			p.deSerialize(is, proto);
			break;
		}
		case 5: {
			ServerParticleTexture tex;
			tex.deSerialize(is, proto, data[0] & 0x10, data[0] & 0x20);
			break;
		}
		case 6: {
			ToolCapabilities caps;
			caps.deSerialize(is);
			break;
		}
		case 7: {
			TileAnimationParams anim;
			anim.deSerialize(is, proto);
			break;
		}
		default: {
			std::unique_ptr<IWritableItemDefManager> mgr(createItemDefManager());
			mgr->deSerialize(is, proto);
			break;
		}
		}
	} catch (const SerializationError &) {
		return -1;
	}
	return 0;
}
