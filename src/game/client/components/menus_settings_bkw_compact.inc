struct SBkwCompactModuleState
{
	bool m_Open = false;
	int m_CheckboxId = 0;
	CButtonContainer m_OpenButton;
	CButtonContainer m_CollapseButton;
};

inline void RenderBkwCompactModuleRow(CMenus *pMenus, CUi *pUi, CUIRect Row, const char *pTitle, int *pEnabled, SBkwCompactModuleState &State)
{
	Row.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	Row.Margin(8.0f, &Row);

	CUIRect Main, Expand;
	Row.VSplitRight(34.0f, &Main, &Expand);
	Main.VSplitRight(6.0f, &Main, nullptr);
	Expand.HMargin(1.0f, &Expand);

	if(pEnabled)
	{
		if(pMenus->DoButton_CheckBox(&State.m_CheckboxId, pTitle, *pEnabled, &Main))
		{
			*pEnabled ^= 1;
			// Match BestClient-style progressive settings: enabling a module reveals
			// its additional options immediately, disabling folds it back down.
			State.m_Open = *pEnabled != 0;
		}
	}
	else
	{
		pUi->DoLabel(&Main, pTitle, 15.0f, TEXTALIGN_ML);
	}

	if(pMenus->DoButton_Menu(&State.m_OpenButton, "+", 0, &Expand, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f))
		State.m_Open = true;
}

inline void RenderBkwCompactCollapseButton(CMenus *pMenus, CUIRect Section, SBkwCompactModuleState &State)
{
	// Existing expanded cards keep all of their controls (including Reset).
	// Put the compact collapse control just to the left of the usual reset area.
	CUIRect Inner = Section;
	Inner.Margin(10.0f, &Inner);
	CUIRect TitleRow;
	Inner.HSplitTop(26.0f, &TitleRow, nullptr);
	CUIRect RightArea;
	TitleRow.VSplitRight(126.0f, nullptr, &RightArea);
	CUIRect Collapse;
	RightArea.VSplitLeft(28.0f, &Collapse, nullptr);
	Collapse.HMargin(1.0f, &Collapse);
	if(pMenus->DoButton_Menu(&State.m_CollapseButton, "-", 0, &Collapse, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f))
		State.m_Open = false;
}
