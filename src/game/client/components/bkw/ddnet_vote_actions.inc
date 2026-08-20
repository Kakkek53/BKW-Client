#ifndef GAME_CLIENT_COMPONENTS_BKW_DDNET_VOTE_ACTIONS_INC
#define GAME_CLIENT_COMPONENTS_BKW_DDNET_VOTE_ACTIONS_INC

#include <string>

namespace BkwDdnetVoteActions
{
inline std::string MapVoteCommand(const std::string &Name)
{
	return "/map \"" + Name + "\"";
}

inline bool IsPlayerVote(const char *pVote)
{
	if(!pVote)
		return false;
	return std::string(pVote).find("kick") != std::string::npos || std::string(pVote).find("spectator") != std::string::npos;
}
}

#endif
