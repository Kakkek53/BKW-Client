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

inline bool OptimizeBkwEffects()
{
	return g_Config.m_BkwOptimizer && g_Config.m_BkwOptimizerScope == 0 && g_Config.m_BkwOptimizerPreset == 1;
}

inline void ApplyOptimizerProfile()
{
	if(!g_Config.m_BkwOptimizer)
		return;

	if(g_Config.m_BkwOptimizerScope == 0)
	{
		// BKW-only mode is intentionally non-destructive. Individual BKW renderers
		// query OptimizeBkwEffects() and temporarily simplify expensive decoration.
		return;
	}

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

inline void OnRender(CGameClient *pGameClient, IClient *pClient)
{
	ApplyOptimizerProfile();
	UpdateTipoCheat(pGameClient, pClient);
	UpdateAdaptiveAntiPing(pClient);
}

inline float AntiPingAverage() { return s_AntiPingAverage; }
inline float AntiPingJitter() { return s_AntiPingJitter; }
} // namespace BkwFeaturePackRuntime

#endif
