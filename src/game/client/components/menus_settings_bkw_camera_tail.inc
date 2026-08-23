	PageView.HSplitTop(g_Config.m_BcCameraDrift ? 258.0f : 108.0f, &Card, &PageView);
	Card.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.48f), IGraphics::CORNER_ALL, 8.0f);
	Card.Margin(10.0f, &Card);

	CUIRect TitleRow, Title, Reset;
	Card.HSplitTop(28.0f, &TitleRow, &Card);
	TitleRow.VSplitRight(92.0f, &Title, &Reset);
	pUi->DoLabel(&Title, "Дрифт камеры", 18.0f, TEXTALIGN_ML);
	if(pMenus->DoButton_Menu(&s_CameraDriftResetButton, "Сбросить", 0, &Reset))
	{
		g_Config.m_BcCameraDriftAmount = 50;
		g_Config.m_BcCameraDriftSmoothness = 20;
		g_Config.m_BcCameraDriftReverse = 0;
	}

	Card.HSplitTop(6.0f, nullptr, &Card);
	CUIRect Toggle;
	Card.HSplitTop(26.0f, &Toggle, &Card);
	if(pMenus->DoButton_CheckBox(&s_CameraDriftToggleId, "Дрифт камеры", g_Config.m_BcCameraDrift, &Toggle))
		g_Config.m_BcCameraDrift ^= 1;

	if(g_Config.m_BcCameraDrift)
	{
		Card.HSplitTop(10.0f, nullptr, &Card);
		CUIRect Amount;
		Card.HSplitTop(28.0f, &Amount, &Card);
		pUi->DoScrollbarOption(&g_Config.m_BcCameraDriftAmount, &g_Config.m_BcCameraDriftAmount, &Amount, "Сила дрифта камеры", 1, 200);

		Card.HSplitTop(8.0f, nullptr, &Card);
		CUIRect Smoothness;
		Card.HSplitTop(28.0f, &Smoothness, &Card);
		pUi->DoScrollbarOption(&g_Config.m_BcCameraDriftSmoothness, &g_Config.m_BcCameraDriftSmoothness, &Smoothness, "Плавность дрифта камеры", 1, 20);

		Card.HSplitTop(10.0f, nullptr, &Card);
		CUIRect DirectionLabel;
		Card.HSplitTop(20.0f, &DirectionLabel, &Card);
		pUi->DoLabel(&DirectionLabel, "Направление дрифта", 13.0f, TEXTALIGN_ML);
		Card.HSplitTop(4.0f, nullptr, &Card);
		CUIRect DirectionButtons, Forward, Backward;
		Card.HSplitTop(30.0f, &DirectionButtons, &Card);
		DirectionButtons.VSplitMid(&Forward, &Backward, 6.0f);
		if(pMenus->DoButton_Menu(&s_CameraDriftForwardButton, "Вперёд", !g_Config.m_BcCameraDriftReverse, &Forward))
			g_Config.m_BcCameraDriftReverse = 0;
		if(pMenus->DoButton_Menu(&s_CameraDriftBackwardButton, "Назад", g_Config.m_BcCameraDriftReverse, &Backward))
			g_Config.m_BcCameraDriftReverse = 1;
	}

	if(Blocked)
	{
		PageView.HSplitTop(10.0f, nullptr, &PageView);
		CUIRect Warning;
		PageView.HSplitTop(42.0f, &Warning, &PageView);
		Warning.Draw(ColorRGBA(0.35f, 0.08f, 0.08f, 0.45f), IGraphics::CORNER_ALL, 6.0f);
		Warning.Margin(8.0f, &Warning);
		pUi->DoLabel(&Warning, "На этом сервере дрифт камеры отключён (FNG / 0xf).", 12.0f, TEXTALIGN_ML);
	}
}
