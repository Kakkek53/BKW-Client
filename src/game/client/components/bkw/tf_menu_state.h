#ifndef GAME_CLIENT_COMPONENTS_BKW_TF_MENU_STATE_H
#define GAME_CLIENT_COMPONENTS_BKW_TF_MENU_STATE_H

#include <cstdint>

class CUi;

namespace BkwTfMenuState
{
inline bool OverlayActive = false;
inline bool SuppressReopen = false;
inline bool PulseSent = false;
inline int SelectedTab = 0;
inline int PendingTab = -1;
inline int ScrollRow = 0;
inline bool WaitingForUpdate = false;
inline uint64_t VoteSignatureBeforeAction = 0;
inline int64_t ActionStartedAt = 0;
inline CUi *pLastUi = nullptr;
} // namespace BkwTfMenuState

// Compatibility aliases keep the existing FastActions integration small while
// making the state truly shared by every translation unit that renders TF Menu.
#define s_BkwTfMenuOverlayActive BkwTfMenuState::OverlayActive
#define s_BkwTfMenuOverlaySuppressReopen BkwTfMenuState::SuppressReopen
#define s_BkwTfMenuOverlayPulseSent BkwTfMenuState::PulseSent
#define s_BkwTfMenuSelectedTab BkwTfMenuState::SelectedTab
#define s_BkwTfMenuPendingTab BkwTfMenuState::PendingTab
#define s_BkwTfMenuScrollRow BkwTfMenuState::ScrollRow
#define s_BkwTfMenuWaitingForUpdate BkwTfMenuState::WaitingForUpdate
#define s_BkwTfMenuVoteSignatureBeforeAction BkwTfMenuState::VoteSignatureBeforeAction
#define s_BkwTfMenuActionStartedAt BkwTfMenuState::ActionStartedAt
#define s_pBkwTfMenuLastUi BkwTfMenuState::pLastUi

#endif
