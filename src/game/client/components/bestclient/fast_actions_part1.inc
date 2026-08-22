/* Copyright © 2026 BestProject Team */
#include "fast_actions.h"

#include <base/math.h>
#include <base/system.h>

#include <engine/client/client.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/client/animstate.h>
#include <game/client/components/bkw/server_detector.inc>
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
#include <game/client/components/bkw/ddnet_vote_overlay.inc>

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