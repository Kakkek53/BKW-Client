#ifndef GAME_CLIENT_COMPONENTS_BKW_EGO_VOTE_REAL_FORMAT_FIX_INC
#define GAME_CLIENT_COMPONENTS_BKW_EGO_VOTE_REAL_FORMAT_FIX_INC

#include <base/str.h>

#include <game/client/components/bkw/ego_vote_overlay.inc>
#include <game/client/ui_listbox.h>

#include <algorithm>
#include <string>

// Parser for the actual Eternal Gores vote layout observed on live servers:
//   ╭──┤ Maps (Insane: 251)
//   │ • 012 | ★★☆ | S | 144 ⚐ | 11:49 ◷
//   ...
//   ╰────────────────┤
// Only the text between "│ • " and the first " | " is the map name.
namespace BkwEgoVoteRealFormat
{
inline bool IsMapsSectionStart(const char *pDescription)
{
	if(!pDescription)
		return false;
	return str_utf8_find_nocase(pDescription, "Maps (") != nullptr &&
		str_find(pDescription, "╭") != nullptr;
}

inline bool IsSectionEnd(const char *pDescription)
{
	if(!pDescription)
		return false;
	// EGO closes framed sections with a bottom-left box drawing character.
	return str_find(pDescription, "╰") != nullptr;
}

inline bool ExtractMapName(const char *pDescription, std::string &OutName)
{
	OutName.clear();
	if(!pDescription)
		return false;

	// The live format uses a bullet after the vertical frame. Search for the
	// bullet instead of hard-coding the exact number of spaces so minor spacing
	// changes on EGO do not break map detection.
	const char *pBullet = str_find(pDescription, "• ");
	if(!pBullet)
		return false;
	const char *pNameStart = pBullet + str_length("• ");
	const char *pSeparator = str_find(pNameStart, " | ");
	if(!pSeparator || pSeparator <= pNameStart)
		return false;

	OutName.assign(pNameStart, pSeparator);
	while(!OutName.empty() && (OutName.front() == ' ' || OutName.front() == '\t'))
		OutName.erase(OutName.begin());
	while(!OutName.empty() && (OutName.back() == ' ' || OutName.back() == '\t'))
		OutName.pop_back();
	return !OutName.empty();
}

inline bool ResolveVoteMap(const void *pItemId, int &OptionIndex, int &MapId)
{
	OptionIndex = -1;
	MapId = -1;
	if(!BkwEgoVote::s_pGameClient || !pItemId)
		return false;

	bool InMapsSection = false;
	int Index = 0;
	for(const CVoteOptionClient *pOption = BkwEgoVote::s_pGameClient->m_Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
	{
		const char *pDescription = pOption->m_aDescription;
		if(IsMapsSectionStart(pDescription))
		{
			InMapsSection = true;
		}
		else if(InMapsSection && IsSectionEnd(pDescription))
		{
			InMapsSection = false;
		}

		if(pOption != pItemId)
			continue;
		if(!InMapsSection)
			return false;

		std::string MapName;
		if(!ExtractMapName(pDescription, MapName))
			return false;

		const BkwEgoVote::SMap *pMap = BkwEgoVote::FindMapByName(MapName);
		if(!pMap)
			return false;

		OptionIndex = Index;
		MapId = pMap->m_Id;
		return true;
	}
	return false;
}

inline void RenderRightAction(const void *pItemId, CUIRect *pItemRect)
{
	if(!pItemRect || !BkwEgoVote::s_pUi || !BkwEgoVote::s_pGameClient || !g_Config.m_BkwEgoMapDetails)
		return;

	int OptionIndex = -1;
	int MapId = -1;
	if(!ResolveVoteMap(pItemId, OptionIndex, MapId))
		return;

	if((int)BkwEgoVote::s_vDetailsButtons.size() <= OptionIndex)
		BkwEgoVote::s_vDetailsButtons.resize(std::max(OptionIndex + 1, BkwEgoVote::s_pGameClient->m_Voting.NumOptions()));

	CUIRect Action;
	const float ButtonWidth = std::min(92.0f, std::max(70.0f, pItemRect->w * 0.22f));
	pItemRect->VSplitRight(ButtonWidth, pItemRect, &Action);
	pItemRect->VSplitRight(4.0f, pItemRect, nullptr);
	Action.HMargin(1.0f, &Action);

	if(BkwEgoVote::s_pGameClient->m_Menus.DoButton_Menu(
		&BkwEgoVote::s_vDetailsButtons[OptionIndex],
		"Подробнее",
		BkwEgoVote::s_SelectedMapId == MapId && BkwEgoVote::s_pUi->IsPopupOpen(&BkwEgoVote::s_DetailsPopupId),
		&Action,
		BUTTONFLAG_LEFT,
		nullptr,
		IGraphics::CORNER_ALL,
		4.0f,
		-0.15f))
	{
		BkwEgoVote::OpenDetails(MapId);
	}
}

inline void OverrideStandardVoteAction(CUi *pUi, CGameClient *pGameClient, IClient *pClient, IHttp *pHttp)
{
	if(!pUi || !pGameClient || !pClient || !pHttp ||
		!g_Config.m_BkwEgoMapDetails ||
		pClient->State() != IClient::STATE_ONLINE ||
		!BkwEgoVote::IsEgoServerName(pClient->ServerInfo().m_aName) ||
		!pGameClient->m_Menus.IsActive() ||
		pGameClient->m_Menus.GamePage() != CMenus::PAGE_CALLVOTE)
	{
		return;
	}

	// UpdateStandardVoteIntegration already handles API loading and teardown.
	// Replace only its row parser/action callback with the live EGO format parser.
	CListBox::SetRightActionCallback(RenderRightAction);
}
} // namespace BkwEgoVoteRealFormat

#endif
