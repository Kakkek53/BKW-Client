#ifndef GAME_CLIENT_COMPONENTS_BKW_TF_MENU_PARSER_INC
#define GAME_CLIENT_COMPONENTS_BKW_TF_MENU_PARSER_INC

#include <base/str.h>

#include <game/client/components/voting.h>

#include <cstddef>

namespace BkwTfMenu
{
inline bool ParseUserName(const CVoting &Voting, char *pBuffer, size_t BufferSize)
{
	if(!pBuffer || BufferSize == 0)
		return false;
	pBuffer[0] = '\0';

	static constexpr const char *pLabel = "Имя пользователя:";
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext)
	{
		const char *pValue = str_find(pOption->m_aDescription, pLabel);
		if(!pValue)
			continue;

		pValue += str_length(pLabel);
		while(*pValue == ' ' || *pValue == '\t')
			++pValue;
		if(*pValue == '\0')
			continue;

		str_copy(pBuffer, pValue, BufferSize);
		int Length = str_length(pBuffer);
		while(Length > 0 && (pBuffer[Length - 1] == ' ' || pBuffer[Length - 1] == '\t' || pBuffer[Length - 1] == '\r' || pBuffer[Length - 1] == '\n'))
			pBuffer[--Length] = '\0';

		if(pBuffer[0] != '\0')
			return true;
	}
	return false;
}
} // namespace BkwTfMenu

#endif
