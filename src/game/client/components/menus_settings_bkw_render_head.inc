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

		static int s_Section = 0;
		static CButtonContainer s_aSectionButtons[6];
		const char *apSections[] = {"Сохранения", "Чекпоинты", "Игрок", "История кланов", "Игровое время", "Выход"};
		const int aSections[] = {LEGACY_SAVES, LEGACY_CHECKPOINTS, LEGACY_PLAYER, LEGACY_CLANS, LEGACY_HOURS, LEGACY_EXIT};
		const float aHeights[] = {0.0f, 245.0f, 250.0f, 250.0f, 260.0f, 390.0f};

		CUIRect Navigation, Details;
		PageView.VSplitLeft(minimum(160.0f, PageView.w * 0.25f), &Navigation, &Details);
		Details.VSplitLeft(12.0f, nullptr, &Details);
		Navigation.Draw(ColorRGBA(0.06f, 0.07f, 0.09f, 0.55f), IGraphics::CORNER_ALL, 10.0f);
		Details.Draw(ColorRGBA(0.08f, 0.09f, 0.12f, 0.42f), IGraphics::CORNER_ALL, 10.0f);
		Navigation.Margin(8.0f, &Navigation);
		Details.Margin(14.0f, &Details);
		CUIRect Caption;
		Navigation.HSplitTop(28.0f, &Caption, &Navigation);
		pUi->DoLabel(&Caption, "ОСНОВНОЕ", 12.0f, TEXTALIGN_ML);
		Navigation.HSplitTop(8.0f, nullptr, &Navigation);
		const float RowHeight = minimum(40.0f, maximum(24.0f, (Navigation.h - 30.0f) / 6.0f));
		for(int i = 0; i < 6; ++i)
		{
			CUIRect Button;
			Navigation.HSplitTop(RowHeight, &Button, &Navigation);
			Navigation.HSplitTop(5.0f, nullptr, &Navigation);
			if(pMenus->DoButton_Menu(&s_aSectionButtons[i], apSections[i], s_Section == i, &Button))
			{
				s_Section = i;
				s_MainScrollRegion.Reset();
				pUi->SetActiveItem(nullptr);
			}
		}

		if(s_Section == 0)
		{
			// Saves owns its list scrollbar; keep its header and action visible.
			RenderLegacySection(LEGACY_SAVES, Details);
		}
		else
		{
			CScrollRegionParams Params;
			Params.m_ScrollUnit = 40.0f;
			Params.m_ScrollbarThickness = 12.0f;
			s_MainScrollRegion.Begin(&Details, &Params);
			Details.h = maximum(Details.h, aHeights[s_Section]);
			s_MainScrollRegion.AddRect(Details);
			if(s_Section == 1)
				RenderCheckpoints(pMenus, pUi, pGameClient, pClient, Details);
			else if(s_Section == 4)
				RenderBkwHoursSettings(pMenus, pUi, pGameClient, pClient, pHttp, Details);
			else
				RenderLegacySection(aSections[s_Section], Details);
			s_MainScrollRegion.End();
		}
		LegacyTab = aSections[s_Section];
	}
