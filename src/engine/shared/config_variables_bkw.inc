// BKW-specific client settings. Included under ConfigDomain::BESTCLIENT.
MACRO_CONFIG_INT(BkwTfMenu, bkw_tf_menu, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable the redesigned TeeFusion menu")
MACRO_CONFIG_INT(BkwDdnetVoteMenu, bkw_ddnet_vote_menu, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable the redesigned DDNet race vote menu")
MACRO_CONFIG_STR(BkwDdnetFavoriteMaps, bkw_ddnet_favorite_maps, 4096, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Favorite DDNet maps for the redesigned vote menu")
