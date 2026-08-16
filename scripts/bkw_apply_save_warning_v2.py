from pathlib import Path

p = Path(__file__).with_name("bkw_apply_save_warning.py")
src = p.read_text(encoding="utf-8")

src = src.replace(
    '"\\t\\t\\t\\tGameClient()->m_Chat.SendChat(0, aCommand);\\n\\t\\t\\t\\ts_SaveStore.Add(std::move(Entry));\\n\\t\\t\\t\\ts_SaveStore.Save(Storage());"',
    '"\\t\\t\\t\\t\\tGameClient()->m_Chat.SendChat(0, aCommand);\\n\\t\\t\\t\\t\\ts_SaveStore.Add(std::move(Entry));\\n\\t\\t\\t\\t\\ts_SaveStore.Save(Storage());"',
)
src = src.replace(
    '"\\t\\t\\t\\tGameClient()->m_Chat.SendChat(0, aCommand);\\n\\t\\t\\t\\ts_SaveStore.Add(std::move(Entry));\\n\\t\\t\\t\\ts_SaveStore.Save(Storage());\\n\\t\\t\\t\\tBkw::SaveWarningState().MarkSaved();"',
    '"\\t\\t\\t\\t\\tGameClient()->m_Chat.SendChat(0, aCommand);\\n\\t\\t\\t\\t\\ts_SaveStore.Add(std::move(Entry));\\n\\t\\t\\t\\t\\ts_SaveStore.Save(Storage());\\n\\t\\t\\t\\t\\tBkw::SaveWarningState().MarkSaved();"',
)
src = src.replace(
    '"\\t\\t\\t\\t\\t\\tGameClient()->m_Chat.SendChat(0, aCommand);\\n\\t\\t\\t\\t\\t\\ts_SaveStore.Remove(Index);"',
    '"\\t\\t\\t\\t\\t\\t\\tGameClient()->m_Chat.SendChat(0, aCommand);\\n\\t\\t\\t\\t\\t\\t\\ts_SaveStore.Remove(Index);"',
)
src = src.replace(
    '"\\t\\t\\t\\t\\t\\tGameClient()->m_Chat.SendChat(0, aCommand);\\n\\t\\t\\t\\t\\t\\tBkw::SaveWarningState().MarkLoaded();\\n\\t\\t\\t\\t\\t\\ts_SaveStore.Remove(Index);"',
    '"\\t\\t\\t\\t\\t\\t\\tGameClient()->m_Chat.SendChat(0, aCommand);\\n\\t\\t\\t\\t\\t\\t\\tBkw::SaveWarningState().MarkLoaded();\\n\\t\\t\\t\\t\\t\\t\\ts_SaveStore.Remove(Index);"',
)

exec(compile(src, str(p), "exec"), {"__name__": "__main__", "__file__": str(p)})
