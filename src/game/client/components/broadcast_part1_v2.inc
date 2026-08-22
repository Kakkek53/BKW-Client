/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "broadcast.h"

#include <base/color.h>
#include <base/log.h>
#include <base/log_color.h>
#include <base/str.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <generated/protocol.h>

#include <game/client/components/bkw/server_detector.inc>
#include <game/client/components/important_alert.h>
#include <game/client/components/motd.h>
#include <game/client/components/scoreboard.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

#include <algorithm>
#include <cstdlib>
#include <string>

namespace
{
constexpr float BKW_TF_DUEL_TIMEOUT_SECONDS = 3.0f;
constexpr float BKW_TF_DUEL_FADE_SECONDS = 0.65f;
constexpr float BKW_TF_DUEL_SCORE_PULSE_SECONDS = 0.28f;

std::string BkwTrimDuelLine(const char *pText)
{
	if(!pText)
		return {};
	std::string Text(pText);
	const size_t Begin = Text.find_first_not_of(" \t\r");
	if(Begin == std::string::npos)
		return {};
	const size_t End = Text.find_last_not_of(" \t\r");
	return Text.substr(Begin, End - Begin + 1);
}

bool BkwIsTeeFusionDuelTitle(const std::string &Text)
{
	return Text == "Дуэль" || Text == "Дyэль" || str_comp_nocase(Text.c_str(), "Duel") == 0;
}

bool BkwParseTeeFusionDuelScore(const std::string &Text, std::string &Name, int &Score)
{
	const size_t Separator = Text.rfind(" - ");
	if(Separator == std::string::npos || Separator == 0)
		return false;

	Name = Text.substr(0, Separator);
	const size_t NameBegin = Name.find_first_not_of(" \t");
	const size_t NameEnd = Name.find_last_not_of(" \t");
	if(NameBegin == std::string::npos)
		return false;
	Name = Name.substr(NameBegin, NameEnd - NameBegin + 1);
	if(Name.empty())
		return false;

	const std::string ScoreText = Text.substr(Separator + 3);
	if(ScoreText.empty())
		return false;

	char *pEnd = nullptr;
	const long ParsedScore = std::strtol(ScoreText.c_str(), &pEnd, 10);
	while(pEnd && (*pEnd == ' ' || *pEnd == '\t'))
		++pEnd;
	if(!pEnd || *pEnd != '\0')
		return false;

	Score = std::clamp((int)ParsedScore, -9999, 9999);
	return true;
}

ColorRGBA BkwDuelAlpha(ColorRGBA Color, float Alpha)
{
	Color.a *= Alpha;
	return Color;
}

void BkwDuelLabel(CUi *pUi, const CUIRect &Rect, const char *pText, float Size, int Align, ColorRGBA Color)
{
	SLabelProperties Props;
	Props.m_MaxWidth = Rect.w;
	Props.m_EllipsisAtEnd = true;
	Props.SetColor(Color);
	pUi->DoLabel(&Rect, pText, Size, Align, Props);
}

void BkwDuelDrawLiquidGlass(CUIRect Card, float Alpha)
{
	for(int i = 4; i >= 1; --i)
	{
		CUIRect Glow = Card;
		const float Expand = (float)i * 2.0f;
		Glow.x -= Expand;
		Glow.y -= Expand;
		Glow.w += Expand * 2.0f;
		Glow.h += Expand * 2.0f;
		Glow.Draw(BkwDuelAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.025f + i * 0.012f), Alpha), IGraphics::CORNER_ALL, 18.0f + Expand);
	}

	Card.Draw(BkwDuelAlpha(ColorRGBA(0.86f, 0.92f, 1.0f, 0.16f), Alpha), IGraphics::CORNER_ALL, 17.0f);
	CUIRect Inner;
	Card.Margin(1.25f, &Inner);
	Inner.Draw4(
		BkwDuelAlpha(ColorRGBA(0.11f, 0.15f, 0.22f, 0.72f), Alpha),
		BkwDuelAlpha(ColorRGBA(0.15f, 0.19f, 0.28f, 0.64f), Alpha),
		BkwDuelAlpha(ColorRGBA(0.055f, 0.075f, 0.12f, 0.70f), Alpha),
		BkwDuelAlpha(ColorRGBA(0.075f, 0.095f, 0.15f, 0.66f), Alpha),
		IGraphics::CORNER_ALL, 16.0f);

	CUIRect Highlight = Inner;
	Highlight.x += 12.0f;
	Highlight.w -= 24.0f;
	Highlight.y += 1.0f;
	Highlight.h = 1.2f;
	Highlight.Draw(BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.27f), Alpha), IGraphics::CORNER_ALL, 1.0f);
}
} // namespace

void CBroadcast::ResetTeeFusionDuel()
{
	m_TeeFusionDuelActive = false;
	m_TeeFusionDuelLastUpdate = 0.0f;
	m_TeeFusionDuelCycle = 0;
	m_TeeFusionDuelPlayerCount = 0;
	for(auto &Player : m_aTeeFusionDuelPlayers)
		Player = STeeFusionDuelPlayer();
}

bool CBroadcast::IsTeeFusionServer() const
{
	return Client()->State() == IClient::STATE_ONLINE && BkwServerDetector::IsTeeFusion(Client()->ServerInfo());
}

void CBroadcast::UpdateTeeFusionDuelPlayer(const char *pName, int Score)
{
	const float Now = Client()->GlobalTime();
	int PlayerIndex = -1;
	for(int i = 0; i < m_TeeFusionDuelPlayerCount; ++i)
	{
		if(str_comp(m_aTeeFusionDuelPlayers[i].m_aName, pName) == 0)
		{
			PlayerIndex = i;
			break;
		}
	}

	if(PlayerIndex < 0 && m_TeeFusionDuelPlayerCount < 2)
	{
		PlayerIndex = m_TeeFusionDuelPlayerCount++;
		str_copy(m_aTeeFusionDuelPlayers[PlayerIndex].m_aName, pName);
		m_aTeeFusionDuelPlayers[PlayerIndex].m_Score = Score;
		m_aTeeFusionDuelPlayers[PlayerIndex].m_ScoreChangedAt = Now;
	}
	else if(PlayerIndex < 0)
	{
		int Replace = -1;
		for(int i = 0; i < 2; ++i)
		{
			if(m_aTeeFusionDuelPlayers[i].m_LastSeenCycle != m_TeeFusionDuelCycle &&
				(Replace < 0 || m_aTeeFusionDuelPlayers[i].m_LastUpdate < m_aTeeFusionDuelPlayers[Replace].m_LastUpdate))
				Replace = i;
		}
		if(Replace < 0)
			Replace = m_aTeeFusionDuelPlayers[0].m_LastUpdate <= m_aTeeFusionDuelPlayers[1].m_LastUpdate ? 0 : 1;
		PlayerIndex = Replace;
		m_aTeeFusionDuelPlayers[PlayerIndex] = STeeFusionDuelPlayer();
		str_copy(m_aTeeFusionDuelPlayers[PlayerIndex].m_aName, pName);
		m_aTeeFusionDuelPlayers[PlayerIndex].m_Score = Score;
		m_aTeeFusionDuelPlayers[PlayerIndex].m_ScoreChangedAt = Now;
	}
	else if(m_aTeeFusionDuelPlayers[PlayerIndex].m_Score != Score)
	{
		m_aTeeFusionDuelPlayers[PlayerIndex].m_Score = Score;
		m_aTeeFusionDuelPlayers[PlayerIndex].m_ScoreChangedAt = Now;
	}

	auto &Player = m_aTeeFusionDuelPlayers[PlayerIndex];
	Player.m_LastUpdate = Now;
	Player.m_LastSeenCycle = m_TeeFusionDuelCycle;
	m_TeeFusionDuelLastUpdate = Now;
	m_TeeFusionDuelActive = true;
}

bool CBroadcast::HandleTeeFusionDuelBroadcast(const char *pText)
{
	if(Client()->State() != IClient::STATE_ONLINE || !pText || pText[0] == '\0')
		return false;

	bool KnownTeeFusion = IsTeeFusionServer();
	const float Now = Client()->GlobalTime();
	if(m_TeeFusionDuelActive && Now - m_TeeFusionDuelLastUpdate > BKW_TF_DUEL_TIMEOUT_SECONDS)
		ResetTeeFusionDuel();

	bool AnyHandled = false;
	bool AnyUnknown = false;
	const char *pCursor = pText;
	char aLine[1024];
	while((pCursor = str_next_token(pCursor, "\n", aLine, sizeof(aLine))))
	{
		const std::string Line = BkwTrimDuelLine(aLine);
		if(Line.empty())
			continue;

		if(BkwIsTeeFusionDuelTitle(Line))
		{
			BkwServerDetector::MarkTeeFusionRuntime(Client()->ServerInfo());
			KnownTeeFusion = true;