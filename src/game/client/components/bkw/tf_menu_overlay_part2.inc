	ReasonBar.Margin(6.0f, &ReasonBar);
	CUIRect Icon, Label, Input;
	ReasonBar.VSplitLeft(22.0f, &Icon, &ReasonBar);
	Icon.Margin(4.0f, &Icon);
	BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::USER, Icon, ColorRGBA(0.86f, 0.87f, 0.90f, 0.95f));
	ReasonBar.VSplitLeft(5.0f, nullptr, &ReasonBar);
	ReasonBar.VSplitLeft(70.0f, &Label, &Input);
	pUi->DoLabel(&Label, "Параметр", 10.2f, TEXTALIGN_ML);
	Input.VSplitLeft(4.0f, nullptr, &Input);
	s_BkwTfReasonInput.SetEmptyText("ник / ID / значение");
	pUi->DoEditBox(&s_BkwTfReasonInput, &Input, 10.2f);
}

inline void BkwTfRenderRootTile(CUi *pUi, CVoting &Voting, const BkwTfMenu::SItem &Item, CUIRect Tile, CButtonContainer *pButton, bool Interactive)
{
	const bool Enabled = !s_BkwTfMenuWaitingForUpdate && Item.m_VoteIndex >= 0;
	const bool Hovered = Interactive && Enabled && pUi->MouseInside(&Tile);
	Tile.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Enabled ? (Hovered ? 0.14f : 0.060f) : 0.025f), IGraphics::CORNER_ALL, 9.0f);
	const CUIRect ButtonRect = Tile;
	Tile.Margin(8.0f, &Tile);
	CUIRect Icon, Label, Arrow;
	Tile.VSplitLeft(30.0f, &Icon, &Label);
	Label.VSplitRight(16.0f, &Label, &Arrow);
	Icon.Margin(4.0f, &Icon);
	Arrow.Margin(3.0f, &Arrow);
	BkwTfSvg::Render(pUi, BkwTfSvgIcon(Item.m_Icon), Icon, ColorRGBA(0.88f, 0.89f, 0.92f, 0.96f));
	Label.VSplitLeft(7.0f, nullptr, &Label);
	pUi->DoLabel(&Label, Item.m_Text.c_str(), 11.0f, TEXTALIGN_ML, {.m_MaxWidth = Label.w});
	BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::CHEVRON_RIGHT, Arrow, ColorRGBA(0.66f, 0.68f, 0.72f, 0.76f));
	if(Interactive && Enabled && pUi->DoButtonLogic(pButton, 0, &ButtonRect, BUTTONFLAG_LEFT))
		BkwTfBeginVoteAction(Voting, Item.m_VoteIndex);
}

inline void BkwTfRenderRoot(CUi *pUi, CVoting &Voting, const BkwTfMenu::SPage &Page, CUIRect Body, bool Interactive)
{
	CUIRect Profile, Sections;
	Body.VSplitLeft(Body.w * 0.39f, &Profile, &Sections);
	Profile.VSplitRight(6.0f, &Profile, nullptr);
	Sections.VSplitLeft(6.0f, nullptr, &Sections);

	Profile.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.030f), IGraphics::CORNER_ALL, 10.0f);
	Sections.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.030f), IGraphics::CORNER_ALL, 10.0f);
	Profile.Margin(10.0f, &Profile);
	Sections.Margin(10.0f, &Sections);

	CUIRect ProfileTitle;
	Profile.HSplitTop(27.0f, &ProfileTitle, &Profile);
	pUi->DoLabel(&ProfileTitle, "Профиль", 13.0f, TEXTALIGN_ML);

	int InfoCount = 0;
	for(const BkwTfMenu::SItem &Item : Page.m_vItems)
	{
		if(Item.m_Kind != BkwTfMenu::EItemKind::INFO)
			continue;
		CUIRect Row;
		Profile.HSplitTop(30.0f, &Row, &Profile);
		Profile.HSplitTop(4.0f, nullptr, &Profile);
		BkwTfRenderInfo(pUi, Item, Row);
		if(++InfoCount >= 8)
			break;
	}

	CUIRect SectionTitle;
	Sections.HSplitTop(27.0f, &SectionTitle, &Sections);
	pUi->DoLabel(&SectionTitle, "Разделы", 13.0f, TEXTALIGN_ML);

	std::vector<const BkwTfMenu::SItem *> vpActions;
	for(const BkwTfMenu::SItem &Item : Page.m_vItems)
	{
		if(Item.m_Kind == BkwTfMenu::EItemKind::ACTION || Item.m_Kind == BkwTfMenu::EItemKind::TOGGLE)
			vpActions.push_back(&Item);
	}

	static CButtonContainer s_aRootButtons[MAX_VOTE_OPTIONS];
	const float Gap = 6.0f;
	const float TileH = 46.0f;
	for(size_t i = 0; i < vpActions.size(); i += 2)
	{
		CUIRect Row;
		Sections.HSplitTop(TileH, &Row, &Sections);
		if(i + 2 < vpActions.size())
			Sections.HSplitTop(Gap, nullptr, &Sections);
		CUIRect Left, Right;
		Row.VSplitMid(&Left, &Right, Gap);
		const BkwTfMenu::SItem &LeftItem = *vpActions[i];
		CButtonContainer *pLeftButton = LeftItem.m_VoteIndex >= 0 && LeftItem.m_VoteIndex < MAX_VOTE_OPTIONS ? &s_aRootButtons[LeftItem.m_VoteIndex] : &s_aRootButtons[0];
		BkwTfRenderRootTile(pUi, Voting, LeftItem, Left, pLeftButton, Interactive);
		if(i + 1 < vpActions.size())
		{
			const BkwTfMenu::SItem &RightItem = *vpActions[i + 1];
			CButtonContainer *pRightButton = RightItem.m_VoteIndex >= 0 && RightItem.m_VoteIndex < MAX_VOTE_OPTIONS ? &s_aRootButtons[RightItem.m_VoteIndex] : &s_aRootButtons[0];
			BkwTfRenderRootTile(pUi, Voting, RightItem, Right, pRightButton, Interactive);
		}
	}
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
		CUIRect EmptyIcon{Content.x + Content.w * 0.5f - 13.0f, Content.y + 22.0f, 26.0f, 26.0f};
		BkwTfSvg::Render(pUi, BkwTfSvg::EIcon::INFO, EmptyIcon, ColorRGBA(0.72f, 0.74f, 0.78f, 0.82f));
		Content.HSplitTop(58.0f, nullptr, &Content);
		pUi->DoLabel(&Content, "На этой странице пока нет доступных действий.", 11.2f, TEXTALIGN_TC);
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
		case BkwTfMenu::EItemKind::SECTION: BkwTfRenderSection(pUi, Item, Row); break;
		case BkwTfMenu::EItemKind::INFO: BkwTfRenderInfo(pUi, Item, Row); break;
		case BkwTfMenu::EItemKind::NOTE: BkwTfRenderNote(pUi, Item, Row); break;
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
		Thumb.Draw(ColorRGBA(0.72f, 0.74f, 0.78f, 0.46f), IGraphics::CORNER_ALL, 2.0f);
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
