#ifndef GAME_CLIENT_COMPONENTS_BKW_SAVE_STORE_H
#define GAME_CLIENT_COMPONENTS_BKW_SAVE_STORE_H

#include <base/io.h>
#include <engine/storage.h>

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Bkw
{
static constexpr const char *SAVE_FILE = "bkw-saves.txt";

struct SSaveEntry
{
	std::string m_Key;
	std::string m_ServerAddress;
	std::string m_Map;
	long long m_SavedAtUnix = 0;
	std::vector<std::string> m_vPlayers;
};

class CSaveStore
{
	std::vector<SSaveEntry> m_vEntries;
	bool m_Loaded = false;

	static std::string Escape(const std::string &Text)
	{
		std::string Result;
		Result.reserve(Text.size());
		for(unsigned char c : Text)
		{
			if(c == '%' || c == '\t' || c == '\n' || c == '\r')
			{
				char aBuf[4];
				static const char *s_pHex = "0123456789ABCDEF";
				aBuf[0] = '%';
				aBuf[1] = s_pHex[(c >> 4) & 0xF];
				aBuf[2] = s_pHex[c & 0xF];
				aBuf[3] = '\0';
				Result += aBuf;
			}
			else
				Result.push_back((char)c);
		}
		return Result;
	}

	static int HexValue(char c)
	{
		if(c >= '0' && c <= '9')
			return c - '0';
		if(c >= 'a' && c <= 'f')
			return 10 + c - 'a';
		if(c >= 'A' && c <= 'F')
			return 10 + c - 'A';
		return -1;
	}

	static std::string Unescape(const std::string &Text)
	{
		std::string Result;
		Result.reserve(Text.size());
		for(size_t i = 0; i < Text.size(); ++i)
		{
			if(Text[i] == '%' && i + 2 < Text.size())
			{
				const int Hi = HexValue(Text[i + 1]);
				const int Lo = HexValue(Text[i + 2]);
				if(Hi >= 0 && Lo >= 0)
				{
					Result.push_back((char)((Hi << 4) | Lo));
					i += 2;
					continue;
				}
			}
			Result.push_back(Text[i]);
		}
		return Result;
	}

	static std::vector<std::string> SplitTabs(const std::string &Line)
	{
		std::vector<std::string> vFields;
		size_t Start = 0;
		while(Start <= Line.size())
		{
			const size_t End = Line.find('\t', Start);
			vFields.push_back(Line.substr(Start, End == std::string::npos ? std::string::npos : End - Start));
			if(End == std::string::npos)
				break;
			Start = End + 1;
		}
		return vFields;
	}

public:
	const std::vector<SSaveEntry> &Entries() const { return m_vEntries; }
	bool IsLoaded() const { return m_Loaded; }

	void Load(IStorage *pStorage)
	{
		if(m_Loaded || !pStorage)
			return;
		m_Loaded = true;
		m_vEntries.clear();

		char *pData = pStorage->ReadFileStr(SAVE_FILE, IStorage::TYPE_SAVE);
		if(!pData)
			return;

		std::istringstream Stream(pData);
		std::string Line;
		while(std::getline(Stream, Line))
		{
			if(!Line.empty() && Line.back() == '\r')
				Line.pop_back();
			if(Line.empty() || Line[0] == '#')
				continue;

			const std::vector<std::string> vFields = SplitTabs(Line);
			if(vFields.size() < 4)
				continue;

			SSaveEntry Entry;
			Entry.m_Key = Unescape(vFields[0]);
			Entry.m_ServerAddress = Unescape(vFields[1]);
			Entry.m_Map = Unescape(vFields[2]);

			size_t PlayersStart = 3;
			if(vFields.size() >= 5)
			{
				char *pEnd = nullptr;
				const long long SavedAt = std::strtoll(vFields[3].c_str(), &pEnd, 10);
				if(pEnd && *pEnd == '\0')
				{
					Entry.m_SavedAtUnix = SavedAt;
					PlayersStart = 4;
				}
			}

			for(size_t i = PlayersStart; i < vFields.size(); ++i)
				Entry.m_vPlayers.push_back(Unescape(vFields[i]));
			std::sort(Entry.m_vPlayers.begin(), Entry.m_vPlayers.end());

			if(!Entry.m_Key.empty() && !Entry.m_ServerAddress.empty() && !Entry.m_Map.empty() && !Entry.m_vPlayers.empty())
				m_vEntries.push_back(std::move(Entry));
		}

		std::free(pData);
	}

	bool Save(IStorage *pStorage) const
	{
		if(!pStorage)
			return false;
		IOHANDLE File = pStorage->OpenFile(SAVE_FILE, IOFLAG_WRITE, IStorage::TYPE_SAVE);
		if(!File)
			return false;

		static constexpr const char *HEADER = "# BKW saves v2\n";
		io_write(File, HEADER, (unsigned)std::char_traits<char>::length(HEADER));
		for(const SSaveEntry &Entry : m_vEntries)
		{
			std::string Line = Escape(Entry.m_Key) + "\t" + Escape(Entry.m_ServerAddress) + "\t" + Escape(Entry.m_Map) + "\t" + std::to_string(Entry.m_SavedAtUnix);
			for(const std::string &Player : Entry.m_vPlayers)
				Line += "\t" + Escape(Player);
			Line += "\n";
			io_write(File, Line.data(), (unsigned)Line.size());
		}
		const bool Ok = io_error(File) == 0;
		io_close(File);
		return Ok;
	}

	std::string NextFreeKey() const
	{
		for(int Number = 1; Number < 1000000; ++Number)
		{
			const std::string Candidate = std::to_string(Number) + "a";
			const bool Used = std::any_of(m_vEntries.begin(), m_vEntries.end(), [&](const SSaveEntry &Entry) {
				return Entry.m_Key == Candidate;
			});
			if(!Used)
				return Candidate;
		}
		return "bkw1a";
	}

	void Add(SSaveEntry Entry)
	{
		std::sort(Entry.m_vPlayers.begin(), Entry.m_vPlayers.end());
		m_vEntries.push_back(std::move(Entry));
	}

	void Remove(size_t Index)
	{
		if(Index < m_vEntries.size())
			m_vEntries.erase(m_vEntries.begin() + Index);
	}
};
}

#endif
