#ifndef GAME_CLIENT_COMPONENTS_BKW_TF_MENU_OVERLAY_INC
#define GAME_CLIENT_COMPONENTS_BKW_TF_MENU_OVERLAY_INC

#include <base/str.h>
#include <base/system.h>

#include <engine/client/client.h>
#include <engine/serverbrowser.h>

#include <game/client/components/bkw/tf_menu_parser.inc>
#include <game/client/components/voting.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

#include <algorithm>
#include <cstdint>
#include <vector>

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

	// Prefer the current server info when it already carries the community tag.
	if(str_comp(pClient->ServerInfo().m_aCommunityId, "teefusion") == 0)
		return true;

	// Otherwise resolve the actually connected address against the server list
	// loaded by the client's active master. This deliberately does not assume
	// ddnet.org, BestClient's master, a server name, or a hardcoded TeeFusion IP.
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

	// Leaving TF Menu must not leave the normal menu parked on PAGE_CALLVOTE.
	// Set the ingame page back to Game while keeping the menu itself closed.
	if(ReturnToGame && s_pBkwTfMenuLastUi)
	{
		if(CGameClient *pGameClient = BkwTfGameClient(s_pBkwTfMenuLastUi))
			pGameClient->m_Menus.SetMenuPage(CMenus::PAGE_GAME);
	}

	// CFastActions forwards input to CUi before handling Escape. Consume the
	// hotkey here while the TF overlay is closing so CMenus::OnRender cannot see
	// the same Escape and immediately reopen the regular in-game/vote menu.
	if(WasActive && s_pBkwTfMenuLastUi)
		s_pBkwTfMenuLastUi->ConsumeHotkey(CUi::HOTKEY_ESCAPE);
}

inline const CVoteOptionClient *BkwTfVoteOptionAt(const CVoting &Voting, int WantedIndex)
{
	int Index = 0;
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
	{
		if(Index == WantedIndex)
			return pOption;
	}
	return nullptr;
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

inline bool BkwTfIsMainRoot(const CVoting &Voting)
{
	char aUserName[64];
	return BkwTfMenu::ParseUserName(Voting, aUserName, sizeof(aUserName));
}

inline void BkwTfCopyValueAt(const CVoting &Voting, int Index, char *pBuffer, size_t BufferSize)
{
	if(!pBuffer || BufferSize == 0)
		return;
	pBuffer[0] = '\0';

	const CVoteOptionClient *pOption = BkwTfVoteOptionAt(Voting, Index);
	if(!pOption)
	{
		str_copy(pBuffer, "—", BufferSize);
		return;
	}

	const char *pValue = str_find(pOption->m_aDescription, ":");
	if(!pValue)
	{
		str_copy(pBuffer, pOption->m_aDescription, BufferSize);
		return;
	}
	++pValue;
	while(*pValue == ' ' || *pValue == '\t' || *pValue == '-' || *pValue == '|')
		++pValue;

	str_copy(pBuffer, *pValue ? pValue : "—", BufferSize);
	int Length = str_length(pBuffer);
	while(Length > 0 && (pBuffer[Length - 1] == ' ' || pBuffer[Length - 1] == '\t' || pBuffer[Length - 1] == '\r' || pBuffer[Length - 1] == '\n' || pBuffer[Length - 1] == '|'))
		pBuffer[--Length] = '\0';
}

inline void BkwTfCleanDisplayText(const char *pSource, char *pBuffer, size_t BufferSize)
{
	if(!pBuffer || BufferSize == 0)
		return;
	pBuffer[0] = '\0';
	if(!pSource)
		return;

	const char *pText = pSource;
	while(*pText == ' ' || *pText == '\t')
		++pText;
	if(str_startswith(pText, "│"))
	{
		pText += str_length("│");
		while(*pText == ' ' || *pText == '\t')
			++pText;
	}
	if(str_startswith(pText, "►"))
	{
		pText += str_length("►");
		while(*pText == ' ' || *pText == '\t')
			++pText;
	}

	str_copy(pBuffer, pText, BufferSize);
	int Length = str_length(pBuffer);
	while(Length > 0 && (pBuffer[Length - 1] == ' ' || pBuffer[Length - 1] == '\t' || pBuffer[Length - 1] == '\r' || pBuffer[Length - 1] == '\n'))
		pBuffer[--Length] = '\0';
}

inline bool BkwTfIsDecorativeOption(const char *pText)
{
	if(!pText || !pText[0])
		return true;
	if(str_startswith(pText, "╭") || str_startswith(pText, "╰") || str_startswith(pText, "─"))
		return true;
	if(str_startswith(pText, "◄"))
		return true;
	return false;
}

inline bool BkwTfIsMailEntry(const char *pText)
{
	if(!pText)
		return false;
	while(*pText == ' ' || *pText == '\t')
		++pText;
	if(*pText < '0' || *pText > '9')
		return false;
	while(*pText >= '0' && *pText <= '9')
		++pText;
	return *pText == '.';
}

inline bool BkwTfOptionClickable(const char *pText, int SelectedTab)
{
	if(!pText || !pText[0] || SelectedTab == 1)
		return false; // Rules are informational.

	if(str_find(pText, "►") || str_find(pText, "[✔]") || str_find(pText, "[  ]"))
		return true;

	if(SelectedTab == 3)
	{
		if(str_find(pText, "✔") || str_find(pText, "⬇") || str_find(pText, "✘") || BkwTfIsMailEntry(pText))
			return true;
	}
	return false;
}

inline void BkwTfBeginVoteAction(CVoting &Voting, int OptionIndex)
{
	if(s_BkwTfMenuWaitingForUpdate || OptionIndex < 0 || OptionIndex >= Voting.NumOptions())
		return;

	BkwTfPulseMenuInput();
	s_BkwTfMenuVoteSignatureBeforeAction = BkwTfVoteSignature(Voting);
	s_BkwTfMenuActionStartedAt = time_get();
	s_BkwTfMenuWaitingForUpdate = true;
	Voting.CallvoteOption(OptionIndex, "");
}

inline void BkwTfUpdateVoteTransition(CVoting &Voting)
{
	if(s_BkwTfMenuWaitingForUpdate)
	{
		const bool TimedOut = time_get() - s_BkwTfMenuActionStartedAt > time_freq() * 3;
		const bool GotNewCompleteList = Voting.NumOptions() > 0 && !Voting.IsReceivingOptions() &&
			BkwTfVoteSignature(Voting) != s_BkwTfMenuVoteSignatureBeforeAction;
		if(GotNewCompleteList || TimedOut)
		{
			s_BkwTfMenuWaitingForUpdate = false;
			s_BkwTfMenuScrollRow = 0;
		}
	}

	if(s_BkwTfMenuWaitingForUpdate || !BkwTfIsMainRoot(Voting))
		return;

	if(s_BkwTfMenuPendingTab >= 0)
	{
		const int TargetTab = s_BkwTfMenuPendingTab;
		s_BkwTfMenuPendingTab = -1;
		s_BkwTfMenuSelectedTab = TargetTab;
		if(TargetTab > 0)
			BkwTfBeginVoteAction(Voting, 10 + TargetTab);
		return;
	}

	if(s_BkwTfMenuSelectedTab != 0)
		s_BkwTfMenuSelectedTab = 0;
}

inline void BkwTfRequestTab(CVoting &Voting, int TargetTab)
{
	if(TargetTab < 0 || TargetTab > 3 || s_BkwTfMenuWaitingForUpdate)
		return;

	if(BkwTfIsMainRoot(Voting))
	{
		s_BkwTfMenuSelectedTab = TargetTab;
		s_BkwTfMenuPendingTab = -1;
		s_BkwTfMenuScrollRow = 0;
		if(TargetTab > 0)
			BkwTfBeginVoteAction(Voting, 10 + TargetTab);
		return;
	}

	s_BkwTfMenuPendingTab = TargetTab;
	s_BkwTfMenuSelectedTab = TargetTab;
	BkwTfBeginVoteAction(Voting, 0);
}

inline void BkwTfRenderMain(CUi *pUi, const CVoting &Voting, CUIRect Content)
{
	CUIRect Title;
	Content.HSplitTop(30.0f, &Title, &Content);
	pUi->DoLabel(&Title, "Главная", 20.0f, TEXTALIGN_ML);
	Content.HSplitTop(5.0f, nullptr, &Content);

	if(!BkwTfIsMainRoot(Voting))
	{
		pUi->DoLabel(&Content, "Возвращаюсь в главное меню TeeFusion…", 12.0f, TEXTALIGN_MC);
		return;
	}

	static const char *s_apInfoLabels[8] = {
		"Имя пользователя", "Discord", "Пин-код", "Язык",
		"Рейтинг", "Уровень", "Опыт", "Валюты"};

	const float Spacing = 3.0f;
	const float RowH = minimum(25.0f, maximum(17.0f, (Content.h - Spacing * 7.0f) / 8.0f));
	for(int Info = 0; Info < 8; ++Info)
	{
		CUIRect Row;
		Content.HSplitTop(RowH, &Row, &Content);
		if(Info != 7)
			Content.HSplitTop(Spacing, nullptr, &Content);
		Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Info % 2 == 0 ? 0.060f : 0.038f), IGraphics::CORNER_ALL, 6.0f);
		Row.VMargin(9.0f, &Row);

		CUIRect Key, Value;
		Row.VSplitLeft(minimum(150.0f, Row.w * 0.43f), &Key, &Value);
		pUi->DoLabel(&Key, s_apInfoLabels[Info], 11.0f, TEXTALIGN_ML);

		char aValue[192];
		BkwTfCopyValueAt(Voting, Info + 1, aValue, sizeof(aValue));
		pUi->DoLabel(&Value, aValue, 11.5f, TEXTALIGN_ML);
	}
}

inline void BkwTfRenderServerPage(CUi *pUi, CVoting &Voting, CUIRect Content, int SelectedTab, bool Interactive)
{
	static const char *s_apTitles[4] = {"Главная", "Правила", "Дуэли", "Почта"};
	CUIRect Header, BackButton;
	Content.HSplitTop(30.0f, &Header, &Content);
	Header.VSplitRight(88.0f, &Header, &BackButton);
	pUi->DoLabel(&Header, s_apTitles[SelectedTab], 20.0f, TEXTALIGN_ML);

	const bool IsRoot = BkwTfIsMainRoot(Voting);
	const CVoteOptionClient *pBack = BkwTfVoteOptionAt(Voting, 1);
	if(!IsRoot && pBack && pBack->m_aDescription[0])
	{
		const bool Hovered = pUi->MouseInside(&BackButton);
		BackButton.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Hovered ? 0.13f : 0.07f), IGraphics::CORNER_ALL, 6.0f);
		pUi->DoLabel(&BackButton, "← Назад", 10.5f, TEXTALIGN_MC);
		static CButtonContainer s_BackButton;
		if(Interactive && !s_BkwTfMenuWaitingForUpdate && pUi->DoButtonLogic(&s_BackButton, 0, &BackButton, BUTTONFLAG_LEFT))
			BkwTfBeginVoteAction(Voting, 1);
	}

	Content.HSplitTop(5.0f, nullptr, &Content);
	if(s_BkwTfMenuWaitingForUpdate || Voting.IsReceivingOptions() || Voting.NumOptions() <= 0 || IsRoot)
	{
		pUi->DoLabel(&Content, "Получаю раздел TeeFusion…", 12.0f, TEXTALIGN_MC);
		return;
	}

	std::vector<int> vRows;
	vRows.reserve(Voting.NumOptions());
	int Index = 0;
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
	{
		if(Index <= 2 || BkwTfIsDecorativeOption(pOption->m_aDescription))
			continue;
		vRows.push_back(Index);
	}

	if(vRows.empty())
	{
		pUi->DoLabel(&Content, "В этом разделе пока нет элементов.", 12.0f, TEXTALIGN_MC);
		return;
	}

	constexpr float RowH = 23.0f;
	constexpr float RowGap = 3.0f;
	const int VisibleRows = maximum(1, static_cast<int>((Content.h + RowGap) / (RowH + RowGap)));
	const int MaxScroll = maximum(0, static_cast<int>(vRows.size()) - VisibleRows);
	s_BkwTfMenuScrollRow = std::clamp(s_BkwTfMenuScrollRow, 0, MaxScroll);

	if(Interactive && pUi->MouseInside(&Content))
	{
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_UP))
			s_BkwTfMenuScrollRow = maximum(0, s_BkwTfMenuScrollRow - 2);
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_DOWN))
			s_BkwTfMenuScrollRow = minimum(MaxScroll, s_BkwTfMenuScrollRow + 2);
	}

	static CButtonContainer s_aOptionButtons[MAX_VOTE_OPTIONS];
	for(int Visible = 0; Visible < VisibleRows && s_BkwTfMenuScrollRow + Visible < static_cast<int>(vRows.size()); ++Visible)
	{
		const int VoteIndex = vRows[s_BkwTfMenuScrollRow + Visible];
		const CVoteOptionClient *pOption = BkwTfVoteOptionAt(Voting, VoteIndex);
		if(!pOption)
			continue;

		CUIRect Row;
		Content.HSplitTop(RowH, &Row, &Content);
		Content.HSplitTop(RowGap, nullptr, &Content);
		const bool Clickable = BkwTfOptionClickable(pOption->m_aDescription, SelectedTab);
		const bool Hovered = Clickable && pUi->MouseInside(&Row);
		Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Hovered ? 0.13f : (Clickable ? 0.070f : 0.040f)), IGraphics::CORNER_ALL, 6.0f);

		char aText[VOTE_DESC_LENGTH];
		BkwTfCleanDisplayText(pOption->m_aDescription, aText, sizeof(aText));
		CUIRect Label = Row;
		Label.VMargin(9.0f, &Label);
		pUi->DoLabel(&Label, aText, Clickable ? 11.0f : 10.7f, TEXTALIGN_ML);

		if(Interactive && Clickable && !s_BkwTfMenuWaitingForUpdate && pUi->DoButtonLogic(&s_aOptionButtons[VoteIndex], 0, &Row, BUTTONFLAG_LEFT))
			BkwTfBeginVoteAction(Voting, VoteIndex);
	}

	if(MaxScroll > 0)
	{
		char aPage[64];
		str_format(aPage, sizeof(aPage), "%d–%d / %d", s_BkwTfMenuScrollRow + 1,
			minimum(static_cast<int>(vRows.size()), s_BkwTfMenuScrollRow + VisibleRows), static_cast<int>(vRows.size()));
		CUIRect PageLabel{Content.x, Content.y + maximum(0.0f, Content.h - 13.0f), Content.w, 13.0f};
		pUi->DoLabel(&PageLabel, aPage, 9.0f, TEXTALIGN_MR);
	}
}

inline void RenderBkwTfMenuOverlay(CUi *pUi, const CVoting &VotingConst, bool Interactive = true)
{
	s_pBkwTfMenuLastUi = pUi;
	s_BkwTfMenuConnectedToTeeFusion = BkwTfIsConnectedTeeFusion(pUi);

	// If this is not a TeeFusion server according to the active master list,
	// immediately hand control back to DDNet's native Call vote page.
	if(!s_BkwTfMenuConnectedToTeeFusion)
	{
		if(Interactive && s_BkwTfMenuOverlayActive)
		{
			CGameClient *pGameClient = BkwTfGameClient(pUi);
			BkwTfMenuOverlayReset(false);
			s_BkwTfMenuOverlaySuppressReopen = true;
			if(pGameClient)
			{
				pGameClient->m_Menus.SetMenuPage(CMenus::PAGE_CALLVOTE);
				pGameClient->m_Menus.SetActive(true);
			}
			pUi->SetActiveItem(nullptr);
		}
		return;
	}

	CVoting &Voting = const_cast<CVoting &>(VotingConst);
	if(Interactive)
	{
		// The normal menu is hidden while TF Menu is open, so drive the shared UI
		// state here. OnCursorMove only updates the pending window-space mouse
		// position; Update converts it into Ui()->MousePos and advances button edges.
		pUi->SetEnabled(true);
		pUi->Update();
		BkwTfUpdateVoteTransition(Voting);
	}

	pUi->MapScreen();
	CUIRect Screen = *pUi->Screen();
	Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, Interactive ? 0.30f : 0.55f), IGraphics::CORNER_NONE, 0.0f);

	const float CardW = minimum(Screen.w - 22.0f, 650.0f);
	const float CardH = minimum(Screen.h - 16.0f, 284.0f);
	CUIRect Card{Screen.x + (Screen.w - CardW) * 0.5f, Screen.y + (Screen.h - CardH) * 0.5f, CardW, CardH};
	Card.Draw(ColorRGBA(0.56f, 0.57f, 0.60f, 0.44f), IGraphics::CORNER_ALL, 16.0f);
	Card.Margin(1.3f, &Card);
	Card.Draw(ColorRGBA(0.028f, 0.030f, 0.035f, 0.992f), IGraphics::CORNER_ALL, 15.0f);

	Card.Margin(12.0f, &Card);
	CUIRect Header, Body;
	Card.HSplitTop(37.0f, &Header, &Body);
	CUIRect Title, Subtitle;
	Header.VSplitLeft(230.0f, &Title, &Subtitle);
	pUi->DoLabel(&Title, "TeeFusion", 22.0f, TEXTALIGN_ML);
	pUi->DoLabel(&Subtitle, "TF Menu  •  ESC — в игру", 10.5f, TEXTALIGN_MR);

	CUIRect Divider;
	Body.HSplitTop(1.0f, &Divider, &Body);
	Divider.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.11f), IGraphics::CORNER_NONE, 0.0f);
	Body.HSplitTop(8.0f, nullptr, &Body);

	CUIRect Nav, Content;
	Body.VSplitLeft(150.0f, &Nav, &Content);
	Nav.VSplitRight(9.0f, &Nav, nullptr);
	Content.VSplitLeft(4.0f, nullptr, &Content);
	Nav.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.040f), IGraphics::CORNER_ALL, 10.0f);
	Content.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 10.0f);

	static const char *s_apTabs[4] = {"Главная", "Правила", "Дуэли", "Почта"};
	static CButtonContainer s_aTabButtons[4];
	CUIRect NavInner;
	Nav.Margin(7.0f, &NavInner);
	for(int Tab = 0; Tab < 4; ++Tab)
	{
		CUIRect Row;
		NavInner.HSplitTop(34.0f, &Row, &NavInner);
		NavInner.HSplitTop(5.0f, nullptr, &NavInner);
		const bool Active = s_BkwTfMenuSelectedTab == Tab;
		const bool Hovered = Interactive && pUi->MouseInside(&Row);
		Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Active ? 0.18f : (Hovered ? 0.10f : 0.045f)), IGraphics::CORNER_ALL, 7.0f);
		CUIRect Label = Row;
		Label.VMargin(9.0f, &Label);
		pUi->DoLabel(&Label, s_apTabs[Tab], Active ? 12.0f : 11.5f, TEXTALIGN_ML);
		if(Interactive && pUi->DoButtonLogic(&s_aTabButtons[Tab], Active, &Row, BUTTONFLAG_LEFT))
			BkwTfRequestTab(Voting, Tab);
	}

	CUIRect ContentInner;
	Content.Margin(12.0f, &ContentInner);
	if(s_BkwTfMenuSelectedTab == 0)
		BkwTfRenderMain(pUi, Voting, ContentInner);
	else
		BkwTfRenderServerPage(pUi, Voting, ContentInner, s_BkwTfMenuSelectedTab, Interactive);
}

#endif
