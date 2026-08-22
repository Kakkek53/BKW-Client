	vLines.reserve(m_vBkwCheckpoints.size() * Segments * 2);
	for(const SBkwCheckpoint &Checkpoint : m_vBkwCheckpoints)
	{
		for(float Radius : {16.0f, 17.5f})
		{
			for(int i = 0; i < Segments; ++i)
			{
				const float A0 = 2.0f * pi * i / Segments;
				const float A1 = 2.0f * pi * (i + 1) / Segments;
				vLines.emplace_back(
					Checkpoint.m_Position.x + std::cos(A0) * Radius,
					Checkpoint.m_Position.y + std::sin(A0) * Radius,
					Checkpoint.m_Position.x + std::cos(A1) * Radius,
					Checkpoint.m_Position.y + std::sin(A1) * Radius);
			}
		}
	}

	Graphics()->TextureClear();
	Graphics()->LinesBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.9f);
	if(!vLines.empty())
		Graphics()->LinesDraw(vLines.data(), vLines.size());
	Graphics()->LinesEnd();
	Graphics()->MapScreen(OldScreen);
}

void CFastActions::BkwLoadCheckpointSettings()
{
	char *pData = Storage()->ReadFileStr(BKW_CHECKPOINT_SETTINGS_FILE, IStorage::TYPE_SAVE);
	if(!pData)
		return;

	std::istringstream Stream(pData);
	std::string Line;
	while(std::getline(Stream, Line))
	{
		if(str_startswith(Line.c_str(), "enabled="))
			m_BkwCheckpointsEnabled = std::atoi(Line.c_str() + 8) != 0;
		else if(str_startswith(Line.c_str(), "mouse="))
			m_BkwCheckpointMouseButton = std::clamp(std::atoi(Line.c_str() + 6), 0, 1);
	}
	std::free(pData);
}

void CFastActions::BkwSaveCheckpointSettings() const
{
	IOHANDLE File = Storage()->OpenFile(BKW_CHECKPOINT_SETTINGS_FILE, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "enabled=%d\nmouse=%d\n", m_BkwCheckpointsEnabled ? 1 : 0, m_BkwCheckpointMouseButton);
	io_write(File, aBuf, str_length(aBuf));
	io_close(File);
}

void CFastActions::SetBkwCheckpointsEnabled(bool Enabled)
{
	if(m_BkwCheckpointsEnabled == Enabled)
		return;
	m_BkwCheckpointsEnabled = Enabled;
	if(!Enabled)
		BkwResetCheckpoints();
	BkwSaveCheckpointSettings();
}

void CFastActions::SetBkwCheckpointMouseButton(int Button)
{
	Button = std::clamp(Button, 0, 1);
	if(m_BkwCheckpointMouseButton == Button)
		return;
	m_BkwCheckpointMouseButton = Button;
	m_BkwCheckpointHolding = false;
	BkwSaveCheckpointSettings();
}

void CFastActions::AddBind(const char *pName, const char *pCommand)
{
	EnsureFixedBindSlots(m_vBinds);
	if(pCommand[0] == '\0')
		return;

	const int NameAsSlot = SlotFromName(pName);
	int Slot = NameAsSlot;
	if(Slot < 0)
	{
		for(int i = 0; i < FAST_ACTIONS_FIXED_SLOTS; i++)
		{
			if(m_vBinds[i].m_aCommand[0] == '\0')
			{
				Slot = i;
				break;
			}
		}
	}
	if(Slot < 0)
		return;

	if(pName[0] == '\0')
	{
		m_vBinds[Slot].m_aName[0] = '\0';
	}
	else if(NameAsSlot < 0)
	{
		str_copy(m_vBinds[Slot].m_aName, pName);
	}
	str_copy(m_vBinds[Slot].m_aCommand, pCommand);
}

void CFastActions::RemoveBind(const char *pName, const char *pCommand)
{
	EnsureFixedBindSlots(m_vBinds);
	const int Slot = SlotFromName(pName);
	if(Slot >= 0)
	{
		if(pCommand[0] == '\0' || str_comp(m_vBinds[Slot].m_aCommand, pCommand) == 0)
			m_vBinds[Slot].m_aCommand[0] = '\0';
		return;
	}

	for(int i = 0; i < FAST_ACTIONS_FIXED_SLOTS; i++)
	{
		if(str_comp(m_vBinds[i].m_aCommand, pCommand) == 0)
		{
			m_vBinds[i].m_aCommand[0] = '\0';
			return;
		}
	}
}

void CFastActions::RemoveBind(int Index)
{
	EnsureFixedBindSlots(m_vBinds);
	if(Index >= FAST_ACTIONS_FIXED_SLOTS || Index < 0)
		return;
	m_vBinds[Index].m_aCommand[0] = '\0';
}

void CFastActions::RemoveAllBinds()
{
	EnsureFixedBindSlots(m_vBinds);
	for(int i = 0; i < FAST_ACTIONS_FIXED_SLOTS; i++)
		m_vBinds[i].m_aCommand[0] = '\0';
}

void CFastActions::OnConsoleInit()
{
	EnsureFixedBindSlots(m_vBinds);
	BkwLoadCheckpointSettings();

	IConfigManager *pConfigManager = Kernel()->RequestInterface<IConfigManager>();
	if(pConfigManager)
		pConfigManager->RegisterCallback(ConfigSaveCallback, this, ConfigDomain::BESTCLIENT);

	Console()->Register("+fa", "", CFGFLAG_CLIENT, ConOpenFa, this, "Open Fast Actions selector");
	Console()->Register("+fa_execute_hover", "", CFGFLAG_CLIENT, ConFaExecuteHover, this, "Execute hovered Fast Actions bind");

	Console()->Register("fa", "i[index] s[name] s[command]", CFGFLAG_CLIENT, ConAddFaLegacy, this, "Set Fast Actions slot bind");
	Console()->Register("add_fa", "s[name] s[command]", CFGFLAG_CLIENT, ConAddFa, this, "Add a bind to Fast Actions");
	Console()->Register("remove_fa", "s[name] s[command]", CFGFLAG_CLIENT, ConRemoveFa, this, "Remove a bind from Fast Actions");
	Console()->Register("delete_all_fa_binds", "", CFGFLAG_CLIENT, ConRemoveAllFaBinds, this, "Removes all Fast Actions binds");
}

void CFastActions::OnReset()
{
	EnsureFixedBindSlots(m_vBinds);
	m_Active = false;
	m_SelectedBind = -1;
	m_DisplayBind = -1;
	m_AnimationTime = 0.0f;
	BkwResetCheckpoints();
	BkwTfMenuOverlayReset();
	BkwDdnetResetView();
	s_BkwTfMenuOverlaySuppressReopen = false;
}

void CFastActions::OnMapLoad()
{
	BkwResetCheckpoints();
	BkwTfMenuOverlayReset();
	BkwDdnetResetView();
	s_BkwTfMenuOverlaySuppressReopen = false;
}

void CFastActions::OnStateChange(int NewState, int OldState)
{
	(void)OldState;
	if(NewState == IClient::STATE_OFFLINE || NewState == IClient::STATE_CONNECTING || NewState == IClient::STATE_LOADING)
	{
		BkwResetCheckpoints();
		s_BkwTfVoteDumpPending = false;
		BkwTfMenuOverlayReset();
		BkwDdnetResetView();
		s_BkwTfMenuOverlaySuppressReopen = false;
		s_BkwTfMenuConnectedToTeeFusion = false;
		s_BkwTfMenuConnectedToDdnet = false;
	}
}

void CFastActions::OnRelease()
{
	m_Active = false;
}

bool CFastActions::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(s_BkwTfMenuOverlayActive)
	{
		Ui()->ConvertMouseMove(&x, &y, CursorType);
		Ui()->OnCursorMove(x, y);
		return true;
	}
	return false;
}

bool CFastActions::OnInput(const IInput::CEvent &Event)
{
	if(s_BkwTfMenuOverlayActive)
	{
		Ui()->OnInput(Event);
		if(Event.m_Key == KEY_ESCAPE && (Event.m_Flags & IInput::FLAG_PRESS) && !(Event.m_Flags & IInput::FLAG_REPEAT))
		{
			BkwTfMenuOverlayReset();
			BkwDdnetResetView();
			s_BkwTfMenuOverlaySuppressReopen = true;
			Ui()->SetActiveItem(nullptr);
			return true;
		}
		return true;
	}

	if(Event.m_Key == KEY_F6 && (Event.m_Flags & IInput::FLAG_PRESS) && !(Event.m_Flags & IInput::FLAG_REPEAT))
	{
		s_BkwTfVoteDumpPending = true;
		s_BkwTfVoteDumpStartedAt = time_get();

		const CVoting &Voting = GameClient()->m_Voting;
		if(Voting.NumOptions() > 0 && !Voting.IsReceivingOptions())
		{
			DumpTfVoteOptions(Voting, Console(), &GameClient()->m_Chat, Client());
			s_BkwTfVoteDumpPending = false;
		}
		else
		{
			const bool PulsedMenuFlag = PulseTfMenuPlayerFlag(GameClient(), Client(), Console());
			char aWaiting[256];
			str_format(aWaiting, sizeof(aWaiting), PulsedMenuFlag ?
				"TF parser: сейчас %d пунктов. Отправил скрытый IN_MENU input, жду vote options..." :
				"TF parser: сейчас %d пунктов, жду получения vote options от сервера...",
				Voting.NumOptions());
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", aWaiting);
			GameClient()->m_Chat.AddColoredLine(aWaiting, ColorRGBA(0.45f, 0.85f, 1.0f, 1.0f));
		}
		return true;
	}

	if(m_BkwCheckpointsEnabled && Client()->State() == IClient::STATE_ONLINE && !GameClient()->m_Menus.IsActive())
	{
		if(Event.m_Key == KEY_MOUSE_3 && (Event.m_Flags & IInput::FLAG_PRESS) && BkwPracticeModeActive())
		{
			m_BkwCheckpointHolding = false;
			m_BkwCheckpointHoldStart = 0;
			BkwTeleportCheckpointAtCursor();
			return true;
		}

		const int ActionKey = m_BkwCheckpointMouseButton == 0 ? KEY_MOUSE_1 : KEY_MOUSE_2;
		if(Event.m_Key == ActionKey)
		{
			if(Event.m_Flags & IInput::FLAG_PRESS)
			{
				m_BkwCheckpointHolding = true;
				m_BkwCheckpointHoldStart = time_get();
			}
			else if(Event.m_Flags & IInput::FLAG_RELEASE)
			{
				if(m_BkwCheckpointHolding)
				{
					const float HeldSeconds = (time_get() - m_BkwCheckpointHoldStart) / (float)time_freq();
					if(HeldSeconds >= BKW_CHECKPOINT_HOLD_SECONDS)
						BkwToggleCheckpointAtTee();
				}
				m_BkwCheckpointHolding = false;
			}
		}
	}

	if(!g_Config.m_BcFastActions)
	{
		m_Active = false;
		m_SelectedBind = -1;
		m_DisplayBind = -1;
		return false;
	}

	if(IsActive() && Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		OnRelease();
		return true;