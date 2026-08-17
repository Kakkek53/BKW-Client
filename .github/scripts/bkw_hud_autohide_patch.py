from pathlib import Path

p=Path('src/engine/shared/config_variables_bestclient.h')
s=p.read_text(encoding='utf-8')
anchor='MACRO_CONFIG_INT(BkwMinimalHudAccent, bkw_minimal_hud_accent, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show BKW minimal HUD accent line")\n'
add=('MACRO_CONFIG_INT(BkwMinimalHudHideScoreboard, bkw_minimal_hud_hide_scoreboard, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide BKW minimal HUD while scoreboard is open")\n'
     'MACRO_CONFIG_INT(BkwMinimalHudHideMenus, bkw_minimal_hud_hide_menus, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide BKW minimal HUD while game menus are open")\n')
if 'BkwMinimalHudHideScoreboard' not in s:
    assert anchor in s
    s=s.replace(anchor,anchor+add,1)
p.write_text(s,encoding='utf-8')

p=Path('src/game/client/components/hud.cpp')
s=p.read_text(encoding='utf-8')
old='''\tif(!g_Config.m_BkwMinimalHud || Client()->State() != IClient::STATE_ONLINE)\n\t\treturn;\n\tconst int LocalId = GameClient()->m_Snap.m_LocalClientId;'''
new='''\tif(!g_Config.m_BkwMinimalHud || Client()->State() != IClient::STATE_ONLINE)\n\t\treturn;\n\tif(g_Config.m_BkwMinimalHudHideScoreboard && GameClient()->m_Scoreboard.IsActive())\n\t\treturn;\n\tif(g_Config.m_BkwMinimalHudHideMenus && GameClient()->m_Menus.IsActive())\n\t\treturn;\n\tconst int LocalId = GameClient()->m_Snap.m_LocalClientId;'''
if 'm_BkwMinimalHudHideScoreboard' not in s:
    assert old in s
    s=s.replace(old,new,1)
p.write_text(s,encoding='utf-8')

p=Path('src/game/client/components/menus_settings.cpp')
s=p.read_text(encoding='utf-8')
ids='\t\tstatic int s_MinimalHudAccentToggleId;'
if 's_MinimalHudHideScoreboardToggleId' not in s:
    assert ids in s
    s=s.replace(ids,ids+'\n\t\tstatic int s_MinimalHudHideScoreboardToggleId;\n\t\tstatic int s_MinimalHudHideMenusToggleId;',1)
# Add auto-hide card after style card controls.
needle='\t\t\t\tif(DoButton_CheckBox(&s_MinimalHudAccentToggleId, "Акцентная линия", g_Config.m_BkwMinimalHudAccent, &AccentRow)) g_Config.m_BkwMinimalHudAccent ^= 1;\n'
ui='''\n\t\t\t\tPageView.HSplitTop(10.0f, nullptr, &PageView);\n\t\t\t\tCUIRect AutoHideCard;\n\t\t\t\tPageView.HSplitTop(88.0f, &AutoHideCard, &PageView);\n\t\t\t\tAutoHideCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);\n\t\t\t\tAutoHideCard.Margin(10.0f, &AutoHideCard);\n\t\t\t\tCUIRect AutoTitle, AutoRow1, AutoRow2;\n\t\t\t\tAutoHideCard.HSplitTop(20.0f, &AutoTitle, &AutoHideCard);\n\t\t\t\tUi()->DoLabel(&AutoTitle, "Автоскрытие", 12.0f, TEXTALIGN_ML);\n\t\t\t\tAutoHideCard.HSplitTop(28.0f, &AutoRow1, &AutoHideCard);\n\t\t\t\tAutoHideCard.HSplitTop(28.0f, &AutoRow2, &AutoHideCard);\n\t\t\t\tif(DoButton_CheckBox(&s_MinimalHudHideScoreboardToggleId, "Скрывать при scoreboard", g_Config.m_BkwMinimalHudHideScoreboard, &AutoRow1)) g_Config.m_BkwMinimalHudHideScoreboard ^= 1;\n\t\t\t\tif(DoButton_CheckBox(&s_MinimalHudHideMenusToggleId, "Скрывать в меню", g_Config.m_BkwMinimalHudHideMenus, &AutoRow2)) g_Config.m_BkwMinimalHudHideMenus ^= 1;\n'''
if '"Скрывать при scoreboard"' not in s:
    assert needle in s
    s=s.replace(needle,needle+ui,1)
p.write_text(s,encoding='utf-8')
