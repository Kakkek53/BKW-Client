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
inline int s_MapWidth = 0;
inline int s_MapHeight = 0;
inline int s_GridWidth = 0;
inline int s_GridHeight = 0;
inline char s_aMapName[128] = "";
inline std::vector<uint8_t> s_vCells;
inline bool s_FullOpen = false;
inline vec2 s_FullCenter(0.0f, 0.0f);

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
	s_FullOpen = false;
	s_FullCenter = vec2(0.0f, 0.0f);
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
		}
	}

	s_FullOpen = false;
	s_FullCenter = vec2(Width * 16.0f, Height * 16.0f);
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

inline float FitScale(const CUIRect &Rect)
{
	if(s_MapWidth <= 0 || s_MapHeight <= 0)
		return 1.0f;
	const float WorldW = s_MapWidth * 32.0f;
	const float WorldH = s_MapHeight * 32.0f;
	return std::max(0.0001f, std::min(Rect.w / WorldW, Rect.h / WorldH));
}

inline vec2 ClampCenter(vec2 Center, const CUIRect &Rect, float Scale)
{
	const float WorldW = s_MapWidth * 32.0f;
	const float WorldH = s_MapHeight * 32.0f;
	const float HalfW = Rect.w / (2.0f * Scale);
	const float HalfH = Rect.h / (2.0f * Scale);
	if(HalfW * 2.0f >= WorldW)
		Center.x = WorldW * 0.5f;
	else
		Center.x = std::clamp(Center.x, HalfW, WorldW - HalfW);
	if(HalfH * 2.0f >= WorldH)
		Center.y = WorldH * 0.5f;
	else
		Center.y = std::clamp(Center.y, HalfH, WorldH - HalfH);
	return Center;
}

inline vec2 WorldToMapScreen(vec2 World, const CUIRect &Rect, vec2 Center, float Scale)
{
	return vec2(Rect.x + Rect.w * 0.5f + (World.x - Center.x) * Scale,
		Rect.y + Rect.h * 0.5f + (World.y - Center.y) * Scale);
}

inline void RenderMapCells(IGraphics *pGraphics, const CUIRect &Rect, vec2 Center, float Scale, float Alpha)
{
	std::array<std::vector<IGraphics::CQuadItem>, CELL_COUNT> avQuads;
	for(int Gy = 0; Gy < s_GridHeight; ++Gy)
	{
		const float WorldY0 = (float)Gy * (s_MapHeight * 32.0f) / s_GridHeight;
		const float WorldY1 = (float)(Gy + 1) * (s_MapHeight * 32.0f) / s_GridHeight;
		for(int Gx = 0; Gx < s_GridWidth; ++Gx)
		{
			const ECellType Type = (ECellType)s_vCells[(size_t)Gy * (size_t)s_GridWidth + (size_t)Gx];
			if(Type == CELL_AIR)
				continue;
			const float WorldX0 = (float)Gx * (s_MapWidth * 32.0f) / s_GridWidth;
			const float WorldX1 = (float)(Gx + 1) * (s_MapWidth * 32.0f) / s_GridWidth;
			const vec2 P0 = WorldToMapScreen(vec2(WorldX0, WorldY0), Rect, Center, Scale);
			const vec2 P1 = WorldToMapScreen(vec2(WorldX1, WorldY1), Rect, Center, Scale);
			if(P1.x < Rect.x || P0.x > Rect.x + Rect.w || P1.y < Rect.y || P0.y > Rect.y + Rect.h)
				continue;
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

inline void RenderPlayers(CGameClient *pGameClient, IGraphics *pGraphics, CUi *pUi, const CUIRect &Rect, vec2 Center, float Scale, float Alpha, bool ShowNames)
{
	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ++ClientId)
	{
		if(!pGameClient->m_aClients[ClientId].m_Active)
			continue;
		const vec2 Pos = pGameClient->m_aClients[ClientId].m_RenderPos;
		const vec2 ScreenPos = WorldToMapScreen(Pos, Rect, Center, Scale);
		if(ScreenPos.x < Rect.x || ScreenPos.x > Rect.x + Rect.w || ScreenPos.y < Rect.y || ScreenPos.y > Rect.y + Rect.h)
			continue;
		const bool Frozen = pGameClient->m_aClients[ClientId].m_Predicted.m_IsInFreeze || pGameClient->m_aClients[ClientId].m_DeepFrozen;
		ColorRGBA Color = ClientId == LocalId ? ColorRGBA(0.30f, 1.00f, 0.42f, Alpha) : ColorRGBA(1.0f, 1.0f, 1.0f, Alpha);
		if(Frozen)
			Color = ColorRGBA(0.35f, 0.82f, 1.0f, Alpha);
		const float Radius = ShowNames ? 3.8f : 2.8f;
		pGraphics->SetColor(Color);
		pGraphics->DrawCircle(ScreenPos.x, ScreenPos.y, Radius, 16);
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

inline void DrawMap(CGameClient *pGameClient, IGraphics *pGraphics, CUi *pUi, CUIRect Rect, vec2 Center, float Zoom, float Alpha, bool ShowNames)
{
	Rect.Draw(ColorRGBA(0.018f, 0.022f, 0.032f, 0.90f * Alpha), IGraphics::CORNER_ALL, ShowNames ? 10.0f : 7.0f);
	CUIRect Inner = Rect;
	Inner.Margin(5.0f, &Inner);
	const float Scale = FitScale(Inner) * std::max(1.0f, Zoom);
	Center = ClampCenter(Center, Inner, Scale);
	RenderMapCells(pGraphics, Inner, Center, Scale, Alpha);
	RenderPlayers(pGameClient, pGraphics, pUi, Inner, Center, Scale, Alpha, ShowNames);
}

inline void OpenFull(CGameClient *pGameClient)
{
	s_FullOpen = true;
	const int LocalId = pGameClient ? pGameClient->m_Snap.m_LocalClientId : -1;
	if(pGameClient && LocalId >= 0 && LocalId < MAX_CLIENTS && pGameClient->m_aClients[LocalId].m_Active)
		s_FullCenter = pGameClient->m_aClients[LocalId].m_RenderPos;
	else
		s_FullCenter = vec2(s_MapWidth * 16.0f, s_MapHeight * 16.0f);
}

inline void HandleFullMapInput(CGameClient *pGameClient, IInput *pInput, CUi *pUi, const CUIRect &MapRect)
{
	if(!s_FullOpen)
		return;
	if(pInput->KeyPress(KEY_ESCAPE))
	{
		s_FullOpen = false;
		return;
	}

	const float Scale = FitScale(MapRect) * std::max(1.0f, g_Config.m_BkwMiniMapFullZoom / 100.0f);
	if(pInput->KeyIsPressed(KEY_MOUSE_1))
	{
		const vec2 Delta = pUi->MouseDelta();
		s_FullCenter -= Delta / Scale;
		s_FullCenter = ClampCenter(s_FullCenter, MapRect, Scale);
	}
}

inline void Update(CGameClient *pGameClient, IClient *pClient, IGraphics *pGraphics, IInput *pInput)
{
	if(!g_Config.m_BkwMiniMap || !pGameClient || !pClient || !pGraphics || !pInput || pClient->State() != IClient::STATE_ONLINE)
	{
		s_FullOpen = false;
		return;
	}
	CCollision *pCollision = pGameClient->Collision();
	if(!EnsureCache(pCollision, pClient))
		return;

	const int Key = g_Config.m_BkwMiniMapFullKey;
	if(Key > KEY_UNKNOWN && Key < KEY_LAST && !pGameClient->m_Menus.IsActive() && !pGameClient->m_Chat.IsActive() && !pGameClient->m_GameConsole.IsActive() && pInput->KeyPress(Key) && CBinds::GetModifierMask(pInput) == g_Config.m_BkwMiniMapFullModifier)
	{
		if(s_FullOpen)
			s_FullOpen = false;
		else
			OpenFull(pGameClient);
	}

	if(pGameClient->m_Menus.IsActive())
		return;

	CUi *pUi = pGameClient->Ui();
	const CScreenRect OldScreen = pGraphics->GetScreen();
	pUi->MapScreen();
	const CUIRect Screen = *pUi->Screen();
	const float Alpha = std::clamp(g_Config.m_BkwMiniMapOpacity / 100.0f, 0.20f, 1.0f);

	if(s_FullOpen)
	{
		CUIRect Shade = Screen;
		Shade.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.52f), IGraphics::CORNER_NONE, 0.0f);
		CUIRect MapRect = Screen;
		MapRect.Margin(28.0f, &MapRect);
		HandleFullMapInput(pGameClient, pInput, pUi, MapRect);
		DrawMap(pGameClient, pGraphics, pUi, MapRect, s_FullCenter, g_Config.m_BkwMiniMapFullZoom / 100.0f, Alpha, true);
		CUIRect Hint{MapRect.x + 10.0f, MapRect.y + 8.0f, MapRect.w - 20.0f, 20.0f};
		pUi->DoLabel(&Hint, "BKW Full Map • удерживай ЛКМ и двигай мышь • Esc/клавиша карты — закрыть", 11.0f, TEXTALIGN_MC);
	}
	else
	{
		const float Size = (float)std::clamp(g_Config.m_BkwMiniMapSize, 120, 300);
		CUIRect Mini{Screen.x + Screen.w - Size - 14.0f, Screen.y + 14.0f, Size, Size * 0.68f};
		const vec2 Center(s_MapWidth * 16.0f, s_MapHeight * 16.0f);
		DrawMap(pGameClient, pGraphics, pUi, Mini, Center, 1.0f, Alpha, false);
	}

	pGraphics->MapScreen(OldScreen);
}

} // namespace BkwMiniMap

#endif // GAME_CLIENT_COMPONENTS_BKW_MINIMAP_INC
