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

		Content.HSplitTop(HoursHeight, &Section, &Content);
		s_MainScrollRegion.AddRect(Section);
		RenderBkwHoursSettings(pMenus, pUi, pGameClient, pClient, pHttp, Section);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(ExitHeight, &Section, &Content);
		s_MainScrollRegion.AddRect(Section);
		RenderLegacySection(LEGACY_EXIT, Section);
		s_MainScrollRegion.End();

		LegacyTab = LEGACY_SAVES;
	}
	else if(s_CurrentTab == TAB_PERSONALIZATION)
	{
		enum
		{
			PERSONALIZATION_SERVERS = 0,
			PERSONALIZATION_GAMEPLAY,
			PERSONALIZATION_VISUAL,
			PERSONALIZATION_SYSTEM,
			PERSONALIZATION_COUNT,
		};
		static int s_PersonalizationTab = PERSONALIZATION_SERVERS;
		static CButtonContainer s_aPersonalizationTabButtons[PERSONALIZATION_COUNT];

		CUIRect CategoryBar;
		PageView.HSplitTop(22.0f, &CategoryBar, &PageView);
		const char *apCategoryNames[PERSONALIZATION_COUNT] = {"Серверы", "Игровое", "Визуал", "Система"};
		const float aCategoryWidths[PERSONALIZATION_COUNT] = {88.0f, 92.0f, 82.0f, 88.0f};
		CUIRect Categories = CategoryBar;
		for(int Category = 0; Category < PERSONALIZATION_COUNT; ++Category)
		{
			CUIRect Button;
			Categories.VSplitLeft(minimum(aCategoryWidths[Category], Categories.w), &Button, &Categories);
			const int Corners = Category == 0 ? IGraphics::CORNER_L : (Category == PERSONALIZATION_COUNT - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE);
			if(pMenus->DoButton_MenuTab(&s_aPersonalizationTabButtons[Category], apCategoryNames[Category], s_PersonalizationTab == Category, &Button, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
				s_PersonalizationTab = Category;
		}
		PageView.HSplitTop(7.0f, nullptr, &PageView);

		constexpr float Gap = 8.0f;
		constexpr float CompactHeight = 36.0f;
		constexpr float TfMenuHeight = 280.0f;
		constexpr float TfDuelHudHeight = 405.0f;
		constexpr float TeeFusionHeight = TfMenuHeight + Gap + TfDuelHudHeight;
		constexpr float LocalDuelHeight = 735.0f;
		constexpr float DdnetMenuHeight = 280.0f;
		constexpr float EgoDetailsHeight = 135.0f;
		constexpr float OptimizerHeight = 245.0f;
		constexpr float TipoCheatHeight = 250.0f;
		constexpr float HookTargetHeight = 220.0f;
		constexpr float AntiPingHeight = 170.0f;
		constexpr float AnimatedBackgroundHeight = 510.0f;
		constexpr float CameraDriftHeight = 320.0f;
		constexpr float HudHeight = 800.0f;
		constexpr float BackgroundHeight = 470.0f;

		static SBkwCompactModuleState s_TeeFusionModule;
		static SBkwCompactModuleState s_LocalDuelModule;
		static SBkwCompactModuleState s_DdnetMenuModule;
		static SBkwCompactModuleState s_EgoDetailsModule;
		static SBkwCompactModuleState s_OptimizerModule;
		static SBkwCompactModuleState s_TipoCheatModule;
		static SBkwCompactModuleState s_HookTargetModule;
		static SBkwCompactModuleState s_AntiPingModule;
		static SBkwCompactModuleState s_AnimatedBackgroundModule;
		static SBkwCompactModuleState s_CameraDriftModule;
		static SBkwCompactModuleState s_HudModule;
		static SBkwCompactModuleState s_BackgroundModule;

		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollUnit = 46.0f;
		ScrollParams.m_ScrollbarThickness = 12.0f;
		CUIRect Content = PageView;
		s_PersonalizationScrollRegion.Begin(&Content, &ScrollParams);

		auto AddGap = [&]() {
			Content.HSplitTop(Gap, nullptr, &Content);
		};

		CUIRect Module;

		if(s_PersonalizationTab == PERSONALIZATION_SERVERS)
		{
			Content.HSplitTop(s_TeeFusionModule.m_Open ? TeeFusionHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_TeeFusionModule.m_Open)
			{
				CUIRect TfMenuSection, TfDuelSection;
				Module.HSplitTop(TfMenuHeight, &TfMenuSection, &Module);
				Module.HSplitTop(Gap, nullptr, &Module);
				Module.HSplitTop(TfDuelHudHeight, &TfDuelSection, &Module);
				RenderTfMenuSettings(pMenus, pUi, pGameClient, TfMenuSection);
				RenderTfDuelHudSettings(pMenus, pUi, TfDuelSection);
				RenderBkwCompactCollapseButton(pMenus, TfMenuSection, s_TeeFusionModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "TeeFusion", nullptr, s_TeeFusionModule);
			AddGap();

			Content.HSplitTop(s_DdnetMenuModule.m_Open ? DdnetMenuHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_DdnetMenuModule.m_Open)
			{
				RenderDdnetVoteMenuSettings(pMenus, pUi, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_DdnetMenuModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "DDNet Race Menu", &g_Config.m_BkwDdnetVoteMenu, s_DdnetMenuModule);
			AddGap();

			Content.HSplitTop(s_EgoDetailsModule.m_Open ? EgoDetailsHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_EgoDetailsModule.m_Open)
			{
				RenderBkwEgoMapDetailsSettings(pMenus, pUi, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_EgoDetailsModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "EGO • статистика карт", &g_Config.m_BkwEgoMapDetails, s_EgoDetailsModule);
		}
		else if(s_PersonalizationTab == PERSONALIZATION_GAMEPLAY)
		{
			Content.HSplitTop(s_LocalDuelModule.m_Open ? LocalDuelHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_LocalDuelModule.m_Open)
			{
				RenderBkwLocalDuelSettings(pMenus, pUi, pGameClient, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_LocalDuelModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "BKW Local Duel", &g_Config.m_BkwLocalDuel, s_LocalDuelModule);
			AddGap();

			Content.HSplitTop(s_HookTargetModule.m_Open ? HookTargetHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_HookTargetModule.m_Open)
			{
				RenderBkwHookTargetSettings(pMenus, pUi, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_HookTargetModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "Hook Target Indicator", &g_Config.m_BkwHookTargetIndicator, s_HookTargetModule);
			AddGap();

			Content.HSplitTop(s_AntiPingModule.m_Open ? AntiPingHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_AntiPingModule.m_Open)
			{
				RenderBkwAdaptiveAntiPingSettings(pMenus, pUi, pClient, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_AntiPingModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "BKW Adaptive AntiPing", &g_Config.m_BkwAdaptiveAntiPing, s_AntiPingModule);
			AddGap();

			Content.HSplitTop(s_TipoCheatModule.m_Open ? TipoCheatHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_TipoCheatModule.m_Open)
			{
				RenderBkwTipoCheatSettings(pMenus, pUi, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_TipoCheatModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "ТипоЧит", &g_Config.m_BkwTipoCheat, s_TipoCheatModule);
		}
		else if(s_PersonalizationTab == PERSONALIZATION_VISUAL)
		{
			Content.HSplitTop(s_CameraDriftModule.m_Open ? CameraDriftHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_CameraDriftModule.m_Open)
			{
				RenderCameraDriftSettings(pMenus, pUi, pGameClient, pClient, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_CameraDriftModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "Дрифт камеры", &g_Config.m_BcCameraDrift, s_CameraDriftModule);
			AddGap();

			Content.HSplitTop(s_HudModule.m_Open ? HudHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_HudModule.m_Open)
			{
				RenderLegacySection(LEGACY_HUD, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_HudModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "HUD", nullptr, s_HudModule);
			AddGap();

			Content.HSplitTop(s_BackgroundModule.m_Open ? BackgroundHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_BackgroundModule.m_Open)
			{
				RenderLegacySection(LEGACY_BACKGROUND, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_BackgroundModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "Background", nullptr, s_BackgroundModule);
		}
		else
		{
			Content.HSplitTop(s_OptimizerModule.m_Open ? OptimizerHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_OptimizerModule.m_Open)
			{
				RenderBkwOptimizerSettings(pMenus, pUi, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_OptimizerModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "BKW Оптимизация", &g_Config.m_BkwOptimizer, s_OptimizerModule);
			AddGap();

			Content.HSplitTop(s_AnimatedBackgroundModule.m_Open ? AnimatedBackgroundHeight : CompactHeight, &Module, &Content);
			s_PersonalizationScrollRegion.AddRect(Module);
			if(s_AnimatedBackgroundModule.m_Open)
			{
				RenderBkwAnimatedBackgroundSettings(pMenus, pUi, pGameClient, Module);
				RenderBkwCompactCollapseButton(pMenus, Module, s_AnimatedBackgroundModule);
			}
			else
				RenderBkwCompactModuleRow(pMenus, pUi, Module, "Animated Background / MP4 → .map", nullptr, s_AnimatedBackgroundModule);
		}

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

#define RenderBkwPage(BKW_VIEW) BkwMenuProxy::Render(this, Ui(), GameClient(), Client(), Input(), Http(), (BKW_VIEW), s_BkwTab, RenderBkwPage)
#include "menus_settings_legacy.inc"
#undef RenderBkwPage
