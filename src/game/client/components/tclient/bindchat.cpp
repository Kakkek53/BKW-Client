#include "bindchat.h"

#include <base/log.h>
#include <base/system.h>

#include <engine/http.h>
#include <engine/shared/config.h>
#include <engine/shared/localization.h>
#include <engine/external/json-parser/json.h>

#include <game/client/components/chat.h>
#include <game/client/gameclient.h>
#include <game/localization.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

static constexpr LOG_COLOR BINDCHAT_PRINT_COLOR{255, 255, 204};
static constexpr size_t BKW_PLAYER_INFO_MAX_RESPONSE_SIZE = 8 * 1024 * 1024;

namespace
{
int BkwJsonInt(const json_value &Value)
{
	if(Value.type == json_integer)
		return (int)Value.u.integer;
	if(Value.type == json_double)
		return (int)Value.u.dbl;
	return 0;
}

const char *BkwJsonString(const json_value &Value)
{
	return Value.type == json_string ? Value.u.string.ptr : "";
}

void BkwFormatRaceTime(double Seconds, char *pBuf, size_t BufSize)
{
	if(Seconds < 0.0)
		Seconds = 0.0;
	const int Minutes = (int)(Seconds / 60.0);
	const double SecondsPart = Seconds - Minutes * 60.0;
	str_format(pBuf, (int)BufSize, "%02d:%05.2f", Minutes, SecondsPart);
}
} // namespace

CBindChat::CBind::CBind(const char *pName, const char *pCommand)
{
	str_copy(m_aName, pName);
	str_copy(m_aCommand, pCommand);
}

bool CBindChat::CBind::CompContent(const CBind &Other) const
{
	if(str_comp(m_aCommand, Other.m_aCommand) != 0)
		return false;
	return true;
}

decltype(CBindChat::BIND_DEFAULTS) CBindChat::BIND_DEFAULTS = {
	{TCLocalizable("Kaomoji"), {
					   {TCLocalizable("Shrug:"), {"!shrug", "chai tclient/builtinscripts/sayemoticon.chai ¯\\_(ツ)_/¯D"}},
					   {TCLocalizable("Flip:"), {"!flip", "chai tclient/builtinscripts/sayemoticon.chai (╯°□°)╯︵ ┻━┻D"}},
					   {TCLocalizable("Unflip:"), {"!unflip", "chai tclient/builtinscripts/sayemoticon.chai ┬─┬ノ( º _ ºノ)D"}},
					   {TCLocalizable("Cute:"), {"!cute", "chai tclient/builtinscripts/sayemoticon.chai ૮ ˶ᵔ ᵕ ᵔ˶ აD"}},
					   {TCLocalizable("Lenny:"), {"!lenny", "chai tclient/builtinscripts/sayemoticon.chai ( ͡° ͜ʖ ͡°)D"}},
				   }},
	{TCLocalizable("Warlist"), {
					   {TCLocalizable("Add war name:"), {"!war", "war_name_index 1"}},
					   {TCLocalizable("Add war clan:"), {"!warclan", "war_clan_index 1"}},
					   {TCLocalizable("Add team name:"), {"!team", "war_name_index 2"}},
					   {TCLocalizable("Add team clan:"), {"!teamclan", "war_clan_index 2"}},
					   {TCLocalizable("Remove war name:"), {"!delwar", "remove_war_name_index 1"}},
					   {TCLocalizable("Remove war clan:"), {"!delwarclan", "remove_war_clan_index 1"}},
					   {TCLocalizable("Remove team name:"), {"!delteam", "remove_war_name_index 2"}},
					   {TCLocalizable("Remove team clan:"), {"!delteamclan", "remove_war_clan_index 2"}},
					   {TCLocalizable("Add [group] [name] [reason]:"), {"!name", "war_name"}},
					   {TCLocalizable("Add [group] [clan] [reason]:"), {"!clan", "war_clan"}},
					   {TCLocalizable("Remove [group] [name]:"), {"!delname", "remove_war_name"}},
					   {TCLocalizable("Remove [group] [clan]:"), {"!delclan", "remove_war_clan"}},
				   }},
	{TCLocalizable("Other"), {
					 {TCLocalizable("Translate:"), {"!translate", "translate"}},
					 {TCLocalizable("Translate ID:"), {"!translateid", "translate_id"}},
					 {TCLocalizable("Mute:"), {"!mute", "add_foe"}},
					 {TCLocalizable("Unmute:"), {"!unmute", "remove_foe"}},
					 {TCLocalizable("Add censor word:"), {".addcensor", "add_censor_list"}},
					 {TCLocalizable("Add filter whitelist:"), {".addwhitelist", "add_white_list"}},
				 }},
};

CBindChat::CBindChat()
{
	OnReset();
}

void CBindChat::ConAddBindchat(IConsole::IResult *pResult, void *pUserData)
{
	const char *pName = pResult->GetString(0);
	const char *pCommand = pResult->GetString(1);

	CBindChat *pThis = static_cast<CBindChat *>(pUserData);
	pThis->AddBind({pName, pCommand});
}

void CBindChat::ConBindchats(IConsole::IResult *pResult, void *pUserData)
{
	CBindChat *pThis = static_cast<CBindChat *>(pUserData);
	if(pResult->NumArguments() == 1)
	{
		const char *pName = pResult->GetString(0);
		for(const CBind &Bind : pThis->m_vBinds)
		{
			if(str_comp_nocase(Bind.m_aName, pName) == 0)
			{
				log_info_color(BINDCHAT_PRINT_COLOR, "bindchat", "%s = %s", Bind.m_aName, Bind.m_aCommand);
				return;
			}
		}
		log_info_color(BINDCHAT_PRINT_COLOR, "bindchat", "%s is not bound", pName);
	}
	else
	{
		for(const CBind &Bind : pThis->m_vBinds)
			log_info_color(BINDCHAT_PRINT_COLOR, "bindchat", "%s = %s", Bind.m_aName, Bind.m_aCommand);
	}
}

void CBindChat::ConRemoveBindchat(IConsole::IResult *pResult, void *pUserData)
{
	const char *aName = pResult->GetString(0);
	CBindChat *pThis = static_cast<CBindChat *>(pUserData);
	if(!pThis->RemoveBind(aName))
		log_info_color(BINDCHAT_PRINT_COLOR, "bindchat", "bindchat \"%s\" not found", aName);
}

void CBindChat::ConRemoveBindchatAll(IConsole::IResult *pResult, void *pUserData)
{
	CBindChat *pThis = static_cast<CBindChat *>(pUserData);
	pThis->RemoveAllBinds();
}

void CBindChat::ConBindchatDefaults(IConsole::IResult *pResult, void *pUserData)
{
	CBindChat *pThis = static_cast<CBindChat *>(pUserData);

	for(const auto &[_, vBindDefaults] : CBindChat::BIND_DEFAULTS)
		for(const CBindChat::CBindDefault &BindDefault : vBindDefaults)
			pThis->AddBind(BindDefault.m_Bind);
}

void CBindChat::ConBkwPlayerInfo(IConsole::IResult *pResult, void *pUserData)
{
	CBindChat *pThis = static_cast<CBindChat *>(pUserData);
	if(pResult->NumArguments() < 1 || pResult->GetString(0)[0] == '\0')
	{
		pThis->PrintBkwPlayerInfoLine("Использование: bkw_player_info <ник>");
		return;
	}
	pThis->StartBkwPlayerInfoRequest(pResult->GetString(0));
}

void CBindChat::ConBkwPlayerInfoEnabled(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CBindChat *>(pUserData)->SetBkwPlayerInfoEnabled(pResult->GetInteger(0) != 0);
}

void CBindChat::ConBkwPlayerInfoChatCommand(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CBindChat *>(pUserData)->SetBkwPlayerInfoChatCommand(pResult->GetString(0));
}

void CBindChat::AddBind(const CBind &Bind)
{
	RemoveBind(Bind.m_aName); // Prevent duplicates
	m_vBinds.push_back(Bind);
}

bool CBindChat::RemoveBind(const char *pName)
{
	for(auto It = m_vBinds.begin(); It != m_vBinds.end(); ++It)
	{
		if(str_comp(It->m_aName, pName) == 0)
		{
			m_vBinds.erase(It);
			return true;
		}
	}
	return false;
}

void CBindChat::RemoveAllBinds()
{
	m_vBinds.clear();
}

CBindChat::CBind *CBindChat::GetBind(const char *pCommand)
{
	if(pCommand[0] == '\0')
		return nullptr;
	for(auto &Bind : m_vBinds)
		if(str_comp_nocase(Bind.m_aCommand, pCommand) == 0)
			return &Bind;
	return nullptr;
}

void CBindChat::OnConsoleInit()
{
	IConfigManager *pConfigManager = Kernel()->RequestInterface<IConfigManager>();
	if(pConfigManager)
		pConfigManager->RegisterCallback(ConfigSaveCallback, this, ConfigDomain::TCLIENTCHATBINDS);

	Console()->Register("bindchat", "s[name] r[command]", CFGFLAG_CLIENT, ConAddBindchat, this, "Add a chat bind");
	Console()->Register("bindchats", "?s[name]", CFGFLAG_CLIENT, ConBindchats, this, "Print command executed by this name or all chat binds");
	Console()->Register("unbindchat", "s[name]", CFGFLAG_CLIENT, ConRemoveBindchat, this, "Remove a chat bind");
	Console()->Register("unbindchatall", "", CFGFLAG_CLIENT, ConRemoveBindchatAll, this, "Removes all chat binds");
	Console()->Register("bindchatdefaults", "", CFGFLAG_CLIENT, ConBindchatDefaults, this, "Adds default chat binds");
	Console()->Register("bkw_player_info", "r[name]", CFGFLAG_CLIENT, ConBkwPlayerInfo, this, "Show DDNet race information for a player locally");
	Console()->Register("bkw_player_info_enabled", "i[enabled]", CFGFLAG_CLIENT, ConBkwPlayerInfoEnabled, this, "Enable BKW local player race information command");
	Console()->Register("bkw_player_info_chat_command", "s[command]", CFGFLAG_CLIENT, ConBkwPlayerInfoChatCommand, this, "Set BKW local chat command for player race information");

	ConBindchatDefaults(nullptr, this);
}

void CBindChat::OnRender()
{
	FinishBkwPlayerInfoRequest();
}

void CBindChat::OnStateChange(int NewState, int OldState)
{
	(void)OldState;
	if(NewState == IClient::STATE_OFFLINE && m_pBkwPlayerInfoRequest)
	{
		m_pBkwPlayerInfoRequest->Abort();
		m_pBkwPlayerInfoRequest = nullptr;
	}
}

void CBindChat::ExecuteBind(const CBindChat::CBind &Bind, const char *pArgs)
{
	char aBuf[BINDCHAT_MAX_CMD] = "";
	str_append(aBuf, Bind.m_aCommand);
	if(pArgs && pArgs[0] != '\0')
	{
		str_append(aBuf, " ");
		str_append(aBuf, pArgs);
		if(aBuf[str_length(aBuf) - 1] == ' ')
			aBuf[str_length(aBuf) - 1] = '\0';
	}
	Console()->ExecuteLine(aBuf, IConsole::CLIENT_ID_UNSPECIFIED);
}

bool CBindChat::MatchBkwPlayerInfoCommand(const char *pText, const char **ppPlayerName) const
{
	if(!m_BkwPlayerInfoEnabled || !pText || m_aBkwPlayerInfoChatCommand[0] == '\0')
		return false;

	const size_t CommandLength = str_length(m_aBkwPlayerInfoChatCommand);
	if(str_comp_nocase_num(pText, m_aBkwPlayerInfoChatCommand, CommandLength) != 0)
		return false;
	if(pText[CommandLength] != '\0' && pText[CommandLength] != ' ')
		return false;

	const char *pName = pText + CommandLength;
	while(*pName == ' ')
		++pName;
	if(ppPlayerName)
		*ppPlayerName = pName;
	return true;
}

bool CBindChat::CheckBindChat(const char *pText)
{
	const char *pIgnoredName = nullptr;
	if(MatchBkwPlayerInfoCommand(pText, &pIgnoredName))
		return true;

	const char *pSpace = str_find(pText, " ");
	size_t SpaceIndex = pSpace ? pSpace - pText : strlen(pText);
	for(const CBind &Bind : m_vBinds)
	{
		if(str_comp_nocase_num(pText, Bind.m_aName, SpaceIndex) == 0)
			return true;
	}
	return false;
}

bool CBindChat::ChatDoBinds(const char *pText)
{
	if(!pText || pText[0] == ' ' || pText[0] == '\0')
		return false;

	CChat &Chat = GameClient()->m_Chat;
	const char *pBkwPlayerName = nullptr;
	if(MatchBkwPlayerInfoCommand(pText, &pBkwPlayerName))
	{
		if(!pBkwPlayerName || pBkwPlayerName[0] == '\0')
			PrintBkwPlayerInfoLine("Укажите ник после команды.");
		else
			StartBkwPlayerInfoRequest(pBkwPlayerName);

		const int Length = str_length(pText);
		CChat::CHistoryEntry *pEntry = Chat.m_History.Allocate(sizeof(CChat::CHistoryEntry) + Length);
		pEntry->m_Team = 0;
		str_copy(pEntry->m_aText, pText, Length + 1);
		return true;
	}

	if(pText[1] == '\0')
		return false;

	const char *pSpace = str_find(pText, " ");
	size_t SpaceIndex = pSpace ? pSpace - pText : strlen(pText);
	for(const CBind &Bind : m_vBinds)
	{
		if(str_startswith_nocase(pText, Bind.m_aName) &&
			str_comp_nocase_num(pText, Bind.m_aName, SpaceIndex) == 0)
		{
			ExecuteBind(Bind, pSpace ? pSpace + 1 : nullptr);
			const int Length = str_length(pText);
			CChat::CHistoryEntry *pEntry = Chat.m_History.Allocate(sizeof(CChat::CHistoryEntry) + Length);
			pEntry->m_Team = 0;
			str_copy(pEntry->m_aText, pText, Length + 1);
			return true;
		}
	}
	return false;
}

std::string CBindChat::BkwUrlEncode(const char *pText)
{
	static constexpr char HEX[] = "0123456789ABCDEF";
	std::string Result;
	if(!pText)
		return Result;
	for(const unsigned char *p = reinterpret_cast<const unsigned char *>(pText); *p; ++p)
	{
		const unsigned char C = *p;
		if((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '-' || C == '_' || C == '.' || C == '~')
			Result.push_back((char)C);
		else
		{
			Result.push_back('%');
			Result.push_back(HEX[(C >> 4) & 0xF]);
			Result.push_back(HEX[C & 0xF]);
		}
	}
	return Result;
}

void CBindChat::StartBkwPlayerInfoRequest(const char *pPlayerName)
{
	if(!m_BkwPlayerInfoEnabled || !pPlayerName || pPlayerName[0] == '\0')
		return;

	if(m_pBkwPlayerInfoRequest)
	{
		m_pBkwPlayerInfoRequest->Abort();
		m_pBkwPlayerInfoRequest = nullptr;
	}

	str_copy(m_aBkwPlayerInfoRequestedName, pPlayerName);
	std::string Url = "https://ddnet.org/players/?json2=";
	Url += BkwUrlEncode(pPlayerName);

	std::shared_ptr<IHttpRequest> pGet = HttpGet(Url.c_str());
	pGet->Timeout(CTimeout{8000, 0, 4096, 8});
	pGet->MaxResponseSize(BKW_PLAYER_INFO_MAX_RESPONSE_SIZE);
	pGet->FailOnErrorStatus(false);
	pGet->LogProgress(HTTPLOG::NONE);
	m_pBkwPlayerInfoRequest = pGet;
	Http()->Run(pGet);

	char aBuf[192];
	str_format(aBuf, sizeof(aBuf), "BKW: получаю race-статистику игрока %s…", pPlayerName);
	PrintBkwPlayerInfoLine(aBuf);
}

void CBindChat::FinishBkwPlayerInfoRequest()
{
	if(!m_pBkwPlayerInfoRequest || !m_pBkwPlayerInfoRequest->Done())
		return;

	const bool HttpDone = m_pBkwPlayerInfoRequest->State() == EHttpState::DONE;
	const int StatusCode = HttpDone ? m_pBkwPlayerInfoRequest->StatusCode() : -1;
	if(!HttpDone || StatusCode < 200 || StatusCode >= 400)
	{
		char aBuf[192];
		str_format(aBuf, sizeof(aBuf), "BKW: не удалось получить статистику %s (HTTP %d).", m_aBkwPlayerInfoRequestedName, StatusCode);
		PrintBkwPlayerInfoLine(aBuf);
		m_pBkwPlayerInfoRequest = nullptr;
		return;
	}

	unsigned char *pResult = nullptr;
	size_t ResultSize = 0;
	m_pBkwPlayerInfoRequest->Result(&pResult, &ResultSize);
	if(!pResult || ResultSize == 0)
	{
		PrintBkwPlayerInfoLine("BKW: DDNet вернул пустой ответ.");
		m_pBkwPlayerInfoRequest = nullptr;
		return;
	}

	json_value *pRoot = json_parse(reinterpret_cast<const char *>(pResult), ResultSize);
	if(!pRoot || pRoot->type != json_object)
	{
		if(pRoot)
			json_value_free(pRoot);
		PrintBkwPlayerInfoLine("BKW: не удалось разобрать ответ DDNet API.");
		m_pBkwPlayerInfoRequest = nullptr;
		return;
	}

	PrintBkwPlayerInfo(*pRoot);
	json_value_free(pRoot);
	m_pBkwPlayerInfoRequest = nullptr;
}

void CBindChat::PrintBkwPlayerInfoLine(const char *pText)
{
	GameClient()->m_Chat.AddLine(CChat::CLIENT_MSG, 0, pText);
}

void CBindChat::PrintBkwPlayerInfo(const json_value &Root)
{
	const char *pPlayer = BkwJsonString(Root["player"]);
	if(pPlayer[0] == '\0')
	{
		char aBuf[192];
		str_format(aBuf, sizeof(aBuf), "BKW: игрок %s не найден в DDNet race-статистике.", m_aBkwPlayerInfoRequestedName);
		PrintBkwPlayerInfoLine(aBuf);
		return;
	}

	const json_value &Points = Root["points"];
	const int CurrentPoints = BkwJsonInt(Points["points"]);
	const int TotalPoints = BkwJsonInt(Points["total"]);
	const int PointsRank = BkwJsonInt(Points["rank"]);

	int FinishedMaps = 0;
	int TotalFinishes = 0;
	const json_value &Types = Root["types"];
	if(Types.type == json_object)
	{
		for(unsigned int TypeIndex = 0; TypeIndex < Types.u.object.length; ++TypeIndex)
		{
			const json_value &TypeValue = *Types.u.object.values[TypeIndex].value;
			const json_value &Maps = TypeValue["maps"];
			if(Maps.type != json_object)
				continue;
			for(unsigned int MapIndex = 0; MapIndex < Maps.u.object.length; ++MapIndex)
			{
				const json_value &Map = *Maps.u.object.values[MapIndex].value;
				const int Finishes = BkwJsonInt(Map["finishes"]);
				if(Finishes > 0)
					++FinishedMaps;
				TotalFinishes += maximum(0, Finishes);
			}
		}
	}

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "━━ BKW • %s ━━", pPlayer);
	PrintBkwPlayerInfoLine(aBuf);
	if(PointsRank > 0)
		str_format(aBuf, sizeof(aBuf), "Поинты: %d / %d   •   Ранг: #%d   •   Карт: %d   •   Финишей: %d", CurrentPoints, TotalPoints, PointsRank, FinishedMaps, TotalFinishes);
	else
		str_format(aBuf, sizeof(aBuf), "Поинты: %d / %d   •   Карт: %d   •   Финишей: %d", CurrentPoints, TotalPoints, FinishedMaps, TotalFinishes);
	PrintBkwPlayerInfoLine(aBuf);

	if(Types.type == json_object)
	{
		std::string TypeLine;
		for(unsigned int TypeIndex = 0; TypeIndex < Types.u.object.length; ++TypeIndex)
		{
			const char *pTypeName = Types.u.object.values[TypeIndex].name;
			const json_value &TypeValue = *Types.u.object.values[TypeIndex].value;
			const json_value &TypePoints = TypeValue["points"];
			if(TypePoints.type != json_object)
				continue;
			const int TypeCurrent = BkwJsonInt(TypePoints["points"]);
			const int TypeTotal = BkwJsonInt(TypePoints["total"]);
			if(TypeCurrent == 0 && TypeTotal == 0)
				continue;

			char aType[128];
			str_format(aType, sizeof(aType), "%s: %d/%d", pTypeName, TypeCurrent, TypeTotal);
			if(!TypeLine.empty() && TypeLine.size() + str_length(aType) + 3 > 190)
			{
				PrintBkwPlayerInfoLine(TypeLine.c_str());
				TypeLine.clear();
			}
			if(!TypeLine.empty())
				TypeLine += "  •  ";
			TypeLine += aType;
		}
		if(!TypeLine.empty())
			PrintBkwPlayerInfoLine(TypeLine.c_str());
	}

	const json_value &LastFinishes = Root["last_finishes"];
	if(LastFinishes.type == json_array && LastFinishes.u.array.length > 0)
	{
		PrintBkwPlayerInfoLine("Последние финиши:");
		const unsigned int Count = minimum(3u, LastFinishes.u.array.length);
		for(unsigned int i = 0; i < Count; ++i)
		{
			const json_value &Finish = LastFinishes[(int)i];
			char aTime[32];
			BkwFormatRaceTime((double)Finish["time"], aTime, sizeof(aTime));
			str_format(aBuf, sizeof(aBuf), "%u. %s — %s (%s)", i + 1, BkwJsonString(Finish["map"]), aTime, BkwJsonString(Finish["type"]));
			PrintBkwPlayerInfoLine(aBuf);
		}
	}

	const json_value &Partners = Root["favorite_partners"];
	if(Partners.type == json_array && Partners.u.array.length > 0)
	{
		PrintBkwPlayerInfoLine("Частые напарники:");
		const unsigned int Count = minimum(3u, Partners.u.array.length);
		for(unsigned int i = 0; i < Count; ++i)
		{
			const json_value &Partner = Partners[(int)i];
			str_format(aBuf, sizeof(aBuf), "%u. %s — %d совместных финишей", i + 1, BkwJsonString(Partner["name"]), BkwJsonInt(Partner["finishes"]));
			PrintBkwPlayerInfoLine(aBuf);
		}
	}
}

void CBindChat::SetBkwPlayerInfoEnabled(bool Enabled)
{
	m_BkwPlayerInfoEnabled = Enabled;
	if(!Enabled && m_pBkwPlayerInfoRequest)
	{
		m_pBkwPlayerInfoRequest->Abort();
		m_pBkwPlayerInfoRequest = nullptr;
	}
}

void CBindChat::SetBkwPlayerInfoChatCommand(const char *pCommand)
{
	if(!pCommand)
		return;
	while(*pCommand == ' ')
		++pCommand;
	char aCommand[BKW_PLAYER_INFO_COMMAND_MAX];
	str_copy(aCommand, pCommand);
	if(char *pSpace = const_cast<char *>(str_find(aCommand, " ")))
		*pSpace = '\0';
	if(aCommand[0] == '\0')
		str_copy(aCommand, ".player");
	str_copy(m_aBkwPlayerInfoChatCommand, aCommand);
}

bool CBindChat::ChatDoAutocomplete(bool ShiftPressed)
{
	CChat &Chat = GameClient()->m_Chat;

	if(m_vBinds.empty())
		return false;
	if(*Chat.m_aCompletionBuffer == '\0')
		return false;

	const CBind *pCompletionBind = nullptr;
	int InitialCompletionChosen = Chat.m_CompletionChosen;
	int InitialCompletionUsed = Chat.m_CompletionUsed;

	if(ShiftPressed && Chat.m_CompletionUsed)
		Chat.m_CompletionChosen--;
	else if(!ShiftPressed)
		Chat.m_CompletionChosen++;
	Chat.m_CompletionChosen = (Chat.m_CompletionChosen + m_vBinds.size()) % m_vBinds.size();

	Chat.m_CompletionUsed = true;
	int Index = Chat.m_CompletionChosen;
	for(int i = 0; i < (int)m_vBinds.size(); i++)
	{
		int CommandIndex = (Index + i) % m_vBinds.size();
		if(str_startswith_nocase(m_vBinds.at(CommandIndex).m_aName, Chat.m_aCompletionBuffer))
		{
			pCompletionBind = &m_vBinds.at(CommandIndex);
			Chat.m_CompletionChosen = CommandIndex;
			break;
		}
	}

	if(pCompletionBind)
	{
		char aBuf[CChat::MAX_LINE_LENGTH];
		str_truncate(aBuf, sizeof(aBuf), Chat.m_Input.GetString(), Chat.m_PlaceholderOffset);
		str_append(aBuf, pCompletionBind->m_aName);
		const char *pSeparator = " ";
		str_append(aBuf, pSeparator);
		str_append(aBuf, Chat.m_Input.GetString() + Chat.m_PlaceholderOffset + Chat.m_PlaceholderLength);

		Chat.m_PlaceholderLength = str_length(pSeparator) + str_length(pCompletionBind->m_aName);
		Chat.m_Input.Set(aBuf);
		Chat.m_Input.SetCursorOffset(Chat.m_PlaceholderOffset + Chat.m_PlaceholderLength);
	}
	else
	{
		Chat.m_CompletionChosen = InitialCompletionChosen;
		Chat.m_CompletionUsed = InitialCompletionUsed;
	}

	return pCompletionBind != nullptr;
}

void CBindChat::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	CBindChat *pThis = (CBindChat *)pUserData;

	{
		char aBuf[160];
		str_format(aBuf, sizeof(aBuf), "bkw_player_info_enabled %d", pThis->m_BkwPlayerInfoEnabled ? 1 : 0);
		pConfigManager->WriteLine(aBuf, ConfigDomain::TCLIENTCHATBINDS);
		char aEscaped[BKW_PLAYER_INFO_COMMAND_MAX * 2 + 64] = "bkw_player_info_chat_command \"";
		char *pEnd = aEscaped + sizeof(aEscaped);
		char *pDst = aEscaped + str_length(aEscaped);
		str_escape(&pDst, pThis->m_aBkwPlayerInfoChatCommand, pEnd);
		str_append(aEscaped, "\"");
		pConfigManager->WriteLine(aEscaped, ConfigDomain::TCLIENTCHATBINDS);
	}

	auto Compare = [&](const CBindChat::CBind &A, const CBindChat::CBind &B) {
		const int Res = str_utf8_comp_nocase(A.m_aName, B.m_aName);
		return Res < 0 || (Res == 0 && str_comp(A.m_aName, B.m_aName) < 0);
	};

	std::vector<std::reference_wrapper<const CBindChat::CBind>> vDefaultBinds;
	for(const auto &[_, vBindDefaults] : CBindChat::BIND_DEFAULTS)
		for(const CBindChat::CBindDefault &BindDefault : vBindDefaults)
			vDefaultBinds.emplace_back(BindDefault.m_Bind);
	std::sort(vDefaultBinds.begin(), vDefaultBinds.end(), Compare);

	std::sort(pThis->m_vBinds.begin(), pThis->m_vBinds.end(), Compare);
	for(CBind &Bind : pThis->m_vBinds)
	{
		const auto It = std::lower_bound(vDefaultBinds.begin(), vDefaultBinds.end(), Bind, Compare);
		if(It != vDefaultBinds.end() && str_utf8_comp_nocase(It->get().m_aName, Bind.m_aName) == 0)
		{
			if(Bind.CompContent(*It))
			{
				vDefaultBinds.erase(It);
				continue;
			}
			else
			{
				vDefaultBinds.erase(It);
			}
		}

		char aBuf[BINDCHAT_MAX_CMD * 2] = "";
		char *pEnd = aBuf + sizeof(aBuf);
		char *pDst;
		str_append(aBuf, "bindchat \"");
		pDst = aBuf + str_length(aBuf);
		str_escape(&pDst, Bind.m_aName, pEnd);
		str_append(aBuf, "\" \"");
		pDst = aBuf + str_length(aBuf);
		str_escape(&pDst, Bind.m_aCommand, pEnd);
		str_append(aBuf, "\"");
		pConfigManager->WriteLine(aBuf, ConfigDomain::TCLIENTCHATBINDS);
	}
	for(const auto &Bind : vDefaultBinds)
	{
		char aBuf[BINDCHAT_MAX_CMD * 2 + 32] = "";
		char *pEnd = aBuf + sizeof(aBuf);
		char *pDst;

		str_append(aBuf, "unbindchat \"");
		pDst = aBuf + str_length(aBuf);
		str_escape(&pDst, Bind.get().m_aName, pEnd);
		str_append(aBuf, "\"");

		pConfigManager->WriteLine(aBuf, ConfigDomain::TCLIENTCHATBINDS);
	}
}
