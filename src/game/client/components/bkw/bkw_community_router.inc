#ifndef GAME_CLIENT_COMPONENTS_BKW_BKW_COMMUNITY_ROUTER_INC
#define GAME_CLIENT_COMPONENTS_BKW_BKW_COMMUNITY_ROUTER_INC

#include <game/client/components/bkw/server_community.inc>

namespace BkwCommunityRouter
{
inline bool UseTfMenu(EServerCommunity Community)
{
	return Community == EServerCommunity::TEE_FUSION;
}

inline bool UseDdnetMenu(EServerCommunity Community)
{
	return Community == EServerCommunity::DDNET;
}

inline bool UseCustomVoteMenu(EServerCommunity Community, bool Enabled)
{
	return Enabled && (UseTfMenu(Community) || UseDdnetMenu(Community));
}

inline bool IsPersonalizationAvailable(EServerCommunity Community)
{
	return UseTfMenu(Community);
}

inline const char *VoteMenuTitle(EServerCommunity Community)
{
	if(UseTfMenu(Community))
		return "TF Menu";
	if(UseDdnetMenu(Community))
		return "DDNet";
	return "Голосование";
}
}

#endif
