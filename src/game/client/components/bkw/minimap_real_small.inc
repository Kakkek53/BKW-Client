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

	vec2 Center = BkwMiniMap::ContentCenter();
	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	if(LocalId >= 0 && LocalId < MAX_CLIENTS && pGameClient->m_aClients[LocalId].m_Active)
		Center = pGameClient->m_aClients[LocalId].m_RenderPos;

	// Match the exact collision minimap scale. This keeps passages readable and
	// makes the decorative map layers align with the existing player markers.
	const float Scale = std::max(BkwMiniMap::MINI_TILE_PIXELS / BkwMiniMap::TILE_SIZE, BkwMiniMap::CoverScaleToContent(Inner));
	Center = BkwMiniMap::ClampCenterToContent(Center, Inner, Scale);

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

	// The regular graphics API only has rectangular scissoring. Build a circular
	// scissor from horizontal strips. The existing schematic circle stays below,
	// so sub-pixel strip edges never expose the game world through the minimap.
	constexpr int Strips = 16;
	const float Radius = BkwMiniMap::CircleRadius(Inner);
	const vec2 CircleCenter = BkwMiniMap::RectCenter(Inner);
	const float PixelScaleX = pGraphics->ScreenWidth() / std::max(1.0f, Screen.w);
	const float PixelScaleY = pGraphics->ScreenHeight() / std::max(1.0f, Screen.h);

	for(int i = 0; i < Strips; ++i)
	{
		const float Y0 = Inner.y + Inner.h * i / Strips;
		const float Y1 = Inner.y + Inner.h * (i + 1) / Strips;
		const float SampleY = (Y0 + Y1) * 0.5f;
		const float Dy = SampleY - CircleCenter.y;
		const float HalfWidth = std::sqrt(std::max(0.0f, Radius * Radius - Dy * Dy));
		const float X0 = CircleCenter.x - HalfWidth;
		const float X1 = CircleCenter.x + HalfWidth;

		const int ClipX = (int)std::floor((X0 - Screen.x) * PixelScaleX);
		const int ClipY = (int)std::floor((Y0 - Screen.y) * PixelScaleY);
		const int ClipW = std::max(1, (int)std::ceil((X1 - X0) * PixelScaleX));
		const int ClipH = std::max(1, (int)std::ceil((Y1 - Y0) * PixelScaleY) + 1);
		pGraphics->ClipEnable(ClipX, ClipY, ClipW, ClipH);
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
