from pathlib import Path

# Config
p=Path('src/engine/shared/config_variables_bestclient.h')
s=p.read_text(encoding='utf-8')
anchor='MACRO_CONFIG_INT(BkwMinimalHudLayout, bkw_minimal_hud_layout, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW minimal HUD layout: 0 horizontal, 1 compact two-line")\n'
add=('MACRO_CONFIG_INT(BkwMinimalHudStyle, bkw_minimal_hud_style, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW minimal HUD style: 0 dark, 1 glass, 2 transparent")\n'
     'MACRO_CONFIG_INT(BkwMinimalHudAccent, bkw_minimal_hud_accent, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show BKW minimal HUD accent line")\n')
if 'BkwMinimalHudStyle' not in s:
    assert anchor in s
    s=s.replace(anchor,anchor+add,1)
p.write_text(s,encoding='utf-8')

# Renderer style
p=Path('src/game/client/components/hud.cpp')
s=p.read_text(encoding='utf-8')
old='\tGraphics()->DrawRect(X, Y, Width, Height, ColorRGBA(0.03f, 0.03f, 0.03f, Alpha), IGraphics::CORNER_ALL, 4.5f * Scale);\n\tfloat TextX = X + PaddingX;'
new='''\tconst int Style = std::clamp(g_Config.m_BkwMinimalHudStyle, 0, 2);\n\tif(Style != 2)\n\t{\n\t\tconst ColorRGBA Background = Style == 1 ? ColorRGBA(0.08f, 0.11f, 0.16f, Alpha * 0.72f) : ColorRGBA(0.03f, 0.03f, 0.03f, Alpha);\n\t\tGraphics()->DrawRect(X, Y, Width, Height, Background, IGraphics::CORNER_ALL, 4.5f * Scale);\n\t}\n\tif(g_Config.m_BkwMinimalHudAccent)\n\t\tGraphics()->DrawRect(X, Y, 1.5f * Scale, Height, ColorRGBA(0.35f, 0.72f, 1.0f, 0.95f), IGraphics::CORNER_L, 4.5f * Scale);\n\tfloat TextX = X + PaddingX;'''
if 'm_BkwMinimalHudStyle' not in s:
    assert old in s
    s=s.replace(old,new,1)
p.write_text(s,encoding='utf-8')

# UI IDs and controls
p=Path('src/game/client/components/menus_settings.cpp')
s=p.read_text(encoding='utf-8')
ids='\t\tstatic CButtonContainer s_MinimalHudLayoutCompactButton;'
if 's_aMinimalHudStyleButtons' not in s:
    assert ids in s
    s=s.replace(ids,ids+'\n\t\tstatic CButtonContainer s_aMinimalHudStyleButtons[3];\n\t\tstatic int s_MinimalHudAccentToggleId;',1)

marker='\t\t\t\tif(DoButton_Menu(&s_aMinimalHudAlphaButtons[i], apAlphas[i], g_Config.m_BkwMinimalHudAlpha == aAlphas[i], &B)) g_Config.m_BkwMinimalHudAlpha = aAlphas[i];\n\t\t\t\t}\n'
ui='''\t\t\t\tPageView.HSplitTop(10.0f, nullptr, &PageView);\n\t\t\t\tCUIRect StyleCard;\n\t\t\t\tPageView.HSplitTop(92.0f, &StyleCard, &PageView);\n\t\t\t\tStyleCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);\n\t\t\t\tStyleCard.Margin(10.0f, &StyleCard);\n\t\t\t\tCUIRect StyleLabel, StyleButtons, AccentRow;\n\t\t\t\tStyleCard.HSplitTop(20.0f, &StyleLabel, &StyleCard);\n\t\t\t\tUi()->DoLabel(&StyleLabel, "Стиль", 12.0f, TEXTALIGN_ML);\n\t\t\t\tStyleCard.HSplitTop(30.0f, &StyleButtons, &StyleCard);\n\t\t\t\tconst char *apStyles[3] = {"Тёмный", "Стекло", "Без фона"};\n\t\t\t\tCUIRect StyleRemain = StyleButtons;\n\t\t\t\tfor(int i = 0; i < 3; ++i)\n\t\t\t\t{\n\t\t\t\t\tCUIRect B; StyleRemain.VSplitLeft(StyleButtons.w / 3.0f, &B, &StyleRemain);\n\t\t\t\t\tif(DoButton_Menu(&s_aMinimalHudStyleButtons[i], apStyles[i], g_Config.m_BkwMinimalHudStyle == i, &B)) g_Config.m_BkwMinimalHudStyle = i;\n\t\t\t\t}\n\t\t\t\tStyleCard.HSplitTop(28.0f, &AccentRow, &StyleCard);\n\t\t\t\tif(DoButton_CheckBox(&s_MinimalHudAccentToggleId, "Акцентная линия", g_Config.m_BkwMinimalHudAccent, &AccentRow)) g_Config.m_BkwMinimalHudAccent ^= 1;\n'''
# Insert only inside HUD branch, after alpha loop.
hud=s.find('else if(s_BkwTab == BKW_TAB_HUD)')
bg=s.find('else if(s_BkwTab == BKW_TAB_BACKGROUND)',hud)
seg=s[hud:bg]
if '"Акцентная линия"' not in seg:
    pos=seg.find(marker)
    assert pos>=0
    seg=seg.replace(marker,marker+ui,1)
    s=s[:hud]+seg+s[bg:]
p.write_text(s,encoding='utf-8')
