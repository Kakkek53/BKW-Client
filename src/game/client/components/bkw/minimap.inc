#ifndef GAME_CLIENT_COMPONENTS_BKW_MINIMAP_INC
#define GAME_CLIENT_COMPONENTS_BKW_MINIMAP_INC

#include <base/math.h>
#include <base/str.h>
#include <base/vmath.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/mapitems.h>
#include <game/client/components/binds.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace BkwMiniMap
{
enum ECellType : uint8_t
{
	CELL_AIR = 0,
	CELL_SOLID,
	CELL_FREEZE,
	CELL_DEATH,
	CELL_START,
	CELL_FINISH,
	CELL_TELE,
	CELL_SPEEDUP,
	CELL_COUNT,
};

inline constexpr float TILE_SIZE = 32.0f;
inline constexpr float MINI_TILE_PIXELS = 3.2f;
inline constexpr int MAX_FULL_VISIBLE_SAMPLES = 260000;
inline int s_MapWidth = 0;
inline int s_MapHeight = 0;
inline char s_aMapName[128] = "";
inline vec2 s_ContentMin(0.0f, 0.0f);
inline vec2 s_ContentMax(0.0f, 0.0f);
inline bool s_FullOpen = false;
inline bool s_Dragging = false;
inline bool s_LateOverlayHealthy = false;
inline vec2 s_FullCenter(0.0f, 0.0f);
inline float s_FullZoom = 1.0f;
inline vec2 s_CursorUiPos(0.0f, 0.0f);
inline vec2 s_LastCursorUiPos(0.0f, 0.0f);

inline ECellType ClassifyTile(const CCollision *pCollision, int Index)
{
	const int Tile = pCollision->GetTileIndex(Index);
	const int Front = pCollision->GetFrontTileIndex(Index);
	auto IsEither = [&](int Value) { return Tile == Value || Front == Value; };

	if(IsEither(TILE_FINISH))
		return CELL_FINISH;
	if(IsEither(TILE_START))
		return CELL_START;
	if(IsEither(TILE_FREEZE) || IsEither(TILE_DFREEZE) || IsEither(TILE_LFREEZE))
		return CELL_FREEZE;
	if(IsEither(TILE_DEATH))
		return CELL_DEATH;
	if(pCollision->IsTeleport(Index) || pCollision->IsEvilTeleport(Index) || pCollision->IsTeleportWeapon(Index) || pCollision->IsTeleportHook(Index) || pCollision->IsTeleCheckpoint(Index))
		return CELL_TELE;
	if(pCollision->IsSpeedup(Index))
		return CELL_SPEEDUP;
	if(IsEither(TILE_SOLID) || IsEither(TILE_NOHOOK))
		return CELL_SOLID;
	return CELL_AIR;
}

inline void ResetCache()
{
	s_MapWidth = 0;
	s_MapHeight = 0;
	s_aMapName[0] = '\0';
	s_ContentMin = vec2(0.0f, 0.0f);
	s_ContentMax = vec2(0.0f, 0.0f);
	s_FullOpen = false;
	s_Dragging = false;
	s_LateOverlayHealthy = false;
	s_FullCenter = vec2(0.0f, 0.0f);
	s_FullZoom = 1.0f;
}

inline bool EnsureCache(CCollision *pCollision, IClient *pClient)
{
	if(!pCollision || !pClient || pCollision->GetWidth() <= 0 || pCollision->GetHeight() <= 0)
		return false;

	const int Width = pCollision->GetWidth();
	const int Height = pCollision->GetHeight();
	const char *pMapName = pClient->ServerInfo().m_aMap;
	if(s_MapWidth == Width && s_MapHeight == Height && str_comp(s_aMapName, pMapName) == 0)
		return true;

	s_MapWidth = Width;
	s_MapHeight = Height;
	str_copy(s_aMapName, pMapName);

	int MinX = Width;
	int MinY = Height;
	int MaxX = -1;
	int MaxY = -1;
	int AllMinX = Width;
	int AllMinY = Height;
	int AllMaxX = -1;
	int AllMaxY = -1;

	// Only bounds are cached. The actual minimap geometry is read from collision
	// tile-by-tile while rendering, so narrow passages can never be merged into a
	// single "highest priority" cell as in the old 256x256 approximation.
	for(int Y = 0; Y < Height; ++Y)
	{
		for(int X = 0; X < Width; ++X)
		{
			const ECellType Type = ClassifyTile(pCollision, Y * Width + X);
			if(Type == CELL_AIR)
				continue;

			AllMinX = std::min(AllMinX, X);
			AllMinY = std::min(AllMinY, Y);
			AllMaxX = std::max(AllMaxX, X);
			AllMaxY = std::max(AllMaxY, Y);

			const bool OuterBorderSolid = Type == CELL_SOLID &&
				(X <= 1 || Y <= 1 || X >= Width - 2 || Y >= Height - 2);
			if(OuterBorderSolid)
				continue;

			MinX = std::min(MinX, X);
			MinY = std::min(MinY, Y);
			MaxX = std::max(MaxX, X);
			MaxY = std::max(MaxY, Y);
		}
	}

	if(MaxX < MinX || MaxY < MinY)
	{
		MinX = AllMinX;
		MinY = AllMinY;
		MaxX = AllMaxX;
		MaxY = AllMaxY;
	}

	const float WorldW = Width * TILE_SIZE;
	const float WorldH = Height * TILE_SIZE;
	if(MaxX >= MinX && MaxY >= MinY)
	{
		s_ContentMin = vec2(MinX * TILE_SIZE, MinY * TILE_SIZE);
		s_ContentMax = vec2((MaxX + 1) * TILE_SIZE, (MaxY + 1) * TILE_SIZE);
		const float Padding = TILE_SIZE * 0.75f;
		s_ContentMin.x = std::max(0.0f, s_ContentMin.x - Padding);
		s_ContentMin.y = std::max(0.0f, s_ContentMin.y - Padding);
		s_ContentMax.x = std::min(WorldW, s_ContentMax.x + Padding);
		s_ContentMax.y = std::min(WorldH, s_ContentMax.y + Padding);
	}
	else
	{
		s_ContentMin = vec2(0.0f, 0.0f);
		s_ContentMax = vec2(WorldW, WorldH);
	}

	s_FullOpen = false;
	s_Dragging = false;
	s_LateOverlayHealthy = false;
	s_FullCenter = (s_ContentMin + s_ContentMax) * 0.5f;
	s_FullZoom = 1.0f;
	return true;
}

inline ColorRGBA CellColor(ECellType Type, float Alpha)
{
	switch(Type)
	{
	case CELL_SOLID: return ColorRGBA(0.58f, 0.60f, 0.64f, 0.86f * Alpha);
	case CELL_FREEZE: return ColorRGBA(0.25f, 0.72f, 1.00f, 0.92f * Alpha);
	case CELL_DEATH: return ColorRGBA(1.00f, 0.30f, 0.30f, 0.92f * Alpha);
	case CELL_START: return ColorRGBA(0.32f, 1.00f, 0.48f, 0.96f * Alpha);
	case CELL_FINISH: return ColorRGBA(1.00f, 0.86f, 0.28f, 0.98f * Alpha);
	case CELL_TELE: return ColorRGBA(0.72f, 0.42f, 1.00f, 0.90f * Alpha);
	case CELL_SPEEDUP: return ColorRGBA(1.00f, 0.62f, 0.22f, 0.90f * Alpha);
	default: return ColorRGBA(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

inline void DrawQuadBatch(IGraphics *pGraphics, const std::vector<IGraphics::CQuadItem> &vQuads, ColorRGBA Color)
{
	if(vQuads.empty())
		return;
	pGraphics->TextureClear();
	pGraphics->QuadsBegin();
	pGraphics->SetColor(Color);
	for(size_t Offset = 0; Offset < vQuads.size(); Offset += GRAPHICS_MAX_QUADS_RENDER_COUNT)
	{
		const int Count = (int)std::min<size_t>(GRAPHICS_MAX_QUADS_RENDER_COUNT, vQuads.size() - Offset);
		pGraphics->QuadsDrawTL(vQuads.data() + Offset, Count);
	}
	pGraphics->QuadsEnd();
}

inline float ContentWidth() { return std::max(TILE_SIZE, s_ContentMax.x - s_ContentMin.x); }
inline float ContentHeight() { return std::max(TILE_SIZE, s_ContentMax.y - s_ContentMin.y); }
inline vec2 ContentCenter() { return (s_ContentMin + s_ContentMax) * 0.5f; }

inline float FitScaleToContent(const CUIRect &Rect)
{
	return std::max(0.0001f, std::min(Rect.w / ContentWidth(), Rect.h / ContentHeight()));
}

inline float CoverScaleToContent(const CUIRect &Rect)
{
	return std::max(0.0001f, std::max(Rect.w / ContentWidth(), Rect.h / ContentHeight()));
}

inline vec2 ClampCenterToContent(vec2 Center, const CUIRect &Rect, float Scale)
{
	const float HalfW = Rect.w / (2.0f * Scale);
	const float HalfH = Rect.h / (2.0f * Scale);
	if(HalfW * 2.0f >= ContentWidth())
		Center.x = ContentCenter().x;
	else
		Center.x = std::clamp(Center.x, s_ContentMin.x + HalfW, s_ContentMax.x - HalfW);
	if(HalfH * 2.0f >= ContentHeight())
		Center.y = ContentCenter().y;
	else
		Center.y = std::clamp(Center.y, s_ContentMin.y + HalfH, s_ContentMax.y - HalfH);
	return Center;
}

inline vec2 WorldToMapScreen(vec2 World, const CUIRect &Rect, vec2 Center, float Scale)
{
	return vec2(Rect.x + Rect.w * 0.5f + (World.x - Center.x) * Scale,
		Rect.y + Rect.h * 0.5f + (World.y - Center.y) * Scale);
}

inline bool PointInside(const CUIRect &Rect, vec2 Point)
{
	return Point.x >= Rect.x && Point.x <= Rect.x + Rect.w && Point.y >= Rect.y && Point.y <= Rect.y + Rect.h;
}

inline vec2 RectCenter(const CUIRect &Rect) { return vec2(Rect.x + Rect.w * 0.5f, Rect.y + Rect.h * 0.5f); }
inline float CircleRadius(const CUIRect &Rect) { return std::min(Rect.w, Rect.h) * 0.5f; }
inline bool PointInsideCircle(const CUIRect &Rect, vec2 Point, float Margin = 0.0f)
{
	return distance(RectCenter(Rect), Point) <= std::max(0.0f, CircleRadius(Rect) - Margin);
}

inline bool RectFullyInsideCircle(float X, float Y, float W, float H, const CUIRect &CircleRect)
{
	return PointInsideCircle(CircleRect, vec2(X, Y)) &&
		PointInsideCircle(CircleRect, vec2(X + W, Y)) &&
		PointInsideCircle(CircleRect, vec2(X, Y + H)) &&
		PointInsideCircle(CircleRect, vec2(X + W, Y + H));
}

inline bool RectIntersectsCircle(float X, float Y, float W, float H, const CUIRect &CircleRect)
{
	const vec2 C = RectCenter(CircleRect);
	const float ClosestX = std::clamp(C.x, X, X + W);
	const float ClosestY = std::clamp(C.y, Y, Y + H);
	return distance(C, vec2(ClosestX, ClosestY)) <= CircleRadius(CircleRect);
}

inline void AppendVisibleQuad(std::vector<IGraphics::CQuadItem> &vQuads, float X, float Y, float W, float H, const CUIRect &Rect, bool CircularClip)
{
	const float X0 = std::max(X, Rect.x);
	const float Y0 = std::max(Y, Rect.y);
	const float X1 = std::min(X + W, Rect.x + Rect.w);
	const float Y1 = std::min(Y + H, Rect.y + Rect.h);
	if(X1 <= X0 || Y1 <= Y0)
		return;

	const float ClippedW = X1 - X0;
	const float ClippedH = Y1 - Y0;
	if(!CircularClip)
	{
		vQuads.emplace_back(X0, Y0, std::max(0.7f, ClippedW), std::max(0.7f, ClippedH));
		return;
	}

	if(!RectIntersectsCircle(X0, Y0, ClippedW, ClippedH, Rect))
		return;
	if(RectFullyInsideCircle(X0, Y0, ClippedW, ClippedH, Rect))
	{
		vQuads.emplace_back(X0, Y0, ClippedW, ClippedH);
		return;
	}

	// Only boundary tiles are split. This fills the round edge much more tightly
	// than dropping every square that touches the circle, while keeping the center
	// of the minimap exact and cheap.
	constexpr int Subdivisions = 4;
	const float SubW = ClippedW / Subdivisions;
	const float SubH = ClippedH / Subdivisions;
	for(int Sy = 0; Sy < Subdivisions; ++Sy)
	{
		for(int Sx = 0; Sx < Subdivisions; ++Sx)
		{
			const float SubX = X0 + Sx * SubW;
			const float SubY = Y0 + Sy * SubH;
			if(PointInsideCircle(Rect, vec2(SubX + SubW * 0.5f, SubY + SubH * 0.5f)))
				vQuads.emplace_back(SubX, SubY, SubW + 0.08f, SubH + 0.08f);
		}
	}
}

inline void RenderCollisionTiles(CCollision *pCollision, IGraphics *pGraphics, const CUIRect &Rect, vec2 Center, float Scale, float Alpha, bool CircularClip)
{
	if(!pCollision || Scale <= 0.0f)
		return;

	const float HalfWorldW = Rect.w / (2.0f * Scale);
	const float HalfWorldH = Rect.h / (2.0f * Scale);
	const int X0 = std::clamp((int)std::floor((Center.x - HalfWorldW) / TILE_SIZE) - 1, 0, s_MapWidth);
	const int Y0 = std::clamp((int)std::floor((Center.y - HalfWorldH) / TILE_SIZE) - 1, 0, s_MapHeight);
	const int X1 = std::clamp((int)std::ceil((Center.x + HalfWorldW) / TILE_SIZE) + 1, 0, s_MapWidth);
	const int Y1 = std::clamp((int)std::ceil((Center.y + HalfWorldH) / TILE_SIZE) + 1, 0, s_MapHeight);
	if(X1 <= X0 || Y1 <= Y0)
		return;

	const int64_t VisibleTiles = (int64_t)(X1 - X0) * (Y1 - Y0);
	int Step = 1;
	if(!CircularClip && VisibleTiles > MAX_FULL_VISIBLE_SAMPLES)
		Step = std::max(1, (int)std::ceil(std::sqrt((double)VisibleTiles / MAX_FULL_VISIBLE_SAMPLES)));

	std::array<std::vector<IGraphics::CQuadItem>, CELL_COUNT> avQuads;
	for(int Y = Y0; Y < Y1; Y += Step)
	{
		for(int X = X0; X < X1; X += Step)
		{
			const int SampleX = std::min(X + Step / 2, s_MapWidth - 1);
			const int SampleY = std::min(Y + Step / 2, s_MapHeight - 1);
			const ECellType Type = ClassifyTile(pCollision, SampleY * s_MapWidth + SampleX);
			if(Type == CELL_AIR)
				continue;

			// Exact mode draws the exact tile. If a gigantic Full Map needs sampling,
			// draw only the sampled tile instead of filling the whole sample block.
			// That may omit tiny detail at extreme zoom-out, but it cannot invent a
			// thick wall across a real passage.
			const int DrawX = Step == 1 ? X : SampleX;
			const int DrawY = Step == 1 ? Y : SampleY;
			const vec2 P0 = WorldToMapScreen(vec2(DrawX * TILE_SIZE, DrawY * TILE_SIZE), Rect, Center, Scale);
			const vec2 P1 = WorldToMapScreen(vec2((DrawX + 1) * TILE_SIZE, (DrawY + 1) * TILE_SIZE), Rect, Center, Scale);
			AppendVisibleQuad(avQuads[Type], P0.x, P0.y, P1.x - P0.x, P1.y - P0.y, Rect, CircularClip);
		}
	}

	for(int Type = CELL_SOLID; Type < CELL_COUNT; ++Type)
		DrawQuadBatch(pGraphics, avQuads[Type], CellColor((ECellType)Type, Alpha));
}

inline void RenderPlayers(CGameClient *pGameClient, IGraphics *pGraphics, CUi *pUi, const CUIRect &Rect, vec2 Center, float Scale, float Alpha, bool ShowNames, bool CircularClip = false)
{
	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ++ClientId)
	{
		if(!pGameClient->m_aClients[ClientId].m_Active)
			continue;
		const vec2 ScreenPos = WorldToMapScreen(pGameClient->m_aClients[ClientId].m_RenderPos, Rect, Center, Scale);
		const bool Frozen = pGameClient->m_aClients[ClientId].m_Predicted.m_IsInFreeze || pGameClient->m_aClients[ClientId].m_DeepFrozen;
		ColorRGBA Color = ClientId == LocalId ? ColorRGBA(0.30f, 1.00f, 0.42f, Alpha) : ColorRGBA(1.0f, 1.0f, 1.0f, Alpha);
		if(Frozen)
			Color = ColorRGBA(0.35f, 0.82f, 1.0f, Alpha);
		const float Radius = ShowNames ? 3.8f : 2.8f;

		if(CircularClip ? !PointInsideCircle(Rect, ScreenPos, Radius) : !PointInside(Rect, ScreenPos))
			continue;

		pGraphics->TextureClear();
		pGraphics->QuadsBegin();
		pGraphics->SetColor(Color);
		pGraphics->DrawCircle(ScreenPos.x, ScreenPos.y, Radius, 16);
		pGraphics->QuadsEnd();

		if(ShowNames)
		{
			CUIRect NameRect{ScreenPos.x + 6.0f, ScreenPos.y - 7.0f, 120.0f, 14.0f};
			SLabelProperties Props;
			Props.m_MaxWidth = NameRect.w;
			Props.m_EllipsisAtEnd = true;
			Props.SetColor(Color);
			pUi->DoLabel(&NameRect, pGameClient->m_aClients[ClientId].m_aName, 10.0f, TEXTALIGN_ML, Props);
		}
	}
}

inline void DrawCircleOutline(IGraphics *pGraphics, vec2 Center, float Radius, ColorRGBA Color)
{
	constexpr int Segments = 96;
	std::array<IGraphics::CLineItem, Segments> aLines;
	for(int i = 0; i < Segments; ++i)
	{
		const float A0 = 2.0f * pi * i / Segments;
		const float A1 = 2.0f * pi * (i + 1) / Segments;
		aLines[i] = IGraphics::CLineItem(Center.x + std::cos(A0) * Radius, Center.y + std::sin(A0) * Radius, Center.x + std::cos(A1) * Radius, Center.y + std::sin(A1) * Radius);
	}
	pGraphics->TextureClear();
	pGraphics->LinesBegin();
	pGraphics->SetColor(Color);
	pGraphics->LinesDraw(aLines.data(), aLines.size());
	pGraphics->LinesEnd();
}

inline float FullScale(const CUIRect &Rect, float Zoom)
{
	CUIRect Inner = Rect;
	Inner.Margin(5.0f, &Inner);
	return CoverScaleToContent(Inner) * std::max(1.0f, Zoom);
}

inline void DrawSchematicFullMap(CGameClient *pGameClient, IGraphics *pGraphics, CUi *pUi, CUIRect Rect, vec2 Center, float Zoom, float Alpha)
{
	Rect.Draw(ColorRGBA(0.018f, 0.022f, 0.032f, 0.96f), IGraphics::CORNER_ALL, 10.0f);
	CUIRect Inner = Rect;
	Inner.Margin(5.0f, &Inner);
	const float Scale = CoverScaleToContent(Inner) * std::max(1.0f, Zoom);
	Center = ClampCenterToContent(Center, Inner, Scale);
	RenderCollisionTiles(pGameClient->Collision(), pGraphics, Inner, Center, Scale, Alpha, false);
	RenderPlayers(pGameClient, pGraphics, pUi, Inner, Center, Scale, Alpha, true);
}

inline void DrawCircularMiniMap(CGameClient *pGameClient, IGraphics *pGraphics, CUi *pUi, CUIRect Rect, float Alpha)
{
	const vec2 CircleCenter = RectCenter(Rect);
	const float Radius = CircleRadius(Rect);

	pGraphics->TextureClear();
	pGraphics->QuadsBegin();
	pGraphics->SetColor(ColorRGBA(0.018f, 0.022f, 0.032f, 0.88f * Alpha));
	pGraphics->DrawCircle(CircleCenter.x, CircleCenter.y, Radius, 96);
	pGraphics->QuadsEnd();

	CUIRect Inner = Rect;
	Inner.Margin(3.0f, &Inner);
	const float Scale = std::max(MINI_TILE_PIXELS / TILE_SIZE, CoverScaleToContent(Inner));
	vec2 Center = ContentCenter();
	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	if(LocalId >= 0 && LocalId < MAX_CLIENTS && pGameClient->m_aClients[LocalId].m_Active)
		Center = pGameClient->m_aClients[LocalId].m_RenderPos;
	Center = ClampCenterToContent(Center, Inner, Scale);

	RenderCollisionTiles(pGameClient->Collision(), pGraphics, Inner, Center, Scale, Alpha, true);
	RenderPlayers(pGameClient, pGraphics, pUi, Inner, Center, Scale, Alpha, false, true);
	DrawCircleOutline(pGraphics, CircleCenter, Radius - 1.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.76f * Alpha));
}

inline CUIRect FullMapRect(const CUIRect &Screen)
{
	CUIRect Result = Screen;
	Result.Margin(12.0f, &Result);
	Result.y += 18.0f;
	Result.h = std::max(80.0f, Result.h - 18.0f);
	return Result;
}

inline CUIRect MiniMapRect(const CUIRect &Screen)
{
	const float Size = (float)std::clamp(g_Config.m_BkwMiniMapSize, 120, 300);
	return CUIRect{Screen.x + Screen.w - Size - 14.0f, Screen.y + 14.0f, Size, Size};
}

inline bool IsFullOpen() { return s_FullOpen; }

inline bool HandleCursorMove(CUi *pUi, float X, float Y, IInput::ECursorType CursorType)
{
	if(!s_FullOpen || !pUi)
		return false;
	pUi->ConvertMouseMove(&X, &Y, CursorType);
	const CUIRect Screen = *pUi->Screen();
	s_CursorUiPos.x = std::clamp(s_CursorUiPos.x + X, Screen.x, Screen.x + Screen.w - 1.0f);
	s_CursorUiPos.y = std::clamp(s_CursorUiPos.y + Y, Screen.y, Screen.y + Screen.h - 1.0f);
	return true;
}

inline void CloseFull(CGameClient *pGameClient, IInput *pInput)
{
	(void)pGameClient;
	(void)pInput;
	if(!s_FullOpen)
		return;
	s_FullOpen = false;
	s_Dragging = false;
	s_LateOverlayHealthy = false;
}

inline void OpenFull(CGameClient *pGameClient, IInput *pInput, IGraphics *pGraphics)
{
	(void)pInput;
	(void)pGraphics;
	s_FullOpen = true;
	s_Dragging = false;
	s_LateOverlayHealthy = false;
	s_FullZoom = std::clamp(g_Config.m_BkwMiniMapFullZoom / 100.0f, 1.0f, 8.0f);
	const int LocalId = pGameClient ? pGameClient->m_Snap.m_LocalClientId : -1;
	if(pGameClient && LocalId >= 0 && LocalId < MAX_CLIENTS && pGameClient->m_aClients[LocalId].m_Active)
		s_FullCenter = pGameClient->m_aClients[LocalId].m_RenderPos;
	else
		s_FullCenter = ContentCenter();
	if(pGameClient)
	{
		const CUIRect Screen = *pGameClient->Ui()->Screen();
		s_CursorUiPos = RectCenter(Screen);
		s_LastCursorUiPos = s_CursorUiPos;
	}
}

inline void ApplyWheelZoom(const CUIRect &MapRect, float NewZoom)
{
	const float OldZoom = s_FullZoom;
	NewZoom = std::clamp(NewZoom, 1.0f, 8.0f);
	if(std::abs(NewZoom - OldZoom) < 0.0001f)
		return;

	CUIRect Inner = MapRect;
	Inner.Margin(5.0f, &Inner);
	const float OldScale = CoverScaleToContent(Inner) * OldZoom;
	const float NewScale = CoverScaleToContent(Inner) * NewZoom;
	const vec2 CursorOffset = s_CursorUiPos - RectCenter(Inner);
	const vec2 WorldUnderCursor = s_FullCenter + CursorOffset / OldScale;
	s_FullCenter = WorldUnderCursor - CursorOffset / NewScale;
	s_FullZoom = NewZoom;
	s_FullCenter = ClampCenterToContent(s_FullCenter, Inner, NewScale);
}

inline void HandleFullMapInput(CGameClient *pGameClient, IInput *pInput, const CUIRect &MapRect)
{
	if(!s_FullOpen)
		return;
	if(pInput->KeyPress(KEY_ESCAPE))
	{
		CloseFull(pGameClient, pInput);
		return;
	}

	if(PointInside(MapRect, s_CursorUiPos))
	{
		if(pInput->KeyPress(KEY_MOUSE_WHEEL_UP))
			ApplyWheelZoom(MapRect, s_FullZoom * 1.16f);
		if(pInput->KeyPress(KEY_MOUSE_WHEEL_DOWN))
			ApplyWheelZoom(MapRect, s_FullZoom / 1.16f);
	}

	CUIRect Inner = MapRect;
	Inner.Margin(5.0f, &Inner);
	const float Scale = CoverScaleToContent(Inner) * s_FullZoom;
	if(pInput->KeyIsPressed(KEY_MOUSE_1) && PointInside(MapRect, s_CursorUiPos))
	{
		if(!s_Dragging)
		{
			s_Dragging = true;
			s_LastCursorUiPos = s_CursorUiPos;
		}
		else
		{
			const vec2 Delta = s_CursorUiPos - s_LastCursorUiPos;
			s_FullCenter -= Delta / Scale;
			s_FullCenter = ClampCenterToContent(s_FullCenter, Inner, Scale);
			s_LastCursorUiPos = s_CursorUiPos;
		}
	}
	else
	{
		s_Dragging = false;
		s_LastCursorUiPos = s_CursorUiPos;
	}
}

inline void DrawMenuCursor(CGameClient *pGameClient, IGraphics *pGraphics)
{
	constexpr float CursorSize = 24.0f;
	const IGraphics::CTextureHandle CursorTexture = pGameClient->CursorTexture();
	if(CursorTexture.IsValid())
	{
		pGraphics->TextureSet(CursorTexture);
		pGraphics->QuadsBegin();
		pGraphics->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		IGraphics::CQuadItem Cursor(s_CursorUiPos.x, s_CursorUiPos.y, CursorSize, CursorSize);
		pGraphics->QuadsDrawTL(&Cursor, 1);
		pGraphics->QuadsEnd();
	}
	else
	{
		pGraphics->TextureClear();
		pGraphics->QuadsBegin();
		pGraphics->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		pGraphics->DrawCircle(s_CursorUiPos.x, s_CursorUiPos.y, 3.5f, 16);
		pGraphics->QuadsEnd();
	}
}

inline void DrawSmallMap(CGameClient *pGameClient, IGraphics *pGraphics)
{
	CUi *pUi = pGameClient->Ui();
	const CScreenRect OldScreen = pGraphics->GetScreen();
	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	const float Alpha = std::clamp(g_Config.m_BkwMiniMapOpacity / 100.0f, 0.20f, 1.0f);
	DrawCircularMiniMap(pGameClient, pGraphics, pUi, MiniMapRect(Screen), Alpha);
	pGraphics->MapScreen(OldScreen);
}

inline void DrawRealDesignFullMap(CGameClient *pGameClient, IGraphics *pGraphics, CUi *pUi, const CUIRect &Screen, const CUIRect &MapRect, float Alpha)
{
	const float Aspect = std::max(0.1f, pGraphics->ScreenAspect());
	// DDNet's world camera is 300 units high at zoom 1. Choose the largest camera
	// viewport that stays inside the content bounds (cover), then our FullZoom
	// acts as zoom-in exactly like the schematic mode.
	const float CoverCameraZoom = std::max(0.01f, std::min(ContentHeight() / 300.0f, ContentWidth() / (300.0f * Aspect)));
	const float CameraZoom = CoverCameraZoom / std::max(1.0f, s_FullZoom);

	pGameClient->m_MapLayersBackground.RenderBkwPreview(s_FullCenter, CameraZoom);
	pGameClient->m_MapLayersForeground.RenderBkwPreview(s_FullCenter, CameraZoom);
	pGraphics->ClipDisable();

	const CScreenRect WorldRect = pGraphics->MapScreenToWorld(s_FullCenter.x, s_FullCenter.y, 100.0f, 100.0f, 100.0f, 0.0f, 0.0f, Aspect, CameraZoom);
	pUi->MapScreen();
	const float Scale = Screen.w / std::max(1.0f, WorldRect.Width());
	RenderPlayers(pGameClient, pGraphics, pUi, Screen, s_FullCenter, Scale, Alpha, true);

	// A subtle frame shows the interactive area while the real map design itself
	// remains almost full-screen.
	MapRect.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.08f), IGraphics::CORNER_ALL, 10.0f);
}

inline void DrawFullOverlay(CGameClient *pGameClient, IGraphics *pGraphics)
{
	if(!s_FullOpen || !g_Config.m_BkwMiniMap || !pGameClient || !pGraphics)
		return;

	CUi *pUi = pGameClient->Ui();
	const CScreenRect OldScreen = pGraphics->GetScreen();
	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	const CUIRect MapRect = FullMapRect(Screen);
	const float Alpha = std::clamp(g_Config.m_BkwMiniMapOpacity / 100.0f, 0.20f, 1.0f);

	Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.88f), IGraphics::CORNER_NONE, 0.0f);
	if(g_Config.m_BkwMiniMapRealDesign)
		DrawRealDesignFullMap(pGameClient, pGraphics, pUi, Screen, MapRect, Alpha);
	else
		DrawSchematicFullMap(pGameClient, pGraphics, pUi, MapRect, s_FullCenter, s_FullZoom, Alpha);

	pUi->MapScreen();
	char aHint[300];
	str_format(aHint, sizeof(aHint), "BKW Full Map • ЛКМ + движение: перемещение • колесо: масштаб %d%% • Esc/клавиша карты: закрыть%s", (int)(s_FullZoom * 100.0f + 0.5f), g_Config.m_BkwMiniMapRealDesign ? " • реальные слои карты" : "");
	CUIRect Hint{Screen.x + 16.0f, Screen.y + 5.0f, Screen.w - 32.0f, 18.0f};
	SLabelProperties HintProps;
	HintProps.m_MaxWidth = Hint.w;
	HintProps.m_EllipsisAtEnd = true;
	pUi->DoLabel(&Hint, aHint, 11.0f, TEXTALIGN_MC, HintProps);
	DrawMenuCursor(pGameClient, pGraphics);
	pGraphics->MapScreen(OldScreen);
}

inline void Update(CGameClient *pGameClient, IClient *pClient, IGraphics *pGraphics, IInput *pInput)
{
	if(!g_Config.m_BkwMiniMap || !pGameClient || !pClient || !pGraphics || !pInput || pClient->State() != IClient::STATE_ONLINE)
	{
		CloseFull(pGameClient, pInput);
		return;
	}
	if(!EnsureCache(pGameClient->Collision(), pClient))
		return;

	const int Key = g_Config.m_BkwMiniMapFullKey;
	const bool TogglePressed = Key > KEY_UNKNOWN && Key < KEY_LAST && !pGameClient->m_Menus.IsActive() && !pGameClient->m_Chat.IsActive() && !pGameClient->m_GameConsole.IsActive() && pInput->KeyPress(Key) && CBinds::GetModifierMask(pInput) == g_Config.m_BkwMiniMapFullModifier;
	if(TogglePressed)
	{
		if(s_FullOpen)
			CloseFull(pGameClient, pInput);
		else
			OpenFull(pGameClient, pInput, pGraphics);
	}

	if(s_FullOpen)
	{
		const CUIRect Screen = *pGameClient->Ui()->Screen();
		HandleFullMapInput(pGameClient, pInput, FullMapRect(Screen));
		if(s_FullOpen && !s_LateOverlayHealthy)
			DrawFullOverlay(pGameClient, pGraphics);
		return;
	}

	if(!pGameClient->m_Menus.IsActive())
		DrawSmallMap(pGameClient, pGraphics);
}

inline void RenderLateOverlay(CGameClient *pGameClient, IGraphics *pGraphics)
{
	if(!s_FullOpen || !g_Config.m_BkwMiniMap || !pGameClient || !pGraphics)
		return;
	s_LateOverlayHealthy = true;
	DrawFullOverlay(pGameClient, pGraphics);
}

} // namespace BkwMiniMap

#endif // GAME_CLIENT_COMPONENTS_BKW_MINIMAP_INC
