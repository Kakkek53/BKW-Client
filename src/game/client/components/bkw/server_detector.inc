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

inline const CServerInfo *FindMasterEntry(IServerBrowser *pServerBrowser, const NETADDR &ServerAddress)
{
	if(!pServerBrowser)
		return nullptr;

	IServerBrowser::CServerEntry *pEntry = pServerBrowser->Find(ServerAddress);
	return pEntry ? &pEntry->m_Info : nullptr;
}

inline bool IsTeeFusion(IServerBrowser *pServerBrowser, const NETADDR &ServerAddress)
{
	// TeeFusion is identified only by the community field received from the
	// master server. The connected address must resolve to a master-list entry
	// whose community id is exactly "teefusion". Do not infer this from the
	// server name, version, gametype or gameplay messages.
	const CServerInfo *pMasterInfo = FindMasterEntry(pServerBrowser, ServerAddress);
	return pMasterInfo && str_comp_nocase(pMasterInfo->m_aCommunityId, "teefusion") == 0;
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

inline ECommunity Detect(IServerBrowser *pServerBrowser, const NETADDR &ServerAddress, const CServerInfo &ServerInfo)
{
	if(IsTeeFusion(pServerBrowser, ServerAddress))
		return ECommunity::TEEFUSION;
	if(IsDdnet(ServerInfo))
		return ECommunity::DDNET;
	return ECommunity::UNKNOWN;
}

} // namespace BkwServerDetector

#endif
