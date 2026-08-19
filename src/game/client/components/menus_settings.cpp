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

static char s_aTfParserTestMessage[192] = "";
static int64_t s_TfParserTestVisibleUntil = 0;

bool TfParserTestVisible()
{
	return s_aTfParserTestMessage[0] != '\0' && time_get() < s_TfParserTestVisibleUntil;
}

void UpdateTfParserTest(IInput *pInput, CGameClient *pGameClient)
{
	if(!pInput->KeyPress(KEY_F6))
		return;

	char aUserName[64];
	if(BkwTfMenu::ParseUserName(pGameClient->m_Voting, aUserName, sizeof(aUserName)))
		str_format(s_aTfParserTestMessage, sizeof(s_aTfParserTestMessage), "F6 TeeFusion parser: Имя пользователя — %s", aUserName);
	else
		str_copy(s_aTfParserTestMessage, "F6 TeeFusion parser: строка «Имя пользователя» не найдена в vote options");
	s_TfParserTestVisibleUntil = time_get() + time_freq() * 7;
}

void RenderTfParserTestBanner(CUi *pUi, CUIRect Anchor)
{
	if(!TfParserTestVisible())
		return;

	CUIRect Banner;
	Anchor.HSplitTop(40.0f, &Banner, nullptr);
	Banner.VMargin(36.0f, &Banner);
	Banner.Draw(ColorRGBA(0.04f, 0.05f, 0.07f, 0.96f), IGraphics::CORNER_ALL, 8.0f);
	Banner.Margin(8.0f, &Banner);
	pUi->DoLabel(&Banner, s_aTfParserTestMessage, 13.0f, TEXTALIGN_MC);
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
	if(TfParserTestVisible())
	{
		str_copy(aStatus, s_aTfParserTestMessage);
	}
	else if(!g_Config.m_BkwTfMenu)
	{
		str_copy(aStatus, "Выключено. F6 — разовый тест чтения TeeFusion vote options.");
	}
	else
	{
		char aUserName[64];
		if(BkwTfMenu::ParseUserName(pGameClient->m_Voting, aUserName, sizeof(aUserName)))
			str_format(aStatus, sizeof(aStatus), "TF parser: Имя пользователя — %s", aUserName);
		else
			str_copy(aStatus, "Новое меню применяется только на серверах TeeFusion.");
	}
	pUi->DoLabel(&Status, aStatus, 11.0f, TEXTALIGN_ML);
}

void RenderDdnetVoteMenuSettings(CMenus *pMenus, CUi *pUi, CUIRect PageView)
{
	static int s_DdnetVoteMenuToggleId;
	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
	PageView.Margin(10.0f, &PageView);

	CUIRect Header, Toggle, Status;
	PageView.HSplitTop(24.0f, &Header, &PageView);
	pUi->DoLabel(&Header, "DDNet Race Menu", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Toggle, &PageView);
	if(pMenus->DoButton_CheckBox(&s_DdnetVoteMenuToggleId, "Новое меню голосования DDNet", g_Config.m_BkwDdnetVoteMenu, &Toggle))
		g_Config.m_BkwDdnetVoteMenu ^= 1;
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(20.0f, &Status, &PageView);
	pUi->DoLabel(&Status, "Карты берутся из DDNet API; кик и наблюдатели используют обычное голосование сервера.", 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Status.w});
}

void RenderCameraDriftSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, IClient *pClient, CUIRect PageView)
{
	static int s_CameraDriftToggleId;
	static CButtonContainer s_CameraDriftResetButton;
	static CButtonContainer s_CameraDriftForwardButton;
	static CButtonContainer s_CameraDriftBackwardButton;

	const bool Online = pClient->State() == IClient::STATE_ONLINE;
	const bool IsFngServer = Online && pGameClient->m_GameInfo.m_PredictFNG;
	const bool Is0xFServer = Online && str_comp_nocase(pGameClient->m_GameInfo.m_aGameType, "0xf") == 0;
	const bool Blocked = IsFngServer || Is0xFServer;

	CUIRect Card;
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
		constexpr float DdnetMenuHeight = 104.0f;
		constexpr float CameraDriftHeight = 320.0f;
		constexpr float HudHeight = 800.0f;
		constexpr float BackgroundHeight = 470.0f;

		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollUnit = 70.0f;
		ScrollParams.m_ScrollbarThickness = 14.0f;
		CUIRect Content = PageView;
		s_PersonalizationScrollRegion.Begin(&Content, &ScrollParams);

		CUIRect TfMenuSection, DdnetMenuSection, CameraDriftSection, HudSection, BackgroundSection;
		Content.HSplitTop(TfMenuHeight, &TfMenuSection, &Content);
		s_PersonalizationScrollRegion.AddRect(TfMenuSection);
		RenderTfMenuSettings(pMenus, pUi, pGameClient, TfMenuSection);
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
