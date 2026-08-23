	CheckpointCard.HSplitTop(22.0f, &Status, &CheckpointCard);

	bool PracticeActive = false;
	const int LocalClientId = pGameClient->m_Snap.m_LocalClientId;
	if(pClient->State() == IClient::STATE_ONLINE && LocalClientId >= 0 && LocalClientId < MAX_CLIENTS)
	{
		const auto &Character = pGameClient->m_Snap.m_aCharacters[LocalClientId];
		PracticeActive = Character.m_Active && Character.m_HasExtendedData && (Character.m_ExtendedData.m_Flags & CHARACTERFLAG_PRACTICE_MODE) != 0;
	}
	char aCheckpointStatus[160];
	str_format(aCheckpointStatus, sizeof(aCheckpointStatus), "Practice: %s   •   Чекпоинтов: %d", PracticeActive ? "активен" : "не активен", pGameClient->m_FastActions.BkwCheckpointCount());
	pUi->DoLabel(&Status, aCheckpointStatus, 11.0f, TEXTALIGN_ML);
}

void RenderTfMenuSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, CUIRect PageView)
{
	static int s_TfMenuToggleId;
	static CButtonContainer s_aTfMenuStyleButtons[4];
	static CButtonContainer s_TfMenuResetButton;

	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);

	CUIRect TitleRow, Title, Reset;
	PageView.HSplitTop(26.0f, &TitleRow, &PageView);
	TitleRow.VSplitRight(92.0f, &Title, &Reset);
	pUi->DoLabel(&Title, "TeeFusion Menu", 18.0f, TEXTALIGN_ML);
	if(pMenus->DoButton_Menu(&s_TfMenuResetButton, "Сбросить", 0, &Reset))
	{
		g_Config.m_BkwTfMenuStyle = 0;
		g_Config.m_BkwTfMenuScale = 100;
		g_Config.m_BkwTfMenuOpacity = 96;
		g_Config.m_BkwTfMenuRounding = 16;
	}

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect Toggle;
	PageView.HSplitTop(24.0f, &Toggle, &PageView);
	if(pMenus->DoButton_CheckBox(&s_TfMenuToggleId, "Новый дизайн меню TF", g_Config.m_BkwTfMenu, &Toggle))
		g_Config.m_BkwTfMenu ^= 1;

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	CUIRect StyleLabel;
	PageView.HSplitTop(18.0f, &StyleLabel, &PageView);
	pUi->DoLabel(&StyleLabel, "Стиль меню", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);

	CUIRect StyleButtons;
	PageView.HSplitTop(30.0f, &StyleButtons, &PageView);
	const char *apStyleNames[4] = {"Standard", "Liquid Glass", "Neon", "Minimal"};
	for(int Style = 0; Style < 4; ++Style)
	{
		CUIRect Button;
		const float ButtonWidth = StyleButtons.w / (float)(4 - Style);
		StyleButtons.VSplitLeft(ButtonWidth, &Button, &StyleButtons);
		Button.VMargin(2.0f, &Button);
		if(pMenus->DoButton_Menu(&s_aTfMenuStyleButtons[Style], apStyleNames[Style], g_Config.m_BkwTfMenuStyle == Style, &Button))
			g_Config.m_BkwTfMenuStyle = Style;
	}

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect ScaleRow;
	PageView.HSplitTop(28.0f, &ScaleRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTfMenuScale, &g_Config.m_BkwTfMenuScale, &ScaleRow, "Размер окна", 75, 125);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect OpacityRow;
	PageView.HSplitTop(28.0f, &OpacityRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTfMenuOpacity, &g_Config.m_BkwTfMenuOpacity, &OpacityRow, "Прозрачность", 45, 100);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect RoundingRow;
	PageView.HSplitTop(28.0f, &RoundingRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTfMenuRounding, &g_Config.m_BkwTfMenuRounding, &RoundingRow, "Скругление", 0, 28);

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	CUIRect Status;
	PageView.HSplitTop(26.0f, &Status, &PageView);
	char aStatus[192];
	if(TfParserTestVisible())
		str_copy(aStatus, s_aTfParserTestMessage);
	else if(!g_Config.m_BkwTfMenu)
		str_copy(aStatus, "Выключено. F6 — разовый тест чтения TeeFusion vote options.");
	else
	{
		char aUserName[64];
		if(BkwTfMenu::ParseUserName(pGameClient->m_Voting, aUserName, sizeof(aUserName)))
			str_format(aStatus, sizeof(aStatus), "TF parser: Имя пользователя — %s", aUserName);
		else
			str_copy(aStatus, "Определение TeeFusion идёт через DDNet master server.");
	}
	pUi->DoLabel(&Status, aStatus, 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Status.w});
}

void RenderTfDuelHudSettings(CMenus *pMenus, CUi *pUi, CUIRect PageView)
{
	static int s_TfDuelHudToggleId;
	static int s_TfDuelHideBroadcastToggleId;
	static CButtonContainer s_aTfDuelStyleButtons[4];
	static CButtonContainer s_aTfDuelPositionButtons[9];
	static CButtonContainer s_TfDuelResetButton;

	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.46f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);

	CUIRect TitleRow, Title, Reset;
	PageView.HSplitTop(26.0f, &TitleRow, &PageView);
	TitleRow.VSplitRight(92.0f, &Title, &Reset);
	pUi->DoLabel(&Title, "TeeFusion Duel HUD", 18.0f, TEXTALIGN_ML);
	if(pMenus->DoButton_Menu(&s_TfDuelResetButton, "Сбросить", 0, &Reset))
	{
		g_Config.m_BkwTfDuelHud = 1;
		g_Config.m_BkwTfDuelHudStyle = 0;
		g_Config.m_BkwTfDuelHideBroadcast = 1;
		g_Config.m_BkwTfDuelHudPosition = 1;
		g_Config.m_BkwTfDuelHudScale = 85;
		g_Config.m_BkwTfDuelHudOpacity = 92;
		g_Config.m_BkwTfDuelHudRounding = 12;
	}

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect Toggle;
	PageView.HSplitTop(24.0f, &Toggle, &PageView);
	if(pMenus->DoButton_CheckBox(&s_TfDuelHudToggleId, "Свой счёт дуэли TeeFusion", g_Config.m_BkwTfDuelHud, &Toggle))
		g_Config.m_BkwTfDuelHud ^= 1;

	PageView.HSplitTop(3.0f, nullptr, &PageView);
	CUIRect HideBroadcast;
	PageView.HSplitTop(24.0f, &HideBroadcast, &PageView);
	if(pMenus->DoButton_CheckBox(&s_TfDuelHideBroadcastToggleId, "Скрывать стандартный broadcast счёта", g_Config.m_BkwTfDuelHideBroadcast, &HideBroadcast))
		g_Config.m_BkwTfDuelHideBroadcast ^= 1;

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	CUIRect StyleLabel;
	PageView.HSplitTop(18.0f, &StyleLabel, &PageView);
	pUi->DoLabel(&StyleLabel, "Стиль табло", 12.0f, TEXTALIGN_ML);

	PageView.HSplitTop(4.0f, nullptr, &PageView);
	CUIRect StyleButtons;
	PageView.HSplitTop(30.0f, &StyleButtons, &PageView);
	const char *apStyleNames[4] = {"Liquid Glass", "Minimal", "Neon", "Classic"};
	for(int Style = 0; Style < 4; ++Style)
	{
		CUIRect Button;
		const float ButtonWidth = StyleButtons.w / (float)(4 - Style);
		StyleButtons.VSplitLeft(ButtonWidth, &Button, &StyleButtons);
		if(Style < 3)
			Button.VMargin(2.0f, &Button);
		if(pMenus->DoButton_Menu(&s_aTfDuelStyleButtons[Style], apStyleNames[Style], g_Config.m_BkwTfDuelHudStyle == Style, &Button))
			g_Config.m_BkwTfDuelHudStyle = Style;
	}

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect PositionLabel;
	PageView.HSplitTop(18.0f, &PositionLabel, &PageView);
	pUi->DoLabel(&PositionLabel, "Положение на экране", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);

	const char *apPositionNames[9] = {"TL", "TC", "TR", "CL", "C", "CR", "BL", "BC", "BR"};
	for(int Row = 0; Row < 3; ++Row)
	{
		CUIRect PositionRow;
		PageView.HSplitTop(26.0f, &PositionRow, &PageView);
		for(int Column = 0; Column < 3; ++Column)
		{
			const int Position = Row * 3 + Column;
			CUIRect PositionButton;
			const float ButtonWidth = PositionRow.w / (float)(3 - Column);
			PositionRow.VSplitLeft(ButtonWidth, &PositionButton, &PositionRow);
			PositionButton.VMargin(2.0f, &PositionButton);
			if(pMenus->DoButton_Menu(&s_aTfDuelPositionButtons[Position], apPositionNames[Position], g_Config.m_BkwTfDuelHudPosition == Position, &PositionButton))
				g_Config.m_BkwTfDuelHudPosition = Position;
		}
		if(Row < 2)
			PageView.HSplitTop(3.0f, nullptr, &PageView);
	}

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect ScaleRow;
	PageView.HSplitTop(28.0f, &ScaleRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTfDuelHudScale, &g_Config.m_BkwTfDuelHudScale, &ScaleRow, "Размер табло", 60, 140);

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect OpacityRow;
	PageView.HSplitTop(28.0f, &OpacityRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTfDuelHudOpacity, &g_Config.m_BkwTfDuelHudOpacity, &OpacityRow, "Прозрачность", 20, 100);

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect RoundingRow;
	PageView.HSplitTop(28.0f, &RoundingRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwTfDuelHudRounding, &g_Config.m_BkwTfDuelHudRounding, &RoundingRow, "Скругление", 0, 24);

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	CUIRect Hint;
	PageView.HSplitTop(32.0f, &Hint, &PageView);
	pUi->DoLabel(&Hint, "Парсер подтверждает счёт только после TeeFusion «Дуэль» и не перехватывает случайные name - number broadcast.", 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Hint.w});
}

void RenderDdnetVoteMenuSettings(CMenus *pMenus, CUi *pUi, CUIRect PageView)
{
	static int s_DdnetVoteMenuToggleId;
	static CButtonContainer s_aDdnetMenuStyleButtons[4];
	static CButtonContainer s_DdnetMenuResetButton;

	PageView.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 8.0f);
	PageView.Margin(10.0f, &PageView);

	CUIRect TitleRow, Title, Reset;
	PageView.HSplitTop(26.0f, &TitleRow, &PageView);
	TitleRow.VSplitRight(92.0f, &Title, &Reset);
	pUi->DoLabel(&Title, "DDNet Race Menu", 18.0f, TEXTALIGN_ML);
	if(pMenus->DoButton_Menu(&s_DdnetMenuResetButton, "Сбросить", 0, &Reset))
	{
		g_Config.m_BkwDdnetMenuStyle = 0;
		g_Config.m_BkwDdnetMenuScale = 100;
		g_Config.m_BkwDdnetMenuOpacity = 96;
		g_Config.m_BkwDdnetMenuRounding = 16;
	}

	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect Toggle;
	PageView.HSplitTop(24.0f, &Toggle, &PageView);
	if(pMenus->DoButton_CheckBox(&s_DdnetVoteMenuToggleId, "Новое меню голосования DDNet", g_Config.m_BkwDdnetVoteMenu, &Toggle))
		g_Config.m_BkwDdnetVoteMenu ^= 1;

	PageView.HSplitTop(7.0f, nullptr, &PageView);
	CUIRect StyleLabel;
	PageView.HSplitTop(18.0f, &StyleLabel, &PageView);
	pUi->DoLabel(&StyleLabel, "Стиль меню", 12.0f, TEXTALIGN_ML);
	PageView.HSplitTop(4.0f, nullptr, &PageView);

	CUIRect StyleButtons;
	PageView.HSplitTop(30.0f, &StyleButtons, &PageView);
	const char *apStyleNames[4] = {"Standard", "Liquid Glass", "Neon", "Minimal"};
	for(int Style = 0; Style < 4; ++Style)
	{
		CUIRect Button;
		const float ButtonWidth = StyleButtons.w / (float)(4 - Style);
		StyleButtons.VSplitLeft(ButtonWidth, &Button, &StyleButtons);
		Button.VMargin(2.0f, &Button);
		if(pMenus->DoButton_Menu(&s_aDdnetMenuStyleButtons[Style], apStyleNames[Style], g_Config.m_BkwDdnetMenuStyle == Style, &Button))
			g_Config.m_BkwDdnetMenuStyle = Style;
	}

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect ScaleRow;
	PageView.HSplitTop(28.0f, &ScaleRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwDdnetMenuScale, &g_Config.m_BkwDdnetMenuScale, &ScaleRow, "Размер окна", 75, 125);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect OpacityRow;
	PageView.HSplitTop(28.0f, &OpacityRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwDdnetMenuOpacity, &g_Config.m_BkwDdnetMenuOpacity, &OpacityRow, "Прозрачность", 45, 100);
	PageView.HSplitTop(5.0f, nullptr, &PageView);
	CUIRect RoundingRow;
	PageView.HSplitTop(28.0f, &RoundingRow, &PageView);
	pUi->DoScrollbarOption(&g_Config.m_BkwDdnetMenuRounding, &g_Config.m_BkwDdnetMenuRounding, &RoundingRow, "Скругление", 0, 28);

	PageView.HSplitTop(6.0f, nullptr, &PageView);
	CUIRect Status;
	PageView.HSplitTop(30.0f, &Status, &PageView);
	pUi->DoLabel(&Status, "В «Подробнее» фон берётся из screenshot карты; игровой кадр полностью закрывается.", 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Status.w});
}

void RenderCameraDriftSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, IClient *pClient, CUIRect PageView)
{
	static int s_CameraDriftToggleId;
	static CButtonContainer s_CameraDriftResetButton;
	static CButtonContainer s_CameraDriftForwardButton;
	static CButtonContainer s_CameraDriftBackwardButton;

	const bool Online = pClient->State() == IClient::STATE_ONLINE;
	const bool IsFngServer = Online && pGameClient->m_GameInfo.m_PredictFNG;
	const bool Is0xFServer = Online && str_comp_nocase(pGameClient->m_GameInfo.m_aGameType, "0xf") == 0;
	const bool Blocked = IsFngServer || Is0xFServer;

	CUIRect Card;
