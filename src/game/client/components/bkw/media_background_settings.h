#ifndef GAME_CLIENT_COMPONENTS_BKW_MEDIA_BACKGROUND_SETTINGS_H
#define GAME_CLIENT_COMPONENTS_BKW_MEDIA_BACKGROUND_SETTINGS_H

#include <base/system.h>
#include <engine/storage.h>

#include <cstdlib>
#include <sstream>
#include <string>

namespace Bkw
{
static constexpr const char *MEDIA_BACKGROUND_SETTINGS_FILE = "bkw-media-background.cfg";

class CMediaBackgroundSettings
{
public:
	bool m_Loaded = false;
	bool m_Enabled = false;
	std::string m_Path;

	void Load(IStorage *pStorage)
	{
		if(m_Loaded || pStorage == nullptr)
			return;
		m_Loaded = true;
		char *pData = pStorage->ReadFileStr(MEDIA_BACKGROUND_SETTINGS_FILE, IStorage::TYPE_SAVE);
		if(!pData)
			return;
		std::istringstream Stream(pData);
		std::string Line;
		while(std::getline(Stream, Line))
		{
			if(str_startswith(Line.c_str(), "enabled="))
				m_Enabled = std::atoi(Line.c_str() + 8) != 0;
			else if(str_startswith(Line.c_str(), "path="))
				m_Path = Line.substr(5);
		}
		std::free(pData);
	}

	void Save(IStorage *pStorage) const
	{
		if(!pStorage)
			return;
		IOHANDLE File = pStorage->OpenFile(MEDIA_BACKGROUND_SETTINGS_FILE, IOFLAG_WRITE, IStorage::TYPE_SAVE);
		if(!File)
			return;
		std::string Data;
		Data += std::string("enabled=") + (m_Enabled ? "1\n" : "0\n");
		Data += "path=" + m_Path + "\n";
		io_write(File, Data.data(), Data.size());
		io_close(File);
	}
};

inline CMediaBackgroundSettings &MediaBackgroundSettings()
{
	static CMediaBackgroundSettings s_Settings;
	return s_Settings;
}
} // namespace Bkw

#endif
