#include "menus.h"

#include <algorithm>

namespace BkwMenuProxy
{
enum
{
	TAB_MAIN = 0,
	TAB_HOURS,
	TAB_PERSONALIZATION,
	TAB_SHOP,
	TAB_COUNT,
};

enum
{
	LEGACY_SAVES = 0,
	LEGACY_CHECKPOINTS,
	LEGACY_PLAYER,
	LEGACY_CLANS,
	LEGACY_HOURS,
	LEGACY_HUD,
	LEGACY_BACKGROUND,
	LEGACY_SHOP,
	LEGACY_EXIT,
};

template<typename TLegacyRenderer>
void Render(CMenus *pMenus, CUi *pUi, CUIRect PageView, int &LegacyTab, TLegacyRenderer &LegacyRenderer)
{
	static int s_CurrentTab = TAB_MAIN;
	static int s_FirstVisibleTab = 0;
	static CButtonContainer s_aTabButtons[TAB_COUNT];
	static CButtonContainer s_PreviousButton;
	static CButtonContainer s_NextButton;

	s_CurrentTab = std::clamp(s_CurrentTab, 0, TAB_COUNT - 1);

	// Match the BestClient submenu proportions and segmented tab styling.
	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect TabBar;
	PageView.HSplitTop(24.0f, &TabBar, &PageView);
	const char *apTabs[TAB_COUNT] = {"Основное", "Часы", "Персонализация", "Магазин"};

	constexpr float MinTabWidth = 128.0f;
	constexpr float PagerWidth = 28.0f;
	int VisibleCount = std::clamp((int)(TabBar.w / MinTabWidth), 1, TAB_COUNT);
	bool Scrollable = VisibleCount < TAB_COUNT;
	if(Scrollable)
		VisibleCount = std::clamp((int)((TabBar.w - PagerWidth * 2.0f) / MinTabWidth), 1, TAB_COUNT);

	const int MaxFirst = std::max(0, TAB_COUNT - VisibleCount);
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

	// The legacy renderer starts with its own 30 px tab bar plus 14 px gap.
	// Shift it above the clip so only the actual section content remains visible.
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
		// Сохранение + чекпоинты + игрок + кланы + предупреждение о выходе.
		constexpr float Gap = 22.0f;
		constexpr float CheckpointsHeight = 245.0f;
		constexpr float PlayerHeight = 205.0f;
		constexpr float ClansHeight = 205.0f;
		constexpr float ExitHeight = 390.0f;
		const float FixedHeight = CheckpointsHeight + PlayerHeight + ClansHeight + ExitHeight + Gap * 4.0f;
		const float SavesHeight = std::max(900.0f, PageView.h - FixedHeight);

		CUIRect Section;
		PageView.HSplitTop(SavesHeight, &Section, &PageView);
		RenderLegacySection(LEGACY_SAVES, Section);
		PageView.HSplitTop(Gap, nullptr, &PageView);
		PageView.HSplitTop(CheckpointsHeight, &Section, &PageView);
		RenderLegacySection(LEGACY_CHECKPOINTS, Section);
		PageView.HSplitTop(Gap, nullptr, &PageView);
		PageView.HSplitTop(PlayerHeight, &Section, &PageView);
		RenderLegacySection(LEGACY_PLAYER, Section);
		PageView.HSplitTop(Gap, nullptr, &PageView);
		PageView.HSplitTop(ClansHeight, &Section, &PageView);
		RenderLegacySection(LEGACY_CLANS, Section);
		PageView.HSplitTop(Gap, nullptr, &PageView);
		PageView.HSplitTop(ExitHeight, &Section, &PageView);
		RenderLegacySection(LEGACY_EXIT, Section);

		// Keep the existing large BKW scroll range for this combined page.
		LegacyTab = LEGACY_SAVES;
	}
	else if(s_CurrentTab == TAB_HOURS)
	{
		RenderLegacySection(LEGACY_HOURS, PageView);
		LegacyTab = LEGACY_HOURS;
	}
	else if(s_CurrentTab == TAB_PERSONALIZATION)
	{
		// HUD and background are one personalization page.
		constexpr float Gap = 22.0f;
		constexpr float BackgroundHeight = 470.0f;
		const float HudHeight = std::max(760.0f, PageView.h - BackgroundHeight - Gap);
		CUIRect HudSection, BackgroundSection;
		PageView.HSplitTop(HudHeight, &HudSection, &PageView);
		PageView.HSplitTop(Gap, nullptr, &PageView);
		PageView.HSplitTop(BackgroundHeight, &BackgroundSection, &PageView);
		RenderLegacySection(LEGACY_HUD, HudSection);
		RenderLegacySection(LEGACY_BACKGROUND, BackgroundSection);

		// Reuse the large legacy scroll budget so both sections remain reachable.
		LegacyTab = LEGACY_SAVES;
	}
	else
	{
		RenderLegacySection(LEGACY_SHOP, PageView);
		LegacyTab = LEGACY_SHOP;
	}
}
} // namespace BkwMenuProxy

// Function-like macro: it does not affect the local `auto RenderBkwPage = ...`
// declaration in the legacy source, only the two call sites below it. The
// unexpanded RenderBkwPage token passed as the last argument is that local
// legacy renderer and is intentionally used to keep all existing BKW logic.
#define RenderBkwPage(BKW_VIEW) BkwMenuProxy::Render(this, Ui(), (BKW_VIEW), s_BkwTab, RenderBkwPage)
#include "menus_settings_legacy.inc"
#undef RenderBkwPage
