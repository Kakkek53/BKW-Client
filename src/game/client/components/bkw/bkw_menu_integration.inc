#ifndef GAME_CLIENT_COMPONENTS_BKW_BKW_MENU_INTEGRATION_INC
#define GAME_CLIENT_COMPONENTS_BKW_BKW_MENU_INTEGRATION_INC

#include <game/client/components/bkw/bkw_community_router.inc>
#include <game/client/components/bkw/bkw_settings_state.inc>

namespace BkwMenuIntegration
{
inline const char *MenuName(bool TeeFusion)
{
	return TeeFusion ? "TF Menu" : "Голосование";
}

inline bool ShouldUseTfMenu(EBkwCommunity Community)
{
	return Community == EBkwCommunity::TEE_FUSION;
}

inline bool ShouldUseDdnetMenu(EBkwCommunity Community)
{
	return Community == EBkwCommunity::DDNET && BkwSettings::m_CustomDdnetVoteMenu;
}

inline bool IsPersonalizationPage(const char *pName)
{
	return pName && (str_comp(pName, "Personalization") == 0 || str_comp(pName, "Персонализация") == 0);
}

inline bool IsDriftCameraOption(const char *pName)
{
	return pName && (str_utf8_find_nocase(pName, "drift") || str_utf8_find_nocase(pName, "камера"));
}
}

#endif
