#include <base/color.h>
#include <base/math.h>
#include <base/system.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/gamecore.h>
#include <game/client/gameclient.h>

#include <algorithm>
#include <cmath>

// Runtime namespaces must stay at file scope. fast_actions_part1.inc and
// part2.inc intentionally split member functions across include boundaries.
#define HOOK_START_DISTANCE (CCharacterCore::PhysicalSize() * 1.5f)
#include <game/client/components/bkw/feature_pack_runtime.inc>
#undef HOOK_START_DISTANCE
#include <game/client/components/bkw/tipo_cheat_local_visual.inc>
#include <game/client/components/bkw/local_duel.inc>
#include <game/client/components/bkw/minimap.inc>
#include <game/client/components/bkw/minimap_view_fix.inc>
#include <game/client/components/bkw/minimap_real_small.inc>
#include <game/client/components/bkw/ego_vote_overlay.inc>
#include <game/client/components/bkw/ego_vote_real_format_fix.inc>
#include <game/client/components/bkw/vote_menu_debug.inc>

#include <game/client/components/bestclient/fast_actions_part1.inc>
#include <game/client/components/bestclient/fast_actions_part2.inc>

// Hook the BKW runtime into the existing render point without duplicating the
// large split Fast Actions implementation. EGO does not replace the vote menu:
// it only registers an optional right-side "Подробнее" action for map rows in
// the standard CListBox and uses a normal UI popup for the API statistics.
// The live EGO parser understands framed rows like:
// "│ • MapName | ★★☆ | S | 144 ⚐ | 11:49 ◷".
// Test build: F5 dumps the server's raw vote-option list to the local console.
#define BkwRenderCheckpoints() \
	do \
	{ \
		/* Legacy TipoCheat used real aim/emote input. Keep it disabled while the old runtime runs, */ \
		/* then render the replacement locally so absolutely nothing from TipoCheat reaches the server. */ \
		const int BkwTipoCheatEnabled = g_Config.m_BkwTipoCheat; \
		g_Config.m_BkwTipoCheat = 0; \
		BkwFeaturePackRuntime::OnRender(GameClient(), Client(), Graphics(), Collision()); \
		g_Config.m_BkwTipoCheat = BkwTipoCheatEnabled; \
		BkwTipoCheatLocal::Render(GameClient(), Client(), Graphics()); \
		BkwLocalDuel::Update(GameClient(), Client(), Graphics()); \
		BkwMiniMapFix::Update(GameClient(), Client(), Graphics(), Input()); \
		BkwMiniMapRealSmall::Render(GameClient(), Client(), Graphics()); \
		BkwVoteMenuDebug::OnRender(GameClient(), Client(), Input()); \
		BkwEgoVote::UpdateStandardVoteIntegration(Ui(), GameClient(), Client(), Http()); \
		BkwEgoVoteRealFormat::OverrideStandardVoteAction(Ui(), GameClient(), Client(), Http()); \
		this->BkwRenderCheckpoints(); \
	} while(0)
#include <game/client/components/bestclient/fast_actions_part3.inc>
#undef BkwRenderCheckpoints
