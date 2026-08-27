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
		// BestClient-style compact module list. Every feature occupies one short row
		// by default. The checkbox changes the feature state and enabling it opens the
		// details immediately; the + / - control can reveal or fold settings without
		// changing the feature itself.
		constexpr float Gap = 10.0f;
		constexpr float CompactHeight = 48.0f;
		constexpr float TfMenuHeight = 280.0f;
		constexpr float TfDuelHudHeight = 405.0f;
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

		static SBkwCompactModuleState s_TfMenuModule;
		static SBkwCompactModuleState s_TfDuelModule;
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
		ScrollParams.m_ScrollUnit = 54.0f;
		ScrollParams.m_ScrollbarThickness = 14.0f;
		CUIRect Content = PageView;
		s_PersonalizationScrollRegion.Begin(&Content, &ScrollParams);

		auto AddGap = [&]() {
			Content.HSplitTop(Gap, nullptr, &Content);
		};

		CUIRect Module;

		Content.HSplitTop(s_TfMenuModule.m_Open ? TfMenuHeight : CompactHeight, &Module, &Content);
		s_PersonalizationScrollRegion.AddRect(Module);
		if(s_TfMenuModule.m_Open)
		{
			RenderTfMenuSettings(pMenus, pUi, pGameClient, Module);
			RenderBkwCompactCollapseButton(pMenus, Module, s_TfMenuModule);
		}
		else
			RenderBkwCompactModuleRow(pMenus, pUi, Module, "TeeFusion Menu", &g_Config.m_BkwTfMenu, s_TfMenuModule);
		AddGap();

		Content.HSplitTop(s_TfDuelModule.m_Open ? TfDuelHudHeight : CompactHeight, &Module, &Content);
		s_PersonalizationScrollRegion.AddRect(Module);
		if(s_TfDuelModule.m_Open)
		{
			RenderTfDuelHudSettings(pMenus, pUi, Module);
			RenderBkwCompactCollapseButton(pMenus, Module, s_TfDuelModule);
		}
		else
			RenderBkwCompactModuleRow(pMenus, pUi, Module, "TeeFusion Duel HUD", &g_Config.m_BkwTfDuelHud, s_TfDuelModule);
		AddGap();

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

		Content.HSplitTop(s_TipoCheatModule.m_Open ? TipoCheatHeight : CompactHeight, &Module, &Content);
		s_PersonalizationScrollRegion.AddRect(Module);
		if(s_TipoCheatModule.m_Open)
		{
			RenderBkwTipoCheatSettings(pMenus, pUi, Module);
			RenderBkwCompactCollapseButton(pMenus, Module, s_TipoCheatModule);
		}
		else
			RenderBkwCompactModuleRow(pMenus, pUi, Module, "ТипоЧит", &g_Config.m_BkwTipoCheat, s_TipoCheatModule);
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
		AddGap();

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
