// Shared one-time setup for the Luanti fuzz harnesses (server build, no client/irrlicht).
#pragma once

#include "defaultsettings.h"
#include "dummygamedef.h"
#include "settings.h"

#include <memory>

// Process-wide, read-only after setup: the default settings layer and a game definition
// holding a few nodes (so block/schematic name<->id mapping has something to resolve against).
inline DummyGameDef *harness_gamedef()
{
	static DummyGameDef *gamedef = [] {
		set_default_settings();
		Settings::createLayer(SL_GLOBAL);

		auto *gd = new DummyGameDef();
		static const char *const names[] = {
			"default:stone", "default:dirt", "default:water_source", "default:torch"};
		for (const char *name : names) {
			ItemDefinition idef;
			idef.type = ITEM_NODE;
			idef.name = name;
			gd->getWritableItemDefManager()->registerItem(idef);
			ContentFeatures f;
			f.name = name;
			gd->getWritableNodeDefManager()->set(f.name, std::move(f));
		}
		return gd;
	}();
	return gamedef;
}
