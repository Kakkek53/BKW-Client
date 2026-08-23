#include <base/math.h>
#include <base/str.h>
#include <base/system.h>

#include <engine/client/client.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>

#include <game/client/components/bkw/server_detector.inc>
#include <game/client/components/bkw/tf_menu_parser.inc>
#include <game/client/components/bkw/tf_svg_icons.inc>
#include <game/client/components/voting.h>
#include <game/client/gameclient.h>
#include <game/client/lineinput.h>
#include <game/client/ui.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

inline CLineInputBuffered<VOTE_REASON_LENGTH> s_BkwTfReasonInput;

inline CGameClient *BkwTfGameClient(CUi *pUi)
{
	if(!pUi || !pUi->Client())
		return nullptr;
	CClient *pConcreteClient = static_cast<CClient *>(pUi->Client());
	return static_cast<CGameClient *>(pConcreteClient->GameClient());
}

inline bool BkwTfIsConnectedTeeFusion(CUi *pUi)
{
	IClient *pClient = pUi ? pUi->Client() : nullptr;
	if(!pClient || pClient->State() != IClient::STATE_ONLINE)
		return false;

	// The FastActions component updates the shared master-server cache before the
	// overlay is rendered. Keep the overlay on exactly the same source of truth.
	return BkwServerDetector::IsTeeFusionCached(pClient->ServerAddress());
}

inline void BkwTfMenuOverlayReset(bool ReturnToGame = true)
{
	const bool WasActive = s_BkwTfMenuOverlayActive;
	s_BkwTfMenuOverlayActive = false;
	s_BkwTfMenuOverlayPulseSent = false;
	s_BkwTfMenuSelectedTab = 0;
	s_BkwTfMenuPendingTab = -1;
	s_BkwTfMenuScrollRow = 0;
	s_BkwTfMenuWaitingForUpdate = false;
	s_BkwTfMenuVoteSignatureBeforeAction = 0;
	s_BkwTfMenuActionStartedAt = 0;
	if(s_BkwTfReasonInput.IsActive())
		s_BkwTfReasonInput.Deactivate();
	s_BkwTfReasonInput.Clear();

	if(ReturnToGame && s_pBkwTfMenuLastUi)
	{
		if(CGameClient *pGameClient = BkwTfGameClient(s_pBkwTfMenuLastUi))
			pGameClient->m_Menus.SetGamePage(CMenus::PAGE_GAME);
	}

	if(WasActive && s_pBkwTfMenuLastUi)
		s_pBkwTfMenuLastUi->ConsumeHotkey(CUi::HOTKEY_ESCAPE);
}

inline uint64_t BkwTfVoteSignature(const CVoting &Voting)
{
	uint64_t Hash = 1469598103934665603ULL;
	int Index = 0;
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
	{
		for(const unsigned char *p = reinterpret_cast<const unsigned char *>(pOption->m_aDescription); *p; ++p)
		{
			Hash ^= *p;
			Hash *= 1099511628211ULL;
		}
		Hash ^= static_cast<uint64_t>(Index + 1);
		Hash *= 1099511628211ULL;
	}
	Hash ^= static_cast<uint64_t>(Voting.NumOptions());
	return Hash;
}

inline void BkwTfPulseMenuInput()
{
	if(!s_pBkwTfMenuLastUi)
		return;
	IClient *pClient = s_pBkwTfMenuLastUi->Client();
	if(!pClient || pClient->State() != IClient::STATE_ONLINE)
		return;

	CClient *pConcreteClient = static_cast<CClient *>(pClient);
	CGameClient *pGameClient = static_cast<CGameClient *>(pConcreteClient->GameClient());
	if(!pGameClient)
		return;

	const bool WasMenuActive = pGameClient->m_Menus.IsActive();
	if(!WasMenuActive)
		pGameClient->m_Menus.SetActive(true);
	pConcreteClient->SendInput();
	if(!WasMenuActive)
		pGameClient->m_Menus.SetActive(false);
}

inline void BkwTfBeginVoteAction(CVoting &Voting, int OptionIndex, const char *pReason = "")
{
	if(s_BkwTfMenuWaitingForUpdate || OptionIndex < 0 || OptionIndex >= Voting.NumOptions())
		return;

	BkwTfPulseMenuInput();
	s_BkwTfMenuVoteSignatureBeforeAction = BkwTfVoteSignature(Voting);
	s_BkwTfMenuActionStartedAt = time_get();
	s_BkwTfMenuWaitingForUpdate = true;
	Voting.CallvoteOption(OptionIndex, pReason ? pReason : "");
}

inline void BkwTfUpdateVoteTransition(const CVoting &Voting)
{
	if(!s_BkwTfMenuWaitingForUpdate)
		return;

	const bool TimedOut = time_get() - s_BkwTfMenuActionStartedAt > time_freq() * 2;
	const bool GotNewCompleteList = Voting.NumOptions() > 0 && !Voting.IsReceivingOptions() &&
		BkwTfVoteSignature(Voting) != s_BkwTfMenuVoteSignatureBeforeAction;
	if(GotNewCompleteList || TimedOut)
	{
		s_BkwTfMenuWaitingForUpdate = false;
		s_BkwTfMenuScrollRow = 0;
	}
}

inline BkwTfSvg::EIcon BkwTfSvgIcon(BkwTfMenu::EIconHint Icon)
{
	switch(Icon)
	{
	case BkwTfMenu::EIconHint::HOME: return BkwTfSvg::EIcon::HOME;
	case BkwTfMenu::EIconHint::BACK: return BkwTfSvg::EIcon::BACK;
	case BkwTfMenu::EIconHint::USER: return BkwTfSvg::EIcon::USER;
	case BkwTfMenu::EIconHint::SHOP: return BkwTfSvg::EIcon::SHOP;
	case BkwTfMenu::EIconHint::GIFT: return BkwTfSvg::EIcon::GIFT;
	case BkwTfMenu::EIconHint::SETTINGS: return BkwTfSvg::EIcon::SETTINGS;
	case BkwTfMenu::EIconHint::TROPHY: return BkwTfSvg::EIcon::TROPHY;
	case BkwTfMenu::EIconHint::CHART: return BkwTfSvg::EIcon::CHART;
	case BkwTfMenu::EIconHint::LEVEL: return BkwTfSvg::EIcon::LEVEL;
	case BkwTfMenu::EIconHint::INVENTORY: return BkwTfSvg::EIcon::INVENTORY;
	case BkwTfMenu::EIconHint::CLAN: return BkwTfSvg::EIcon::CLAN;
	case BkwTfMenu::EIconHint::CROWN: return BkwTfSvg::EIcon::CROWN;
	case BkwTfMenu::EIconHint::MAIL: return BkwTfSvg::EIcon::MAIL;
	case BkwTfMenu::EIconHint::GAME: return BkwTfSvg::EIcon::GAME;
	case BkwTfMenu::EIconHint::CHECK: return BkwTfSvg::EIcon::CHECK;
	case BkwTfMenu::EIconHint::WARNING: return BkwTfSvg::EIcon::WARNING;
	case BkwTfMenu::EIconHint::COINS: return BkwTfSvg::EIcon::COINS;
	case BkwTfMenu::EIconHint::INFO:
	case BkwTfMenu::EIconHint::GENERIC:
	default: return BkwTfSvg::EIcon::INFO;
	}
}

inline ColorRGBA BkwTfIconColor(const BkwTfMenu::SItem &Item, bool Enabled = true)
{
	if(!Enabled)
		return ColorRGBA(0.52f, 0.54f, 0.58f, 0.60f);
	if(Item.m_Negative || Item.m_Warning)
		return ColorRGBA(1.0f, 0.45f, 0.40f, 0.98f);
	if(Item.m_Checked)
		return ColorRGBA(0.42f, 0.92f, 0.58f, 0.98f);
	return ColorRGBA(0.82f, 0.84f, 0.88f, 0.96f);
}

inline const char *BkwTfRootLabel(int VoteIndex, const char *pFallback)
{
	switch(VoteIndex)
	{
	case 11: return "Правила";
	case 12: return "Дуэли";
	case 13: return "Почта";
	case 14: return "Награды";
	case 15: return "Рейтинг";
	case 16: return "Уровень";
	case 17: return "Клан";
	case 18: return "Магазин";
	case 19: return "Инвентарь";
	case 20: return "VIP";
	case 21: return "Статистика";
	case 22: return "Настройки";
	default: return pFallback;
	}
}

inline BkwTfMenu::EIconHint BkwTfRootIcon(int VoteIndex, BkwTfMenu::EIconHint Fallback)
{
	switch(VoteIndex)
	{
	case 11: return BkwTfMenu::EIconHint::INFO;
	case 12: return BkwTfMenu::EIconHint::GAME;
	case 13: return BkwTfMenu::EIconHint::MAIL;
	case 14: return BkwTfMenu::EIconHint::GIFT;
	case 15: return BkwTfMenu::EIconHint::TROPHY;
	case 16: return BkwTfMenu::EIconHint::LEVEL;
	case 17: return BkwTfMenu::EIconHint::CLAN;
	case 18: return BkwTfMenu::EIconHint::SHOP;
	case 19: return BkwTfMenu::EIconHint::INVENTORY;
	case 20: return BkwTfMenu::EIconHint::CROWN;
	case 21: return BkwTfMenu::EIconHint::CHART;
	case 22: return BkwTfMenu::EIconHint::SETTINGS;
	default: return Fallback;
	}
}

inline std::string BkwTfFriendlyActionText(const std::string &Text)
{
	if(Text == "Manage Clan") return "Управление кланом";
	if(Text == "View Last Deposits" || Text == "Last Deposits") return "Последние взносы";
	if(Text == "Create Duel Request" || Text == "Create duel request") return "Создать запрос на дуэль";
	if(Text == "Receive items") return "Забрать предметы";
	if(Text == "Delete this mail") return "Удалить письмо";
	if(Text == "Mark all as read") return "Отметить всё прочитанным";
	if(Text == "Receive all items") return "Забрать все предметы";
	if(Text == "Delete read mails") return "Удалить прочитанные";
	if(Text == "Last Deposits:") return "Последние взносы";
	return Text;
}

inline bool BkwTfIsUxNoise(const BkwTfMenu::SItem &Item, const BkwTfMenu::SPage &Page)
{
	if(Item.m_Text.empty())
		return true;
	if(BkwTfMenu::Contains(Item.m_Text, "<пapaмeтp>") || BkwTfMenu::Contains(Item.m_Text, "<parameter>"))
		return true;
	if(BkwTfMenu::Contains(Item.m_Text, "Иcпoльзyйтe пpичинy гoлocoвaния") || BkwTfMenu::Contains(Item.m_Text, "Use vote reason"))
		return true;
	if(Item.m_Kind == BkwTfMenu::EItemKind::SECTION && (Item.m_Text == Page.m_Title || Item.m_Text == "Pʀᴏғɪʟᴇ Iɴғᴏ" || Item.m_Text == "Profile Info"))
		return true;
	return false;
}

inline void BkwTfPolishPage(BkwTfMenu::SPage &Page)
{
	bool HasClanRootAction = false;
	int RewardLinks = 0;
	for(const BkwTfMenu::SItem &Item : Page.m_vItems)
	{
		const std::string &Match = Item.m_Detail.empty() ? Item.m_Text : Item.m_Detail;
		if(BkwTfMenu::StartsWith(Match, "/clan_members") || BkwTfMenu::StartsWith(Match, "/clan_leave") || Item.m_Text == "Manage Clan")
			HasClanRootAction = true;
		if(Item.m_Kind == BkwTfMenu::EItemKind::ACTION && BkwTfMenu::Contains(Item.m_Text, "Нaгpaды зa"))
			++RewardLinks;
	}

	if(Page.m_Title == "Взносы клана" && HasClanRootAction)
	{
		Page.m_Title = "Клан";
		Page.m_Icon = BkwTfMenu::EIconHint::CLAN;
	}
	if(Page.m_Title == "Награды за вход" && RewardLinks >= 3)
	{
		Page.m_Title = "Награды";
		Page.m_Icon = BkwTfMenu::EIconHint::GIFT;
	}

	std::vector<BkwTfMenu::SItem> vClean;
	vClean.reserve(Page.m_vItems.size());
	for(BkwTfMenu::SItem Item : Page.m_vItems)
	{
		if(BkwTfIsUxNoise(Item, Page))
			continue;

		if(Item.m_Kind == BkwTfMenu::EItemKind::ACTION || Item.m_Kind == BkwTfMenu::EItemKind::TOGGLE)
		{
			Item.m_Text = BkwTfFriendlyActionText(Item.m_Text);
			if(Page.m_IsRoot)
			{
				Item.m_Text = BkwTfRootLabel(Item.m_VoteIndex, Item.m_Text.c_str());
				Item.m_Icon = BkwTfRootIcon(Item.m_VoteIndex, Item.m_Icon);
			}
			if(!Item.m_Detail.empty() && BkwTfMenu::StartsWith(Item.m_Detail, "/"))
				Item.m_Detail.clear();
		}
		vClean.push_back(std::move(Item));
	}
	Page.m_vItems = std::move(vClean);

	Page.m_RequiresReason = false;
	for(const BkwTfMenu::SItem &Item : Page.m_vItems)
		Page.m_RequiresReason = Page.m_RequiresReason || Item.m_NeedsReason;
}

inline bool BkwTfRenderHeaderIconButton(CUi *pUi, const CUIRect &Rect, BkwTfSvg::EIcon Icon, CButtonContainer *pButton, bool Interactive, bool Enabled)
{
	const bool Hovered = Interactive && Enabled && pUi->MouseInside(&Rect);
	Rect.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Enabled ? (Hovered ? 0.15f : 0.075f) : 0.025f), IGraphics::CORNER_ALL, 8.0f);
	CUIRect IconRect = Rect;
	IconRect.Margin(8.0f, &IconRect);
	BkwTfSvg::Render(pUi, Icon, IconRect, Enabled ? ColorRGBA(0.90f, 0.91f, 0.93f, 0.96f) : ColorRGBA(0.45f, 0.46f, 0.49f, 0.50f));
	return Interactive && Enabled && pUi->DoButtonLogic(pButton, 0, &Rect, BUTTONFLAG_LEFT);
}

inline float BkwTfItemHeight(const BkwTfMenu::SItem &Item)
{
	switch(Item.m_Kind)
	{
	case BkwTfMenu::EItemKind::SECTION: return 22.0f;
	case BkwTfMenu::EItemKind::NOTE: return 28.0f;
	case BkwTfMenu::EItemKind::INFO: return 30.0f;
	case BkwTfMenu::EItemKind::ACTION:
	case BkwTfMenu::EItemKind::TOGGLE: return Item.m_Detail.empty() ? 33.0f : 39.0f;
	}
	return 32.0f;
}

inline void BkwTfRenderSection(CUi *pUi, const BkwTfMenu::SItem &Item, CUIRect Row)
{
	CUIRect Icon, Label;
	Row.VSplitLeft(22.0f, &Icon, &Label);
	Icon.Margin(5.0f, &Icon);
	BkwTfSvg::Render(pUi, BkwTfSvgIcon(Item.m_Icon), Icon, ColorRGBA(0.66f, 0.68f, 0.72f, 0.82f));
	pUi->DoLabel(&Label, Item.m_Text.c_str(), 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Label.w});
}

inline void BkwTfRenderInfo(CUi *pUi, const BkwTfMenu::SItem &Item, CUIRect Row)
{
	Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Item.m_Warning ? 0.065f : 0.038f), IGraphics::CORNER_ALL, 7.0f);
	Row.Margin(6.0f, &Row);
	CUIRect Icon, Text;
	Row.VSplitLeft(21.0f, &Icon, &Text);
	Icon.Margin(4.0f, &Icon);
	BkwTfSvg::Render(pUi, Item.m_Warning ? BkwTfSvg::EIcon::WARNING : BkwTfSvgIcon(Item.m_Icon), Icon, BkwTfIconColor(Item));
	Text.VSplitLeft(5.0f, nullptr, &Text);

	std::string Key, Value;
	BkwTfMenu::SplitLabelValue(Item.m_Text, Key, Value);
	if(!Key.empty() && !Value.empty())
	{
		CUIRect KeyRect, ValueRect;
		Text.VSplitLeft(minimum(180.0f, Text.w * 0.43f), &KeyRect, &ValueRect);
		pUi->DoLabel(&KeyRect, Key.c_str(), 10.2f, TEXTALIGN_ML, {.m_MaxWidth = KeyRect.w - 4.0f});
		pUi->DoLabel(&ValueRect, Value.c_str(), 10.8f, TEXTALIGN_ML, {.m_MaxWidth = ValueRect.w});
	}
	else
		pUi->DoLabel(&Text, Item.m_Text.c_str(), 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Text.w});
}

inline void BkwTfRenderNote(CUi *pUi, const BkwTfMenu::SItem &Item, CUIRect Row)
{
	Row.Draw(ColorRGBA(Item.m_Warning ? 0.40f : 0.14f, Item.m_Warning ? 0.12f : 0.15f, Item.m_Warning ? 0.10f : 0.18f, 0.30f), IGraphics::CORNER_ALL, 6.0f);
	Row.Margin(6.0f, &Row);
	CUIRect Icon, Text;
	Row.VSplitLeft(20.0f, &Icon, &Text);
	Icon.Margin(4.0f, &Icon);
	BkwTfSvg::Render(pUi, Item.m_Warning ? BkwTfSvg::EIcon::WARNING : BkwTfSvg::EIcon::INFO, Icon, BkwTfIconColor(Item));
	Text.VSplitLeft(5.0f, nullptr, &Text);
	pUi->DoLabel(&Text, Item.m_Text.c_str(), 9.8f, TEXTALIGN_ML, {.m_MaxWidth = Text.w});
}

inline bool BkwTfRenderAction(CUi *pUi, const BkwTfMenu::SItem &Item, CUIRect Row, CButtonContainer *pButton, bool Interactive, bool Waiting)
{
	const bool NeedsValue = Item.m_NeedsReason;
	const bool HasValue = !s_BkwTfReasonInput.IsEmpty();
	const bool Enabled = !Waiting && Item.m_VoteIndex >= 0 && (!NeedsValue || HasValue);
	const bool Hovered = Interactive && Enabled && pUi->MouseInside(&Row);

	ColorRGBA Background(1.0f, 1.0f, 1.0f, Enabled ? (Hovered ? 0.13f : 0.060f) : 0.024f);
	if(Item.m_Negative)
		Background = ColorRGBA(0.55f, 0.12f, 0.10f, Enabled ? (Hovered ? 0.30f : 0.18f) : 0.09f);
	Row.Draw(Background, IGraphics::CORNER_ALL, 7.0f);

	const CUIRect ButtonRect = Row;
	Row.Margin(6.0f, &Row);
	CUIRect Icon, Text, EndIcon;
	Row.VSplitLeft(23.0f, &Icon, &Text);
	Text.VSplitRight(19.0f, &Text, &EndIcon);
	Icon.Margin(4.0f, &Icon);
	EndIcon.Margin(4.0f, &EndIcon);

	if(Item.m_Kind == BkwTfMenu::EItemKind::TOGGLE)
	{
		const BkwTfSvg::EIcon ToggleIcon = Item.m_Negative ? BkwTfSvg::EIcon::CLOSE : (Item.m_Checked ? BkwTfSvg::EIcon::CHECK : BkwTfSvg::EIcon::TOGGLE_OFF);
		BkwTfSvg::Render(pUi, ToggleIcon, Icon, BkwTfIconColor(Item, Enabled));
	}
	else
		BkwTfSvg::Render(pUi, BkwTfSvgIcon(Item.m_Icon), Icon, BkwTfIconColor(Item, Enabled));

	Text.VSplitLeft(5.0f, nullptr, &Text);
	pUi->DoLabel(&Text, Item.m_Text.c_str(), 10.8f, TEXTALIGN_ML, {.m_MaxWidth = Text.w});
	BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::CHEVRON_RIGHT, EndIcon, Enabled ? ColorRGBA(0.73f, 0.75f, 0.80f, 0.72f) : ColorRGBA(0.42f, 0.43f, 0.46f, 0.42f));
	return Interactive && Enabled && pUi->DoButtonLogic(pButton, 0, &ButtonRect, BUTTONFLAG_LEFT);
}

inline void BkwTfRenderReason(CUi *pUi, CUIRect ReasonBar)
{
	ReasonBar.Draw(ColorRGBA(0.055f, 0.058f, 0.065f, 0.96f), IGraphics::CORNER_ALL, 8.0f);
