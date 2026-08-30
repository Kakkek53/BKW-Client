// BKW-specific client settings. Included under ConfigDomain::BESTCLIENT.
MACRO_CONFIG_INT(BkwTfMenu, bkw_tf_menu, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable the redesigned TeeFusion menu")
MACRO_CONFIG_INT(BkwTfMenuStyle, bkw_tf_menu_style, 0, 0, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion menu style: 0 standard, 1 liquid glass, 2 neon, 3 minimal")
MACRO_CONFIG_INT(BkwTfMenuScale, bkw_tf_menu_scale, 100, 75, 125, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion menu window scale in percent")
MACRO_CONFIG_INT(BkwTfMenuOpacity, bkw_tf_menu_opacity, 96, 45, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion menu window opacity in percent")
MACRO_CONFIG_INT(BkwTfMenuRounding, bkw_tf_menu_rounding, 16, 0, 28, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion menu corner radius")
MACRO_CONFIG_INT(BkwDdnetVoteMenu, bkw_ddnet_vote_menu, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable the redesigned DDNet race vote menu")
MACRO_CONFIG_INT(BkwDdnetMenuStyle, bkw_ddnet_menu_style, 0, 0, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "DDNet vote menu style: 0 standard, 1 liquid glass, 2 neon, 3 minimal")
MACRO_CONFIG_INT(BkwDdnetMenuScale, bkw_ddnet_menu_scale, 100, 75, 125, CFGFLAG_CLIENT | CFGFLAG_SAVE, "DDNet vote menu window scale in percent")
MACRO_CONFIG_INT(BkwDdnetMenuOpacity, bkw_ddnet_menu_opacity, 96, 45, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "DDNet vote menu window opacity in percent")
MACRO_CONFIG_INT(BkwDdnetMenuRounding, bkw_ddnet_menu_rounding, 16, 0, 28, CFGFLAG_CLIENT | CFGFLAG_SAVE, "DDNet vote menu corner radius")
MACRO_CONFIG_STR(BkwDdnetFavoriteMaps, bkw_ddnet_favorite_maps, 4096, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Favorite DDNet maps for the redesigned vote menu")
MACRO_CONFIG_INT(BkwEgoMapDetails, bkw_ego_map_details, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show EGO map details buttons in the standard vote menu")
MACRO_CONFIG_INT(BkwTfDuelHud, bkw_tf_duel_hud, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable the custom TeeFusion duel scoreboard")
MACRO_CONFIG_INT(BkwTfDuelHudStyle, bkw_tf_duel_hud_style, 0, 0, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion duel scoreboard style: 0 glass, 1 minimal, 2 neon, 3 classic")
MACRO_CONFIG_INT(BkwTfDuelHideBroadcast, bkw_tf_duel_hide_broadcast, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide TeeFusion duel broadcast spam when the custom scoreboard is active")
MACRO_CONFIG_INT(BkwTfDuelHudPosition, bkw_tf_duel_hud_position, 1, 0, 8, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion duel HUD anchor: 0 TL, 1 TC, 2 TR, 3 CL, 4 C, 5 CR, 6 BL, 7 BC, 8 BR")
MACRO_CONFIG_INT(BkwTfDuelHudScale, bkw_tf_duel_hud_scale, 85, 60, 140, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion duel HUD scale in percent")
MACRO_CONFIG_INT(BkwTfDuelHudOpacity, bkw_tf_duel_hud_opacity, 92, 20, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion duel HUD opacity in percent")
MACRO_CONFIG_INT(BkwTfDuelHudRounding, bkw_tf_duel_hud_rounding, 12, 0, 24, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion duel HUD corner radius")

// BKW Local Duel. Score changes are tracked locally from freeze state; optional chat messages use the normal public say command.
MACRO_CONFIG_INT(BkwLocalDuel, bkw_local_duel, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable local freeze duel tracking")
MACRO_CONFIG_STR(BkwLocalDuelOpponent, bkw_local_duel_opponent, 64, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Opponent name for BKW local duel")
MACRO_CONFIG_INT(BkwLocalDuelLimit, bkw_local_duel_limit, 5, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Local duel score limit, 0 for infinite fun duel")
MACRO_CONFIG_INT(BkwLocalDuelChatScore, bkw_local_duel_chat_score, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Send local duel score changes to public chat")
MACRO_CONFIG_INT(BkwLocalDuelSelectKey, bkw_local_duel_select_key, 27, 0, 511, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Key used to start local duel opponent selection")
MACRO_CONFIG_INT(BkwLocalDuelSelectModifier, bkw_local_duel_select_modifier, 0, 0, 31, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Modifier mask used for local duel opponent selection")
MACRO_CONFIG_INT(BkwLocalDuelHud, bkw_local_duel_hud, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show BKW local duel HUD")
MACRO_CONFIG_INT(BkwLocalDuelHudStyle, bkw_local_duel_hud_style, 0, 0, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Local duel HUD style: 0 glass, 1 minimal, 2 neon, 3 classic")
MACRO_CONFIG_INT(BkwLocalDuelHudPosition, bkw_local_duel_hud_position, 1, 0, 8, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Local duel HUD anchor: 0 TL, 1 TC, 2 TR, 3 CL, 4 C, 5 CR, 6 BL, 7 BC, 8 BR")
MACRO_CONFIG_INT(BkwLocalDuelHudScale, bkw_local_duel_hud_scale, 85, 60, 140, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Local duel HUD scale in percent")
MACRO_CONFIG_INT(BkwLocalDuelHudOpacity, bkw_local_duel_hud_opacity, 92, 20, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Local duel HUD opacity in percent")
MACRO_CONFIG_INT(BkwLocalDuelHudRounding, bkw_local_duel_hud_rounding, 12, 0, 24, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Local duel HUD corner radius")

// BKW Mini Map.
MACRO_CONFIG_INT(BkwMiniMap, bkw_mini_map, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show BKW minimap in the top-right corner")
MACRO_CONFIG_INT(BkwMiniMapSize, bkw_mini_map_size, 180, 120, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW minimap size")
MACRO_CONFIG_INT(BkwMiniMapOpacity, bkw_mini_map_opacity, 90, 20, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW minimap opacity")
MACRO_CONFIG_INT(BkwMiniMapAutoZoom, bkw_mini_map_auto_zoom, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Automatically zoom the minimap out until a non-frozen player is visible")
MACRO_CONFIG_INT(BkwMiniMapRealDesign, bkw_mini_map_real_design, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render the real map design layers in BKW minimap and full map")
MACRO_CONFIG_INT(BkwMiniMapFullKey, bkw_mini_map_full_key, 0, 0, 511, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Key used to toggle the full BKW map")
MACRO_CONFIG_INT(BkwMiniMapFullModifier, bkw_mini_map_full_modifier, 0, 0, 31, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Modifier mask used to toggle the full BKW map")
MACRO_CONFIG_INT(BkwMiniMapFullZoom, bkw_mini_map_full_zoom, 180, 100, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Zoom of the full BKW map in percent")

// BKW Feature Pack: optimizer.
MACRO_CONFIG_INT(BkwOptimizer, bkw_optimizer, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable BKW optimization profiles")
MACRO_CONFIG_INT(BkwOptimizerScope, bkw_optimizer_scope, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Optimization scope: 0 BKW features only, 1 whole client")
MACRO_CONFIG_INT(BkwOptimizerPreset, bkw_optimizer_preset, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Optimization preset: 0 balanced, 1 max FPS, 2 low latency")

// BKW Feature Pack: TipoCheat fun tools. They use the normal client input/rate limits.
MACRO_CONFIG_INT(BkwTipoCheat, bkw_tipo_cheat, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable TipoCheat fun tools")
MACRO_CONFIG_INT(BkwTipoCheatSpin, bkw_tipo_cheat_spin, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Automatically rotate the aim and lock manual mouse rotation")
MACRO_CONFIG_INT(BkwTipoCheatSpinSpeed, bkw_tipo_cheat_spin_speed, 45, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TipoCheat spin speed")
MACRO_CONFIG_INT(BkwTipoCheatEmoteDance, bkw_tipo_cheat_emote_dance, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Cycle through emoticons with a safe interval")
MACRO_CONFIG_INT(BkwTipoCheatEmoteInterval, bkw_tipo_cheat_emote_interval, 2200, 1200, 10000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Emote dance interval in milliseconds")

// Hook target indicator.
MACRO_CONFIG_INT(BkwHookTargetIndicator, bkw_hook_target_indicator, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show a subtle marker when the current hook path reaches a player")
MACRO_CONFIG_COL(BkwHookTargetColor, bkw_hook_target_color, 0x5533FF66U, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Hook target indicator color")
MACRO_CONFIG_INT(BkwHookTargetSize, bkw_hook_target_size, 58, 32, 96, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hook target indicator square size")

// Adaptive AntiPing. This tunes DDNet's existing prediction controls instead of replacing physics.
MACRO_CONFIG_INT(BkwAdaptiveAntiPing, bkw_adaptive_antiping, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable adaptive AntiPing tuning")
MACRO_CONFIG_INT(BkwAdaptiveAntiPingStrength, bkw_adaptive_antiping_strength, 55, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Adaptive AntiPing strength")
MACRO_CONFIG_INT(BkwAdaptiveAntiPingStatus, bkw_adaptive_antiping_status, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show adaptive AntiPing status in BKW settings")

// MP4 -> standard DDNet .map converter. Uses atlas images, quads and a STEP color envelope.
MACRO_CONFIG_STR(BkwVideoMapOutput, bkw_video_map_output, 128, "bkw_video.map", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Output map filename for the BKW video map converter")
MACRO_CONFIG_INT(BkwVideoMapFps, bkw_video_map_fps, 15, 5, 30, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Requested FPS for MP4 to map conversion")
MACRO_CONFIG_INT(BkwVideoMapDimension, bkw_video_map_dimension, 256, 128, 512, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum decoded video frame dimension for MP4 to map conversion")
MACRO_CONFIG_INT(BkwVideoMapMaxSeconds, bkw_video_map_max_seconds, 15, 1, 60, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum video duration in seconds for MP4 to map conversion")
