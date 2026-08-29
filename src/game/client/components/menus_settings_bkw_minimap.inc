void RenderBkwMiniMapSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, CUIRect PageView)
{
	static int s_Enable;
	static int s_RealDesign;
	static CButtonContainer s_KeyReader;
	static CButtonContainer s_KeyClear;

	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.44f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(9.0f, &PageView);

	CUIRect Row;
	PageView.HSplitTop(24.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "BKW Mini Map", 17.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);

	PageView.HSplitTop(22.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Enable, "Показывать круглую мини-карту справа сверху", g_Config.m_BkwMiniMap, &Row))
		g_Config.m_BkwMiniMap ^= 1;

	if(!g_Config.m_BkwMiniMap)
		return;

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwMiniMapSize, &g_Config.m_BkwMiniMapSize, &Row, "Размер мини-карты", 120, 300);

	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwMiniMapOpacity, &g_Config.m_BkwMiniMapOpacity, &Row, "Прозрачность", 20, 100, &CUi::ms_LinearScrollbarScale, 0, "%");

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(22.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_RealDesign, "Реальные тайлы и фон в мини-карте и Full Map (экспериментально)", g_Config.m_BkwMiniMapRealDesign, &Row))
		g_Config.m_BkwMiniMapRealDesign ^= 1;

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(18.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Клавиша полной карты", 11.5f, TEXTALIGN_ML);
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	const CBindSlot CurrentBind(g_Config.m_BkwMiniMapFullKey, g_Config.m_BkwMiniMapFullModifier);
	const CKeyBinder::CKeyReaderResult KeyResult = pGameClient->m_KeyBinder.DoKeyReader(&s_KeyReader, &s_KeyClear, &Row, CurrentBind, false);
	if(!KeyResult.m_Aborted && KeyResult.m_Bind != CurrentBind)
	{
		g_Config.m_BkwMiniMapFullKey = KeyResult.m_Bind.m_Key;
		g_Config.m_BkwMiniMapFullModifier = KeyResult.m_Bind.m_ModifierMask;
	}

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwMiniMapFullZoom, &g_Config.m_BkwMiniMapFullZoom, &Row, "Начальный масштаб полной карты", 100, 500, &CUi::ms_LinearScrollbarScale, 0, "%");

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(44.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Без real-design мини-карта показывает точные collision-тайлы без склеивания проходов. С real-design обе карты используют настоящие слои и фон текущей карты; игроки остаются поверх.", 10.0f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
}