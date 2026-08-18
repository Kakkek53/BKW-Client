#include "menus.h"

#include <base/str.h>

#include <engine/client.h>
#include <engine/shared/config.h>

#include <game/client/gameclient.h>

#include "bkw/tf_menu_parser.inc"

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

void RenderCheckpoints(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, IClient *pClient, CUIRect PageView)
{
	static int s_CheckpointsToggleId;
	static CButtonContainer s_CheckpointMouseLeftButton;
	static CButtonContainer s_CheckpointMouseRightButton;

	CUIRect Header;
	PageView.HSplitTop(28.0f, &Header, &PageView);
	pUi->DoLabel(&Header, "Чекпоинты", 22.0f, TEXTALIGN_ML);
	PageView.HSplitTop(8.0f, nullptr, &PageView);

	const bool CheckpointsEnabled = pGameClient->m_FastActions.BkwCheckpointsEnabled();
	CUIRect CheckpointToggle;
	PageView.HSplitTop(28.0f, &CheckpointToggle, &PageView);
	if(pMenus->DoButton_CheckBox(&s_CheckpointsToggleId, "Чекпоинты", CheckpointsEnabled ? 1 : 0, &CheckpointToggle))
		pGameClient->m_FastActions.SetBkwCheckpointsEnabled(!CheckpointsEnabled);

	if(!pGameClient->m_FastActions.BkwCheckpointsEnabled())
		return;

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect CheckpointCard;
	PageView.HSplitTop(158.0f, &CheckpointCard, &PageView);
	CheckpointCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
	CheckpointCard.Margin(10.0f, &CheckpointCard);

	CUIRect Description, MouseLabel, MouseButtons, Help1, Help2, Status;
	CheckpointCard.HSplitTop(30.0f, &Description, &CheckpointCard);
	pUi->DoLabel(&Description, "Работает только когда сервер подтвердил режим /practice.", 12.0f, TEXTALIGN_ML);
	CheckpointCard.HSplitTop(22.0f, &MouseLabel, &CheckpointCard);
	pUi->DoLabel(&MouseLabel, "Кнопка создания / удаления:", 12.0f, TEXTALIGN_ML);
	CheckpointCard.HSplitTop(28.0f, &MouseButtons, &CheckpointCard);
	CUIRect LeftMouseButton, RightMouseButton;
	MouseButtons.VSplitMid(&LeftMouseButton, &RightMouseButton, 6.0f);
	const int MouseButton = pGameClient->m_FastActions.BkwCheckpointMouseButton();
	if(pMenus->DoButton_Menu(&s_CheckpointMouseLeftButton, "ЛКМ", MouseButton == 0, &LeftMouseButton))
		pGameClient->m_FastActions.SetBkwCheckpointMouseButton(0);
	if(pMenus->DoButton_Menu(&s_CheckpointMouseRightButton, "ПКМ", MouseButton == 1, &RightMouseButton))
		pGameClient->m_FastActions.SetBkwCheckpointMouseButton(1);

	CheckpointCard.HSplitTop(22.0f, &Help1, &CheckpointCard);
	pUi->DoLabel(&Help1, "Удержание 0.35 сек.: создать точку у tee. Повторить рядом с точкой — удалить.", 11.0f, TEXTALIGN_ML);
	CheckpointCard.HSplitTop(22.0f, &Help2, &CheckpointCard);
	pUi->DoLabel(&Help2, "Нажатие колёсика: /tpxy к последнему чекпоинту.", 11.0f, TEXTALIGN_ML);
	CheckpointCard.HSplitTop(22.0f, &Status, &CheckpointCard);

	bool PracticeActive = false;
	const int LocalClientId = pGameClient->m_Snap.m_LocalClientId;
	if(pClient->State() == IClient::STATE_ONLINE && LocalClientId >= 0 && LocalClientId < MAX_CLIENTS)
	{
		const auto &Character = pGameClient->m_Snap.m_aCharacters[LocalClientId];
		PracticeActive = Character.m_Active && Character.m_HasExtendedData && (Character.m_ExtendedData.m_Flags & CHARACTERFLAG_PRACTICE_MODE) != 0;
	}
	char aCheckpointStatus[160];
	str_format(aCheckpointStatus, sizeof(aCheckpointStatus), "Practice: %s   •   Чекпоинтов: %d", PracticeActive ? "активен" : "не активен", pGameClient->m_FastActions.BkwCheckpointCount());
	pUi->DoLabel(&Status, aCheckpointStatus, 11.0f, TEXTALIGN_ML);
}

void RenderTfMenuSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, CUIRect PageView)
{
	static int s_TfMenuToggleId;

	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
	PageView.Margin(10.0f, &PageView);

	CUIRect Header, Toggle, Status;
	PageView.HSplitTop(24.0f, &Header, &PageView);
	pUi->DoLabel(&Header, "TeeFusion Menu", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Toggle, &PageView);
	if(pMenus->DoButton_CheckBox(&s_TfMenuToggleId, "Новый дизайн меню TF", g_Config.m_BcTfMenu, &Toggle))
		g_Config.m_BcTfMenu ^= 1;

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(20.0f, &Status, &PageView);
	char aUserName[64];
	char aStatus[192];
	if(BkwTfMenu::ParseUserName(pGameClient->m_Voting, aUserName, sizeof(aUserName)))
		str_format(aStatus, sizeof(aStatus), "TF parser: Имя пользователя — %s", aUserName);
	else
		str_copy(aStatus, "TF parser: строка «Имя пользователя:» пока не найдена");
	pUi->DoLabel(&Status, aStatus, 11.0f, TEXTALIGN_ML);
}

template<typename TLegacyRenderer>
void Render(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, IClient *pClient, CUIRect PageView, int &LegacyTab, TLegacyRenderer &LegacyRenderer)
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
		constexpr float SavesHeight = 1050.0f;
		constexpr float CheckpointsHeight = 245.0f;
		constexpr float PlayerHeight = 205.0f;
		constexpr float ClansHeight = 205.0f;
		constexpr float ExitHeight = 390.0f;

		CUIRect Section;
		PageView.HSplitTop(SavesHeight, &Section, &PageView);
		RenderLegacySection(LEGACY_SAVES, Section);
		PageView.HSplitTop(Gap, nullptr, &PageView);
		PageView.HSplitTop(CheckpointsHeight, &Section, &PageView);
		RenderCheckpoints(pMenus, pUi, pGameClient, pClient, Section);
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
		// TeeFusion toggle + HUD + background are one personalization page.
		constexpr float Gap = 22.0f;
		constexpr float TfMenuHeight = 104.0f;
		constexpr float HudHeight = 800.0f;
		constexpr float BackgroundHeight = 470.0f;
		CUIRect TfMenuSection, HudSection, BackgroundSection;
		PageView.HSplitTop(TfMenuHeight, &TfMenuSection, &PageView);
		PageView.HSplitTop(Gap, nullptr, &PageView);
		PageView.HSplitTop(HudHeight, &HudSection, &PageView);
		PageView.HSplitTop(Gap, nullptr, &PageView);
		PageView.HSplitTop(BackgroundHeight, &BackgroundSection, &PageView);
		RenderTfMenuSettings(pMenus, pUi, pGameClient, TfMenuSection);
		RenderLegacySection(LEGACY_HUD, HudSection);
		RenderLegacySection(LEGACY_BACKGROUND, BackgroundSection);

		// Reuse the large legacy scroll budget so all personalization sections remain reachable.
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
#define RenderBkwPage(BKW_VIEW) BkwMenuProxy::Render(this, Ui(), GameClient(), Client(), (BKW_VIEW), s_BkwTab, RenderBkwPage)
#include "menus_settings_legacy.inc"
#undef RenderBkwPage
