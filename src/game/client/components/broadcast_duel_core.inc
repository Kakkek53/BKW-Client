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
constexpr float BKW_TF_DUEL_CANDIDATE_WINDOW_SECONDS = 2.2f;

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
	if(Name.empty() || Name.size() >= 64)
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

	if(ParsedScore < 0 || ParsedScore > 9999)
		return false;

	Score = (int)ParsedScore;
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

void BkwDuelDrawLiquidGlass(CUIRect Card, float Alpha, float Rounding)
{
	for(int i = 3; i >= 1; --i)
	{
		CUIRect Glow = Card;
		const float Expand = (float)i * 1.7f;
		Glow.x -= Expand;
		Glow.y -= Expand;
		Glow.w += Expand * 2.0f;
		Glow.h += Expand * 2.0f;
		Glow.Draw(BkwDuelAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.028f + i * 0.012f), Alpha), IGraphics::CORNER_ALL, Rounding + Expand);
	}

	Card.Draw(BkwDuelAlpha(ColorRGBA(0.82f, 0.90f, 1.0f, 0.17f), Alpha), IGraphics::CORNER_ALL, Rounding);
	CUIRect Inner;
	Card.Margin(1.0f, &Inner);
	Inner.Draw4(
		BkwDuelAlpha(ColorRGBA(0.10f, 0.14f, 0.21f, 0.78f), Alpha),
		BkwDuelAlpha(ColorRGBA(0.13f, 0.17f, 0.25f, 0.72f), Alpha),
		BkwDuelAlpha(ColorRGBA(0.045f, 0.060f, 0.10f, 0.78f), Alpha),
		BkwDuelAlpha(ColorRGBA(0.065f, 0.080f, 0.13f, 0.74f), Alpha),
		IGraphics::CORNER_ALL, std::max(0.0f, Rounding - 1.0f));
}

CUIRect BkwDuelCardRect(const CUIRect &Screen, float Width, float Height, int Position)
{
	constexpr float Margin = 14.0f;
	const int Column = std::clamp(Position, 0, 8) % 3;
	const int Row = std::clamp(Position, 0, 8) / 3;

	CUIRect Card{0.0f, 0.0f, Width, Height};
	if(Column == 0)
		Card.x = Screen.x + Margin;
	else if(Column == 1)
		Card.x = Screen.x + (Screen.w - Width) * 0.5f;
	else
		Card.x = Screen.x + Screen.w - Width - Margin;

	if(Row == 0)
		Card.y = Screen.y + Margin;
	else if(Row == 1)
		Card.y = Screen.y + (Screen.h - Height) * 0.5f;
	else
		Card.y = Screen.y + Screen.h - Height - Margin;

	return Card;
}
} // namespace

void CBroadcast::ResetTeeFusionDuel()
{
	m_TeeFusionDuelActive = false;
	m_TeeFusionDuelLastUpdate = 0.0f;
	m_TeeFusionDuelLastTitleAt = -1000.0f;
	m_TeeFusionDuelCycle = 0;
	m_TeeFusionDuelPlayerCount = 0;
	m_TeeFusionDuelCandidateCount = 0;
	for(auto &Player : m_aTeeFusionDuelPlayers)
		Player = STeeFusionDuelPlayer();
	for(auto &Candidate : m_aTeeFusionDuelCandidates)
		Candidate = STeeFusionDuelCandidate();
}

bool CBroadcast::IsTeeFusionServer() const
{
	return Client()->State() == IClient::STATE_ONLINE &&
		BkwServerDetector::IsTeeFusion(Http(), Client()->ServerAddress());
}

void CBroadcast::BufferTeeFusionDuelCandidate(const char *pName, int Score, float SeenAt)
{
	for(int i = 0; i < m_TeeFusionDuelCandidateCount; ++i)
	{
		if(str_comp(m_aTeeFusionDuelCandidates[i].m_aName, pName) == 0)
		{
			m_aTeeFusionDuelCandidates[i].m_Score = Score;
			m_aTeeFusionDuelCandidates[i].m_SeenAt = SeenAt;
			return;
		}
	}
