from pathlib import Path

p = Path('src/engine/shared/config_variables_bestclient.h')
s = p.read_text(encoding='utf-8')
line = 'MACRO_CONFIG_INT(BkwMinimalHud, bkw_minimal_hud, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable BKW minimal HUD")\n'
if 'BkwMinimalHud' not in s:
    s += '\n' + line
p.write_text(s, encoding='utf-8')

p = Path('src/game/client/components/hud.h')
s = p.read_text(encoding='utf-8')
anchor = '\tvoid RenderLocalTime(bool ForcePreview = false);\n'
if 'RenderBkwMinimalHud' not in s:
    assert anchor in s
    s = s.replace(anchor, anchor + '\tvoid RenderBkwMinimalHud();\n', 1)
p.write_text(s, encoding='utf-8')

p = Path('src/game/client/components/hud.cpp')
s = p.read_text(encoding='utf-8')
if 'void CHud::RenderBkwMinimalHud()' not in s:
    anchor = 'bool CHud::RebuildFinishPredictionPathData() const\n'
    assert anchor in s
    impl = '''void CHud::RenderBkwMinimalHud()\n{\n\tif(!g_Config.m_BkwMinimalHud || Client()->State() != IClient::STATE_ONLINE)\n\t\treturn;\n\tconst int LocalId = GameClient()->m_Snap.m_LocalClientId;\n\tif(LocalId < 0 || LocalId >= MAX_CLIENTS)\n\t\treturn;\n\tconst float FrameTime = Client()->RenderFrameTime();\n\tconst int Fps = FrameTime > 0.000001f ? (int)std::round(1.0f / FrameTime) : 0;\n\tint Ping = 0;\n\tif(GameClient()->m_Snap.m_apPlayerInfos[LocalId])\n\t\tPing = maximum(0, GameClient()->m_Snap.m_apPlayerInfos[LocalId]->m_Latency);\n\tconst int Team = GameClient()->m_Teams.Team(LocalId);\n\tbool Practice = false;\n\tconst auto &Character = GameClient()->m_Snap.m_aCharacters[LocalId];\n\tif(Character.m_Active && Character.m_HasExtendedData)\n\t\tPractice = (Character.m_ExtendedData.m_Flags & CHARACTERFLAG_PRACTICE_MODE) != 0;\n\tchar aLine[128];\n\tif(Practice)\n\t\tstr_format(aLine, sizeof(aLine), "%d FPS   %d ms   TEAM %d   PRACTICE", Fps, Ping, Team);\n\telse\n\t\tstr_format(aLine, sizeof(aLine), "%d FPS   %d ms   TEAM %d", Fps, Ping, Team);\n\tconst float FontSize = 6.5f;\n\tconst float PaddingX = 6.0f;\n\tconst float PaddingY = 4.0f;\n\tconst float Width = TextRender()->TextWidth(FontSize, aLine) + PaddingX * 2.0f;\n\tconst float Height = FontSize + PaddingY * 2.0f;\n\tconst float X = 5.0f;\n\tconst float Y = 5.0f;\n\tGraphics()->DrawRect(X, Y, Width, Height, ColorRGBA(0.03f, 0.03f, 0.03f, 0.62f), IGraphics::CORNER_ALL, 4.5f);\n\tTextRender()->TextColor(0.94f, 0.96f, 1.0f, 1.0f);\n\tTextRender()->Text(X + PaddingX, Y + PaddingY, FontSize, aLine, -1.0f);\n\tTextRender()->TextColor(TextRender()->DefaultTextColor());\n}\n\n'''
    s = s.replace(anchor, impl + anchor, 1)
if 'RenderBkwMinimalHud();' not in s:
    anchor = '\tRenderLocalTime();\n'
    assert anchor in s
    s = s.replace(anchor, anchor + '\tRenderBkwMinimalHud();\n', 1)
p.write_text(s, encoding='utf-8')

p = Path('src/game/client/components/menus_settings.cpp')
s = p.read_text(encoding='utf-8')
if 'BKW_TAB_HUD' not in s:
    s = s.replace('\t\tBKW_TAB_HOURS,\n\t\tBKW_TAB_BACKGROUND,', '\t\tBKW_TAB_HOURS,\n\t\tBKW_TAB_HUD,\n\t\tBKW_TAB_BACKGROUND,', 1)
    s = s.replace('static int s_MediaBackgroundToggleId;', 'static int s_MinimalHudToggleId;\n\t\tstatic int s_MediaBackgroundToggleId;', 1)
    s = s.replace('{"Сохранение", "Чекпоинты", "Игрок", "Кланы", "Часы", "Фон", "Выход"}', '{"Сохранение", "Чекпоинты", "Игрок", "Кланы", "Часы", "HUD", "Фон", "Выход"}', 1)
if 'BKW — Минималистичный HUD' not in s:
    anchor = '\t\telse if(s_BkwTab == BKW_TAB_BACKGROUND)\n'
    assert anchor in s
    block = '''\t\telse if(s_BkwTab == BKW_TAB_HUD)\n\t\t{\n\t\t\tCUIRect Header;\n\t\t\tPageView.HSplitTop(28.0f, &Header, &PageView);\n\t\t\tUi()->DoLabel(&Header, "BKW — Минималистичный HUD", 22.0f, TEXTALIGN_ML);\n\t\t\tPageView.HSplitTop(8.0f, nullptr, &PageView);\n\t\t\tCUIRect Toggle;\n\t\t\tPageView.HSplitTop(28.0f, &Toggle, &PageView);\n\t\t\tif(DoButton_CheckBox(&s_MinimalHudToggleId, "Минималистичный HUD", g_Config.m_BkwMinimalHud, &Toggle))\n\t\t\t\tg_Config.m_BkwMinimalHud ^= 1;\n\t\t\tPageView.HSplitTop(10.0f, nullptr, &PageView);\n\t\t\tCUIRect Card;\n\t\t\tPageView.HSplitTop(92.0f, &Card, &PageView);\n\t\t\tCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);\n\t\t\tCard.Margin(10.0f, &Card);\n\t\t\tCUIRect L1, L2, L3;\n\t\t\tCard.HSplitTop(24.0f, &L1, &Card);\n\t\t\tCard.HSplitTop(24.0f, &L2, &Card);\n\t\t\tCard.HSplitTop(24.0f, &L3, &Card);\n\t\t\tUi()->DoLabel(&L1, "Компактная панель в левом верхнем углу.", 12.0f, TEXTALIGN_ML);\n\t\t\tUi()->DoLabel(&L2, "Показывает FPS, точный ping и номер team.", 11.0f, TEXTALIGN_ML);\n\t\t\tUi()->DoLabel(&L3, "В режиме /practice дополнительно показывает PRACTICE.", 11.0f, TEXTALIGN_ML);\n\t\t}\n'''
    s = s.replace(anchor, block + anchor, 1)
p.write_text(s, encoding='utf-8')
