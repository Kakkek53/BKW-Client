template<typename TLegacyRenderer>
void Render(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, IClient *pClient, IInput *pInput, IHttp *pHttp, CUIRect PageView, int &LegacyTab, TLegacyRenderer &LegacyRenderer)
{
	static int s_CurrentTab = TAB_MAIN;
	static CButtonContainer s_aTabButtons[TAB_COUNT];
	static CScrollRegion s_MainScrollRegion;
	static CScrollRegion s_PersonalizationScrollRegion;

	s_CurrentTab = std::clamp(s_CurrentTab, 0, static_cast<int>(TAB_COUNT) - 1);
	UpdateTfParserTest(pInput, pGameClient);
	const CUIRect OverlayAnchor = PageView;

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	CUIRect TabBar;
	PageView.HSplitTop(22.0f, &TabBar, &PageView);
	const char *apTabs[TAB_COUNT] = {"Основное", "Персонализация", "Магазин"};
	const float aTabWidths[TAB_COUNT] = {96.0f, 136.0f, 92.0f};

	CUIRect CompactTabs = TabBar;
	for(int Tab = 0; Tab < TAB_COUNT; ++Tab)
	{
		CUIRect Button;
		CompactTabs.VSplitLeft(minimum(aTabWidths[Tab], CompactTabs.w), &Button, &CompactTabs);
		const int Corners = Tab == 0 ? IGraphics::CORNER_L : (Tab == TAB_COUNT - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE);
		if(pMenus->DoButton_MenuTab(&s_aTabButtons[Tab], apTabs[Tab], s_CurrentTab == Tab, &Button, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
			s_CurrentTab = Tab;
	}
	PageView.HSplitTop(8.0f, nullptr, &PageView);

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
		constexpr float Gap = 18.0f;
		constexpr float SavesHeight = 1050.0f;
		constexpr float CheckpointsHeight = 245.0f;
		constexpr float PlayerHeight = 230.0f;
		constexpr float ClansHeight = 230.0f;
		constexpr float HoursHeight = 230.0f;
		constexpr float ExitHeight = 390.0f;

		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollUnit = 64.0f;
		ScrollParams.m_ScrollbarThickness = 14.0f;
		CUIRect Content = PageView;
