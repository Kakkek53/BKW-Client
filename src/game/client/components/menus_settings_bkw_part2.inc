	PageView.HSplitTop(g_Config.m_BcCameraDrift ? 258.0f : 108.0f, &Card, &PageView);
	Card.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.48f), IGraphics::CORNER_ALL, 8.0f);
	Card.Margin(10.0f, &Card);

	CUIRect TitleRow, Title, Reset;
	Card.HSplitTop(28.0f, &TitleRow, &Card);
	TitleRow.VSplitRight(92.0f, &Title, &Reset);
	pUi->DoLabel(&Title, "Дрифт камеры", 18.0f, TEXTALIGN_ML);
	if(pMenus->DoButton_Menu(&s_CameraDriftResetButton, "Сбросить", 0, &Reset))
	{
		g_Config.m_BcCameraDriftAmount = 50;
		g_Config.m_BcCameraDriftSmoothness = 20;
		g_Config.m_BcCameraDriftReverse = 0;
	}

	Card.HSplitTop(6.0f, nullptr, &Card);
	CUIRect Toggle;
	Card.HSplitTop(26.0f, &Toggle, &Card);
	if(pMenus->DoButton_CheckBox(&s_CameraDriftToggleId, "Дрифт камеры", g_Config.m_BcCameraDrift, &Toggle))
		g_Config.m_BcCameraDrift ^= 1;

	if(g_Config.m_BcCameraDrift)
	{
		Card.HSplitTop(10.0f, nullptr, &Card);
		CUIRect Amount;
		Card.HSplitTop(28.0f, &Amount, &Card);
		pUi->DoScrollbarOption(&g_Config.m_BcCameraDriftAmount, &g_Config.m_BcCameraDriftAmount, &Amount, "Сила дрифта камеры", 1, 200);

		Card.HSplitTop(8.0f, nullptr, &Card);
		CUIRect Smoothness;
		Card.HSplitTop(28.0f, &Smoothness, &Card);
		pUi->DoScrollbarOption(&g_Config.m_BcCameraDriftSmoothness, &g_Config.m_BcCameraDriftSmoothness, &Smoothness, "Плавность дрифта камеры", 1, 20);

		Card.HSplitTop(10.0f, nullptr, &Card);
		CUIRect DirectionLabel;
		Card.HSplitTop(20.0f, &DirectionLabel, &Card);
		pUi->DoLabel(&DirectionLabel, "Направление дрифта", 13.0f, TEXTALIGN_ML);
		Card.HSplitTop(4.0f, nullptr, &Card);
		CUIRect DirectionButtons, Forward, Backward;
		Card.HSplitTop(30.0f, &DirectionButtons, &Card);
		DirectionButtons.VSplitMid(&Forward, &Backward, 6.0f);
		if(pMenus->DoButton_Menu(&s_CameraDriftForwardButton, "Вперёд", !g_Config.m_BcCameraDriftReverse, &Forward))
			g_Config.m_BcCameraDriftReverse = 0;
		if(pMenus->DoButton_Menu(&s_CameraDriftBackwardButton, "Назад", g_Config.m_BcCameraDriftReverse, &Backward))
			g_Config.m_BcCameraDriftReverse = 1;
	}

	if(Blocked)
	{
		PageView.HSplitTop(10.0f, nullptr, &PageView);
		CUIRect Warning;
		PageView.HSplitTop(42.0f, &Warning, &PageView);
		Warning.Draw(ColorRGBA(0.35f, 0.08f, 0.08f, 0.45f), IGraphics::CORNER_ALL, 6.0f);
		Warning.Margin(8.0f, &Warning);
		pUi->DoLabel(&Warning, "На этом сервере дрифт камеры отключён (FNG / 0xf).", 12.0f, TEXTALIGN_ML);
	}
}

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
		s_MainScrollRegion.Begin(&Content, &ScrollParams);

		CUIRect Section;
		Content.HSplitTop(SavesHeight, &Section, &Content);
		s_MainScrollRegion.AddRect(Section);
		RenderLegacySection(LEGACY_SAVES, Section);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(CheckpointsHeight, &Section, &Content);
		s_MainScrollRegion.AddRect(Section);
		RenderCheckpoints(pMenus, pUi, pGameClient, pClient, Section);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(PlayerHeight, &Section, &Content);
		s_MainScrollRegion.AddRect(Section);
		RenderLegacySection(LEGACY_PLAYER, Section);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(ClansHeight, &Section, &Content);
		s_MainScrollRegion.AddRect(Section);
		RenderLegacySection(LEGACY_CLANS, Section);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(ExitHeight, &Section, &Content);
		s_MainScrollRegion.AddRect(Section);
		RenderLegacySection(LEGACY_EXIT, Section);
		s_MainScrollRegion.End();

		LegacyTab = LEGACY_SAVES;
	}
	else if(s_CurrentTab == TAB_HOURS)
	{
		RenderLegacySection(LEGACY_HOURS, PageView);
		LegacyTab = LEGACY_HOURS;
	}
	else if(s_CurrentTab == TAB_PERSONALIZATION)
	{
		constexpr float Gap = 22.0f;
		constexpr float TfMenuHeight = 104.0f;
		constexpr float TfDuelHudHeight = 405.0f;
		constexpr float DdnetMenuHeight = 104.0f;
		constexpr float CameraDriftHeight = 320.0f;
		constexpr float HudHeight = 800.0f;
		constexpr float BackgroundHeight = 470.0f;

		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollUnit = 70.0f;
		ScrollParams.m_ScrollbarThickness = 14.0f;
		CUIRect Content = PageView;
		s_PersonalizationScrollRegion.Begin(&Content, &ScrollParams);

		CUIRect TfMenuSection, TfDuelHudSection, DdnetMenuSection, CameraDriftSection, HudSection, BackgroundSection;
		Content.HSplitTop(TfMenuHeight, &TfMenuSection, &Content);
		s_PersonalizationScrollRegion.AddRect(TfMenuSection);
		RenderTfMenuSettings(pMenus, pUi, pGameClient, TfMenuSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(TfDuelHudHeight, &TfDuelHudSection, &Content);
		s_PersonalizationScrollRegion.AddRect(TfDuelHudSection);
		RenderTfDuelHudSettings(pMenus, pUi, TfDuelHudSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(DdnetMenuHeight, &DdnetMenuSection, &Content);
		s_PersonalizationScrollRegion.AddRect(DdnetMenuSection);
		RenderDdnetVoteMenuSettings(pMenus, pUi, DdnetMenuSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(CameraDriftHeight, &CameraDriftSection, &Content);
		s_PersonalizationScrollRegion.AddRect(CameraDriftSection);
		RenderCameraDriftSettings(pMenus, pUi, pGameClient, pClient, CameraDriftSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(HudHeight, &HudSection, &Content);
		s_PersonalizationScrollRegion.AddRect(HudSection);
		RenderLegacySection(LEGACY_HUD, HudSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(BackgroundHeight, &BackgroundSection, &Content);
		s_PersonalizationScrollRegion.AddRect(BackgroundSection);
		RenderLegacySection(LEGACY_BACKGROUND, BackgroundSection);
		s_PersonalizationScrollRegion.End();

		LegacyTab = LEGACY_HUD;
	}
	else
	{
		RenderLegacySection(LEGACY_SHOP, PageView);
		LegacyTab = LEGACY_SHOP;
	}

	RenderTfParserTestBanner(pUi, OverlayAnchor);
}
} // namespace BkwMenuProxy

#define RenderBkwPage(BKW_VIEW) BkwMenuProxy::Render(this, Ui(), GameClient(), Client(), Input(), (BKW_VIEW), s_BkwTab, RenderBkwPage)
#include "menus_settings_legacy.inc"
#undef RenderBkwPage
