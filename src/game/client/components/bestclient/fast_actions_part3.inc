	}
	if(IsActive() && Event.m_Flags & IInput::FLAG_PRESS)
	{
		const int Slot = KeyToSlotIndex(Event.m_Key);
		if(Slot >= 0 && Slot < FAST_ACTIONS_FIXED_SLOTS)
		{
			m_SelectedBind = Slot;
			m_DisplayBind = Slot;
			return true;
		}
	}
	return false;
}

void CFastActions::OnRender()
{
	const bool Online = Client()->State() == IClient::STATE_ONLINE;
	if(Online)
	{
		const CServerInfo &ServerInfo = Client()->ServerInfo();
		const BkwServerDetector::ECommunity Community = BkwServerDetector::Detect(ServerBrowser(), Client()->ServerAddress(), ServerInfo);
		s_BkwTfMenuConnectedToTeeFusion = Community == BkwServerDetector::ECommunity::TEEFUSION;
		s_BkwTfMenuConnectedToDdnet = Community == BkwServerDetector::ECommunity::DDNET;
	}
	else
	{
		s_BkwTfMenuConnectedToTeeFusion = false;
		s_BkwTfMenuConnectedToDdnet = false;
	}
	BkwDdnetVoteState::pHttp = Http();
	const bool CustomVoteMenuEnabled =
		(s_BkwTfMenuConnectedToTeeFusion && g_Config.m_BkwTfMenu) ||
		(s_BkwTfMenuConnectedToDdnet && g_Config.m_BkwDdnetVoteMenu);

	if(GameClient()->m_Menus.IsActive() && GameClient()->m_Menus.GamePage() != CMenus::PAGE_CALLVOTE)
		s_BkwTfMenuOverlaySuppressReopen = false;

	if(CustomVoteMenuEnabled &&
		!s_BkwTfMenuOverlayActive &&
		!s_BkwTfMenuOverlaySuppressReopen &&
		Online &&
		GameClient()->m_Menus.IsActive() &&
		GameClient()->m_Menus.GamePage() == CMenus::PAGE_CALLVOTE)
	{
		s_BkwTfMenuOverlayActive = true;
		s_BkwTfMenuOverlayPulseSent = false;
		s_BkwTfMenuSelectedTab = 0;
		BkwDdnetResetView();
		GameClient()->m_Menus.SetActive(false);
		Ui()->SetActiveItem(nullptr);
	}

	if(s_BkwTfMenuOverlayActive)
	{
		if(!CustomVoteMenuEnabled || !Online)
		{
			BkwTfMenuOverlayReset();
			BkwDdnetResetView();
		}
		else
		{
			const CVoting &Voting = GameClient()->m_Voting;
			if(s_BkwTfMenuConnectedToTeeFusion && Voting.NumOptions() <= 0 && !s_BkwTfMenuOverlayPulseSent)
			{
				PulseTfMenuPlayerFlag(GameClient(), Client(), Console());
				s_BkwTfMenuOverlayPulseSent = true;
			}
			if(s_BkwTfMenuConnectedToDdnet)
				RenderBkwDdnetVoteOverlay(Ui(), GameClient(), Voting, true);
			else
				RenderBkwTfMenuOverlay(Ui(), Voting);
			return;
		}
	}

	if(s_BkwTfVoteDumpPending)
	{
		const CVoting &Voting = GameClient()->m_Voting;
		if(Voting.NumOptions() > 0 && !Voting.IsReceivingOptions())
		{
			DumpTfVoteOptions(Voting, Console(), &GameClient()->m_Chat, Client());
			s_BkwTfVoteDumpPending = false;
		}
		else if(time_get() - s_BkwTfVoteDumpStartedAt >= time_freq() * BKW_TF_VOTE_DUMP_TIMEOUT_SECONDS)
		{
			char aTimeout[224];
			str_format(aTimeout, sizeof(aTimeout), "TF parser: за %d сек. сервер не прислал vote options (сейчас %d).", BKW_TF_VOTE_DUMP_TIMEOUT_SECONDS, Voting.NumOptions());
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", aTimeout);
			GameClient()->m_Chat.AddColoredLine(aTimeout, ColorRGBA(1.0f, 0.65f, 0.35f, 1.0f));
			Client()->Notify("TeeFusion vote parser", aTimeout);
			s_BkwTfVoteDumpPending = false;
		}
	}

	BkwRenderCheckpoints();

	if(!g_Config.m_BcFastActions)
	{
		m_Active = false;
		m_SelectedBind = -1;
		m_DisplayBind = -1;
		m_AnimationTime = 0.0f;
		return;
	}

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	static const auto QuadEaseInOut = [](float t) -> float {
		if(t == 0.0f)
			return 0.0f;
		if(t == 1.0f)
			return 1.0f;
		return (t < 0.5f) ? (2.0f * t * t) : (1.0f - std::pow(-2.0f * t + 2.0f, 2) / 2.0f);
	};

	static const float s_FontSize = 16.0f;

	const float AnimationTime = (float)g_Config.m_TcAnimateWheelTime / 1000.0f;
	const bool SelectedBindValid = m_SelectedBind >= 0 && m_SelectedBind < FAST_ACTIONS_FIXED_SLOTS;
	const bool ShouldBeVisible = m_Active && SelectedBindValid;
	std::array<float, 2> aAnimationPhase;
	if(AnimationTime <= 0.0f)
	{
		if(!ShouldBeVisible)
		{
			m_DisplayBind = -1;
			m_AnimationTime = 0.0f;
			return;
		}

		m_DisplayBind = m_SelectedBind;
		m_AnimationTime = 0.0f;
		aAnimationPhase.fill(1.0f);
	}
	else
	{
		if(ShouldBeVisible && m_DisplayBind != m_SelectedBind)
		{
			m_DisplayBind = m_SelectedBind;
			m_AnimationTime = 0.0f;
		}

		const float Delta = Client()->RenderFrameTime();
		if(ShouldBeVisible)
			m_AnimationTime = minimum(AnimationTime, m_AnimationTime + Delta);
		else
			m_AnimationTime = maximum(0.0f, m_AnimationTime - Delta);

		if(!ShouldBeVisible && m_AnimationTime <= 0.0f)
		{
			m_DisplayBind = -1;
			return;
		}

		if(m_DisplayBind < 0 || m_DisplayBind >= FAST_ACTIONS_FIXED_SLOTS)
			return;

		const float Progress = std::clamp(m_AnimationTime / AnimationTime, 0.0f, 1.0f);
		aAnimationPhase[0] = QuadEaseInOut(Progress);
		aAnimationPhase[1] = aAnimationPhase[0] * aAnimationPhase[0];
	}

	if(m_DisplayBind < 0 || m_DisplayBind >= FAST_ACTIONS_FIXED_SLOTS)
		return;

	const CUIRect Screen = *Ui()->Screen();

	Ui()->MapScreen();

	const CBind &SelectedBind = m_vBinds[m_DisplayBind];
	char aText[FAST_ACTIONS_MAX_CMD + 16];
	if(SelectedBind.m_aName[0] != '\0')
		str_copy(aText, SelectedBind.m_aName);
	else if(SelectedBind.m_aCommand[0] != '\0')
		str_copy(aText, SelectedBind.m_aCommand);
	else
		str_format(aText, sizeof(aText), Localize("Slot %d is empty"), m_DisplayBind + 1);

	const float TextWidth = TextRender()->TextWidth(s_FontSize, aText);
	const float BoxW = std::clamp(TextWidth + 52.0f, 180.0f, 680.0f) * aAnimationPhase[1];
	const float BoxH = 52.0f * aAnimationPhase[1];
	const float BoxX = Screen.w / 2.0f - BoxW / 2.0f;
	const float BoxY = Screen.h * 0.74f - BoxH / 2.0f;
	Graphics()->DrawRect(BoxX, BoxY, BoxW, BoxH, ColorRGBA(0.0f, 0.0f, 0.0f, 0.55f * aAnimationPhase[0]), IGraphics::CORNER_ALL, 12.0f);

	CUIRect TextRect{BoxX + 14.0f, BoxY + 6.0f, BoxW - 28.0f, BoxH - 12.0f};
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, aAnimationPhase[0]);
	Ui()->DoLabel(&TextRect, aText, s_FontSize * aAnimationPhase[1], TEXTALIGN_MC);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CFastActions::ExecuteBind(int Bind)
{
	if(!g_Config.m_BcFastActions)
		return;

	if(Bind >= 0 && Bind < FAST_ACTIONS_FIXED_SLOTS && m_vBinds[Bind].m_aCommand[0] != '\0')
	{
		const char *pCommand = m_vBinds[Bind].m_aCommand;
		if(pCommand[0] == '/')
		{
			char aBuf[FAST_ACTIONS_MAX_CMD * 2 + 16] = "";
			char *pEnd = aBuf + sizeof(aBuf);
			char *pDst;
			str_append(aBuf, "say \"");
			pDst = aBuf + str_length(aBuf);
			str_escape(&pDst, pCommand, pEnd);
			str_append(aBuf, "\"");
			Console()->ExecuteLine(aBuf, IConsole::CLIENT_ID_UNSPECIFIED);
		}
		else
		{
			Console()->ExecuteLine(pCommand, IConsole::CLIENT_ID_UNSPECIFIED);
		}
	}
}

void CFastActions::ExecuteHoveredBind()
{
	if(!g_Config.m_BcFastActions)
		return;

	if(m_SelectedBind >= 0)
		Console()->ExecuteLine(m_vBinds[m_SelectedBind].m_aCommand, IConsole::CLIENT_ID_UNSPECIFIED);
}

void CFastActions::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	CFastActions *pThis = (CFastActions *)pUserData;
	EnsureFixedBindSlots(pThis->m_vBinds);

	for(int i = 0; i < FAST_ACTIONS_FIXED_SLOTS; i++)
	{
		const CBind &Bind = pThis->m_vBinds[i];
		if(Bind.m_aCommand[0] == '\0')
			continue;

		char aBuf[FAST_ACTIONS_MAX_NAME * 2 + FAST_ACTIONS_MAX_CMD * 2 + 32] = "";
		char *pEnd = aBuf + sizeof(aBuf);
		char *pDst;
		str_format(aBuf, sizeof(aBuf), "fa %d \"", i);
		pDst = aBuf + str_length(aBuf);
		const char *pName = IsLegacySlotName(Bind.m_aName, i) ? "" : Bind.m_aName;
		str_escape(&pDst, pName, pEnd);
		str_append(aBuf, "\" \"");
		pDst = aBuf + str_length(aBuf);
		str_escape(&pDst, Bind.m_aCommand, pEnd);
		str_append(aBuf, "\"");
		pConfigManager->WriteLine(aBuf, ConfigDomain::BESTCLIENT);
	}
}