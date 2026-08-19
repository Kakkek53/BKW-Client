#ifndef GAME_CLIENT_COMPONENTS_BKW_TF_MENU_OVERLAY_INC
#define GAME_CLIENT_COMPONENTS_BKW_TF_MENU_OVERLAY_INC

#include <base/math.h>
#include <base/str.h>
#include <base/system.h>

#include <engine/client/client.h>
#include <engine/serverbrowser.h>

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
	CGameClient *pGameClient = BkwTfGameClient(pUi);
	IClient *pClient = pUi ? pUi->Client() : nullptr;
	if(!pGameClient || !pClient || pClient->State() != IClient::STATE_ONLINE)
		return false;

	if(str_comp(pClient->ServerInfo().m_aCommunityId, "teefusion") == 0)
		return true;

	IServerBrowser *pServerBrowser = pGameClient->ServerBrowser();
	if(!pServerBrowser)
		return false;
	IServerBrowser::CServerEntry *pEntry = pServerBrowser->Find(pClient->ServerAddress());
	return pEntry && str_comp(pEntry->m_Info.m_aCommunityId, "teefusion") == 0;
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
	if(Item.m_Icon == BkwTfMenu::EIconHint::SHOP || Item.m_Icon == BkwTfMenu::EIconHint::COINS || Item.m_Icon == BkwTfMenu::EIconHint::GIFT)
		return ColorRGBA(1.0f, 0.78f, 0.32f, 0.98f);
	return ColorRGBA(0.56f, 0.78f, 1.0f, 0.96f);
}

inline float BkwTfItemHeight(const BkwTfMenu::SItem &Item)
{
	switch(Item.m_Kind)
	{
	case BkwTfMenu::EItemKind::SECTION: return 25.0f;
	case BkwTfMenu::EItemKind::NOTE: return 30.0f;
	case BkwTfMenu::EItemKind::INFO: return 31.0f;
	case BkwTfMenu::EItemKind::ACTION:
	case BkwTfMenu::EItemKind::TOGGLE: return Item.m_Detail.empty() ? 35.0f : 42.0f;
	}
	return 34.0f;
}

inline bool BkwTfRenderNavButton(CUi *pUi, const CUIRect &Rect, BkwTfSvg::EIcon Icon, const char *pText, CButtonContainer *pButton, bool Interactive, bool Enabled)
{
	const bool Hovered = Interactive && Enabled && pUi->MouseInside(&Rect);
	Rect.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Enabled ? (Hovered ? 0.13f : 0.070f) : 0.025f), IGraphics::CORNER_ALL, 7.0f);

	CUIRect IconRect, Label;
	Rect.VSplitLeft(Rect.h, &IconRect, &Label);
	IconRect.Margin(7.0f, &IconRect);
	Label.VSplitLeft(2.0f, nullptr, &Label);
	BkwTfSvg::Render(pUi, Icon, IconRect, Enabled ? ColorRGBA(0.75f, 0.86f, 1.0f, 0.96f) : ColorRGBA(0.45f, 0.46f, 0.48f, 0.50f));
	pUi->DoLabel(&Label, pText, 10.5f, TEXTALIGN_ML);
	return Interactive && Enabled && pUi->DoButtonLogic(pButton, 0, &Rect, BUTTONFLAG_LEFT);
}

inline void BkwTfRenderSection(CUi *pUi, const BkwTfMenu::SItem &Item, CUIRect Row)
{
	CUIRect Icon, Label;
	Row.VSplitLeft(25.0f, &Icon, &Label);
	Icon.Margin(5.0f, &Icon);
	BkwTfSvg::Render(pUi, BkwTfSvgIcon(Item.m_Icon), Icon, ColorRGBA(0.62f, 0.80f, 1.0f, 0.82f));
	pUi->DoLabel(&Label, Item.m_Text.c_str(), 11.4f, TEXTALIGN_ML);
}

inline void BkwTfRenderInfo(CUi *pUi, const BkwTfMenu::SItem &Item, CUIRect Row)
{
	Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Item.m_Warning ? 0.065f : 0.042f), IGraphics::CORNER_ALL, 7.0f);
	Row.Margin(6.0f, &Row);
	CUIRect Icon, Text;
	Row.VSplitLeft(23.0f, &Icon, &Text);
	Icon.Margin(4.0f, &Icon);
	BkwTfSvg::Render(pUi, Item.m_Warning ? BkwTfSvg::EIcon::WARNING : BkwTfSvgIcon(Item.m_Icon), Icon, BkwTfIconColor(Item));
	Text.VSplitLeft(5.0f, nullptr, &Text);

	std::string Key, Value;
	BkwTfMenu::SplitLabelValue(Item.m_Text, Key, Value);
	if(!Key.empty() && !Value.empty())
	{
		CUIRect KeyRect, ValueRect;
		Text.VSplitLeft(minimum(185.0f, Text.w * 0.44f), &KeyRect, &ValueRect);
		pUi->DoLabel(&KeyRect, Key.c_str(), 10.6f, TEXTALIGN_ML, {.m_MaxWidth = KeyRect.w - 4.0f});
		pUi->DoLabel(&ValueRect, Value.c_str(), 11.2f, TEXTALIGN_ML, {.m_MaxWidth = ValueRect.w});
	}
	else
	{
		pUi->DoLabel(&Text, Item.m_Text.c_str(), 10.9f, TEXTALIGN_ML, {.m_MaxWidth = Text.w});
	}
}

inline void BkwTfRenderNote(CUi *pUi, const BkwTfMenu::SItem &Item, CUIRect Row)
{
	Row.Draw(ColorRGBA(Item.m_Warning ? 0.40f : 0.16f, Item.m_Warning ? 0.12f : 0.18f, Item.m_Warning ? 0.10f : 0.23f, 0.34f), IGraphics::CORNER_ALL, 6.0f);
	Row.Margin(6.0f, &Row);
	CUIRect Icon, Text;
	Row.VSplitLeft(22.0f, &Icon, &Text);
	Icon.Margin(4.0f, &Icon);
	BkwTfSvg::Render(pUi, Item.m_Warning ? BkwTfSvg::EIcon::WARNING : BkwTfSvg::EIcon::INFO, Icon, BkwTfIconColor(Item));
	Text.VSplitLeft(5.0f, nullptr, &Text);
	pUi->DoLabel(&Text, Item.m_Text.c_str(), 10.2f, TEXTALIGN_ML, {.m_MaxWidth = Text.w});
}

inline bool BkwTfRenderAction(CUi *pUi, const BkwTfMenu::SItem &Item, CUIRect Row, CButtonContainer *pButton, bool Interactive, bool Waiting)
{
	const bool NeedsValue = Item.m_NeedsReason;
	const bool HasValue = !s_BkwTfReasonInput.IsEmpty();
	const bool Enabled = !Waiting && Item.m_VoteIndex >= 0 && (!NeedsValue || HasValue);
	const bool Hovered = Interactive && Enabled && pUi->MouseInside(&Row);

	ColorRGBA Background(1.0f, 1.0f, 1.0f, Enabled ? (Hovered ? 0.135f : 0.070f) : 0.027f);
	if(Item.m_Negative)
		Background = ColorRGBA(0.55f, 0.12f, 0.10f, Enabled ? (Hovered ? 0.32f : 0.20f) : 0.10f);
	Row.Draw(Background, IGraphics::CORNER_ALL, 7.0f);

	const CUIRect ButtonRect = Row;
	Row.Margin(6.0f, &Row);
	CUIRect Icon, Text, EndIcon;
	Row.VSplitLeft(24.0f, &Icon, &Text);
	Text.VSplitRight(22.0f, &Text, &EndIcon);
	Icon.Margin(4.0f, &Icon);
	EndIcon.Margin(5.0f, &EndIcon);

	if(Item.m_Kind == BkwTfMenu::EItemKind::TOGGLE)
	{
		const BkwTfSvg::EIcon ToggleIcon = Item.m_Negative ? BkwTfSvg::EIcon::CLOSE : (Item.m_Checked ? BkwTfSvg::EIcon::CHECK : BkwTfSvg::EIcon::TOGGLE_OFF);
		BkwTfSvg::Render(pUi, ToggleIcon, Icon, BkwTfIconColor(Item, Enabled));
	}
	else
	{
		BkwTfSvg::Render(pUi, BkwTfSvgIcon(Item.m_Icon), Icon, BkwTfIconColor(Item, Enabled));
	}

	Text.VSplitLeft(6.0f, nullptr, &Text);
	if(Item.m_Detail.empty())
	{
		pUi->DoLabel(&Text, Item.m_Text.c_str(), 11.0f, TEXTALIGN_ML, {.m_MaxWidth = Text.w});
	}
	else
	{
		CUIRect MainText, DetailText;
		Text.HSplitMid(&MainText, &DetailText, 1.0f);
		pUi->DoLabel(&MainText, Item.m_Text.c_str(), 10.9f, TEXTALIGN_ML, {.m_MaxWidth = MainText.w});
		pUi->DoLabel(&DetailText, Item.m_Detail.c_str(), 8.8f, TEXTALIGN_ML, {.m_MaxWidth = DetailText.w});
	}

	BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::CHEVRON_RIGHT, EndIcon, Enabled ? ColorRGBA(0.75f, 0.82f, 0.91f, 0.72f) : ColorRGBA(0.42f, 0.43f, 0.46f, 0.42f));
	return Interactive && Enabled && pUi->DoButtonLogic(pButton, 0, &ButtonRect, BUTTONFLAG_LEFT);
}

inline void BkwTfRenderReason(CUi *pUi, CUIRect ReasonBar)
{
	ReasonBar.Draw(ColorRGBA(0.05f, 0.06f, 0.08f, 0.92f), IGraphics::CORNER_ALL, 8.0f);
	ReasonBar.Margin(6.0f, &ReasonBar);

	CUIRect Icon, Label, Input;
	ReasonBar.VSplitLeft(24.0f, &Icon, &ReasonBar);
	Icon.Margin(4.0f, &Icon);
	BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::USER, Icon, ColorRGBA(0.61f, 0.80f, 1.0f, 0.95f));
	ReasonBar.VSplitLeft(6.0f, nullptr, &ReasonBar);
	ReasonBar.VSplitLeft(76.0f, &Label, &Input);
	pUi->DoLabel(&Label, "Параметр", 10.5f, TEXTALIGN_ML);
	Input.VSplitLeft(4.0f, nullptr, &Input);
	s_BkwTfReasonInput.SetEmptyText("ник / ID / значение");
	pUi->DoEditBox(&s_BkwTfReasonInput, &Input, 10.5f);
}

inline int BkwTfVisibleRows(const BkwTfMenu::SPage &Page, float Height, int Start)
{
	float Used = 0.0f;
	int Count = 0;
	for(int i = maximum(0, Start); i < static_cast<int>(Page.m_vItems.size()); ++i)
	{
		const float H = BkwTfItemHeight(Page.m_vItems[i]) + 4.0f;
		if(Count > 0 && Used + H > Height)
			break;
		Used += H;
		++Count;
	}
	return maximum(1, Count);
}

inline void BkwTfRenderPageItems(CUi *pUi, CVoting &Voting, const BkwTfMenu::SPage &Page, CUIRect Content, bool Interactive)
{
	if(Page.m_vItems.empty())
	{
		CUIRect Empty = Content;
		Empty.Margin(14.0f, &Empty);
		CUIRect EmptyIcon{Empty.x + Empty.w * 0.5f - 13.0f, Empty.y + 16.0f, 26.0f, 26.0f};
		BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::INFO, EmptyIcon, ColorRGBA(0.58f, 0.76f, 1.0f, 0.82f));
		Empty.HSplitTop(55.0f, nullptr, &Empty);
		pUi->DoLabel(&Empty, "TeeFusion не прислал элементов для этой страницы.", 11.5f, TEXTALIGN_TC);
		return;
	}

	const int MaxStart = maximum(0, static_cast<int>(Page.m_vItems.size()) - 1);
	s_BkwTfMenuScrollRow = std::clamp(s_BkwTfMenuScrollRow, 0, MaxStart);
	int VisibleRows = BkwTfVisibleRows(Page, Content.h, s_BkwTfMenuScrollRow);
	if(s_BkwTfMenuScrollRow + VisibleRows > static_cast<int>(Page.m_vItems.size()))
	{
		s_BkwTfMenuScrollRow = maximum(0, static_cast<int>(Page.m_vItems.size()) - VisibleRows);
		VisibleRows = BkwTfVisibleRows(Page, Content.h, s_BkwTfMenuScrollRow);
	}

	if(Interactive && pUi->MouseInside(&Content))
	{
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_UP))
			s_BkwTfMenuScrollRow = maximum(0, s_BkwTfMenuScrollRow - 2);
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_DOWN))
			s_BkwTfMenuScrollRow = minimum(MaxStart, s_BkwTfMenuScrollRow + 2);
	}

	static CButtonContainer s_aItemButtons[MAX_VOTE_OPTIONS];
	const CUIRect OriginalContent = Content;
	const int First = s_BkwTfMenuScrollRow;
	const int Last = minimum(static_cast<int>(Page.m_vItems.size()), First + VisibleRows);
	for(int i = First; i < Last; ++i)
	{
		const BkwTfMenu::SItem &Item = Page.m_vItems[i];
		const float RowH = BkwTfItemHeight(Item);
		CUIRect Row;
		Content.HSplitTop(RowH, &Row, &Content);
		Content.HSplitTop(4.0f, nullptr, &Content);

		switch(Item.m_Kind)
		{
		case BkwTfMenu::EItemKind::SECTION:
			BkwTfRenderSection(pUi, Item, Row);
			break;
		case BkwTfMenu::EItemKind::INFO:
			BkwTfRenderInfo(pUi, Item, Row);
			break;
		case BkwTfMenu::EItemKind::NOTE:
			BkwTfRenderNote(pUi, Item, Row);
			break;
		case BkwTfMenu::EItemKind::ACTION:
		case BkwTfMenu::EItemKind::TOGGLE:
		{
			CButtonContainer *pButton = Item.m_VoteIndex >= 0 && Item.m_VoteIndex < MAX_VOTE_OPTIONS ? &s_aItemButtons[Item.m_VoteIndex] : &s_aItemButtons[0];
			if(BkwTfRenderAction(pUi, Item, Row, pButton, Interactive, s_BkwTfMenuWaitingForUpdate))
			{
				const char *pReason = Item.m_NeedsReason ? s_BkwTfReasonInput.GetString() : "";
				BkwTfBeginVoteAction(Voting, Item.m_VoteIndex, pReason);
				if(Item.m_NeedsReason)
					s_BkwTfReasonInput.Clear();
			}
			break;
		}
		}
	}

	if(Page.m_vItems.size() > static_cast<size_t>(VisibleRows))
	{
		const float TrackW = 3.0f;
		CUIRect Track{OriginalContent.x + OriginalContent.w - TrackW, OriginalContent.y, TrackW, OriginalContent.h};
		Track.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 2.0f);
		const float Fraction = VisibleRows / static_cast<float>(Page.m_vItems.size());
		const float ThumbH = maximum(14.0f, Track.h * Fraction);
		const float MaxOffset = maximum(1.0f, static_cast<float>(Page.m_vItems.size() - VisibleRows));
		const float ThumbY = Track.y + (Track.h - ThumbH) * (s_BkwTfMenuScrollRow / MaxOffset);
		CUIRect Thumb{Track.x, ThumbY, Track.w, ThumbH};
		Thumb.Draw(ColorRGBA(0.58f, 0.76f, 1.0f, 0.52f), IGraphics::CORNER_ALL, 2.0f);
	}
}

inline void RenderBkwTfMenuOverlay(CUi *pUi, const CVoting &VotingConst, bool Interactive = true)
{
	s_pBkwTfMenuLastUi = pUi;
	s_BkwTfMenuConnectedToTeeFusion = BkwTfIsConnectedTeeFusion(pUi);

	if(!s_BkwTfMenuConnectedToTeeFusion)
	{
		if(Interactive && s_BkwTfMenuOverlayActive)
		{
			CGameClient *pGameClient = BkwTfGameClient(pUi);
			BkwTfMenuOverlayReset(false);
			s_BkwTfMenuOverlaySuppressReopen = true;
			if(pGameClient)
			{
				pGameClient->m_Menus.SetGamePage(CMenus::PAGE_CALLVOTE);
				pGameClient->m_Menus.SetActive(true);
			}
			pUi->SetActiveItem(nullptr);
		}
		return;
	}

	CVoting &Voting = const_cast<CVoting &>(VotingConst);
	if(Interactive)
	{
		pUi->SetEnabled(true);
		pUi->Update();
		BkwTfUpdateVoteTransition(Voting);
	}

	pUi->MapScreen();
	CUIRect Screen = *pUi->Screen();
	Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, Interactive ? 0.30f : 0.55f), IGraphics::CORNER_NONE, 0.0f);

	const float CardW = minimum(Screen.w - 20.0f, 760.0f);
	const float CardH = minimum(Screen.h - 16.0f, 455.0f);
	CUIRect Card{Screen.x + (Screen.w - CardW) * 0.5f, Screen.y + (Screen.h - CardH) * 0.5f, CardW, CardH};
	Card.Draw(ColorRGBA(0.58f, 0.62f, 0.70f, 0.42f), IGraphics::CORNER_ALL, 17.0f);
	Card.Margin(1.4f, &Card);
	Card.Draw(ColorRGBA(0.026f, 0.029f, 0.035f, 0.993f), IGraphics::CORNER_ALL, 16.0f);
	Card.Margin(12.0f, &Card);

	BkwTfMenu::SPage Page;
	const bool HasCompleteVoteList = Voting.NumOptions() > 0 && !Voting.IsReceivingOptions();
	if(HasCompleteVoteList)
		Page = BkwTfMenu::ParsePage(Voting);

	CUIRect Header, Body;
	Card.HSplitTop(46.0f, &Header, &Body);

	CUIRect PageIcon, Title, Nav;
	Header.VSplitLeft(36.0f, &PageIcon, &Header);
	PageIcon.Margin(5.0f, &PageIcon);
	BkwTfSvg::Render(pUi, HasCompleteVoteList ? BkwTfSvgIcon(Page.m_Icon) : BkwTfSvg::EIcon::REFRESH, PageIcon, ColorRGBA(0.62f, 0.81f, 1.0f, 0.98f));
	Header.VSplitLeft(6.0f, nullptr, &Header);
	Header.VSplitRight(224.0f, &Title, &Nav);

	CUIRect MainTitle, Subtitle;
	Title.HSplitTop(25.0f, &MainTitle, &Subtitle);
	pUi->DoLabel(&MainTitle, HasCompleteVoteList ? Page.m_Title.c_str() : "TeeFusion", 19.0f, TEXTALIGN_ML, {.m_MaxWidth = MainTitle.w});
	char aSubtitle[128];
	str_format(aSubtitle, sizeof(aSubtitle), "TF Menu — %d пунктов — ESC в игру", Voting.NumOptions());
	pUi->DoLabel(&Subtitle, aSubtitle, 9.5f, TEXTALIGN_ML, {.m_MaxWidth = Subtitle.w});

	static CButtonContainer s_HomeButton;
	static CButtonContainer s_BackButton;
	CUIRect HomeButton, BackButton;
	Nav.VSplitMid(&BackButton, &HomeButton, 6.0f);
	if(HasCompleteVoteList)
	{
		const bool CanBack = Page.m_BackVoteIndex >= 0 && !s_BkwTfMenuWaitingForUpdate;
		const bool CanHome = Page.m_HomeVoteIndex >= 0 && !Page.m_IsRoot && !s_BkwTfMenuWaitingForUpdate;
		if(BkwTfRenderNavButton(pUi, BackButton, BkwTfSvg::EIcon::BACK, "Назад", &s_BackButton, Interactive, CanBack))
			BkwTfBeginVoteAction(Voting, Page.m_BackVoteIndex);
		if(BkwTfRenderNavButton(pUi, HomeButton, BkwTfSvg::EIcon::HOME, "Главная", &s_HomeButton, Interactive, CanHome))
			BkwTfBeginVoteAction(Voting, Page.m_HomeVoteIndex);
	}
	else
	{
		BkwTfRenderNavButton(pUi, BackButton, BkwTfSvg::EIcon::BACK, "Назад", &s_BackButton, false, false);
		BkwTfRenderNavButton(pUi, HomeButton, BkwTfSvg::EIcon::HOME, "Главная", &s_HomeButton, false, false);
	}

	CUIRect Divider;
	Body.HSplitTop(1.0f, &Divider, &Body);
	Divider.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.105f), IGraphics::CORNER_NONE, 0.0f);
	Body.HSplitTop(9.0f, nullptr, &Body);

	if(!HasCompleteVoteList || s_BkwTfMenuWaitingForUpdate)
	{
		CUIRect LoadingIcon{Body.x + Body.w * 0.5f - 18.0f, Body.y + Body.h * 0.34f, 36.0f, 36.0f};
		BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::REFRESH, LoadingIcon, ColorRGBA(0.62f, 0.80f, 1.0f, 0.92f));
		CUIRect LoadingText = Body;
		LoadingText.HSplitTop(Body.h * 0.34f + 48.0f, nullptr, &LoadingText);
		pUi->DoLabel(&LoadingText, s_BkwTfMenuWaitingForUpdate ? "Обновляю страницу TeeFusion…" : "Получаю меню TeeFusion…", 12.0f, TEXTALIGN_TC);
		return;
	}

	CUIRect ReasonBar;
	if(Page.m_RequiresReason)
	{
		Body.HSplitBottom(43.0f, &Body, &ReasonBar);
		Body.HSplitBottom(7.0f, &Body, nullptr);
	}
	else if(s_BkwTfReasonInput.IsActive())
	{
		s_BkwTfReasonInput.Deactivate();
		s_BkwTfReasonInput.Clear();
	}

	Body.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.027f), IGraphics::CORNER_ALL, 10.0f);
	Body.Margin(9.0f, &Body);
	BkwTfRenderPageItems(pUi, Voting, Page, Body, Interactive);

	if(Page.m_RequiresReason)
		BkwTfRenderReason(pUi, ReasonBar);
}

#endif
