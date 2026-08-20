#ifndef GAME_CLIENT_COMPONENTS_BKW_BKW_DDNET_MAP_UI_INC
#define GAME_CLIENT_COMPONENTS_BKW_BKW_DDNET_MAP_UI_INC

#include <string>

namespace BkwDdnetMapUi
{
struct SCard
{
	std::string m_Name;
	std::string m_Mapper;
	std::string m_Type;
	std::string m_Image;
	int m_Difficulty = 0;
};

inline std::string VoteCommand(const std::string &Name)
{
	return "/map \"" + Name + "\"";
}

inline std::string ImageUrl(const std::string &Name)
{
	return "https://ddnet.org/ranks/maps/" + Name + ".png";
}

inline std::string InfoUrl(const std::string &Name)
{
	return "https://ddnet.org/maps/?json=" + Name;
}
}

#endif
