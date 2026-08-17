#ifndef GAME_CLIENT_COMPONENTS_BKW_DDSTATS_HOURS_H
#define GAME_CLIENT_COMPONENTS_BKW_DDSTATS_HOURS_H

#include <base/str.h>
#include <base/time.h>

#include <engine/http.h>
#include <engine/external/json-parser/json.h>

#include <cstdint>
#include <memory>
#include <string>

namespace Bkw
{
class CDdStatsHours
{
	std::shared_ptr<IHttpRequest> m_pRequest;
	std::string m_Player;
	std::string m_StartOfPlaytime;
	int64_t m_TotalSecondsPlayed = 0;
	int64_t m_AverageSecondsPlayed = 0;
	int64_t m_LastSuccessfulUpdate = 0;
	bool m_Loaded = false;
	bool m_Error = false;
	int m_HttpStatus = 0;

	static std::string UrlEncode(const char *pText)
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

	static int64_t JsonInt64(const json_value &Value)
	{
		if(Value.type == json_integer)
			return (int64_t)Value.u.integer;
		if(Value.type == json_double)
			return (int64_t)Value.u.dbl;
		return 0;
	}

public:
	static constexpr size_t MAX_RESPONSE_SIZE = 16 * 1024 * 1024;
	static constexpr int64_t CACHE_SECONDS = 10 * 60;

	void Reset()
	{
		if(m_pRequest)
			m_pRequest->Abort();
		m_pRequest = nullptr;
		m_Player.clear();
		m_StartOfPlaytime.clear();
		m_TotalSecondsPlayed = 0;
		m_AverageSecondsPlayed = 0;
		m_LastSuccessfulUpdate = 0;
		m_Loaded = false;
		m_Error = false;
		m_HttpStatus = 0;
	}

	bool CacheFreshFor(const char *pPlayer) const
	{
		if(!m_Loaded || !pPlayer || pPlayer[0] == '\0' || str_comp(m_Player.c_str(), pPlayer) != 0 || m_LastSuccessfulUpdate <= 0)
			return false;
		const int64_t Now = time_timestamp();
		return Now >= m_LastSuccessfulUpdate && Now - m_LastSuccessfulUpdate < CACHE_SECONDS;
	}

	void Request(IHttp *pHttp, const char *pPlayer, bool Force = false)
	{
		if(!pHttp || !pPlayer || pPlayer[0] == '\0')
			return;
		if(!Force && CacheFreshFor(pPlayer))
			return;
		if(m_pRequest)
			m_pRequest->Abort();

		m_Player = pPlayer;
		m_StartOfPlaytime.clear();
		m_TotalSecondsPlayed = 0;
		m_AverageSecondsPlayed = 0;
		m_Loaded = false;
		m_Error = false;
		m_HttpStatus = 0;

		std::string Url = "https://ddstats.tw/player/json?player=";
		Url += UrlEncode(pPlayer);
		auto pGet = HttpGet(Url.c_str());
		pGet->Timeout(CTimeout{8000, 0, 4096, 8});
		pGet->MaxResponseSize(MAX_RESPONSE_SIZE);
		pGet->FailOnErrorStatus(false);
		pGet->LogProgress(HTTPLOG::NONE);
		m_pRequest = pGet;
		pHttp->Run(pGet);
	}

	void Poll()
	{
		if(!m_pRequest || !m_pRequest->Done())
			return;

		const bool Done = m_pRequest->State() == EHttpState::DONE;
		m_HttpStatus = Done ? m_pRequest->StatusCode() : -1;
		if(!Done || m_HttpStatus < 200 || m_HttpStatus >= 400)
		{
			m_Error = true;
			m_pRequest = nullptr;
			return;
		}

		unsigned char *pResult = nullptr;
		size_t ResultSize = 0;
		m_pRequest->Result(&pResult, &ResultSize);
		if(!pResult || ResultSize == 0)
		{
			m_Error = true;
			m_pRequest = nullptr;
			return;
		}

		json_value *pRoot = json_parse(reinterpret_cast<const char *>(pResult), ResultSize);
		if(!pRoot || pRoot->type != json_object)
		{
			if(pRoot)
				json_value_free(pRoot);
			m_Error = true;
			m_pRequest = nullptr;
			return;
		}

		const json_value &GeneralActivity = (*pRoot)["general_activity"];
		if(GeneralActivity.type != json_object)
		{
			json_value_free(pRoot);
			m_Error = true;
			m_pRequest = nullptr;
			return;
		}

		m_TotalSecondsPlayed = JsonInt64(GeneralActivity["total_seconds_played"]);
		m_AverageSecondsPlayed = JsonInt64(GeneralActivity["average_seconds_played"]);
		if(m_TotalSecondsPlayed < 0)
			m_TotalSecondsPlayed = 0;
		if(m_AverageSecondsPlayed < 0)
			m_AverageSecondsPlayed = 0;

		const json_value &Start = GeneralActivity["start_of_playtime"];
		if(Start.type == json_string && Start.u.string.ptr)
			m_StartOfPlaytime = Start.u.string.ptr;

		const json_value &Profile = (*pRoot)["profile"];
		if(Profile.type == json_object)
		{
			const json_value &Name = Profile["name"];
			if(Name.type == json_string && Name.u.string.ptr && Name.u.string.ptr[0] != '\0')
				m_Player = Name.u.string.ptr;
		}

		json_value_free(pRoot);
		m_Loaded = true;
		m_Error = false;
		m_LastSuccessfulUpdate = time_timestamp();
		m_pRequest = nullptr;
	}

	bool Loading() const { return m_pRequest != nullptr; }
	bool Loaded() const { return m_Loaded; }
	bool Error() const { return m_Error; }
	int HttpStatus() const { return m_HttpStatus; }
	const char *Player() const { return m_Player.c_str(); }
	const char *StartOfPlaytime() const { return m_StartOfPlaytime.c_str(); }
	int64_t TotalSecondsPlayed() const { return m_TotalSecondsPlayed; }
	int64_t AverageSecondsPlayed() const { return m_AverageSecondsPlayed; }
	int64_t LastSuccessfulUpdate() const { return m_LastSuccessfulUpdate; }
	double TotalHours() const { return (double)m_TotalSecondsPlayed / 3600.0; }

	static void FormatDuration(int64_t Seconds, char *pBuf, size_t BufSize, bool IncludeSeconds = false)
	{
		if(Seconds < 0)
			Seconds = 0;
		const int64_t Hours = Seconds / 3600;
		const int64_t Minutes = (Seconds % 3600) / 60;
		const int64_t SecondsPart = Seconds % 60;
		if(IncludeSeconds)
			str_format(pBuf, (int)BufSize, "%lld ч %02lld мин %02lld сек", (long long)Hours, (long long)Minutes, (long long)SecondsPart);
		else
			str_format(pBuf, (int)BufSize, "%lld ч %02lld мин", (long long)Hours, (long long)Minutes);
	}
};

inline CDdStatsHours &DdStatsHoursState()
{
	static CDdStatsHours s_State;
	return s_State;
}
} // namespace Bkw

#endif // GAME_CLIENT_COMPONENTS_BKW_DDSTATS_HOURS_H
