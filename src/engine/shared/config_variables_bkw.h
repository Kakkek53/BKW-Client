/* Copyright © 2026 BestProject Team */
// This file can be included several times.

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#endif

// BKW media backgrounds
MACRO_CONFIG_INT(BcMenuMediaBackground, bc_menu_media_background, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable media background in the main menu")
MACRO_CONFIG_INT(BcGameMediaBackground, bc_game_media_background, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable media background in game")
MACRO_CONFIG_STR(BcMenuMediaBackgroundPath, bc_menu_media_background_path, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Media background path")
MACRO_CONFIG_INT(BcGameMediaBackgroundOffset, bc_game_media_background_offset, 0, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "In-game media background map offset in percent")
