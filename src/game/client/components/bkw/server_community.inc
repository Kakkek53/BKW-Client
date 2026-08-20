#ifndef GAME_CLIENT_COMPONENTS_BKW_SERVER_COMMUNITY_INC
#define GAME_CLIENT_COMPONENTS_BKW_SERVER_COMMUNITY_INC

#include <base/str.h>
#include <string>

namespace BkwServerCommunity
{
enum class ECommunity
{
	UNKNOWN,
	TEE_FUSION,
	DDNET
};

inline ECommunity Detect(const std::string &Name, const std::string &Version)
{
	std::string Text = Name + " " + Version;
	for(char &c : Text)
		c = str_tolower(c);

	if(Text.find("teefusion") != std::string::npos || Text.find("tee fusion") != std::string::npos)
		return ECommunity::TEE_FUSION;

	if(Text.find("ddnet") != std::string::npos)
		return ECommunity::DDNET;

	return ECommunity::UNKNOWN;
}
}

#endif
