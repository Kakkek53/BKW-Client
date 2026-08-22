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

inline int BkwDdnetHexValue(char Ch)
{
	if(Ch >= '0' && Ch <= '9')
		return Ch - '0';
	if(Ch >= 'A' && Ch <= 'F')
		return Ch - 'A' + 10;
	if(Ch >= 'a' && Ch <= 'f')
		return Ch - 'a' + 10;
	return -1;
}

inline std::string BkwDdnetEncodeFavorite(const std::string &Name)
{
	static const char *pHex = "0123456789ABCDEF";
	std::string Result;
	Result.reserve(Name.size());
	for(unsigned char Ch : Name)
	{
		if(Ch == '%' || Ch == '|')
		{
			Result.push_back('%');
			Result.push_back(pHex[(Ch >> 4) & 0xF]);
			Result.push_back(pHex[Ch & 0xF]);
		}
		else
			Result.push_back((char)Ch);
	}
	return Result;
}

inline std::string BkwDdnetDecodeFavorite(const std::string &Encoded)
{
	std::string Result;
	Result.reserve(Encoded.size());
	for(size_t i = 0; i < Encoded.size(); ++i)
	{
		if(Encoded[i] == '%' && i + 2 < Encoded.size())
		{
			const int Hi = BkwDdnetHexValue(Encoded[i + 1]);
			const int Lo = BkwDdnetHexValue(Encoded[i + 2]);
			if(Hi >= 0 && Lo >= 0)
			{
				Result.push_back((char)((Hi << 4) | Lo));
				i += 2;
				continue;
			}
		}
		Result.push_back(Encoded[i]);
	}
	return Result;
}

inline void BkwDdnetLoadFavorites()
{
	if(BkwDdnetVoteState::FavoritesLoaded)
		return;
	BkwDdnetVoteState::FavoritesLoaded = true;
	BkwDdnetVoteState::vFavoriteMaps.clear();

	const std::string Serialized = g_Config.m_BkwDdnetFavoriteMaps;
	size_t Start = 0;
	while(Start <= Serialized.size())
	{
		const size_t End = Serialized.find('|', Start);
		const std::string Part = Serialized.substr(Start, End == std::string::npos ? std::string::npos : End - Start);
		if(!Part.empty())
		{
			const std::string Name = BkwDdnetDecodeFavorite(Part);
			if(!Name.empty() && std::find(BkwDdnetVoteState::vFavoriteMaps.begin(), BkwDdnetVoteState::vFavoriteMaps.end(), Name) == BkwDdnetVoteState::vFavoriteMaps.end())
				BkwDdnetVoteState::vFavoriteMaps.push_back(Name);
		}
		if(End == std::string::npos)
			break;
		Start = End + 1;
	}
}

inline void BkwDdnetSaveFavorites()
{
	std::string Serialized;
	for(const std::string &Name : BkwDdnetVoteState::vFavoriteMaps)
	{
		const std::string Encoded = BkwDdnetEncodeFavorite(Name);
		const size_t Extra = Encoded.size() + (Serialized.empty() ? 0 : 1);
		if(Serialized.size() + Extra >= sizeof(g_Config.m_BkwDdnetFavoriteMaps))
			break;
		if(!Serialized.empty())
			Serialized.push_back('|');
		Serialized += Encoded;
	}
	str_copy(g_Config.m_BkwDdnetFavoriteMaps, Serialized.c_str(), sizeof(g_Config.m_BkwDdnetFavoriteMaps));
}

inline bool BkwDdnetIsFavorite(const std::string &Name)
{
	BkwDdnetLoadFavorites();
	return std::find(BkwDdnetVoteState::vFavoriteMaps.begin(), BkwDdnetVoteState::vFavoriteMaps.end(), Name) != BkwDdnetVoteState::vFavoriteMaps.end();
}

inline void BkwDdnetToggleFavorite(const std::string &Name)
{
	BkwDdnetLoadFavorites();
	auto It = std::find(BkwDdnetVoteState::vFavoriteMaps.begin(), BkwDdnetVoteState::vFavoriteMaps.end(), Name);
	if(It == BkwDdnetVoteState::vFavoriteMaps.end())
		BkwDdnetVoteState::vFavoriteMaps.push_back(Name);
	else
		BkwDdnetVoteState::vFavoriteMaps.erase(It);
	BkwDdnetSaveFavorites();
}

inline const char *BkwDdnetTypeFilterName(int Filter)
{
	static const char *s_apTypes[] = {"Все типы", "Novice", "Moderate", "Brutal", "Insane", "Dummy", "Solo", "Oldschool"};
	Filter = std::clamp(Filter, 0, (int)(sizeof(s_apTypes) / sizeof(s_apTypes[0])) - 1);
	return s_apTypes[Filter];
}

inline int BkwDdnetTypeFilterCount()
{
	return 8;
}

inline const char *BkwDdnetSortName(int SortMode)
{
	switch(SortMode)
	{
	case BkwDdnetVoteState::SORT_RELEASE_NEWEST: return "Новые сначала";
	case BkwDdnetVoteState::SORT_RELEASE_OLDEST: return "Старые сначала";
	case BkwDdnetVoteState::SORT_DIFFICULTY: return "Сложные сначала";
	case BkwDdnetVoteState::SORT_POINTS: return "Больше очков";
	default: return "По названию";
	}
}

inline std::vector<int> BkwDdnetFilteredMaps()
{
	BkwDdnetLoadFavorites();
	std::vector<int> vResult;
	const char *pSearch = BkwDdnetVoteState::SearchInput.GetString();
	vResult.reserve(BkwDdnetVoteState::vMaps.size());
	for(int i = 0; i < (int)BkwDdnetVoteState::vMaps.size(); ++i)
	{
		const auto &Map = BkwDdnetVoteState::vMaps[i];
		if(pSearch[0] && !str_utf8_find_nocase(Map.m_Name.c_str(), pSearch) && !str_utf8_find_nocase(Map.m_Type.c_str(), pSearch) && !str_utf8_find_nocase(Map.m_Mapper.c_str(), pSearch))
			continue;
		if(BkwDdnetVoteState::TypeFilter > 0 && str_comp_nocase(Map.m_Type.c_str(), BkwDdnetTypeFilterName(BkwDdnetVoteState::TypeFilter)) != 0)
			continue;
		if(BkwDdnetVoteState::DifficultyFilter > 0 && Map.m_Difficulty != BkwDdnetVoteState::DifficultyFilter)
			continue;
		if(BkwDdnetVoteState::FavoritesOnly && !BkwDdnetIsFavorite(Map.m_Name))
			continue;
		vResult.push_back(i);
	}

	std::stable_sort(vResult.begin(), vResult.end(), [](int A, int B) {
		const auto &MapA = BkwDdnetVoteState::vMaps[A];
		const auto &MapB = BkwDdnetVoteState::vMaps[B];
		switch(BkwDdnetVoteState::SortMode)
		{
		case BkwDdnetVoteState::SORT_RELEASE_NEWEST:
			if(MapA.m_Release != MapB.m_Release)
				return MapA.m_Release > MapB.m_Release;
			break;
		case BkwDdnetVoteState::SORT_RELEASE_OLDEST:
			if(MapA.m_Release != MapB.m_Release)
				return MapA.m_Release < MapB.m_Release;
			break;
		case BkwDdnetVoteState::SORT_DIFFICULTY:
			if(MapA.m_Difficulty != MapB.m_Difficulty)
				return MapA.m_Difficulty > MapB.m_Difficulty;
			break;
		case BkwDdnetVoteState::SORT_POINTS:
			if(MapA.m_Points != MapB.m_Points)
				return MapA.m_Points > MapB.m_Points;
			break;
		default:
			break;
		}
		return str_comp_nocase(MapA.m_Name.c_str(), MapB.m_Name.c_str()) < 0;
	});
	return vResult;
}

inline bool BkwDdnetFilterButton(CUi *pUi, CButtonContainer *pButton, const char *pText, CUIRect Rect, bool Interactive, bool Active = false)
{
	const bool Hovered = Interactive && pUi->MouseInside(&Rect);
	Rect.Draw(Active ? ColorRGBA(0.20f, 0.45f, 0.82f, Hovered ? 0.72f : 0.58f) : ColorRGBA(1.0f, 1.0f, 1.0f, Hovered ? 0.13f : 0.065f), IGraphics::CORNER_ALL, 6.0f);
	pUi->DoLabel(&Rect, pText, 8.8f, TEXTALIGN_MC, {.m_MaxWidth = Rect.w - 6.0f});
	return Interactive && pUi->DoButtonLogic(pButton, 0, &Rect, BUTTONFLAG_LEFT);
}

inline bool BkwDdnetFavoriteButton(CUi *pUi, CButtonContainer *pButton, CUIRect Rect, bool Interactive, bool Favorite)
{
	const bool Hovered = Interactive && pUi->MouseInside(&Rect);
	Rect.Draw(Favorite ? ColorRGBA(0.92f, 0.68f, 0.16f, Hovered ? 0.92f : 0.78f) : ColorRGBA(0.02f, 0.025f, 0.03f, Hovered ? 0.78f : 0.62f), IGraphics::CORNER_ALL, 6.0f);
	pUi->DoLabel(&Rect, Favorite ? "★" : "☆", 14.0f, TEXTALIGN_MC);
	return Interactive && pUi->DoButtonLogic(pButton, 0, &Rect, BUTTONFLAG_LEFT);
}

