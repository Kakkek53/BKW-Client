from pathlib import Path

# 1) Config options
p = Path('src/engine/shared/config_variables_bestclient.h')
s = p.read_text(encoding='utf-8')
anchor = 'MACRO_CONFIG_INT(BkwMinimalHud, bkw_minimal_hud, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable BKW minimal HUD")\n'
assert anchor in s
options = '''MACRO_CONFIG_INT(BkwMinimalHudFps, bkw_minimal_hud_fps, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show FPS in BKW minimal HUD")
MACRO_CONFIG_INT(BkwMinimalHudPing, bkw_minimal_hud_ping, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show ping in BKW minimal HUD")
MACRO_CONFIG_INT(BkwMinimalHudTeam, bkw_minimal_hud_team, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show team in BKW minimal HUD")
MACRO_CONFIG_INT(BkwMinimalHudPractice, bkw_minimal_hud_practice, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show practice state in BKW minimal HUD")
MACRO_CONFIG_INT(BkwMinimalHudCorner, bkw_minimal_hud_corner, 0, 0, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW minimal HUD corner: 0 TL, 1 TR, 2 BL, 3 BR")
MACRO_CONFIG_INT(BkwMinimalHudScale, bkw_minimal_hud_scale, 100, 75, 125, CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW minimal HUD scale percent")
MACRO_CONFIG_INT(BkwMinimalHudAlpha, bkw_minimal_hud_alpha, 60, 40, 80, CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW minimal HUD background opacity percent")
'''
if 'BkwMinimalHudFps' not in s:
    s = s.replace(anchor, anchor + options, 1)
p.write_text(s, encoding='utf-8')

# 2) Replace HUD renderer
p = Path('src/game/client/components/hud.cpp')
s = p.read_text(encoding='utf-8')
start = s.index('void CHud::RenderBkwMinimalHud()\n{')
end = s.index('\n}\n\nbool CHud::RebuildFinishPredictionPathData() const', start) + 3
new_impl = r'''void CHud::RenderBkwMinimalHud()
{
	if(!g_Config.m_BkwMinimalHud || Client()->State() != IClient::STATE_ONLINE)
		return;

	const int LocalId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalId < 0 || LocalId >= MAX_CLIENTS)
		return;

	char aLine[256] = {};
	auto AppendPart = [&](const char *pText) {
		if(aLine[0] != '\0')
			str_append(aLine, "   ", sizeof(aLine));
		str_append(aLine, pText, sizeof(aLine));
	};

	if(g_Config.m_BkwMinimalHudFps)
	{
		const float FrameTime = Client()->RenderFrameTime();
		const int Fps = FrameTime > 0.000001f ? (int)std::round(1.0f / FrameTime) : 0;
		char aPart[32];
		str_format(aPart, sizeof(aPart), "%d FPS", Fps);
		AppendPart(aPart);
	}

	if(g_Config.m_BkwMinimalHudPing)
	{
		int Ping = 0;
		if(GameClient()->m_Snap.m_apPlayerInfos[LocalId])
			Ping = maximum(0, GameClient()->m_Snap.m_apPlayerInfos[LocalId]->m_Latency);
		char aPart[32];
		str_format(aPart, sizeof(aPart), "%d ms", Ping);
		AppendPart(aPart);
	}

	if(g_Config.m_BkwMinimalHudTeam)
	{
		char aPart[32];
		str_format(aPart, sizeof(aPart), "TEAM %d", GameClient()->m_Teams.Team(LocalId));
		AppendPart(aPart);
	}

	if(g_Config.m_BkwMinimalHudPractice)
	{
		const auto &Character = GameClient()->m_Snap.m_aCharacters[LocalId];
		const bool Practice = Character.m_Active && Character.m_HasExtendedData && (Character.m_ExtendedData.m_Flags & CHARACTERFLAG_PRACTICE_MODE) != 0;
		if(Practice)
			AppendPart("PRACTICE");
	}

	if(aLine[0] == '\0')
		return;

	const float Scale = std::clamp(g_Config.m_BkwMinimalHudScale / 100.0f, 0.75f, 1.25f);
	const float FontSize = 6.5f * Scale;
	const float PaddingX = 6.0f * Scale;
	const float PaddingY = 4.0f * Scale;
	const float Margin = 5.0f * Scale;
	const float Width = TextRender()->TextWidth(FontSize, aLine) + PaddingX * 2.0f;
	const float Height = FontSize + PaddingY * 2.0f;

	float X = Margin;
	float Y = Margin;
	const int Corner = std::clamp(g_Config.m_BkwMinimalHudCorner, 0, 3);
	if(Corner == 1 || Corner == 3)
		X = m_Width - Width - Margin;
	if(Corner == 2 || Corner == 3)
		Y = m_Height - Height - Margin;

	const float Alpha = std::clamp(g_Config.m_BkwMinimalHudAlpha / 100.0f, 0.4f, 0.8f);
	Graphics()->DrawRect(X, Y, Width, Height, ColorRGBA(0.03f, 0.03f, 0.03f, Alpha), IGraphics::CORNER_ALL, 4.5f * Scale);
	TextRender()->TextColor(0.94f, 0.96f, 1.0f, 1.0f);
	TextRender()->Text(X + PaddingX, Y + PaddingY, FontSize, aLine, -1.0f);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
}
'''
s = s[:start] + new_impl + s[end:]
p.write_text(s, encoding='utf-8')

# 3) Expand BKW HUD settings UI
p = Path('src/game/client/components/menus_settings.cpp')
s = p.read_text(encoding='utf-8')
# extra ids/buttons
old = 'static int s_MinimalHudToggleId;\n\t\tstatic int s_MediaBackgroundToggleId;'
new = '''static int s_MinimalHudToggleId;
		static int s_MinimalHudFpsToggleId;
		static int s_MinimalHudPingToggleId;
		static int s_MinimalHudTeamToggleId;
		static int s_MinimalHudPracticeToggleId;
		static CButtonContainer s_aMinimalHudCornerButtons[4];
		static CButtonContainer s_aMinimalHudScaleButtons[3];
		static CButtonContainer s_aMinimalHudAlphaButtons[3];
		static int s_MediaBackgroundToggleId;'''
if 's_MinimalHudFpsToggleId' not in s:
    assert old in s
    s = s.replace(old, new, 1)

start = s.index('\t\telse if(s_BkwTab == BKW_TAB_HUD)\n\t\t{')
end = s.index('\t\telse if(s_BkwTab == BKW_TAB_BACKGROUND)\n', start)
block = r'''		else if(s_BkwTab == BKW_TAB_HUD)
		{
			CUIRect Header;
			PageView.HSplitTop(28.0f, &Header, &PageView);
			Ui()->DoLabel(&Header, "BKW — Минималистичный HUD", 22.0f, TEXTALIGN_ML);
			PageView.HSplitTop(8.0f, nullptr, &PageView);

			CUIRect Toggle;
			PageView.HSplitTop(28.0f, &Toggle, &PageView);
			if(DoButton_CheckBox(&s_MinimalHudToggleId, "Минималистичный HUD", g_Config.m_BkwMinimalHud, &Toggle))
				g_Config.m_BkwMinimalHud ^= 1;

			if(g_Config.m_BkwMinimalHud)
			{
				PageView.HSplitTop(8.0f, nullptr, &PageView);
				CUIRect ElementsCard;
				PageView.HSplitTop(124.0f, &ElementsCard, &PageView);
				ElementsCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
				ElementsCard.Margin(10.0f, &ElementsCard);
				CUIRect Title, Row1, Row2;
				ElementsCard.HSplitTop(24.0f, &Title, &ElementsCard);
				Ui()->DoLabel(&Title, "Элементы", 15.0f, TEXTALIGN_ML);
				ElementsCard.HSplitTop(34.0f, &Row1, &ElementsCard);
				ElementsCard.HSplitTop(34.0f, &Row2, &ElementsCard);
				CUIRect Fps, Ping, Team, Practice;
				Row1.VSplitMid(&Fps, &Ping, 8.0f);
				Row2.VSplitMid(&Team, &Practice, 8.0f);
				if(DoButton_CheckBox(&s_MinimalHudFpsToggleId, "FPS", g_Config.m_BkwMinimalHudFps, &Fps)) g_Config.m_BkwMinimalHudFps ^= 1;
				if(DoButton_CheckBox(&s_MinimalHudPingToggleId, "Ping", g_Config.m_BkwMinimalHudPing, &Ping)) g_Config.m_BkwMinimalHudPing ^= 1;
				if(DoButton_CheckBox(&s_MinimalHudTeamToggleId, "Team", g_Config.m_BkwMinimalHudTeam, &Team)) g_Config.m_BkwMinimalHudTeam ^= 1;
				if(DoButton_CheckBox(&s_MinimalHudPracticeToggleId, "Practice", g_Config.m_BkwMinimalHudPractice, &Practice)) g_Config.m_BkwMinimalHudPractice ^= 1;

				PageView.HSplitTop(10.0f, nullptr, &PageView);
				CUIRect LayoutCard;
				PageView.HSplitTop(166.0f, &LayoutCard, &PageView);
				LayoutCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
				LayoutCard.Margin(10.0f, &LayoutCard);
				CUIRect L1, Corners, L2, Scales, L3, Alphas;
				LayoutCard.HSplitTop(20.0f, &L1, &LayoutCard);
				Ui()->DoLabel(&L1, "Положение", 12.0f, TEXTALIGN_ML);
				LayoutCard.HSplitTop(30.0f, &Corners, &LayoutCard);
				const char *apCorners[4] = {"↖", "↗", "↙", "↘"};
				CUIRect CornerRemain = Corners;
				for(int i = 0; i < 4; ++i)
				{
					CUIRect B; CornerRemain.VSplitLeft(Corners.w / 4.0f, &B, &CornerRemain);
					if(DoButton_Menu(&s_aMinimalHudCornerButtons[i], apCorners[i], g_Config.m_BkwMinimalHudCorner == i, &B)) g_Config.m_BkwMinimalHudCorner = i;
				}
				LayoutCard.HSplitTop(20.0f, &L2, &LayoutCard);
				Ui()->DoLabel(&L2, "Масштаб", 12.0f, TEXTALIGN_ML);
				LayoutCard.HSplitTop(30.0f, &Scales, &LayoutCard);
				const int aScales[3] = {75, 100, 125};
				const char *apScales[3] = {"75%", "100%", "125%"};
				CUIRect ScaleRemain = Scales;
				for(int i = 0; i < 3; ++i)
				{
					CUIRect B; ScaleRemain.VSplitLeft(Scales.w / 3.0f, &B, &ScaleRemain);
					if(DoButton_Menu(&s_aMinimalHudScaleButtons[i], apScales[i], g_Config.m_BkwMinimalHudScale == aScales[i], &B)) g_Config.m_BkwMinimalHudScale = aScales[i];
				}
				LayoutCard.HSplitTop(20.0f, &L3, &LayoutCard);
				Ui()->DoLabel(&L3, "Прозрачность фона", 12.0f, TEXTALIGN_ML);
				LayoutCard.HSplitTop(30.0f, &Alphas, &LayoutCard);
				const int aAlphas[3] = {40, 60, 80};
				const char *apAlphas[3] = {"40%", "60%", "80%"};
				CUIRect AlphaRemain = Alphas;
				for(int i = 0; i < 3; ++i)
				{
					CUIRect B; AlphaRemain.VSplitLeft(Alphas.w / 3.0f, &B, &AlphaRemain);
					if(DoButton_Menu(&s_aMinimalHudAlphaButtons[i], apAlphas[i], g_Config.m_BkwMinimalHudAlpha == aAlphas[i], &B)) g_Config.m_BkwMinimalHudAlpha = aAlphas[i];
				}
			}
		}
'''
s = s[:start] + block + s[end:]
p.write_text(s, encoding='utf-8')
