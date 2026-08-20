#ifndef GAME_CLIENT_COMPONENTS_BKW_BKW_VOTE_THEME_INC
#define GAME_CLIENT_COMPONENTS_BKW_BKW_VOTE_THEME_INC

#include <base/color.h>

namespace BkwVoteTheme
{
inline constexpr float CardRadius = 10.0f;
inline constexpr float ButtonRadius = 6.0f;
inline constexpr float IconStroke = 2.0f;

inline ColorRGBA CardColor()
{
	return ColorRGBA(0.08f, 0.08f, 0.1f, 0.92f);
}

inline ColorRGBA AccentColor()
{
	return ColorRGBA(0.25f, 0.55f, 0.95f, 1.0f);
}
}

#endif
