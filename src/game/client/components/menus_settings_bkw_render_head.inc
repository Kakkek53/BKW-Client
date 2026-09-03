void RenderBkwUpdateSettings(CMenus *pMenus, CUi *pUi, CUIRect View)
{
	View.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	View.Margin(10.0f, &View);
	CUIRect Row;
	View.HSplitTop(24.0f, &Row, &View);
	pUi->DoLabel(&Row, BKW_VERSION_LABEL, 16.0f, TEXTALIGN_ML);
	static int s_aChannels[2];
	View.HSplitTop(28.0f, &Row, &View);
	CUIRect Beta, Release;
	Row.VSplitMid(&Beta, &Release, 8.0f);
	if(pMenus->DoButton_CheckBox(&s_aChannels[0], "Только Beta", g_Config.m_BkwUpdateChannel == 0, &Beta))
		g_Config.m_BkwUpdateChannel = 0;
	if(pMenus->DoButton_CheckBox(&s_aChannels[1], "Только Release", g_Config.m_BkwUpdateChannel == 1, &Release))
		g_Config.m_BkwUpdateChannel = 1;
	View.HSplitTop(24.0f, &Row, &View);
	static int s_AutoDownload;
	if(pMenus->DoButton_CheckBox(&s_AutoDownload, "Скачивать обновления автоматически", g_Config.m_BcAutoUpdate, &Row))
		g_Config.m_BcAutoUpdate ^= 1;
#if defined(CONF_AUTOUPDATE)
	IUpdater *pUpdater = pMenus->BkwUpdater();
	char aStatus[256];
	pUpdater->GetCurrentFile(aStatus, sizeof(aStatus));
	View.HSplitTop(24.0f, &Row, &View);
	pUi->DoLabel(&Row, aStatus, 11.0f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
	View.HSplitTop(26.0f, &Row, &View);
	static CButtonContainer s_Check, s_Action;
	CUIRect Check, Action;
	Row.VSplitMid(&Check, &Action, 8.0f);
	const auto State = pUpdater->GetCurrentState();
	if(State != IUpdater::GETTING_MANIFEST && State != IUpdater::DOWNLOADING)
	{
		if(pMenus->DoButton_Menu(&s_Check, "Проверить", 0, &Check))
			pUpdater->CheckForUpdate();
		if(State == IUpdater::VERSION_AVAILABLE && pMenus->DoButton_Menu(&s_Action, "Скачать", 0, &Action))
			pUpdater->InitiateUpdate();
		else if(State == IUpdater::NEED_RESTART && pMenus->DoButton_Menu(&s_Action, "Установить", 0, &Action))
			pUpdater->ApplyUpdateAndRestart();
	}
	else
	{
		char aProgress[64];
		str_format(aProgress, sizeof(aProgress), State == IUpdater::DOWNLOADING ? "Загрузка: %d%%" : "Проверка…", pUpdater->GetCurrentPercent());
		pUi->DoLabel(&Row, aProgress, 12.0f, TEXTALIGN_ML);
	}
#else
	View.HSplitTop(28.0f, &Row, &View);
	pUi->DoLabel(&Row, "Обновление вручную: github.com/Kakkek53/test", 11.0f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
#endif
}

void RenderBkwGlassSettings(CMenus *pMenus, CUi *pUi, CUIRect View)
{
	View.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	View.Margin(10.0f, &View);
	CUIRect Row;
	View.HSplitTop(24.0f, &Row, &View);
	if(pMenus->DoButton_CheckBox(&g_Config.m_BkwUiGlass, Localize("Liquid glass"), g_Config.m_BkwUiGlass, &Row))
		g_Config.m_BkwUiGlass ^= 1;
	View.HSplitTop(6.0f, nullptr, &View);
	View.HSplitTop(24.0f, &Row, &View);
	pUi->DoScrollbarOption(&g_Config.m_BkwUiGlassTransparency, &g_Config.m_BkwUiGlassTransparency, &Row, Localize("Transparency"), 0, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
	View.HSplitTop(6.0f, nullptr, &View);
	View.HSplitTop(24.0f, &Row, &View);
	pUi->DoScrollbarOption(&g_Config.m_BkwUiGlassBlur, &g_Config.m_BkwUiGlassBlur, &Row, Localize("Blur"), 0, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
	View.HSplitTop(6.0f, nullptr, &View);
	static CButtonContainer s_GlassColorReset;
	pMenus->DoLine_ColorPicker(&s_GlassColorReset, 24.0f, 13.0f, 0.0f, &View, Localize("Glass color"), &g_Config.m_BkwUiGlassColor, color_cast<ColorRGBA>(ColorHSLA(0x000020U)), false);
	if(g_Config.m_BkwUiGlass && !pUi->Graphics()->IsQuadBufferingEnabled())
	{
		View.HSplitTop(6.0f, nullptr, &View);
		View.HSplitTop(18.0f, &Row, &View);
		pUi->DoLabel(&Row, Localize("Blur requires OpenGL 3.3+ or Vulkan"), 11.0f, TEXTALIGN_ML);
	}
}

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
	const char *apTabs[TAB_COUNT] = {"Основное", "Персонализация", "Аккаунт", "Магазин"};
	const float aTabWidths[TAB_COUNT] = {96.0f, 136.0f, 94.0f, 92.0f};

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
		// Match Personalization: one scrollable page of grouped cards, no sidebar.
		CScrollRegionParams Params;
		Params.m_ScrollUnit = 46.0f;
		Params.m_ScrollbarThickness = 12.0f;
		Params.m_ScrollbarMargin = 5.0f;
		CUIRect Content = PageView;
		s_MainScrollRegion.Begin(&Content, &Params);
		Content.VMargin(5.0f, &Content);
		const bool TwoColumns = Content.w >= 620.0f;
		CUIRect Left = Content, Right = Content;
		if(TwoColumns)
			Content.VSplitMid(&Left, &Right, 18.0f);
		auto Card = [&](CUIRect &Column, float Height, int SectionId) {
			CUIRect Section;
			Column.HSplitTop(Height, &Section, &Column);
			Column.HSplitTop(10.0f, nullptr, &Column);
			s_MainScrollRegion.AddRect(Section);
			if(SectionId == -1)
			{
				RenderBkwUpdateSettings(pMenus, pUi, Section);
				return;
			}
			Section.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
			Section.Margin(12.0f, &Section);
			if(SectionId == LEGACY_CHECKPOINTS)
				RenderCheckpoints(pMenus, pUi, pGameClient, pClient, Section);
			else if(SectionId == LEGACY_HOURS)
				RenderBkwHoursSettings(pMenus, pUi, pGameClient, pClient, pHttp, Section);
			else
				RenderLegacySection(SectionId, Section);
		};

		Card(Left, 155.0f, -1);
		Card(TwoColumns ? Right : Left, 280.0f, LEGACY_CHECKPOINTS);
		Card(Left, 260.0f, LEGACY_PLAYER);
		Card(TwoColumns ? Right : Left, 260.0f, LEGACY_CLANS);
		Card(Left, 280.0f, LEGACY_HOURS);
		Card(TwoColumns ? Right : Left, 420.0f, LEGACY_EXIT);

		// Give saved teams the full page width: status and action rows need room.
		CUIRect Saves = Content;
		Saves.y = TwoColumns ? maximum(Left.y, Right.y) : Left.y;
		Card(Saves, 440.0f, LEGACY_SAVES);
		s_MainScrollRegion.End();
		LegacyTab = LEGACY_SAVES;
	}
