	CMenuBackground *pSelf = (CMenuBackground *)pUser;
	const char *pSuffix = str_endswith(pName, ".map");
	if(IsDir || !pSuffix)
		return 0;
	char aFullName[128];
	char aThemeName[128];
	str_truncate(aFullName, sizeof(aFullName), pName, pSuffix - pName);

	bool IsDay = false;
	bool IsNight = false;
	if((pSuffix = str_endswith(aFullName, "_day")))
	{
		str_truncate(aThemeName, sizeof(aThemeName), pName, pSuffix - aFullName);
		IsDay = true;
	}
	else if((pSuffix = str_endswith(aFullName, "_night")))
	{
		str_truncate(aThemeName, sizeof(aThemeName), pName, pSuffix - aFullName);
		IsNight = true;
	}
	else
		str_copy(aThemeName, aFullName);

	if(str_comp(aThemeName, "none") == 0 || str_comp(aThemeName, "auto") == 0 || str_comp(aThemeName, "rand") == 0) // "none", "auto" and "rand" reserved, disallowed for maps
		return 0;

	// try to edit an existing theme
	for(auto &Theme : pSelf->m_vThemes)
	{
		if(str_comp(Theme.m_Name.c_str(), aThemeName) == 0)
		{
			if(IsDay)
				Theme.m_HasDay = true;
			if(IsNight)
				Theme.m_HasNight = true;
			return 0;
		}
	}

	// make new theme
	log_trace("menuthemes", "added theme '%s' from 'themes/%s'", aThemeName, pName);
	pSelf->m_vThemes.emplace_back(aThemeName, IsDay, IsNight);
	pSelf->LoadThemeIcon(pSelf->m_vThemes.back());

	if(time_get_nanoseconds() - pSelf->m_ThemeScanStartTime > 500ms)
	{
		pSelf->GameClient()->m_Menus.RenderLoading(Localize("Loading menu themes"), "", 0);
	}
	return 0;
}

void CMenuBackground::LoadMenuBackground(bool HasDayHint, bool HasNightHint)
{
	if(!m_IsInit)
		return;

	if(m_Loaded && m_pMap == m_pBackgroundMap.get())
		m_pMap->Unload();

	m_Loaded = false;
	m_pMap = m_pBackgroundMap.get();
	m_pLayers = m_pBackgroundLayers;
	m_pImages = m_pBackgroundImages;

	ResetPositions();

	str_copy(m_aMapName, g_Config.m_ClMenuMap);

	if(g_Config.m_ClMenuMap[0] != '\0')
	{
		m_Loading = true;

		const char *pMenuMap = g_Config.m_ClMenuMap;
		if(str_comp(pMenuMap, "auto") == 0)
		{
			const ETimeSeason Season = time_season();
			switch(Season)
			{
			case ETimeSeason::SPRING:
			case ETimeSeason::EASTER:
				pMenuMap = "heavens";
				break;
			case ETimeSeason::SUMMER:
				pMenuMap = "jungle";
				break;
			case ETimeSeason::AUTUMN:
			case ETimeSeason::HALLOWEEN:
				pMenuMap = "autumn";
				break;
			case ETimeSeason::WINTER:
			case ETimeSeason::XMAS:
				pMenuMap = "winter";
				break;
			case ETimeSeason::NEWYEAR:
				pMenuMap = "newyear";
				break;
			default:
				dbg_assert_failed("Invalid season: %d", (int)Season);
			}
		}
		else if(str_comp(pMenuMap, "rand") == 0)
		{
			// make sure to load themes
			const std::vector<CTheme> &vThemesRef = GetThemes();
			if(vThemesRef.size() > PREDEFINED_THEMES_COUNT)
			{
				int RandomThemeIndex = rand() % (vThemesRef.size() - PREDEFINED_THEMES_COUNT);
				if(RandomThemeIndex + PREDEFINED_THEMES_COUNT < (int)vThemesRef.size())
					pMenuMap = vThemesRef[RandomThemeIndex + PREDEFINED_THEMES_COUNT].m_Name.c_str();
			}
		}

		char aBuf[128];

		const int HourOfTheDay = time_houroftheday();
		const bool IsDaytime = HourOfTheDay >= 6 && HourOfTheDay < 18;

		if(!m_Loaded && ((HasDayHint && IsDaytime) || (HasNightHint && !IsDaytime)))
		{
			str_format(aBuf, sizeof(aBuf), "themes/%s_%s.map", pMenuMap, IsDaytime ? "day" : "night");