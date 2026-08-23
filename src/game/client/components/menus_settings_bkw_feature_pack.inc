#include "bkw/video_map_converter.inc"

void RenderBkwOptimizerSettings(CMenus *pMenus, CUi *pUi, CUIRect PageView)
{
	static int s_Enable;
	static CButtonContainer s_aScope[2];
	static CButtonContainer s_aPreset[3];
	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);

	CUIRect Row;
	PageView.HSplitTop(26.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "BKW Оптимизация", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Enable, "Включить профиль оптимизации", g_Config.m_BkwOptimizer, &Row))
		g_Config.m_BkwOptimizer ^= 1;

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	PageView.HSplitTop(18.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Область", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(30.0f, &Row, &PageView);
	CUIRect Left, Right;
	Row.VSplitMid(&Left, &Right, 6.0f);
	if(pMenus->DoButton_Menu(&s_aScope[0], "Только BKW", g_Config.m_BkwOptimizerScope == 0, &Left))
		g_Config.m_BkwOptimizerScope = 0;
	if(pMenus->DoButton_Menu(&s_aScope[1], "Весь клиент", g_Config.m_BkwOptimizerScope == 1, &Right))
		g_Config.m_BkwOptimizerScope = 1;

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	PageView.HSplitTop(18.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Профиль", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(30.0f, &Row, &PageView);
	const char *apPresets[3] = {"Баланс", "Макс. FPS", "Мин. задержка"};
	for(int i = 0; i < 3; ++i)
	{
		CUIRect Button;
		Row.VSplitLeft(Row.w / (float)(3 - i), &Button, &Row);
		Button.VMargin(2.0f, &Button);
		if(pMenus->DoButton_Menu(&s_aPreset[i], apPresets[i], g_Config.m_BkwOptimizerPreset == i, &Button))
			g_Config.m_BkwOptimizerPreset = i;
	}
	PageView.HSplitTop(8.0f, nullptr, &PageView);
	PageView.HSplitTop(34.0f, &Row, &PageView);
	pUi->DoLabel(&Row, g_Config.m_BkwOptimizerScope == 0 ? "Только BKW: упрощает тяжёлые эффекты BKW без изменения всей игры." : "Весь клиент: использует BestClient Optimizer, FPS fog и профиль задержки.", 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
}

void RenderBkwTipoCheatSettings(CMenus *pMenus, CUi *pUi, CUIRect PageView)
{
	static int s_Enable;
	static int s_Spin;
	static int s_Emotes;
	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);
	CUIRect Row;
	PageView.HSplitTop(26.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "ТипоЧит", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Enable, "Включить ТипоЧит", g_Config.m_BkwTipoCheat, &Row))
		g_Config.m_BkwTipoCheat ^= 1;
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Spin, "Крутилка: автоматический быстрый aim", g_Config.m_BkwTipoCheatSpin, &Row))
		g_Config.m_BkwTipoCheatSpin ^= 1;
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTipoCheatSpinSpeed, &g_Config.m_BkwTipoCheatSpinSpeed, &Row, "Скорость крутилки", 1, 100);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Emotes, "Emote Dance", g_Config.m_BkwTipoCheatEmoteDance, &Row))
		g_Config.m_BkwTipoCheatEmoteDance ^= 1;
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTipoCheatEmoteInterval, &g_Config.m_BkwTipoCheatEmoteInterval, &Row, "Интервал эмодзи", 1200, 10000, &CUi::ms_LinearScrollbarScale, 0, "ms");
	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(34.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Крутилка использует обычный input клиента; Emote Dance не обходит сетевые rate limits.", 10.3f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
}

void RenderBkwHookTargetSettings(CMenus *pMenus, CUi *pUi, CUIRect PageView)
{
	static int s_Enable;
	static CButtonContainer s_aColors[5];
	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);
	CUIRect Row;
	PageView.HSplitTop(26.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Hook Target Indicator", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Enable, "Показывать цель, до которой дотягивается hook", g_Config.m_BkwHookTargetIndicator, &Row))
		g_Config.m_BkwHookTargetIndicator ^= 1;
	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwHookTargetSize, &g_Config.m_BkwHookTargetSize, &Row, "Размер маркера", 32, 96);
	PageView.HSplitTop(7.0f, nullptr, &PageView);
	PageView.HSplitTop(18.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Цвет", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	const char *apNames[5] = {"Зелёный", "Голубой", "Фиолет.", "Жёлтый", "Белый"};
	const unsigned aColors[5] = {0x5533FF66U, 0x5544CCFFU, 0x55BB66FFU, 0x55FFDD55U, 0x55FFFFFFU};
	for(int i = 0; i < 5; ++i)
	{
		CUIRect Button;
		Row.VSplitLeft(Row.w / (float)(5 - i), &Button, &Row);
		Button.VMargin(1.5f, &Button);
		if(pMenus->DoButton_Menu(&s_aColors[i], apNames[i], g_Config.m_BkwHookTargetColor == aColors[i], &Button))
			g_Config.m_BkwHookTargetColor = aColors[i];
	}
	PageView.HSplitTop(7.0f, nullptr, &PageView);
	PageView.HSplitTop(34.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Рисуются только тонкие уголки вокруг цели; сервер этот визуальный маркер не получает.", 10.3f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
}

void RenderBkwAdaptiveAntiPingSettings(CMenus *pMenus, CUi *pUi, IClient *pClient, CUIRect PageView)
{
	static int s_Enable;
	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);
	CUIRect Row;
	PageView.HSplitTop(26.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "BKW Adaptive AntiPing", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Enable, "Автоматически настраивать prediction", g_Config.m_BkwAdaptiveAntiPing, &Row))
		g_Config.m_BkwAdaptiveAntiPing ^= 1;
	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwAdaptiveAntiPingStrength, &g_Config.m_BkwAdaptiveAntiPingStrength, &Row, "Сила адаптации", 0, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
	PageView.HSplitTop(8.0f, nullptr, &PageView);
	PageView.HSplitTop(44.0f, &Row, &PageView);
	char aStatus[192];
	if(pClient->State() == IClient::STATE_ONLINE && g_Config.m_BkwAdaptiveAntiPing)
		str_format(aStatus, sizeof(aStatus), "Ping: %d ms   •   prediction: %d%%   •   margin: %.1f ms", pClient->ServerInfo().m_Latency, g_Config.m_ClAntiPingPercent, g_Config.m_ClPredictionMargin / 10.0f);
	else
		str_copy(aStatus, "Использует встроенный DDNet AntiPing и меняет процент/margin по ping jitter.");
	pUi->DoLabel(&Row, aStatus, 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
}

void RenderBkwAnimatedBackgroundSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, CUIRect PageView)
{
	static int s_MenuEnable;
	static int s_GameEnable;
	static CButtonContainer s_Reload;
	static CButtonContainer s_Convert;
	static CLineInput s_PathInput;
	static CLineInput s_OutputInput;
	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);
	CUIRect Row;
	PageView.HSplitTop(26.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Animated Background / MP4 → .map", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_MenuEnable, "Видео-фон в меню", g_Config.m_BcMenuMediaBackground, &Row))
		g_Config.m_BcMenuMediaBackground ^= 1;
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_GameEnable, "Видео-фон поверх background слоя игры", g_Config.m_BcGameMediaBackground, &Row))
		g_Config.m_BcGameMediaBackground ^= 1;
	PageView.HSplitTop(7.0f, nullptr, &PageView);
	PageView.HSplitTop(30.0f, &Row, &PageView);
	s_PathInput.SetBuffer(g_Config.m_BcMenuMediaBackgroundPath, sizeof(g_Config.m_BcMenuMediaBackgroundPath));
	s_PathInput.SetEmptyText("Путь к .mp4 / .webm / .gif / .png");
	pUi->DoEditBox(&s_PathInput, &Row, 11.0f);
	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(30.0f, &Row, &PageView);
	if(pMenus->DoButton_Menu(&s_Reload, "Перезагрузить видео-фон", 0, &Row))
		pGameClient->m_MenuBackground.ReloadBkwMediaBackground();

	PageView.HSplitTop(11.0f, nullptr, &PageView);
	PageView.HSplitTop(1.0f, &Row, &PageView);
	Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.10f), IGraphics::CORNER_NONE, 0.0f);
	PageView.HSplitTop(9.0f, nullptr, &PageView);
	PageView.HSplitTop(22.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Экспорт в обычную DDNet .map", 14.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);
	PageView.HSplitTop(36.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Кадры упаковываются в embedded RGBA atlas, отображаются quads и переключаются одним Color Envelope с точками Step по alpha.", 10.2f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(30.0f, &Row, &PageView);
	s_OutputInput.SetBuffer(g_Config.m_BkwVideoMapOutput, sizeof(g_Config.m_BkwVideoMapOutput));
	s_OutputInput.SetEmptyText("bkw_video.map");
	pUi->DoEditBox(&s_OutputInput, &Row, 11.0f);

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwVideoMapFps, &g_Config.m_BkwVideoMapFps, &Row, "FPS карты", 5, 30);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwVideoMapDimension, &g_Config.m_BkwVideoMapDimension, &Row, "Макс. сторона кадра", 128, 512, &CUi::ms_LinearScrollbarScale, 0, " px");
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwVideoMapMaxSeconds, &g_Config.m_BkwVideoMapMaxSeconds, &Row, "Длительность", 1, 60, &CUi::ms_LinearScrollbarScale, 0, " s");

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	PageView.HSplitTop(32.0f, &Row, &PageView);
	if(pMenus->DoButton_Menu(&s_Convert, "Конвертировать MP4 → .map", 0, &Row))
	{
		BkwVideoMap::Convert(
			pGameClient->Graphics(),
			pGameClient->Storage(),
			g_Config.m_BcMenuMediaBackgroundPath,
			g_Config.m_BkwVideoMapOutput,
			g_Config.m_BkwVideoMapFps,
			g_Config.m_BkwVideoMapDimension,
			g_Config.m_BkwVideoMapMaxSeconds);
	}
	PageView.HSplitTop(7.0f, nullptr, &PageView);
	PageView.HSplitTop(46.0f, &Row, &PageView);
	pUi->DoLabel(&Row, BkwVideoMap::s_aStatus, 10.2f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(38.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Результат сохраняется в maps/<имя>.map и открывается обычным редактором: группа BKW Video, atlas layers, quads и BKW Video Step envelope остаются редактируемыми.", 10.0f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
}
