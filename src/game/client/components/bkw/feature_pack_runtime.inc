#ifndef GAME_CLIENT_COMPONENTS_BKW_FEATURE_PACK_RUNTIME_INC
#define GAME_CLIENT_COMPONENTS_BKW_FEATURE_PACK_RUNTIME_INC

namespace BkwFeaturePackRuntime
{
inline float s_AntiPingAverage = 0.0f;
inline float s_AntiPingJitter = 0.0f;
inline int64_t s_LastAntiPingUpdate = 0;
inline int64_t s_LastEmoteAt = 0;
inline int s_EmoteIndex = 0;
inline bool s_WasSpinActive = false;
inline bool s_BkwVisualOverride = false;
inline int s_SavedTfMenuStyle = 0;
inline int s_SavedDdnetMenuStyle = 0;
inline int s_SavedDuelStyle = 0;

inline bool OptimizeBkwEffects()
{
	return g_Config.m_BkwOptimizer && g_Config.m_BkwOptimizerScope == 0 && g_Config.m_BkwOptimizerPreset == 1;
}

inline void ApplyOptimizerProfile()
{
	const bool BkwMaxFps = OptimizeBkwEffects();
	if(BkwMaxFps && !s_BkwVisualOverride)
	{
		s_SavedTfMenuStyle = g_Config.m_BkwTfMenuStyle;
		s_SavedDdnetMenuStyle = g_Config.m_BkwDdnetMenuStyle;
		s_SavedDuelStyle = g_Config.m_BkwTfDuelHudStyle;
		s_BkwVisualOverride = true;
	}
	if(BkwMaxFps)
	{
		// Temporary low-cost visual overrides. They are restored when the BKW-only
		// Max FPS profile is disabled or changed.
		g_Config.m_BkwTfMenuStyle = 3;
		g_Config.m_BkwDdnetMenuStyle = 3;
		g_Config.m_BkwTfDuelHudStyle = 1;
		return;
	}
	if(s_BkwVisualOverride)
	{
		g_Config.m_BkwTfMenuStyle = s_SavedTfMenuStyle;
		g_Config.m_BkwDdnetMenuStyle = s_SavedDdnetMenuStyle;
		g_Config.m_BkwTfDuelHudStyle = s_SavedDuelStyle;
		s_BkwVisualOverride = false;
	}

	if(!g_Config.m_BkwOptimizer || g_Config.m_BkwOptimizerScope == 0)
		return;

	g_Config.m_BcOptimizer = 1;
	switch(g_Config.m_BkwOptimizerPreset)
	{
	case 1: // Max FPS
		g_Config.m_BcOptimizerDisableParticles = 1;
		g_Config.m_BcOptimizerFpsFog = 1;
		g_Config.m_BcOptimizerFpsFogCullMapTiles = 1;
		break;
	case 2: // Low latency
		g_Config.m_BcOptimizerDisableParticles = 0;
		g_Config.m_BcOptimizerFpsFog = 0;
		g_Config.m_BcOptimizerFpsFogCullMapTiles = 0;
		g_Config.m_ClAntiPing = 1;
		g_Config.m_ClAntiPingPlayers = 2;
		g_Config.m_ClAntiPingSmooth = 1;
		g_Config.m_ClAntiPingPreInput = 1;
		g_Config.m_ClSubTickAiming = 1;
		break;
	default: // Balanced
		g_Config.m_BcOptimizerDisableParticles = 0;
		g_Config.m_BcOptimizerFpsFog = 0;
		g_Config.m_BcOptimizerFpsFogCullMapTiles = 0;
		break;
	}
}

inline void UpdateTipoCheat(CGameClient *pGameClient, IClient *pClient)
{
	const bool SpinActive = g_Config.m_BkwTipoCheat && g_Config.m_BkwTipoCheatSpin &&
		pClient->State() == IClient::STATE_ONLINE && !pGameClient->m_Snap.m_SpecInfo.m_Active;

	if(SpinActive)
	{
		const int Dummy = g_Config.m_ClDummy;
		const float TurnsPerSecond = 1.0f + g_Config.m_BkwTipoCheatSpinSpeed * 0.22f;
		const float Angle = pClient->LocalTime() * TurnsPerSecond * 2.0f * pi;
		pGameClient->m_Controls.m_aMousePos[Dummy] = direction(Angle) * 200.0f;
		pGameClient->m_Controls.m_aMouseInputType[Dummy] = CControls::EMouseInputType::AUTOMATED;
		pGameClient->m_Controls.ClampMousePos();
	}
	else if(s_WasSpinActive)
	{
		pGameClient->m_Controls.m_aMouseInputType[g_Config.m_ClDummy] = CControls::EMouseInputType::RELATIVE;
	}
	s_WasSpinActive = SpinActive;

	if(!g_Config.m_BkwTipoCheat || !g_Config.m_BkwTipoCheatEmoteDance || pClient->State() != IClient::STATE_ONLINE)
		return;

	const int64_t Now = time_get();
	const int64_t Interval = time_freq() * (int64_t)g_Config.m_BkwTipoCheatEmoteInterval / 1000;
	if(s_LastEmoteAt != 0 && Now - s_LastEmoteAt < Interval)
		return;

	pGameClient->m_Emoticon.Emote(s_EmoteIndex % NUM_EMOTICONS);
	s_EmoteIndex = (s_EmoteIndex + 1) % NUM_EMOTICONS;
	s_LastEmoteAt = Now;
}

inline void UpdateAdaptiveAntiPing(IClient *pClient)
{
	if(!g_Config.m_BkwAdaptiveAntiPing || pClient->State() != IClient::STATE_ONLINE)
	{
		s_AntiPingAverage = 0.0f;
		s_AntiPingJitter = 0.0f;
		s_LastAntiPingUpdate = 0;
		return;
	}

	const int64_t Now = time_get();
	if(s_LastAntiPingUpdate != 0 && Now - s_LastAntiPingUpdate < time_freq() / 2)
		return;
	s_LastAntiPingUpdate = Now;

	const float Ping = (float)maximum(0, pClient->ServerInfo().m_Latency);
	if(s_AntiPingAverage <= 0.0f)
		s_AntiPingAverage = Ping;
	const float Delta = absolute(Ping - s_AntiPingAverage);
	s_AntiPingAverage = mix(s_AntiPingAverage, Ping, 0.22f);
	s_AntiPingJitter = mix(s_AntiPingJitter, Delta, 0.28f);

	const float Strength = g_Config.m_BkwAdaptiveAntiPingStrength / 100.0f;
	const int Percent = std::clamp(100 - round_to_int(s_AntiPingJitter * 0.55f * Strength), 72, 100);
	const int Margin = std::clamp(round_to_int((7.0f + s_AntiPingJitter * (0.45f + Strength * 0.55f)) * 10.0f), 50, 900);

	g_Config.m_ClAntiPing = 1;
	g_Config.m_ClAntiPingPlayers = Strength > 0.72f ? 3 : 2;
	g_Config.m_ClAntiPingGrenade = 1;
	g_Config.m_ClAntiPingWeapons = 1;
	g_Config.m_ClAntiPingSmooth = 1;
	g_Config.m_ClAntiPingGunfire = 1;
	g_Config.m_ClAntiPingPreInput = 1;
	g_Config.m_ClAntiPingLimit = 0;
	g_Config.m_ClAntiPingPercent = Percent;
	g_Config.m_ClPredictionMargin = Margin;
}

inline void RenderHookTargetIndicator(CGameClient *pGameClient, IClient *pClient, IGraphics *pGraphics, CCollision *pCollision)
{
	if(!g_Config.m_BkwHookTargetIndicator || pClient->State() != IClient::STATE_ONLINE || pGameClient->m_Snap.m_SpecInfo.m_Active)
		return;

	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	if(LocalId < 0 || LocalId >= MAX_CLIENTS || !pGameClient->m_aClients[LocalId].m_Active)
		return;

	vec2 Aim = pGameClient->m_Controls.m_aMousePos[g_Config.m_ClDummy];
	if(length(Aim) < 0.001f)
		return;
	const vec2 Dir = normalize(Aim);
	const vec2 LocalPos = pGameClient->m_aClients[LocalId].m_RenderPos;
	const vec2 Start = LocalPos + Dir * HOOK_START_DISTANCE;
	float HookLength = pGameClient->m_aClients[LocalId].m_Predicted.m_Tuning.m_HookLength;
	if(HookLength < HOOK_START_DISTANCE)
		HookLength = 380.0f;
	const vec2 FullEnd = LocalPos + Dir * HookLength;

	vec2 CollisionEnd = FullEnd;
	if(pCollision)
	{
		int Tele = 0;
		vec2 HitPos;
		const int Hit = pCollision->IntersectLineTeleHook(Start, FullEnd, &HitPos, nullptr, &Tele);
		if(Hit)
			CollisionEnd = HitPos;
	}

	vec2 CharacterEnd = CollisionEnd;
	vec2 TargetPos;
	const int TargetId = pGameClient->IntersectCharacter(Start, CollisionEnd, CharacterEnd, LocalId, &TargetPos);
	if(TargetId < 0 || TargetId >= MAX_CLIENTS || !pGameClient->m_aClients[TargetId].m_Active)
		return;

	float ScreenW, ScreenH;
	pGraphics->CalcScreenParams(pGraphics->ScreenAspect(), pGameClient->m_Camera.m_Zoom, &ScreenW, &ScreenH);
	const vec2 Center = pGameClient->m_Camera.m_Center;
	pGraphics->MapScreen(Center.x - ScreenW * 0.5f, Center.y - ScreenH * 0.5f, Center.x + ScreenW * 0.5f, Center.y + ScreenH * 0.5f);

	const vec2 Pos = pGameClient->m_aClients[TargetId].m_RenderPos;
	const float Half = g_Config.m_BkwHookTargetSize * 0.5f;
	const float Corner = Half * 0.46f;
	const ColorRGBA Color = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_BkwHookTargetColor, true));
	IGraphics::CLineItem aLines[] = {
		{Pos.x - Half, Pos.y - Half, Pos.x - Half + Corner, Pos.y - Half},
		{Pos.x - Half, Pos.y - Half, Pos.x - Half, Pos.y - Half + Corner},
		{Pos.x + Half, Pos.y - Half, Pos.x + Half - Corner, Pos.y - Half},
		{Pos.x + Half, Pos.y - Half, Pos.x + Half, Pos.y - Half + Corner},
		{Pos.x - Half, Pos.y + Half, Pos.x - Half + Corner, Pos.y + Half},
		{Pos.x - Half, Pos.y + Half, Pos.x - Half, Pos.y + Half - Corner},
		{Pos.x + Half, Pos.y + Half, Pos.x + Half - Corner, Pos.y + Half},
		{Pos.x + Half, Pos.y + Half, Pos.x + Half, Pos.y + Half - Corner},
	};
	pGraphics->TextureClear();
	pGraphics->LinesBegin();
	pGraphics->SetColor(Color);
	pGraphics->LinesDraw(aLines, sizeof(aLines) / sizeof(aLines[0]));
	pGraphics->LinesEnd();
}

inline void OnRender(CGameClient *pGameClient, IClient *pClient, IGraphics *pGraphics, CCollision *pCollision)
{
	ApplyOptimizerProfile();
	UpdateTipoCheat(pGameClient, pClient);
	UpdateAdaptiveAntiPing(pClient);
	RenderHookTargetIndicator(pGameClient, pClient, pGraphics, pCollision);
}

inline float AntiPingAverage() { return s_AntiPingAverage; }
inline float AntiPingJitter() { return s_AntiPingJitter; }
} // namespace BkwFeaturePackRuntime

#endif
