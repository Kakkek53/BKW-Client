#include <game/client/components/bestclient/fast_actions_part1.inc>
#include <game/client/components/bkw/feature_pack_runtime.inc>
#include <game/client/components/bestclient/fast_actions_part2.inc>

// Hook the feature pack into the existing render point without duplicating the
// large split Fast Actions implementation. The original member call remains
// intact and this macro only exists while part3 is being included.
#define BkwRenderCheckpoints() \
	do \
	{ \
		BkwFeaturePackRuntime::OnRender(GameClient(), Client(), Graphics()); \
		this->BkwRenderCheckpoints(); \
	} while(0)
#include <game/client/components/bestclient/fast_actions_part3.inc>
#undef BkwRenderCheckpoints
