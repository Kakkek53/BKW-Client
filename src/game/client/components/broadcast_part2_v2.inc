void CBroadcast::OnReset()
{
	m_BroadcastTick = 0;
	m_BroadcastRenderOffset = -1.0f;
	TextRender()->DeleteTextContainer(m_TextContainerIndex);
	ResetTeeFusionDuel();
}

void CBroadcast::OnWindowResize()
{
	m_BroadcastRenderOffset = -1.0f;
	TextRender()->DeleteTextContainer(m_TextContainerIndex);
}

void CBroadcast::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		ResetTeeFusionDuel();
		return;
	}

	RenderServerBroadcast();
	RenderTeeFusionDuelHud();
}

void CBroadcast::RenderServerBroadcast()
{
	if(GameClient()->m_Scoreboard.IsActive() ||
		GameClient()->m_Motd.IsActive() ||
		GameClient()->m_ImportantAlert.IsActive() ||
		!g_Config.m_ClShowBroadcasts)
	{
		return;
	}

	const float SecondsRemaining = (m_BroadcastTick - Client()->GameTick(g_Config.m_ClDummy)) / (float)Client()->GameTickSpeed();
	if(SecondsRemaining <= 0.0f)
	{
		TextRender()->DeleteTextContainer(m_TextContainerIndex);
		return;
	}

	const float Height = 300.0f;
	const float Width = Height * Graphics()->ScreenAspect();
	Graphics()->MapScreenToSize(Width, Height);

	if(m_BroadcastRenderOffset < 0.0f)
		m_BroadcastRenderOffset = Width / 2.0f - TextRender()->TextWidth(12.0f, m_aBroadcastText, -1, Width) / 2.0f;

	if(!m_TextContainerIndex.Valid())
	{
		CTextCursor Cursor;
		Cursor.SetPosition(vec2(m_BroadcastRenderOffset, 40.0f));
		Cursor.m_FontSize = 12.0f;
		Cursor.m_LineWidth = Width;
		TextRender()->CreateTextContainer(m_TextContainerIndex, &Cursor, m_aBroadcastText);
	}
	if(m_TextContainerIndex.Valid())
	{
		const float Alpha = SecondsRemaining >= 1.0f ? 1.0f : SecondsRemaining;
		ColorRGBA TextColor = TextRender()->DefaultTextColor();
		TextColor.a *= Alpha;
		ColorRGBA OutlineColor = TextRender()->DefaultTextOutlineColor();
		OutlineColor.a *= Alpha;
		TextRender()->RenderTextContainer(m_TextContainerIndex, TextColor, OutlineColor);
	}
}

void CBroadcast::RenderTeeFusionDuelHud()
{
	if(!g_Config.m_BkwTfDuelHud || !IsTeeFusionServer())
	{
		if(m_TeeFusionDuelActive)
			ResetTeeFusionDuel();
		return;
	}

	// Do not show a half-filled scoreboard. The parser keeps candidates until both
	// duel participants have been confirmed by the TeeFusion title cycle.
	if(!m_TeeFusionDuelActive || m_TeeFusionDuelPlayerCount < 2)
		return;

	const float Age = Client()->GlobalTime() - m_TeeFusionDuelLastUpdate;
	if(Age >= BKW_TF_DUEL_TIMEOUT_SECONDS)
	{
		ResetTeeFusionDuel();
		return;
	}

	if(GameClient()->m_Scoreboard.IsActive() || GameClient()->m_Motd.IsActive() || GameClient()->m_ImportantAlert.IsActive())
		return;

	float Alpha = 1.0f;
	const float FadeStart = BKW_TF_DUEL_TIMEOUT_SECONDS - BKW_TF_DUEL_FADE_SECONDS;
	if(Age > FadeStart)
		Alpha = std::clamp((BKW_TF_DUEL_TIMEOUT_SECONDS - Age) / BKW_TF_DUEL_FADE_SECONDS, 0.0f, 1.0f);
	Alpha *= std::clamp(g_Config.m_BkwTfDuelHudOpacity / 100.0f, 0.20f, 1.0f);

	Ui()->MapScreen();
	const CUIRect Screen = *Ui()->Screen();
	const float Scale = std::clamp(g_Config.m_BkwTfDuelHudScale / 100.0f, 0.60f, 1.40f);
	const float CardWidth = std::min(318.0f * Scale, std::max(160.0f, Screen.w - 28.0f));
	const float CardHeight = 46.0f * Scale;
	CUIRect Card = BkwDuelCardRect(Screen, CardWidth, CardHeight, g_Config.m_BkwTfDuelHudPosition);
	const float Rounding = std::clamp((float)g_Config.m_BkwTfDuelHudRounding * Scale, 0.0f, CardHeight * 0.5f);

	const auto &LeftPlayer = m_aTeeFusionDuelPlayers[0];
	const auto &RightPlayer = m_aTeeFusionDuelPlayers[1];
	char aLeftScore[24];
	char aRightScore[24];
	str_format(aLeftScore, sizeof(aLeftScore), "%d", LeftPlayer.m_Score);
	str_format(aRightScore, sizeof(aRightScore), "%d", RightPlayer.m_Score);

	const float Now = Client()->GlobalTime();
	const float LeftPulse = std::clamp(1.0f - (Now - LeftPlayer.m_ScoreChangedAt) / BKW_TF_DUEL_SCORE_PULSE_SECONDS, 0.0f, 1.0f);
	const float RightPulse = std::clamp(1.0f - (Now - RightPlayer.m_ScoreChangedAt) / BKW_TF_DUEL_SCORE_PULSE_SECONDS, 0.0f, 1.0f);
	const int Style = std::clamp(g_Config.m_BkwTfDuelHudStyle, 0, 3);
