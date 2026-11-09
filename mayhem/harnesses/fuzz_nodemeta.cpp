// NodeMetadataList and Inventory/ItemStack deserialization (node metadata blobs in map
// blocks, player inventories, detached inventories).
#include "harness_common.h"
#include "exceptions.h"
#include "inventory.h"
#include "nodemetadata.h"

#include <sstream>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	harness_gamedef();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 1)
		return -1;
	const unsigned which = data[0] % 4;
	std::istringstream is(std::string(reinterpret_cast<const char *>(data) + 1, size - 1),
			std::ios::binary);
	const IItemDefManager *idef = harness_gamedef()->getItemDefManager();
	try {
		switch (which) {
		case 0: {
			NodeMetadataList list;
			list.deSerialize(is, idef, false);
			break;
		}
		case 1: {
			NodeMetadataList list;
			list.deSerialize(is, idef, true);
			break;
		}
		case 2: {
			Inventory inv(idef);
			inv.deSerialize(is);
			break;
		}
		default: {
			ItemStack stack;
			stack.deSerialize(is, idef);
			break;
		}
		}
	} catch (const SerializationError &) {
		return -1;
	}
	return 0;
}
