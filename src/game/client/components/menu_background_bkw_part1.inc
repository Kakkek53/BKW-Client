#include "menu_background.h"

#include <base/dbg.h>
#include <base/log.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/graphics.h>
#include <engine/map.h>
#include <engine/shared/config.h>

#include <game/client/components/bkw/tf_menu_overlay.inc>
#include <game/client/components/bkw/ddnet_vote_overlay.inc>
#include <game/client/components/camera.h>
#include <game/client/components/mapimages.h>
#include <game/client/components/maplayers.h>
#include <game/client/gameclient.h>
#include <game/layers.h>
#include <game/localization.h>
#include <game/mapitems.h>

#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

std::array<vec2, CMenuBackground::NUM_POS> GenerateMenuBackgroundPositions()
{
	std::array<vec2, CMenuBackground::NUM_POS> Positions;

	Positions[CMenuBackground::POS_START] = vec2(500.0f, 500.0f);
	Positions[CMenuBackground::POS_BROWSER_INTERNET] = vec2(1000.0f, 1000.0f);
	Positions[CMenuBackground::POS_BROWSER_LAN] = vec2(1100.0f, 1000.0f);
	Positions[CMenuBackground::POS_DEMOS] = vec2(900.0f, 100.0f);
	Positions[CMenuBackground::POS_NEWS] = vec2(500.0f, 750.0f);
	Positions[CMenuBackground::POS_BROWSER_FAVORITES] = vec2(1250.0f, 500.0f);
	Positions[CMenuBackground::POS_SETTINGS_LANGUAGE] = vec2(500.0f, 1200.0f);
	Positions[CMenuBackground::POS_SETTINGS_GENERAL] = vec2(500.0f, 1000.0f);
	Positions[CMenuBackground::POS_SETTINGS_PLAYER] = vec2(600.0f, 1000.0f);
	Positions[CMenuBackground::POS_SETTINGS_TEE] = vec2(700.0f, 1000.0f);
	Positions[CMenuBackground::POS_SETTINGS_APPEARANCE] = vec2(200.0f, 1000.0f);
	Positions[CMenuBackground::POS_SETTINGS_CONTROLS] = vec2(800.0f, 1000.0f);
	Positions[CMenuBackground::POS_SETTINGS_GRAPHICS] = vec2(900.0f, 1000.0f);
	Positions[CMenuBackground::POS_SETTINGS_SOUND] = vec2(1000.0f, 1000.0f);
	Positions[CMenuBackground::POS_SETTINGS_DDNET] = vec2(1200.0f, 200.0f);
	Positions[CMenuBackground::POS_SETTINGS_ASSETS] = vec2(500.0f, 500.0f);
	Positions[CMenuBackground::POS_SETTINGS_CREDITS] = vec2(1100.0f, 1000.0f);
	for(int i = 0; i < CMenuBackground::POS_BROWSER_CUSTOM_NUM; ++i)
		Positions[CMenuBackground::POS_BROWSER_CUSTOM0 + i] = vec2(500.0f + (75.0f * (float)i), 650.0f - (75.0f * (float)i));
	for(int i = 0; i < CMenuBackground::POS_SETTINGS_RESERVED_NUM; ++i)
		Positions[CMenuBackground::POS_SETTINGS_RESERVED0 + i] = vec2(0, 0);
	for(int i = 0; i < CMenuBackground::POS_RESERVED_NUM; ++i)
		Positions[CMenuBackground::POS_RESERVED0 + i] = vec2(0, 0);

	return Positions;
}

CMenuBackground::CMenuBackground() :
	CBackground(ERenderType::RENDERTYPE_FULL_DESIGN, false)
{
	m_RotationCenter = vec2(0.0f, 0.0f);
	m_AnimationStartPos = vec2(0.0f, 0.0f);
	m_Camera.m_Center = vec2(0.0f, 0.0f);
	m_Camera.m_PrevCenter = vec2(0.0f, 0.0f); // unused in this class
	m_ChangedPosition = false;

	ResetPositions();

	m_CurrentPosition = -1;
	m_MoveTime = 0.0f;

	m_IsInit = false;
	m_Loading = false;
}

void CMenuBackground::OnInterfacesInit(CGameClient *pClient)
{
	CComponentInterfaces::OnInterfacesInit(pClient);
	m_pImages->OnInterfacesInit(pClient);
	m_Camera.OnInterfacesInit(pClient);
}

void CMenuBackground::OnInit()
{
	m_pBackgroundMap = CreateMap();
	m_pMap = m_pBackgroundMap.get();

	m_IsInit = true;
	m_BkwMediaBackground.Init(Graphics(), Storage());
	m_BkwMediaBackground.SyncFromConfig();

	if(g_Config.m_ClMenuMap[0] != '\0')
		LoadMenuBackground();

	m_Camera.m_ZoomSet = false;
	m_Camera.m_ZoomSmoothingTarget = 0;
}

void CMenuBackground::ResetPositions()
{
	m_aPositions = GenerateMenuBackgroundPositions();
}

void CMenuBackground::LoadThemeIcon(CTheme &Theme)
{
	char aIconPath[IO_MAX_PATH_LENGTH];
	str_format(aIconPath, sizeof(aIconPath), "themes/%s.png", Theme.m_Name.empty() ? "none" : Theme.m_Name.c_str());
	Theme.m_IconTexture = Graphics()->LoadTexture(aIconPath, IStorage::TYPE_ALL);
	if(Theme.m_IconTexture.IsNullTexture())
	{
		log_error("menuthemes", "failed to load theme icon '%s'", aIconPath);
	}
	else
	{
		log_trace("menuthemes", "loaded theme icon '%s'", aIconPath);
	}
}

int CMenuBackground::ThemeScan(const char *pName, int IsDir, int DirType, void *pUser)
{