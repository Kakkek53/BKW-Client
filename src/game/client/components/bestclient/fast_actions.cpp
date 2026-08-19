/* Copyright © 2026 BestProject Team */
#include "fast_actions.h"

#include <base/math.h>
#include <base/system.h>

#include <engine/client/client.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/client/animstate.h>
#include <game/client/components/bkw/tf_menu_parser.inc>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/localization.h>
#include <game/mapitems.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

namespace
{
constexpr const char *BKW_CHECKPOINT_SETTINGS_FILE = "bkw-checkpoints.cfg";
constexpr float BKW_CHECKPOINT_HOLD_SECONDS = 0.35f;
constexpr float BKW_CHECKPOINT_REMOVE_DISTANCE = 42.0f;
constexpr int BKW_CHECKPOINT_MAX = 32;
constexpr int BKW_TF_VOTE_DUMP_TIMEOUT_SECONDS = 8;

bool s_BkwTfVoteDumpPending = false;
int64_t s_BkwTfVoteDumpStartedAt = 0;

void EnsureFixedBindSlots(std::vector<CFastActions::CBind> &vBinds)
{
	if(vBinds.size() != FAST_ACTIONS_FIXED_SLOTS)
		vBinds.resize(FAST_ACTIONS_FIXED_SLOTS);
}

int SlotFromName(const char *pName)
{
	if(!pName || pName[0] == '\0')
		return -1;
	char *pEnd = nullptr;
	const long Value = std::strtol(pName, &pEnd, 10);
	if(*pEnd != '\0')
		return -1;
	const int Index = (int)Value - 1;
	return Index >= 0 && Index < FAST_ACTIONS_FIXED_SLOTS ? Index : -1;
}

bool IsLegacySlotName(const char *pName, int SlotIndex)
{
	if(!pName || pName[0] == '\0')
		return false;

	char aSlotName[16];
	str_format(aSlotName, sizeof(aSlotName), "%d", SlotIndex + 1);
	return str_comp(pName, aSlotName) == 0;
}

int KeyToSlotIndex(int Key)
{
	switch(Key)
	{
	case KEY_1: return 0;
	case KEY_KP_1: return 0;
	case KEY_2: return 1;
	case KEY_KP_2: return 1;
	case KEY_3: return 2;
	case KEY_KP_3: return 2;
	case KEY_4: return 3;
	case KEY_KP_4: return 3;
	case KEY_5: return 4;
	case KEY_KP_5: return 4;
	case KEY_6: return 5;
	case KEY_KP_6: return 5;
	default: return -1;
	}
}

void DumpTfVoteOptions(const CVoting &Voting, IConsole *pConsole, CChat *pChat, IClient *pClient)
{
	pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", "===== TeeFusion vote options dump =====");

	int Index = 0;
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
	{
		char aLine[VOTE_DESC_LENGTH + 32];
		str_format(aLine, sizeof(aLine), "[%d] %s", Index, pOption->m_aDescription);
		pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", aLine);
	}

	char aUserName[64];
	char aSummary[256];
	if(BkwTfMenu::ParseUserName(Voting, aUserName, sizeof(aUserName)))
		str_format(aSummary, sizeof(aSummary), "TF parser: %d пунктов. Имя пользователя: %s", Voting.NumOptions(), aUserName);
	else
		str_format(aSummary, sizeof(aSummary), "TF parser: %d пунктов. Имя пользователя не найдено. Полный список в локальной консоли.", Voting.NumOptions());

	pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", aSummary);
	pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", Voting.IsReceivingOptions() ? "Сервер еще отправляет группу vote options." : "Группа vote options сейчас завершена.");
	pChat->AddColoredLine(aSummary, ColorRGBA(0.45f, 0.85f, 1.0f, 1.0f));
	pClient->Notify("TeeFusion vote parser", aSummary);
}

bool PulseTfMenuPlayerFlag(CGameClient *pGameClient, IClient *pClient, IConsole *pConsole)
{
	if(pClient->State() != IClient::STATE_ONLINE)
		return false;

	const bool WasMenuActive = pGameClient->m_Menus.IsActive();
	if(!WasMenuActive)
		pGameClient->m_Menus.SetActive(true);

	// CControls::SnapInput maps an active menu to PLAYERFLAG_IN_MENU. Force one
	// input packet while this temporary state is active, then restore the local UI
	// state before the frame reaches CMenus::OnRender, so nothing flashes onscreen.
	static_cast<CClient *>(pClient)->SendInput();

	if(!WasMenuActive)
		pGameClient->m_Menus.SetActive(false);

	pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", "Отправлен скрытый PLAYERFLAG_IN_MENU для запроса TeeFusion vote options.");
	return true;
}

#include <game/client/components/bkw/tf_menu_overlay.inc>

} // namespace

CFastActions::CFastActions()
{
	OnReset();
}

void CFastActions::ConFaExecuteHover(IConsole::IResult *pResult, void *pUserData)
{
	CFastActions *pThis = (CFastActions *)pUserData;
	if(!g_Config.m_BcFastActions)
		return;
	pThis->ExecuteHoveredBind();
}

void CFastActions::ConOpenFa(IConsole::IResult *pResult, void *pUserData)
{
	CFastActions *pThis = (CFastActions *)pUserData;
	if(!g_Config.m_BcFastActions)
	{
		pThis->m_Active = false;
		pThis->m_SelectedBind = -1;
		return;
	}
	if(pThis->Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		if(pThis->GameClient()->m_Emoticon.IsActive())
		{
			pThis->m_Active = false;
		}
		else
		{
			const bool NewActive = pResult->GetInteger(0) != 0;
			if(!NewActive && pThis->m_SelectedBind >= 0)
				pThis->ExecuteBind(pThis->m_SelectedBind);
			if(NewActive && !pThis->m_Active)
				pThis->m_SelectedBind = -1;
			if(!NewActive)
				pThis->m_SelectedBind = -1;
			pThis->m_Active = NewActive;
		}
	}
}

void CFastActions::ConAddFaLegacy(IConsole::IResult *pResult, void *pUserData)
{
	int BindPos = pResult->GetInteger(0);
	if(BindPos < 0 || BindPos >= FAST_ACTIONS_FIXED_SLOTS)
		return;

	const char *aName = pResult->GetString(1);
	const char *aCommand = pResult->GetString(2);

	CFastActions *pThis = static_cast<CFastActions *>(pUserData);
	EnsureFixedBindSlots(pThis->m_vBinds);
	str_copy(pThis->m_vBinds[BindPos].m_aName, aName);
	str_copy(pThis->m_vBinds[BindPos].m_aCommand, aCommand);
}

void CFastActions::ConAddFa(IConsole::IResult *pResult, void *pUserData)
{
	const char *aName = pResult->GetString(0);
	const char *aCommand = pResult->GetString(1);

	CFastActions *pThis = static_cast<CFastActions *>(pUserData);
	pThis->AddBind(aName, aCommand);
}

void CFastActions::ConRemoveFa(IConsole::IResult *pResult, void *pUserData)
{
	const char *aName = pResult->GetString(0);
	const char *aCommand = pResult->GetString(1);

	CFastActions *pThis = static_cast<CFastActions *>(pUserData);
	pThis->RemoveBind(aName, aCommand);
}

void CFastActions::ConRemoveAllFaBinds(IConsole::IResult *pResult, void *pUserData)
{
	CFastActions *pThis = (CFastActions *)pUserData;
	pThis->RemoveAllBinds();
}

bool CFastActions::BkwPracticeModeActive() const
{
	if(!m_BkwCheckpointsEnabled || Client()->State() != IClient::STATE_ONLINE)
		return false;

	const int LocalClientId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalClientId < 0 || LocalClientId >= MAX_CLIENTS)
		return false;

	const auto &Character = GameClient()->m_Snap.m_aCharacters[LocalClientId];
	return Character.m_Active && Character.m_HasExtendedData && (Character.m_ExtendedData.m_Flags & CHARACTERFLAG_PRACTICE_MODE) != 0;
}

void CFastActions::BkwResetCheckpoints()
{
	m_vBkwCheckpoints.clear();
	m_BkwCheckpointHolding = false;
	m_BkwCheckpointHoldStart = 0;
}

void CFastActions::BkwToggleCheckpointAtTee()
{
	if(!BkwPracticeModeActive())
		return;

	const vec2 TeePos = GameClient()->m_LocalCharacterPos;
	int ClosestIndex = -1;
	float ClosestDistance = BKW_CHECKPOINT_REMOVE_DISTANCE;
	for(size_t i = 0; i < m_vBkwCheckpoints.size(); ++i)
	{
		const float Dist = distance(TeePos, m_vBkwCheckpoints[i].m_Position);
		if(Dist <= ClosestDistance)
		{
			ClosestDistance = Dist;
			ClosestIndex = (int)i;
		}
	}

	if(ClosestIndex >= 0)
	{
		m_vBkwCheckpoints.erase(m_vBkwCheckpoints.begin() + ClosestIndex);
		return;
	}

	if((int)m_vBkwCheckpoints.size() >= BKW_CHECKPOINT_MAX)
		m_vBkwCheckpoints.erase(m_vBkwCheckpoints.begin());

	SBkwCheckpoint Checkpoint;
	Checkpoint.m_Position = TeePos;
	m_vBkwCheckpoints.push_back(Checkpoint);
}

void CFastActions::BkwTeleportCheckpointAtCursor()
{
	if(!BkwPracticeModeActive() || m_vBkwCheckpoints.empty())
		return;

	const vec2 Pos = m_vBkwCheckpoints.back().m_Position;
	char aCommand[128];
	str_format(aCommand, sizeof(aCommand), "/tpxy %.2f %.2f", Pos.x / 32.0f, Pos.y / 32.0f);
	GameClient()->m_Chat.SendChat(0, aCommand);
}

void CFastActions::BkwRenderCheckpoints()
{
	if(!BkwPracticeModeActive() || m_vBkwCheckpoints.empty())
		return;

	const CScreenRect OldScreen = Graphics()->GetScreen();
	const CMapItemGroup *pGameGroup = GameClient()->Layers()->GameGroup();
	if(pGameGroup)
	{
		const int ParallaxZoom = std::clamp(maximum(pGameGroup->m_ParallaxX, pGameGroup->m_ParallaxY), 0, 100);
		const CScreenRect WorldScreen = Graphics()->MapScreenToWorld(
			GameClient()->m_Camera.m_Center.x, GameClient()->m_Camera.m_Center.y,
			pGameGroup->m_ParallaxX, pGameGroup->m_ParallaxY, (float)ParallaxZoom,
			pGameGroup->m_OffsetX, pGameGroup->m_OffsetY,
			Graphics()->ScreenAspect(), GameClient()->m_Camera.m_Zoom);
		Graphics()->MapScreen(WorldScreen);
	}

	const int Segments = 32;
	std::vector<IGraphics::CLineItem> vLines;
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
	s_BkwTfMenuOverlaySuppressReopen = false;
}

void CFastActions::OnMapLoad()
{
	BkwResetCheckpoints();
	BkwTfMenuOverlayReset();
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
		s_BkwTfMenuOverlaySuppressReopen = false;
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
	if(GameClient()->m_Menus.IsActive() && GameClient()->m_Menus.GamePage() != CMenus::PAGE_CALLVOTE)
		s_BkwTfMenuOverlaySuppressReopen = false;

	if(g_Config.m_BkwTfMenu &&
		!s_BkwTfMenuOverlayActive &&
		!s_BkwTfMenuOverlaySuppressReopen &&
		Client()->State() == IClient::STATE_ONLINE &&
		GameClient()->m_Menus.IsActive() &&
		GameClient()->m_Menus.GamePage() == CMenus::PAGE_CALLVOTE)
	{
		s_BkwTfMenuOverlayActive = true;
		s_BkwTfMenuOverlayPulseSent = false;
		s_BkwTfMenuSelectedTab = 0;
		GameClient()->m_Menus.SetActive(false);
		Ui()->SetActiveItem(nullptr);
	}

	if(s_BkwTfMenuOverlayActive)
	{
		if(!g_Config.m_BkwTfMenu || Client()->State() != IClient::STATE_ONLINE)
		{
			BkwTfMenuOverlayReset();
		}
		else
		{
			const CVoting &Voting = GameClient()->m_Voting;
			if(Voting.NumOptions() <= 0 && !s_BkwTfMenuOverlayPulseSent)
			{
				PulseTfMenuPlayerFlag(GameClient(), Client(), Console());
				s_BkwTfMenuOverlayPulseSent = true;
			}
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
