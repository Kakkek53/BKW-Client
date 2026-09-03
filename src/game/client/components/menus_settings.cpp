#include <base/fs.h>
#include <base/math.h>
#include <base/mem.h>
#include <base/str.h>
#include <base/system.h>

#include <engine/shared/datafile.h>
#include <engine/storage.h>
#include <engine/updater.h>
#include <engine/shared/bkw_version.h>

#include <game/client/components/bkw/cloud_account.h>
#include <game/client/components/bkw/ddstats_hours.h>
#include <game/client/components/media_decoder.h>
#include <game/gamecore.h>
#include <game/localization.h>
#include <game/mapitems.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iterator>
#include <vector>

namespace
{
void BkwVideoMapStrToInts(int *pInts, size_t NumInts, const char *pStr)
{
	if(pInts == nullptr || NumInts == 0)
		return;

	// DDNet map names stored in int arrays reserve the final byte. Keep using
	// the engine's canonical StrToInts packing, but truncate long generated
	// names first so a layer name can never trigger StrToInts's debug assert.
	char aSafeName[128];
	const size_t Capacity = std::min(sizeof(aSafeName), NumInts * sizeof(int));
	str_copy(aSafeName, pStr != nullptr ? pStr : "", Capacity);
	StrToInts(pInts, NumInts, aSafeName);
}
}

#define StrToInts BkwVideoMapStrToInts
#include "bkw/video_map_converter.inc"
#undef StrToInts

// Keep the Local Duel namespace at file scope. The BKW settings files below are
// included inside BkwMenuProxy, so including it there would create a different
// nested namespace than the runtime uses.
#include "bkw/local_duel.inc"

#include "menus_settings_bkw_part1.inc"
#include "menus_settings_bkw_part2.inc"
