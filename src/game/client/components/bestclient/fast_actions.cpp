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

// Runtime namespaces must stay at file scope. fast_actions_part1.inc and
// part2.inc intentionally split member functions across include boundaries.
#define HOOK_START_DISTANCE (CCharacterCore::PhysicalSize() * 1.5f)
#include <game/client/components/bkw/feature_pack_runtime.inc>
#undef HOOK_START_DISTANCE
#include <game/client/components/bkw/local_duel.inc>
#include <game/client/components/bkw/ego_vote_overlay.inc>

#include <game/client/components/bestclient/fast_actions_part1.inc>
#include <game/client/components/bestclient/fast_actions_part2.inc>

// Hook the BKW runtime into the existing render point without duplicating the
// large split Fast Actions implementation. Local Duel is independent from the
// TeeFusion duel parser and works from client-side player freeze state. EGO's
// vote overlay is activated only for server names containing both "EGO |" and
// "[eternal-gores.com]" and therefore does not touch the TeeFusion/DDNet
// community detection path.
#define BkwRenderCheckpoints() \
	do \
	{ \
		if(g_Config.m_BkwOptimizer && g_Config.m_BkwOptimizerScope == 1 && g_Config.m_BkwOptimizerPreset == 2) \
			BkwFeaturePackRuntime::SaveAntiPingSettings(); \
		BkwFeaturePackRuntime::OnRender(GameClient(), Client(), Graphics(), Collision()); \
		BkwLocalDuel::Update(GameClient(), Client(), Graphics()); \
		if(BkwEgoVote::UpdateAndRender(Ui(), GameClient(), Client(), Input(), Http())) \
			return; \
		this->BkwRenderCheckpoints(); \
	} while(0)
#include <game/client/components/bestclient/fast_actions_part3.inc>
#undef BkwRenderCheckpoints