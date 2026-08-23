inline void BkwDdnetRenderMapCards(CUi *pUi, CGameClient *pGameClient, CUIRect Body, bool Interactive)
{
	if(!BkwDdnetVoteState::CatalogReady)
	{
		const char *pText = BkwDdnetVoteState::CatalogFailed ? "Не удалось загрузить каталог карт DDNet." : "Загружаю каталог карт DDNet…";
		pUi->DoLabel(&Body, pText, 12.0f, TEXTALIGN_MC);
		return;
	}
	BkwDdnetLoadFavorites();

	CUIRect Search;
	Body.HSplitTop(29.0f, &Search, &Body);
	Search.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 7.0f);
	Search.Margin(5.0f, &Search);
	BkwDdnetVoteState::SearchInput.SetEmptyText("Поиск по названию, типу или автору…");
	if(Interactive)
		pUi->DoEditBox(&BkwDdnetVoteState::SearchInput, &Search, 10.2f);
	else
		pUi->DoLabel(&Search, BkwDdnetVoteState::SearchInput.IsEmpty() ? "Поиск по названию, типу или автору…" : BkwDdnetVoteState::SearchInput.GetString(), 10.2f, TEXTALIGN_ML, {.m_MaxWidth = Search.w});
	Body.HSplitTop(6.0f, nullptr, &Body);

	CUIRect Filters;
	Body.HSplitTop(27.0f, &Filters, &Body);
	static CButtonContainer s_TypeFilterButton;
	static CButtonContainer s_DifficultyFilterButton;
	static CButtonContainer s_SortButton;
	static CButtonContainer s_FavoritesFilterButton;
	static CButtonContainer s_ResetFiltersButton;

	CUIRect TypeButton, DifficultyButton, SortButton, FavoritesButton, ResetButton;
	Filters.VSplitLeft(142.0f, &TypeButton, &Filters);
	Filters.VSplitLeft(5.0f, nullptr, &Filters);
	Filters.VSplitLeft(112.0f, &DifficultyButton, &Filters);
	Filters.VSplitLeft(5.0f, nullptr, &Filters);
	Filters.VSplitLeft(168.0f, &SortButton, &Filters);
	Filters.VSplitLeft(5.0f, nullptr, &Filters);
	Filters.VSplitLeft(125.0f, &FavoritesButton, &Filters);
	Filters.VSplitLeft(5.0f, nullptr, &Filters);
	ResetButton = Filters;

	char aType[96];
	str_format(aType, sizeof(aType), "Тип: %s", BkwDdnetTypeFilterName(BkwDdnetVoteState::TypeFilter));
	if(BkwDdnetFilterButton(pUi, &s_TypeFilterButton, aType, TypeButton, Interactive, BkwDdnetVoteState::TypeFilter != 0))
	{
		BkwDdnetVoteState::TypeFilter = (BkwDdnetVoteState::TypeFilter + 1) % BkwDdnetTypeFilterCount();
		BkwDdnetVoteState::ScrollRow = 0;
	}

	char aDifficulty[64];
	if(BkwDdnetVoteState::DifficultyFilter == 0)
		str_copy(aDifficulty, "★: любая");
	else
		str_format(aDifficulty, sizeof(aDifficulty), "★: %d", BkwDdnetVoteState::DifficultyFilter);
	if(BkwDdnetFilterButton(pUi, &s_DifficultyFilterButton, aDifficulty, DifficultyButton, Interactive, BkwDdnetVoteState::DifficultyFilter != 0))
	{
		BkwDdnetVoteState::DifficultyFilter = (BkwDdnetVoteState::DifficultyFilter + 1) % 6;
		BkwDdnetVoteState::ScrollRow = 0;
	}

	char aSort[96];
	str_format(aSort, sizeof(aSort), "Сорт.: %s", BkwDdnetSortName(BkwDdnetVoteState::SortMode));
	if(BkwDdnetFilterButton(pUi, &s_SortButton, aSort, SortButton, Interactive, BkwDdnetVoteState::SortMode != BkwDdnetVoteState::SORT_NAME))
	{
		BkwDdnetVoteState::SortMode = (BkwDdnetVoteState::SortMode + 1) % BkwDdnetVoteState::NUM_SORTS;
		BkwDdnetVoteState::ScrollRow = 0;
	}

	char aFavorites[80];
	str_format(aFavorites, sizeof(aFavorites), "★ Избранные (%d)", (int)BkwDdnetVoteState::vFavoriteMaps.size());
	if(BkwDdnetFilterButton(pUi, &s_FavoritesFilterButton, aFavorites, FavoritesButton, Interactive, BkwDdnetVoteState::FavoritesOnly))
	{
		BkwDdnetVoteState::FavoritesOnly = !BkwDdnetVoteState::FavoritesOnly;
		BkwDdnetVoteState::ScrollRow = 0;
	}

	if(BkwDdnetFilterButton(pUi, &s_ResetFiltersButton, "Сброс", ResetButton, Interactive))
	{
		BkwDdnetVoteState::TypeFilter = 0;
		BkwDdnetVoteState::DifficultyFilter = 0;
		BkwDdnetVoteState::SortMode = BkwDdnetVoteState::SORT_NAME;
		BkwDdnetVoteState::FavoritesOnly = false;
		BkwDdnetVoteState::ScrollRow = 0;
	}
	Body.HSplitTop(7.0f, nullptr, &Body);

	const std::vector<int> vFiltered = BkwDdnetFilteredMaps();
	if(vFiltered.empty())
	{
		pUi->DoLabel(&Body, BkwDdnetVoteState::FavoritesOnly ? "В избранном нет карт с такими фильтрами." : "Карты по этим условиям не найдены.", 11.5f, TEXTALIGN_MC);
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
	static CButtonContainer s_aFavoriteButtons[Columns * VisibleRows];

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
			Card.HSplitTop(Card.h * 0.43f, &Image, &Card);
			Card.HSplitBottom(27.0f, &Info, &Buttons);
			Image.Draw(ColorRGBA(0.02f, 0.022f, 0.027f, 0.95f), IGraphics::CORNER_ALL, 6.0f);
			if(MapIndex < (int)BkwDdnetVoteState::vThumbnails.size() && BkwDdnetVoteState::vThumbnails[MapIndex].m_Texture.IsValid())
				BkwDdnetRenderTextureCover(pUi, BkwDdnetVoteState::vThumbnails[MapIndex].m_Texture, Image, 360.0f / 225.0f);
			else
				pUi->DoLabel(&Image, "DDNet", 12.0f, TEXTALIGN_MC);

			CUIRect FavoriteRect{Image.x + Image.w - 27.0f, Image.y + 4.0f, 23.0f, 23.0f};
			const bool Favorite = BkwDdnetIsFavorite(Map.m_Name);
			if(BkwDdnetFavoriteButton(pUi, &s_aFavoriteButtons[Slot], FavoriteRect, Interactive, Favorite))
			{
				BkwDdnetToggleFavorite(Map.m_Name);
				if(BkwDdnetVoteState::FavoritesOnly)
					BkwDdnetVoteState::ScrollRow = 0;
			}

			Info.HSplitTop(3.0f, nullptr, &Info);
			CUIRect Name, Meta, Mapper, Release;
			Info.HSplitTop(17.0f, &Name, &Info);
			Info.HSplitTop(14.0f, &Meta, &Info);
			Info.HSplitTop(14.0f, &Mapper, &Info);
			Info.HSplitTop(14.0f, &Release, &Info);
			pUi->DoLabel(&Name, Map.m_Name.c_str(), 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Name.w});

			char aMeta[128];
			str_format(aMeta, sizeof(aMeta), "%s   %s   •   %d очк.", Map.m_Type.c_str(), BkwDdnetStars(Map.m_Difficulty).c_str(), Map.m_Points);
			pUi->DoLabel(&Meta, aMeta, 8.2f, TEXTALIGN_ML, {.m_MaxWidth = Meta.w});

			char aMapper[192];
			str_format(aMapper, sizeof(aMapper), "Автор: %s", Map.m_Mapper.c_str());
			pUi->DoLabel(&Mapper, aMapper, 8.0f, TEXTALIGN_ML, {.m_MaxWidth = Mapper.w});

			char aRelease[160];
			str_format(aRelease, sizeof(aRelease), "Релиз: %s", Map.m_Release.empty() ? "—" : Map.m_Release.c_str());
			pUi->DoLabel(&Release, aRelease, 7.8f, TEXTALIGN_ML, {.m_MaxWidth = Release.w});

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
	static CButtonContainer s_FavoriteButton;
	CUIRect BackRow;
	Body.HSplitTop(30.0f, &BackRow, &Body);
	CUIRect Back, Favorite, Spacer;
	BackRow.VSplitLeft(125.0f, &Back, &Spacer);
	Spacer.VSplitLeft(8.0f, nullptr, &Spacer);
	Spacer.VSplitLeft(145.0f, &Favorite, &Spacer);
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
	if(BkwDdnetSmallButton(pUi, &s_FavoriteButton, BkwDdnetIsFavorite(Map.m_Name) ? "★ В избранном" : "☆ В избранное", Favorite, Interactive))
		BkwDdnetToggleFavorite(Map.m_Name);

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

	// DDNet thumbnails are 360x225. Give the detail image an actual 16:10-ish
	// rectangle instead of stretching it to the full height of the left column.
	const float ImageAspect = 360.0f / 225.0f;
	const float ImageHeight = minimum(Left.w / ImageAspect, maximum(80.0f, Left.h - 46.0f));
	CUIRect Image;
	Left.HSplitTop(ImageHeight, &Image, &Left);
	Image.Draw(ColorRGBA(0.02f, 0.022f, 0.027f, 0.86f), IGraphics::CORNER_ALL, 7.0f);
	if(MapIndex < (int)BkwDdnetVoteState::vThumbnails.size() && BkwDdnetVoteState::vThumbnails[MapIndex].m_Texture.IsValid())
		BkwDdnetRenderTextureCover(pUi, BkwDdnetVoteState::vThumbnails[MapIndex].m_Texture, Image, ImageAspect);
	else
		pUi->DoLabel(&Image, "DDNet", 16.0f, TEXTALIGN_MC);

	Left.HSplitTop(8.0f, nullptr, &Left);
	CUIRect Vote;
	Left.HSplitTop(30.0f, &Vote, &Left);
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
