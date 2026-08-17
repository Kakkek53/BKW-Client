from pathlib import Path

# configs
p = Path('src/engine/shared/config_variables_bestclient.h')
s = p.read_text(encoding='utf-8')
anchor = 'MACRO_CONFIG_INT(BkwMinimalHudPractice, bkw_minimal_hud_practice, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show practice state in BKW minimal HUD")\n'
add = ('MACRO_CONFIG_INT(BkwMinimalHudRaceTime, bkw_minimal_hud_race_time, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show race time in BKW minimal HUD")\n'
       'MACRO_CONFIG_INT(BkwMinimalHudPbDelta, bkw_minimal_hud_pb_delta, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show PB delta in BKW minimal HUD")\n')
if 'BkwMinimalHudRaceTime' not in s:
    assert anchor in s
    s = s.replace(anchor, anchor + add, 1)
p.write_text(s, encoding='utf-8')

# HUD renderer
p = Path('src/game/client/components/hud.cpp')
s = p.read_text(encoding='utf-8')
start = s.find('void CHud::RenderBkwMinimalHud()\n{')
end_anchor = 'bool CHud::RebuildFinishPredictionPathData() const\n'
end = s.find(end_anchor, start)
assert start >= 0 and end > start
impl = r'''void CHud::RenderBkwMinimalHud()
{
	if(!g_Config.m_BkwMinimalHud || Client()->State() != IClient::STATE_ONLINE)
		return;
	const int LocalId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalId < 0 || LocalId >= MAX_CLIENTS)
		return;

	const float Scale = std::clamp(g_Config.m_BkwMinimalHudScale / 100.0f, 0.75f, 1.25f);
	const float FontSize = 6.5f * Scale;
	const float PaddingX = 6.0f * Scale;
	const float PaddingY = 4.0f * Scale;
	const float Gap = 5.0f * Scale;
	const float Alpha = std::clamp(g_Config.m_BkwMinimalHudAlpha / 100.0f, 0.4f, 0.8f);

	char aLine[256] = {};
	auto Append = [&](const char *pText) {
		if(aLine[0])
			str_append(aLine, "   ", sizeof(aLine));
		str_append(aLine, pText, sizeof(aLine));
	};

	if(g_Config.m_BkwMinimalHudFps)
	{
		const float FrameTime = Client()->RenderFrameTime();
		const int Fps = FrameTime > 0.000001f ? (int)std::round(1.0f / FrameTime) : 0;
		char aBuf[32]; str_format(aBuf, sizeof(aBuf), "%d FPS", Fps); Append(aBuf);
	}
	if(g_Config.m_BkwMinimalHudPing)
	{
		int Ping = 0;
		if(GameClient()->m_Snap.m_apPlayerInfos[LocalId])
			Ping = maximum(0, GameClient()->m_Snap.m_apPlayerInfos[LocalId]->m_Latency);
		char aBuf[32]; str_format(aBuf, sizeof(aBuf), "%d ms", Ping); Append(aBuf);
	}
	if(g_Config.m_BkwMinimalHudTeam)
	{
		char aBuf[32]; str_format(aBuf, sizeof(aBuf), "TEAM %d", GameClient()->m_Teams.Team(LocalId)); Append(aBuf);
	}
	const auto &Character = GameClient()->m_Snap.m_aCharacters[LocalId];
	const bool Practice = Character.m_Active && Character.m_HasExtendedData && (Character.m_ExtendedData.m_Flags & CHARACTERFLAG_PRACTICE_MODE) != 0;
	if(g_Config.m_BkwMinimalHudPractice && Practice)
		Append("PRACTICE");

	if(g_Config.m_BkwMinimalHudRaceTime && m_DDRaceTime > 0)
	{
		char aTime[32];
		str_time(m_DDRaceTime, ETimeFormat::HOURS_CENTISECS, aTime, sizeof(aTime));
		char aBuf[48]; str_format(aBuf, sizeof(aBuf), "TIME %s", aTime); Append(aBuf);
	}

	char aDelta[48] = {};
	bool HasDelta = false;
	float Delta = 0.0f;
	if(g_Config.m_BkwMinimalHudPbDelta && m_DDRaceTime > 0 && m_aPlayerRecord[g_Config.m_ClDummy] > 0.0f)
	{
		Delta = (float)m_DDRaceTime / 100.0f - m_aPlayerRecord[g_Config.m_ClDummy];
		char aDeltaTime[32];
		str_time_float(std::fabs(Delta), ETimeFormat::HOURS_CENTISECS, aDeltaTime, sizeof(aDeltaTime));
		str_format(aDelta, sizeof(aDelta), "PB %c%s", Delta <= 0.0f ? '-' : '+', aDeltaTime);
		HasDelta = true;
	}

	if(!aLine[0] && !HasDelta)
		return;
	const float MainWidth = TextRender()->TextWidth(FontSize, aLine);
	const float DeltaWidth = HasDelta ? TextRender()->TextWidth(FontSize, aDelta) : 0.0f;
	const float Width = PaddingX * 2.0f + MainWidth + (HasDelta && aLine[0] ? Gap : 0.0f) + DeltaWidth;
	const float Height = FontSize + PaddingY * 2.0f;
	const float Margin = 5.0f * Scale;
	float X = Margin, Y = Margin;
	const int Corner = std::clamp(g_Config.m_BkwMinimalHudCorner, 0, 3);
	if(Corner == 1 || Corner == 3) X = m_Width - Width - Margin;
	if(Corner == 2 || Corner == 3) Y = m_Height - Height - Margin;

	Graphics()->DrawRect(X, Y, Width, Height, ColorRGBA(0.03f, 0.03f, 0.03f, Alpha), IGraphics::CORNER_ALL, 4.5f * Scale);
	float TextX = X + PaddingX;
	if(aLine[0])
	{
		TextRender()->TextColor(0.94f, 0.96f, 1.0f, 1.0f);
		TextRender()->Text(TextX, Y + PaddingY, FontSize, aLine, -1.0f);
		TextX += MainWidth + Gap;
	}
	if(HasDelta)
	{
		TextRender()->TextColor(Delta <= 0.0f ? ColorRGBA(0.45f, 1.0f, 0.55f, 1.0f) : ColorRGBA(1.0f, 0.45f, 0.45f, 1.0f));
		TextRender()->Text(TextX, Y + PaddingY, FontSize, aDelta, -1.0f);
	}
	TextRender()->TextColor(TextRender()->DefaultTextColor());
}

'''
s = s[:start] + impl + s[end:]
p.write_text(s, encoding='utf-8')

# Settings UI: add two checkboxes after Practice checkbox if current option block exists
p = Path('src/game/client/components/menus_settings.cpp')
s = p.read_text(encoding='utf-8')
if 's_MinimalHudRaceTimeToggleId' not in s:
    s = s.replace('static int s_MinimalHudToggleId;', 'static int s_MinimalHudToggleId;\n\t\tstatic int s_MinimalHudRaceTimeToggleId;\n\t\tstatic int s_MinimalHudPbDeltaToggleId;', 1)

marker = '"Practice"'
pos = s.find(marker)
if pos >= 0 and '"Race time"' not in s[pos:pos+2500]:
    # insert a small extra row before the next settings section/header, using simple labels/buttons
    insert_at = s.find('PageView.HSplitTop(12.0f, nullptr, &PageView);', pos)
    if insert_at < 0:
        insert_at = s.find('PageView.HSplitTop(10.0f, nullptr, &PageView);', pos)
    if insert_at > pos:
        block = '''CUIRect RaceOptions;\n\t\t\tPageView.HSplitTop(28.0f, &RaceOptions, &PageView);\n\t\t\tCUIRect RaceTimeOpt, PbDeltaOpt;\n\t\t\tRaceOptions.VSplitMid(&RaceTimeOpt, &PbDeltaOpt, 6.0f);\n\t\t\tif(DoButton_CheckBox(&s_MinimalHudRaceTimeToggleId, "Race time", g_Config.m_BkwMinimalHudRaceTime, &RaceTimeOpt))\n\t\t\t\tg_Config.m_BkwMinimalHudRaceTime ^= 1;\n\t\t\tif(DoButton_CheckBox(&s_MinimalHudPbDeltaToggleId, "PB delta", g_Config.m_BkwMinimalHudPbDelta, &PbDeltaOpt))\n\t\t\t\tg_Config.m_BkwMinimalHudPbDelta ^= 1;\n\t\t\t'''
        s = s[:insert_at] + block + s[insert_at:]
p.write_text(s, encoding='utf-8')
