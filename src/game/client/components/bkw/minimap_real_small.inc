#ifndef GAME_CLIENT_COMPONENTS_BKW_MINIMAP_REAL_SMALL_INC
#define GAME_CLIENT_COMPONENTS_BKW_MINIMAP_REAL_SMALL_INC

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/components/maplayers.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

#include <algorithm>
#include <cmath>

namespace BkwMiniMapRealSmall
{
inline void Render(CGameClient *pGameClient, IClient *pClient, IGraphics *pGraphics)
{
	if(!g_Config.m_BkwMiniMap || !g_Config.m_BkwMiniMapRealDesign ||
		!pGameClient || !pClient || !pGraphics ||
		pClient->State() != IClient::STATE_ONLINE ||
		BkwMiniMap::IsFullOpen() || pGameClient->m_Menus.IsActive())
		return;

	CUi *pUi = pGameClient->Ui();
	const CScreenRect OldScreen = pGraphics->GetScreen();
	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	const CUIRect Mini = BkwMiniMap::MiniMapRect(Screen);
	CUIRect Inner = Mini;
	Inner.Margin(3.0f, &Inner);

	// Use exactly the same center/scale as the collision minimap, including its
	// auto zoom that ignores frozen players.
	vec2 Center;
	float Scale;
	BkwMiniMap::ComputeMiniView(pGameClient, Inner, Center, Scale);

	float BaseWidth = 1.0f;
	float BaseHeight = 1.0f;
	pGraphics->CalcScreenParams(std::max(0.1f, pGraphics->ScreenAspect()), 1.0f, &BaseWidth, &BaseHeight);
	const float CameraZoom = std::max(0.01f, Screen.w / std::max(1.0f, BaseWidth * Scale));

	// CMapRenderer always places its camera center in the middle of the screen.
	// Offset the temporary camera so the local minimap center lands in the actual
	// top-right circle instead of in the middle of the monitor.
	const vec2 ScreenCenter = BkwMiniMap::RectCenter(Screen);
	const vec2 MiniCenter = BkwMiniMap::RectCenter(Inner);
	const vec2 PreviewCenter = Center - (MiniCenter - ScreenCenter) / Scale;

	// Graphics scissoring is rectangular, so approximate the round mask with
	// conservative horizontal strips. Every strip rectangle is fully INSIDE the
	// mathematical circle: widths are calculated from the farthest strip edge,
	// never from its midpoint, and pixel coordinates are rounded inward.
	constexpr int Strips = 32;
	const vec2 CircleCenter = BkwMiniMap::RectCenter(Inner);
	const float PixelScaleX = pGraphics->ScreenWidth() / std::max(1.0f, Screen.w);
	const float PixelScaleY = pGraphics->ScreenHeight() / std::max(1.0f, Screen.h);
	const float PixelSafetyUi = std::max(1.0f / std::max(0.001f, PixelScaleX), 1.0f / std::max(0.001f, PixelScaleY));
	const float SafeRadius = std::max(0.0f, BkwMiniMap::CircleRadius(Inner) - PixelSafetyUi * 1.5f);

	for(int i = 0; i < Strips; ++i)
	{
		const float Y0 = Inner.y + Inner.h * i / Strips;
		const float Y1 = Inner.y + Inner.h * (i + 1) / Strips;
		const float FarDy = std::max(std::abs(Y0 - CircleCenter.y), std::abs(Y1 - CircleCenter.y));
		if(FarDy >= SafeRadius)
			continue;

		const float HalfWidth = std::sqrt(std::max(0.0f, SafeRadius * SafeRadius - FarDy * FarDy));
		const float X0 = CircleCenter.x - HalfWidth;
		const float X1 = CircleCenter.x + HalfWidth;

		const int ClipX0 = (int)std::ceil((X0 - Screen.x) * PixelScaleX);
		const int ClipX1 = (int)std::floor((X1 - Screen.x) * PixelScaleX);
		const int ClipY0 = (int)std::ceil((Y0 - Screen.y) * PixelScaleY);
		const int ClipY1 = (int)std::floor((Y1 - Screen.y) * PixelScaleY);
		if(ClipX1 <= ClipX0 || ClipY1 <= ClipY0)
			continue;

		pGraphics->ClipEnable(ClipX0, ClipY0, ClipX1 - ClipX0, ClipY1 - ClipY0);
		pGameClient->m_MapLayersBackground.RenderBkwPreview(PreviewCenter, CameraZoom);
		pGameClient->m_MapLayersForeground.RenderBkwPreview(PreviewCenter, CameraZoom);
		pGraphics->ClipDisable();
	}

	// Restore UI mapping after map-layer rendering and redraw dynamic markers on
	// top of the real design so players remain easy to see in the small circle.
	pUi->MapScreen();
	const float Alpha = std::clamp(g_Config.m_BkwMiniMapOpacity / 100.0f, 0.20f, 1.0f);
	BkwMiniMap::RenderPlayers(pGameClient, pGraphics, pUi, Inner, Center, Scale, Alpha, false, true);
	BkwMiniMap::DrawCircleOutline(pGraphics, BkwMiniMap::RectCenter(Mini), BkwMiniMap::CircleRadius(Mini) - 1.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.76f * Alpha));
	pGraphics->MapScreen(OldScreen);
}
} // namespace BkwMiniMapRealSmall

#endif // GAME_CLIENT_COMPONENTS_BKW_MINIMAP_REAL_SMALL_INC
