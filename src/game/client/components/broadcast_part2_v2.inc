			if(!m_TeeFusionDuelActive)
				ResetTeeFusionDuel();
			m_TeeFusionDuelActive = true;
			m_TeeFusionDuelLastUpdate = Now;
			++m_TeeFusionDuelCycle;
			AnyHandled = true;
			continue;
		}

		std::string Name;
		int Score = 0;
		// Score lines are independent broadcasts. If server metadata already
		// identifies TeeFusion, they may be accepted even before the next title.
		// If metadata is missing, the title above establishes the runtime hint and
		// the following score broadcasts are accepted immediately.
		if(KnownTeeFusion && BkwParseTeeFusionDuelScore(Line, Name, Score))
		{
			UpdateTeeFusionDuelPlayer(Name.c_str(), Score);
			AnyHandled = true;
		}
		else
		{
			AnyUnknown = true;
		}
	}

	// Suppress the original broadcast only when the entire message belongs to the
	// duel HUD. Mixed server messages keep rendering normally.
	return AnyHandled && !AnyUnknown;
}

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

	if(!m_TeeFusionDuelActive || m_TeeFusionDuelPlayerCount <= 0)
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

	Ui()->MapScreen();
	const CUIRect Screen = *Ui()->Screen();
	const float CardWidth = minimum(420.0f, Screen.w - 24.0f);
	CUIRect Card = {Screen.x + (Screen.w - CardWidth) * 0.5f, Screen.y + 16.0f, CardWidth, 84.0f};

	const auto &LeftPlayer = m_aTeeFusionDuelPlayers[0];
	const bool HasRight = m_TeeFusionDuelPlayerCount >= 2;
	const auto &RightPlayer = m_aTeeFusionDuelPlayers[HasRight ? 1 : 0];
	char aLeftScore[24];
	char aRightScore[24];
	str_format(aLeftScore, sizeof(aLeftScore), "%d", LeftPlayer.m_Score);
	if(HasRight)
		str_format(aRightScore, sizeof(aRightScore), "%d", RightPlayer.m_Score);
	else
		str_copy(aRightScore, "—");
