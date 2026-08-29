/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "maplayers.h"

#include <game/client/gameclient.h>
#include <game/localization.h>

#include <algorithm>

CMapLayers::CMapLayers(ERenderType Type, bool OnlineOnly)
{
	m_Type = Type;
	m_OnlineOnly = OnlineOnly;

	// static parameters for ingame rendering
	m_Params.m_RenderType = m_Type;
	m_Params.m_RenderInvalidTiles = false;
	m_Params.m_TileAndQuadBuffering = true;
	m_Params.m_RenderTileBorder = true;
	m_Params.m_FpsFogEnabled = false;
	m_Params.m_FpsFogMode = 0;
	m_Params.m_FpsFogRadiusTiles = 0;
	m_Params.m_FpsFogZoomPercent = 0;
	m_Params.m_FpsFogCullMapTiles = false;
}

void CMapLayers::OnInit()
{
	m_pLayers = Layers();
	m_pImages = &GameClient()->m_MapImages;
	m_MapRenderer.Clear();
}

CCamera *CMapLayers::GetCurCamera()
{
	return &GameClient()->m_Camera;
}

void CMapLayers::OnMapLoad()
{
	FRenderUploadCallback FRenderCallback = [&](const char *pTitle, const char *pMessage, int IncreaseCounter) { GameClient()->m_Menus.RenderLoading(pTitle, pMessage, IncreaseCounter); };
	auto FRenderCallbackOptional = std::make_optional<FRenderUploadCallback>(FRenderCallback);

	const char *pLoadingTitle = Localize("Loading map");
	const char *pLoadingMessage = Localize("Uploading map data to GPU");
	GameClient()->m_Menus.RenderLoading(pLoadingTitle, pLoadingMessage, 0);

	// can't do that in CMapLayers::OnInit, because some of this interfaces are not available yet
	m_MapRenderer.OnInit(Graphics(), TextRender(), RenderMap());

	m_EnvEvaluator = CEnvelopeState(m_pLayers->Map(), m_OnlineOnly);
	m_EnvEvaluator.OnInterfacesInit(GameClient());
	m_MapRenderer.Load(m_Type, m_pLayers, m_pImages, &m_EnvEvaluator, FRenderCallbackOptional);
}

void CMapLayers::OnRender()
{
	if(m_OnlineOnly && Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	// dynamic parameters for ingame rendering
	m_Params.m_EntityOverlayVal = m_Type == RENDERTYPE_FULL_DESIGN ? 0 : g_Config.m_ClOverlayEntities;
	m_Params.m_Center = GetCurCamera()->m_Center;
	m_Params.m_Zoom = GetCurCamera()->m_Zoom;
	m_Params.m_RenderText = g_Config.m_ClTextEntities;
	m_Params.m_DebugRenderGroupClips = g_Config.m_DbgRenderGroupClips;
	m_Params.m_DebugRenderQuadClips = g_Config.m_DbgRenderQuadClips;
	m_Params.m_DebugRenderClusterClips = g_Config.m_DbgRenderClusterClips;
	m_Params.m_DebugRenderTileClips = g_Config.m_DbgRenderTileClips;

	const bool OptimizerFpsFogEnabled = GameClient()->OptimizerFpsFogEnabled();
	m_Params.m_FpsFogEnabled = OptimizerFpsFogEnabled;
	m_Params.m_FpsFogMode = g_Config.m_BcOptimizerFpsFogMode;
	m_Params.m_FpsFogRadiusTiles = g_Config.m_BcOptimizerFpsFogRadiusTiles;
	m_Params.m_FpsFogZoomPercent = g_Config.m_BcOptimizerFpsFogZoomPercent;
	m_Params.m_FpsFogCullMapTiles = OptimizerFpsFogEnabled && g_Config.m_BcOptimizerFpsFogCullMapTiles != 0;

	m_MapRenderer.Render(m_Params);
}

void CMapLayers::RenderBkwPreview(vec2 Center, float Zoom)
{
	if(m_OnlineOnly && Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	CRenderLayerParams Params{};
	Params.m_RenderType = m_Type;
	Params.m_EntityOverlayVal = 0;
	Params.m_Center = Center;
	Params.m_Zoom = std::max(Zoom, 0.01f);
	Params.m_RenderText = false;
	Params.m_RenderInvalidTiles = false;
	Params.m_TileAndQuadBuffering = true;
	Params.m_RenderTileBorder = false;
	Params.m_DebugRenderGroupClips = false;
	Params.m_DebugRenderQuadClips = false;
	Params.m_DebugRenderClusterClips = false;
	Params.m_DebugRenderTileClips = false;
	Params.m_FpsFogEnabled = false;
	Params.m_FpsFogMode = 0;
	Params.m_FpsFogRadiusTiles = 0;
	Params.m_FpsFogZoomPercent = 0;
	Params.m_FpsFogCullMapTiles = false;

	// BKW previews may use an external pixel clip (the circular minimap is built
	// from horizontal clipped strips). Normal map groups call ClipDisable() when
	// gfx_noclip is disabled, so temporarily force the no-group-clip path while
	// keeping the caller's external clip intact. The gameplay setting is restored
	// immediately after rendering and normal map rendering is unaffected.
	const int OldNoClip = g_Config.m_GfxNoclip;
	g_Config.m_GfxNoclip = 1;
	m_MapRenderer.Render(Params);
	g_Config.m_GfxNoclip = OldNoClip;
}