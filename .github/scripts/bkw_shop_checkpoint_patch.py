from pathlib import Path
import re


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise RuntimeError(f"missing anchor: {label}")
    return text.replace(old, new, 1)


# Wire the already implemented texture shop into the BKW settings pages.
p = Path("src/game/client/components/menus_settings.cpp")
s = p.read_text(encoding="utf-8")

if "BKW_TAB_SHOP" not in s:
    s = replace_once(
        s,
        "\t\tBKW_TAB_BACKGROUND,\n\t\tBKW_TAB_EXIT,",
        "\t\tBKW_TAB_BACKGROUND,\n\t\tBKW_TAB_SHOP,\n\t\tBKW_TAB_EXIT,",
        "BKW tab enum",
    )

old_tabs = 'const char *apBkwTabs[BKW_TAB_LENGTH] = {"Сохранение", "Чекпоинты", "Игрок", "Кланы", "Часы", "HUD", "Фон", "Выход"};'
new_tabs = 'const char *apBkwTabs[BKW_TAB_LENGTH] = {"Сохранение", "Чекпоинты", "Игрок", "Кланы", "Часы", "HUD", "Фон", "Магазин", "Выход"};'
if old_tabs in s:
    s = s.replace(old_tabs, new_tabs, 1)
elif "\"Магазин\"" not in s:
    raise RuntimeError("missing anchor: BKW tab labels")

if "s_BkwTab == BKW_TAB_SHOP" not in s:
    s = replace_once(
        s,
        "\t\telse if(s_BkwTab == BKW_TAB_EXIT)\n\t\t{",
        "\t\telse if(s_BkwTab == BKW_TAB_SHOP)\n\t\t{\n\t\t\tRenderSettingsAssetsShop(PageView);\n\t\t}\n\t\telse if(s_BkwTab == BKW_TAB_EXIT)\n\t\t{",
        "BKW page dispatch",
    )

p.write_text(s, encoding="utf-8")


# Fix checkpoint coordinates and restore middle-click teleport.
p = Path("src/game/client/components/bestclient/fast_actions.cpp")
s = p.read_text(encoding="utf-8")

# DDNet displays player position in block coordinates, i.e. world coordinates / 32.
s = replace_once(
    s,
    'str_format(aCommand, sizeof(aCommand), "/tpxy %.2f %.2f", Pos.x, Pos.y);',
    'str_format(aCommand, sizeof(aCommand), "/tpxy %.2f %.2f", Pos.x / 32.0f, Pos.y / 32.0f);',
    "checkpoint /tpxy coordinates",
)

# Remove the LMB+RMB combination state that was added by the previous control change.
pattern_state = re.compile(
    r"\n\tstatic bool s_LeftMouseDown = false;\n"
    r"\tstatic bool s_RightMouseDown = false;\n"
    r"\tstatic bool s_TeleportComboTriggered = false;\n"
    r".*?"
    r"\n\tif\(!s_LeftMouseDown \|\| !s_RightMouseDown\)\n"
    r"\t\ts_TeleportComboTriggered = false;\n",
    re.S,
)
s, count = pattern_state.subn("\n", s, count=1)
if count != 1 and "s_TeleportComboTriggered" in s:
    raise RuntimeError("failed to remove LMB+RMB checkpoint state")

# Remove the old combo action itself.
pattern_combo = re.compile(
    r"\n\t\tif\(\(Event\.m_Key == KEY_MOUSE_1 \|\| Event\.m_Key == KEY_MOUSE_2\)"
    r".*?"
    r"\n\t\t\treturn true;\n"
    r"\t\t}\n",
    re.S,
)
s, count = pattern_combo.subn("\n", s, count=1)
if count != 1 and "s_TeleportComboTriggered" in s:
    raise RuntimeError("failed to remove LMB+RMB checkpoint action")

middle_click = """\t\tif(Event.m_Key == KEY_MOUSE_3 && (Event.m_Flags & IInput::FLAG_PRESS) && BkwPracticeModeActive())\n\t\t{\n\t\t\tm_BkwCheckpointHolding = false;\n\t\t\tm_BkwCheckpointHoldStart = 0;\n\t\t\tBkwTeleportCheckpointAtCursor();\n\t\t\treturn true;\n\t\t}\n\n"""
if "Event.m_Key == KEY_MOUSE_3" not in s:
    s = replace_once(
        s,
        "\t\tconst int ActionKey = m_BkwCheckpointMouseButton == 0 ? KEY_MOUSE_1 : KEY_MOUSE_2;\n",
        middle_click + "\t\tconst int ActionKey = m_BkwCheckpointMouseButton == 0 ? KEY_MOUSE_1 : KEY_MOUSE_2;\n",
        "checkpoint action key",
    )

if "s_TeleportComboTriggered" in s or "s_LeftMouseDown" in s or "s_RightMouseDown" in s:
    raise RuntimeError("old checkpoint mouse combo is still present")
if "Pos.x / 32.0f" not in s or "Event.m_Key == KEY_MOUSE_3" not in s:
    raise RuntimeError("checkpoint fixes were not applied")

p.write_text(s, encoding="utf-8")

print("BKW shop/checkpoint patch applied")
