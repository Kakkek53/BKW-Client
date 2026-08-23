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

void RenderTfDuelHudSettings(CMenus *pMenus, CUi *pUi, CUIRect PageView)
{
	static int s_TfDuelHudToggleId;
	static int s_TfDuelHideBroadcastToggleId;
	static CButtonContainer s_aTfDuelStyleButtons[4];
	static CButtonContainer s_aTfDuelPositionButtons[9];
	static CButtonContainer s_TfDuelResetButton;

	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);

	CUIRect TitleRow, Title, Reset;
	PageView.HSplitTop(26.0f, &TitleRow, &PageView);
	TitleRow.VSplitRight(92.0f, &Title, &Reset);
	pUi->DoLabel(&Title, "TeeFusion Duel HUD", 18.0f, TEXTALIGN_ML);
	if(pMenus->DoButton_Menu(&s_TfDuelResetButton, "Сбросить", 0, &Reset))
	{
		g_Config.m_BkwTfDuelHud = 1;
		g_Config.m_BkwTfDuelHudStyle = 0;
		g_Config.m_BkwTfDuelHideBroadcast = 1;
		g_Config.m_BkwTfDuelHudPosition = 1;
		g_Config.m_BkwTfDuelHudScale = 85;
		g_Config.m_BkwTfDuelHudOpacity = 92;
		g_Config.m_BkwTfDuelHudRounding = 12;
	}

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect Toggle;
	PageView.HSplitTop(24.0f, &Toggle, &PageView);
	if(pMenus->DoButton_CheckBox(&s_TfDuelHudToggleId, "Свой счёт дуэли TeeFusion", g_Config.m_BkwTfDuelHud, &Toggle))
		g_Config.m_BkwTfDuelHud ^= 1;

	PageView.HSplitTop(3.0f, nullptr, &PageView);
	CUIRect HideBroadcast;
	PageView.HSplitTop(24.0f, &HideBroadcast, &PageView);
	if(pMenus->DoButton_CheckBox(&s_TfDuelHideBroadcastToggleId, "Скрывать стандартный broadcast счёта", g_Config.m_BkwTfDuelHideBroadcast, &HideBroadcast))
		g_Config.m_BkwTfDuelHideBroadcast ^= 1;

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	CUIRect StyleLabel;
	PageView.HSplitTop(18.0f, &StyleLabel, &PageView);
	pUi->DoLabel(&StyleLabel, "Стиль табло", 12.0f, TEXTALIGN_ML);

	PageView.HSplitTop(4.0f, nullptr, &PageView);
	CUIRect StyleButtons;
	PageView.HSplitTop(30.0f, &StyleButtons, &PageView);
	const char *apStyleNames[4] = {"Liquid Glass", "Minimal", "Neon", "Classic"};
	for(int Style = 0; Style < 4; ++Style)
	{
		CUIRect Button;
		const float ButtonWidth = StyleButtons.w / (float)(4 - Style);
		StyleButtons.VSplitLeft(ButtonWidth, &Button, &StyleButtons);
		if(Style < 3)
			Button.VMargin(2.0f, &Button);
		if(pMenus->DoButton_Menu(&s_aTfDuelStyleButtons[Style], apStyleNames[Style], g_Config.m_BkwTfDuelHudStyle == Style, &Button))
			g_Config.m_BkwTfDuelHudStyle = Style;
	}

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect PositionLabel;
	PageView.HSplitTop(18.0f, &PositionLabel, &PageView);
	pUi->DoLabel(&PositionLabel, "Положение на экране", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);

	const char *apPositionNames[9] = {"TL", "TC", "TR", "CL", "C", "CR", "BL", "BC", "BR"};
	for(int Row = 0; Row < 3; ++Row)
	{
		CUIRect PositionRow;
		PageView.HSplitTop(26.0f, &PositionRow, &PageView);
		for(int Column = 0; Column < 3; ++Column)
		{
			const int Position = Row * 3 + Column;
			CUIRect PositionButton;
			const float ButtonWidth = PositionRow.w / (float)(3 - Column);
			PositionRow.VSplitLeft(ButtonWidth, &PositionButton, &PositionRow);
			PositionButton.VMargin(2.0f, &PositionButton);
			if(pMenus->DoButton_Menu(&s_aTfDuelPositionButtons[Position], apPositionNames[Position], g_Config.m_BkwTfDuelHudPosition == Position, &PositionButton))
				g_Config.m_BkwTfDuelHudPosition = Position;
		}
		if(Row < 2)
			PageView.HSplitTop(3.0f, nullptr, &PageView);
	}

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect ScaleRow;
	PageView.HSplitTop(28.0f, &ScaleRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTfDuelHudScale, &g_Config.m_BkwTfDuelHudScale, &ScaleRow, "Размер табло", 60, 140);

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect OpacityRow;
	PageView.HSplitTop(28.0f, &OpacityRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTfDuelHudOpacity, &g_Config.m_BkwTfDuelHudOpacity, &OpacityRow, "Прозрачность", 20, 100);

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect RoundingRow;
	PageView.HSplitTop(28.0f, &RoundingRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTfDuelHudRounding, &g_Config.m_BkwTfDuelHudRounding, &RoundingRow, "Скругление", 0, 24);

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	CUIRect Hint;
	PageView.HSplitTop(32.0f, &Hint, &PageView);
	pUi->DoLabel(&Hint, "Парсер подтверждает счёт только после TeeFusion «Дуэль» и не перехватывает случайные name - number broadcast.", 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Hint.w});
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
