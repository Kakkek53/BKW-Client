	else if(s_CurrentTab == TAB_PERSONALIZATION)
	{
		// One Personalization page, no nested tabs and no +/- expand controls.
		// Disabled toggle-based modules use one compact row; enabling them reveals
		// their options immediately, matching BestClient's progressive settings.
		constexpr float Gap = 9.0f;
		constexpr float ColumnGap = 24.0f;
		constexpr float CompactHeight = 36.0f;
		constexpr float TfHeaderHeight = 56.0f;
		constexpr float TfMenuHeight = 280.0f;
		constexpr float TfDuelHudHeight = 405.0f;
		constexpr float LocalDuelHeight = 735.0f;
		constexpr float DdnetMenuHeight = 280.0f;
		constexpr float EgoDetailsHeight = 135.0f;
		constexpr float TipoCheatHeight = 250.0f;
		constexpr float HookTargetHeight = 220.0f;
		constexpr float MiniMapHeight = 300.0f;
		constexpr float CameraDriftHeight = 320.0f;

		static SBkwCompactToggleState s_LocalDuelToggle;
		static SBkwCompactToggleState s_DdnetMenuToggle;
		static SBkwCompactToggleState s_EgoDetailsToggle;
		static SBkwCompactToggleState s_TipoCheatToggle;
		static SBkwCompactToggleState s_HookTargetToggle;
		static SBkwCompactToggleState s_CameraDriftToggle;
		static SBkwCompactToggleState s_MiniMapToggle;

		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollUnit = 46.0f;
		ScrollParams.m_ScrollbarThickness = 12.0f;
		ScrollParams.m_ScrollbarMargin = 5.0f;
		CUIRect Content = PageView;
		s_PersonalizationScrollRegion.Begin(&Content, &ScrollParams);
		Content.VSplitRight(5.0f, &Content, nullptr);
		Content.VSplitLeft(5.0f, nullptr, &Content);
		CUIRect LeftColumn, RightColumn;
		Content.VSplitMid(&LeftColumn, &RightColumn, ColumnGap);

		auto AddGap = [&](CUIRect &Column) {
			Column.HSplitTop(Gap, nullptr, &Column);
		};
		auto AddRect = [&](CUIRect &Column, float Height, CUIRect &Rect) {
			Column.HSplitTop(Height, &Rect, &Column);
			s_PersonalizationScrollRegion.AddRect(Rect);
		};
		CUIRect Module;

		// ----- Left column -----
		AddRect(LeftColumn, 140.0f + (g_Config.m_BkwUiGlass && !pUi->Graphics()->IsQuadBufferingEnabled() ? 24.0f : 0.0f), Module);
		RenderBkwGlassSettings(pMenus, pUi, Module);
		AddGap(LeftColumn);

		float TeeFusionHeight = TfHeaderHeight;
		if(g_Config.m_BkwTfMenu)
			TeeFusionHeight += Gap + TfMenuHeight;
		if(g_Config.m_BkwTfDuelHud)
			TeeFusionHeight += Gap + TfDuelHudHeight;
		AddRect(LeftColumn, TeeFusionHeight, Module);
		CUIRect TeeFusionContent = Module, TfHeader;
		TeeFusionContent.HSplitTop(TfHeaderHeight, &TfHeader, &TeeFusionContent);
		RenderBkwTeeFusionHeader(pMenus, pUi, TfHeader);
		if(g_Config.m_BkwTfMenu)
		{
			TeeFusionContent.HSplitTop(Gap, nullptr, &TeeFusionContent);
			CUIRect TfMenuSection;
			TeeFusionContent.HSplitTop(TfMenuHeight, &TfMenuSection, &TeeFusionContent);
			RenderTfMenuSettings(pMenus, pUi, pGameClient, TfMenuSection);
		}
		if(g_Config.m_BkwTfDuelHud)
		{
			TeeFusionContent.HSplitTop(Gap, nullptr, &TeeFusionContent);
			CUIRect TfDuelSection;
			TeeFusionContent.HSplitTop(TfDuelHudHeight, &TfDuelSection, &TeeFusionContent);
			RenderTfDuelHudSettings(pMenus, pUi, TfDuelSection);
		}
		AddGap(LeftColumn);

		AddRect(LeftColumn, g_Config.m_BkwEgoMapDetails ? EgoDetailsHeight : CompactHeight, Module);
		if(g_Config.m_BkwEgoMapDetails)
			RenderBkwEgoMapDetailsSettings(pMenus, pUi, Module);
		else
			RenderBkwCompactToggleRow(pMenus, pUi, Module, "EGO • статистика карт", &g_Config.m_BkwEgoMapDetails, s_EgoDetailsToggle);
		AddGap(LeftColumn);

		AddRect(LeftColumn, g_Config.m_BkwHookTargetIndicator ? HookTargetHeight : CompactHeight, Module);
		if(g_Config.m_BkwHookTargetIndicator)
			RenderBkwHookTargetSettings(pMenus, pUi, Module);
		else
			RenderBkwCompactToggleRow(pMenus, pUi, Module, "Hook Target Indicator", &g_Config.m_BkwHookTargetIndicator, s_HookTargetToggle);
		AddGap(LeftColumn);

		AddRect(LeftColumn, g_Config.m_BkwMiniMap ? MiniMapHeight : CompactHeight, Module);
		if(g_Config.m_BkwMiniMap)
			RenderBkwMiniMapSettings(pMenus, pUi, pGameClient, Module);
		else
			RenderBkwCompactToggleRow(pMenus, pUi, Module, "BKW Mini Map", &g_Config.m_BkwMiniMap, s_MiniMapToggle);
		AddGap(LeftColumn);

		AddRect(LeftColumn, g_Config.m_BcCameraDrift ? CameraDriftHeight : CompactHeight, Module);
		if(g_Config.m_BcCameraDrift)
			RenderCameraDriftSettings(pMenus, pUi, pGameClient, pClient, Module);
		else
			RenderBkwCompactToggleRow(pMenus, pUi, Module, "Дрифт камеры", &g_Config.m_BcCameraDrift, s_CameraDriftToggle);
		AddGap(LeftColumn);


		// ----- Right column -----
		AddRect(RightColumn, g_Config.m_BkwDdnetVoteMenu ? DdnetMenuHeight : CompactHeight, Module);
		if(g_Config.m_BkwDdnetVoteMenu)
			RenderDdnetVoteMenuSettings(pMenus, pUi, Module);
		else
			RenderBkwCompactToggleRow(pMenus, pUi, Module, "DDNet Race Menu", &g_Config.m_BkwDdnetVoteMenu, s_DdnetMenuToggle);
		AddGap(RightColumn);

		AddRect(RightColumn, g_Config.m_BkwLocalDuel ? LocalDuelHeight : CompactHeight, Module);
		if(g_Config.m_BkwLocalDuel)
			RenderBkwLocalDuelSettings(pMenus, pUi, pGameClient, Module);
		else
			RenderBkwCompactToggleRow(pMenus, pUi, Module, "BKW Local Duel", &g_Config.m_BkwLocalDuel, s_LocalDuelToggle);
		AddGap(RightColumn);


		AddRect(RightColumn, g_Config.m_BkwTipoCheat ? TipoCheatHeight : CompactHeight, Module);
		if(g_Config.m_BkwTipoCheat)
			RenderBkwTipoCheatSettings(pMenus, pUi, Module);
		else
			RenderBkwCompactToggleRow(pMenus, pUi, Module, "ТипоЧит", &g_Config.m_BkwTipoCheat, s_TipoCheatToggle);
		AddGap(RightColumn);


		s_PersonalizationScrollRegion.End();
		LegacyTab = LEGACY_HUD;
	}
	else if(s_CurrentTab == TAB_ACCOUNT)
	{
		RenderBkwAccountSettings(pMenus, pUi, pGameClient, pClient, pInput, pHttp, PageView);
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

#define RenderBkwPage(BKW_VIEW) BkwMenuProxy::Render(this, Ui(), GameClient(), Client(), Input(), Http(), (BKW_VIEW), s_BkwTab, RenderBkwPage)
#include "menus_settings_legacy.inc"
#undef RenderBkwPage
