#include "menus.h"

#include <base/str.h>
#include <base/system.h>

#include <engine/client.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include <game/client/gameclient.h>
#include <game/client/ui_scrollregion.h>

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

void RenderTfParserTest(IInput *pInput, CUi *pUi, CGameClient *pGameClient, CUIRect Anchor)
{
	static char s_aMessage[192] = "";
	static int64_t s_VisibleUntil = 0;

	if(pInput->KeyPress(KEY_F6))
	{
		char aUserName[64];
		if(BkwTfMenu::ParseUserName(pGameClient->m_Voting, aUserName, sizeof(aUserName)))
			str_format(s_aMessage, sizeof(s_aMessage), "F6 TeeFusion parser: Имя пользователя — %s", aUserName);
		else
			str_copy(s_aMessage, "F6 TeeFusion parser: строка «Имя пользователя» не найдена в vote options");
		s_VisibleUntil = time_get() + time_freq() * 7;
	}

	if(s_aMessage[0] == '\0' || time_get() >= s_VisibleUntil)
		return;

	CUIRect Banner;
	Anchor.HSplitTop(38.0f, &Banner, nullptr);
	Banner.VMargin(36.0f, &Banner);
	Banner.Draw(ColorRGBA(0.04f, 0.05f, 0.07f, 0.94f), IGraphics::CORNER_ALL, 8.0f);
	Banner.Margin(8.0f, &Banner);
	pUi->DoLabel(&Banner, s_aMessage, 13.0f, TEXTALIGN_MC);
}

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
	if(pMenus->DoButton_CheckBox(&s_TfMenuToggleId, "Новый дизайн меню TF", g_Config.m_BkwTfMenu, &Toggle))
		g_Config.m_BkwTfMenu ^= 1;

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(20.0f, &Status, &PageView);
	char aStatus[192];
	if(!g_Config.m_BkwTfMenu)
	{
		str_copy(aStatus, "Парсер выключен. F6 — разовый тест чтения vote options.");
	}
	else
	{
		char aUserName[64];
		if(BkwTfMenu::ParseUserName(pGameClient->m_Voting, aUserName, sizeof(aUserName)))
			str_format(aStatus, sizeof(aStatus), "TF parser: Имя пользователя — %s", aUserName);
		else
			str_copy(aStatus, "TF parser: строка «Имя пользователя» пока не найдена");
	}
	pUi->DoLabel(&Status, aStatus, 11.0f, TEXTALIGN_ML);
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

	RenderTfParserTest(pInput, pUi, pGameClient, PageView);

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
		constexpr float HudHeight = 800.0f;
		constexpr float BackgroundHeight = 470.0f;

		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollUnit = 70.0f;
		ScrollParams.m_ScrollbarThickness = 14.0f;
		CUIRect Content = PageView;
		s_PersonalizationScrollRegion.Begin(&Content, &ScrollParams);

		CUIRect TfMenuSection, HudSection, BackgroundSection;
		Content.HSplitTop(TfMenuHeight, &TfMenuSection, &Content);
		s_PersonalizationScrollRegion.AddRect(TfMenuSection);
		RenderTfMenuSettings(pMenus, pUi, pGameClient, TfMenuSection);
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
}
} // namespace BkwMenuProxy

#define RenderBkwPage(BKW_VIEW) BkwMenuProxy::Render(this, Ui(), GameClient(), Client(), Input(), (BKW_VIEW), s_BkwTab, RenderBkwPage)
#include "menus_settings_legacy.inc"
#undef RenderBkwPage
