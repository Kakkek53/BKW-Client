#ifndef GAME_CLIENT_COMPONENTS_BKW_SERVER_DETECTOR_INC
#define GAME_CLIENT_COMPONENTS_BKW_SERVER_DETECTOR_INC

#include <base/str.h>
#include <engine/serverbrowser.h>

#include <cctype>
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

inline bool ContainsNoCase(const char *pText, const char *pNeedle)
{
	return pText && ContainsNoCase(std::string(pText), pNeedle);
}

inline bool IsTeeFusion(const CServerInfo &ServerInfo)
{
	// Community IDs are the preferred signal, but they are not guaranteed to be
	// populated for every server-browser path/version. Fall back to visible
	// server metadata so TeeFusion-specific UI still works on the live servers.
	if(str_comp_nocase(ServerInfo.m_aCommunityId, "teefusion") == 0)
		return true;

	return ContainsNoCase(ServerInfo.m_aName, "teefusion") ||
		ContainsNoCase(ServerInfo.m_aName, "tee fusion") ||
		ContainsNoCase(ServerInfo.m_aVersion, "teefusion") ||
		ContainsNoCase(ServerInfo.m_aVersion, "tee fusion") ||
		ContainsNoCase(ServerInfo.m_aGameType, "teefusion") ||
		ContainsNoCase(ServerInfo.m_aGameType, "tee fusion") ||
		ContainsNoCase(ServerInfo.m_aCommunityType, "teefusion") ||
		ContainsNoCase(ServerInfo.m_aCommunityType, "tee fusion");
}

inline bool IsDdnet(const CServerInfo &ServerInfo)
{
	if(str_comp_nocase(ServerInfo.m_aCommunityId, "ddnet") == 0)
		return true;
	return ContainsNoCase(ServerInfo.m_aName, "ddnet") ||
		ContainsNoCase(ServerInfo.m_aVersion, "ddnet") ||
		ContainsNoCase(ServerInfo.m_aGameType, "ddnet") ||
		ContainsNoCase(ServerInfo.m_aCommunityType, "ddnet");
}

inline ECommunity Detect(const CServerInfo &ServerInfo)
{
	if(IsTeeFusion(ServerInfo))
		return ECommunity::TEEFUSION;
	if(IsDdnet(ServerInfo))
		return ECommunity::DDNET;
	return ECommunity::UNKNOWN;
}

inline ECommunity Detect(const std::string &ServerName, const std::string &GameVersion)
{
	if(ContainsNoCase(ServerName, "teefusion") || ContainsNoCase(ServerName, "tee fusion") ||
		ContainsNoCase(GameVersion, "teefusion") || ContainsNoCase(GameVersion, "tee fusion"))
		return ECommunity::TEEFUSION;

	if(ContainsNoCase(ServerName, "ddnet") || ContainsNoCase(GameVersion, "ddnet"))
		return ECommunity::DDNET;

	return ECommunity::UNKNOWN;
}

} // namespace BkwServerDetector

#endif
