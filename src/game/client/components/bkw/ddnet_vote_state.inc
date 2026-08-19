#ifndef GAME_CLIENT_COMPONENTS_BKW_DDNET_VOTE_STATE_INC
#define GAME_CLIENT_COMPONENTS_BKW_DDNET_VOTE_STATE_INC

#include <engine/graphics.h>
#include <engine/http.h>

#include <game/client/lineinput.h>

#include <memory>
#include <string>
#include <vector>

namespace BkwDdnetVoteState
{
struct SMap
{
	std::string m_Name;
	std::string m_Thumbnail;
	std::string m_Type;
	std::string m_Mapper;
	int m_Points = 0;
	int m_Difficulty = 0;
};

struct SThumbnail
{
	std::shared_ptr<IHttpRequest> m_pRequest;
	IGraphics::CTextureHandle m_Texture;
	bool m_Failed = false;
};

struct SRank
{
	int m_Rank = 0;
	std::string m_Player;
	double m_Time = 0.0;
	std::string m_Country;
};

struct SStats
{
	bool m_Ready = false;
	bool m_Failed = false;
	std::string m_Name;
	std::string m_Type;
	std::string m_Mapper;
	int m_Points = 0;
	int m_Difficulty = 0;
	double m_AverageTime = 0.0;
	int m_Finishes = 0;
	int m_Finishers = 0;
	int m_BiggestTeam = 0;
	std::vector<SRank> m_vRanks;
};

inline bool CatalogRequested = false;
inline bool CatalogReady = false;
inline bool CatalogFailed = false;
inline std::shared_ptr<IHttpRequest> pCatalogRequest;
inline std::vector<SMap> vMaps;
inline std::vector<SThumbnail> vThumbnails;

inline int SelectedTab = 0;
inline int ScrollRow = 0;
inline bool DetailOpen = false;
inline int SelectedMap = -1;
inline CLineInputBuffered<96> SearchInput;

inline std::shared_ptr<IHttpRequest> pStatsRequest;
inline std::string StatsRequestedMap;
inline SStats Stats;
}

#endif
