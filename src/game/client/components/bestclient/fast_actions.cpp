#include <base/color.h>
#include <base/math.h>
#include <base/system.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/gamecore.h>
#include <game/client/gameclient.h>

#include <algorithm>

// feature_pack_runtime.inc defines a namespace and therefore must be included at
// file scope. fast_actions_part1.inc and part2.inc intentionally split member
// functions across include boundaries, so inserting it between those files
// places the namespace inside CFastActions::BkwRenderCheckpoints().
#define HOOK_START_DISTANCE (CCharacterCore::PhysicalSize() * 1.5f)
#include <game/client/components/bkw/feature_pack_runtime.inc>
#undef HOOK_START_DISTANCE

#include <game/client/components/bestclient/fast_actions_part1.inc>
#include <game/client/components/bestclient/fast_actions_part2.inc>

// Hook the feature pack into the existing render point without duplicating the
// large split Fast Actions implementation. The original member call remains
// intact and this macro only exists while part3 is being included.
#define BkwRenderCheckpoints() \
	do \
	{ \
		if(g_Config.m_BkwOptimizer && g_Config.m_BkwOptimizerScope == 1 && g_Config.m_BkwOptimizerPreset == 2) \
			BkwFeaturePackRuntime::SaveAntiPingSettings(); \
		BkwFeaturePackRuntime::OnRender(GameClient(), Client(), Graphics(), Collision()); \
		this->BkwRenderCheckpoints(); \
	} while(0)
#include <game/client/components/bestclient/fast_actions_part3.inc>
#undef BkwRenderCheckpoints
