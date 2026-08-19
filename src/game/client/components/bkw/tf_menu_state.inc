#ifndef GAME_CLIENT_COMPONENTS_BKW_TF_MENU_STATE_INC
#define GAME_CLIENT_COMPONENTS_BKW_TF_MENU_STATE_INC

#include <game/client/components/bkw/ddnet_vote_state.inc>

#include <cstdint>

class CUi;

namespace BkwTfMenuState
{
inline bool OverlayActive = false;
inline bool SuppressReopen = false;
inline bool PulseSent = false;
inline bool ConnectedToTeeFusion = false;
inline bool ConnectedToDdnet = false;
inline int SelectedTab = 0;
inline int PendingTab = -1;
inline int ScrollRow = 0;
inline bool WaitingForUpdate = false;
inline uint64_t VoteSignatureBeforeAction = 0;
inline int64_t ActionStartedAt = 0;
inline CUi *pLastUi = nullptr;
} // namespace BkwTfMenuState

#define s_BkwTfMenuOverlayActive BkwTfMenuState::OverlayActive
#define s_BkwTfMenuOverlaySuppressReopen BkwTfMenuState::SuppressReopen
#define s_BkwTfMenuOverlayPulseSent BkwTfMenuState::PulseSent
#define s_BkwTfMenuConnectedToTeeFusion BkwTfMenuState::ConnectedToTeeFusion
#define s_BkwTfMenuConnectedToDdnet BkwTfMenuState::ConnectedToDdnet
#define s_BkwTfMenuSelectedTab BkwTfMenuState::SelectedTab
#define s_BkwTfMenuPendingTab BkwTfMenuState::PendingTab
#define s_BkwTfMenuScrollRow BkwTfMenuState::ScrollRow
#define s_BkwTfMenuWaitingForUpdate BkwTfMenuState::WaitingForUpdate
#define s_BkwTfMenuVoteSignatureBeforeAction BkwTfMenuState::VoteSignatureBeforeAction
#define s_BkwTfMenuActionStartedAt BkwTfMenuState::ActionStartedAt
#define s_pBkwTfMenuLastUi BkwTfMenuState::pLastUi

#endif
