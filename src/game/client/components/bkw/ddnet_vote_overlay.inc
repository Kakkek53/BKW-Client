#ifndef GAME_CLIENT_COMPONENTS_BKW_DDNET_VOTE_OVERLAY_INC
#define GAME_CLIENT_COMPONENTS_BKW_DDNET_VOTE_OVERLAY_INC

#include <base/math.h>
#include <base/str.h>

#include <engine/http.h>
#include <engine/shared/json.h>

#include <game/client/components/bkw/ddnet_vote_state.inc>
#include <game/client/components/bkw/tf_svg_icons.inc>
#include <game/client/components/voting.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

inline bool BkwDdnetJsonString(const json_value *pObject, const char *pKey, std::string &Out)
{
	const json_value *pValue = json_object_get(pObject, pKey);
	if(!pValue || pValue->type != json_string)
		return false;
	Out = pValue->u.string.ptr;
	return true;
}

inline int BkwDdnetJsonInt(const json_value *pObject, const char *pKey, int Default = 0)
{
	const json_value *pValue = json_object_get(pObject, pKey);
	if(!pValue)
		return Default;
	if(pValue->type == json_integer)
		return (int)pValue->u.integer;
	if(pValue->type == json_double)
		return (int)pValue->u.dbl;
	return Default;
}

inline double BkwDdnetJsonDouble(const json_value *pObject, const char *pKey, double Default = 0.0)
{
	const json_value *pValue = json_object_get(pObject, pKey);
	if(!pValue)
		return Default;
	if(pValue->type == json_double)
		return pValue->u.dbl;
	if(pValue->type == json_integer)
		return (double)pValue->u.integer;
	return Default;
}

inline void BkwDdnetResetView()
{
	BkwDdnetVoteState::SelectedTab = 0;
	BkwDdnetVoteState::ScrollRow = 0;
	BkwDdnetVoteState::DetailOpen = false;
	BkwDdnetVoteState::SelectedMap = -1;
	BkwDdnetVoteState::SearchInput.Clear();
	if(BkwDdnetVoteState::SearchInput.IsActive())
		BkwDdnetVoteState::SearchInput.Deactivate();
	BkwDdnetVoteState::ReasonInput.Clear();
	if(BkwDdnetVoteState::ReasonInput.IsActive())
		BkwDdnetVoteState::ReasonInput.Deactivate();
	BkwDdnetVoteState::pStatsRequest.reset();
	BkwDdnetVoteState::StatsRequestedMap.clear();
	BkwDdnetVoteState::Stats = BkwDdnetVoteState::SStats();
}

inline void BkwDdnetStartCatalogRequest()
{
	if(BkwDdnetVoteState::CatalogRequested || !BkwDdnetVoteState::pHttp)
		return;
	BkwDdnetVoteState::CatalogRequested = true;
	BkwDdnetVoteState::CatalogFailed = false;
	BkwDdnetVoteState::pCatalogRequest = HttpGet("https://ddnet.org/releases/maps.json");
	BkwDdnetVoteState::pCatalogRequest->MaxResponseSize(32 * 1024 * 1024);
	BkwDdnetVoteState::pCatalogRequest->Timeout(CTimeout{5000, 20000, 1024, 8});
	BkwDdnetVoteState::pCatalogRequest->LogProgress(HTTPLOG::FAILURE);
	BkwDdnetVoteState::pHttp->Run(BkwDdnetVoteState::pCatalogRequest);
}

inline void BkwDdnetUpdateCatalog()
{
	BkwDdnetStartCatalogRequest();
	if(BkwDdnetVoteState::CatalogReady || BkwDdnetVoteState::CatalogFailed || !BkwDdnetVoteState::pCatalogRequest || !BkwDdnetVoteState::pCatalogRequest->Done())
		return;

	if(BkwDdnetVoteState::pCatalogRequest->State() != EHttpState::DONE)
	{
		BkwDdnetVoteState::CatalogFailed = true;
		BkwDdnetVoteState::pCatalogRequest.reset();
		return;
	}

	json_value *pRoot = BkwDdnetVoteState::pCatalogRequest->ResultJson();
	if(!pRoot || pRoot->type != json_array)
	{
		if(pRoot)
			json_value_free(pRoot);
		BkwDdnetVoteState::CatalogFailed = true;
		BkwDdnetVoteState::pCatalogRequest.reset();
		return;
	}

	std::vector<BkwDdnetVoteState::SMap> vMaps;
	vMaps.reserve(pRoot->u.array.length);
	for(unsigned i = 0; i < pRoot->u.array.length; ++i)
	{
		const json_value *pEntry = pRoot->u.array.values[i];
		if(!pEntry || pEntry->type != json_object)
			continue;

		BkwDdnetVoteState::SMap Map;
		if(!BkwDdnetJsonString(pEntry, "name", Map.m_Name) || Map.m_Name.empty())
			continue;
		BkwDdnetJsonString(pEntry, "thumbnail", Map.m_Thumbnail);
		BkwDdnetJsonString(pEntry, "type", Map.m_Type);
		BkwDdnetJsonString(pEntry, "mapper", Map.m_Mapper);
		BkwDdnetJsonString(pEntry, "release", Map.m_Release);
		Map.m_Points = BkwDdnetJsonInt(pEntry, "points");
		Map.m_Difficulty = std::clamp(BkwDdnetJsonInt(pEntry, "difficulty"), 0, 5);
		vMaps.push_back(std::move(Map));
	}
	json_value_free(pRoot);

	BkwDdnetVoteState::vMaps = std::move(vMaps);
	BkwDdnetVoteState::vThumbnails.clear();
	BkwDdnetVoteState::vThumbnails.resize(BkwDdnetVoteState::vMaps.size());
	BkwDdnetVoteState::CatalogReady = !BkwDdnetVoteState::vMaps.empty();
	BkwDdnetVoteState::CatalogFailed = !BkwDdnetVoteState::CatalogReady;
	BkwDdnetVoteState::pCatalogRequest.reset();
}

inline void BkwDdnetUpdateThumbnail(CUi *pUi, int MapIndex)
{
	if(!pUi || !BkwDdnetVoteState::pHttp || MapIndex < 0 || MapIndex >= (int)BkwDdnetVoteState::vMaps.size())
		return;
	auto &Thumb = BkwDdnetVoteState::vThumbnails[MapIndex];
	const auto &Map = BkwDdnetVoteState::vMaps[MapIndex];
	if(Thumb.m_Texture.IsValid() || Thumb.m_Failed || Map.m_Thumbnail.empty())
		return;

	if(!Thumb.m_pRequest)
	{
		Thumb.m_pRequest = HttpGet(Map.m_Thumbnail.c_str());
		Thumb.m_pRequest->MaxResponseSize(8 * 1024 * 1024);
		Thumb.m_pRequest->Timeout(CTimeout{5000, 15000, 1024, 8});
		Thumb.m_pRequest->LogProgress(HTTPLOG::FAILURE);
		BkwDdnetVoteState::pHttp->Run(Thumb.m_pRequest);
		return;
	}

	if(!Thumb.m_pRequest->Done())
		return;
	if(Thumb.m_pRequest->State() != EHttpState::DONE)
	{
		Thumb.m_Failed = true;
		Thumb.m_pRequest.reset();
		return;
	}

	unsigned char *pData = nullptr;
	size_t DataSize = 0;
	Thumb.m_pRequest->Result(&pData, &DataSize);
	CImageInfo Image;
	if(pData && DataSize > 0 && pUi->Graphics()->LoadPng(Image, pData, DataSize, Map.m_Name.c_str()))
		Thumb.m_Texture = pUi->Graphics()->LoadTextureRawMove(Image, 0, Map.m_Name.c_str());
	else
		Thumb.m_Failed = true;
	Thumb.m_pRequest.reset();
}

inline void BkwDdnetRenderTexture(CUi *pUi, IGraphics::CTextureHandle Texture, const CUIRect &Rect, float Alpha = 1.0f)
{
	if(!Texture.IsValid())
		return;
	pUi->Graphics()->TextureSet(Texture);
	pUi->Graphics()->QuadsBegin();
	pUi->Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
	pUi->Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
	IGraphics::CQuadItem Quad(Rect.x, Rect.y, Rect.w, Rect.h);
	pUi->Graphics()->QuadsDrawTL(&Quad, 1);
	pUi->Graphics()->QuadsEnd();
	pUi->Graphics()->TextureClear();
}

inline void BkwDdnetRenderTextureCover(CUi *pUi, IGraphics::CTextureHandle Texture, const CUIRect &Rect, float SourceAspect, float Alpha = 1.0f)
{
	if(!Texture.IsValid() || Rect.w <= 0.0f || Rect.h <= 0.0f || SourceAspect <= 0.0f)
		return;

	float U0 = 0.0f;
	float V0 = 0.0f;
	float U1 = 1.0f;
	float V1 = 1.0f;
	const float RectAspect = Rect.w / Rect.h;
	if(RectAspect > SourceAspect)
	{
		const float VisibleHeight = SourceAspect / RectAspect;
		V0 = (1.0f - VisibleHeight) * 0.5f;
		V1 = 1.0f - V0;
	}
	else if(RectAspect < SourceAspect)
	{
		const float VisibleWidth = RectAspect / SourceAspect;
		U0 = (1.0f - VisibleWidth) * 0.5f;
		U1 = 1.0f - U0;
	}

	pUi->Graphics()->TextureSet(Texture);
	pUi->Graphics()->QuadsBegin();
	pUi->Graphics()->QuadsSetSubset(U0, V0, U1, V1);
	pUi->Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
	IGraphics::CQuadItem Quad(Rect.x, Rect.y, Rect.w, Rect.h);
	pUi->Graphics()->QuadsDrawTL(&Quad, 1);
	pUi->Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
	pUi->Graphics()->QuadsEnd();
	pUi->Graphics()->TextureClear();
}

inline std::string BkwDdnetStars(int Difficulty)
{
	std::string Result;
	for(int i = 0; i < 5; ++i)
		Result += i < Difficulty ? "★" : "☆";
	return Result;
}

inline void BkwDdnetSendMapVote(CGameClient *pGameClient, const std::string &Name)
{
	if(!pGameClient || Name.empty())
		return;
	char aCommand[256];
	// DDNet accepts quoted map names for both single-word and multi-word maps.
	// Always quote so the command has one predictable form: /map "name".
	str_format(aCommand, sizeof(aCommand), "/map \"%s\"", Name.c_str());
	pGameClient->m_Chat.SendChat(0, aCommand);
}

inline void BkwDdnetStartStats(int MapIndex)
{
	if(!BkwDdnetVoteState::pHttp || MapIndex < 0 || MapIndex >= (int)BkwDdnetVoteState::vMaps.size())
		return;
	const auto &Map = BkwDdnetVoteState::vMaps[MapIndex];
	BkwDdnetVoteState::DetailOpen = true;
	BkwDdnetVoteState::SelectedMap = MapIndex;
	BkwDdnetVoteState::Stats = BkwDdnetVoteState::SStats();
	BkwDdnetVoteState::StatsRequestedMap = Map.m_Name;

	char aEscaped[192];
	EscapeUrl(aEscaped, Map.m_Name.c_str());
	char aUrl[256];
	str_format(aUrl, sizeof(aUrl), "https://ddnet.org/maps/?json=%s", aEscaped);
	BkwDdnetVoteState::pStatsRequest = HttpGet(aUrl);
	BkwDdnetVoteState::pStatsRequest->MaxResponseSize(8 * 1024 * 1024);
	BkwDdnetVoteState::pStatsRequest->Timeout(CTimeout{5000, 15000, 1024, 8});
	BkwDdnetVoteState::pStatsRequest->LogProgress(HTTPLOG::FAILURE);
	BkwDdnetVoteState::pHttp->Run(BkwDdnetVoteState::pStatsRequest);
}

inline void BkwDdnetUpdateStats()
{
	if(!BkwDdnetVoteState::pStatsRequest || BkwDdnetVoteState::Stats.m_Ready || BkwDdnetVoteState::Stats.m_Failed || !BkwDdnetVoteState::pStatsRequest->Done())
		return;
	if(BkwDdnetVoteState::pStatsRequest->State() != EHttpState::DONE)
	{
		BkwDdnetVoteState::Stats.m_Failed = true;
		BkwDdnetVoteState::pStatsRequest.reset();
		return;
	}

	json_value *pRoot = BkwDdnetVoteState::pStatsRequest->ResultJson();
	if(!pRoot || pRoot->type != json_object)
	{
		if(pRoot)
			json_value_free(pRoot);
		BkwDdnetVoteState::Stats.m_Failed = true;
		BkwDdnetVoteState::pStatsRequest.reset();
		return;
	}

	auto &Stats = BkwDdnetVoteState::Stats;
	BkwDdnetJsonString(pRoot, "name", Stats.m_Name);
	BkwDdnetJsonString(pRoot, "type", Stats.m_Type);
	BkwDdnetJsonString(pRoot, "mapper", Stats.m_Mapper);
	Stats.m_Points = BkwDdnetJsonInt(pRoot, "points");
	Stats.m_Difficulty = std::clamp(BkwDdnetJsonInt(pRoot, "difficulty"), 0, 5);
	Stats.m_AverageTime = BkwDdnetJsonDouble(pRoot, "average_time");
	Stats.m_Finishes = BkwDdnetJsonInt(pRoot, "finishes");
	Stats.m_Finishers = BkwDdnetJsonInt(pRoot, "finishers");
	Stats.m_BiggestTeam = BkwDdnetJsonInt(pRoot, "biggest_team");

	const json_value *pRanks = json_object_get(pRoot, "ranks");
	if(pRanks && pRanks->type == json_array)
	{
		const int Count = minimum(5, (int)pRanks->u.array.length);
		for(int i = 0; i < Count; ++i)
		{
			const json_value *pRank = pRanks->u.array.values[i];
			if(!pRank || pRank->type != json_object)
				continue;
			BkwDdnetVoteState::SRank Rank;
			Rank.m_Rank = BkwDdnetJsonInt(pRank, "rank", i + 1);
			BkwDdnetJsonString(pRank, "player", Rank.m_Player);
			BkwDdnetJsonString(pRank, "country", Rank.m_Country);
			Rank.m_Time = BkwDdnetJsonDouble(pRank, "time");
			Stats.m_vRanks.push_back(std::move(Rank));
		}
	}
	Stats.m_Ready = true;
	json_value_free(pRoot);
	BkwDdnetVoteState::pStatsRequest.reset();
}

inline bool BkwDdnetSmallButton(CUi *pUi, CButtonContainer *pButton, const char *pText, CUIRect Rect, bool Interactive, bool Accent = false, bool Negative = false, BkwTfSvg::EIcon Icon = BkwTfSvg::EIcon::COUNT)
{
	const bool Hovered = Interactive && pUi->MouseInside(&Rect);
	ColorRGBA Color = Accent ? ColorRGBA(0.20f, 0.45f, 0.82f, Hovered ? 0.88f : 0.68f) : ColorRGBA(1.0f, 1.0f, 1.0f, Hovered ? 0.16f : 0.08f);
	if(Negative)
		Color = ColorRGBA(0.62f, 0.14f, 0.12f, Hovered ? 0.70f : 0.48f);
	Rect.Draw(Color, IGraphics::CORNER_ALL, 6.0f);

	CUIRect Label = Rect;
	if(Icon != BkwTfSvg::EIcon::COUNT)
	{
		CUIRect IconRect;
		Label.VSplitLeft(23.0f, &IconRect, &Label);
		IconRect.Margin(5.0f, &IconRect);
		BkwTfSvg::Render(pUi, Icon, IconRect, ColorRGBA(1.0f, 1.0f, 1.0f, 0.90f));
		Label.VSplitLeft(1.0f, nullptr, &Label);
	}
	pUi->DoLabel(&Label, pText, 9.3f, TEXTALIGN_MC, {.m_MaxWidth = Label.w - 4.0f});
	return Interactive && pUi->DoButtonLogic(pButton, 0, &Rect, BUTTONFLAG_LEFT);
}

inline std::vector<int> BkwDdnetFilteredMaps()
{
	std::vector<int> vResult;
	const char *pSearch = BkwDdnetVoteState::SearchInput.GetString();
	vResult.reserve(BkwDdnetVoteState::vMaps.size());
	for(int i = 0; i < (int)BkwDdnetVoteState::vMaps.size(); ++i)
	{
		const auto &Map = BkwDdnetVoteState::vMaps[i];
		if(!pSearch[0] || str_utf8_find_nocase(Map.m_Name.c_str(), pSearch) || str_utf8_find_nocase(Map.m_Type.c_str(), pSearch) || str_utf8_find_nocase(Map.m_Mapper.c_str(), pSearch))
			vResult.push_back(i);
	}
	return vResult;
}

inline void BkwDdnetRenderMapCards(CUi *pUi, CGameClient *pGameClient, CUIRect Body, bool Interactive)
{
	if(!BkwDdnetVoteState::CatalogReady)
	{
		const char *pText = BkwDdnetVoteState::CatalogFailed ? "Не удалось загрузить каталог карт DDNet." : "Загружаю каталог карт DDNet…";
		pUi->DoLabel(&Body, pText, 12.0f, TEXTALIGN_MC);
		return;
	}

	CUIRect Search;
	Body.HSplitTop(30.0f, &Search, &Body);
	Search.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 7.0f);
	Search.Margin(5.0f, &Search);
	BkwDdnetVoteState::SearchInput.SetEmptyText("Поиск по названию, типу или автору…");
	if(Interactive)
		pUi->DoEditBox(&BkwDdnetVoteState::SearchInput, &Search, 10.5f);
	else
		pUi->DoLabel(&Search, BkwDdnetVoteState::SearchInput.IsEmpty() ? "Поиск по названию, типу или автору…" : BkwDdnetVoteState::SearchInput.GetString(), 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Search.w});
	Body.HSplitTop(8.0f, nullptr, &Body);

	const std::vector<int> vFiltered = BkwDdnetFilteredMaps();
	if(vFiltered.empty())
	{
		pUi->DoLabel(&Body, "Карты по этому запросу не найдены.", 11.5f, TEXTALIGN_MC);
		return;
	}

	constexpr int Columns = 3;
	constexpr int VisibleRows = 2;
	const int TotalRows = ((int)vFiltered.size() + Columns - 1) / Columns;
	const int MaxScroll = maximum(0, TotalRows - VisibleRows);
	BkwDdnetVoteState::ScrollRow = std::clamp(BkwDdnetVoteState::ScrollRow, 0, MaxScroll);
	if(Interactive && pUi->MouseInside(&Body))
	{
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_UP))
			BkwDdnetVoteState::ScrollRow = maximum(0, BkwDdnetVoteState::ScrollRow - 1);
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_DOWN))
			BkwDdnetVoteState::ScrollRow = minimum(MaxScroll, BkwDdnetVoteState::ScrollRow + 1);
	}

	const float Gap = 8.0f;
	const float CardW = (Body.w - Gap * (Columns - 1)) / Columns;
	const float CardH = (Body.h - Gap) / VisibleRows;
	static CButtonContainer s_aVoteButtons[Columns * VisibleRows];
	static CButtonContainer s_aDetailsButtons[Columns * VisibleRows];

	int Slot = 0;
	for(int Row = 0; Row < VisibleRows; ++Row)
	{
		for(int Col = 0; Col < Columns; ++Col, ++Slot)
		{
			const int FilterIndex = (BkwDdnetVoteState::ScrollRow + Row) * Columns + Col;
			if(FilterIndex >= (int)vFiltered.size())
				continue;
			const int MapIndex = vFiltered[FilterIndex];
			const auto &Map = BkwDdnetVoteState::vMaps[MapIndex];
			if(Interactive)
				BkwDdnetUpdateThumbnail(pUi, MapIndex);

			CUIRect Card{Body.x + Col * (CardW + Gap), Body.y + Row * (CardH + Gap), CardW, CardH};
			Card.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.050f), IGraphics::CORNER_ALL, 9.0f);
			Card.Margin(6.0f, &Card);

			CUIRect Image, Info, Buttons;
			Card.HSplitTop(Card.h * 0.40f, &Image, &Card);
			Card.HSplitBottom(27.0f, &Info, &Buttons);
			Image.Draw(ColorRGBA(0.02f, 0.022f, 0.027f, 0.95f), IGraphics::CORNER_ALL, 6.0f);
			if(MapIndex < (int)BkwDdnetVoteState::vThumbnails.size() && BkwDdnetVoteState::vThumbnails[MapIndex].m_Texture.IsValid())
				BkwDdnetRenderTextureCover(pUi, BkwDdnetVoteState::vThumbnails[MapIndex].m_Texture, Image, 360.0f / 225.0f);
			else
				pUi->DoLabel(&Image, "DDNet", 12.0f, TEXTALIGN_MC);

			Info.HSplitTop(4.0f, nullptr, &Info);
			CUIRect Name, Meta, Mapper, Release;
			Info.HSplitTop(18.0f, &Name, &Info);
			Info.HSplitTop(15.0f, &Meta, &Info);
			Info.HSplitTop(15.0f, &Mapper, &Info);
			Info.HSplitTop(15.0f, &Release, &Info);
			pUi->DoLabel(&Name, Map.m_Name.c_str(), 10.8f, TEXTALIGN_ML, {.m_MaxWidth = Name.w});

			char aMeta[128];
			str_format(aMeta, sizeof(aMeta), "%s   %s", Map.m_Type.c_str(), BkwDdnetStars(Map.m_Difficulty).c_str());
			pUi->DoLabel(&Meta, aMeta, 8.6f, TEXTALIGN_ML, {.m_MaxWidth = Meta.w});

			char aMapper[192];
			str_format(aMapper, sizeof(aMapper), "Автор: %s", Map.m_Mapper.c_str());
			pUi->DoLabel(&Mapper, aMapper, 8.2f, TEXTALIGN_ML, {.m_MaxWidth = Mapper.w});

			char aRelease[160];
			str_format(aRelease, sizeof(aRelease), "Релиз: %s", Map.m_Release.empty() ? "—" : Map.m_Release.c_str());
			pUi->DoLabel(&Release, aRelease, 8.0f, TEXTALIGN_ML, {.m_MaxWidth = Release.w});

			CUIRect Details, Vote;
			Buttons.VSplitMid(&Details, &Vote, 5.0f);
			if(BkwDdnetSmallButton(pUi, &s_aDetailsButtons[Slot], "Подробнее", Details, Interactive, false, false, BkwTfSvg::EIcon::INFO))
				BkwDdnetStartStats(MapIndex);
			if(BkwDdnetSmallButton(pUi, &s_aVoteButtons[Slot], "Голосовать", Vote, Interactive, true, false, BkwTfSvg::EIcon::CHECK))
				BkwDdnetSendMapVote(pGameClient, Map.m_Name);
		}
	}
}

inline void BkwDdnetRenderStats(CUi *pUi, CGameClient *pGameClient, CUIRect Body, bool Interactive)
{
	static CButtonContainer s_BackButton;
	static CButtonContainer s_VoteButton;
	CUIRect BackRow;
	Body.HSplitTop(30.0f, &BackRow, &Body);
	CUIRect Back, Spacer;
	BackRow.VSplitLeft(125.0f, &Back, &Spacer);
	if(BkwDdnetSmallButton(pUi, &s_BackButton, "К картам", Back, Interactive, false, false, BkwTfSvg::EIcon::BACK))
	{
		BkwDdnetVoteState::DetailOpen = false;
		BkwDdnetVoteState::SelectedMap = -1;
		return;
	}
	Body.HSplitTop(8.0f, nullptr, &Body);

	if(BkwDdnetVoteState::SelectedMap < 0 || BkwDdnetVoteState::SelectedMap >= (int)BkwDdnetVoteState::vMaps.size())
		return;
	const int MapIndex = BkwDdnetVoteState::SelectedMap;
	const auto &Map = BkwDdnetVoteState::vMaps[MapIndex];
	if(Interactive)
	{
		BkwDdnetUpdateThumbnail(pUi, MapIndex);
		BkwDdnetUpdateStats();
	}

	CUIRect Left, Right;
	Body.VSplitLeft(Body.w * 0.46f, &Left, &Right);
	Left.VSplitRight(8.0f, &Left, nullptr);
	Right.VSplitLeft(8.0f, nullptr, &Right);
	Left.Draw(ColorRGBA(0.02f, 0.024f, 0.03f, 0.78f), IGraphics::CORNER_ALL, 9.0f);
	Right.Draw(ColorRGBA(0.02f, 0.024f, 0.03f, 0.78f), IGraphics::CORNER_ALL, 9.0f);
	Left.Margin(8.0f, &Left);
	Right.Margin(10.0f, &Right);

	CUIRect Image, Vote;
	Left.HSplitTop(Left.h - 38.0f, &Image, &Vote);
	Image.Draw(ColorRGBA(0.02f, 0.022f, 0.027f, 0.86f), IGraphics::CORNER_ALL, 7.0f);
	if(MapIndex < (int)BkwDdnetVoteState::vThumbnails.size() && BkwDdnetVoteState::vThumbnails[MapIndex].m_Texture.IsValid())
		BkwDdnetRenderTexture(pUi, BkwDdnetVoteState::vThumbnails[MapIndex].m_Texture, Image);
	else
		pUi->DoLabel(&Image, "DDNet", 16.0f, TEXTALIGN_MC);
	Vote.HSplitTop(8.0f, nullptr, &Vote);
	if(BkwDdnetSmallButton(pUi, &s_VoteButton, "Голосовать", Vote, Interactive, true, false, BkwTfSvg::EIcon::CHECK))
		BkwDdnetSendMapVote(pGameClient, Map.m_Name);

	CUIRect Name, Meta, Mapper, Release;
	Right.HSplitTop(25.0f, &Name, &Right);
	Right.HSplitTop(20.0f, &Meta, &Right);
	Right.HSplitTop(20.0f, &Mapper, &Right);
	Right.HSplitTop(20.0f, &Release, &Right);
	pUi->DoLabel(&Name, Map.m_Name.c_str(), 15.0f, TEXTALIGN_ML, {.m_MaxWidth = Name.w});

	char aMeta[160];
	str_format(aMeta, sizeof(aMeta), "%s   %s   •   %d очк.", Map.m_Type.c_str(), BkwDdnetStars(Map.m_Difficulty).c_str(), Map.m_Points);
	pUi->DoLabel(&Meta, aMeta, 9.6f, TEXTALIGN_ML, {.m_MaxWidth = Meta.w});

	char aMapper[192];
	str_format(aMapper, sizeof(aMapper), "Автор: %s", Map.m_Mapper.c_str());
	pUi->DoLabel(&Mapper, aMapper, 9.4f, TEXTALIGN_ML, {.m_MaxWidth = Mapper.w});

	char aRelease[160];
	str_format(aRelease, sizeof(aRelease), "Релиз: %s", Map.m_Release.empty() ? "—" : Map.m_Release.c_str());
	pUi->DoLabel(&Release, aRelease, 9.2f, TEXTALIGN_ML, {.m_MaxWidth = Release.w});
	Right.HSplitTop(8.0f, nullptr, &Right);

	const auto &Stats = BkwDdnetVoteState::Stats;
	if(Stats.m_Failed)
	{
		pUi->DoLabel(&Right, "Не удалось загрузить статистику карты.", 10.5f, TEXTALIGN_TC);
		return;
	}
	if(!Stats.m_Ready)
	{
		pUi->DoLabel(&Right, "Загружаю статистику…", 10.5f, TEXTALIGN_TC);
		return;
	}

	char aLine[192];
	CUIRect Line;
	Right.HSplitTop(22.0f, &Line, &Right);
	str_format(aLine, sizeof(aLine), "Финишей: %d    Игроков: %d", Stats.m_Finishes, Stats.m_Finishers);
	pUi->DoLabel(&Line, aLine, 9.6f, TEXTALIGN_ML);
	Right.HSplitTop(22.0f, &Line, &Right);
	str_format(aLine, sizeof(aLine), "Среднее время: %.1f сек.    Макс. team: %d", Stats.m_AverageTime, Stats.m_BiggestTeam);
	pUi->DoLabel(&Line, aLine, 9.6f, TEXTALIGN_ML);
	Right.HSplitTop(8.0f, nullptr, &Right);
	Right.HSplitTop(22.0f, &Line, &Right);
	pUi->DoLabel(&Line, "Лучшие результаты", 11.0f, TEXTALIGN_ML);
	for(const auto &Rank : Stats.m_vRanks)
	{
		Right.HSplitTop(21.0f, &Line, &Right);
		str_format(aLine, sizeof(aLine), "#%d  %s   %.2f сек.   %s", Rank.m_Rank, Rank.m_Player.c_str(), Rank.m_Time, Rank.m_Country.c_str());
		pUi->DoLabel(&Line, aLine, 9.1f, TEXTALIGN_ML, {.m_MaxWidth = Line.w});
	}
}

inline void BkwDdnetRenderPlayers(CUi *pUi, CGameClient *pGameClient, CVoting &Voting, CUIRect Body, bool Interactive, bool Spectate)
{
	CUIRect ReasonRow;
	Body.HSplitTop(34.0f, &ReasonRow, &Body);
	CUIRect ReasonLabel, ReasonEdit;
	ReasonRow.VSplitLeft(70.0f, &ReasonLabel, &ReasonEdit);
	pUi->DoLabel(&ReasonLabel, "Причина:", 10.5f, TEXTALIGN_ML);
	ReasonEdit.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 7.0f);
	ReasonEdit.Margin(5.0f, &ReasonEdit);
	BkwDdnetVoteState::ReasonInput.SetEmptyText("Укажите причину…");
	if(Interactive)
		pUi->DoEditBox(&BkwDdnetVoteState::ReasonInput, &ReasonEdit, 10.5f);
	else
		pUi->DoLabel(&ReasonEdit, BkwDdnetVoteState::ReasonInput.IsEmpty() ? "Укажите причину…" : BkwDdnetVoteState::ReasonInput.GetString(), 10.5f, TEXTALIGN_ML, {.m_MaxWidth = ReasonEdit.w});
	Body.HSplitTop(8.0f, nullptr, &Body);

	std::vector<int> vClients;
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ++ClientId)
	{
		if(!pGameClient->m_aClients[ClientId].m_Active || ClientId == pGameClient->m_Snap.m_LocalClientId)
			continue;
		vClients.push_back(ClientId);
	}
	if(vClients.empty())
	{
		pUi->DoLabel(&Body, "Нет подходящих игроков.", 11.5f, TEXTALIGN_MC);
		return;
	}

	const int VisibleRows = maximum(1, (int)(Body.h / 35.0f));
	const int MaxScroll = maximum(0, (int)vClients.size() - VisibleRows);
	BkwDdnetVoteState::ScrollRow = std::clamp(BkwDdnetVoteState::ScrollRow, 0, MaxScroll);
	if(Interactive && pUi->MouseInside(&Body))
	{
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_UP))
			BkwDdnetVoteState::ScrollRow = maximum(0, BkwDdnetVoteState::ScrollRow - 2);
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_DOWN))
			BkwDdnetVoteState::ScrollRow = minimum(MaxScroll, BkwDdnetVoteState::ScrollRow + 2);
	}

	static CButtonContainer s_aKickButtons[MAX_CLIENTS];
	static CButtonContainer s_aSpecButtons[MAX_CLIENTS];
	for(int i = 0; i < VisibleRows && BkwDdnetVoteState::ScrollRow + i < (int)vClients.size(); ++i)
	{
		const int ClientId = vClients[BkwDdnetVoteState::ScrollRow + i];
		CUIRect Row;
		Body.HSplitTop(31.0f, &Row, &Body);
		Body.HSplitTop(4.0f, nullptr, &Body);
		Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.045f), IGraphics::CORNER_ALL, 7.0f);
		Row.Margin(5.0f, &Row);
		CUIRect Name, Action;
		Row.VSplitRight(140.0f, &Name, &Action);
		pUi->DoLabel(&Name, pGameClient->m_aClients[ClientId].m_aName, 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Name.w - 5.0f});
		CButtonContainer *pButton = Spectate ? &s_aSpecButtons[ClientId] : &s_aKickButtons[ClientId];
		if(BkwDdnetSmallButton(pUi, pButton, Spectate ? "В наблюдатели" : "Кик с сервера", Action, Interactive, false, !Spectate, Spectate ? BkwTfSvg::EIcon::USER : BkwTfSvg::EIcon::WARNING))
		{
			const char *pReason = BkwDdnetVoteState::ReasonInput.GetString();
			if(Spectate)
				Voting.CallvoteSpectate(ClientId, pReason);
			else
				Voting.CallvoteKick(ClientId, pReason);
			BkwDdnetVoteState::ReasonInput.Clear();
		}
	}
}

inline void RenderBkwDdnetVoteOverlay(CUi *pUi, CGameClient *pGameClient, const CVoting &VotingConst, bool Interactive)
{
	if(!pUi || !pGameClient)
		return;
	CVoting &Voting = const_cast<CVoting &>(VotingConst);
	if(Interactive)
	{
		pUi->SetEnabled(true);
		pUi->Update();
		BkwDdnetUpdateCatalog();
		if(BkwDdnetVoteState::DetailOpen)
			BkwDdnetUpdateStats();
	}

	pUi->MapScreen();
	CUIRect Screen = *pUi->Screen();

	// The details page uses the selected map preview as the scene background.
	// A strong dark overlay keeps text readable while retaining the map artwork.
	if(BkwDdnetVoteState::DetailOpen && BkwDdnetVoteState::SelectedMap >= 0 && BkwDdnetVoteState::SelectedMap < (int)BkwDdnetVoteState::vThumbnails.size())
	{
		const int MapIndex = BkwDdnetVoteState::SelectedMap;
		if(Interactive)
			BkwDdnetUpdateThumbnail(pUi, MapIndex);
		const auto &Thumb = BkwDdnetVoteState::vThumbnails[MapIndex];
		if(Thumb.m_Texture.IsValid())
			BkwDdnetRenderTexture(pUi, Thumb.m_Texture, Screen, 0.58f);
		Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.72f), IGraphics::CORNER_NONE, 0.0f);
	}
	else
	{
		Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, Interactive ? 0.34f : 0.58f), IGraphics::CORNER_NONE, 0.0f);
	}

	const float CardW = minimum(Screen.w - 18.0f, 820.0f);
	const float CardH = minimum(Screen.h - 14.0f, 480.0f);
	CUIRect Card{Screen.x + (Screen.w - CardW) * 0.5f, Screen.y + (Screen.h - CardH) * 0.5f, CardW, CardH};
	Card.Draw(ColorRGBA(0.66f, 0.68f, 0.72f, 0.62f), IGraphics::CORNER_ALL, 18.0f);
	Card.Margin(3.2f, &Card);
	Card.Draw(ColorRGBA(0.024f, 0.026f, 0.031f, BkwDdnetVoteState::DetailOpen ? 0.91f : 0.995f), IGraphics::CORNER_ALL, 16.0f);
	Card.Margin(13.0f, &Card);

	CUIRect Header, Body;
	Card.HSplitTop(48.0f, &Header, &Body);
	CUIRect Title, Tabs;
	Header.VSplitLeft(215.0f, &Title, &Tabs);
	CUIRect TitleIcon, TitleText;
	Title.VSplitLeft(32.0f, &TitleIcon, &TitleText);
	TitleIcon.Margin(6.0f, &TitleIcon);
	BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::GAME, TitleIcon, ColorRGBA(0.58f, 0.78f, 1.0f, 0.96f));
	pUi->DoLabel(&TitleText, "DDNet Голосование", 16.0f, TEXTALIGN_ML);

	static CButtonContainer s_aTabButtons[3];
	const char *apTabs[3] = {"Карты", "Кик", "Наблюдатели"};
	const BkwTfSvg::EIcon aTabIcons[3] = {BkwTfSvg::EIcon::GAME, BkwTfSvg::EIcon::WARNING, BkwTfSvg::EIcon::USER};
	for(int Tab = 0; Tab < 3; ++Tab)
	{
		CUIRect Button;
		Tabs.VSplitLeft(Tabs.w / (3 - Tab), &Button, &Tabs);
		const bool Active = BkwDdnetVoteState::SelectedTab == Tab;
		const bool Hovered = Interactive && pUi->MouseInside(&Button);
		Button.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Active ? 0.16f : (Hovered ? 0.10f : 0.045f)), IGraphics::CORNER_ALL, 7.0f);

		CUIRect IconRect, LabelRect;
		Button.VSplitLeft(25.0f, &IconRect, &LabelRect);
		IconRect.Margin(6.0f, &IconRect);
		BkwTfSvg::Render(pUi, aTabIcons[Tab], IconRect, ColorRGBA(1.0f, 1.0f, 1.0f, Active ? 1.0f : 0.82f));
		LabelRect.VSplitLeft(1.0f, nullptr, &LabelRect);
		pUi->DoLabel(&LabelRect, apTabs[Tab], 10.0f, TEXTALIGN_MC, {.m_MaxWidth = LabelRect.w - 2.0f});
		if(Interactive && pUi->DoButtonLogic(&s_aTabButtons[Tab], 0, &Button, BUTTONFLAG_LEFT))
		{
			BkwDdnetVoteState::SelectedTab = Tab;
			BkwDdnetVoteState::ScrollRow = 0;
			BkwDdnetVoteState::DetailOpen = false;
		}
		if(Tab < 2)
			Tabs.VSplitLeft(5.0f, nullptr, &Tabs);
	}

	CUIRect Divider;
	Body.HSplitTop(1.0f, &Divider, &Body);
	Divider.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.10f), IGraphics::CORNER_NONE, 0.0f);
	Body.HSplitTop(9.0f, nullptr, &Body);

	if(BkwDdnetVoteState::SelectedTab == 0)
	{
		if(BkwDdnetVoteState::DetailOpen)
			BkwDdnetRenderStats(pUi, pGameClient, Body, Interactive);
		else
			BkwDdnetRenderMapCards(pUi, pGameClient, Body, Interactive);
	}
	else
	{
		BkwDdnetRenderPlayers(pUi, pGameClient, Voting, Body, Interactive, BkwDdnetVoteState::SelectedTab == 2);
	}
}

#endif
