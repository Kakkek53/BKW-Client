from pathlib import Path

path = Path("src/game/client/components/menus_settings.cpp")
text = path.read_text(encoding="utf-8")


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    text = text.replace(old, new, 1)


old_enum = '''\tenum
\t{
\t\tBKW_TAB_SAVES = 0,
\t\tBKW_TAB_CHECKPOINTS,
\t\tBKW_TAB_PLAYER,
\t\tBKW_TAB_CLANS,
\t\tBKW_TAB_HOURS,
\t\tBKW_TAB_HUD,
\t\tBKW_TAB_BACKGROUND,
\t\tBKW_TAB_SHOP,
\t\tBKW_TAB_EXIT,
\t\tBKW_TAB_LENGTH,
\t};
\tstatic int s_BkwTab = BKW_TAB_SAVES;
'''
new_enum = '''\tenum
\t{
\t\tBKW_TAB_MAIN = 0,
\t\tBKW_TAB_HOURS,
\t\tBKW_TAB_PERSONALIZATION,
\t\tBKW_TAB_SHOP,
\t\tBKW_TAB_LENGTH,
\t};
\tstatic int s_BkwTab = BKW_TAB_MAIN;
'''
replace_once(old_enum, new_enum, "BKW enum")

old_statics = '''\t\tstatic bool s_SaveWarningInputsInitialized = false;
\t\tstatic CButtonContainer s_aBkwTabButtons[BKW_TAB_LENGTH];
'''
new_statics = '''\t\tstatic bool s_SaveWarningInputsInitialized = false;
\t\tstatic CButtonContainer s_aBkwTabButtons[BKW_TAB_LENGTH];
\t\tstatic CButtonContainer s_BkwTabPrevButton;
\t\tstatic CButtonContainer s_BkwTabNextButton;
\t\tstatic int s_BkwFirstVisibleTab = 0;
'''
replace_once(old_statics, new_statics, "BKW tab statics")

old_tabs = '''\t\ts_BkwTab = std::clamp(s_BkwTab, 0, BKW_TAB_LENGTH - 1);
\t\tCUIRect BkwTabBar;
\t\tPageView.HSplitTop(30.0f, &BkwTabBar, &PageView);
\t\tconst char *apBkwTabs[BKW_TAB_LENGTH] = {"Сохранение", "Чекпоинты", "Игрок", "Кланы", "Часы", "HUD", "Фон", "Магазин", "Выход"};
\t\tCUIRect RemainingTabs = BkwTabBar;
\t\tconst float TabWidth = BkwTabBar.w / (float)BKW_TAB_LENGTH;
\t\tfor(int i = 0; i < BKW_TAB_LENGTH; ++i)
\t\t{
\t\t\tCUIRect TabButton;
\t\t\tRemainingTabs.VSplitLeft(TabWidth, &TabButton, &RemainingTabs);
\t\t\tconst int Corners = i == 0 ? (IGraphics::CORNER_TL | IGraphics::CORNER_BL) : (i == BKW_TAB_LENGTH - 1 ? (IGraphics::CORNER_TR | IGraphics::CORNER_BR) : IGraphics::CORNER_NONE);
\t\t\tif(DoButton_MenuTab(&s_aBkwTabButtons[i], apBkwTabs[i], s_BkwTab == i, &TabButton, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
\t\t\t\ts_BkwTab = i;
\t\t}
\t\tPageView.HSplitTop(14.0f, nullptr, &PageView);
'''
new_tabs = '''\t\ts_BkwTab = std::clamp(s_BkwTab, 0, BKW_TAB_LENGTH - 1);
\t\tCUIRect BkwTabBar;
\t\tPageView.HSplitTop(8.0f, nullptr, &PageView);
\t\tPageView.HSplitTop(24.0f, &BkwTabBar, &PageView);
\t\tconst char *apBkwTabs[BKW_TAB_LENGTH] = {"Основное", "Часы", "Персонализация", "Магазин"};

\t\t// BestClient-style segmented submenu. On narrow layouts the strip becomes
\t\t// pageable instead of squeezing labels until they are unreadable.
\t\tconstexpr float MinBkwTabWidth = 128.0f;
\t\tconstexpr float BkwPagerWidth = 28.0f;
\t\tint VisibleTabCount = std::clamp((int)(BkwTabBar.w / MinBkwTabWidth), 1, BKW_TAB_LENGTH);
\t\tbool TabsScrollable = VisibleTabCount < BKW_TAB_LENGTH;
\t\tif(TabsScrollable)
\t\t\tVisibleTabCount = std::clamp((int)((BkwTabBar.w - BkwPagerWidth * 2.0f) / MinBkwTabWidth), 1, BKW_TAB_LENGTH);

\t\tconst int MaxFirstVisibleTab = maximum(0, BKW_TAB_LENGTH - VisibleTabCount);
\t\ts_BkwFirstVisibleTab = std::clamp(s_BkwFirstVisibleTab, 0, MaxFirstVisibleTab);

\t\tCUIRect VisibleTabs = BkwTabBar;
\t\tif(TabsScrollable)
\t\t{
\t\t\tCUIRect PrevButton, NextButton;
\t\t\tVisibleTabs.VSplitLeft(BkwPagerWidth, &PrevButton, &VisibleTabs);
\t\t\tVisibleTabs.VSplitRight(BkwPagerWidth, &VisibleTabs, &NextButton);
\t\t\tif(DoButton_Menu(&s_BkwTabPrevButton, "<", 0, &PrevButton) && s_BkwFirstVisibleTab > 0)
\t\t\t\t--s_BkwFirstVisibleTab;
\t\t\tif(DoButton_Menu(&s_BkwTabNextButton, ">", 0, &NextButton) && s_BkwFirstVisibleTab < MaxFirstVisibleTab)
\t\t\t\t++s_BkwFirstVisibleTab;

\t\t\tif(Ui()->MouseHovered(&BkwTabBar))
\t\t\t{
\t\t\t\tif(Ui()->ConsumeHotkey(CUi::HOTKEY_SCROLL_UP) && s_BkwFirstVisibleTab > 0)
\t\t\t\t\t--s_BkwFirstVisibleTab;
\t\t\t\tif(Ui()->ConsumeHotkey(CUi::HOTKEY_SCROLL_DOWN) && s_BkwFirstVisibleTab < MaxFirstVisibleTab)
\t\t\t\t\t++s_BkwFirstVisibleTab;
\t\t\t}
\t\t}

\t\tconst float TabWidth = VisibleTabs.w / (float)VisibleTabCount;
\t\tfor(int VisibleIndex = 0; VisibleIndex < VisibleTabCount; ++VisibleIndex)
\t\t{
\t\t\tconst int Tab = s_BkwFirstVisibleTab + VisibleIndex;
\t\t\tCUIRect TabButton;
\t\t\tVisibleTabs.VSplitLeft(TabWidth, &TabButton, &VisibleTabs);
\t\t\tconst int Corners = VisibleIndex == 0 ? IGraphics::CORNER_L : (VisibleIndex == VisibleTabCount - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE);
\t\t\tif(DoButton_MenuTab(&s_aBkwTabButtons[Tab], apBkwTabs[Tab], s_BkwTab == Tab, &TabButton, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
\t\t\t\ts_BkwTab = Tab;
\t\t}
\t\tPageView.HSplitTop(10.0f, nullptr, &PageView);
'''
replace_once(old_tabs, new_tabs, "BKW tab bar")

# Map the old individual pages into the four new groups.
for old, new in (
    ("BKW_TAB_SAVES", "BKW_TAB_MAIN"),
    ("BKW_TAB_CHECKPOINTS", "BKW_TAB_MAIN"),
    ("BKW_TAB_PLAYER", "BKW_TAB_MAIN"),
    ("BKW_TAB_CLANS", "BKW_TAB_MAIN"),
    ("BKW_TAB_HUD", "BKW_TAB_PERSONALIZATION"),
    ("BKW_TAB_BACKGROUND", "BKW_TAB_PERSONALIZATION"),
    ("BKW_TAB_EXIT", "BKW_TAB_MAIN"),
):
    text = text.replace(old, new)

# The grouped sections must all render sequentially, so they can no longer be an else-if chain.
text = text.replace("\t\telse if(s_BkwTab ==", "\t\tif(s_BkwTab ==")

# Add visual breathing room between the sections inside the combined pages and simplify headings.
section_replacements = [
    (
        '''\t\tif(s_BkwTab == BKW_TAB_MAIN)\n\t\t{\n\t\t\tCUIRect Header;\n\t\t\tPageView.HSplitTop(28.0f, &Header, &PageView);\n\t\t\tUi()->DoLabel(&Header, "BKW — Чекпоинты", 22.0f, TEXTALIGN_ML);''',
        '''\t\tif(s_BkwTab == BKW_TAB_MAIN)\n\t\t{\n\t\t\tPageView.HSplitTop(24.0f, nullptr, &PageView);\n\t\t\tCUIRect Header;\n\t\t\tPageView.HSplitTop(28.0f, &Header, &PageView);\n\t\t\tUi()->DoLabel(&Header, "Чекпоинты", 22.0f, TEXTALIGN_ML);''',
        "checkpoints section",
    ),
    (
        '''\t\tif(s_BkwTab == BKW_TAB_MAIN)\n\t\t{\n\t\t\tCUIRect Header;\n\t\t\tPageView.HSplitTop(28.0f, &Header, &PageView);\n\t\t\tUi()->DoLabel(&Header, "BKW — Инф. о игроке (рейс)", 22.0f, TEXTALIGN_ML);''',
        '''\t\tif(s_BkwTab == BKW_TAB_MAIN)\n\t\t{\n\t\t\tPageView.HSplitTop(24.0f, nullptr, &PageView);\n\t\t\tCUIRect Header;\n\t\t\tPageView.HSplitTop(28.0f, &Header, &PageView);\n\t\t\tUi()->DoLabel(&Header, "Информация об игроке", 22.0f, TEXTALIGN_ML);''',
        "player section",
    ),
    (
        '''\t\tif(s_BkwTab == BKW_TAB_MAIN)\n\t\t{\n\t\t\tCUIRect Header;\n\t\t\tPageView.HSplitTop(28.0f, &Header, &PageView);\n\t\t\tUi()->DoLabel(&Header, "BKW — История кланов", 22.0f, TEXTALIGN_ML);''',
        '''\t\tif(s_BkwTab == BKW_TAB_MAIN)\n\t\t{\n\t\t\tPageView.HSplitTop(24.0f, nullptr, &PageView);\n\t\t\tCUIRect Header;\n\t\t\tPageView.HSplitTop(28.0f, &Header, &PageView);\n\t\t\tUi()->DoLabel(&Header, "История кланов", 22.0f, TEXTALIGN_ML);''',
        "clans section",
    ),
    (
        '''\t\tif(s_BkwTab == BKW_TAB_MAIN)\n\t\t{\n\t\t\tCUIRect Header, Toggle;\n\t\t\tPageView.HSplitTop(28.0f, &Header, &PageView);\n\t\t\tUi()->DoLabel(&Header, "BKW — Предупреждение о выходе", 22.0f, TEXTALIGN_ML);''',
        '''\t\tif(s_BkwTab == BKW_TAB_MAIN)\n\t\t{\n\t\t\tPageView.HSplitTop(24.0f, nullptr, &PageView);\n\t\t\tCUIRect Header, Toggle;\n\t\t\tPageView.HSplitTop(28.0f, &Header, &PageView);\n\t\t\tUi()->DoLabel(&Header, "Предупреждение о выходе", 22.0f, TEXTALIGN_ML);''',
        "exit warning section",
    ),
    (
        '''\t\tif(s_BkwTab == BKW_TAB_PERSONALIZATION)\n\t\t{\n\t\t\tif(!s_MediaBackgroundPathInitialized)''',
        '''\t\tif(s_BkwTab == BKW_TAB_PERSONALIZATION)\n\t\t{\n\t\t\tPageView.HSplitTop(24.0f, nullptr, &PageView);\n\t\t\tif(!s_MediaBackgroundPathInitialized)''',
        "background spacing",
    ),
]
for old, new, label in section_replacements:
    replace_once(old, new, label)

text = text.replace('Ui()->DoLabel(&Header, "BKW — Сохранение", 22.0f, TEXTALIGN_ML);', 'Ui()->DoLabel(&Header, "Сохранение", 22.0f, TEXTALIGN_ML);')
text = text.replace('Ui()->DoLabel(&Header, "BKW — Часы", 22.0f, TEXTALIGN_ML);', 'Ui()->DoLabel(&Header, "Часы", 22.0f, TEXTALIGN_ML);')
text = text.replace('Ui()->DoLabel(&Header, "BKW — Минималистичный HUD", 22.0f, TEXTALIGN_ML);', 'Ui()->DoLabel(&Header, "Минималистичный HUD", 22.0f, TEXTALIGN_ML);')
text = text.replace('Ui()->DoLabel(&Header, "BKW — Фон", 22.0f, TEXTALIGN_ML);', 'Ui()->DoLabel(&Header, "Фон", 22.0f, TEXTALIGN_ML);')

# Keep the UI text in sync with the already changed middle-mouse checkpoint control.
replace_once(
    'Ui()->DoLabel(&Help2, "ЛКМ + ПКМ одновременно: /tpxy к последнему чекпоинту.", 11.0f, TEXTALIGN_ML);',
    'Ui()->DoLabel(&Help2, "Нажатие колёсика: /tpxy к последнему чекпоинту.", 11.0f, TEXTALIGN_ML);',
    "checkpoint middle mouse hint",
)

old_scroll = '''\t\t\tif(Page == SETTINGS_CREDITS)\n\t\t\t\tVirtualHeightBoost = s_BkwTab == BKW_TAB_MAIN ? 2350.0f : 360.0f;'''
new_scroll = '''\t\t\tif(Page == SETTINGS_CREDITS)\n\t\t\t{\n\t\t\t\tif(s_BkwTab == BKW_TAB_MAIN)\n\t\t\t\t\tVirtualHeightBoost = 3200.0f;\n\t\t\t\telse if(s_BkwTab == BKW_TAB_PERSONALIZATION)\n\t\t\t\t\tVirtualHeightBoost = 1200.0f;\n\t\t\t\telse\n\t\t\t\t\tVirtualHeightBoost = 420.0f;\n\t\t\t}'''
replace_once(old_scroll, new_scroll, "BKW scroll heights")

# Structural sanity checks.
for forbidden in ("BKW_TAB_SAVES", "BKW_TAB_CHECKPOINTS", "BKW_TAB_PLAYER", "BKW_TAB_CLANS", "BKW_TAB_HUD", "BKW_TAB_BACKGROUND", "BKW_TAB_EXIT"):
    if forbidden in text:
        raise RuntimeError(f"stale tab id remains: {forbidden}")
if text.count("if(s_BkwTab == BKW_TAB_MAIN)") != 5:
    raise RuntimeError("expected five sections in Основное")
if text.count("if(s_BkwTab == BKW_TAB_PERSONALIZATION)") != 2:
    raise RuntimeError("expected HUD and background in Персонализация")
if '"Основное", "Часы", "Персонализация", "Магазин"' not in text:
    raise RuntimeError("new BKW tab names missing")

path.write_text(text, encoding="utf-8")
