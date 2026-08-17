from pathlib import Path

# Configs
p = Path('src/engine/shared/config_variables_bestclient.h')
s = p.read_text(encoding='utf-8')
anchor = 'MACRO_CONFIG_INT(BkwMinimalHudPbDelta, bkw_minimal_hud_pb_delta, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show PB delta in BKW minimal HUD")\n'
add = ('MACRO_CONFIG_INT(BkwMinimalHudSpeed, bkw_minimal_hud_speed, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show speed in BKW minimal HUD")\n'
       'MACRO_CONFIG_INT(BkwMinimalHudCheckpoint, bkw_minimal_hud_checkpoint, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show checkpoint in BKW minimal HUD")\n')
if 'BkwMinimalHudSpeed' not in s:
    assert anchor in s
    s = s.replace(anchor, anchor + add, 1)
p.write_text(s, encoding='utf-8')

# HUD renderer: inject speed/checkpoint after practice block.
p = Path('src/game/client/components/hud.cpp')
s = p.read_text(encoding='utf-8')
needle = '\tif(g_Config.m_BkwMinimalHudPractice && Practice)\n\t\tAppend("PRACTICE");\n\n'
block = r'''	if(g_Config.m_BkwMinimalHudSpeed && Character.m_Active)
	{
		const CMovementInformation Move = GetMovementInformation(LocalId, g_Config.m_ClDummy);
		const float Speed = length(Move.m_Speed);
		char aBuf[40];
		str_format(aBuf, sizeof(aBuf), "SPD %.1f", Speed);
		Append(aBuf);
	}
	if(g_Config.m_BkwMinimalHudCheckpoint)
	{
		const int Checkpoint = GetCheckpointId();
		if(Checkpoint >= 0)
		{
			char aBuf[32];
			str_format(aBuf, sizeof(aBuf), "CP %d", Checkpoint);
			Append(aBuf);
		}
	}

'''
if 'm_BkwMinimalHudSpeed && Character.m_Active' not in s:
    assert needle in s
    s = s.replace(needle, needle + block, 1)
p.write_text(s, encoding='utf-8')

# Settings UI: IDs and two checkboxes next to race options.
p = Path('src/game/client/components/menus_settings.cpp')
s = p.read_text(encoding='utf-8')
if 's_MinimalHudSpeedToggleId' not in s:
    anchor_ids = 'static int s_MinimalHudPbDeltaToggleId;'
    assert anchor_ids in s
    s = s.replace(anchor_ids, anchor_ids + '\n\t\tstatic int s_MinimalHudSpeedToggleId;\n\t\tstatic int s_MinimalHudCheckpointToggleId;', 1)

if '"Speed"' not in s[s.find('BKW — Минималистичный HUD'):s.find('BKW — Минималистичный HUD') + 7000]:
    marker = 'if(DoButton_CheckBox(&s_MinimalHudPbDeltaToggleId, "PB delta", g_Config.m_BkwMinimalHudPbDelta, &PbDeltaOpt))\n\t\t\t\tg_Config.m_BkwMinimalHudPbDelta ^= 1;\n'
    assert marker in s
    ui = '''\t\t\tCUIRect MovementOptions;\n\t\t\tPageView.HSplitTop(28.0f, &MovementOptions, &PageView);\n\t\t\tCUIRect SpeedOpt, CheckpointOpt;\n\t\t\tMovementOptions.VSplitMid(&SpeedOpt, &CheckpointOpt, 6.0f);\n\t\t\tif(DoButton_CheckBox(&s_MinimalHudSpeedToggleId, "Speed", g_Config.m_BkwMinimalHudSpeed, &SpeedOpt))\n\t\t\t\tg_Config.m_BkwMinimalHudSpeed ^= 1;\n\t\t\tif(DoButton_CheckBox(&s_MinimalHudCheckpointToggleId, "Checkpoint", g_Config.m_BkwMinimalHudCheckpoint, &CheckpointOpt))\n\t\t\t\tg_Config.m_BkwMinimalHudCheckpoint ^= 1;\n'''
    s = s.replace(marker, marker + ui, 1)
p.write_text(s, encoding='utf-8')
