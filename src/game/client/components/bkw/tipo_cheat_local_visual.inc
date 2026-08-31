#ifndef GAME_CLIENT_COMPONENTS_BKW_TIPO_CHEAT_LOCAL_VISUAL_INC
#define GAME_CLIENT_COMPONENTS_BKW_TIPO_CHEAT_LOCAL_VISUAL_INC

namespace BkwTipoCheatLocal
{
inline void MapWorld(CGameClient *pGameClient, IGraphics *pGraphics)
{
	const CMapItemGroup *pGameGroup = pGameClient->Layers()->GameGroup();
	if(!pGameGroup)
		return;
	const int ParallaxZoom = std::clamp(maximum(pGameGroup->m_ParallaxX, pGameGroup->m_ParallaxY), 0, 100);
	const CScreenRect WorldScreen = pGraphics->MapScreenToWorld(
		pGameClient->m_Camera.m_Center.x, pGameClient->m_Camera.m_Center.y,
		pGameGroup->m_ParallaxX, pGameGroup->m_ParallaxY, (float)ParallaxZoom,
		pGameGroup->m_OffsetX, pGameGroup->m_OffsetY,
		pGraphics->ScreenAspect(), pGameClient->m_Camera.m_Zoom);
	pGraphics->MapScreen(WorldScreen);
}

inline void RenderSpin(IGraphics *pGraphics, vec2 Pos, float Time)
{
	const float TurnsPerSecond = 1.0f + g_Config.m_BkwTipoCheatSpinSpeed * 0.22f;
	const float Angle = Time * TurnsPerSecond * 2.0f * pi;
	const float Radius = 43.0f;
	const vec2 Tip = Pos + direction(Angle) * Radius;

	constexpr int Segments = 32;
	IGraphics::CLineItem aRing[Segments];
	for(int i = 0; i < Segments; ++i)
	{
		const float A0 = 2.0f * pi * i / Segments;
		const float A1 = 2.0f * pi * (i + 1) / Segments;
		const vec2 P0 = Pos + direction(A0) * Radius;
		const vec2 P1 = Pos + direction(A1) * Radius;
		aRing[i] = IGraphics::CLineItem(P0.x, P0.y, P1.x, P1.y);
	}

	pGraphics->TextureClear();
	pGraphics->LinesBegin();
	pGraphics->SetColor(ColorRGBA(0.25f, 0.85f, 1.0f, 0.30f));
	pGraphics->LinesDraw(aRing, Segments);
	IGraphics::CLineItem AimLine(Pos.x, Pos.y, Tip.x, Tip.y);
	pGraphics->SetColor(ColorRGBA(0.35f, 0.95f, 1.0f, 0.92f));
	pGraphics->LinesDraw(&AimLine, 1);
	pGraphics->LinesEnd();

	pGraphics->QuadsBegin();
	pGraphics->SetColor(ColorRGBA(0.35f, 0.95f, 1.0f, 0.82f));
	pGraphics->DrawCircle(Tip.x, Tip.y, 5.5f, 18);
	pGraphics->QuadsEnd();
}

inline void RenderLocalEmote(IGraphics *pGraphics, vec2 Pos, float Time)
{
	const int IntervalMs = maximum(1200, g_Config.m_BkwTipoCheatEmoteInterval);
	const int Frame = ((int)(Time * 1000.0f) / IntervalMs) % 4;
	const float Bob = std::sin(Time * 4.0f) * 3.0f;
	const vec2 Bubble = Pos + vec2(0.0f, -62.0f + Bob);
	const float R = 13.0f;

	pGraphics->TextureClear();
	pGraphics->QuadsBegin();
	pGraphics->SetColor(ColorRGBA(0.06f, 0.07f, 0.10f, 0.82f));
	pGraphics->DrawCircle(Bubble.x, Bubble.y, R + 2.0f, 24);
	pGraphics->SetColor(ColorRGBA(1.0f, 0.86f, 0.24f, 0.95f));
	pGraphics->DrawCircle(Bubble.x, Bubble.y, R, 24);
	pGraphics->QuadsEnd();

	IGraphics::CLineItem aFace[5];
	int NumLines = 0;
	const float EyeY = Bubble.y - 3.0f;
	aFace[NumLines++] = IGraphics::CLineItem(Bubble.x - 5.0f, EyeY, Bubble.x - 3.0f, EyeY);
	aFace[NumLines++] = IGraphics::CLineItem(Bubble.x + 3.0f, EyeY, Bubble.x + 5.0f, EyeY);
	if(Frame == 0)
	{
		aFace[NumLines++] = IGraphics::CLineItem(Bubble.x - 5.0f, Bubble.y + 3.0f, Bubble.x, Bubble.y + 6.0f);
		aFace[NumLines++] = IGraphics::CLineItem(Bubble.x, Bubble.y + 6.0f, Bubble.x + 5.0f, Bubble.y + 3.0f);
	}
	else if(Frame == 1)
	{
		aFace[NumLines++] = IGraphics::CLineItem(Bubble.x - 5.0f, Bubble.y + 5.0f, Bubble.x + 5.0f, Bubble.y + 5.0f);
	}
	else if(Frame == 2)
	{
		aFace[NumLines++] = IGraphics::CLineItem(Bubble.x - 5.0f, Bubble.y + 6.0f, Bubble.x, Bubble.y + 2.0f);
		aFace[NumLines++] = IGraphics::CLineItem(Bubble.x, Bubble.y + 2.0f, Bubble.x + 5.0f, Bubble.y + 6.0f);
	}
	else
	{
		aFace[NumLines++] = IGraphics::CLineItem(Bubble.x - 4.0f, Bubble.y + 3.0f, Bubble.x + 4.0f, Bubble.y + 6.0f);
	}

	pGraphics->LinesBegin();
	pGraphics->SetColor(ColorRGBA(0.08f, 0.08f, 0.10f, 0.92f));
	pGraphics->LinesDraw(aFace, NumLines);
	pGraphics->LinesEnd();
}

inline void Render(CGameClient *pGameClient, IClient *pClient, IGraphics *pGraphics)
{
	if(!g_Config.m_BkwTipoCheat || pClient->State() != IClient::STATE_ONLINE || pGameClient->m_Snap.m_SpecInfo.m_Active)
		return;

	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	if(LocalId < 0 || LocalId >= MAX_CLIENTS || !pGameClient->m_aClients[LocalId].m_Active)
		return;

	const CScreenRect OldScreen = pGraphics->GetScreen();
	MapWorld(pGameClient, pGraphics);
	const vec2 Pos = pGameClient->m_aClients[LocalId].m_RenderPos;
	const float Time = pClient->LocalTime();

	if(g_Config.m_BkwTipoCheatSpin)
		RenderSpin(pGraphics, Pos, Time);
	if(g_Config.m_BkwTipoCheatEmoteDance)
		RenderLocalEmote(pGraphics, Pos, Time);

	pGraphics->MapScreen(OldScreen);
}
} // namespace BkwTipoCheatLocal

#endif // GAME_CLIENT_COMPONENTS_BKW_TIPO_CHEAT_LOCAL_VISUAL_INC
