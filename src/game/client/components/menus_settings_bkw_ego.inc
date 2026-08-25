void RenderBkwEgoMapDetailsSettings(CMenus *pMenus, CUi *pUi, CUIRect PageView)
{
	static int s_Enable;
	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);

	CUIRect Row;
	PageView.HSplitTop(26.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "EGO • статистика карт", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Enable, "Показывать «Подробнее» у карт EGO", g_Config.m_BkwEgoMapDetails, &Row))
		g_Config.m_BkwEgoMapDetails ^= 1;

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	PageView.HSplitTop(44.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Стандартное меню голосования не заменяется. Кнопка появляется только у карт в разделе Maps (...) на серверах EGO и показывает данные eternal-gores.com.", 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
}
