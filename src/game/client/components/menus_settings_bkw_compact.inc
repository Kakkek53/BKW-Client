struct SBkwCompactToggleState
{
	int m_CheckboxId = 0;
};

inline void RenderBkwCompactToggleRow(CMenus *pMenus, CUi *pUi, CUIRect Row, const char *pTitle, int *pEnabled, SBkwCompactToggleState &State)
{
	Row.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.40f), IGraphics::CORNER_ALL, 6.0f);
	Row.Margin(5.0f, &Row);
	if(pEnabled)
	{
		if(pMenus->DoButton_CheckBox(&State.m_CheckboxId, pTitle, *pEnabled, &Row))
			*pEnabled ^= 1;
	}
	else
		pUi->DoLabel(&Row, pTitle, 14.0f, TEXTALIGN_ML);
}

inline void RenderBkwTeeFusionHeader(CMenus *pMenus, CUi *pUi, CUIRect Row)
{
	static int s_MenuToggle;
	static int s_DuelToggle;
	Row.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.40f), IGraphics::CORNER_ALL, 6.0f);
	Row.Margin(6.0f, &Row);
	CUIRect Title, Toggles;
	Row.HSplitTop(18.0f, &Title, &Toggles);
	pUi->DoLabel(&Title, "TeeFusion", 15.0f, TEXTALIGN_ML);
	CUIRect MenuToggle, DuelToggle;
	Toggles.VSplitMid(&MenuToggle, &DuelToggle, 5.0f);
	if(pMenus->DoButton_CheckBox(&s_MenuToggle, "Menu", g_Config.m_BkwTfMenu, &MenuToggle))
		g_Config.m_BkwTfMenu ^= 1;
	if(pMenus->DoButton_CheckBox(&s_DuelToggle, "Duel HUD", g_Config.m_BkwTfDuelHud, &DuelToggle))
		g_Config.m_BkwTfDuelHud ^= 1;
}
