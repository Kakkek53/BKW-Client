#ifndef GAME_CLIENT_COMPONENTS_BKW_DDNET_VOTE_STATE_INC
#define GAME_CLIENT_COMPONENTS_BKW_DDNET_VOTE_STATE_INC

#include <engine/graphics.h>
#include <engine/http.h>
#include <game/client/lineinput.h>
#include <game/voting.h>
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
	std::string m_Release;
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
	std::string m_Country;
	double m_Time = 0.0;
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

enum EMapSort
{
	SORT_NAME = 0,
	SORT_RELEASE_NEWEST,
	SORT_RELEASE_OLDEST,
	SORT_DIFFICULTY,
	SORT_POINTS,
	NUM_SORTS,
};

inline IHttp *pHttp = nullptr;
inline bool CatalogRequested = false;
inline bool CatalogReady = false;
inline bool CatalogFailed = false;
inline std::shared_ptr<IHttpRequest> pCatalogRequest;
inline std::vector<SMap> vMaps;
inline std::vector<SThumbnail> vThumbnails;

inline bool DetailOpen = false;
inline int SelectedMap = -1;
inline std::string StatsRequestedMap;
inline std::shared_ptr<IHttpRequest> pStatsRequest;
inline SStats Stats;

inline int SelectedTab = 0;
inline int ScrollRow = 0;
inline bool CustomMenuEnabled = true;
inline CLineInputBuffered<128> SearchInput;
inline CLineInputBuffered<VOTE_REASON_LENGTH> ReasonInput;

// Catalog browsing state. TypeFilter == 0 means all types, otherwise it is an
// index into the compact type list rendered by the overlay. DifficultyFilter
// follows the same convention: 0 = all, 1..5 = exact difficulty.
inline int TypeFilter = 0;
inline int DifficultyFilter = 0;
inline int SortMode = SORT_NAME;
inline bool FavoritesOnly = false;
inline bool FavoritesLoaded = false;
inline std::vector<std::string> vFavoriteMaps;
}

#endif
