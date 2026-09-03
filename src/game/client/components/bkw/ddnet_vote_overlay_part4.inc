inline void BkwDdnetRenderPlayers(CUi *pUi, CGameClient *pGameClient, CVoting &Voting, CUIRect Body, bool Interactive, bool Spectate)
{
	CUIRect ReasonRow;
	Body.HSplitTop(34.0f, &ReasonRow, &Body);
	CUIRect ReasonLabel, ReasonEdit;
	ReasonRow.VSplitLeft(70.0f, &ReasonLabel, &ReasonEdit);
	pUi->DoLabel(&ReasonLabel, "Причина:", 10.5f, TEXTALIGN_ML);
	ReasonEdit.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 7.0f);
	ReasonEdit.Margin(5.0f, &ReasonEdit);
	BkwDdnetVoteState::ReasonInput.SetEmptyText("Укажите причину…");
	if(Interactive)
		pUi->DoEditBox(&BkwDdnetVoteState::ReasonInput, &ReasonEdit, 10.5f);
	else
		pUi->DoLabel(&ReasonEdit, BkwDdnetVoteState::ReasonInput.IsEmpty() ? "Укажите причину…" : BkwDdnetVoteState::ReasonInput.GetString(), 10.5f, TEXTALIGN_ML, {.m_MaxWidth = ReasonEdit.w});
	Body.HSplitTop(8.0f, nullptr, &Body);

	std::vector<int> vClients;
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ++ClientId)
	{
		if(!pGameClient->m_aClients[ClientId].m_Active || ClientId == pGameClient->m_Snap.m_LocalClientId)
			continue;
		vClients.push_back(ClientId);
	}
	if(vClients.empty())
	{
		pUi->DoLabel(&Body, "Нет подходящих игроков.", 11.5f, TEXTALIGN_MC);
		return;
	}

	const int VisibleRows = maximum(1, (int)(Body.h / 35.0f));
	const int MaxScroll = maximum(0, (int)vClients.size() - VisibleRows);
	BkwDdnetVoteState::ScrollRow = std::clamp(BkwDdnetVoteState::ScrollRow, 0, MaxScroll);
	if(Interactive && pUi->MouseInside(&Body))
	{
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_UP))
			BkwDdnetVoteState::ScrollRow = maximum(0, BkwDdnetVoteState::ScrollRow - 2);
		if(pUi->ConsumeHotkey(CUi::HOTKEY_SCROLL_DOWN))
			BkwDdnetVoteState::ScrollRow = minimum(MaxScroll, BkwDdnetVoteState::ScrollRow + 2);
	}

	static CButtonContainer s_aKickButtons[MAX_CLIENTS];
	static CButtonContainer s_aSpecButtons[MAX_CLIENTS];
	for(int i = 0; i < VisibleRows && BkwDdnetVoteState::ScrollRow + i < (int)vClients.size(); ++i)
	{
		const int ClientId = vClients[BkwDdnetVoteState::ScrollRow + i];
		CUIRect Row;
		Body.HSplitTop(31.0f, &Row, &Body);
		Body.HSplitTop(4.0f, nullptr, &Body);
		Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.045f), IGraphics::CORNER_ALL, 7.0f);
		Row.Margin(5.0f, &Row);
		CUIRect Name, Action;
		Row.VSplitRight(140.0f, &Name, &Action);
		pUi->DoLabel(&Name, pGameClient->m_aClients[ClientId].m_aName, 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Name.w - 5.0f});
		CButtonContainer *pButton = Spectate ? &s_aSpecButtons[ClientId] : &s_aKickButtons[ClientId];
		if(BkwDdnetSmallButton(pUi, pButton, Spectate ? "В наблюдатели" : "Кик с сервера", Action, Interactive, false, !Spectate, Spectate ? BkwTfSvg::EIcon::USER : BkwTfSvg::EIcon::WARNING))
		{
			const char *pReason = BkwDdnetVoteState::ReasonInput.GetString();
			if(Spectate)
				Voting.CallvoteSpectate(ClientId, pReason);
			else
				Voting.CallvoteKick(ClientId, pReason);
			BkwDdnetVoteState::ReasonInput.Clear();
		}
	}
}

inline void RenderBkwDdnetVoteOverlay(CUi *pUi, CGameClient *pGameClient, const CVoting &VotingConst, bool Interactive)
{
	if(!pUi || !pGameClient)
		return;
	CVoting &Voting = const_cast<CVoting &>(VotingConst);
	if(Interactive)
	{
		pUi->SetEnabled(true);
		pUi->Update();
		BkwDdnetUpdateCatalog();
		if(BkwDdnetVoteState::DetailOpen)
			BkwDdnetUpdateStats();
	}
	BkwDdnetLoadFavorites();

	pUi->MapScreen();
	CUIRect Screen = *pUi->Screen();

	if(BkwDdnetVoteState::DetailOpen && BkwDdnetVoteState::SelectedMap >= 0 && BkwDdnetVoteState::SelectedMap < (int)BkwDdnetVoteState::vThumbnails.size())
	{
		// Completely hide the live game first, then use the selected map screenshot
		// as the detail-page backdrop. Cover preserves the DDNet 360x225 aspect.
		Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f), IGraphics::CORNER_NONE, 0.0f);
		const int MapIndex = BkwDdnetVoteState::SelectedMap;
		if(Interactive)
			BkwDdnetUpdateThumbnail(pUi, MapIndex);
		const auto &Thumb = BkwDdnetVoteState::vThumbnails[MapIndex];
		if(Thumb.m_Texture.IsValid())
			BkwDdnetRenderTextureCover(pUi, Thumb.m_Texture, Screen, 360.0f / 225.0f, 0.74f);
		Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.58f), IGraphics::CORNER_NONE, 0.0f);
	}
	else
	{
		Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, Interactive ? 0.34f : 0.58f), IGraphics::CORNER_NONE, 0.0f);
	}

	const float MenuScale = std::clamp(g_Config.m_BkwDdnetMenuScale / 100.0f, 0.75f, 1.25f);
	const float MenuOpacity = std::clamp(g_Config.m_BkwDdnetMenuOpacity / 100.0f, 0.45f, 1.0f);
	const float Rounding = std::clamp((float)g_Config.m_BkwDdnetMenuRounding, 0.0f, 28.0f);
	const int MenuStyle = std::clamp(g_Config.m_BkwDdnetMenuStyle, 0, 3);
	if(MenuStyle == 1)
		pUi->Graphics()->BlurMenuBackground(g_Config.m_BkwUiGlassBlur);
	CUIRect::SetGlassColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_BkwUiGlassColor)));
	const CUIRect::CScopedGlass Material(MenuStyle == 1 ? 1.0f - g_Config.m_BkwUiGlassTransparency / 100.0f : -1.0f);
	const float CardW = minimum(Screen.w - 18.0f, 820.0f * MenuScale);
	const float CardH = minimum(Screen.h - 14.0f, 480.0f * MenuScale);
	CUIRect Card{Screen.x + (Screen.w - CardW) * 0.5f, Screen.y + (Screen.h - CardH) * 0.5f, CardW, CardH};

	if(MenuStyle == 1)
	{
		// Liquid Glass: translucent dark layers with a thin cool edge, no white bloom.
		Card.Draw(ColorRGBA(0.35f, 0.55f, 0.78f, 0.18f * MenuOpacity), IGraphics::CORNER_ALL, Rounding + 1.0f);
		Card.Margin(1.3f, &Card);
		Card.Draw(ColorRGBA(0.030f, 0.040f, 0.060f, 0.88f * MenuOpacity), IGraphics::CORNER_ALL, maximum(0.0f, Rounding - 1.0f));
	}
	else if(MenuStyle == 2)
	{
		// Neon: dark frame with a restrained cyan/purple accent.
		Card.Draw(ColorRGBA(0.06f, 0.18f, 0.28f, 0.90f * MenuOpacity), IGraphics::CORNER_ALL, Rounding + 1.0f);
		Card.Margin(1.4f, &Card);
		Card.Draw(ColorRGBA(0.018f, 0.020f, 0.034f, 0.97f * MenuOpacity), IGraphics::CORNER_ALL, maximum(0.0f, Rounding - 1.0f));
		CUIRect Accent = Card;
		Accent.h = 2.0f;
		Accent.Draw4(ColorRGBA(0.18f, 0.78f, 1.0f, 0.90f), ColorRGBA(0.82f, 0.25f, 1.0f, 0.90f), ColorRGBA(0.18f, 0.78f, 1.0f, 0.90f), ColorRGBA(0.82f, 0.25f, 1.0f, 0.90f), IGraphics::CORNER_T, maximum(0.0f, Rounding - 1.0f));
	}
	else if(MenuStyle == 3)
	{
		Card.Draw(ColorRGBA(0.018f, 0.020f, 0.024f, 0.92f * MenuOpacity), IGraphics::CORNER_ALL, Rounding);
	}
	else
	{
		// Standard: intentionally dark border, replacing the old bright grey frame.
		Card.Draw(ColorRGBA(0.12f, 0.13f, 0.16f, 0.78f * MenuOpacity), IGraphics::CORNER_ALL, Rounding + 1.0f);
		Card.Margin(1.6f, &Card);
		Card.Draw(ColorRGBA(0.024f, 0.026f, 0.031f, (BkwDdnetVoteState::DetailOpen ? 0.93f : 0.99f) * MenuOpacity), IGraphics::CORNER_ALL, maximum(0.0f, Rounding - 1.0f));
	}
	Card.Margin(13.0f, &Card);

	CUIRect Header, Body;
	Card.HSplitTop(48.0f, &Header, &Body);
	CUIRect Title, Tabs;
	Header.VSplitLeft(215.0f, &Title, &Tabs);
	CUIRect TitleIcon, TitleText;
	Title.VSplitLeft(32.0f, &TitleIcon, &TitleText);
	TitleIcon.Margin(6.0f, &TitleIcon);
	BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::GAME, TitleIcon, ColorRGBA(0.58f, 0.78f, 1.0f, 0.96f));
	pUi->DoLabel(&TitleText, "DDNet Голосование", 16.0f, TEXTALIGN_ML);

	static CButtonContainer s_aTabButtons[3];
	const char *apTabs[3] = {"Карты", "Кик", "Наблюдатели"};
	const BkwTfSvg::EIcon aTabIcons[3] = {BkwTfSvg::EIcon::GAME, BkwTfSvg::EIcon::WARNING, BkwTfSvg::EIcon::USER};
	for(int Tab = 0; Tab < 3; ++Tab)
	{
		CUIRect Button;
		Tabs.VSplitLeft(Tabs.w / (3 - Tab), &Button, &Tabs);
		const bool Active = BkwDdnetVoteState::SelectedTab == Tab;
		const bool Hovered = Interactive && pUi->MouseInside(&Button);
		Button.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Active ? 0.16f : (Hovered ? 0.10f : 0.045f)), IGraphics::CORNER_ALL, 7.0f);

		CUIRect IconRect, LabelRect;
		Button.VSplitLeft(25.0f, &IconRect, &LabelRect);
		IconRect.Margin(6.0f, &IconRect);
		BkwTfSvg::Render(pUi, aTabIcons[Tab], IconRect, ColorRGBA(1.0f, 1.0f, 1.0f, Active ? 1.0f : 0.82f));
		LabelRect.VSplitLeft(1.0f, nullptr, &LabelRect);
		pUi->DoLabel(&LabelRect, apTabs[Tab], 10.0f, TEXTALIGN_MC, {.m_MaxWidth = LabelRect.w - 2.0f});
		if(Interactive && pUi->DoButtonLogic(&s_aTabButtons[Tab], 0, &Button, BUTTONFLAG_LEFT))
		{
			BkwDdnetVoteState::SelectedTab = Tab;
			BkwDdnetVoteState::ScrollRow = 0;
			BkwDdnetVoteState::DetailOpen = false;
		}
		if(Tab < 2)
			Tabs.VSplitLeft(5.0f, nullptr, &Tabs);
	}

	CUIRect Divider;
	Body.HSplitTop(1.0f, &Divider, &Body);
	Divider.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.10f), IGraphics::CORNER_NONE, 0.0f);
	Body.HSplitTop(9.0f, nullptr, &Body);

	if(BkwDdnetVoteState::SelectedTab == 0)
	{
		if(BkwDdnetVoteState::DetailOpen)
			BkwDdnetRenderStats(pUi, pGameClient, Body, Interactive);
		else
			BkwDdnetRenderMapCards(pUi, pGameClient, Body, Interactive);
	}
	else
	{
		BkwDdnetRenderPlayers(pUi, pGameClient, Voting, Body, Interactive, BkwDdnetVoteState::SelectedTab == 2);
	}
}
