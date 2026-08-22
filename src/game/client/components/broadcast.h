/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_BROADCAST_H
#define GAME_CLIENT_COMPONENTS_BROADCAST_H

#include <engine/textrender.h>

#include <game/client/component.h>

class CBroadcast : public CComponent
{
	struct STeeFusionDuelPlayer
	{
		char m_aName[64] = "";
		int m_Score = 0;
		float m_LastUpdate = 0.0f;
		float m_ScoreChangedAt = 0.0f;
		int m_LastSeenCycle = -1;
	};

	// broadcasts
	char m_aBroadcastText[1024];
	int m_BroadcastTick;
	float m_BroadcastRenderOffset;
	STextContainerIndex m_TextContainerIndex;

	// TeeFusion duel HUD
	bool m_TeeFusionDuelActive = false;
	float m_TeeFusionDuelLastUpdate = 0.0f;
	int m_TeeFusionDuelCycle = 0;
	int m_TeeFusionDuelPlayerCount = 0;
	STeeFusionDuelPlayer m_aTeeFusionDuelPlayers[2];

	void RenderServerBroadcast();
	void RenderTeeFusionDuelHud();
	void ResetTeeFusionDuel();
	bool IsTeeFusionServer() const;
	bool HandleTeeFusionDuelBroadcast(const char *pText);
	void UpdateTeeFusionDuelPlayer(const char *pName, int Score);

public:
	int Sizeof() const override { return sizeof(*this); }
	void OnReset() override;
	void OnWindowResize() override;
	void OnRender() override;
	void OnMessage(int MsgType, void *pRawMsg) override;

	void DoBroadcast(const char *pText);
};

#endif
