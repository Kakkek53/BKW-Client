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
		constexpr float TfMenuHeight = 280.0f;
		constexpr float TfDuelHudHeight = 405.0f;
		constexpr float LocalDuelHeight = 735.0f;
		constexpr float DdnetMenuHeight = 280.0f;
		constexpr float OptimizerHeight = 245.0f;
		constexpr float TipoCheatHeight = 250.0f;
		constexpr float HookTargetHeight = 220.0f;
		constexpr float AntiPingHeight = 170.0f;
		constexpr float AnimatedBackgroundHeight = 510.0f;
		constexpr float CameraDriftHeight = 320.0f;
		constexpr float HudHeight = 800.0f;
		constexpr float BackgroundHeight = 470.0f;

		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollUnit = 70.0f;
		ScrollParams.m_ScrollbarThickness = 14.0f;
		CUIRect Content = PageView;
		s_PersonalizationScrollRegion.Begin(&Content, &ScrollParams);

		CUIRect TfMenuSection, TfDuelHudSection, LocalDuelSection, DdnetMenuSection, OptimizerSection, TipoCheatSection, HookTargetSection, AntiPingSection, AnimatedBackgroundSection, CameraDriftSection, HudSection, BackgroundSection;
		Content.HSplitTop(TfMenuHeight, &TfMenuSection, &Content);
		s_PersonalizationScrollRegion.AddRect(TfMenuSection);
		RenderTfMenuSettings(pMenus, pUi, pGameClient, TfMenuSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(TfDuelHudHeight, &TfDuelHudSection, &Content);
		s_PersonalizationScrollRegion.AddRect(TfDuelHudSection);
		RenderTfDuelHudSettings(pMenus, pUi, TfDuelHudSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(LocalDuelHeight, &LocalDuelSection, &Content);
		s_PersonalizationScrollRegion.AddRect(LocalDuelSection);
		RenderBkwLocalDuelSettings(pMenus, pUi, pGameClient, LocalDuelSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(DdnetMenuHeight, &DdnetMenuSection, &Content);
		s_PersonalizationScrollRegion.AddRect(DdnetMenuSection);
		RenderDdnetVoteMenuSettings(pMenus, pUi, DdnetMenuSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(OptimizerHeight, &OptimizerSection, &Content);
		s_PersonalizationScrollRegion.AddRect(OptimizerSection);
		RenderBkwOptimizerSettings(pMenus, pUi, OptimizerSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(TipoCheatHeight, &TipoCheatSection, &Content);
		s_PersonalizationScrollRegion.AddRect(TipoCheatSection);
		RenderBkwTipoCheatSettings(pMenus, pUi, TipoCheatSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(HookTargetHeight, &HookTargetSection, &Content);
		s_PersonalizationScrollRegion.AddRect(HookTargetSection);
		RenderBkwHookTargetSettings(pMenus, pUi, HookTargetSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(AntiPingHeight, &AntiPingSection, &Content);
		s_PersonalizationScrollRegion.AddRect(AntiPingSection);
		RenderBkwAdaptiveAntiPingSettings(pMenus, pUi, pClient, AntiPingSection);
		Content.HSplitTop(Gap, nullptr, &Content);

		Content.HSplitTop(AnimatedBackgroundHeight, &AnimatedBackgroundSection, &Content);
		s_PersonalizationScrollRegion.AddRect(AnimatedBackgroundSection);
		RenderBkwAnimatedBackgroundSettings(pMenus, pUi, pGameClient, AnimatedBackgroundSection);
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
