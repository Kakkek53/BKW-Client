#include <base/system.h>

#include <engine/client.h>
#include <engine/console.h>
#include <engine/input.h>
#include <engine/keys.h>

#include <game/client/components/voting.h>
#include <game/client/gameclient.h>

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
	Print(pConsole, "END");
	Print(pConsole, "====================================================");
}

inline void OnRender(CGameClient *pGameClient, IClient *pClient, IInput *pInput)
{
	if(pInput && pInput->KeyPress(KEY_F5))
		Dump(pGameClient, pClient);
}
} // namespace BkwVoteMenuDebug
