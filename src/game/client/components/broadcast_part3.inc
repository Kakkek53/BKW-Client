	ColorRGBA NameLeft(0.94f, 0.96f, 1.0f, 0.96f);
	ColorRGBA NameRight(0.94f, 0.96f, 1.0f, 0.96f);
	ColorRGBA ScoreLeft(0.67f, 0.86f, 1.0f, 1.0f);
	ColorRGBA ScoreRight(0.88f, 0.76f, 1.0f, 1.0f);
	ColorRGBA VsColor(1.0f, 1.0f, 1.0f, 0.45f);

	if(Style == 0)
	{
		// Liquid Glass: layered translucent surface without the old oversized header.
		BkwDuelDrawLiquidGlass(Card, Alpha, Rounding);
	}
	else if(Style == 1)
	{
		// Minimal: almost no chrome, only enough contrast to keep names readable.
		CUIRect Shadow = Card;
		Shadow.y += 1.5f * Scale;
		Shadow.Draw(BkwDuelAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.12f), Alpha), IGraphics::CORNER_ALL, Rounding);
		Card.Draw(BkwDuelAlpha(ColorRGBA(0.02f, 0.025f, 0.035f, 0.25f), Alpha), IGraphics::CORNER_ALL, Rounding);
		ScoreLeft = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
		ScoreRight = ScoreLeft;
		VsColor = ColorRGBA(1.0f, 1.0f, 1.0f, 0.34f);
	}
	else if(Style == 2)
	{
		// Neon: cyan vs purple, but the geometry is identical to every other style.
		Card.Draw(BkwDuelAlpha(ColorRGBA(0.018f, 0.022f, 0.038f, 0.91f), Alpha), IGraphics::CORNER_ALL, Rounding);
		CUIRect Accent = Card;
		Accent.h = std::max(1.4f, 1.8f * Scale);
		Accent.Draw4(
			BkwDuelAlpha(ColorRGBA(0.18f, 0.78f, 1.0f, 0.95f), Alpha),
			BkwDuelAlpha(ColorRGBA(0.82f, 0.25f, 1.0f, 0.95f), Alpha),
			BkwDuelAlpha(ColorRGBA(0.18f, 0.78f, 1.0f, 0.95f), Alpha),
			BkwDuelAlpha(ColorRGBA(0.82f, 0.25f, 1.0f, 0.95f), Alpha),
			IGraphics::CORNER_T, Rounding);
		NameLeft = ColorRGBA(0.52f, 0.88f, 1.0f, 1.0f);
		NameRight = ColorRGBA(0.92f, 0.58f, 1.0f, 1.0f);
		ScoreLeft = ColorRGBA(0.38f, 0.82f, 1.0f, 1.0f);
		ScoreRight = ColorRGBA(0.86f, 0.38f, 1.0f, 1.0f);
	}
	else
	{
		// Classic: opaque esports-style strip.
		Card.Draw(BkwDuelAlpha(ColorRGBA(0.035f, 0.035f, 0.045f, 0.90f), Alpha), IGraphics::CORNER_ALL, Rounding);
		ScoreLeft = ColorRGBA(0.74f, 0.86f, 1.0f, 1.0f);
		ScoreRight = ScoreLeft;
		VsColor = ColorRGBA(1.0f, 1.0f, 1.0f, 0.54f);
	}

	// One shared, mathematically symmetric layout for all styles:
	// 30% name | 12% score | 16% VS | 12% score | 30% name.
	CUIRect Row = Card;
	Row.Margin(6.0f * Scale, &Row);
	const float NameW = Row.w * 0.30f;
	const float ScoreW = Row.w * 0.12f;
	const float VsW = Row.w * 0.16f;
	CUIRect LeftName{Row.x, Row.y, NameW, Row.h};
	CUIRect LeftScore{LeftName.x + LeftName.w, Row.y, ScoreW, Row.h};
	CUIRect Vs{LeftScore.x + LeftScore.w, Row.y, VsW, Row.h};
	CUIRect RightScore{Vs.x + Vs.w, Row.y, ScoreW, Row.h};
	CUIRect RightName{RightScore.x + RightScore.w, Row.y, NameW, Row.h};

	const float NameSize = 9.5f * Scale;
	const float ScoreSize = 18.0f * Scale;
	const float VsSize = 8.5f * Scale;
	BkwDuelLabel(Ui(), LeftName, LeftPlayer.m_aName, NameSize, TEXTALIGN_MR, BkwDuelAlpha(NameLeft, Alpha));
	BkwDuelLabel(Ui(), LeftScore, aLeftScore, ScoreSize + LeftPulse * 1.5f * Scale, TEXTALIGN_MC, BkwDuelAlpha(ScoreLeft, Alpha));
	BkwDuelLabel(Ui(), Vs, "VS", VsSize, TEXTALIGN_MC, BkwDuelAlpha(VsColor, Alpha));
	BkwDuelLabel(Ui(), RightScore, aRightScore, ScoreSize + RightPulse * 1.5f * Scale, TEXTALIGN_MC, BkwDuelAlpha(ScoreRight, Alpha));
	BkwDuelLabel(Ui(), RightName, RightPlayer.m_aName, NameSize, TEXTALIGN_ML, BkwDuelAlpha(NameRight, Alpha));
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
				log_info_color(LogColor, "broadcast", "%s", aLine);
		}
	}
}
