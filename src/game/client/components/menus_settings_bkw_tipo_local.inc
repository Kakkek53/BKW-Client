void RenderBkwTipoCheatSettings(CMenus *pMenus, CUi *pUi, CUIRect PageView)
{
	static int s_Enable;
	static int s_Spin;
	static int s_Emotes;

	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);
	CUIRect Row;

	PageView.HSplitTop(26.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "ТипоЧит • локальный", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);

	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Enable, "Включить локальные визуальные эффекты", g_Config.m_BkwTipoCheat, &Row))
		g_Config.m_BkwTipoCheat ^= 1;

	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Spin, "Крутилка вокруг моего tee (только экран)", g_Config.m_BkwTipoCheatSpin, &Row))
		g_Config.m_BkwTipoCheatSpin ^= 1;

	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTipoCheatSpinSpeed, &g_Config.m_BkwTipoCheatSpinSpeed, &Row, "Скорость визуальной крутилки", 1, 100);

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Emotes, "Локальная Emote Dance над моим tee", g_Config.m_BkwTipoCheatEmoteDance, &Row))
		g_Config.m_BkwTipoCheatEmoteDance ^= 1;

	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTipoCheatEmoteInterval, &g_Config.m_BkwTipoCheatEmoteInterval, &Row, "Интервал локальной анимации", 1200, 10000, &CUi::ms_LinearScrollbarScale, 0, "ms");

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(42.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "100% локально: не меняет aim/input и не отправляет emote или другие TipoCheat-действия на сервер. Эффекты видите только вы.", 10.1f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
}
