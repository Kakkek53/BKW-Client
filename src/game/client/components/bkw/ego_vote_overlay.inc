#ifndef GAME_CLIENT_COMPONENTS_BKW_EGO_VOTE_OVERLAY_INC
#define GAME_CLIENT_COMPONENTS_BKW_EGO_VOTE_OVERLAY_INC

#include <base/str.h>

#include <engine/client.h>
#include <engine/http.h>
#include <engine/shared/config.h>
#include <engine/shared/json.h>

#include <game/client/components/menus.h>
#include <game/client/components/voting.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace BkwEgoVote
{
struct SMap
{
	int m_Id = -1;
	std::string m_Name;
	std::string m_ServerType;
	std::string m_ServerTypeLabel;
	std::string m_Difficulty;
	int m_Points = 0;
	std::string m_Author;
	int m_Stars = 0;
	std::string m_ReleasedAt;
	int m_TotalFinishes = 0;
	int64_t m_AverageTimeMs = -1;
	std::string m_LengthLabel;
};

struct SRank
{
	int m_Rank = 0;
	std::string m_Nick;
	double m_TimeSeconds = 0.0;
	std::string m_Clan;
};

struct SLeaderboard
{
	bool m_Ready = false;
	bool m_Failed = false;
	int m_MapId = -1;
	std::string m_MapName;
	std::string m_MapCreator;
	std::string m_MapDifficulty;
	int m_MapPoints = 0;
	std::string m_MapReleasedAt;
	int m_TotalUsers = 0;
	std::vector<SRank> m_vRanks;
};

inline CUi *s_pUi = nullptr;
inline CGameClient *s_pGameClient = nullptr;
inline IHttp *s_pHttp = nullptr;

inline std::vector<SMap> s_vMaps;
inline std::shared_ptr<IHttpRequest> s_pCatalogRequest;
inline int s_CatalogOffset = 0;
inline int s_CatalogTotal = -1;
inline bool s_CatalogReady = false;
inline bool s_CatalogFailed = false;

inline SLeaderboard s_Leaderboard;
inline std::shared_ptr<IHttpRequest> s_pLeaderboardRequest;
inline int s_SelectedMapId = -1;

inline std::vector<CButtonContainer> s_vDetailsButtons;
inline CButtonContainer s_DetailCloseButton;
inline CButtonContainer s_RetryLeaderboardButton;
inline SPopupMenuId s_DetailsPopupId;

inline const json_value *JsonField(const json_value *pObject, const char *pKey)
{
	if(!pObject || pObject->type != json_object)
		return nullptr;
	return json_object_get(pObject, pKey);
}

inline bool JsonString(const json_value *pObject, const char *pKey, std::string &Out)
{
	const json_value *pValue = JsonField(pObject, pKey);
	if(!pValue || pValue->type != json_string)
		return false;
	Out = pValue->u.string.ptr;
	return true;
}

inline int JsonInt(const json_value *pObject, const char *pKey, int Default = 0)
{
	const json_value *pValue = JsonField(pObject, pKey);
	if(!pValue)
		return Default;
	if(pValue->type == json_integer)
		return (int)pValue->u.integer;
	if(pValue->type == json_double)
		return (int)pValue->u.dbl;
	return Default;
}

inline int64_t JsonInt64(const json_value *pObject, const char *pKey, int64_t Default = 0)
{
	const json_value *pValue = JsonField(pObject, pKey);
	if(!pValue)
		return Default;
	if(pValue->type == json_integer)
		return (int64_t)pValue->u.integer;
	if(pValue->type == json_double)
		return (int64_t)pValue->u.dbl;
	return Default;
}

inline double JsonDouble(const json_value *pObject, const char *pKey, double Default = 0.0)
{
	const json_value *pValue = JsonField(pObject, pKey);
	if(!pValue)
		return Default;
	if(pValue->type == json_double)
		return pValue->u.dbl;
	if(pValue->type == json_integer)
		return (double)pValue->u.integer;
	return Default;
}

inline std::string Trim(const char *pText)
{
	if(!pText)
		return {};
	const char *pStart = pText;
	while(*pStart == ' ' || *pStart == '\t')
		++pStart;
	const char *pEnd = pStart + str_length(pStart);
	while(pEnd > pStart && (pEnd[-1] == ' ' || pEnd[-1] == '\t'))
		--pEnd;
	return std::string(pStart, pEnd);
}

inline bool IsEgoServerName(const char *pName)
{
	return pName && str_utf8_find_nocase(pName, "EGO |") != nullptr && str_utf8_find_nocase(pName, "[eternal-gores.com]") != nullptr;
}

inline bool IsMapsHeader(const char *pDescription)
{
	if(!pDescription)
		return false;
	const char *pMaps = str_utf8_find_nocase(pDescription, "Maps (");
	if(!pMaps)
		return false;
	const char *pColon = str_find(pMaps, ":");
	const char *pClose = str_find(pMaps, ")");
	return pColon && pClose && pColon < pClose;
}

inline bool LooksLikeGroupHeader(const char *pDescription)
{
	if(!pDescription)
		return false;
	const char *pOpen = str_find(pDescription, "(");
	const char *pColon = pOpen ? str_find(pOpen, ":") : nullptr;
	const char *pClose = pColon ? str_find(pColon, ")") : nullptr;
	return pOpen && pColon && pClose && pColon < pClose;
}

inline const SMap *FindMapById(int MapId)
{
	for(const auto &Map : s_vMaps)
		if(Map.m_Id == MapId)
			return &Map;
	return nullptr;
}

inline const SMap *FindMapByName(const std::string &Name)
{
	for(const auto &Map : s_vMaps)
		if(str_comp(Map.m_Name.c_str(), Name.c_str()) == 0)
			return &Map;
	for(const auto &Map : s_vMaps)
		if(str_comp_nocase(Map.m_Name.c_str(), Name.c_str()) == 0)
			return &Map;
	return nullptr;
}

inline void RetryCatalog()
{
	s_vMaps.clear();
	s_pCatalogRequest.reset();
	s_CatalogOffset = 0;
	s_CatalogTotal = -1;
	s_CatalogReady = false;
	s_CatalogFailed = false;
}

inline void StartCatalogRequest(IHttp *pHttp)
{
	if(!pHttp || s_CatalogReady || s_CatalogFailed || s_pCatalogRequest)
		return;
	char aUrl[192];
	str_format(aUrl, sizeof(aUrl), "https://eternal-gores.com/api/maps?limit=100&offset=%d", s_CatalogOffset);
	s_pCatalogRequest = HttpGet(aUrl);
	s_pCatalogRequest->MaxResponseSize(16 * 1024 * 1024);
	s_pCatalogRequest->Timeout(CTimeout{5000, 20000, 1024, 8});
	s_pCatalogRequest->LogProgress(HTTPLOG::FAILURE);
	pHttp->Run(s_pCatalogRequest);
}

inline void UpdateCatalog(IHttp *pHttp)
{
	StartCatalogRequest(pHttp);
	if(!s_pCatalogRequest || !s_pCatalogRequest->Done())
		return;
	if(s_pCatalogRequest->State() != EHttpState::DONE)
	{
		s_CatalogFailed = true;
		s_pCatalogRequest.reset();
		return;
	}

	json_value *pRoot = s_pCatalogRequest->ResultJson();
	if(!pRoot || pRoot->type != json_object)
	{
		if(pRoot)
			json_value_free(pRoot);
		s_CatalogFailed = true;
		s_pCatalogRequest.reset();
		return;
	}

	const json_value *pItems = JsonField(pRoot, "items");
	if(!pItems || pItems->type != json_array)
	{
		json_value_free(pRoot);
		s_CatalogFailed = true;
		s_pCatalogRequest.reset();
		return;
	}

	if(s_CatalogTotal < 0)
		s_CatalogTotal = JsonInt(pRoot, "total", (int)pItems->u.array.length);

	const int Before = (int)s_vMaps.size();
	for(unsigned i = 0; i < pItems->u.array.length; ++i)
	{
		const json_value *pEntry = pItems->u.array.values[i];
		if(!pEntry || pEntry->type != json_object)
			continue;
		SMap Map;
		Map.m_Id = JsonInt(pEntry, "id", -1);
		if(Map.m_Id < 0 || !JsonString(pEntry, "name", Map.m_Name) || Map.m_Name.empty())
			continue;
		JsonString(pEntry, "server_type", Map.m_ServerType);
		JsonString(pEntry, "server_type_label", Map.m_ServerTypeLabel);
		JsonString(pEntry, "difficulty", Map.m_Difficulty);
		Map.m_Points = JsonInt(pEntry, "points");
		JsonString(pEntry, "author", Map.m_Author);
		Map.m_Stars = JsonInt(pEntry, "stars");
		JsonString(pEntry, "released_at", Map.m_ReleasedAt);
		Map.m_TotalFinishes = JsonInt(pEntry, "total_finishes");
		Map.m_AverageTimeMs = JsonInt64(pEntry, "average_time_ms", -1);
		JsonString(pEntry, "length_label", Map.m_LengthLabel);
		s_vMaps.push_back(std::move(Map));
	}
	const int Added = (int)s_vMaps.size() - Before;
	const int Received = (int)pItems->u.array.length;
	json_value_free(pRoot);
	s_pCatalogRequest.reset();

	if(Received <= 0 || Added <= 0 || (s_CatalogTotal >= 0 && (int)s_vMaps.size() >= s_CatalogTotal))
	{
		s_CatalogReady = !s_vMaps.empty();
		s_CatalogFailed = s_vMaps.empty();
		return;
	}
	s_CatalogOffset += Received;
	StartCatalogRequest(pHttp);
}

inline void StartLeaderboardRequest(IHttp *pHttp, int MapId)
{
	if(!pHttp || MapId < 0)
		return;
	if(s_pLeaderboardRequest && s_Leaderboard.m_MapId == MapId)
		return;
	if(s_Leaderboard.m_Ready && s_Leaderboard.m_MapId == MapId)
		return;

	s_pLeaderboardRequest.reset();
	s_Leaderboard = SLeaderboard();
	s_Leaderboard.m_MapId = MapId;
	s_SelectedMapId = MapId;
	char aUrl[192];
	str_format(aUrl, sizeof(aUrl), "https://eternal-gores.com/api/leaderboard?map_id=%d", MapId);
	s_pLeaderboardRequest = HttpGet(aUrl);
	s_pLeaderboardRequest->MaxResponseSize(16 * 1024 * 1024);
	s_pLeaderboardRequest->Timeout(CTimeout{5000, 20000, 1024, 8});
	s_pLeaderboardRequest->LogProgress(HTTPLOG::FAILURE);
	pHttp->Run(s_pLeaderboardRequest);
}

inline void UpdateLeaderboard()
{
	if(!s_pLeaderboardRequest || !s_pLeaderboardRequest->Done())
		return;
	if(s_pLeaderboardRequest->State() != EHttpState::DONE)
	{
		s_Leaderboard.m_Failed = true;
		s_pLeaderboardRequest.reset();
		return;
	}

	json_value *pRoot = s_pLeaderboardRequest->ResultJson();
	if(!pRoot || pRoot->type != json_object)
	{
		if(pRoot)
			json_value_free(pRoot);
		s_Leaderboard.m_Failed = true;
		s_pLeaderboardRequest.reset();
		return;
	}

	s_Leaderboard.m_MapId = JsonInt(pRoot, "map_id", s_Leaderboard.m_MapId);
	JsonString(pRoot, "map_name", s_Leaderboard.m_MapName);
	JsonString(pRoot, "map_creator", s_Leaderboard.m_MapCreator);
	JsonString(pRoot, "map_difficulty", s_Leaderboard.m_MapDifficulty);
	s_Leaderboard.m_MapPoints = JsonInt(pRoot, "map_points");
	JsonString(pRoot, "map_released_at", s_Leaderboard.m_MapReleasedAt);
	s_Leaderboard.m_TotalUsers = JsonInt(pRoot, "total_users");
	const json_value *pItems = JsonField(pRoot, "items");
	if(pItems && pItems->type == json_array)
	{
		s_Leaderboard.m_vRanks.reserve(pItems->u.array.length);
		for(unsigned i = 0; i < pItems->u.array.length; ++i)
		{
			const json_value *pEntry = pItems->u.array.values[i];
			if(!pEntry || pEntry->type != json_object)
				continue;
			SRank Rank;
			Rank.m_Rank = JsonInt(pEntry, "rank", (int)i + 1);
			JsonString(pEntry, "nick", Rank.m_Nick);
			Rank.m_TimeSeconds = JsonDouble(pEntry, "time_seconds", JsonDouble(pEntry, "best_time", 0.0));
			const json_value *pClan = JsonField(pEntry, "clan");
			if(pClan && pClan->type == json_object)
				JsonString(pClan, "name", Rank.m_Clan);
			if(!Rank.m_Nick.empty())
				s_Leaderboard.m_vRanks.push_back(std::move(Rank));
		}
	}
	json_value_free(pRoot);
	s_Leaderboard.m_Ready = true;
	s_Leaderboard.m_Failed = false;
	s_pLeaderboardRequest.reset();
}

inline void FormatDurationMs(char *pBuf, size_t BufSize, int64_t Milliseconds)
{
	if(Milliseconds < 0)
	{
		str_copy(pBuf, "—", BufSize);
		return;
	}
	const int64_t Minutes = Milliseconds / 60000;
	const int64_t Seconds = (Milliseconds / 1000) % 60;
	const int64_t Millis = Milliseconds % 1000;
	if(Minutes > 0)
		str_format(pBuf, BufSize, "%lld:%02lld.%03lld", (long long)Minutes, (long long)Seconds, (long long)Millis);
	else
		str_format(pBuf, BufSize, "%lld.%03lld s", (long long)Seconds, (long long)Millis);
}

inline void FormatDurationSeconds(char *pBuf, size_t BufSize, double Seconds)
{
	FormatDurationMs(pBuf, BufSize, Seconds > 0.0 ? (int64_t)std::llround(Seconds * 1000.0) : -1);
}

inline bool ResolveVoteMap(const void *pItemId, int &OptionIndex, int &MapId)
{
	if(!s_pGameClient || !pItemId)
		return false;
	bool InMapsSection = false;
	int Index = 0;
	for(const CVoteOptionClient *pOption = s_pGameClient->m_Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
	{
		if(LooksLikeGroupHeader(pOption->m_aDescription))
			InMapsSection = IsMapsHeader(pOption->m_aDescription);
		if(pOption != pItemId)
			continue;
		if(!InMapsSection || LooksLikeGroupHeader(pOption->m_aDescription))
			return false;
		const SMap *pMap = FindMapByName(Trim(pOption->m_aDescription));
		if(!pMap)
			return false;
		OptionIndex = Index;
		MapId = pMap->m_Id;
		return true;
	}
	return false;
}

inline CUi::EPopupMenuFunctionResult RenderDetailsPopup(void *, CUIRect View, bool Active)
{
	if(!s_pUi || !s_pGameClient)
		return CUi::POPUP_CLOSE_CURRENT;
	UpdateLeaderboard();
	if(Active && s_pUi->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
		return CUi::POPUP_CLOSE_CURRENT;

	View.Margin(10.0f, &View);
	const SMap *pMap = FindMapById(s_SelectedMapId);
	CUIRect Header, Close;
	View.HSplitTop(28.0f, &Header, &View);
	Header.VSplitRight(88.0f, &Header, &Close);
	const char *pTitle = pMap ? pMap->m_Name.c_str() : (!s_Leaderboard.m_MapName.empty() ? s_Leaderboard.m_MapName.c_str() : "Карта EGO");
	s_pUi->DoLabel(&Header, pTitle, 18.0f, TEXTALIGN_ML, {.m_MaxWidth = Header.w});
	if(s_pGameClient->m_Menus.DoButton_Menu(&s_DetailCloseButton, "Закрыть", 0, &Close))
		return CUi::POPUP_CLOSE_CURRENT;

	View.HSplitTop(7.0f, nullptr, &View);
	CUIRect Meta;
	View.HSplitTop(83.0f, &Meta, &View);
	Meta.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 8.0f);
	Meta.Margin(8.0f, &Meta);

	char aLine[512];
	CUIRect Line;
	Meta.HSplitTop(20.0f, &Line, &Meta);
	const char *pDifficulty = pMap && !pMap->m_Difficulty.empty() ? pMap->m_Difficulty.c_str() : s_Leaderboard.m_MapDifficulty.c_str();
	const int Points = pMap ? pMap->m_Points : s_Leaderboard.m_MapPoints;
	const int Stars = pMap ? pMap->m_Stars : 0;
	const char *pLength = pMap && !pMap->m_LengthLabel.empty() ? pMap->m_LengthLabel.c_str() : "—";
	str_format(aLine, sizeof(aLine), "Сложность: %s   •   %d очк.   •   %d★   •   длина: %s", pDifficulty && pDifficulty[0] ? pDifficulty : "—", Points, Stars, pLength);
	s_pUi->DoLabel(&Line, aLine, 11.0f, TEXTALIGN_ML, {.m_MaxWidth = Line.w});

	Meta.HSplitTop(20.0f, &Line, &Meta);
	const char *pAuthor = pMap && !pMap->m_Author.empty() ? pMap->m_Author.c_str() : s_Leaderboard.m_MapCreator.c_str();
	str_format(aLine, sizeof(aLine), "Автор: %s", pAuthor && pAuthor[0] ? pAuthor : "—");
	s_pUi->DoLabel(&Line, aLine, 11.0f, TEXTALIGN_ML, {.m_MaxWidth = Line.w});

	Meta.HSplitTop(20.0f, &Line, &Meta);
	char aAverage[64];
	FormatDurationMs(aAverage, sizeof(aAverage), pMap ? pMap->m_AverageTimeMs : -1);
	const char *pRelease = pMap && !pMap->m_ReleasedAt.empty() ? pMap->m_ReleasedAt.c_str() : s_Leaderboard.m_MapReleasedAt.c_str();
	str_format(aLine, sizeof(aLine), "Релиз: %s   •   финишей: %d   •   среднее: %s", pRelease && pRelease[0] ? pRelease : "—", pMap ? pMap->m_TotalFinishes : s_Leaderboard.m_TotalUsers, aAverage);
	s_pUi->DoLabel(&Line, aLine, 11.0f, TEXTALIGN_ML, {.m_MaxWidth = Line.w});

	View.HSplitTop(8.0f, nullptr, &View);
	CUIRect LeaderTitle;
	View.HSplitTop(23.0f, &LeaderTitle, &View);
	if(s_Leaderboard.m_Ready)
		str_format(aLine, sizeof(aLine), "Таблица лидеров • %d финишеров", s_Leaderboard.m_TotalUsers);
	else
		str_copy(aLine, "Таблица лидеров");
	s_pUi->DoLabel(&LeaderTitle, aLine, 14.0f, TEXTALIGN_ML);

	if(s_Leaderboard.m_Failed)
	{
		CUIRect Error, Retry;
		View.HSplitTop(32.0f, &Error, &View);
		Error.VSplitRight(110.0f, &Error, &Retry);
		s_pUi->DoLabel(&Error, "Не удалось загрузить статистику.", 11.0f, TEXTALIGN_ML);
		if(s_pGameClient->m_Menus.DoButton_Menu(&s_RetryLeaderboardButton, "Повторить", 0, &Retry))
		{
			s_Leaderboard.m_Failed = false;
			StartLeaderboardRequest(s_pHttp, s_SelectedMapId);
		}
		return CUi::POPUP_KEEP_OPEN;
	}
	if(!s_Leaderboard.m_Ready)
	{
		CUIRect Loading;
		View.HSplitTop(30.0f, &Loading, &View);
		s_pUi->DoLabel(&Loading, "Загрузка статистики с eternal-gores.com…", 11.0f, TEXTALIGN_ML);
		return CUi::POPUP_KEEP_OPEN;
	}

	CUIRect TableHeader;
	View.HSplitTop(22.0f, &TableHeader, &View);
	TableHeader.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.07f), IGraphics::CORNER_ALL, 5.0f);
	CUIRect RankCol, NickCol, TimeCol, ClanCol;
	TableHeader.VSplitLeft(42.0f, &RankCol, &TableHeader);
	TableHeader.VSplitLeft(std::max(150.0f, TableHeader.w * 0.42f), &NickCol, &TableHeader);
	TableHeader.VSplitLeft(105.0f, &TimeCol, &ClanCol);
	s_pUi->DoLabel(&RankCol, "#", 10.0f, TEXTALIGN_MC);
	s_pUi->DoLabel(&NickCol, "Игрок", 10.0f, TEXTALIGN_ML);
	s_pUi->DoLabel(&TimeCol, "Время", 10.0f, TEXTALIGN_MC);
	s_pUi->DoLabel(&ClanCol, "Клан", 10.0f, TEXTALIGN_ML);

	const int MaxRows = std::max(1, std::min(14, (int)(View.h / 23.0f)));
	for(int i = 0; i < MaxRows && i < (int)s_Leaderboard.m_vRanks.size(); ++i)
	{
		const SRank &Rank = s_Leaderboard.m_vRanks[i];
		CUIRect Row;
		View.HSplitTop(23.0f, &Row, &View);
		if(i % 2 == 1)
			Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.025f), IGraphics::CORNER_ALL, 3.0f);
		CUIRect RankRect, NickRect, TimeRect, ClanRect;
		Row.VSplitLeft(42.0f, &RankRect, &Row);
		Row.VSplitLeft(std::max(150.0f, Row.w * 0.42f), &NickRect, &Row);
		Row.VSplitLeft(105.0f, &TimeRect, &ClanRect);
		char aRank[24], aTime[64];
		str_format(aRank, sizeof(aRank), "%d", Rank.m_Rank);
		FormatDurationSeconds(aTime, sizeof(aTime), Rank.m_TimeSeconds);
		s_pUi->DoLabel(&RankRect, aRank, 10.5f, TEXTALIGN_MC);
		s_pUi->DoLabel(&NickRect, Rank.m_Nick.c_str(), 10.5f, TEXTALIGN_ML, {.m_MaxWidth = NickRect.w, .m_EllipsisAtEnd = true});
		s_pUi->DoLabel(&TimeRect, aTime, 10.5f, TEXTALIGN_MC);
		s_pUi->DoLabel(&ClanRect, Rank.m_Clan.empty() ? "—" : Rank.m_Clan.c_str(), 10.5f, TEXTALIGN_ML, {.m_MaxWidth = ClanRect.w, .m_EllipsisAtEnd = true});
	}
	return CUi::POPUP_KEEP_OPEN;
}

inline void OpenDetails(int MapId)
{
	if(!s_pUi || !s_pGameClient || !s_pHttp || MapId < 0)
		return;
	StartLeaderboardRequest(s_pHttp, MapId);
	const CUIRect Screen = *s_pUi->Screen();
	const float Width = std::min(820.0f, std::max(420.0f, Screen.w - 48.0f));
	const float Height = std::min(560.0f, std::max(320.0f, Screen.h - 48.0f));
	SPopupMenuProperties Props;
	Props.m_BackgroundColor = ColorRGBA(0.035f, 0.040f, 0.052f, 0.99f);
	Props.m_BorderColor = ColorRGBA(0.30f, 0.42f, 0.60f, 0.78f);
	s_pUi->DoPopupMenu(&s_DetailsPopupId, Screen.x + (Screen.w - Width) * 0.5f, Screen.y + (Screen.h - Height) * 0.5f, Width, Height, nullptr, RenderDetailsPopup, Props);
}

inline void RenderRightAction(const void *pItemId, CUIRect *pItemRect)
{
	if(!pItemRect || !s_pUi || !s_pGameClient || !g_Config.m_BkwEgoMapDetails)
		return;
	int OptionIndex = -1;
	int MapId = -1;
	if(!ResolveVoteMap(pItemId, OptionIndex, MapId))
		return;
	if(OptionIndex < 0)
		return;
	if((int)s_vDetailsButtons.size() <= OptionIndex)
		s_vDetailsButtons.resize(std::max(OptionIndex + 1, s_pGameClient->m_Voting.NumOptions()));

	CUIRect Action;
	const float ButtonWidth = std::min(92.0f, std::max(70.0f, pItemRect->w * 0.22f));
	pItemRect->VSplitRight(ButtonWidth, pItemRect, &Action);
	pItemRect->VSplitRight(4.0f, pItemRect, nullptr);
	Action.HMargin(1.0f, &Action);
	if(s_pGameClient->m_Menus.DoButton_Menu(&s_vDetailsButtons[OptionIndex], "Подробнее", s_SelectedMapId == MapId && s_pUi->IsPopupOpen(&s_DetailsPopupId), &Action, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 4.0f, -0.15f))
		OpenDetails(MapId);
}

inline void UpdateStandardVoteIntegration(CUi *pUi, CGameClient *pGameClient, IClient *pClient, IHttp *pHttp)
{
	s_pUi = pUi;
	s_pGameClient = pGameClient;
	s_pHttp = pHttp;
	const bool Enabled = pUi && pGameClient && pClient && pHttp &&
		g_Config.m_BkwEgoMapDetails &&
		pClient->State() == IClient::STATE_ONLINE &&
		IsEgoServerName(pClient->ServerInfo().m_aName) &&
		pGameClient->m_Menus.IsActive() &&
		pGameClient->m_Menus.GamePage() == CMenus::PAGE_CALLVOTE;

	if(Enabled)
	{
		UpdateCatalog(pHttp);
		CListBox::SetRightActionCallback(RenderRightAction);
		return;
	}

	if(CListBox::RightActionCallback() == RenderRightAction)
		CListBox::SetRightActionCallback(nullptr);
	if(pUi && pUi->IsPopupOpen(&s_DetailsPopupId))
		pUi->ClosePopupMenu(&s_DetailsPopupId);
	s_SelectedMapId = -1;
	s_pLeaderboardRequest.reset();
	s_Leaderboard = SLeaderboard();
}
} // namespace BkwEgoVote

#endif
