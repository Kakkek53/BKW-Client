#ifndef GAME_CLIENT_COMPONENTS_BKW_DDNET_MAPS_CATALOG_INC
#define GAME_CLIENT_COMPONENTS_BKW_DDNET_MAPS_CATALOG_INC

#include <string>
#include <vector>

namespace BkwDdnetMaps
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

inline std::string MakeVoteCommand(const std::string &Name)
{
	return "/map \"" + Name + "\"";
}

inline std::string MakeInfoUrl(const std::string &Name)
{
	return "https://ddnet.org/maps/?json=" + Name;
}

inline std::string MakeImageUrl(const std::string &Name)
{
	return "https://ddnet.org/ranks/maps/" + Name + ".png";
}
}

#endif
