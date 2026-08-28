#ifndef GAME_CLIENT_COMPONENTS_BKW_MINIMAP_INC
#define GAME_CLIENT_COMPONENTS_BKW_MINIMAP_INC

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

inline constexpr int MAX_GRID_DIMENSION = 256;
inline constexpr float TILE_SIZE = 32.0f;
inline int s_MapWidth = 0;
inline int s_MapHeight = 0;
inline int s_GridWidth = 0;
inline int s_GridHeight = 0;
inline char s_aMapName[128] = "";
inline std::vector<uint8_t> s_vCells;
inline vec2 s_ContentMin(0.0f, 0.0f);
inline vec2 s_ContentMax(0.0f, 0.0f);
inline bool s_FullOpen = false;
inline bool s_Dragging = false;
inline bool s_LateOverlayHealthy = false;
inline vec2 s_FullCenter(0.0f, 0.0f);
inline float s_FullZoom = 1.0f;
inline vec2 s_CursorUiPos(0.0f, 0.0f);
inline vec2 s_LastCursorUiPos(0.0f, 0.0f);

inline int CellPriority(ECellType Type)
{
	switch(Type)
	{
	case CELL_FINISH: return 8;
	case CELL_START: return 7;
	case CELL_FREEZE: return 6;
	case CELL_DEATH: return 5;
	case CELL_TELE: return 4;
	case CELL_SPEEDUP: return 3;
	case CELL_SOLID: return 2;
	default: return 0;
	}
}

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
	s_GridWidth = 0;
	s_GridHeight = 0;
	s_aMapName[0] = '\0';
	s_vCells.clear();
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
	if(!s_vCells.empty() && s_MapWidth == Width && s_MapHeight == Height && str_comp(s_aMapName, pMapName) == 0)
		return true;

	s_MapWidth = Width;
	s_MapHeight = Height;
	str_copy(s_aMapName, pMapName);
	s_GridWidth = std::max(1, std::min(Width, MAX_GRID_DIMENSION));
	s_GridHeight = std::max(1, std::min(Height, MAX_GRID_DIMENSION));
	s_vCells.assign((size_t)s_GridWidth * (size_t)s_GridHeight, CELL_AIR);

	int MinGx = s_GridWidth;
	int MinGy = s_GridHeight;
	int MaxGx = -1;
	int MaxGy = -1;
	int AllMinGx = s_GridWidth;
	int AllMinGy = s_GridHeight;
	int AllMaxGx = -1;
	int AllMaxGy = -1;

	for(int Gy = 0; Gy < s_GridHeight; ++Gy)
	{
		const int Y0 = Gy * Height / s_GridHeight;
		const int Y1 = std::max(Y0 + 1, (Gy + 1) * Height / s_GridHeight);
		for(int Gx = 0; Gx < s_GridWidth; ++Gx)
		{
			const int X0 = Gx * Width / s_GridWidth;
			const int X1 = std::max(X0 + 1, (Gx + 1) * Width / s_GridWidth);
			ECellType Best = CELL_AIR;
			int BestPriority = 0;
			for(int Y = Y0; Y < Y1 && BestPriority < 8; ++Y)
			{
				for(int X = X0; X < X1 && BestPriority < 8; ++X)
				{
					const int Index = Y * Width + X;
					const ECellType Type = ClassifyTile(pCollision, Index);
					const int Priority = CellPriority(Type);
					if(Priority > BestPriority)
					{
						Best = Type;
						BestPriority = Priority;
					}
				}
			}
			s_vCells[(size_t)Gy * (size_t)s_GridWidth + (size_t)Gx] = Best;
			if(Best == CELL_AIR)
				continue;

			AllMinGx = std::min(AllMinGx, Gx);
			AllMinGy = std::min(AllMinGy, Gy);
			AllMaxGx = std::max(AllMaxGx, Gx);
			AllMaxGy = std::max(AllMaxGy, Gy);

			// Many DDNet maps have a pure solid/nohook safety border around the entire
			// map file. It is not useful minimap content and used to create enormous
			// empty zones. Ignore only that outer border when calculating auto-fit.
			const bool OuterBorderSolid = Best == CELL_SOLID &&
				(Gx <= 1 || Gy <= 1 || Gx >= s_GridWidth - 2 || Gy >= s_GridHeight - 2);
			if(!OuterBorderSolid)
			{
				MinGx = std::min(MinGx, Gx);
				MinGy = std::min(MinGy, Gy);
				MaxGx = std::max(MaxGx, Gx);
				MaxGy = std::max(MaxGy, Gy);
			}
		}
	}

	if(MaxGx < MinGx || MaxGy < MinGy)
	{
		MinGx = AllMinGx;
		MinGy = AllMinGy;
		MaxGx = AllMaxGx;
		MaxGy = AllMaxGy;
	}

	const float WorldW = Width * TILE_SIZE;
	const float WorldH = Height * TILE_SIZE;
	if(MaxGx >= MinGx && MaxGy >= MinGy)
	{
		s_ContentMin.x = (float)MinGx * WorldW / s_GridWidth;
		s_ContentMin.y = (float)MinGy * WorldH / s_GridHeight;
		s_ContentMax.x = (float)(MaxGx + 1) * WorldW / s_GridWidth;
		s_ContentMax.y = (float)(MaxGy + 1) * WorldH / s_GridHeight;

		const float Padding = std::max(TILE_SIZE, std::max(s_ContentMax.x - s_ContentMin.x, s_ContentMax.y - s_ContentMin.y) * 0.0125f);
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

inline float ContentWidth()
{
	return std::max(TILE_SIZE, s_ContentMax.x - s_ContentMin.x);
}

inline float ContentHeight()
{
	return std::max(TILE_SIZE, s_ContentMax.y - s_ContentMin.y);
}

inline vec2 ContentCenter()
{
	return (s_ContentMin + s_ContentMax) * 0.5f;
}

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
		Center.x = (s_ContentMin.x + s_ContentMax.x) * 0.5f;
	else
		Center.x = std::clamp(Center.x, s_ContentMin.x + HalfW, s_ContentMax.x - HalfW);
	if(HalfH * 2.0f >= ContentHeight())
		Center.y = (s_ContentMin.y + s_ContentMax.y) * 0.5f;
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

inline vec2 RectCenter(const CUIRect &Rect)
{
	return vec2(Rect.x + Rect.w * 0.5f, Rect.y + Rect.h * 0.5f);
}

inline float CircleRadius(const CUIRect &Rect)
{
	return std::min(Rect.w, Rect.h) * 0.5f;
}

inline bool PointInsideCircle(const CUIRect &Rect, vec2 Point, float Margin = 0.0f)
{
	return distance(RectCenter(Rect), Point) <= std::max(0.0f, CircleRadius(Rect) - Margin);
}

inline void RenderMapCells(IGraphics *pGraphics, const CUIRect &Rect, vec2 Center, float Scale, float Alpha, bool CircularClip = false)
{
	std::array<std::vector<IGraphics::CQuadItem>, CELL_COUNT> avQuads;
	const vec2 ClipCenter = RectCenter(Rect);
	const float ClipRadius = CircleRadius(Rect);
	for(int Gy = 0; Gy < s_GridHeight; ++Gy)
	{
		const float WorldY0 = (float)Gy * (s_MapHeight * TILE_SIZE) / s_GridHeight;
		const float WorldY1 = (float)(Gy + 1) * (s_MapHeight * TILE_SIZE) / s_GridHeight;
		for(int Gx = 0; Gx < s_GridWidth; ++Gx)
		{
			const ECellType Type = (ECellType)s_vCells[(size_t)Gy * (size_t)s_GridWidth + (size_t)Gx];
			if(Type == CELL_AIR)
				continue;
			const float WorldX0 = (float)Gx * (s_MapWidth * TILE_SIZE) / s_GridWidth;
			const float WorldX1 = (float)(Gx + 1) * (s_MapWidth * TILE_SIZE) / s_GridWidth;
			const vec2 P0 = WorldToMapScreen(vec2(WorldX0, WorldY0), Rect, Center, Scale);
			const vec2 P1 = WorldToMapScreen(vec2(WorldX1, WorldY1), Rect, Center, Scale);
			if(P1.x < Rect.x || P0.x > Rect.x + Rect.w || P1.y < Rect.y || P0.y > Rect.y + Rect.h)
				continue;

			if(CircularClip)
			{
				const vec2 CellCenter = (P0 + P1) * 0.5f;
				const float HalfDiagonal = length(P1 - P0) * 0.5f;
				if(distance(CellCenter, ClipCenter) + HalfDiagonal > ClipRadius)
					continue;
			}

			const float X = std::max(P0.x, Rect.x);
			const float Y = std::max(P0.y, Rect.y);
			const float W = std::min(P1.x, Rect.x + Rect.w) - X;
			const float H = std::min(P1.y, Rect.y + Rect.h) - Y;
			if(W > 0.0f && H > 0.0f)
				avQuads[Type].emplace_back(X, Y, std::max(0.7f, W), std::max(0.7f, H));
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
		const vec2 Pos = pGameClient->m_aClients[ClientId].m_RenderPos;
		const vec2 ScreenPos = WorldToMapScreen(Pos, Rect, Center, Scale);
		const bool Frozen = pGameClient->m_aClients[ClientId].m_Predicted.m_IsInFreeze || pGameClient->m_aClients[ClientId].m_DeepFrozen;
		ColorRGBA Color = ClientId == LocalId ? ColorRGBA(0.30f, 1.00f, 0.42f, Alpha) : ColorRGBA(1.0f, 1.0f, 1.0f, Alpha);
		if(Frozen)
			Color = ColorRGBA(0.35f, 0.82f, 1.0f, Alpha);
		const float Radius = ShowNames ? 3.8f : 2.8f;

		if(CircularClip)
		{
			if(!PointInsideCircle(Rect, ScreenPos, Radius))
				continue;
		}
		else if(!PointInside(Rect, ScreenPos))
		{
			continue;
		}

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
	constexpr int Segments = 72;
	std::array<IGraphics::CLineItem, Segments> aLines;
	for(int i = 0; i < Segments; ++i)
	{
		const float A0 = 2.0f * pi * i / Segments;
		const float A1 = 2.0f * pi * (i + 1) / Segments;
		aLines[i] = IGraphics::CLineItem(
			Center.x + std::cos(A0) * Radius,
			Center.y + std::sin(A0) * Radius,
			Center.x + std::cos(A1) * Radius,
			Center.y + std::sin(A1) * Radius);
	}
	pGraphics->TextureClear();
	pGraphics->LinesBegin();
	pGraphics->SetColor(Color);
	pGraphics->LinesDraw(aLines.data(), aLines.size());
	pGraphics->LinesEnd();
}

inline void DrawMap(CGameClient *pGameClient, IGraphics *pGraphics, CUi *pUi, CUIRect Rect, vec2 Center, float Zoom, float Alpha, bool ShowNames)
{
	Rect.Draw(ColorRGBA(0.018f, 0.022f, 0.032f, 0.94f * Alpha), IGraphics::CORNER_ALL, ShowNames ? 10.0f : 7.0f);
	CUIRect Inner = Rect;
	Inner.Margin(5.0f, &Inner);
	const float Scale = FitScaleToContent(Inner) * std::max(1.0f, Zoom);
	Center = ClampCenterToContent(Center, Inner, Scale);
	RenderMapCells(pGraphics, Inner, Center, Scale, Alpha);
	RenderPlayers(pGameClient, pGraphics, pUi, Inner, Center, Scale, Alpha, ShowNames);
}

inline void DrawCircularMiniMap(CGameClient *pGameClient, IGraphics *pGraphics, CUi *pUi, CUIRect Rect, float Alpha)
{
	const vec2 CircleCenter = RectCenter(Rect);
	const float Radius = CircleRadius(Rect);

	pGraphics->TextureClear();
	pGraphics->QuadsBegin();
	pGraphics->SetColor(ColorRGBA(0.018f, 0.022f, 0.032f, 0.82f * Alpha));
	pGraphics->DrawCircle(CircleCenter.x, CircleCenter.y, Radius, 72);
	pGraphics->QuadsEnd();

	CUIRect Inner = Rect;
	Inner.Margin(5.0f, &Inner);
	const float Scale = CoverScaleToContent(Inner) * 1.04f;
	vec2 Center = ContentCenter();
	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	if(LocalId >= 0 && LocalId < MAX_CLIENTS && pGameClient->m_aClients[LocalId].m_Active)
		Center = pGameClient->m_aClients[LocalId].m_RenderPos;
	Center = ClampCenterToContent(Center, Inner, Scale);

	RenderMapCells(pGraphics, Inner, Center, Scale, Alpha, true);
	RenderPlayers(pGameClient, pGraphics, pUi, Inner, Center, Scale, Alpha, false, true);
	DrawCircleOutline(pGraphics, CircleCenter, Radius - 1.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.72f * Alpha));
}

inline CUIRect FullMapRect(const CUIRect &Screen)
{
	CUIRect Available = Screen;
	Available.Margin(28.0f, &Available);
	const float ContentAspect = ContentWidth() / ContentHeight();
	const float AvailableAspect = Available.w / std::max(1.0f, Available.h);
	CUIRect Result = Available;
	if(AvailableAspect > ContentAspect)
	{
		Result.w = Result.h * ContentAspect;
		Result.x = Available.x + (Available.w - Result.w) * 0.5f;
	}
	else
	{
		Result.h = Result.w / std::max(0.01f, ContentAspect);
		Result.y = Available.y + (Available.h - Result.h) * 0.5f;
	}
	return Result;
}

inline CUIRect MiniMapRect(const CUIRect &Screen)
{
	const float Size = (float)std::clamp(g_Config.m_BkwMiniMapSize, 120, 300);
	return CUIRect{Screen.x + Screen.w - Size - 14.0f, Screen.y + 14.0f, Size, Size};
}

inline bool IsFullOpen()
{
	return s_FullOpen;
}

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

	const CUIRect Inner = [&]() {
		CUIRect R = MapRect;
		R.Margin(5.0f, &R);
		return R;
	}();
	const float OldScale = FitScaleToContent(Inner) * OldZoom;
	const float NewScale = FitScaleToContent(Inner) * NewZoom;
	const vec2 MapScreenCenter = RectCenter(Inner);
	const vec2 CursorOffset = s_CursorUiPos - MapScreenCenter;
	const vec2 WorldUnderCursor = s_FullCenter + CursorOffset / OldScale;
	s_FullCenter = WorldUnderCursor - CursorOffset / NewScale;
	s_FullZoom = NewZoom;
	s_FullCenter = ClampCenterToContent(s_FullCenter, Inner, NewScale);
}

inline void HandleFullMapInput(CGameClient *pGameClient, IInput *pInput, IGraphics *pGraphics, CUi *pUi, const CUIRect &MapRect)
{
	(void)pGraphics;
	(void)pUi;
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
	const float Scale = FitScaleToContent(Inner) * s_FullZoom;
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
		CUIRect Cursor{s_CursorUiPos.x, s_CursorUiPos.y, 7.0f, 7.0f};
		Cursor.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f), IGraphics::CORNER_ALL, 3.5f);
	}
}

inline void DrawSmallMap(CGameClient *pGameClient, IGraphics *pGraphics)
{
	CUi *pUi = pGameClient->Ui();
	const CScreenRect OldScreen = pGraphics->GetScreen();
	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	const float Alpha = std::clamp(g_Config.m_BkwMiniMapOpacity / 100.0f, 0.20f, 1.0f);
	const CUIRect Mini = MiniMapRect(Screen);
	DrawCircularMiniMap(pGameClient, pGraphics, pUi, Mini, Alpha);
	pGraphics->MapScreen(OldScreen);
}

inline void DrawFullOverlay(CGameClient *pGameClient, IGraphics *pGraphics)
{
	if(!s_FullOpen || !g_Config.m_BkwMiniMap || !pGameClient || !pGraphics)
		return;

	CUi *pUi = pGameClient->Ui();
	const CScreenRect OldScreen = pGraphics->GetScreen();
	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	const float Alpha = std::clamp(g_Config.m_BkwMiniMapOpacity / 100.0f, 0.20f, 1.0f);

	CUIRect Shade = Screen;
	Shade.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.86f), IGraphics::CORNER_NONE, 0.0f);
	const CUIRect MapRect = FullMapRect(Screen);
	DrawMap(pGameClient, pGraphics, pUi, MapRect, s_FullCenter, s_FullZoom, Alpha, true);

	char aHint[256];
	str_format(aHint, sizeof(aHint), "BKW Full Map • ЛКМ + движение: перемещение • колесо: масштаб %d%% • Esc/клавиша карты: закрыть", (int)(s_FullZoom * 100.0f + 0.5f));
	CUIRect Hint{Screen.x + 16.0f, Screen.y + 7.0f, Screen.w - 32.0f, 18.0f};
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
		CUi *pUi = pGameClient->Ui();
		const CUIRect Screen = *pUi->Screen();
		HandleFullMapInput(pGameClient, pInput, pGraphics, pUi, FullMapRect(Screen));
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
