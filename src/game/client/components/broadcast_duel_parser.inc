
	int Index = -1;
	if(m_TeeFusionDuelCandidateCount < 2)
		Index = m_TeeFusionDuelCandidateCount++;
	else
		Index = m_aTeeFusionDuelCandidates[0].m_SeenAt <= m_aTeeFusionDuelCandidates[1].m_SeenAt ? 0 : 1;

	m_aTeeFusionDuelCandidates[Index] = STeeFusionDuelCandidate();
	str_copy(m_aTeeFusionDuelCandidates[Index].m_aName, pName);
	m_aTeeFusionDuelCandidates[Index].m_Score = Score;
	m_aTeeFusionDuelCandidates[Index].m_SeenAt = SeenAt;
}

void CBroadcast::UpdateTeeFusionDuelPlayer(const char *pName, int Score)
{
	const float Now = Client()->GlobalTime();
	int PlayerIndex = -1;
	for(int i = 0; i < m_TeeFusionDuelPlayerCount; ++i)
	{
		if(str_comp(m_aTeeFusionDuelPlayers[i].m_aName, pName) == 0)
		{
			PlayerIndex = i;
			break;
		}
	}

	if(PlayerIndex < 0 && m_TeeFusionDuelPlayerCount < 2)
	{
		PlayerIndex = m_TeeFusionDuelPlayerCount++;
		str_copy(m_aTeeFusionDuelPlayers[PlayerIndex].m_aName, pName);
		m_aTeeFusionDuelPlayers[PlayerIndex].m_Score = Score;
		m_aTeeFusionDuelPlayers[PlayerIndex].m_ScoreChangedAt = Now;
	}
	else if(PlayerIndex < 0)
	{
		int Replace = -1;
		for(int i = 0; i < 2; ++i)
		{
			if(m_aTeeFusionDuelPlayers[i].m_LastSeenCycle != m_TeeFusionDuelCycle &&
				(Replace < 0 || m_aTeeFusionDuelPlayers[i].m_LastUpdate < m_aTeeFusionDuelPlayers[Replace].m_LastUpdate))
				Replace = i;
		}
		if(Replace < 0)
			Replace = m_aTeeFusionDuelPlayers[0].m_LastUpdate <= m_aTeeFusionDuelPlayers[1].m_LastUpdate ? 0 : 1;
		PlayerIndex = Replace;
		m_aTeeFusionDuelPlayers[PlayerIndex] = STeeFusionDuelPlayer();
		str_copy(m_aTeeFusionDuelPlayers[PlayerIndex].m_aName, pName);
		m_aTeeFusionDuelPlayers[PlayerIndex].m_Score = Score;
		m_aTeeFusionDuelPlayers[PlayerIndex].m_ScoreChangedAt = Now;
	}
	else if(m_aTeeFusionDuelPlayers[PlayerIndex].m_Score != Score)
	{
		m_aTeeFusionDuelPlayers[PlayerIndex].m_Score = Score;
		m_aTeeFusionDuelPlayers[PlayerIndex].m_ScoreChangedAt = Now;
	}

	auto &Player = m_aTeeFusionDuelPlayers[PlayerIndex];
	Player.m_LastUpdate = Now;
	Player.m_LastSeenCycle = m_TeeFusionDuelCycle;
	m_TeeFusionDuelLastUpdate = Now;
	m_TeeFusionDuelActive = true;
}

void CBroadcast::ConfirmTeeFusionDuelCandidates(float Now)
{
	const bool WasActive = m_TeeFusionDuelActive;
	for(int i = 0; i < m_TeeFusionDuelCandidateCount; ++i)
	{
		const auto &Candidate = m_aTeeFusionDuelCandidates[i];
		if(Candidate.m_aName[0] == '\0' || Now - Candidate.m_SeenAt > BKW_TF_DUEL_CANDIDATE_WINDOW_SECONDS)
			continue;
		UpdateTeeFusionDuelPlayer(Candidate.m_aName, Candidate.m_Score);
	}

	if(m_TeeFusionDuelActive)
		m_TeeFusionDuelLastUpdate = Now;

	if(g_Config.m_Debug && !WasActive && m_TeeFusionDuelPlayerCount >= 2)
	{
		log_info("bkw-duel", "confirmed TeeFusion duel: %s %d VS %d %s",
			m_aTeeFusionDuelPlayers[0].m_aName, m_aTeeFusionDuelPlayers[0].m_Score,
			m_aTeeFusionDuelPlayers[1].m_Score, m_aTeeFusionDuelPlayers[1].m_aName);
	}
}

bool CBroadcast::HandleTeeFusionDuelBroadcast(const char *pText)
{
	if(!IsTeeFusionServer() || !pText || pText[0] == '\0')
		return false;

	const float Now = Client()->GlobalTime();
	if(m_TeeFusionDuelActive && Now - m_TeeFusionDuelLastUpdate > BKW_TF_DUEL_TIMEOUT_SECONDS)
		ResetTeeFusionDuel();

	bool AnyHandled = false;
	bool AnyUnknown = false;
	const char *pCursor = pText;
	char aLine[1024];
	while((pCursor = str_next_token(pCursor, "\n", aLine, sizeof(aLine))))
	{
		const std::string Line = BkwTrimDuelLine(aLine);
		if(Line.empty())
			continue;

		if(BkwIsTeeFusionDuelTitle(Line))
		{
			m_TeeFusionDuelLastTitleAt = Now;
			++m_TeeFusionDuelCycle;
			ConfirmTeeFusionDuelCandidates(Now);
			if(m_TeeFusionDuelActive)
				m_TeeFusionDuelLastUpdate = Now;
			AnyHandled = true;
			continue;
		}

		std::string Name;
		int Score = 0;
		if(BkwParseTeeFusionDuelScore(Line, Name, Score))
		{
			BufferTeeFusionDuelCandidate(Name.c_str(), Score, Now);

			bool MatchesCurrentPlayer = false;
			for(int i = 0; i < m_TeeFusionDuelPlayerCount; ++i)
			{
				if(str_comp(m_aTeeFusionDuelPlayers[i].m_aName, Name.c_str()) == 0)
				{
					MatchesCurrentPlayer = true;
					break;
				}
			}

			const bool RecentDuelTitle = Now - m_TeeFusionDuelLastTitleAt <= BKW_TF_DUEL_CANDIDATE_WINDOW_SECONDS;
			// Support both real TeeFusion orders:
			//   score -> score -> title
			// and
			//   title -> score -> score.
			// Once two players are known, an unrelated third name still cannot replace
			// them until a later explicit title confirms the buffered candidates.
			const bool ConfirmedContext =
				(m_TeeFusionDuelActive && MatchesCurrentPlayer) ||
				(RecentDuelTitle && m_TeeFusionDuelPlayerCount < 2);
			if(ConfirmedContext)
			{
				UpdateTeeFusionDuelPlayer(Name.c_str(), Score);
				AnyHandled = true;
			}
			else
			{
				AnyUnknown = true;
			}
		}
		else
		{
			AnyUnknown = true;
		}
	}

	return AnyHandled && !AnyUnknown;
}
