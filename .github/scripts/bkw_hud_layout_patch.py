from pathlib import Path

# Config
p = Path('src/engine/shared/config_variables_bestclient.h')
s = p.read_text(encoding='utf-8')
anchor = 'MACRO_CONFIG_INT(BkwMinimalHudAlpha, bkw_minimal_hud_alpha, 60, 40, 80, CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW minimal HUD background opacity percent")\n'
line = 'MACRO_CONFIG_INT(BkwMinimalHudLayout, bkw_minimal_hud_layout, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW minimal HUD layout: 0 horizontal, 1 compact two-line")\n'
if 'BkwMinimalHudLayout' not in s:
    assert anchor in s
    s = s.replace(anchor, anchor + line, 1)
p.write_text(s, encoding='utf-8')

# HUD renderer
p = Path('src/game/client/components/hud.cpp')
s = p.read_text(encoding='utf-8')
start = s.find('void CHud::RenderBkwMinimalHud()\n{')
end_anchor = 'bool CHud::RebuildFinishPredictionPathData() const\n'
end = s.find(end_anchor, start)
assert start >= 0 and end > start
old = s[start:end]
# Keep logic generation, but split line before race info in compact layout.
old = old.replace('char aLine[256] = {};', 'char aLine[256] = {};\n\tchar aLine2[192] = {};')
old = old.replace('auto Append = [&](const char *pText) {\n\t\tif(aLine[0])\n\t\t\tstr_append(aLine, "   ", sizeof(aLine));\n\t\tstr_append(aLine, pText, sizeof(aLine));\n\t};', '''auto Append = [&](const char *pText) {\n\t\tif(aLine[0])\n\t\t\tstr_append(aLine, "   ", sizeof(aLine));\n\t\tstr_append(aLine, pText, sizeof(aLine));\n\t};\n\tauto Append2 = [&](const char *pText) {\n\t\tif(aLine2[0])\n\t\t\tstr_append(aLine2, "   ", sizeof(aLine2));\n\t\tstr_append(aLine2, pText, sizeof(aLine2));\n\t};''')
old = old.replace('char aBuf[48]; str_format(aBuf, sizeof(aBuf), "TIME %s", aTime); Append(aBuf);', 'char aBuf[48]; str_format(aBuf, sizeof(aBuf), "TIME %s", aTime); (g_Config.m_BkwMinimalHudLayout ? Append2 : Append)(aBuf);')
old = old.replace('const float MainWidth = TextRender()->TextWidth(FontSize, aLine);', 'const float MainWidth = TextRender()->TextWidth(FontSize, aLine);\n\tconst float Line2Width = TextRender()->TextWidth(FontSize, aLine2);')
old = old.replace('const float Width = PaddingX * 2.0f + MainWidth + (HasDelta && aLine[0] ? Gap : 0.0f) + DeltaWidth;\n\tconst float Height = FontSize + PaddingY * 2.0f;', '''const bool Compact = g_Config.m_BkwMinimalHudLayout != 0;\n\tconst float CompactSecondWidth = Line2Width + (HasDelta && aLine2[0] ? Gap : 0.0f) + DeltaWidth;\n\tconst float HorizontalWidth = MainWidth + (HasDelta && aLine[0] ? Gap : 0.0f) + DeltaWidth;\n\tconst float Width = PaddingX * 2.0f + (Compact ? maximum(MainWidth, CompactSecondWidth) : HorizontalWidth);\n\tconst float Height = (Compact && (aLine2[0] || HasDelta) ? FontSize * 2.0f + PaddingY * 2.0f + 2.0f * Scale : FontSize + PaddingY * 2.0f);''')
# Replace rendering tail for two lines.
needle = '''\tfloat TextX = X + PaddingX;\n\tif(aLine[0])\n\t{\n\t\tTextRender()->TextColor(0.94f, 0.96f, 1.0f, 1.0f);\n\t\tTextRender()->Text(TextX, Y + PaddingY, FontSize, aLine, -1.0f);\n\t\tTextX += MainWidth + Gap;\n\t}\n\tif(HasDelta)\n\t{\n\t\tTextRender()->TextColor(Delta <= 0.0f ? ColorRGBA(0.45f, 1.0f, 0.55f, 1.0f) : ColorRGBA(1.0f, 0.45f, 0.45f, 1.0f));\n\t\tTextRender()->Text(TextX, Y + PaddingY, FontSize, aDelta, -1.0f);\n\t}\n'''
repl = '''\tfloat TextX = X + PaddingX;\n\tif(aLine[0])\n\t{\n\t\tTextRender()->TextColor(0.94f, 0.96f, 1.0f, 1.0f);\n\t\tTextRender()->Text(TextX, Y + PaddingY, FontSize, aLine, -1.0f);\n\t\tif(!Compact)\n\t\t\tTextX += MainWidth + Gap;\n\t}\n\tfloat DeltaY = Y + PaddingY;\n\tif(Compact && (aLine2[0] || HasDelta))\n\t{\n\t\tTextX = X + PaddingX;\n\t\tDeltaY += FontSize + 2.0f * Scale;\n\t\tif(aLine2[0])\n\t\t{\n\t\t\tTextRender()->TextColor(0.94f, 0.96f, 1.0f, 1.0f);\n\t\t\tTextRender()->Text(TextX, DeltaY, FontSize, aLine2, -1.0f);\n\t\t\tTextX += Line2Width + Gap;\n\t\t}\n\t}\n\tif(HasDelta)\n\t{\n\t\tTextRender()->TextColor(Delta <= 0.0f ? ColorRGBA(0.45f, 1.0f, 0.55f, 1.0f) : ColorRGBA(1.0f, 0.45f, 0.45f, 1.0f));\n\t\tTextRender()->Text(TextX, DeltaY, FontSize, aDelta, -1.0f);\n\t}\n'''
assert needle in old
old = old.replace(needle, repl, 1)
s = s[:start] + old + s[end:]
p.write_text(s, encoding='utf-8')

# Settings UI
p = Path('src/game/client/components/menus_settings.cpp')
s = p.read_text(encoding='utf-8')
if 's_MinimalHudLayoutHorizontalButton' not in s:
    anchor_ids = 'static int s_MinimalHudCheckpointToggleId;'
    assert anchor_ids in s
    s = s.replace(anchor_ids, anchor_ids + '\n\t\tstatic CButtonContainer s_MinimalHudLayoutHorizontalButton;\n\t\tstatic CButtonContainer s_MinimalHudLayoutCompactButton;', 1)
marker = 'if(DoButton_CheckBox(&s_MinimalHudCheckpointToggleId, "Checkpoint", g_Config.m_BkwMinimalHudCheckpoint, &CheckpointOpt))\n\t\t\t\tg_Config.m_BkwMinimalHudCheckpoint ^= 1;\n'
if '"Двухстрочный"' not in s[s.find('BKW — Минималистичный HUD'):s.find('BKW — Минималистичный HUD') + 9000]:
    assert marker in s
    ui = '''\t\t\tPageView.HSplitTop(8.0f, nullptr, &PageView);\n\t\t\tCUIRect LayoutLabel, LayoutButtons;\n\t\t\tPageView.HSplitTop(20.0f, &LayoutLabel, &PageView);\n\t\t\tUi()->DoLabel(&LayoutLabel, "Режим HUD:", 12.0f, TEXTALIGN_ML);\n\t\t\tPageView.HSplitTop(30.0f, &LayoutButtons, &PageView);\n\t\t\tCUIRect HorizontalBtn, CompactBtn;\n\t\t\tLayoutButtons.VSplitMid(&HorizontalBtn, &CompactBtn, 6.0f);\n\t\t\tif(DoButton_Menu(&s_MinimalHudLayoutHorizontalButton, "Горизонтальный", g_Config.m_BkwMinimalHudLayout == 0, &HorizontalBtn))\n\t\t\t\tg_Config.m_BkwMinimalHudLayout = 0;\n\t\t\tif(DoButton_Menu(&s_MinimalHudLayoutCompactButton, "Двухстрочный", g_Config.m_BkwMinimalHudLayout == 1, &CompactBtn))\n\t\t\t\tg_Config.m_BkwMinimalHudLayout = 1;\n'''
    s = s.replace(marker, marker + ui, 1)
p.write_text(s, encoding='utf-8')
