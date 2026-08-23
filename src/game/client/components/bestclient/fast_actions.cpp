#include <game/collision.h>

#include <game/client/components/bestclient/fast_actions_part1.inc>

// DDNet keeps this value local to CPlayers::RenderHookCollLine. Reuse the same
// physics formula for the BKW hook-target indicator without introducing a
// second magic number.
#define HOOK_START_DISTANCE (CCharacterCore::PhysicalSize() * 1.5f)
#include <game/client/components/bkw/feature_pack_runtime.inc>
#undef HOOK_START_DISTANCE

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
