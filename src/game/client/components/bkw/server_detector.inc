#ifndef GAME_CLIENT_COMPONENTS_BKW_SERVER_DETECTOR_INC
#define GAME_CLIENT_COMPONENTS_BKW_SERVER_DETECTOR_INC

#include <base/log.h>
#include <base/net.h>
#include <base/str.h>

#include <engine/http.h>
#include <engine/serverbrowser.h>
#include <engine/shared/json.h>

#include <cctype>
#include <memory>
#include <string>
#include <vector>

namespace BkwServerDetector
{

enum class ECommunity
{
	UNKNOWN = 0,
	TEEFUSION,
	DDNET,
};

inline constexpr const char *s_apMasterUrls[] = {
	"https://master1.ddnet.org/ddnet/15/servers.json",
	"https://master2.ddnet.org/ddnet/15/servers.json",
	"https://master3.ddnet.org/ddnet/15/servers.json",
	"https://master4.ddnet.org/ddnet/15/servers.json",
};
inline int s_MasterUrlIndex = 0;
inline std::shared_ptr<IHttpRequest> s_pMasterRequest;
inline bool s_MasterReady = false;
inline bool s_MasterFailed = false;
inline std::vector<NETADDR> s_vTeeFusionAddresses;

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

inline void StartMasterRequest(IHttp *pHttp)
{
	if(!pHttp || s_MasterReady || s_MasterFailed || s_pMasterRequest)
		return;
	if(s_MasterUrlIndex < 0 || s_MasterUrlIndex >= (int)std::size(s_apMasterUrls))
	{
		s_MasterFailed = true;
		return;
	}

	s_pMasterRequest = HttpGet(s_apMasterUrls[s_MasterUrlIndex]);
	s_pMasterRequest->MaxResponseSize(64 * 1024 * 1024);
	s_pMasterRequest->Timeout(CTimeout{5000, 20000, 1024, 8});
	s_pMasterRequest->LogProgress(HTTPLOG::FAILURE);
	pHttp->Run(s_pMasterRequest);
}

inline void TryNextMaster(IHttp *pHttp)
{
	s_pMasterRequest.reset();
	++s_MasterUrlIndex;
	if(s_MasterUrlIndex >= (int)std::size(s_apMasterUrls))
	{
		s_MasterFailed = true;
		log_error("bkw-master", "failed to load DDNet master server list for TeeFusion detection");
		return;
	}
	StartMasterRequest(pHttp);
}

inline bool ParseMasterTeeFusionAddresses(json_value *pRoot, std::vector<NETADDR> &vAddresses)
{
	if(!pRoot || pRoot->type != json_object)
		return false;

	const json_value *pServers = json_object_get(pRoot, "servers");
	if(!pServers || pServers->type != json_array)
		return false;

	for(unsigned ServerIndex = 0; ServerIndex < pServers->u.array.length; ++ServerIndex)
	{
		const json_value *pServer = pServers->u.array.values[ServerIndex];
		if(!pServer || pServer->type != json_object)
			continue;

		const json_value *pCommunity = json_object_get(pServer, "community");
		if(!pCommunity || pCommunity->type != json_string || str_comp_nocase(pCommunity->u.string.ptr, "teefusion") != 0)
			continue;

		const json_value *pAddresses = json_object_get(pServer, "addresses");
		if(!pAddresses || pAddresses->type != json_array)
			continue;

		for(unsigned AddressIndex = 0; AddressIndex < pAddresses->u.array.length; ++AddressIndex)
		{
			const json_value *pAddress = pAddresses->u.array.values[AddressIndex];
			if(!pAddress || pAddress->type != json_string)
				continue;

			NETADDR Address;
			if(net_addr_from_url(&Address, pAddress->u.string.ptr, nullptr, 0) == 0 && Address.port != 0)
				vAddresses.push_back(Address);
		}
	}

	return true;
}

inline void UpdateMaster(IHttp *pHttp)
{
	if(s_MasterReady || s_MasterFailed)
		return;

	StartMasterRequest(pHttp);
	if(!s_pMasterRequest || !s_pMasterRequest->Done())
		return;

	if(s_pMasterRequest->State() != EHttpState::DONE)
	{
		TryNextMaster(pHttp);
		return;
	}

	json_value *pRoot = s_pMasterRequest->ResultJson();
	std::vector<NETADDR> vAddresses;
	const bool Parsed = ParseMasterTeeFusionAddresses(pRoot, vAddresses);
	if(pRoot)
		json_value_free(pRoot);

	if(!Parsed)
	{
		TryNextMaster(pHttp);
		return;
	}

	s_vTeeFusionAddresses = std::move(vAddresses);
	s_MasterReady = true;
	s_pMasterRequest.reset();
	log_info("bkw-master", "loaded %d TeeFusion addresses from DDNet master", (int)s_vTeeFusionAddresses.size());
}

inline bool IsTeeFusion(IHttp *pHttp, const NETADDR &ServerAddress)
{
	UpdateMaster(pHttp);
	if(!s_MasterReady)
		return false;

	for(const NETADDR &Address : s_vTeeFusionAddresses)
	{
		if(net_addr_comp(&Address, &ServerAddress) == 0)
			return true;
	}
	return false;
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

inline ECommunity Detect(IHttp *pHttp, const NETADDR &ServerAddress, const CServerInfo &ServerInfo)
{
	if(IsTeeFusion(pHttp, ServerAddress))
		return ECommunity::TEEFUSION;
	if(IsDdnet(ServerInfo))
		return ECommunity::DDNET;
	return ECommunity::UNKNOWN;
}

} // namespace BkwServerDetector

#endif
