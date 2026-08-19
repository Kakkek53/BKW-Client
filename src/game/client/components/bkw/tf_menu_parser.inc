#ifndef GAME_CLIENT_COMPONENTS_BKW_TF_MENU_PARSER_INC
#define GAME_CLIENT_COMPONENTS_BKW_TF_MENU_PARSER_INC

#include <base/str.h>

#include <game/client/components/bkw/tf_menu_state.inc>
#include <game/client/components/voting.h>

#include <cstddef>

namespace BkwTfMenu
{
inline bool ParseUserName(const CVoting &Voting, char *pBuffer, size_t BufferSize)
{
	if(!pBuffer || BufferSize == 0)
		return false;
	pBuffer[0] = '\0';

	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext)
	{
		// TeeFusion mixes Cyrillic and Latin lookalikes in labels, e.g.
		// "Имя пoльзoвaтeля". Match only the stable prefix instead of the
		// full word "пользователя" so those homoglyphs do not matter.
		const char *pLabel = str_utf8_find_nocase(pOption->m_aDescription, "Имя");
		if(!pLabel)
			pLabel = str_utf8_find_nocase(pOption->m_aDescription, "Username");
		if(!pLabel)
			continue;

		const char *pValue = str_find(pLabel, ":");
		if(!pValue)
			continue;
		++pValue;

		while(*pValue == ' ' || *pValue == '\t' || *pValue == '-' || *pValue == '|')
			++pValue;
		if(*pValue == '\0')
			continue;

		str_copy(pBuffer, pValue, BufferSize);
		int Length = str_length(pBuffer);
		while(Length > 0 && (pBuffer[Length - 1] == ' ' || pBuffer[Length - 1] == '\t' || pBuffer[Length - 1] == '\r' || pBuffer[Length - 1] == '\n' || pBuffer[Length - 1] == '|'))
			pBuffer[--Length] = '\0';

		if(pBuffer[0] != '\0')
			return true;
	}
	return false;
}
} // namespace BkwTfMenu

#endif
