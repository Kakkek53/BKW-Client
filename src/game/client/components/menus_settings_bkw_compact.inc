struct SBkwCompactModuleState
{
	bool m_Open = false;
	int m_CheckboxId = 0;
	CButtonContainer m_OpenButton;
	CButtonContainer m_CollapseButton;
};

inline void RenderBkwCompactModuleRow(CMenus *pMenus, CUi *pUi, CUIRect Row, const char *pTitle, int *pEnabled, SBkwCompactModuleState &State)
{
	Row.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.40f), IGraphics::CORNER_ALL, 6.0f);
	Row.Margin(5.0f, &Row);

	CUIRect Main, Expand;
	Row.VSplitRight(28.0f, &Main, &Expand);
	Main.VSplitRight(5.0f, &Main, nullptr);
	Expand.HMargin(1.0f, &Expand);

	if(pEnabled)
	{
		if(pMenus->DoButton_CheckBox(&State.m_CheckboxId, pTitle, *pEnabled, &Main))
		{
			*pEnabled ^= 1;
			State.m_Open = *pEnabled != 0;
		}
	}
	else
	{
		pUi->DoLabel(&Main, pTitle, 14.0f, TEXTALIGN_ML);
	}

	if(pMenus->DoButton_Menu(&State.m_OpenButton, "+", 0, &Expand, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 4.0f))
		State.m_Open = true;
}

inline void RenderBkwCompactCollapseButton(CMenus *pMenus, CUIRect Section, SBkwCompactModuleState &State)
{
	CUIRect Inner = Section;
	Inner.Margin(10.0f, &Inner);
	CUIRect TitleRow;
	Inner.HSplitTop(26.0f, &TitleRow, nullptr);
	CUIRect RightArea;
	TitleRow.VSplitRight(126.0f, nullptr, &RightArea);
	CUIRect Collapse;
	RightArea.VSplitLeft(28.0f, &Collapse, nullptr);
	Collapse.HMargin(1.0f, &Collapse);
	if(pMenus->DoButton_Menu(&State.m_CollapseButton, "-", 0, &Collapse, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 4.0f))
		State.m_Open = false;
}
