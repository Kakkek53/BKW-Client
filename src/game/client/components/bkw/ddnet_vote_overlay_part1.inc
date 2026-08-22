
#include <base/math.h>
#include <base/str.h>

#include <engine/http.h>
#include <engine/shared/config.h>
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

