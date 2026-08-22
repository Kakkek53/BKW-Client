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
	// TeeFusion has used both the normal Cyrillic spelling and a variant where
	// the second character is a Latin 'y'. Keep both so the parser survives
	// the exact broadcast form seen on the live server.
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
	// The renderer has no cheap generic backdrop-blur primitive for HUD widgets.
	// Multiple soft translucent shells + a dense glass body give the same visual
	// hierarchy without introducing a new post-processing pass.
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
	return Client()->State() == IClient::STATE_ONLINE && str_comp(Client()->ServerInfo().m_aCommunityId, "teefusion") == 0;
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
		// A new duel can begin immediately after the old one while the server keeps
		// broadcasting. Prefer replacing a player not seen in this title/score cycle.
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
	if(!IsTeeFusionServer() || !pText || pText[0] == '\0')
		return false;

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
			if(!m_TeeFusionDuelActive)
				ResetTeeFusionDuel();
			m_TeeFusionDuelActive = true;
			m_TeeFusionDuelLastUpdate = Now;
			++m_TeeFusionDuelCycle;
			AnyHandled = true;
			continue;
		}

		std::string Name;
		int Score = 0;
		if(m_TeeFusionDuelActive && BkwParseTeeFusionDuelScore(Line, Name, Score))
		{
			UpdateTeeFusionDuelPlayer(Name.c_str(), Score);
			AnyHandled = true;
		}
		else
		{
			AnyUnknown = true;
		}
	}

	// Suppress the original broadcast only when the entire message belongs to the
	// duel HUD. Mixed server messages keep rendering normally.
	return AnyHandled && !AnyUnknown;
}

void CBroadcast::OnReset()
{
	m_BroadcastTick = 0;
	m_BroadcastRenderOffset = -1.0f;
	TextRender()->DeleteTextContainer(m_TextContainerIndex);
	ResetTeeFusionDuel();
}

void CBroadcast::OnWindowResize()
{
	m_BroadcastRenderOffset = -1.0f;
	TextRender()->DeleteTextContainer(m_TextContainerIndex);
}

void CBroadcast::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		ResetTeeFusionDuel();
		return;
	}

	RenderServerBroadcast();
	RenderTeeFusionDuelHud();
}

void CBroadcast::RenderServerBroadcast()
{
	if(GameClient()->m_Scoreboard.IsActive() ||
		GameClient()->m_Motd.IsActive() ||
		GameClient()->m_ImportantAlert.IsActive() ||
		!g_Config.m_ClShowBroadcasts)
	{
		return;
	}

	const float SecondsRemaining = (m_BroadcastTick - Client()->GameTick(g_Config.m_ClDummy)) / (float)Client()->GameTickSpeed();
	if(SecondsRemaining <= 0.0f)
	{
		TextRender()->DeleteTextContainer(m_TextContainerIndex);
		return;
	}

	const float Height = 300.0f;
	const float Width = Height * Graphics()->ScreenAspect();
	Graphics()->MapScreenToSize(Width, Height);

	if(m_BroadcastRenderOffset < 0.0f)
		m_BroadcastRenderOffset = Width / 2.0f - TextRender()->TextWidth(12.0f, m_aBroadcastText, -1, Width) / 2.0f;

	if(!m_TextContainerIndex.Valid())
	{
		CTextCursor Cursor;
		Cursor.SetPosition(vec2(m_BroadcastRenderOffset, 40.0f));
		Cursor.m_FontSize = 12.0f;
		Cursor.m_LineWidth = Width;
		TextRender()->CreateTextContainer(m_TextContainerIndex, &Cursor, m_aBroadcastText);
	}
	if(m_TextContainerIndex.Valid())
	{
		const float Alpha = SecondsRemaining >= 1.0f ? 1.0f : SecondsRemaining;
		ColorRGBA TextColor = TextRender()->DefaultTextColor();
		TextColor.a *= Alpha;
		ColorRGBA OutlineColor = TextRender()->DefaultTextOutlineColor();
		OutlineColor.a *= Alpha;
		TextRender()->RenderTextContainer(m_TextContainerIndex, TextColor, OutlineColor);
	}
}

void CBroadcast::RenderTeeFusionDuelHud()
{
	if(!g_Config.m_BkwTfDuelHud || !IsTeeFusionServer())
	{
		if(m_TeeFusionDuelActive)
			ResetTeeFusionDuel();
		return;
	}

	if(!m_TeeFusionDuelActive || m_TeeFusionDuelPlayerCount <= 0)
		return;

	const float Age = Client()->GlobalTime() - m_TeeFusionDuelLastUpdate;
	if(Age >= BKW_TF_DUEL_TIMEOUT_SECONDS)
	{
		ResetTeeFusionDuel();
		return;
	}

	if(GameClient()->m_Scoreboard.IsActive() || GameClient()->m_Motd.IsActive() || GameClient()->m_ImportantAlert.IsActive())
		return;

	float Alpha = 1.0f;
	const float FadeStart = BKW_TF_DUEL_TIMEOUT_SECONDS - BKW_TF_DUEL_FADE_SECONDS;
	if(Age > FadeStart)
		Alpha = std::clamp((BKW_TF_DUEL_TIMEOUT_SECONDS - Age) / BKW_TF_DUEL_FADE_SECONDS, 0.0f, 1.0f);

	Ui()->MapScreen();
	const CUIRect Screen = *Ui()->Screen();
	const float CardWidth = minimum(420.0f, Screen.w - 24.0f);
	CUIRect Card = {Screen.x + (Screen.w - CardWidth) * 0.5f, Screen.y + 16.0f, CardWidth, 84.0f};

	const auto &LeftPlayer = m_aTeeFusionDuelPlayers[0];
	const bool HasRight = m_TeeFusionDuelPlayerCount >= 2;
	const auto &RightPlayer = m_aTeeFusionDuelPlayers[HasRight ? 1 : 0];
	char aLeftScore[24];
	char aRightScore[24];
	str_format(aLeftScore, sizeof(aLeftScore), "%d", LeftPlayer.m_Score);
	if(HasRight)
		str_format(aRightScore, sizeof(aRightScore), "%d", RightPlayer.m_Score);
	else
		str_copy(aRightScore, "—");

	const float Now = Client()->GlobalTime();
	const float LeftPulse = std::clamp(1.0f - (Now - LeftPlayer.m_ScoreChangedAt) / BKW_TF_DUEL_SCORE_PULSE_SECONDS, 0.0f, 1.0f);
	const float RightPulse = HasRight ? std::clamp(1.0f - (Now - RightPlayer.m_ScoreChangedAt) / BKW_TF_DUEL_SCORE_PULSE_SECONDS, 0.0f, 1.0f) : 0.0f;

	const int Style = std::clamp(g_Config.m_BkwTfDuelHudStyle, 0, 3);
	if(Style == 0)
	{
		BkwDuelDrawLiquidGlass(Card, Alpha);

		CUIRect Header, Content;
		Card.HSplitTop(22.0f, &Header, &Content);
		BkwDuelLabel(Ui(), Header, "TEEFUSION  •  DUEL", 8.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.82f, 0.90f, 1.0f, 0.86f), Alpha));

		Content.Margin(8.0f, &Content);
		CUIRect Left, Center, Right;
		Content.VSplitLeft(Content.w * 0.43f, &Left, &Content);
		Content.VSplitRight(Content.w * 0.43f, &Center, &Right);

		CUIRect LeftName, LeftScore, RightName, RightScore;
		Left.HSplitTop(18.0f, &LeftName, &LeftScore);
		Right.HSplitTop(18.0f, &RightName, &RightScore);
		BkwDuelLabel(Ui(), LeftName, LeftPlayer.m_aName, 10.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.93f, 0.96f, 1.0f, 0.93f), Alpha));
		BkwDuelLabel(Ui(), RightName, HasRight ? RightPlayer.m_aName : "Ожидание…", 10.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.93f, 0.96f, 1.0f, HasRight ? 0.93f : 0.46f), Alpha));
		BkwDuelLabel(Ui(), LeftScore, aLeftScore, 24.0f + LeftPulse * 1.8f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.64f + LeftPulse * 0.20f, 0.83f + LeftPulse * 0.12f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), RightScore, aRightScore, 24.0f + RightPulse * 1.8f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.88f + RightPulse * 0.10f, 0.77f + RightPulse * 0.12f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), Center, "VS", 9.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.40f), Alpha));
	}
	else if(Style == 1)
	{
		// Minimal: almost no chrome, only a subtle readability shadow and divider.
		CUIRect Shadow = Card;
		Shadow.y += 2.0f;
		Shadow.Draw(BkwDuelAlpha(ColorRGBA(0.0f, 0.0f, 0.0f, 0.14f), Alpha), IGraphics::CORNER_ALL, 12.0f);
		Card.Draw(BkwDuelAlpha(ColorRGBA(0.02f, 0.025f, 0.035f, 0.24f), Alpha), IGraphics::CORNER_ALL, 12.0f);
		CUIRect Header, Row;
		Card.HSplitTop(20.0f, &Header, &Row);
		BkwDuelLabel(Ui(), Header, "DUEL", 8.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.50f), Alpha));
		Row.Margin(10.0f, &Row);
		CUIRect Left, Mid, Right;
		Row.VSplitLeft(Row.w * 0.44f, &Left, &Row);
		Row.VSplitRight(Row.w * 0.44f, &Mid, &Right);
		char aLeft[96], aRight[96];
		str_format(aLeft, sizeof(aLeft), "%s   %s", LeftPlayer.m_aName, aLeftScore);
		str_format(aRight, sizeof(aRight), "%s   %s", aRightScore, HasRight ? RightPlayer.m_aName : "…");
		BkwDuelLabel(Ui(), Left, aLeft, 13.0f, TEXTALIGN_MR, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.96f), Alpha));
		BkwDuelLabel(Ui(), Right, aRight, 13.0f, TEXTALIGN_ML, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.96f), Alpha));
		CUIRect Divider = Mid;
		Divider.x += Divider.w * 0.5f - 0.5f;
		Divider.w = 1.0f;
		Divider.y += 10.0f;
		Divider.h -= 20.0f;
		Divider.Draw(BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.28f), Alpha), IGraphics::CORNER_ALL, 1.0f);
	}
	else if(Style == 2)
	{
		// Neon arena card.
		Card.Draw(BkwDuelAlpha(ColorRGBA(0.018f, 0.022f, 0.038f, 0.90f), Alpha), IGraphics::CORNER_ALL, 11.0f);
		CUIRect TopAccent = Card;
		TopAccent.h = 2.0f;
		TopAccent.Draw4(
			BkwDuelAlpha(ColorRGBA(0.18f, 0.78f, 1.0f, 0.95f), Alpha),
			BkwDuelAlpha(ColorRGBA(0.82f, 0.25f, 1.0f, 0.95f), Alpha),
			BkwDuelAlpha(ColorRGBA(0.18f, 0.78f, 1.0f, 0.95f), Alpha),
			BkwDuelAlpha(ColorRGBA(0.82f, 0.25f, 1.0f, 0.95f), Alpha),
			IGraphics::CORNER_T, 11.0f);

		CUIRect Header, Row;
		Card.HSplitTop(21.0f, &Header, &Row);
		BkwDuelLabel(Ui(), Header, "TEEFUSION DUEL", 8.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.75f, 0.80f, 0.92f, 0.78f), Alpha));
		Row.Margin(8.0f, &Row);
		CUIRect Left, Center, Right;
		Row.VSplitLeft(Row.w * 0.44f, &Left, &Row);
		Row.VSplitRight(Row.w * 0.44f, &Center, &Right);
		CUIRect LeftName, LeftScore, RightName, RightScore;
		Left.HSplitTop(18.0f, &LeftName, &LeftScore);
		Right.HSplitTop(18.0f, &RightName, &RightScore);
		BkwDuelLabel(Ui(), LeftName, LeftPlayer.m_aName, 9.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.52f, 0.88f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), RightName, HasRight ? RightPlayer.m_aName : "Ожидание…", 9.5f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.92f, 0.58f, 1.0f, HasRight ? 1.0f : 0.45f), Alpha));
		BkwDuelLabel(Ui(), LeftScore, aLeftScore, 25.0f + LeftPulse * 2.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.38f, 0.82f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), RightScore, aRightScore, 25.0f + RightPulse * 2.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.86f, 0.38f, 1.0f, 1.0f), Alpha));
		BkwDuelLabel(Ui(), Center, "VS", 9.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.36f), Alpha));
	}
	else
	{
		// Classic: compact esports-style scoreboard.
		Card.h = 64.0f;
		Card.Draw(BkwDuelAlpha(ColorRGBA(0.035f, 0.035f, 0.045f, 0.88f), Alpha), IGraphics::CORNER_ALL, 5.0f);
		CUIRect Header, Row;
		Card.HSplitTop(18.0f, &Header, &Row);
		Header.Draw(BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.065f), Alpha), IGraphics::CORNER_T, 5.0f);
		BkwDuelLabel(Ui(), Header, "TEEFUSION • ДУЭЛЬ", 8.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.90f, 0.92f, 0.96f, 0.84f), Alpha));
		Row.Margin(7.0f, &Row);
		CUIRect Left, Score, Right;
		Row.VSplitLeft(Row.w * 0.37f, &Left, &Row);
		Row.VSplitRight(Row.w * 0.37f, &Score, &Right);
		BkwDuelLabel(Ui(), Left, LeftPlayer.m_aName, 11.0f, TEXTALIGN_MR, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, 0.94f), Alpha));
		BkwDuelLabel(Ui(), Right, HasRight ? RightPlayer.m_aName : "…", 11.0f, TEXTALIGN_ML, BkwDuelAlpha(ColorRGBA(1.0f, 1.0f, 1.0f, HasRight ? 0.94f : 0.40f), Alpha));
		char aScore[64];
		str_format(aScore, sizeof(aScore), "%s   :   %s", aLeftScore, aRightScore);
		BkwDuelLabel(Ui(), Score, aScore, 18.0f, TEXTALIGN_MC, BkwDuelAlpha(ColorRGBA(0.72f, 0.84f, 1.0f, 1.0f), Alpha));
	}
}

void CBroadcast::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_BROADCAST)
	{
		const CNetMsg_Sv_Broadcast *pMsg = (CNetMsg_Sv_Broadcast *)pRawMsg;
		DoBroadcast(pMsg->m_pMessage);
	}
}

void CBroadcast::DoBroadcast(const char *pText)
{
	const char *pOriginalText = pText;
	const bool DuelBroadcast = g_Config.m_BkwTfDuelHud && HandleTeeFusionDuelBroadcast(pText);
	if(DuelBroadcast && g_Config.m_BkwTfDuelHideBroadcast)
	{
		m_aBroadcastText[0] = '\0';
		m_BroadcastTick = 0;
		m_BroadcastRenderOffset = -1.0f;
		TextRender()->DeleteTextContainer(m_TextContainerIndex);
	}
	else
	{
		str_copy(m_aBroadcastText, pText);
		m_BroadcastTick = Client()->GameTick(g_Config.m_ClDummy) + Client()->GameTickSpeed() * 10;
		m_BroadcastRenderOffset = -1.0f;
		TextRender()->DeleteTextContainer(m_TextContainerIndex);
	}

	if(g_Config.m_ClPrintBroadcasts)
	{
		const LOG_COLOR LogColor = color_cast<LOG_COLOR>(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageHighlightColor)));
		char aLine[sizeof(m_aBroadcastText)];
		while((pOriginalText = str_next_token(pOriginalText, "\n", aLine, sizeof(aLine))))
		{
			if(aLine[0] != '\0')
			{
				log_info_color(LogColor, "broadcast", "%s", aLine);
			}
		}
	}
}
