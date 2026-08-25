#ifndef GAME_CLIENT_COMPONENTS_BKW_LOCAL_DUEL_INC
#define GAME_CLIENT_COMPONENTS_BKW_LOCAL_DUEL_INC

#include <base/str.h>
#include <base/system.h>

#include <engine/console.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include <game/client/components/binds.h>
#include <game/client/gameclient.h>

#include <algorithm>

namespace BkwLocalDuel
{
enum class ESetupStage
{
	NONE = 0,
	SELECT_PLAYER,
	SELECT_LIMIT,
};

inline ESetupStage s_SetupStage = ESetupStage::NONE;
inline int s_PendingOpponentId = -1;
inline int s_OpponentId = -1;
inline int s_LocalScore = 0;
inline int s_OpponentScore = 0;
inline bool s_Active = false;
inline bool s_Finished = false;
inline bool s_LocalArmed = false;
inline bool s_OpponentArmed = false;
inline int64_t s_LocalUnfrozenSince = 0;
inline int64_t s_OpponentUnfrozenSince = 0;
inline char s_aLastOpponentConfig[64] = "";
inline char s_aPendingChat[256] = "";
inline int64_t s_PendingChatAt = 0;

inline int LocalScore() { return s_LocalScore; }
inline int OpponentScore() { return s_OpponentScore; }
inline bool Active() { return s_Active; }
inline bool Finished() { return s_Finished; }
inline ESetupStage SetupStage() { return s_SetupStage; }

inline bool ValidClient(const CGameClient *pGameClient, int ClientId)
{
	return pGameClient != nullptr && ClientId >= 0 && ClientId < MAX_CLIENTS && pGameClient->m_aClients[ClientId].m_Active;
}

inline const char *ClientName(const CGameClient *pGameClient, int ClientId)
{
	return ValidClient(pGameClient, ClientId) ? pGameClient->m_aClients[ClientId].m_aName : "?";
}

inline bool IsFrozen(const CGameClient *pGameClient, int ClientId)
{
	return ValidClient(pGameClient, ClientId) && pGameClient->m_aClients[ClientId].m_Predicted.m_IsInFreeze;
}

inline int ResolveOpponent(const CGameClient *pGameClient, int LocalId)
{
	if(!pGameClient || g_Config.m_BkwLocalDuelOpponent[0] == '\0')
		return -1;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(i == LocalId || !ValidClient(pGameClient, i))
			continue;
		if(str_comp(pGameClient->m_aClients[i].m_aName, g_Config.m_BkwLocalDuelOpponent) == 0)
			return i;
	}
	return -1;
}

inline bool SendPublicChat(CGameClient *pGameClient, const char *pText)
{
	if(!pGameClient || !pText || pText[0] == '\0' || pGameClient->Client()->State() != IClient::STATE_ONLINE)
		return false;

	char aCommand[640] = "say \"";
	char *pDst = aCommand + str_length(aCommand);
	char *pEnd = aCommand + sizeof(aCommand);
	str_escape(&pDst, pText, pEnd);
	str_append(aCommand, "\"");
	pGameClient->Console()->ExecuteLine(aCommand, IConsole::CLIENT_ID_UNSPECIFIED);
	return true;
}

inline void QueuePublicChat(const char *pText, int DelayMs)
{
	str_copy(s_aPendingChat, pText ? pText : "");
	s_PendingChatAt = time_get() + time_freq() * (int64_t)std::max(0, DelayMs) / 1000;
}

inline void FlushPendingChat(CGameClient *pGameClient)
{
	if(s_aPendingChat[0] == '\0' || time_get() < s_PendingChatAt)
		return;
	if(!SendPublicChat(pGameClient, s_aPendingChat))
		return;
	s_aPendingChat[0] = '\0';
	s_PendingChatAt = 0;
}

inline void AnnounceScore(CGameClient *pGameClient, int LocalId)
{
	if(!g_Config.m_BkwLocalDuelChatScore || !ValidClient(pGameClient, LocalId) || !ValidClient(pGameClient, s_OpponentId))
		return;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "[%s: %d] vs [%s: %d]", ClientName(pGameClient, LocalId), s_LocalScore, ClientName(pGameClient, s_OpponentId), s_OpponentScore);
	SendPublicChat(pGameClient, aBuf);
}

inline void QueueWinner(CGameClient *pGameClient, int LocalId, bool Draw)
{
	if(!ValidClient(pGameClient, LocalId) || !ValidClient(pGameClient, s_OpponentId))
		return;
	char aBuf[256];
	if(Draw)
		str_format(aBuf, sizeof(aBuf), "DRAW %s vs %s", ClientName(pGameClient, LocalId), ClientName(pGameClient, s_OpponentId));
	else if(s_LocalScore > s_OpponentScore)
		str_format(aBuf, sizeof(aBuf), "%s WIN %s", ClientName(pGameClient, LocalId), ClientName(pGameClient, s_OpponentId));
	else
		str_format(aBuf, sizeof(aBuf), "%s WIN %s", ClientName(pGameClient, s_OpponentId), ClientName(pGameClient, LocalId));
	QueuePublicChat(aBuf, g_Config.m_BkwLocalDuelChatScore ? 1200 : 0);
}

inline void InitializeFreezeArming(CGameClient *pGameClient, int LocalId)
{
	const int64_t Now = time_get();
	const bool LocalFrozen = IsFrozen(pGameClient, LocalId);
	const bool OpponentFrozen = IsFrozen(pGameClient, s_OpponentId);
	s_LocalArmed = !LocalFrozen;
	s_OpponentArmed = !OpponentFrozen;
	s_LocalUnfrozenSince = LocalFrozen ? 0 : Now;
	s_OpponentUnfrozenSince = OpponentFrozen ? 0 : Now;
}

inline void Stop()
{
	s_SetupStage = ESetupStage::NONE;
	s_PendingOpponentId = -1;
	s_Active = false;
	s_Finished = false;
	s_LocalArmed = false;
	s_OpponentArmed = false;
	s_LocalUnfrozenSince = 0;
	s_OpponentUnfrozenSince = 0;
	s_aPendingChat[0] = '\0';
	s_PendingChatAt = 0;
}

inline void ResetScores(CGameClient *pGameClient, bool StartIfPossible = true)
{
	s_LocalScore = 0;
	s_OpponentScore = 0;
	s_Finished = false;
	s_aPendingChat[0] = '\0';
	s_PendingChatAt = 0;
	str_copy(s_aLastOpponentConfig, g_Config.m_BkwLocalDuelOpponent);

	const int LocalId = pGameClient ? pGameClient->m_Snap.m_LocalClientId : -1;
	if(pGameClient && ValidClient(pGameClient, LocalId))
		s_OpponentId = ResolveOpponent(pGameClient, LocalId);
	else
		s_OpponentId = -1;

	s_Active = StartIfPossible && g_Config.m_BkwLocalDuel && pGameClient && ValidClient(pGameClient, LocalId) && ValidClient(pGameClient, s_OpponentId);
	if(s_Active)
		InitializeFreezeArming(pGameClient, LocalId);
	else
	{
		s_LocalArmed = false;
		s_OpponentArmed = false;
		s_LocalUnfrozenSince = 0;
		s_OpponentUnfrozenSince = 0;
	}
}

inline void BeginSelectPlayer()
{
	s_SetupStage = ESetupStage::SELECT_PLAYER;
	s_PendingOpponentId = -1;
}

inline void CancelSetup()
{
	s_SetupStage = ESetupStage::NONE;
	s_PendingOpponentId = -1;
}

inline void StartDuelWithOpponent(CGameClient *pGameClient, int OpponentId, int Limit)
{
	const int LocalId = pGameClient ? pGameClient->m_Snap.m_LocalClientId : -1;
	if(!ValidClient(pGameClient, LocalId) || !ValidClient(pGameClient, OpponentId) || OpponentId == LocalId)
		return;

	g_Config.m_BkwLocalDuel = 1;
	g_Config.m_BkwLocalDuelLimit = std::clamp(Limit, 0, 100);
	str_copy(g_Config.m_BkwLocalDuelOpponent, pGameClient->m_aClients[OpponentId].m_aName);
	str_copy(s_aLastOpponentConfig, g_Config.m_BkwLocalDuelOpponent);
	s_OpponentId = OpponentId;
	s_SetupStage = ESetupStage::NONE;
	s_PendingOpponentId = -1;
	s_LocalScore = 0;
	s_OpponentScore = 0;
	s_Active = true;
	s_Finished = false;
	s_aPendingChat[0] = '\0';
	s_PendingChatAt = 0;
	InitializeFreezeArming(pGameClient, LocalId);
}

inline int FindPlayerAtAim(CGameClient *pGameClient, int LocalId)
{
	if(!ValidClient(pGameClient, LocalId))
		return -1;

	const vec2 CursorWorld = pGameClient->m_aClients[LocalId].m_RenderPos + pGameClient->m_Controls.m_aMousePos[g_Config.m_ClDummy];
	int BestId = -1;
	float BestDistance = 74.0f;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(i == LocalId || !ValidClient(pGameClient, i))
			continue;
		const float Dist = distance(CursorWorld, pGameClient->m_aClients[i].m_RenderPos);
		if(Dist < BestDistance)
		{
			BestDistance = Dist;
			BestId = i;
		}
	}
	return BestId;
}

inline CUIRect CardRect(const CUIRect &Screen, float Width, float Height, int Position)
{
	constexpr float Margin = 14.0f;
	const int Clamped = std::clamp(Position, 0, 8);
	const int Column = Clamped % 3;
	const int Row = Clamped / 3;
	CUIRect Card{0.0f, 0.0f, Width, Height};
	Card.x = Column == 0 ? Screen.x + Margin : Column == 1 ? Screen.x + (Screen.w - Width) * 0.5f : Screen.x + Screen.w - Width - Margin;
	Card.y = Row == 0 ? Screen.y + Margin : Row == 1 ? Screen.y + (Screen.h - Height) * 0.5f : Screen.y + Screen.h - Height - Margin;
	return Card;
}

inline void DrawLabel(CUi *pUi, const CUIRect &Rect, const char *pText, float Size, int Align, ColorRGBA Color)
{
	SLabelProperties Props;
	Props.m_MaxWidth = Rect.w;
	Props.m_EllipsisAtEnd = true;
	Props.SetColor(Color);
	pUi->DoLabel(&Rect, pText, Size, Align, Props);
}

inline void DrawHud(CGameClient *pGameClient, IGraphics *pGraphics)
{
	if(!g_Config.m_BkwLocalDuel || !g_Config.m_BkwLocalDuelHud || (!s_Active && !s_Finished) || g_Config.m_BkwLocalDuelOpponent[0] == '\0')
		return;

	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	if(!ValidClient(pGameClient, LocalId))
		return;

	CUi *pUi = pGameClient->Ui();
	const CScreenRect OldScreen = pGraphics->GetScreen();
	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	const float Scale = g_Config.m_BkwLocalDuelHudScale / 100.0f;
	const float Width = 360.0f * Scale;
	const float Height = 58.0f * Scale;
	const float Alpha = g_Config.m_BkwLocalDuelHudOpacity / 100.0f;
	const float Rounding = g_Config.m_BkwLocalDuelHudRounding * Scale;
	CUIRect Card = CardRect(Screen, Width, Height, g_Config.m_BkwLocalDuelHudPosition);

	switch(g_Config.m_BkwLocalDuelHudStyle)
	{
	case 1:
		Card.Draw(ColorRGBA(0.025f, 0.028f, 0.035f, 0.88f * Alpha), IGraphics::CORNER_ALL, Rounding);
		break;
	case 2:
	{
		CUIRect Glow = Card;
		Glow.x -= 2.0f * Scale;
		Glow.y -= 2.0f * Scale;
		Glow.w += 4.0f * Scale;
		Glow.h += 4.0f * Scale;
		Glow.Draw(ColorRGBA(0.42f, 0.22f, 0.95f, 0.22f * Alpha), IGraphics::CORNER_ALL, Rounding + 2.0f * Scale);
		Card.Draw(ColorRGBA(0.035f, 0.045f, 0.070f, 0.92f * Alpha), IGraphics::CORNER_ALL, Rounding);
		break;
	}
	case 3:
		Card.Draw(ColorRGBA(0.10f, 0.10f, 0.11f, 0.90f * Alpha), IGraphics::CORNER_ALL, Rounding);
		break;
	default:
		Card.Draw4(ColorRGBA(0.10f, 0.14f, 0.21f, 0.82f * Alpha), ColorRGBA(0.12f, 0.17f, 0.25f, 0.78f * Alpha), ColorRGBA(0.04f, 0.06f, 0.10f, 0.86f * Alpha), ColorRGBA(0.06f, 0.08f, 0.13f, 0.82f * Alpha), IGraphics::CORNER_ALL, Rounding);
		break;
	}

	Card.Margin(8.0f * Scale, &Card);
	CUIRect LeftName, LeftScore, Vs, RightScore, RightName;
	const float NameW = (Card.w - 96.0f * Scale) * 0.5f;
	Card.VSplitLeft(NameW, &LeftName, &Card);
	Card.VSplitLeft(30.0f * Scale, &LeftScore, &Card);
	Card.VSplitLeft(36.0f * Scale, &Vs, &Card);
	Card.VSplitLeft(30.0f * Scale, &RightScore, &RightName);

	char aLeftScore[16], aRightScore[16];
	str_format(aLeftScore, sizeof(aLeftScore), "%d", s_LocalScore);
	str_format(aRightScore, sizeof(aRightScore), "%d", s_OpponentScore);
	const ColorRGBA Text(1.0f, 1.0f, 1.0f, Alpha);
	const ColorRGBA Accent(0.74f, 0.82f, 1.0f, Alpha);
	DrawLabel(pUi, LeftName, ClientName(pGameClient, LocalId), 15.0f * Scale, TEXTALIGN_ML, Text);
	DrawLabel(pUi, LeftScore, aLeftScore, 19.0f * Scale, TEXTALIGN_MC, Accent);
	DrawLabel(pUi, Vs, "VS", 12.0f * Scale, TEXTALIGN_MC, ColorRGBA(0.72f, 0.72f, 0.75f, Alpha));
	DrawLabel(pUi, RightScore, aRightScore, 19.0f * Scale, TEXTALIGN_MC, Accent);
	DrawLabel(pUi, RightName, g_Config.m_BkwLocalDuelOpponent, 15.0f * Scale, TEXTALIGN_MR, Text);
	pGraphics->MapScreen(OldScreen);
}

inline void DrawSelectionTarget(CGameClient *pGameClient, IGraphics *pGraphics, int ClientId)
{
	if(!ValidClient(pGameClient, ClientId))
		return;
	const CScreenRect OldScreen = pGraphics->GetScreen();
	const CMapItemGroup *pGameGroup = pGameClient->Layers()->GameGroup();
	if(pGameGroup)
	{
		const int ParallaxZoom = std::clamp(maximum(pGameGroup->m_ParallaxX, pGameGroup->m_ParallaxY), 0, 100);
		pGraphics->MapScreen(pGraphics->MapScreenToWorld(pGameClient->m_Camera.m_Center.x, pGameClient->m_Camera.m_Center.y, pGameGroup->m_ParallaxX, pGameGroup->m_ParallaxY, (float)ParallaxZoom, pGameGroup->m_OffsetX, pGameGroup->m_OffsetY, pGraphics->ScreenAspect(), pGameClient->m_Camera.m_Zoom));
	}
	const vec2 Pos = pGameClient->m_aClients[ClientId].m_RenderPos;
	const float Half = 32.0f;
	const float Corner = 13.0f;
	IGraphics::CLineItem aLines[] = {
		{Pos.x - Half, Pos.y - Half, Pos.x - Half + Corner, Pos.y - Half}, {Pos.x - Half, Pos.y - Half, Pos.x - Half, Pos.y - Half + Corner},
		{Pos.x + Half, Pos.y - Half, Pos.x + Half - Corner, Pos.y - Half}, {Pos.x + Half, Pos.y - Half, Pos.x + Half, Pos.y - Half + Corner},
		{Pos.x - Half, Pos.y + Half, Pos.x - Half + Corner, Pos.y + Half}, {Pos.x - Half, Pos.y + Half, Pos.x - Half, Pos.y + Half - Corner},
		{Pos.x + Half, Pos.y + Half, Pos.x + Half - Corner, Pos.y + Half}, {Pos.x + Half, Pos.y + Half, Pos.x + Half, Pos.y + Half - Corner},
	};
	pGraphics->TextureClear();
	pGraphics->LinesBegin();
	pGraphics->SetColor(ColorRGBA(0.28f, 1.0f, 0.48f, 0.82f));
	pGraphics->LinesDraw(aLines, sizeof(aLines) / sizeof(aLines[0]));
	pGraphics->LinesEnd();
	pGraphics->MapScreen(OldScreen);
}

inline void DrawSetupPrompt(CGameClient *pGameClient, IGraphics *pGraphics, int CandidateId)
{
	if(s_SetupStage == ESetupStage::NONE)
		return;
	CUi *pUi = pGameClient->Ui();
	const CScreenRect OldScreen = pGraphics->GetScreen();
	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	CUIRect Prompt{Screen.x + (Screen.w - 520.0f) * 0.5f, Screen.y + 18.0f, 520.0f, 58.0f};
	Prompt.Draw(ColorRGBA(0.025f, 0.03f, 0.045f, 0.94f), IGraphics::CORNER_ALL, 12.0f);
	Prompt.Margin(8.0f, &Prompt);
	char aText[256];
	if(s_SetupStage == ESetupStage::SELECT_PLAYER)
	{
		if(CandidateId >= 0)
			str_format(aText, sizeof(aText), "Выбери игрока для дуэли • ЛКМ: %s • Esc: отмена", ClientName(pGameClient, CandidateId));
		else
			str_copy(aText, "Выбери игрока для дуэли • наведи прицел на tee и нажми ЛКМ • Esc: отмена");
	}
	else
	{
		const char *pName = ClientName(pGameClient, s_PendingOpponentId);
		str_format(aText, sizeof(aText), "%s выбран • 1–9: играть до N • 0: фан бесконечно • Enter: текущий лимит %d", pName, g_Config.m_BkwLocalDuelLimit);
	}
	DrawLabel(pUi, Prompt, aText, 13.0f, TEXTALIGN_MC, ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
	pGraphics->MapScreen(OldScreen);
}

inline void UpdateArming(bool Frozen, bool &Armed, int64_t &UnfrozenSince, int64_t Now)
{
	if(Frozen)
	{
		UnfrozenSince = 0;
		return;
	}
	if(UnfrozenSince == 0)
		UnfrozenSince = Now;
	if(!Armed && Now - UnfrozenSince >= time_freq() * 300 / 1000)
		Armed = true;
}

inline void HandleSetupInput(CGameClient *pGameClient, IGraphics *pGraphics)
{
	IInput *pInput = pGameClient->Input();
	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	if(!ValidClient(pGameClient, LocalId))
		return;

	if(s_SetupStage == ESetupStage::NONE)
	{
		const int Key = g_Config.m_BkwLocalDuelSelectKey;
		if(g_Config.m_BkwLocalDuel && Key > KEY_UNKNOWN && Key < KEY_LAST && !pGameClient->m_Menus.IsActive() && !pGameClient->m_Chat.IsActive() && !pGameClient->m_GameConsole.IsActive() && pInput->KeyPress(Key) && CBinds::GetModifierMask(pInput) == g_Config.m_BkwLocalDuelSelectModifier)
			BeginSelectPlayer();
		return;
	}

	if(pInput->KeyPress(KEY_ESCAPE))
	{
		CancelSetup();
		return;
	}

	if(s_SetupStage == ESetupStage::SELECT_PLAYER)
	{
		const int CandidateId = FindPlayerAtAim(pGameClient, LocalId);
		if(CandidateId >= 0)
			DrawSelectionTarget(pGameClient, pGraphics, CandidateId);
		DrawSetupPrompt(pGameClient, pGraphics, CandidateId);
		if(CandidateId >= 0 && pInput->KeyPress(KEY_MOUSE_1))
		{
			s_PendingOpponentId = CandidateId;
			s_SetupStage = ESetupStage::SELECT_LIMIT;
		}
		return;
	}

	DrawSetupPrompt(pGameClient, pGraphics, -1);
	int SelectedLimit = -1;
	if(pInput->KeyPress(KEY_0) || pInput->KeyPress(KEY_KP_0))
		SelectedLimit = 0;
	else
	{
		const int aKeys[9] = {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9};
		const int aKpKeys[9] = {KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_4, KEY_KP_5, KEY_KP_6, KEY_KP_7, KEY_KP_8, KEY_KP_9};
		for(int i = 0; i < 9; ++i)
		{
			if(pInput->KeyPress(aKeys[i]) || pInput->KeyPress(aKpKeys[i]))
			{
				SelectedLimit = i + 1;
				break;
			}
		}
	}
	if((pInput->KeyPress(KEY_RETURN) || pInput->KeyPress(KEY_KP_ENTER)) && SelectedLimit < 0)
		SelectedLimit = g_Config.m_BkwLocalDuelLimit;
	if(SelectedLimit >= 0 && ValidClient(pGameClient, s_PendingOpponentId))
		StartDuelWithOpponent(pGameClient, s_PendingOpponentId, SelectedLimit);
}

inline void Update(CGameClient *pGameClient, IClient *pClient, IGraphics *pGraphics)
{
	FlushPendingChat(pGameClient);
	if(!g_Config.m_BkwLocalDuel || pClient->State() != IClient::STATE_ONLINE)
	{
		s_SetupStage = ESetupStage::NONE;
		s_Active = false;
		s_Finished = false;
		return;
	}

	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	if(!ValidClient(pGameClient, LocalId))
		return;

	if(str_comp(s_aLastOpponentConfig, g_Config.m_BkwLocalDuelOpponent) != 0)
	{
		str_copy(s_aLastOpponentConfig, g_Config.m_BkwLocalDuelOpponent);
		s_OpponentId = ResolveOpponent(pGameClient, LocalId);
		s_LocalScore = 0;
		s_OpponentScore = 0;
		s_Finished = false;
		s_Active = false;
	}
	else if(!ValidClient(pGameClient, s_OpponentId))
		s_OpponentId = ResolveOpponent(pGameClient, LocalId);

	HandleSetupInput(pGameClient, pGraphics);
	if(s_SetupStage != ESetupStage::NONE)
		return;

	if(!s_Active || !ValidClient(pGameClient, s_OpponentId))
	{
		DrawHud(pGameClient, pGraphics);
		return;
	}

	const int64_t Now = time_get();
	const bool LocalFrozen = IsFrozen(pGameClient, LocalId);
	const bool OpponentFrozen = IsFrozen(pGameClient, s_OpponentId);
	UpdateArming(LocalFrozen, s_LocalArmed, s_LocalUnfrozenSince, Now);
	UpdateArming(OpponentFrozen, s_OpponentArmed, s_OpponentUnfrozenSince, Now);

	bool ScoreChanged = false;
	if(LocalFrozen && s_LocalArmed)
	{
		s_LocalArmed = false;
		s_LocalUnfrozenSince = 0;
		++s_OpponentScore;
		ScoreChanged = true;
	}
	if(OpponentFrozen && s_OpponentArmed)
	{
		s_OpponentArmed = false;
		s_OpponentUnfrozenSince = 0;
		++s_LocalScore;
		ScoreChanged = true;
	}

	if(ScoreChanged)
	{
		AnnounceScore(pGameClient, LocalId);
		const int Limit = g_Config.m_BkwLocalDuelLimit;
		if(Limit > 0 && (s_LocalScore >= Limit || s_OpponentScore >= Limit))
		{
			const bool Draw = s_LocalScore >= Limit && s_OpponentScore >= Limit && s_LocalScore == s_OpponentScore;
			s_Active = false;
			s_Finished = true;
			QueueWinner(pGameClient, LocalId, Draw);
		}
	}

	DrawHud(pGameClient, pGraphics);
}

inline void BuildStatus(CGameClient *pGameClient, char *pBuf, size_t BufSize)
{
	if(!pBuf || BufSize == 0)
		return;
	if(g_Config.m_BkwLocalDuelOpponent[0] == '\0')
	{
		str_copy(pBuf, "Противник не выбран.", BufSize);
		return;
	}
	const int LocalId = pGameClient ? pGameClient->m_Snap.m_LocalClientId : -1;
	const int OppId = pGameClient && ValidClient(pGameClient, LocalId) ? ResolveOpponent(pGameClient, LocalId) : -1;
	if(s_Finished)
		str_format(pBuf, BufSize, "Завершено: %d : %d • %s", s_LocalScore, s_OpponentScore, g_Config.m_BkwLocalDuelOpponent);
	else if(s_Active)
		str_format(pBuf, BufSize, "Идёт дуэль: %d : %d • %s", s_LocalScore, s_OpponentScore, g_Config.m_BkwLocalDuelOpponent);
	else if(OppId >= 0)
		str_format(pBuf, BufSize, "Противник найден: %s • нажмите «Старт / сброс счёта».", g_Config.m_BkwLocalDuelOpponent);
	else
		str_format(pBuf, BufSize, "Ожидаю игрока: %s", g_Config.m_BkwLocalDuelOpponent);
}
} // namespace BkwLocalDuel

#endif
