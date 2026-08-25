#ifndef GAME_CLIENT_COMPONENTS_BKW_EGO_VOTE_OVERLAY_INC
#define GAME_CLIENT_COMPONENTS_BKW_EGO_VOTE_OVERLAY_INC

#include <base/str.h>

#include <engine/http.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/shared/json.h>

#include <game/client/components/menus.h>
#include <game/client/components/voting.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

#include <algorithm>
#include <cmath>
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

struct SVoteRow
{
	int m_OptionIndex = -1;
	std::string m_Description;
	bool m_Header = false;
	bool m_InMapsSection = false;
	int m_MapId = -1;
};

inline bool s_Active = false;
inline bool s_SuppressReopen = false;
inline int s_Page = 0;
inline int s_SelectedMapId = -1;

inline std::vector<SMap> s_vMaps;
inline std::shared_ptr<IHttpRequest> s_pCatalogRequest;
inline int s_CatalogOffset = 0;
inline int s_CatalogTotal = -1;
inline bool s_CatalogReady = false;
inline bool s_CatalogFailed = false;

inline SLeaderboard s_Leaderboard;
inline std::shared_ptr<IHttpRequest> s_pLeaderboardRequest;

inline std::vector<CButtonContainer> s_vVoteButtons;
inline std::vector<CButtonContainer> s_vDetailsButtons;
inline CButtonContainer s_BackToDefaultButton;
inline CButtonContainer s_CloseButton;
inline CButtonContainer s_PrevButton;
inline CButtonContainer s_NextButton;
inline CButtonContainer s_DetailCloseButton;
inline CButtonContainer s_RetryCatalogButton;
inline CButtonContainer s_RetryLeaderboardButton;

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
	return pOpen && pColon && pClose;
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
	{
		if(str_comp(Map.m_Name.c_str(), Name.c_str()) == 0)
			return &Map;
	}
	for(const auto &Map : s_vMaps)
	{
		if(str_comp_nocase(Map.m_Name.c_str(), Name.c_str()) == 0)
			return &Map;
	}
	return nullptr;
}

inline void ResetLeaderboard()
{
	s_pLeaderboardRequest.reset();
	s_Leaderboard = SLeaderboard();
	s_SelectedMapId = -1;
}

inline void ResetOverlay()
{
	s_Active = false;
	s_Page = 0;
	ResetLeaderboard();
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
	str_format(aUrl, sizeof(aUrl), "https://eternal-gores.com/api/maps?limit=500&offset=%d", s_CatalogOffset);
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

inline std::vector<SVoteRow> BuildVoteRows(const CVoting &Voting)
{
	std::vector<SVoteRow> vRows;
	vRows.reserve(Voting.NumOptions());
	bool InMapsSection = false;
	int OptionIndex = 0;
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++OptionIndex)
	{
		SVoteRow Row;
		Row.m_OptionIndex = OptionIndex;
		Row.m_Description = Trim(pOption->m_aDescription);
		if(LooksLikeGroupHeader(pOption->m_aDescription))
		{
			Row.m_Header = true;
			InMapsSection = IsMapsHeader(pOption->m_aDescription);
		}
		Row.m_InMapsSection = InMapsSection;
		if(!Row.m_Header && Row.m_InMapsSection)
		{
			if(const SMap *pMap = FindMapByName(Row.m_Description))
				Row.m_MapId = pMap->m_Id;
		}
		vRows.push_back(std::move(Row));
	}
	return vRows;
}

inline void RenderApiStatus(CUi *pUi, CUIRect Rect)
{
	char aStatus[160];
	if(s_CatalogFailed)
		str_copy(aStatus, "EGO API: не удалось загрузить каталог карт");
	else if(s_CatalogReady)
		str_format(aStatus, sizeof(aStatus), "EGO API: %d карт", (int)s_vMaps.size());
	else if(s_CatalogTotal > 0)
		str_format(aStatus, sizeof(aStatus), "EGO API: карты %d / %d", (int)s_vMaps.size(), s_CatalogTotal);
	else
		str_copy(aStatus, "EGO API: загрузка карт…");
	pUi->DoLabel(&Rect, aStatus, 10.0f, TEXTALIGN_MR);
}

inline void RenderDetails(CUi *pUi, CGameClient *pGameClient, IHttp *pHttp, const CUIRect &Screen)
{
	if(s_SelectedMapId < 0)
		return;

	UpdateLeaderboard();
	const SMap *pMap = FindMapById(s_SelectedMapId);

	Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.52f), IGraphics::CORNER_NONE, 0.0f);
	CUIRect Panel;
	Panel.w = std::min(Screen.w - 36.0f, 920.0f);
	Panel.h = std::min(Screen.h - 36.0f, 650.0f);
	Panel.x = Screen.x + (Screen.w - Panel.w) * 0.5f;
	Panel.y = Screen.y + (Screen.h - Panel.h) * 0.5f;
	Panel.Draw(ColorRGBA(0.035f, 0.040f, 0.052f, 0.98f), IGraphics::CORNER_ALL, 16.0f);
	Panel.Margin(16.0f, &Panel);

	CUIRect Header, Close;
	Panel.HSplitTop(36.0f, &Header, &Panel);
	Header.VSplitRight(110.0f, &Header, &Close);
	const char *pTitle = pMap ? pMap->m_Name.c_str() : (!s_Leaderboard.m_MapName.empty() ? s_Leaderboard.m_MapName.c_str() : "Карта EGO");
	pUi->DoLabel(&Header, pTitle, 22.0f, TEXTALIGN_ML);
	if(pGameClient->m_Menus.DoButton_Menu(&s_DetailCloseButton, "Назад", 0, &Close))
		ResetLeaderboard();

	Panel.HSplitTop(8.0f, nullptr, &Panel);
	CUIRect Meta;
	Panel.HSplitTop(96.0f, &Meta, &Panel);
	Meta.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 10.0f);
	Meta.Margin(10.0f, &Meta);

	char aLine[512];
	CUIRect Line;
	Meta.HSplitTop(22.0f, &Line, &Meta);
	const char *pDifficulty = pMap && !pMap->m_Difficulty.empty() ? pMap->m_Difficulty.c_str() : s_Leaderboard.m_MapDifficulty.c_str();
	const int Points = pMap ? pMap->m_Points : s_Leaderboard.m_MapPoints;
	const int Stars = pMap ? pMap->m_Stars : 0;
	const char *pLength = pMap && !pMap->m_LengthLabel.empty() ? pMap->m_LengthLabel.c_str() : "—";
	str_format(aLine, sizeof(aLine), "Сложность: %s   •   %d очк.   •   %d★   •   длина: %s", pDifficulty && pDifficulty[0] ? pDifficulty : "—", Points, Stars, pLength);
	pUi->DoLabel(&Line, aLine, 12.0f, TEXTALIGN_ML);

	Meta.HSplitTop(22.0f, &Line, &Meta);
	const char *pAuthor = pMap && !pMap->m_Author.empty() ? pMap->m_Author.c_str() : s_Leaderboard.m_MapCreator.c_str();
	str_format(aLine, sizeof(aLine), "Автор: %s", pAuthor && pAuthor[0] ? pAuthor : "—");
	pUi->DoLabel(&Line, aLine, 12.0f, TEXTALIGN_ML);

	Meta.HSplitTop(22.0f, &Line, &Meta);
	char aAverage[64];
	FormatDurationMs(aAverage, sizeof(aAverage), pMap ? pMap->m_AverageTimeMs : -1);
	const char *pRelease = pMap && !pMap->m_ReleasedAt.empty() ? pMap->m_ReleasedAt.c_str() : s_Leaderboard.m_MapReleasedAt.c_str();
	str_format(aLine, sizeof(aLine), "Релиз: %s   •   финишей: %d   •   среднее время: %s", pRelease && pRelease[0] ? pRelease : "—", pMap ? pMap->m_TotalFinishes : s_Leaderboard.m_TotalUsers, aAverage);
	pUi->DoLabel(&Line, aLine, 12.0f, TEXTALIGN_ML);

	Panel.HSplitTop(12.0f, nullptr, &Panel);
	CUIRect LeaderTitle;
	Panel.HSplitTop(26.0f, &LeaderTitle, &Panel);
	if(s_Leaderboard.m_Ready)
		str_format(aLine, sizeof(aLine), "Таблица лидеров • %d игроков с финишем", s_Leaderboard.m_TotalUsers);
	else
		str_copy(aLine, "Таблица лидеров");
	pUi->DoLabel(&LeaderTitle, aLine, 16.0f, TEXTALIGN_ML);

	Panel.HSplitTop(5.0f, nullptr, &Panel);
	if(s_Leaderboard.m_Failed)
	{
		CUIRect Error, Retry;
		Panel.HSplitTop(34.0f, &Error, &Panel);
		Error.VSplitRight(140.0f, &Error, &Retry);
		pUi->DoLabel(&Error, "Не удалось получить leaderboard для этой карты.", 12.0f, TEXTALIGN_ML);
		if(pGameClient->m_Menus.DoButton_Menu(&s_RetryLeaderboardButton, "Повторить", 0, &Retry))
		{
			s_Leaderboard.m_Failed = false;
			StartLeaderboardRequest(pHttp, s_SelectedMapId);
		}
		return;
	}
	if(!s_Leaderboard.m_Ready)
	{
		CUIRect Loading;
		Panel.HSplitTop(34.0f, &Loading, &Panel);
		pUi->DoLabel(&Loading, "Загрузка статистики с eternal-gores.com…", 12.0f, TEXTALIGN_ML);
		return;
	}

	CUIRect TableHeader;
	Panel.HSplitTop(24.0f, &TableHeader, &Panel);
	TableHeader.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.07f), IGraphics::CORNER_ALL, 6.0f);
	CUIRect RankCol, NickCol, TimeCol, ClanCol;
	TableHeader.VSplitLeft(52.0f, &RankCol, &TableHeader);
	TableHeader.VSplitLeft(std::max(160.0f, TableHeader.w * 0.42f), &NickCol, &TableHeader);
	TableHeader.VSplitLeft(120.0f, &TimeCol, &ClanCol);
	pUi->DoLabel(&RankCol, "#", 11.0f, TEXTALIGN_MC);
	pUi->DoLabel(&NickCol, "Игрок", 11.0f, TEXTALIGN_ML);
	pUi->DoLabel(&TimeCol, "Время", 11.0f, TEXTALIGN_MC);
	pUi->DoLabel(&ClanCol, "Клан", 11.0f, TEXTALIGN_ML);

	const int MaxRows = std::max(1, std::min(12, (int)(Panel.h / 25.0f)));
	for(int i = 0; i < MaxRows && i < (int)s_Leaderboard.m_vRanks.size(); ++i)
	{
		const SRank &Rank = s_Leaderboard.m_vRanks[i];
		CUIRect Row;
		Panel.HSplitTop(25.0f, &Row, &Panel);
		if(i % 2 == 1)
			Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.025f), IGraphics::CORNER_ALL, 4.0f);
		CUIRect RankRect, NickRect, TimeRect, ClanRect;
		Row.VSplitLeft(52.0f, &RankRect, &Row);
		Row.VSplitLeft(std::max(160.0f, Row.w * 0.42f), &NickRect, &Row);
		Row.VSplitLeft(120.0f, &TimeRect, &ClanRect);
		char aRank[24], aTime[64];
		str_format(aRank, sizeof(aRank), "%d", Rank.m_Rank);
		FormatDurationSeconds(aTime, sizeof(aTime), Rank.m_TimeSeconds);
		pUi->DoLabel(&RankRect, aRank, 11.5f, TEXTALIGN_MC);
		pUi->DoLabel(&NickRect, Rank.m_Nick.c_str(), 11.5f, TEXTALIGN_ML, {.m_MaxWidth = NickRect.w});
		pUi->DoLabel(&TimeRect, aTime, 11.5f, TEXTALIGN_MC);
		pUi->DoLabel(&ClanRect, Rank.m_Clan.empty() ? "—" : Rank.m_Clan.c_str(), 11.5f, TEXTALIGN_ML, {.m_MaxWidth = ClanRect.w});
	}
}

inline bool RenderOverlay(CUi *pUi, CGameClient *pGameClient, IInput *pInput, IHttp *pHttp)
{
	UpdateCatalog(pHttp);
	UpdateLeaderboard();

	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	Screen.Draw(ColorRGBA(0.015f, 0.018f, 0.024f, 0.985f), IGraphics::CORNER_NONE, 0.0f);

	CUIRect Panel = Screen;
	Panel.Margin(24.0f, &Panel);
	Panel.Draw(ColorRGBA(0.035f, 0.040f, 0.052f, 0.98f), IGraphics::CORNER_ALL, 14.0f);
	Panel.Margin(14.0f, &Panel);

	CUIRect Header;
	Panel.HSplitTop(38.0f, &Header, &Panel);
	CUIRect Title, Status, DefaultButton, CloseButton;
	Header.VSplitRight(112.0f, &Header, &CloseButton);
	Header.VSplitRight(150.0f, &Header, &DefaultButton);
	Header.VSplitMid(&Title, &Status, 8.0f);
	pUi->DoLabel(&Title, "EGO • Голосование", 21.0f, TEXTALIGN_ML);
	RenderApiStatus(pUi, Status);
	if(pGameClient->m_Menus.DoButton_Menu(&s_BackToDefaultButton, "Обычное меню", 0, &DefaultButton))
	{
		s_Active = false;
		s_SuppressReopen = true;
		ResetLeaderboard();
		pGameClient->m_Menus.SetActive(true);
		pUi->SetActiveItem(nullptr);
		return false;
	}
	if(pGameClient->m_Menus.DoButton_Menu(&s_CloseButton, "Закрыть", 0, &CloseButton) || (pInput && pInput->KeyPress(KEY_ESCAPE)))
	{
		ResetOverlay();
		return false;
	}

	Panel.HSplitTop(8.0f, nullptr, &Panel);
	if(s_CatalogFailed)
	{
		CUIRect Error, Retry;
		Panel.HSplitTop(34.0f, &Error, &Panel);
		Error.VSplitRight(150.0f, &Error, &Retry);
		pUi->DoLabel(&Error, "EGO API недоступен. Голосование работает, но статистика карт временно скрыта.", 11.5f, TEXTALIGN_ML);
		if(pGameClient->m_Menus.DoButton_Menu(&s_RetryCatalogButton, "Повторить API", 0, &Retry))
			RetryCatalog();
		Panel.HSplitTop(6.0f, nullptr, &Panel);
	}

	std::vector<SVoteRow> vRows = BuildVoteRows(pGameClient->m_Voting);
	if(s_vVoteButtons.size() != vRows.size())
	{
		s_vVoteButtons.clear();
		s_vDetailsButtons.clear();
		s_vVoteButtons.resize(vRows.size());
		s_vDetailsButtons.resize(vRows.size());
	}

	CUIRect Footer;
	Panel.HSplitBottom(34.0f, &Panel, &Footer);
	const float RowHeight = 29.0f;
	const int RowsPerPage = std::max(5, (int)(Panel.h / RowHeight));
	const int PageCount = std::max(1, ((int)vRows.size() + RowsPerPage - 1) / RowsPerPage);
	s_Page = std::clamp(s_Page, 0, PageCount - 1);
	const int First = s_Page * RowsPerPage;
	const int Last = std::min((int)vRows.size(), First + RowsPerPage);

	for(int i = First; i < Last; ++i)
	{
		const SVoteRow &VoteRow = vRows[i];
		CUIRect Row;
		Panel.HSplitTop(RowHeight, &Row, &Panel);
		Row.VMargin(2.0f, &Row);
		Row.HMargin(1.5f, &Row);

		if(VoteRow.m_Header)
		{
			Row.Draw(VoteRow.m_InMapsSection ? ColorRGBA(0.20f, 0.34f, 0.55f, 0.30f) : ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 6.0f);
			Row.Margin(8.0f, &Row);
			pUi->DoLabel(&Row, VoteRow.m_Description.c_str(), 12.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
			continue;
		}

		CUIRect VoteButton = Row;
		CUIRect DetailsButton;
		if(VoteRow.m_InMapsSection && VoteRow.m_MapId >= 0)
		{
			Row.VSplitRight(108.0f, &VoteButton, &DetailsButton);
			VoteButton.VSplitRight(5.0f, &VoteButton, nullptr);
		}
		if(pGameClient->m_Menus.DoButton_Menu(&s_vVoteButtons[i], VoteRow.m_Description.c_str(), 0, &VoteButton))
		{
			pGameClient->m_Voting.CallvoteOption(VoteRow.m_OptionIndex, "");
			ResetOverlay();
			return false;
		}
		if(VoteRow.m_InMapsSection && VoteRow.m_MapId >= 0)
		{
			if(pGameClient->m_Menus.DoButton_Menu(&s_vDetailsButtons[i], "Подробнее", s_SelectedMapId == VoteRow.m_MapId, &DetailsButton))
				StartLeaderboardRequest(pHttp, VoteRow.m_MapId);
		}
	}

	char aPage[96];
	str_format(aPage, sizeof(aPage), "Страница %d / %d", s_Page + 1, PageCount);
	CUIRect Prev, PageLabel, Next;
	Footer.VSplitLeft(130.0f, &Prev, &Footer);
	Footer.VSplitRight(130.0f, &PageLabel, &Next);
	if(pGameClient->m_Menus.DoButton_Menu(&s_PrevButton, "← Назад", 0, &Prev) && s_Page > 0)
		--s_Page;
	if(pGameClient->m_Menus.DoButton_Menu(&s_NextButton, "Вперёд →", 0, &Next) && s_Page + 1 < PageCount)
		++s_Page;
	pUi->DoLabel(&PageLabel, aPage, 11.5f, TEXTALIGN_MC);

	if(s_SelectedMapId >= 0)
		RenderDetails(pUi, pGameClient, pHttp, Screen);
	return true;
}

inline bool UpdateAndRender(CUi *pUi, CGameClient *pGameClient, IClient *pClient, IInput *pInput, IHttp *pHttp)
{
	if(!pUi || !pGameClient || !pClient)
		return false;

	const bool Online = pClient->State() == IClient::STATE_ONLINE;
	const bool EgoServer = Online && IsEgoServerName(pClient->ServerInfo().m_aName);

	if(s_SuppressReopen && !pGameClient->m_Menus.IsActive())
		s_SuppressReopen = false;
	if(pGameClient->m_Menus.IsActive() && pGameClient->m_Menus.GamePage() != CMenus::PAGE_CALLVOTE)
		s_SuppressReopen = false;

	if(!EgoServer)
	{
		if(s_Active)
			ResetOverlay();
		return false;
	}

	UpdateCatalog(pHttp);

	if(!s_Active && !s_SuppressReopen && pGameClient->m_Menus.IsActive() && pGameClient->m_Menus.GamePage() == CMenus::PAGE_CALLVOTE)
	{
		s_Active = true;
		s_Page = 0;
		ResetLeaderboard();
		pGameClient->m_Menus.SetActive(false);
		pUi->SetActiveItem(nullptr);
	}

	if(!s_Active)
		return false;
	return RenderOverlay(pUi, pGameClient, pInput, pHttp);
}
} // namespace BkwEgoVote

#endif