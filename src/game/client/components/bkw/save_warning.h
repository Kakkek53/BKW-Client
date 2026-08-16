#ifndef GAME_CLIENT_COMPONENTS_BKW_SAVE_WARNING_H
#define GAME_CLIENT_COMPONENTS_BKW_SAVE_WARNING_H

#include <base/system.h>
#include <base/vmath.h>

#include <engine/storage.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

namespace Bkw
{

static constexpr const char *SAVE_WARNING_SETTINGS_FILE = "bkw-save-warning.cfg";

enum class ESaveWarningExitAction
{
	NONE = 0,
	DISCONNECT,
	QUIT,
};

class CSaveWarningState
{
	static std::string Trim(const std::string &Value)
	{
		size_t Begin = 0;
		while(Begin < Value.size() && std::isspace((unsigned char)Value[Begin]))
			++Begin;
		size_t End = Value.size();
		while(End > Begin && std::isspace((unsigned char)Value[End - 1]))
			--End;
		return Value.substr(Begin, End - Begin);
	}

	static std::string Lower(std::string Value)
	{
		std::transform(Value.begin(), Value.end(), Value.begin(), [](unsigned char C) { return (char)std::tolower(C); });
		return Value;
	}

public:
	bool m_Loaded = false;
	bool m_Enabled = true;
	std::string m_ExcludedIps;
	std::string m_ExcludedCommunities;
	std::string m_ExcludedGameTypes;

	bool m_RaceStarted = false;
	bool m_RaceFinished = false;
	bool m_ProgressSaved = false;

	bool m_PopupActive = false;
	ESaveWarningExitAction m_ExitAction = ESaveWarningExitAction::NONE;
	int64_t m_ExitAfterSaveAt = 0;

	void ResetRace()
	{
		m_RaceStarted = false;
		m_RaceFinished = false;
		m_ProgressSaved = false;
		m_PopupActive = false;
		m_ExitAction = ESaveWarningExitAction::NONE;
		m_ExitAfterSaveAt = 0;
	}

	void MarkStarted()
	{
		m_RaceStarted = true;
		m_RaceFinished = false;
		m_ProgressSaved = false;
	}

	void MarkFinished()
	{
		m_RaceStarted = false;
		m_RaceFinished = true;
		m_ProgressSaved = false;
	}

	void MarkKilled()
	{
		m_RaceStarted = false;
		m_RaceFinished = false;
		m_ProgressSaved = false;
	}

	void MarkSaved()
	{
		if(m_RaceStarted && !m_RaceFinished)
			m_ProgressSaved = true;
	}

	void MarkLoaded()
	{
		m_RaceStarted = true;
		m_RaceFinished = false;
		m_ProgressSaved = false;
	}

	bool HasUnsavedProgress() const
	{
		return m_Enabled && m_RaceStarted && !m_RaceFinished && !m_ProgressSaved;
	}

	static bool ListMatches(const std::string &List, const char *pValue)
	{
		if(List.empty() || pValue == nullptr || pValue[0] == '\0')
			return false;

		const std::string Haystack = Lower(pValue);
		std::string Normalized = List;
		std::replace(Normalized.begin(), Normalized.end(), ';', ',');
		std::replace(Normalized.begin(), Normalized.end(), '\n', ',');
		std::istringstream Stream(Normalized);
		std::string Token;
		while(std::getline(Stream, Token, ','))
		{
			Token = Lower(Trim(Token));
			if(Token.empty())
				continue;
			if(Haystack == Token || Haystack.find(Token) != std::string::npos)
				return true;
		}
		return false;
	}

	void Load(IStorage *pStorage)
	{
		if(m_Loaded || pStorage == nullptr)
			return;
		m_Loaded = true;

		char *pData = pStorage->ReadFileStr(SAVE_WARNING_SETTINGS_FILE, IStorage::TYPE_SAVE);
		if(!pData)
			return;

		std::istringstream Stream(pData);
		std::string Line;
		while(std::getline(Stream, Line))
		{
			if(str_startswith(Line.c_str(), "enabled="))
				m_Enabled = std::atoi(Line.c_str() + 8) != 0;
			else if(str_startswith(Line.c_str(), "ips="))
				m_ExcludedIps = Line.substr(4);
			else if(str_startswith(Line.c_str(), "communities="))
				m_ExcludedCommunities = Line.substr(12);
			else if(str_startswith(Line.c_str(), "gametypes="))
				m_ExcludedGameTypes = Line.substr(10);
		}
		std::free(pData);
	}

	void Save(IStorage *pStorage) const
	{
		if(pStorage == nullptr)
			return;
		IOHANDLE File = pStorage->OpenFile(SAVE_WARNING_SETTINGS_FILE, IOFLAG_WRITE, IStorage::TYPE_SAVE);
		if(!File)
			return;

		std::string Data;
		Data += std::string("enabled=") + (m_Enabled ? "1\n" : "0\n");
		Data += "ips=" + m_ExcludedIps + "\n";
		Data += "communities=" + m_ExcludedCommunities + "\n";
		Data += "gametypes=" + m_ExcludedGameTypes + "\n";
		io_write(File, Data.data(), Data.size());
		io_close(File);
	}
};

inline CSaveWarningState &SaveWarningState()
{
	static CSaveWarningState s_State;
	return s_State;
}

} // namespace Bkw

#endif