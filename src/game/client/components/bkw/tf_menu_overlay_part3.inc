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
	Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, Interactive ? 0.34f : 0.58f), IGraphics::CORNER_NONE, 0.0f);

	const float MenuScale = std::clamp(g_Config.m_BkwTfMenuScale / 100.0f, 0.75f, 1.25f);
	const float MenuOpacity = std::clamp(g_Config.m_BkwTfMenuOpacity / 100.0f, 0.45f, 1.0f);
	const float Rounding = std::clamp((float)g_Config.m_BkwTfMenuRounding, 0.0f, 28.0f);
	const int MenuStyle = std::clamp(g_Config.m_BkwTfMenuStyle, 0, 3);
	const float CardW = minimum(Screen.w - 18.0f, 820.0f * MenuScale);
	const float CardH = minimum(Screen.h - 14.0f, 480.0f * MenuScale);
	CUIRect Card{Screen.x + (Screen.w - CardW) * 0.5f, Screen.y + (Screen.h - CardH) * 0.5f, CardW, CardH};

	if(MenuStyle == 1)
	{
		// Liquid Glass: cool thin edge + translucent dark glass. Deliberately no
		// large white top/bottom glow that the previous outer frame produced.
		Card.Draw(ColorRGBA(0.34f, 0.52f, 0.72f, 0.17f * MenuOpacity), IGraphics::CORNER_ALL, Rounding + 1.0f);
		Card.Margin(1.2f, &Card);
		Card.Draw(ColorRGBA(0.030f, 0.040f, 0.058f, 0.88f * MenuOpacity), IGraphics::CORNER_ALL, maximum(0.0f, Rounding - 1.0f));
	}
	else if(MenuStyle == 2)
	{
		Card.Draw(ColorRGBA(0.055f, 0.16f, 0.25f, 0.88f * MenuOpacity), IGraphics::CORNER_ALL, Rounding + 1.0f);
		Card.Margin(1.3f, &Card);
		Card.Draw(ColorRGBA(0.018f, 0.020f, 0.034f, 0.97f * MenuOpacity), IGraphics::CORNER_ALL, maximum(0.0f, Rounding - 1.0f));
		CUIRect Accent = Card;
		Accent.h = 2.0f;
		Accent.Draw4(ColorRGBA(0.18f, 0.78f, 1.0f, 0.88f), ColorRGBA(0.82f, 0.25f, 1.0f, 0.88f), ColorRGBA(0.18f, 0.78f, 1.0f, 0.88f), ColorRGBA(0.82f, 0.25f, 1.0f, 0.88f), IGraphics::CORNER_T, maximum(0.0f, Rounding - 1.0f));
	}
	else if(MenuStyle == 3)
	{
		Card.Draw(ColorRGBA(0.018f, 0.020f, 0.024f, 0.92f * MenuOpacity), IGraphics::CORNER_ALL, Rounding);
	}
	else
	{
		// Standard uses a dark neutral outline instead of the old light-grey frame.
		Card.Draw(ColorRGBA(0.11f, 0.12f, 0.145f, 0.76f * MenuOpacity), IGraphics::CORNER_ALL, Rounding + 1.0f);
		Card.Margin(1.5f, &Card);
		Card.Draw(ColorRGBA(0.024f, 0.026f, 0.031f, 0.995f * MenuOpacity), IGraphics::CORNER_ALL, maximum(0.0f, Rounding - 1.0f));
	}
	Card.Margin(13.0f, &Card);

	BkwTfMenu::SPage Page;
	const bool HasCompleteVoteList = Voting.NumOptions() > 0 && !Voting.IsReceivingOptions();
	if(HasCompleteVoteList)
	{
		Page = BkwTfMenu::ParsePage(Voting);
		BkwTfPolishPage(Page);
	}

	CUIRect Header, Body;
	Card.HSplitTop(45.0f, &Header, &Body);

	CUIRect PageIcon, Title, Nav;
	Header.VSplitLeft(34.0f, &PageIcon, &Header);
	PageIcon.Margin(5.0f, &PageIcon);
	BkwTfSvg::Render(pUi, HasCompleteVoteList ? BkwTfSvgIcon(Page.m_Icon) : BkwTfSvg::EIcon::REFRESH, PageIcon, ColorRGBA(0.90f, 0.91f, 0.93f, 0.98f));
	Header.VSplitLeft(7.0f, nullptr, &Header);
	Header.VSplitRight(74.0f, &Title, &Nav);

	CUIRect MainTitle, Subtitle;
	Title.HSplitTop(24.0f, &MainTitle, &Subtitle);
	pUi->DoLabel(&MainTitle, HasCompleteVoteList ? Page.m_Title.c_str() : "TeeFusion", 18.5f, TEXTALIGN_ML, {.m_MaxWidth = MainTitle.w});
	pUi->DoLabel(&Subtitle, "TF Menu", 9.3f, TEXTALIGN_ML, {.m_MaxWidth = Subtitle.w});

	static CButtonContainer s_HomeButton;
	static CButtonContainer s_BackButton;
	CUIRect BackButton, HomeButton;
	Nav.VSplitLeft(34.0f, &BackButton, &Nav);
	Nav.VSplitRight(34.0f, &Nav, &HomeButton);
	if(HasCompleteVoteList)
	{
		const bool CanBack = Page.m_BackVoteIndex >= 0 && !s_BkwTfMenuWaitingForUpdate;
		const bool CanHome = Page.m_HomeVoteIndex >= 0 && !Page.m_IsRoot && !s_BkwTfMenuWaitingForUpdate;
		if(BkwTfRenderHeaderIconButton(pUi, BackButton, BkwTfSvg::EIcon::BACK, &s_BackButton, Interactive, CanBack))
			BkwTfBeginVoteAction(Voting, Page.m_BackVoteIndex);
		if(BkwTfRenderHeaderIconButton(pUi, HomeButton, BkwTfSvg::EIcon::HOME, &s_HomeButton, Interactive, CanHome))
			BkwTfBeginVoteAction(Voting, Page.m_HomeVoteIndex);
	}
	else
	{
		BkwTfRenderHeaderIconButton(pUi, BackButton, BkwTfSvg::EIcon::BACK, &s_BackButton, false, false);
		BkwTfRenderHeaderIconButton(pUi, HomeButton, BkwTfSvg::EIcon::HOME, &s_HomeButton, false, false);
	}

	CUIRect Divider;
	Body.HSplitTop(1.0f, &Divider, &Body);
	Divider.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.095f), IGraphics::CORNER_NONE, 0.0f);
	Body.HSplitTop(9.0f, nullptr, &Body);

	if(!HasCompleteVoteList || s_BkwTfMenuWaitingForUpdate)
	{
		CUIRect LoadingIcon{Body.x + Body.w * 0.5f - 17.0f, Body.y + Body.h * 0.34f, 34.0f, 34.0f};
		BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::REFRESH, LoadingIcon, ColorRGBA(0.82f, 0.84f, 0.88f, 0.92f));
		CUIRect LoadingText = Body;
		LoadingText.HSplitTop(Body.h * 0.34f + 46.0f, nullptr, &LoadingText);
		pUi->DoLabel(&LoadingText, s_BkwTfMenuWaitingForUpdate ? "Обновляю…" : "Получаю данные TeeFusion…", 11.5f, TEXTALIGN_TC);
		return;
	}

	if(Page.m_IsRoot)
	{
		BkwTfRenderRoot(pUi, Voting, Page, Body, Interactive);
		return;
	}

	CUIRect ReasonBar;
	if(Page.m_RequiresReason)
	{
		Body.HSplitBottom(41.0f, &Body, &ReasonBar);
		Body.HSplitBottom(7.0f, &Body, nullptr);
	}
	else if(s_BkwTfReasonInput.IsActive())
	{
		s_BkwTfReasonInput.Deactivate();
		s_BkwTfReasonInput.Clear();
	}

	Body.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.024f), IGraphics::CORNER_ALL, 10.0f);
	Body.Margin(9.0f, &Body);
	BkwTfRenderPageItems(pUi, Voting, Page, Body, Interactive);

	if(Page.m_RequiresReason)
		BkwTfRenderReason(pUi, ReasonBar);
}
