void RenderBkwLocalDuelSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, CUIRect PageView)
{
	static int s_Enable;
	static int s_PublicScore;
	static int s_Hud;
	static int s_Infinite;
	static CButtonContainer s_SelectPlayer;
	static CButtonContainer s_StartReset;
	static CButtonContainer s_Stop;
	static CButtonContainer s_KeyReader;
	static CButtonContainer s_KeyClear;
	static CButtonContainer s_aStyles[4];
	static CButtonContainer s_aPositions[9];
	static CLineInput s_OpponentInput;

	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);

	CUIRect Row;
	PageView.HSplitTop(26.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "BKW Local Duel", 18.0f, TEXTALIGN_ML);
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Локальный PvP-счёт: если один из двух игроков входит во freeze, очко получает соперник.", 10.4f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Enable, "Включить Local Duel", g_Config.m_BkwLocalDuel, &Row))
	{
		g_Config.m_BkwLocalDuel ^= 1;
		if(!g_Config.m_BkwLocalDuel)
			::BkwLocalDuel::Stop();
	}
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_PublicScore, "Отправлять изменение счёта в общий чат", g_Config.m_BkwLocalDuelChatScore, &Row))
		g_Config.m_BkwLocalDuelChatScore ^= 1;
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Hud, "Показывать Duel HUD", g_Config.m_BkwLocalDuelHud, &Row))
		g_Config.m_BkwLocalDuelHud ^= 1;

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	PageView.HSplitTop(18.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Противник", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(30.0f, &Row, &PageView);
	CUIRect OpponentEdit, SelectButton;
	Row.VSplitRight(150.0f, &OpponentEdit, &SelectButton);
	OpponentEdit.VMargin(2.0f, &OpponentEdit);
	SelectButton.VMargin(2.0f, &SelectButton);
	s_OpponentInput.SetBuffer(g_Config.m_BkwLocalDuelOpponent, sizeof(g_Config.m_BkwLocalDuelOpponent));
	s_OpponentInput.SetEmptyText("Ник игрока");
	pUi->DoEditBox(&s_OpponentInput, &OpponentEdit, 11.0f);
	if(pMenus->DoButton_Menu(&s_SelectPlayer, "Выбрать ЛКМ", 0, &SelectButton))
	{
		g_Config.m_BkwLocalDuel = 1;
		::BkwLocalDuel::BeginSelectPlayer();
		pGameClient->m_Menus.SetActive(false);
		pUi->SetActiveItem(nullptr);
	}

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(18.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Клавиша выбора игрока", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(30.0f, &Row, &PageView);
	const CBindSlot CurrentBind(g_Config.m_BkwLocalDuelSelectKey, g_Config.m_BkwLocalDuelSelectModifier);
	const CKeyBinder::CKeyReaderResult KeyResult = pGameClient->m_KeyBinder.DoKeyReader(&s_KeyReader, &s_KeyClear, &Row, CurrentBind, false);
	if(!KeyResult.m_Aborted && KeyResult.m_Bind != CurrentBind)
	{
		g_Config.m_BkwLocalDuelSelectKey = KeyResult.m_Bind.m_Key;
		g_Config.m_BkwLocalDuelSelectModifier = KeyResult.m_Bind.m_ModifierMask;
	}

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	PageView.HSplitTop(24.0f, &Row, &PageView);
	if(pMenus->DoButton_CheckBox(&s_Infinite, "Фан-дуэль до бесконечности (∞)", g_Config.m_BkwLocalDuelLimit == 0, &Row))
		g_Config.m_BkwLocalDuelLimit = g_Config.m_BkwLocalDuelLimit == 0 ? 5 : 0;
	if(g_Config.m_BkwLocalDuelLimit > 0)
	{
		PageView.HSplitTop(4.0f, nullptr, &PageView);
		PageView.HSplitTop(28.0f, &Row, &PageView);
		pUi->DoScrollbarOption(&g_Config.m_BkwLocalDuelLimit, &g_Config.m_BkwLocalDuelLimit, &Row, "Играть до", 1, 50);
	}
	else
	{
		PageView.HSplitTop(32.0f, nullptr, &PageView);
	}

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(30.0f, &Row, &PageView);
	CUIRect StartButton, StopButton;
	Row.VSplitMid(&StartButton, &StopButton, 6.0f);
	if(pMenus->DoButton_Menu(&s_StartReset, "Старт / сброс счёта", 0, &StartButton))
	{
		g_Config.m_BkwLocalDuel = 1;
		::BkwLocalDuel::ResetScores(pGameClient, true);
	}
	if(pMenus->DoButton_Menu(&s_Stop, "Остановить", 0, &StopButton))
		::BkwLocalDuel::Stop();

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	PageView.HSplitTop(18.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Стиль HUD", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	const char *apStyles[4] = {"Glass", "Minimal", "Neon", "Classic"};
	for(int i = 0; i < 4; ++i)
	{
		CUIRect Button;
		Row.VSplitLeft(Row.w / (float)(4 - i), &Button, &Row);
		Button.VMargin(2.0f, &Button);
		if(pMenus->DoButton_Menu(&s_aStyles[i], apStyles[i], g_Config.m_BkwLocalDuelHudStyle == i, &Button))
			g_Config.m_BkwLocalDuelHudStyle = i;
	}

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(18.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "Позиция HUD", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	const char *apPositions[9] = {"ЛВ", "В", "ПВ", "Л", "Ц", "П", "ЛН", "Н", "ПН"};
	for(int r = 0; r < 3; ++r)
	{
		PageView.HSplitTop(25.0f, &Row, &PageView);
		for(int c = 0; c < 3; ++c)
		{
			const int Index = r * 3 + c;
			CUIRect Button;
			Row.VSplitLeft(Row.w / (float)(3 - c), &Button, &Row);
			Button.VMargin(2.0f, &Button);
			if(pMenus->DoButton_Menu(&s_aPositions[Index], apPositions[Index], g_Config.m_BkwLocalDuelHudPosition == Index, &Button))
				g_Config.m_BkwLocalDuelHudPosition = Index;
		}
		if(r != 2)
			PageView.HSplitTop(2.0f, nullptr, &PageView);
	}

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwLocalDuelHudScale, &g_Config.m_BkwLocalDuelHudScale, &Row, "Размер HUD", 60, 140, &CUi::ms_LinearScrollbarScale, 0, "%");
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwLocalDuelHudOpacity, &g_Config.m_BkwLocalDuelHudOpacity, &Row, "Прозрачность HUD", 20, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(28.0f, &Row, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwLocalDuelHudRounding, &g_Config.m_BkwLocalDuelHudRounding, &Row, "Скругление HUD", 0, 24);

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	PageView.HSplitTop(40.0f, &Row, &PageView);
	char aStatus[256];
	::BkwLocalDuel::BuildStatus(pGameClient, aStatus, sizeof(aStatus));
	pUi->DoLabel(&Row, aStatus, 10.4f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
	PageView.HSplitTop(3.0f, nullptr, &PageView);
	PageView.HSplitTop(42.0f, &Row, &PageView);
	pUi->DoLabel(&Row, "При изменении счёта общий чат получает: [Ник1: 1] vs [Ник2: 0]. При достижении лимита после него отправляется «Ник1 WIN Ник2».", 10.0f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
}
