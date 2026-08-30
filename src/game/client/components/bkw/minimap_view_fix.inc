#ifndef GAME_CLIENT_COMPONENTS_BKW_MINIMAP_VIEW_FIX_INC
#define GAME_CLIENT_COMPONENTS_BKW_MINIMAP_VIEW_FIX_INC

namespace BkwMiniMapFix
{
inline vec2 KeepAnchorInsideCircle(vec2 Center, vec2 Anchor, const CUIRect &Rect, float Scale)
{
	if(Scale <= 0.0f)
		return Center;

	// Keep enough room for the local tee marker and the white outline. This is a
	// hard guarantee applied after content clamping, because near map bounds the
	// content clamp can otherwise move the camera far enough that the local tee
	// leaves the circular minimap.
	const float SafeRadius = std::max(1.0f, BkwMiniMap::CircleRadius(Rect) - 5.5f);
	const vec2 Offset = (Anchor - Center) * Scale;
	const float OffsetLength = length(Offset);
	if(OffsetLength <= SafeRadius || OffsetLength <= 0.0001f)
		return Center;

	const vec2 LimitedOffset = Offset * (SafeRadius / OffsetLength);
	return Anchor - LimitedOffset / Scale;
}

inline vec2 ClampMiniCenter(vec2 DesiredCenter, vec2 Anchor, const CUIRect &Rect, float Scale)
{
	return KeepAnchorInsideCircle(BkwMiniMap::ClampCenterToContent(DesiredCenter, Rect, Scale), Anchor, Rect, Scale);
}

inline void ComputeMiniView(CGameClient *pGameClient, const CUIRect &Rect, vec2 &Center, float &Scale)
{
	Scale = std::max(BkwMiniMap::MINI_TILE_PIXELS / BkwMiniMap::TILE_SIZE, BkwMiniMap::CoverScaleToContent(Rect));
	vec2 Anchor = BkwMiniMap::ContentCenter();
	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	const bool LocalActive = LocalId >= 0 && LocalId < MAX_CLIENTS && pGameClient->m_aClients[LocalId].m_Active;
	if(LocalActive)
		Anchor = pGameClient->m_aClients[LocalId].m_RenderPos;

	Center = ClampMiniCenter(Anchor, Anchor, Rect, Scale);

	// Auto zoom is a rescue aid, not a permanent radar mode: it is allowed only
	// while our own tee is frozen. When we are moving normally the minimap keeps
	// its regular local scale even if every other player is outside the circle.
	if(!g_Config.m_BkwMiniMapAutoZoom || !LocalActive || !BkwMiniMap::IsPlayerFrozen(pGameClient, LocalId))
		return;
	if(!BkwMiniMap::HasEligibleMiniMapPlayer(pGameClient) || BkwMiniMap::HasVisibleEligibleMiniMapPlayer(pGameClient, Rect, Center, Scale))
		return;

	const float SafeRadius = std::max(1.0f, BkwMiniMap::CircleRadius(Rect) - 5.0f);
	const float HalfContentDiagonal = 0.5f * std::sqrt(BkwMiniMap::ContentWidth() * BkwMiniMap::ContentWidth() + BkwMiniMap::ContentHeight() * BkwMiniMap::ContentHeight());
	const float MinScale = std::max(0.0001f, std::min(Scale, SafeRadius / std::max(BkwMiniMap::TILE_SIZE, HalfContentDiagonal)));
	vec2 MinCenter = ClampMiniCenter(Anchor, Anchor, Rect, MinScale);

	if(!BkwMiniMap::HasVisibleEligibleMiniMapPlayer(pGameClient, Rect, MinCenter, MinScale))
	{
		Scale = MinScale;
		Center = MinCenter;
		return;
	}

	// Find the closest zoom-out that exposes at least one other active,
	// non-frozen player. Every candidate also keeps our own tee inside the circle.
	float Low = MinScale;
	float High = Scale;
	vec2 BestCenter = MinCenter;
	for(int Iteration = 0; Iteration < 12; ++Iteration)
	{
		const float Mid = (Low + High) * 0.5f;
		const vec2 MidCenter = ClampMiniCenter(Anchor, Anchor, Rect, Mid);
		if(BkwMiniMap::HasVisibleEligibleMiniMapPlayer(pGameClient, Rect, MidCenter, Mid))
		{
			Low = Mid;
			BestCenter = MidCenter;
		}
		else
		{
			High = Mid;
		}
	}
	Scale = Low;
	Center = BestCenter;
}

inline void DrawCircularMiniMap(CGameClient *pGameClient, IGraphics *pGraphics, CUi *pUi, CUIRect Rect, float Alpha)
{
	const vec2 CircleCenter = BkwMiniMap::RectCenter(Rect);
	const float Radius = BkwMiniMap::CircleRadius(Rect);

	pGraphics->TextureClear();
	pGraphics->QuadsBegin();
	pGraphics->SetColor(ColorRGBA(0.018f, 0.022f, 0.032f, 0.88f * Alpha));
	pGraphics->DrawCircle(CircleCenter.x, CircleCenter.y, Radius, 96);
	pGraphics->QuadsEnd();

	CUIRect Inner = Rect;
	Inner.Margin(3.0f, &Inner);
	vec2 Center;
	float Scale;
	ComputeMiniView(pGameClient, Inner, Center, Scale);

	BkwMiniMap::RenderCollisionTiles(pGameClient->Collision(), pGraphics, Inner, Center, Scale, Alpha, true);
	BkwMiniMap::RenderPlayers(pGameClient, pGraphics, pUi, Inner, Center, Scale, Alpha, false, true);
	BkwMiniMap::DrawCircleOutline(pGraphics, CircleCenter, Radius - 1.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.76f * Alpha));
}

inline void DrawSmallMap(CGameClient *pGameClient, IGraphics *pGraphics)
{
	CUi *pUi = pGameClient->Ui();
	const CScreenRect OldScreen = pGraphics->GetScreen();
	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	const float Alpha = std::clamp(g_Config.m_BkwMiniMapOpacity / 100.0f, 0.20f, 1.0f);
	DrawCircularMiniMap(pGameClient, pGraphics, pUi, BkwMiniMap::MiniMapRect(Screen), Alpha);
	pGraphics->MapScreen(OldScreen);
}

inline void Update(CGameClient *pGameClient, IClient *pClient, IGraphics *pGraphics, IInput *pInput)
{
	if(!g_Config.m_BkwMiniMap || !pGameClient || !pClient || !pGraphics || !pInput || pClient->State() != IClient::STATE_ONLINE)
	{
		BkwMiniMap::CloseFull(pGameClient, pInput);
		return;
	}
	if(!BkwMiniMap::EnsureCache(pGameClient->Collision(), pClient))
		return;

	const int Key = g_Config.m_BkwMiniMapFullKey;
	const bool TogglePressed = Key > KEY_UNKNOWN && Key < KEY_LAST && !pGameClient->m_Menus.IsActive() && !pGameClient->m_Chat.IsActive() && !pGameClient->m_GameConsole.IsActive() && pInput->KeyPress(Key) && CBinds::GetModifierMask(pInput) == g_Config.m_BkwMiniMapFullModifier;
	if(TogglePressed)
	{
		if(BkwMiniMap::s_FullOpen)
			BkwMiniMap::CloseFull(pGameClient, pInput);
		else
			BkwMiniMap::OpenFull(pGameClient, pInput, pGraphics);
	}

	if(BkwMiniMap::s_FullOpen)
	{
		const CUIRect Screen = *pGameClient->Ui()->Screen();
		BkwMiniMap::HandleFullMapInput(pGameClient, pInput, BkwMiniMap::FullMapRect(Screen));
		if(BkwMiniMap::s_FullOpen && !BkwMiniMap::s_LateOverlayHealthy)
			BkwMiniMap::DrawFullOverlay(pGameClient, pGraphics);
		return;
	}

	if(!pGameClient->m_Menus.IsActive())
		DrawSmallMap(pGameClient, pGraphics);
}
} // namespace BkwMiniMapFix

#endif // GAME_CLIENT_COMPONENTS_BKW_MINIMAP_VIEW_FIX_INC
