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

	// Use exactly the same corrected center/scale as the collision minimap. This
	// keeps the local tee inside the circle and only enables auto zoom while the
	// local tee is frozen.
	vec2 Center;
	float Scale;
	BkwMiniMapFix::ComputeMiniView(pGameClient, Inner, Center, Scale);

	float BaseWidth = 1.0f;
	float BaseHeight = 1.0f;
	pGraphics->CalcScreenParams(std::max(0.1f, pGraphics->ScreenAspect()), 1.0f, &BaseWidth, &BaseHeight);
	const float CameraZoom = std::max(0.01f, Screen.w / std::max(1.0f, BaseWidth * Scale));

	// CMapRenderer places its temporary camera in the middle of the whole screen.
	// Offset it so the requested world center lands in the top-right minimap.
	const vec2 ScreenCenter = BkwMiniMap::RectCenter(Screen);
	const vec2 MiniCenter = BkwMiniMap::RectCenter(Inner);
	const vec2 PreviewCenter = Center - (MiniCenter - ScreenCenter) / Scale;

	const float PixelScaleX = pGraphics->ScreenWidth() / std::max(1.0f, Screen.w);
	const float PixelScaleY = pGraphics->ScreenHeight() / std::max(1.0f, Screen.h);
	const float PixelSafetyUi = std::max(1.0f / std::max(0.001f, PixelScaleX), 1.0f / std::max(0.001f, PixelScaleY));
	const vec2 CircleCenter = BkwMiniMap::RectCenter(Inner);
	const float SafeRadius = std::max(0.0f, BkwMiniMap::CircleRadius(Inner) - PixelSafetyUi * 1.5f);

	// Build the circular mask from contiguous physical-pixel bands. Every band
	// starts exactly where the previous one ended, so no horizontal gaps appear.
	const int CirclePixelY0 = (int)std::ceil((CircleCenter.y - SafeRadius - Screen.y) * PixelScaleY);
	const int CirclePixelY1 = (int)std::floor((CircleCenter.y + SafeRadius - Screen.y) * PixelScaleY);
	const int CirclePixelHeight = std::max(0, CirclePixelY1 - CirclePixelY0);
	constexpr int MaxBands = 64;
	const int Bands = std::clamp(CirclePixelHeight, 1, MaxBands);

	for(int i = 0; i < Bands; ++i)
	{
		const int ClipY0 = CirclePixelY0 + CirclePixelHeight * i / Bands;
		const int ClipY1 = CirclePixelY0 + CirclePixelHeight * (i + 1) / Bands;
		if(ClipY1 <= ClipY0)
			continue;

		const float Y0 = Screen.y + ClipY0 / PixelScaleY;
		const float Y1 = Screen.y + ClipY1 / PixelScaleY;
		const float FarDy = std::max(std::abs(Y0 - CircleCenter.y), std::abs(Y1 - CircleCenter.y));
		if(FarDy >= SafeRadius)
			continue;

		const float HalfWidth = std::sqrt(std::max(0.0f, SafeRadius * SafeRadius - FarDy * FarDy));
		const int ClipX0 = (int)std::ceil((CircleCenter.x - HalfWidth - Screen.x) * PixelScaleX);
		const int ClipX1 = (int)std::floor((CircleCenter.x + HalfWidth - Screen.x) * PixelScaleX);
		if(ClipX1 <= ClipX0)
			continue;

		const int ClipW = ClipX1 - ClipX0;
		const int ClipH = ClipY1 - ClipY0;

		pGraphics->ClipEnable(ClipX0, ClipY0, ClipW, ClipH);
		pGameClient->m_MapLayersBackground.RenderBkwPreview(PreviewCenter, CameraZoom);
		pGraphics->ClipEnable(ClipX0, ClipY0, ClipW, ClipH);
		pGameClient->m_MapLayersForeground.RenderBkwPreview(PreviewCenter, CameraZoom);
		pGraphics->ClipDisable();
	}

	pUi->MapScreen();
	const float Alpha = std::clamp(g_Config.m_BkwMiniMapOpacity / 100.0f, 0.20f, 1.0f);
	BkwMiniMap::RenderPlayers(pGameClient, pGraphics, pUi, Inner, Center, Scale, Alpha, false, true);
	BkwMiniMap::DrawCircleOutline(pGraphics, BkwMiniMap::RectCenter(Mini), BkwMiniMap::CircleRadius(Mini) - 1.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.76f * Alpha));
	pGraphics->MapScreen(OldScreen);
}
} // namespace BkwMiniMapRealSmall

#endif // GAME_CLIENT_COMPONENTS_BKW_MINIMAP_REAL_SMALL_INC
