template<typename TLegacyRenderer>
void Render(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, IClient *pClient, IInput *pInput, CUIRect PageView, int &LegacyTab, TLegacyRenderer &LegacyRenderer)
{
	static int s_CurrentTab = TAB_MAIN;
	static int s_FirstVisibleTab = 0;
	static CButtonContainer s_aTabButtons[TAB_COUNT];
	static CButtonContainer s_PreviousButton;
	static CButtonContainer s_NextButton;
	static CScrollRegion s_MainScrollRegion;
	static CScrollRegion s_PersonalizationScrollRegion;

	s_CurrentTab = std::clamp(s_CurrentTab, 0, static_cast<int>(TAB_COUNT) - 1);
	UpdateTfParserTest(pInput, pGameClient);
	const CUIRect OverlayAnchor = PageView;

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect TabBar;
	PageView.HSplitTop(24.0f, &TabBar, &PageView);
	const char *apTabs[TAB_COUNT] = {"Основное", "Часы", "Персонализация", "Магазин"};

	constexpr float MinTabWidth = 128.0f;
	constexpr float PagerWidth = 28.0f;
	int VisibleCount = std::clamp((int)(TabBar.w / MinTabWidth), 1, static_cast<int>(TAB_COUNT));
	bool Scrollable = VisibleCount < TAB_COUNT;
	if(Scrollable)
		VisibleCount = std::clamp((int)((TabBar.w - PagerWidth * 2.0f) / MinTabWidth), 1, static_cast<int>(TAB_COUNT));

	const int MaxFirst = std::max(0, static_cast<int>(TAB_COUNT) - VisibleCount);
	s_FirstVisibleTab = std::clamp(s_FirstVisibleTab, 0, MaxFirst);

	CUIRect VisibleTabs = TabBar;
	if(Scrollable)
	{
		CUIRect Previous, Next;
		VisibleTabs.VSplitLeft(PagerWidth, &Previous, &VisibleTabs);
		VisibleTabs.VSplitRight(PagerWidth, &VisibleTabs, &Next);
		if(pMenus->DoButton_Menu(&s_PreviousButton, "<", 0, &Previous) && s_FirstVisibleTab > 0)
			--s_FirstVisibleTab;
		if(pMenus->DoButton_Menu(&s_NextButton, ">", 0, &Next) && s_FirstVisibleTab < MaxFirst)
			++s_FirstVisibleTab;

		if(pUi->MouseHovered(&TabBar))
		{
			if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_UP) && s_FirstVisibleTab > 0)
				--s_FirstVisibleTab;
			if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_DOWN) && s_FirstVisibleTab < MaxFirst)
				++s_FirstVisibleTab;
		}
	}

	const float TabWidth = VisibleTabs.w / (float)VisibleCount;
	for(int VisibleIndex = 0; VisibleIndex < VisibleCount; ++VisibleIndex)
	{
		const int Tab = s_FirstVisibleTab + VisibleIndex;
		CUIRect Button;
		VisibleTabs.VSplitLeft(TabWidth, &Button, &VisibleTabs);
		const int Corners = VisibleIndex == 0 ? IGraphics::CORNER_L : (VisibleIndex == VisibleCount - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE);
		if(pMenus->DoButton_MenuTab(&s_aTabButtons[Tab], apTabs[Tab], s_CurrentTab == Tab, &Button, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
		{
			s_CurrentTab = Tab;
			if(s_CurrentTab < s_FirstVisibleTab)
				s_FirstVisibleTab = s_CurrentTab;
			else if(s_CurrentTab >= s_FirstVisibleTab + VisibleCount)
				s_FirstVisibleTab = s_CurrentTab - VisibleCount + 1;
		}
	}
	PageView.HSplitTop(10.0f, nullptr, &PageView);

	auto RenderLegacySection = [&](int Tab, CUIRect Section) {
		LegacyTab = Tab;
		CUIRect LegacyView = Section;
		LegacyView.y -= 44.0f;
		LegacyView.h += 44.0f;
		pUi->ClipEnable(&Section);
		LegacyRenderer(LegacyView);
		pUi->ClipDisable();
	};

	if(s_CurrentTab == TAB_MAIN)
	{
		constexpr float Gap = 22.0f;
		constexpr float SavesHeight = 1050.0f;
		constexpr float CheckpointsHeight = 245.0f;
		constexpr float PlayerHeight = 230.0f;
		constexpr float ClansHeight = 230.0f;
		constexpr float ExitHeight = 390.0f;

		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollUnit = 70.0f;
		ScrollParams.m_ScrollbarThickness = 14.0f;
		CUIRect Content = PageView;
