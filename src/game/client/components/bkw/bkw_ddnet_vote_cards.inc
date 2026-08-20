#ifndef GAME_CLIENT_COMPONENTS_BKW_BKW_DDNET_VOTE_CARDS_INC
#define GAME_CLIENT_COMPONENTS_BKW_BKW_DDNET_VOTE_CARDS_INC

#include <string>

namespace BkwDdnetVoteCards
{
struct SCard
{
	std::string m_Name;
	std::string m_Image;
	std::string m_Type;
	std::string m_Mapper;
	std::string m_Release;
	int m_Difficulty = 0;
};

inline bool IsValid(const SCard &Card)
{
	return !Card.m_Name.empty();
}

inline std::string MapVoteCommand(const SCard &Card)
{
	return "/map \"" + Card.m_Name + "\"";
}
}

#endif
