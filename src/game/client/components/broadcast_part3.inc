	const float Now = Client()->GlobalTime();
	const float LeftPulse = std::clamp(1.0f - (Now - LeftPlayer.m_ScoreChangedAt) / BKW_TF_DUEL_SCORE_PULSE_SECONDS, 0.0f, 1.0f);
	const float RightPulse = HasRight ? std::clamp(1.0f - (Now - RightPlayer.m_ScoreChangedAt) / BKW_TF_DUEL_SCORE_PULSE_SECONDS, 0.0f, 1.0f) : 0.0f;

	const int Style = std::clamp(g_Config.m_BkwTfDuelHudStyle, 0, 3);
	if(Style == 0)
	{
		BkwDuelDrawLiquidGlass(Card, Alpha);

		CUIRect Header, Content;
		Card.HSplitTop(22.0f, &Header, &Content);
		BkwDuelLabel(Ui(), Header, "TEEFUSION  •  DUEL", 8.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.82f, 0.90f, 1.0f, 0.86f), Alpha));

		Content.Margin(8.0f, &Content);
		CUIRect Left, Center, Right;
		Content.VSplitLeft(Content.w * 0.43f, &Left, &Content);
		Content.VSplitRight(Content.w * 0.43f, &Center, &Right);

		CUIRect LeftName, LeftScore, RightName, RightScore;
		Left.HSplitTop(18.0f, &LeftName, &LeftScore);
		Right.HSplitTop(18.0f, &RightName, &RightScore);
		BkwDuelLabel(Ui(), LeftName, LeftPlayer.m_aName, 10.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.93f, 0.96f, 1.0f, 0.93f), Alpha));
		BkwDuelLabel(Ui(), RightName, HasRight ? RightPlayer.m_aName : "Ожидание…", 10.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.93f, 0.96f, 1.0f, HasRight ? 0.93f : 0.46f), Alpha));
		BkwDuelLabel(Ui(), LeftScore, aLeftScore, 24.0f + LeftPulse * 1.8f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.64f + LeftPulse * 0.20f, 0.83f + LeftPulse * 0.12f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), RightScore, aRightScore, 24.0f + RightPulse * 1.8f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.88f + RightPulse * 0.10f, 0.77f + RightPulse * 0.12f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), Center, "VS", 9.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.40f), Alpha));
	}
	else if(Style == 1)
	{
		// Minimal: almost no chrome, only a subtle readability shadow and divider.
		CUIRect Shadow = Card;
		Shadow.y += 2.0f;
		Shadow.Draw(BkwDuelAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.14f), Alpha), IGraphics::CORNER_ALL, 12.0f);
		Card.Draw(BkwDuelAlpha(ColorRGBA(0.02f, 0.025f, 0.035f, 0.24f), Alpha), IGraphics::CORNER_ALL, 12.0f);
		CUIRect Header, Row;
		Card.HSplitTop(20.0f, &Header, &Row);
		BkwDuelLabel(Ui(), Header, "DUEL", 8.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.50f), Alpha));
		Row.Margin(10.0f, &Row);
		CUIRect Left, Mid, Right;
		Row.VSplitLeft(Row.w * 0.44f, &Left, &Row);
		Row.VSplitRight(Row.w * 0.44f, &Mid, &Right);
		char aLeft[96], aRight[96];
		str_format(aLeft, sizeof(aLeft), "%s   %s", LeftPlayer.m_aName, aLeftScore);
		str_format(aRight, sizeof(aRight), "%s   %s", aRightScore, HasRight ? RightPlayer.m_aName : "…");
		BkwDuelLabel(Ui(), Left, aLeft, 13.0f, TEXTALIGN_MR, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.96f), Alpha));
		BkwDuelLabel(Ui(), Right, aRight, 13.0f, TEXTALIGN_ML, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.96f), Alpha));
		CUIRect Divider = Mid;
		Divider.x += Divider.w * 0.5f - 0.5f;
		Divider.w = 1.0f;
		Divider.y += 10.0f;
		Divider.h -= 20.0f;
		Divider.Draw(BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.28f), Alpha), IGraphics::CORNER_ALL, 1.0f);
	}
	else if(Style == 2)
	{
		// Neon arena card.
		Card.Draw(BkwDuelAlpha(ColorRGBA(0.018f, 0.022f, 0.038f, 0.90f), Alpha), IGraphics::CORNER_ALL, 11.0f);
		CUIRect TopAccent = Card;
		TopAccent.h = 2.0f;
		TopAccent.Draw4(
			BkwDuelAlpha(ColorRGBA(0.18f, 0.78f, 1.0f, 0.95f), Alpha),
			BkwDuelAlpha(ColorRGBA(0.82f, 0.25f, 1.0f, 0.95f), Alpha),
			BkwDuelAlpha(ColorRGBA(0.18f, 0.78f, 1.0f, 0.95f), Alpha),
			BkwDuelAlpha(ColorRGBA(0.82f, 0.25f, 1.0f, 0.95f), Alpha),
			IGraphics::CORNER_T, 11.0f);

		CUIRect Header, Row;
		Card.HSplitTop(21.0f, &Header, &Row);
		BkwDuelLabel(Ui(), Header, "TEEFUSION DUEL", 8.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.75f, 0.80f, 0.92f, 0.78f), Alpha));
		Row.Margin(8.0f, &Row);
		CUIRect Left, Center, Right;
		Row.VSplitLeft(Row.w * 0.44f, &Left, &Row);
		Row.VSplitRight(Row.w * 0.44f, &Center, &Right);
		CUIRect LeftName, LeftScore, RightName, RightScore;
		Left.HSplitTop(18.0f, &LeftName, &LeftScore);
		Right.HSplitTop(18.0f, &RightName, &RightScore);
		BkwDuelLabel(Ui(), LeftName, LeftPlayer.m_aName, 9.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.52f, 0.88f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), RightName, HasRight ? RightPlayer.m_aName : "Ожидание…", 9.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.92f, 0.58f, 1.0f, HasRight ? 1.0f : 0.45f), Alpha));
		BkwDuelLabel(Ui(), LeftScore, aLeftScore, 25.0f + LeftPulse * 2.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.38f, 0.82f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), RightScore, aRightScore, 25.0f + RightPulse * 2.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.86f, 0.38f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), Center, "VS", 9.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.36f), Alpha));
	}
	else
	{
		// Classic: compact esports-style scoreboard.
		Card.h = 64.0f;
		Card.Draw(BkwDuelAlpha(ColorRGBA(0.035f, 0.035f, 0.045f, 0.88f), Alpha), IGraphics::CORNER_ALL, 5.0f);
		CUIRect Header, Row;
		Card.HSplitTop(18.0f, &Header, &Row);
		Header.Draw(BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.065f), Alpha), IGraphics::CORNER_T, 5.0f);
		BkwDuelLabel(Ui(), Header, "TEEFUSION • ДУЭЛЬ", 8.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.90f, 0.92f, 0.96f, 0.84f), Alpha));
		Row.Margin(7.0f, &Row);
		CUIRect Left, Score, Right;
		Row.VSplitLeft(Row.w * 0.37f, &Left, &Row);
		Row.VSplitRight(Row.w * 0.37f, &Score, &Right);
		BkwDuelLabel(Ui(), Left, LeftPlayer.m_aName, 11.0f, TEXTALIGN_MR, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.94f), Alpha));
		BkwDuelLabel(Ui(), Right, HasRight ? RightPlayer.m_aName : "…", 11.0f, TEXTALIGN_ML, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, HasRight ? 0.94f : 0.40f), Alpha));
		char aScore[64];
		str_format(aScore, sizeof(aScore), "%s   :   %s", aLeftScore, aRightScore);
		BkwDuelLabel(Ui(), Score, aScore, 18.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.72f, 0.84f, 1.0f, 1.0f), Alpha));
	}
}

void CBroadcast::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_BROADCAST)
	{
		const CNetMsg_Sv_Broadcast *pMsg = (CNetMsg_Sv_Broadcast *)pRawMsg;
		DoBroadcast(pMsg->m_pMessage);
	}
}

void CBroadcast::DoBroadcast(const char *pText)
{
	const char *pOriginalText = pText;
	const bool DuelBroadcast = g_Config.m_BkwTfDuelHud && HandleTeeFusionDuelBroadcast(pText);
	if(DuelBroadcast && g_Config.m_BkwTfDuelHideBroadcast)
	{
		m_aBroadcastText[0] = '\0';
		m_BroadcastTick = 0;
		m_BroadcastRenderOffset = -1.0f;
		TextRender()->DeleteTextContainer(m_TextContainerIndex);
	}
	else
	{
		str_copy(m_aBroadcastText, pText);
		m_BroadcastTick = Client()->GameTick(g_Config.m_ClDummy) + Client()->GameTickSpeed() * 10;
		m_BroadcastRenderOffset = -1.0f;
		TextRender()->DeleteTextContainer(m_TextContainerIndex);
	}

	if(g_Config.m_ClPrintBroadcasts)
	{
		const LOG_COLOR LogColor = color_cast<LOG_COLOR>(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageHighlightColor)));
		char aLine[sizeof(m_aBroadcastText)];
		while((pOriginalText = str_next_token(pOriginalText, "\n", aLine, sizeof(aLine))))
		{
			if(aLine[0] != '\0')
			{
				log_info_color(LogColor, "broadcast", "%s", aLine);
			}
		}
	}
}