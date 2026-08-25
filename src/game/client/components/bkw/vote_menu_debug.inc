#include <base/system.h>

#include <engine/client.h>
#include <engine/console.h>
#include <engine/input.h>
#include <engine/keys.h>

#include <game/client/components/voting.h>
#include <game/client/gameclient.h>

#include <game/client/components/bkw/ego_vote_real_format_fix.inc>

namespace BkwVoteMenuDebug
{
inline void Print(IConsole *pConsole, const char *pText)
{
	if(pConsole)
		pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "bkw-vote-dump", pText);
}

inline void Dump(CGameClient *pGameClient, IClient *pClient)
{
	if(!pGameClient || !pClient)
		return;

	IConsole *pConsole = pGameClient->Console();
	if(!pConsole)
		return;

	Print(pConsole, "================ BKW VOTE MENU DUMP ================");
	Print(pConsole, "Copy everything between BEGIN and END and send it back.");
	Print(pConsole, "BEGIN");

	if(pClient->State() != IClient::STATE_ONLINE)
	{
		Print(pConsole, "state=not_online");
		Print(pConsole, "END");
		Print(pConsole, "====================================================");
		return;
	}

	const CServerInfo &ServerInfo = pClient->ServerInfo();
	char aBuf[1024];
	str_format(aBuf, sizeof(aBuf), "server_name=\"%s\"", ServerInfo.m_aName);
	Print(pConsole, aBuf);
	str_format(aBuf, sizeof(aBuf), "server_address=\"%s\"", ServerInfo.m_aAddress);
	Print(pConsole, aBuf);
	str_format(aBuf, sizeof(aBuf), "server_gametype=\"%s\"", ServerInfo.m_aGameType);
	Print(pConsole, aBuf);
	str_format(aBuf, sizeof(aBuf), "server_map=\"%s\"", ServerInfo.m_aMap);
	Print(pConsole, aBuf);
	str_format(aBuf, sizeof(aBuf), "vote_options_reported=%d", pGameClient->m_Voting.NumOptions());
	Print(pConsole, aBuf);
	str_format(aBuf, sizeof(aBuf), "ego_detected=%d ego_details_enabled=%d catalog_ready=%d catalog_failed=%d api_maps_loaded=%d",
		BkwEgoVote::IsEgoServerName(ServerInfo.m_aName) ? 1 : 0,
		g_Config.m_BkwEgoMapDetails,
		BkwEgoVote::s_CatalogReady ? 1 : 0,
		BkwEgoVote::s_CatalogFailed ? 1 : 0,
		(int)BkwEgoVote::s_vMaps.size());
	Print(pConsole, aBuf);
	Print(pConsole, "--- RAW OPTIONS IN EXACT ORDER ---");

	int Index = 0;
	for(const CVoteOptionClient *pOption = pGameClient->m_Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
	{
		// Keep the description untouched. The surrounding quotes make leading/trailing
		// spaces visible in the copied log, which is important for diagnosing grouping.
		str_format(aBuf, sizeof(aBuf), "[%03d] \"%s\"", Index, pOption->m_aDescription);
		Print(pConsole, aBuf);
	}

	str_format(aBuf, sizeof(aBuf), "vote_options_iterated=%d", Index);
	Print(pConsole, aBuf);

	if(BkwEgoVote::IsEgoServerName(ServerInfo.m_aName))
	{
		Print(pConsole, "--- EGO MAP PARSER ---");
		bool InMapsSection = false;
		Index = 0;
		for(const CVoteOptionClient *pOption = pGameClient->m_Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
		{
			const char *pDescription = pOption->m_aDescription;
			if(BkwEgoVoteRealFormat::IsMapsSectionStart(pDescription))
			{
				InMapsSection = true;
				str_format(aBuf, sizeof(aBuf), "section_start[%03d]=\"%s\"", Index, pDescription);
				Print(pConsole, aBuf);
				continue;
			}
			if(InMapsSection && BkwEgoVoteRealFormat::IsSectionEnd(pDescription))
			{
				str_format(aBuf, sizeof(aBuf), "section_end[%03d]=\"%s\"", Index, pDescription);
				Print(pConsole, aBuf);
				InMapsSection = false;
				continue;
			}
			if(!InMapsSection)
				continue;

			std::string MapName;
			if(!BkwEgoVoteRealFormat::ExtractMapName(pDescription, MapName))
				continue;
			const BkwEgoVote::SMap *pMap = BkwEgoVote::FindMapByName(MapName);
			if(pMap)
				str_format(aBuf, sizeof(aBuf), "parsed[%03d] name=\"%s\" api_map_id=%d", Index, MapName.c_str(), pMap->m_Id);
			else
				str_format(aBuf, sizeof(aBuf), "parsed[%03d] name=\"%s\" api_map_id=NOT_FOUND", Index, MapName.c_str());
			Print(pConsole, aBuf);
		}
	}

	Print(pConsole, "END");
	Print(pConsole, "====================================================");
}

inline void OnRender(CGameClient *pGameClient, IClient *pClient, IInput *pInput)
{
	if(pInput && pInput->KeyPress(KEY_F5))
		Dump(pGameClient, pClient);
}
} // namespace BkwVoteMenuDebug
