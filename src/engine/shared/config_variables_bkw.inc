// BKW-specific client settings. Included under ConfigDomain::BESTCLIENT.
MACRO_CONFIG_INT(BkwTfMenu, bkw_tf_menu, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable the redesigned TeeFusion menu")
MACRO_CONFIG_INT(BkwDdnetVoteMenu, bkw_ddnet_vote_menu, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable the redesigned DDNet race vote menu")
MACRO_CONFIG_STR(BkwDdnetFavoriteMaps, bkw_ddnet_favorite_maps, 4096, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Favorite DDNet maps for the redesigned vote menu")
MACRO_CONFIG_INT(BkwTfDuelHud, bkw_tf_duel_hud, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable the custom TeeFusion duel scoreboard")
MACRO_CONFIG_INT(BkwTfDuelHudStyle, bkw_tf_duel_hud_style, 0, 0, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "TeeFusion duel scoreboard style: 0 glass, 1 minimal, 2 neon, 3 classic")
MACRO_CONFIG_INT(BkwTfDuelHideBroadcast, bkw_tf_duel_hide_broadcast, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide TeeFusion duel broadcast spam when the custom scoreboard is active")
