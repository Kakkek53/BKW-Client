#ifndef GAME_CLIENT_COMPONENTS_BKW_SERVER_DETECTOR_INC
#define GAME_CLIENT_COMPONENTS_BKW_SERVER_DETECTOR_INC

#include <string>

namespace BkwServerDetector
{

enum class ECommunity
{
	UNKNOWN = 0,
	TEEFUSION,
	DDNET,
};

inline bool ContainsNoCase(const std::string &Text, const char *pNeedle)
{
	if(!pNeedle || !*pNeedle)
		return false;
	std::string LowerText = Text;
	std::string LowerNeedle = pNeedle;
	for(char &C : LowerText)
		C = (char)std::tolower((unsigned char)C);
	for(char &C : LowerNeedle)
		C = (char)std::tolower((unsigned char)C);
	return LowerText.find(LowerNeedle) != std::string::npos;
}

inline ECommunity Detect(const std::string &ServerName, const std::string &GameVersion)
{
	if(ContainsNoCase(ServerName, "teefusion") || ContainsNoCase(GameVersion, "teefusion"))
		return ECommunity::TEEFUSION;

	if(ContainsNoCase(ServerName, "ddnet") || ContainsNoCase(GameVersion, "ddnet"))
		return ECommunity::DDNET;

	return ECommunity::UNKNOWN;
}

}

#endif
