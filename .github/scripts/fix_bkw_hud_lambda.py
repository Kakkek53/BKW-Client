from pathlib import Path
p=Path('src/game/client/components/hud.cpp')
s=p.read_text(encoding='utf-8')
old='\t\tchar aBuf[48]; str_format(aBuf, sizeof(aBuf), "TIME %s", aTime); (g_Config.m_BkwMinimalHudLayout ? Append2 : Append)(aBuf);\n'
new='\t\tchar aBuf[48];\n\t\tstr_format(aBuf, sizeof(aBuf), "TIME %s", aTime);\n\t\tif(g_Config.m_BkwMinimalHudLayout)\n\t\t\tAppend2(aBuf);\n\t\telse\n\t\t\tAppend(aBuf);\n'
assert s.count(old)==1, s.count(old)
s=s.replace(old,new,1)
p.write_text(s,encoding='utf-8')
