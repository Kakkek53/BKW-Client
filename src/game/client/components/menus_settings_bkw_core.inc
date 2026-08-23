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
