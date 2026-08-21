#ifndef GAME_CLIENT_COMPONENTS_BKW_TF_MENU_STATE_INC
#define GAME_CLIENT_COMPONENTS_BKW_TF_MENU_STATE_INC

#include <cstdint>

class CUi;

namespace BkwTfMenuState
{
inline bool OverlayActive = false;
inline bool SuppressReopen = false;
inline bool PulseSent = false;

// Server community routing
inline bool ConnectedToTeeFusion = false;
inline bool ConnectedToDdnet = false;
inline bool CommunityResolved = false;
inline int Community = 0; // 0 unknown, 1 TeeFusion, 2 DDNet

// Menu state
inline int SelectedTab = 0;
inline int PendingTab = -1;
inline int ScrollRow = 0;
inline bool WaitingForUpdate = false;
inline uint64_t VoteSignatureBeforeAction = 0;
inline int64_t ActionStartedAt = 0;

// Personalization page
inline bool PersonalizationOpen = false;
inline bool DriftCameraVisible = false;
inline bool DriftCameraEnabled = false;

// UI preferences
inline bool UseCustomVoteMenu = true;
inline bool ShowMapCards = true;

inline CUi *pLastUi = nullptr;
}

#define s_BkwTfMenuOverlayActive BkwTfMenuState::OverlayActive
#define s_BkwTfMenuOverlaySuppressReopen BkwTfMenuState::SuppressReopen
#define s_BkwTfMenuOverlayPulseSent BkwTfMenuState::PulseSent
#define s_BkwTfMenuConnectedToTeeFusion BkwTfMenuState::ConnectedToTeeFusion
#define s_BkwTfMenuConnectedToDdnet BkwTfMenuState::ConnectedToDdnet
#define s_BkwTfMenuCommunityResolved BkwTfMenuState::CommunityResolved
#define s_BkwTfMenuCommunity BkwTfMenuState::Community
#define s_BkwTfMenuPersonalizationOpen BkwTfMenuState::PersonalizationOpen
#define s_BkwTfMenuDriftCameraVisible BkwTfMenuState::DriftCameraVisible
#define s_BkwTfMenuDriftCameraEnabled BkwTfMenuState::DriftCameraEnabled
#define s_BkwTfMenuSelectedTab BkwTfMenuState::SelectedTab
#define s_BkwTfMenuPendingTab BkwTfMenuState::PendingTab
#define s_BkwTfMenuScrollRow BkwTfMenuState::ScrollRow
#define s_BkwTfMenuWaitingForUpdate BkwTfMenuState::WaitingForUpdate
#define s_BkwTfMenuVoteSignatureBeforeAction BkwTfMenuState::VoteSignatureBeforeAction
#define s_BkwTfMenuActionStartedAt BkwTfMenuState::ActionStartedAt
#define s_BkwTfMenuUseCustomVoteMenu BkwTfMenuState::UseCustomVoteMenu
#define s_BkwTfMenuShowMapCards BkwTfMenuState::ShowMapCards
#define s_pBkwTfMenuLastUi BkwTfMenuState::pLastUi

#endif
