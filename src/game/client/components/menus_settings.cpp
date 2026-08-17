/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "menus.h"

#include <base/dbg.h>
#include <base/math.h>
#include <base/str.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/components/bkw/ddstats_hours.h>
#include <game/client/components/bkw/save_store.h>
#include <game/client/components/bkw/save_warning.h>
#include <game/client/components/menu_background.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>
#include <game/localization.h>

#include <algorithm>
#include <ctime>
#include <string>
#include <vector>

void CMenus::SetNeedSendInfo()
{
	if(m_Dummy)
		m_NeedSendDummyinfo = true;
	else
		m_NeedSendinfo = true;
}

void CMenus::RenderSettings(CUIRect MainView)
{
	if(g_Config.m_UiSettingsPage < 0 || g_Config.m_UiSettingsPage >= SETTINGS_LENGTH)
		g_Config.m_UiSettingsPage = SETTINGS_GENERAL;
	if(g_Config.m_UiSettingsPage == SETTINGS_LANGUAGE)
		g_Config.m_UiSettingsPage = SETTINGS_GENERAL;
	if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
		g_Config.m_UiSettingsPage = SETTINGS_TEE;

	if(m_GifWheelEditorOpen)
	{
		RenderSettingsBestClientGifWheelFullscreen(*Ui()->Screen());
		return;
	}
	if(m_AssetsEditorState.m_VisualsEditorOpen && m_AssetsEditorState.m_FullscreenOpen)
	{
		RenderAssetsEditorScreen(*Ui()->Screen());
		return;
	}

	g_Config.m_BcSettingsLayout = minimum(maximum(g_Config.m_BcSettingsLayout, 0), 1);

	enum
	{
		BKW_TAB_SAVES = 0,
		BKW_TAB_CHECKPOINTS,
		BKW_TAB_PLAYER,
		BKW_TAB_CLANS,
		BKW_TAB_HOURS,
		BKW_TAB_HUD,
		BKW_TAB_BACKGROUND,
		BKW_TAB_EXIT,
		BKW_TAB_LENGTH,
	};
	static int s_BkwTab = BKW_TAB_SAVES;

	auto RenderBkwPage = [&](CUIRect PageView) {
		static Bkw::CSaveStore s_SaveStore;
		static CButtonContainer s_SaveButton;
		static CButtonContainer s_aLoadButtons[256];
		static CButtonContainer s_aDeleteButtons[256];
		static int s_CheckpointsToggleId;
		static CButtonContainer s_CheckpointMouseLeftButton;
		static CButtonContainer s_CheckpointMouseRightButton;
		static int s_PlayerInfoToggleId;
		static CLineInputBuffered<64> s_PlayerInfoCommandInput;
		static bool s_PlayerInfoCommandInitialized = false;
		static int s_ClansToggleId;
		static CLineInputBuffered<64> s_ClansCommandInput;
		static bool s_ClansCommandInitialized = false;
		static CLineInputBuffered<64> s_HoursPlayerInput;
		static bool s_HoursPlayerInitialized = false;
		static bool s_HoursAutoRequested = false;
		static CButtonContainer s_HoursRefreshButton;
		static int s_MinimalHudToggleId;
		static int s_MinimalHudRaceTimeToggleId;
		static int s_MinimalHudPbDeltaToggleId;
		static int s_MinimalHudSpeedToggleId;
		static int s_MinimalHudCheckpointToggleId;
		static CButtonContainer s_MinimalHudLayoutHorizontalButton;
		static CButtonContainer s_MinimalHudLayoutCompactButton;
		static CButtonContainer s_aMinimalHudStyleButtons[3];
		static int s_MinimalHudAccentToggleId;
		static int s_MinimalHudHideScoreboardToggleId;
		static int s_MinimalHudHideMenusToggleId;
		static int s_MinimalHudFpsToggleId;
		static int s_MinimalHudPingToggleId;
		static int s_MinimalHudTeamToggleId;
		static int s_MinimalHudPracticeToggleId;
		static CButtonContainer s_aMinimalHudCornerButtons[4];
		static CButtonContainer s_aMinimalHudScaleButtons[3];
		static CButtonContainer s_aMinimalHudAlphaButtons[3];
		static int s_MediaBackgroundToggleId;
		static CLineInputBuffered<IO_MAX_PATH_LENGTH> s_MediaBackgroundPathInput;
		static bool s_MediaBackgroundPathInitialized = false;
		static CButtonContainer s_MediaBackgroundReloadButton;
		static CButtonContainer s_MediaBackgroundClearButton;
		static int s_SaveWarningToggleId;
		static CLineInputBuffered<256> s_SaveWarningIpsInput;
		static CLineInputBuffered<256> s_SaveWarningCommunitiesInput;
		static CLineInputBuffered<256> s_SaveWarningGameTypesInput;
		static bool s_SaveWarningInputsInitialized = false;
		static CButtonContainer s_aBkwTabButtons[BKW_TAB_LENGTH];

		s_SaveStore.Load(Storage());
		auto &SaveWarning = Bkw::SaveWarningState();
		SaveWarning.Load(Storage());
		if(!s_SaveWarningInputsInitialized)
		{
			s_SaveWarningIpsInput.Set(SaveWarning.m_ExcludedIps.c_str());
			s_SaveWarningCommunitiesInput.Set(SaveWarning.m_ExcludedCommunities.c_str());
			s_SaveWarningGameTypesInput.Set(SaveWarning.m_ExcludedGameTypes.c_str());
			s_SaveWarningInputsInitialized = true;
		}
		if(!s_PlayerInfoCommandInitialized)
		{
			s_PlayerInfoCommandInput.Set(GameClient()->m_BindChat.BkwPlayerInfoChatCommand());
			s_PlayerInfoCommandInitialized = true;
		}
		if(!s_ClansCommandInitialized)
		{
			s_ClansCommandInput.Set(GameClient()->m_BindChat.BkwClansChatCommand());
			s_ClansCommandInitialized = true;
		}

		s_BkwTab = std::clamp(s_BkwTab, 0, BKW_TAB_LENGTH - 1);
		CUIRect BkwTabBar;
		PageView.HSplitTop(30.0f, &BkwTabBar, &PageView);
		const char *apBkwTabs[BKW_TAB_LENGTH] = {"Сохранение", "Чекпоинты", "Игрок", "Кланы", "Часы", "HUD", "Фон", "Выход"};
		CUIRect RemainingTabs = BkwTabBar;
		const float TabWidth = BkwTabBar.w / (float)BKW_TAB_LENGTH;
		for(int i = 0; i < BKW_TAB_LENGTH; ++i)
		{
			CUIRect TabButton;
			RemainingTabs.VSplitLeft(TabWidth, &TabButton, &RemainingTabs);
			const int Corners = i == 0 ? (IGraphics::CORNER_TL | IGraphics::CORNER_BL) : (i == BKW_TAB_LENGTH - 1 ? (IGraphics::CORNER_TR | IGraphics::CORNER_BR) : IGraphics::CORNER_NONE);
			if(DoButton_MenuTab(&s_aBkwTabButtons[i], apBkwTabs[i], s_BkwTab == i, &TabButton, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
				s_BkwTab = i;
		}
		PageView.HSplitTop(14.0f, nullptr, &PageView);

		const bool Online = Client()->State() == IClient::STATE_ONLINE;
		const CServerInfo &ServerInfo = Client()->ServerInfo();
		const int LocalClientId = GameClient()->m_Snap.m_LocalClientId;

		if(s_BkwTab == BKW_TAB_SAVES)
		{
			int CurrentTeam = -1;
			std::vector<std::string> vCurrentPlayers;
			if(Online && LocalClientId >= 0 && LocalClientId < MAX_CLIENTS)
			{
				CurrentTeam = GameClient()->m_Teams.Team(LocalClientId);
				if(CurrentTeam > TEAM_FLOCK && CurrentTeam < GameClient()->m_Teams.TeamSuper())
				{
					for(int ClientId = 0; ClientId < MAX_CLIENTS; ++ClientId)
					{
						if(!GameClient()->m_aClients[ClientId].m_Active)
							continue;
						if(GameClient()->m_Teams.Team(ClientId) != CurrentTeam)
							continue;
						const char *pName = GameClient()->m_aClients[ClientId].m_aName;
						if(pName[0] != '\0')
							vCurrentPlayers.emplace_back(pName);
					}
					std::sort(vCurrentPlayers.begin(), vCurrentPlayers.end());
				}
			}
			const bool InDdraceTeam = CurrentTeam > TEAM_FLOCK && CurrentTeam < GameClient()->m_Teams.TeamSuper() && !vCurrentPlayers.empty();

			CUIRect Header, Info, SaveButtonRect;
			PageView.HSplitTop(28.0f, &Header, &PageView);
			Ui()->DoLabel(&Header, "BKW — Сохранение", 22.0f, TEXTALIGN_ML);
			PageView.HSplitTop(8.0f, nullptr, &PageView);

			PageView.HSplitTop(74.0f, &Info, &PageView);
			Info.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.45f), IGraphics::CORNER_ALL, 6.0f);
			Info.Margin(10.0f, &Info);
			char aInfo[512];
			if(!Online)
				str_copy(aInfo, "Подключитесь к серверу и войдите в /team, чтобы создать сохранение.");
			else if(!InDdraceTeam)
				str_format(aInfo, sizeof(aInfo), "Карта: %s\nВойдите в отдельную /team (не team 0), чтобы сохранить команду.", ServerInfo.m_aMap);
			else
				str_format(aInfo, sizeof(aInfo), "Карта: %s   •   Игроков в team: %d\nСервер: %s", ServerInfo.m_aMap, (int)vCurrentPlayers.size(), ServerInfo.m_aAddress);
			Ui()->DoLabel(&Info, aInfo, 13.0f, TEXTALIGN_ML);

			PageView.HSplitTop(10.0f, nullptr, &PageView);
			PageView.HSplitTop(32.0f, &SaveButtonRect, &PageView);
			if(InDdraceTeam)
			{
				if(DoButton_Menu(&s_SaveButton, "Сохраниться", 0, &SaveButtonRect))
				{
					Bkw::SSaveEntry Entry;
					Entry.m_Key = s_SaveStore.NextFreeKey();
					Entry.m_ServerAddress = ServerInfo.m_aAddress;
					Entry.m_Map = ServerInfo.m_aMap;
					Entry.m_SavedAtUnix = (long long)std::time(nullptr);
					Entry.m_vPlayers = vCurrentPlayers;

					char aCommand[128];
					str_format(aCommand, sizeof(aCommand), "/save %s", Entry.m_Key.c_str());
					GameClient()->m_Chat.SendChat(0, aCommand);
					s_SaveStore.Add(std::move(Entry));
					s_SaveStore.Save(Storage());
					Bkw::SaveWarningState().MarkSaved();
				}
			}
			else
			{
				SaveButtonRect.Draw(ColorRGBA(0.18f, 0.18f, 0.18f, 0.45f), IGraphics::CORNER_ALL, 5.0f);
				TextRender()->TextColor(0.55f, 0.55f, 0.55f, 1.0f);
				Ui()->DoLabel(&SaveButtonRect, "Сохраниться", 14.0f, TEXTALIGN_MC);
				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}

			PageView.HSplitTop(16.0f, nullptr, &PageView);
			CUIRect ListHeader;
			PageView.HSplitTop(24.0f, &ListHeader, &PageView);
			Ui()->DoLabel(&ListHeader, "Сохранения", 18.0f, TEXTALIGN_ML);

			if(s_SaveStore.Entries().empty())
			{
				CUIRect Empty;
				PageView.HSplitTop(34.0f, &Empty, &PageView);
				TextRender()->TextColor(0.65f, 0.65f, 0.65f, 1.0f);
				Ui()->DoLabel(&Empty, "Пока нет сохранений.", 13.0f, TEXTALIGN_ML);
				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
			else
			{
				const size_t MaxRendered = minimum<size_t>(s_SaveStore.Entries().size(), 256);
				for(size_t Index = 0; Index < MaxRendered; ++Index)
				{
					const Bkw::SSaveEntry &Entry = s_SaveStore.Entries()[Index];
					const bool ServerMatches = Online && str_comp(Entry.m_ServerAddress.c_str(), ServerInfo.m_aAddress) == 0;
					const bool MapMatches = Online && str_comp(Entry.m_Map.c_str(), ServerInfo.m_aMap) == 0;
					const bool TeamMatches = InDdraceTeam && Entry.m_vPlayers == vCurrentPlayers;
					const long long NowUnix = (long long)std::time(nullptr);
					const int CooldownLeft = Entry.m_SavedAtUnix > 0 ? maximum(0, 30 - (int)(NowUnix - Entry.m_SavedAtUnix)) : 0;
					const bool CooldownReady = CooldownLeft == 0;
					const bool CanLoad = ServerMatches && MapMatches && TeamMatches && CooldownReady;

					CUIRect Card;
					PageView.HSplitTop(104.0f, &Card, &PageView);
					PageView.HSplitTop(8.0f, nullptr, &PageView);
					Card.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
					Card.Margin(8.0f, &Card);

					CUIRect Title, Meta, Players, Buttons;
					Card.HSplitTop(20.0f, &Title, &Card);
					Card.HSplitTop(22.0f, &Meta, &Card);
					Card.HSplitTop(22.0f, &Players, &Card);
					Card.HSplitBottom(28.0f, &Card, &Buttons);

					char aTitle[128];
					str_format(aTitle, sizeof(aTitle), "Сохранение %s", Entry.m_Key.c_str());
					Ui()->DoLabel(&Title, aTitle, 15.0f, TEXTALIGN_ML);

					char aMeta[512];
					str_format(aMeta, sizeof(aMeta), "%s  •  %s", Entry.m_Map.c_str(), Entry.m_ServerAddress.c_str());
					Ui()->DoLabel(&Meta, aMeta, 11.0f, TEXTALIGN_ML);

					std::string PlayerText;
					for(size_t PlayerIndex = 0; PlayerIndex < Entry.m_vPlayers.size(); ++PlayerIndex)
					{
						if(PlayerIndex != 0)
							PlayerText += ", ";
						PlayerText += Entry.m_vPlayers[PlayerIndex];
					}
					Ui()->DoLabel(&Players, PlayerText.c_str(), 11.0f, TEXTALIGN_ML);

					CUIRect State, LoadButton, DeleteButton;
					Buttons.VSplitRight(90.0f, &State, &DeleteButton);
					State.VSplitRight(10.0f, &State, nullptr);
					State.VSplitRight(110.0f, &State, &LoadButton);
					State.VSplitRight(10.0f, &State, nullptr);

					char aState[256];
					if(ServerMatches && MapMatches && TeamMatches && !CooldownReady)
						str_format(aState, sizeof(aState), "✓ Сервер  ✓ Карта  ✓ Команда   •   Можно загрузить через %d сек.", CooldownLeft);
					else if(CanLoad)
						str_copy(aState, "✓ Сервер  ✓ Карта  ✓ Команда   •   Готово к загрузке");
					else if(!Online)
						str_copy(aState, "Не подключены к серверу");
					else
						str_format(aState, sizeof(aState), "%s Сервер   %s Карта   %s Команда", ServerMatches ? "✓" : "✕", MapMatches ? "✓" : "✕", TeamMatches ? "✓" : "✕");
					Ui()->DoLabel(&State, aState, 10.0f, TEXTALIGN_ML);

					if(CanLoad)
					{
						if(DoButton_Menu(&s_aLoadButtons[Index], "Загрузиться", 0, &LoadButton))
						{
							char aCommand[128];
							str_format(aCommand, sizeof(aCommand), "/load %s", Entry.m_Key.c_str());
							GameClient()->m_Chat.SendChat(0, aCommand);
							Bkw::SaveWarningState().MarkLoaded();
							s_SaveStore.Remove(Index);
							s_SaveStore.Save(Storage());
							break;
						}
					}
					else
					{
						LoadButton.Draw(ColorRGBA(0.18f, 0.18f, 0.18f, 0.45f), IGraphics::CORNER_ALL, 5.0f);
						TextRender()->TextColor(0.55f, 0.55f, 0.55f, 1.0f);
						Ui()->DoLabel(&LoadButton, "Загрузиться", 12.0f, TEXTALIGN_MC);
						TextRender()->TextColor(TextRender()->DefaultTextColor());
					}

					if(DoButton_Menu(&s_aDeleteButtons[Index], "Удалить", 0, &DeleteButton))
					{
						s_SaveStore.Remove(Index);
						s_SaveStore.Save(Storage());
						break;
					}
				}
			}
		}
		else if(s_BkwTab == BKW_TAB_CHECKPOINTS)
		{
			CUIRect Header;
			PageView.HSplitTop(28.0f, &Header, &PageView);
			Ui()->DoLabel(&Header, "BKW — Чекпоинты", 22.0f, TEXTALIGN_ML);
			PageView.HSplitTop(8.0f, nullptr, &PageView);

			const bool CheckpointsEnabled = GameClient()->m_FastActions.BkwCheckpointsEnabled();
			CUIRect CheckpointToggle;
			PageView.HSplitTop(28.0f, &CheckpointToggle, &PageView);
			if(DoButton_CheckBox(&s_CheckpointsToggleId, "Чекпоинты", CheckpointsEnabled ? 1 : 0, &CheckpointToggle))
				GameClient()->m_FastActions.SetBkwCheckpointsEnabled(!CheckpointsEnabled);

			if(GameClient()->m_FastActions.BkwCheckpointsEnabled())
			{
				PageView.HSplitTop(8.0f, nullptr, &PageView);
				CUIRect CheckpointCard;
				PageView.HSplitTop(158.0f, &CheckpointCard, &PageView);
				CheckpointCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
				CheckpointCard.Margin(10.0f, &CheckpointCard);

				CUIRect Description, MouseLabel, MouseButtons, Help1, Help2, Status;
				CheckpointCard.HSplitTop(30.0f, &Description, &CheckpointCard);
				Ui()->DoLabel(&Description, "Работает только когда сервер подтвердил режим /practice.", 12.0f, TEXTALIGN_ML);
				CheckpointCard.HSplitTop(22.0f, &MouseLabel, &CheckpointCard);
				Ui()->DoLabel(&MouseLabel, "Кнопка создания / удаления:", 12.0f, TEXTALIGN_ML);
				CheckpointCard.HSplitTop(28.0f, &MouseButtons, &CheckpointCard);
				CUIRect LeftMouseButton, RightMouseButton;
				MouseButtons.VSplitMid(&LeftMouseButton, &RightMouseButton, 6.0f);
				const int MouseButton = GameClient()->m_FastActions.BkwCheckpointMouseButton();
				if(DoButton_Menu(&s_CheckpointMouseLeftButton, "ЛКМ", MouseButton == 0, &LeftMouseButton))
					GameClient()->m_FastActions.SetBkwCheckpointMouseButton(0);
				if(DoButton_Menu(&s_CheckpointMouseRightButton, "ПКМ", MouseButton == 1, &RightMouseButton))
					GameClient()->m_FastActions.SetBkwCheckpointMouseButton(1);

				CheckpointCard.HSplitTop(22.0f, &Help1, &CheckpointCard);
				Ui()->DoLabel(&Help1, "Удержание 0.35 сек.: создать точку у tee. Повторить рядом с точкой — удалить.", 11.0f, TEXTALIGN_ML);
				CheckpointCard.HSplitTop(22.0f, &Help2, &CheckpointCard);
				Ui()->DoLabel(&Help2, "ЛКМ + ПКМ одновременно: /tpxy к последнему чекпоинту.", 11.0f, TEXTALIGN_ML);
				CheckpointCard.HSplitTop(22.0f, &Status, &CheckpointCard);

				bool PracticeActive = false;
				if(Online && LocalClientId >= 0 && LocalClientId < MAX_CLIENTS)
				{
					const auto &Character = GameClient()->m_Snap.m_aCharacters[LocalClientId];
					PracticeActive = Character.m_Active && Character.m_HasExtendedData && (Character.m_ExtendedData.m_Flags & CHARACTERFLAG_PRACTICE_MODE) != 0;
				}
				char aCheckpointStatus[160];
				str_format(aCheckpointStatus, sizeof(aCheckpointStatus), "Practice: %s   •   Чекпоинтов: %d", PracticeActive ? "активен" : "не активен", GameClient()->m_FastActions.BkwCheckpointCount());
				TextRender()->TextColor(PracticeActive ? ColorRGBA(0.55f, 1.0f, 0.55f, 1.0f) : ColorRGBA(0.75f, 0.75f, 0.75f, 1.0f));
				Ui()->DoLabel(&Status, aCheckpointStatus, 11.0f, TEXTALIGN_ML);
				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
		}
		else if(s_BkwTab == BKW_TAB_PLAYER)
		{
			CUIRect Header;
			PageView.HSplitTop(28.0f, &Header, &PageView);
			Ui()->DoLabel(&Header, "BKW — Инф. о игроке (рейс)", 22.0f, TEXTALIGN_ML);
			PageView.HSplitTop(8.0f, nullptr, &PageView);

			const bool PlayerInfoEnabled = GameClient()->m_BindChat.BkwPlayerInfoEnabled();
			CUIRect PlayerInfoToggle;
			PageView.HSplitTop(28.0f, &PlayerInfoToggle, &PageView);
			if(DoButton_CheckBox(&s_PlayerInfoToggleId, "Инф. о игроке (рейс)", PlayerInfoEnabled ? 1 : 0, &PlayerInfoToggle))
				GameClient()->m_BindChat.SetBkwPlayerInfoEnabled(!PlayerInfoEnabled);

			if(GameClient()->m_BindChat.BkwPlayerInfoEnabled())
			{
				PageView.HSplitTop(8.0f, nullptr, &PageView);
				CUIRect PlayerInfoCard;
				PageView.HSplitTop(126.0f, &PlayerInfoCard, &PageView);
				PlayerInfoCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
				PlayerInfoCard.Margin(10.0f, &PlayerInfoCard);

				CUIRect Description, CommandLabel, CommandInput, Example, Privacy;
				PlayerInfoCard.HSplitTop(24.0f, &Description, &PlayerInfoCard);
				Ui()->DoLabel(&Description, "Показывает DDNet race-статистику локально. Сообщение не отправляется на сервер.", 11.0f, TEXTALIGN_ML);
				PlayerInfoCard.HSplitTop(20.0f, &CommandLabel, &PlayerInfoCard);
				Ui()->DoLabel(&CommandLabel, "Локальная команда:", 11.0f, TEXTALIGN_ML);
				PlayerInfoCard.HSplitTop(28.0f, &CommandInput, &PlayerInfoCard);
				if(Ui()->DoEditBox(&s_PlayerInfoCommandInput, &CommandInput, 12.0f))
				{
					GameClient()->m_BindChat.SetBkwPlayerInfoChatCommand(s_PlayerInfoCommandInput.GetString());
					if(!s_PlayerInfoCommandInput.IsActive())
						s_PlayerInfoCommandInput.Set(GameClient()->m_BindChat.BkwPlayerInfoChatCommand());
				}
				PlayerInfoCard.HSplitTop(22.0f, &Example, &PlayerInfoCard);
				char aPlayerInfoExample[160];
				str_format(aPlayerInfoExample, sizeof(aPlayerInfoExample), "Пример: %s DDNET PRO COACH", GameClient()->m_BindChat.BkwPlayerInfoChatCommand());
				Ui()->DoLabel(&Example, aPlayerInfoExample, 11.0f, TEXTALIGN_ML);
				PlayerInfoCard.HSplitTop(22.0f, &Privacy, &PlayerInfoCard);
				TextRender()->TextColor(ColorRGBA(0.6f, 0.9f, 0.6f, 1.0f));
				Ui()->DoLabel(&Privacy, "Результат виден только вам.", 11.0f, TEXTALIGN_ML);
				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
		}
		else if(s_BkwTab == BKW_TAB_CLANS)
		{
			CUIRect Header;
			PageView.HSplitTop(28.0f, &Header, &PageView);
			Ui()->DoLabel(&Header, "BKW — История кланов", 22.0f, TEXTALIGN_ML);
			PageView.HSplitTop(8.0f, nullptr, &PageView);

			const bool ClansEnabled = GameClient()->m_BindChat.BkwClansEnabled();
			CUIRect ClansToggle;
			PageView.HSplitTop(28.0f, &ClansToggle, &PageView);
			if(DoButton_CheckBox(&s_ClansToggleId, "История кланов", ClansEnabled ? 1 : 0, &ClansToggle))
				GameClient()->m_BindChat.SetBkwClansEnabled(!ClansEnabled);

			if(GameClient()->m_BindChat.BkwClansEnabled())
			{
				PageView.HSplitTop(8.0f, nullptr, &PageView);
				CUIRect ClansCard;
				PageView.HSplitTop(126.0f, &ClansCard, &PageView);
				ClansCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
				ClansCard.Margin(10.0f, &ClansCard);

				CUIRect Description, CommandLabel, CommandInput, Example, Privacy;
				ClansCard.HSplitTop(24.0f, &Description, &ClansCard);
				Ui()->DoLabel(&Description, "Показывает историю кланов игрока с Teerank. Команда остаётся локальной.", 11.0f, TEXTALIGN_ML);
				ClansCard.HSplitTop(20.0f, &CommandLabel, &ClansCard);
				Ui()->DoLabel(&CommandLabel, "Локальная команда:", 11.0f, TEXTALIGN_ML);
				ClansCard.HSplitTop(28.0f, &CommandInput, &ClansCard);
				if(Ui()->DoEditBox(&s_ClansCommandInput, &CommandInput, 12.0f))
				{
					GameClient()->m_BindChat.SetBkwClansChatCommand(s_ClansCommandInput.GetString());
					if(!s_ClansCommandInput.IsActive())
						s_ClansCommandInput.Set(GameClient()->m_BindChat.BkwClansChatCommand());
				}
				ClansCard.HSplitTop(22.0f, &Example, &ClansCard);
				char aClansExample[160];
				str_format(aClansExample, sizeof(aClansExample), "Пример: %s Akella", GameClient()->m_BindChat.BkwClansChatCommand());
				Ui()->DoLabel(&Example, aClansExample, 11.0f, TEXTALIGN_ML);
				ClansCard.HSplitTop(22.0f, &Privacy, &ClansCard);
				TextRender()->TextColor(ColorRGBA(0.6f, 0.9f, 0.6f, 1.0f));
				Ui()->DoLabel(&Privacy, "Источник: Teerank • результат виден только вам.", 11.0f, TEXTALIGN_ML);
				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
		}
		else if(s_BkwTab == BKW_TAB_HOURS)
		{
			auto &Hours = Bkw::DdStatsHoursState();
			Hours.Poll();

			if(!s_HoursPlayerInitialized)
			{
				if(Online && LocalClientId >= 0 && LocalClientId < MAX_CLIENTS && GameClient()->m_aClients[LocalClientId].m_Active)
					s_HoursPlayerInput.Set(GameClient()->m_aClients[LocalClientId].m_aName);
				s_HoursPlayerInitialized = true;
			}
			if(!s_HoursAutoRequested && s_HoursPlayerInput.GetString()[0] != '\0')
			{
				Hours.RequestIfStale(Http(), s_HoursPlayerInput.GetString());
				s_HoursAutoRequested = true;
			}

			CUIRect Header;
			PageView.HSplitTop(28.0f, &Header, &PageView);
			Ui()->DoLabel(&Header, "BKW — Часы", 22.0f, TEXTALIGN_ML);
			PageView.HSplitTop(8.0f, nullptr, &PageView);

			CUIRect Info;
			PageView.HSplitTop(54.0f, &Info, &PageView);
			Info.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
			Info.Margin(10.0f, &Info);
			Ui()->DoLabel(&Info, "Игровое время берётся из DDStats. Данные кэшируются на 10 минут.", 11.0f, TEXTALIGN_ML);

			PageView.HSplitTop(12.0f, nullptr, &PageView);
			CUIRect PlayerLabel, PlayerRow;
			PageView.HSplitTop(20.0f, &PlayerLabel, &PageView);
			Ui()->DoLabel(&PlayerLabel, "Ник DDStats:", 12.0f, TEXTALIGN_ML);
			PageView.HSplitTop(30.0f, &PlayerRow, &PageView);
			CUIRect PlayerEdit, RefreshButton;
			PlayerRow.VSplitRight(120.0f, &PlayerEdit, &RefreshButton);
			PlayerEdit.VSplitRight(8.0f, &PlayerEdit, nullptr);
			Ui()->DoEditBox(&s_HoursPlayerInput, &PlayerEdit, 12.0f);
			if(DoButton_Menu(&s_HoursRefreshButton, Hours.Loading() ? "Загрузка..." : "Обновить", 0, &RefreshButton) && !Hours.Loading() && s_HoursPlayerInput.GetString()[0] != '\0')
				Hours.Request(Http(), s_HoursPlayerInput.GetString(), true);

			PageView.HSplitTop(14.0f, nullptr, &PageView);
			CUIRect StatsCard;
			PageView.HSplitTop(176.0f, &StatsCard, &PageView);
			StatsCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
			StatsCard.Margin(12.0f, &StatsCard);

			if(Hours.Loading())
			{
				Ui()->DoLabel(&StatsCard, "Получаю статистику DDStats...", 16.0f, TEXTALIGN_MC);
			}
			else if(Hours.Error())
			{
				char aError[160];
				str_format(aError, sizeof(aError), "Не удалось получить DDStats (HTTP %d).", Hours.HttpStatus());
				TextRender()->TextColor(ColorRGBA(1.0f, 0.55f, 0.55f, 1.0f));
				Ui()->DoLabel(&StatsCard, aError, 14.0f, TEXTALIGN_MC);
				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
			else if(Hours.Loaded())
			{
				CUIRect PlayerLine, BigHours, ExactLine, StartLine, AverageLine, SourceLine;
				StatsCard.HSplitTop(20.0f, &PlayerLine, &StatsCard);
				char aPlayer[128];
				str_format(aPlayer, sizeof(aPlayer), "Игрок: %s", Hours.Player());
				Ui()->DoLabel(&PlayerLine, aPlayer, 12.0f, TEXTALIGN_MC);
				StatsCard.HSplitTop(48.0f, &BigHours, &StatsCard);
				char aBigHours[96];
				str_format(aBigHours, sizeof(aBigHours), "%.1f ч", Hours.TotalHours());
				Ui()->DoLabel(&BigHours, aBigHours, 30.0f, TEXTALIGN_MC);
				StatsCard.HSplitTop(22.0f, &ExactLine, &StatsCard);
				char aExact[128];
				Bkw::CDdStatsHours::FormatDuration(Hours.TotalSecondsPlayed(), aExact, sizeof(aExact), true);
				Ui()->DoLabel(&ExactLine, aExact, 12.0f, TEXTALIGN_MC);
				StatsCard.HSplitTop(22.0f, &StartLine, &StatsCard);
				char aStart[128];
				str_format(aStart, sizeof(aStart), "Статистика учитывается с: %s", Hours.StartOfPlaytime()[0] != '\0' ? Hours.StartOfPlaytime() : "неизвестно");
				Ui()->DoLabel(&StartLine, aStart, 11.0f, TEXTALIGN_MC);
				StatsCard.HSplitTop(22.0f, &AverageLine, &StatsCard);
				char aAverageDuration[96];
				Bkw::CDdStatsHours::FormatDuration(Hours.AverageSecondsPlayed(), aAverageDuration, sizeof(aAverageDuration));
				char aAverage[160];
				str_format(aAverage, sizeof(aAverage), "Среднее игровое время: %s", aAverageDuration);
				Ui()->DoLabel(&AverageLine, aAverage, 11.0f, TEXTALIGN_MC);
				StatsCard.HSplitTop(22.0f, &SourceLine, &StatsCard);
				TextRender()->TextColor(ColorRGBA(0.65f, 0.85f, 1.0f, 1.0f));
				Ui()->DoLabel(&SourceLine, "Источник: DDStats", 10.0f, TEXTALIGN_MC);
				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
			else
			{
				Ui()->DoLabel(&StatsCard, "Введите ник и нажмите «Обновить».", 14.0f, TEXTALIGN_MC);
			}
		}
		else if(s_BkwTab == BKW_TAB_HUD)
		{
			CUIRect Header;
			PageView.HSplitTop(28.0f, &Header, &PageView);
			Ui()->DoLabel(&Header, "BKW — Минималистичный HUD", 22.0f, TEXTALIGN_ML);
			PageView.HSplitTop(8.0f, nullptr, &PageView);

			CUIRect Toggle;
			PageView.HSplitTop(28.0f, &Toggle, &PageView);
			if(DoButton_CheckBox(&s_MinimalHudToggleId, "Минималистичный HUD", g_Config.m_BkwMinimalHud, &Toggle))
				g_Config.m_BkwMinimalHud ^= 1;

\t\t\tif(g_Config.m_BkwMinimalHud)\n\t\t\t{\n\t\t\t\tPageView.HSplitTop(8.0f, nullptr, &PageView);\n\t\t\t\tCUIRect PreviewCard;\n\t\t\t\tPageView.HSplitTop(g_Config.m_BkwMinimalHudLayout ? 84.0f : 62.0f, &PreviewCard, &PageView);\n\t\t\t\tPreviewCard.Draw(ColorRGBA(0.055f, 0.06f, 0.075f, 0.55f), IGraphics::CORNER_ALL, 6.0f);\n\t\t\t\tCUIRect PreviewLabel, PreviewArea;\n\t\t\t\tPreviewCard.HSplitTop(20.0f, &PreviewLabel, &PreviewArea);\n\t\t\t\tPreviewLabel.Margin(8.0f, &PreviewLabel);\n\t\t\t\tUi()->DoLabel(&PreviewLabel, "Предпросмотр", 11.0f, TEXTALIGN_ML);\n\t\t\t\tPreviewArea.Margin(8.0f, &PreviewArea);\n\n\t\t\t\tconst float PreviewScale = std::clamp(g_Config.m_BkwMinimalHudScale / 100.0f, 0.75f, 1.25f);\n\t\t\t\tconst float FontSize = 6.5f * PreviewScale;\n\t\t\t\tconst float PadX = 6.0f * PreviewScale;\n\t\t\t\tconst float PadY = 4.0f * PreviewScale;\n\t\t\t\tconst float Alpha = std::clamp(g_Config.m_BkwMinimalHudAlpha / 100.0f, 0.4f, 0.8f);\n\t\t\t\tconst char *pLine1 = "144 FPS   38 ms   TEAM 3   SPD 12.4   CP 3";\n\t\t\t\tconst char *pLine2 = "TIME 01:24.38   PB -0.42";\n\t\t\t\tconst bool CompactPreview = g_Config.m_BkwMinimalHudLayout != 0;\n\t\t\t\tconst float W1 = TextRender()->TextWidth(FontSize, pLine1);\n\t\t\t\tconst float W2 = CompactPreview ? TextRender()->TextWidth(FontSize, pLine2) : 0.0f;\n\t\t\t\tconst float HudW = minimum(PreviewArea.w, maximum(W1, W2) + PadX * 2.0f);\n\t\t\t\tconst float HudH = (CompactPreview ? FontSize * 2.0f + 2.0f * PreviewScale : FontSize) + PadY * 2.0f;\n\t\t\t\tconst float HudX = PreviewArea.x + (PreviewArea.w - HudW) / 2.0f;\n\t\t\t\tconst float HudY = PreviewArea.y + maximum(0.0f, (PreviewArea.h - HudH) / 2.0f);\n\t\t\t\tconst int Style = std::clamp(g_Config.m_BkwMinimalHudStyle, 0, 2);\n\t\t\t\tif(Style != 2)\n\t\t\t\t{\n\t\t\t\t\tconst ColorRGBA Bg = Style == 1 ? ColorRGBA(0.08f, 0.11f, 0.16f, Alpha * 0.72f) : ColorRGBA(0.03f, 0.03f, 0.03f, Alpha);\n\t\t\t\t\tGraphics()->DrawRect(HudX, HudY, HudW, HudH, Bg, IGraphics::CORNER_ALL, 4.5f * PreviewScale);\n\t\t\t\t}\n\t\t\t\tif(g_Config.m_BkwMinimalHudAccent)\n\t\t\t\t\tGraphics()->DrawRect(HudX, HudY, 1.5f * PreviewScale, HudH, ColorRGBA(0.35f, 0.72f, 1.0f, 0.95f), IGraphics::CORNER_L, 4.5f * PreviewScale);\n\t\t\t\tTextRender()->TextColor(0.94f, 0.96f, 1.0f, 1.0f);\n\t\t\t\tTextRender()->Text(HudX + PadX, HudY + PadY, FontSize, pLine1, HudW - PadX * 2.0f);\n\t\t\t\tif(CompactPreview)\n\t\t\t\t{\n\t\t\t\t\tTextRender()->TextColor(0.45f, 1.0f, 0.55f, 1.0f);\n\t\t\t\t\tTextRender()->Text(HudX + PadX, HudY + PadY + FontSize + 2.0f * PreviewScale, FontSize, pLine2, HudW - PadX * 2.0f);\n\t\t\t\t}\n\t\t\t\tTextRender()->TextColor(TextRender()->DefaultTextColor());\n\t\t\t\tPageView.HSplitTop(10.0f, nullptr, &PageView);\n				CUIRect ElementsCard;
				PageView.HSplitTop(124.0f, &ElementsCard, &PageView);
				ElementsCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
				ElementsCard.Margin(10.0f, &ElementsCard);
				CUIRect Title, Row1, Row2;
				ElementsCard.HSplitTop(24.0f, &Title, &ElementsCard);
				Ui()->DoLabel(&Title, "Элементы", 15.0f, TEXTALIGN_ML);
				ElementsCard.HSplitTop(34.0f, &Row1, &ElementsCard);
				ElementsCard.HSplitTop(34.0f, &Row2, &ElementsCard);
				CUIRect Fps, Ping, Team, Practice;
				Row1.VSplitMid(&Fps, &Ping, 8.0f);
				Row2.VSplitMid(&Team, &Practice, 8.0f);
				if(DoButton_CheckBox(&s_MinimalHudFpsToggleId, "FPS", g_Config.m_BkwMinimalHudFps, &Fps)) g_Config.m_BkwMinimalHudFps ^= 1;
				if(DoButton_CheckBox(&s_MinimalHudPingToggleId, "Ping", g_Config.m_BkwMinimalHudPing, &Ping)) g_Config.m_BkwMinimalHudPing ^= 1;
				if(DoButton_CheckBox(&s_MinimalHudTeamToggleId, "Team", g_Config.m_BkwMinimalHudTeam, &Team)) g_Config.m_BkwMinimalHudTeam ^= 1;
				if(DoButton_CheckBox(&s_MinimalHudPracticeToggleId, "Practice", g_Config.m_BkwMinimalHudPractice, &Practice)) g_Config.m_BkwMinimalHudPractice ^= 1;

				PageView.HSplitTop(10.0f, nullptr, &PageView);
				CUIRect LayoutCard;
				PageView.HSplitTop(166.0f, &LayoutCard, &PageView);
				LayoutCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
				LayoutCard.Margin(10.0f, &LayoutCard);
				CUIRect L1, Corners, L2, Scales, L3, Alphas;
				LayoutCard.HSplitTop(20.0f, &L1, &LayoutCard);
				Ui()->DoLabel(&L1, "Положение", 12.0f, TEXTALIGN_ML);
				LayoutCard.HSplitTop(30.0f, &Corners, &LayoutCard);
				const char *apCorners[4] = {"↖", "↗", "↙", "↘"};
				CUIRect CornerRemain = Corners;
				for(int i = 0; i < 4; ++i)
				{
					CUIRect B; CornerRemain.VSplitLeft(Corners.w / 4.0f, &B, &CornerRemain);
					if(DoButton_Menu(&s_aMinimalHudCornerButtons[i], apCorners[i], g_Config.m_BkwMinimalHudCorner == i, &B)) g_Config.m_BkwMinimalHudCorner = i;
				}
				LayoutCard.HSplitTop(20.0f, &L2, &LayoutCard);
				Ui()->DoLabel(&L2, "Масштаб", 12.0f, TEXTALIGN_ML);
				LayoutCard.HSplitTop(30.0f, &Scales, &LayoutCard);
				const int aScales[3] = {75, 100, 125};
				const char *apScales[3] = {"75%", "100%", "125%"};
				CUIRect ScaleRemain = Scales;
				for(int i = 0; i < 3; ++i)
				{
					CUIRect B; ScaleRemain.VSplitLeft(Scales.w / 3.0f, &B, &ScaleRemain);
					if(DoButton_Menu(&s_aMinimalHudScaleButtons[i], apScales[i], g_Config.m_BkwMinimalHudScale == aScales[i], &B)) g_Config.m_BkwMinimalHudScale = aScales[i];
				}
				LayoutCard.HSplitTop(20.0f, &L3, &LayoutCard);
				Ui()->DoLabel(&L3, "Прозрачность фона", 12.0f, TEXTALIGN_ML);
				LayoutCard.HSplitTop(30.0f, &Alphas, &LayoutCard);
				const int aAlphas[3] = {40, 60, 80};
				const char *apAlphas[3] = {"40%", "60%", "80%"};
				CUIRect AlphaRemain = Alphas;
				for(int i = 0; i < 3; ++i)
				{
					CUIRect B; AlphaRemain.VSplitLeft(Alphas.w / 3.0f, &B, &AlphaRemain);
					if(DoButton_Menu(&s_aMinimalHudAlphaButtons[i], apAlphas[i], g_Config.m_BkwMinimalHudAlpha == aAlphas[i], &B)) g_Config.m_BkwMinimalHudAlpha = aAlphas[i];
				}
				PageView.HSplitTop(10.0f, nullptr, &PageView);
				CUIRect StyleCard;
				PageView.HSplitTop(92.0f, &StyleCard, &PageView);
				StyleCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
				StyleCard.Margin(10.0f, &StyleCard);
				CUIRect StyleLabel, StyleButtons, AccentRow;
				StyleCard.HSplitTop(20.0f, &StyleLabel, &StyleCard);
				Ui()->DoLabel(&StyleLabel, "Стиль", 12.0f, TEXTALIGN_ML);
				StyleCard.HSplitTop(30.0f, &StyleButtons, &StyleCard);
				const char *apStyles[3] = {"Тёмный", "Стекло", "Без фона"};
				CUIRect StyleRemain = StyleButtons;
				for(int i = 0; i < 3; ++i)
				{
					CUIRect B; StyleRemain.VSplitLeft(StyleButtons.w / 3.0f, &B, &StyleRemain);
					if(DoButton_Menu(&s_aMinimalHudStyleButtons[i], apStyles[i], g_Config.m_BkwMinimalHudStyle == i, &B)) g_Config.m_BkwMinimalHudStyle = i;
				}
				StyleCard.HSplitTop(28.0f, &AccentRow, &StyleCard);
				if(DoButton_CheckBox(&s_MinimalHudAccentToggleId, "Акцентная линия", g_Config.m_BkwMinimalHudAccent, &AccentRow)) g_Config.m_BkwMinimalHudAccent ^= 1;

				PageView.HSplitTop(10.0f, nullptr, &PageView);
				CUIRect AutoHideCard;
				PageView.HSplitTop(88.0f, &AutoHideCard, &PageView);
				AutoHideCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
				AutoHideCard.Margin(10.0f, &AutoHideCard);
				CUIRect AutoTitle, AutoRow1, AutoRow2;
				AutoHideCard.HSplitTop(20.0f, &AutoTitle, &AutoHideCard);
				Ui()->DoLabel(&AutoTitle, "Автоскрытие", 12.0f, TEXTALIGN_ML);
				AutoHideCard.HSplitTop(28.0f, &AutoRow1, &AutoHideCard);
				AutoHideCard.HSplitTop(28.0f, &AutoRow2, &AutoHideCard);
				if(DoButton_CheckBox(&s_MinimalHudHideScoreboardToggleId, "Скрывать при scoreboard", g_Config.m_BkwMinimalHudHideScoreboard, &AutoRow1)) g_Config.m_BkwMinimalHudHideScoreboard ^= 1;
				if(DoButton_CheckBox(&s_MinimalHudHideMenusToggleId, "Скрывать в меню", g_Config.m_BkwMinimalHudHideMenus, &AutoRow2)) g_Config.m_BkwMinimalHudHideMenus ^= 1;
			}
		}
		else if(s_BkwTab == BKW_TAB_BACKGROUND)
		{
			if(!s_MediaBackgroundPathInitialized)
			{
				s_MediaBackgroundPathInput.Set(g_Config.m_BcMenuMediaBackgroundPath);
				s_MediaBackgroundPathInitialized = true;
			}

			CUIRect Header, Toggle, Info, PathLabel, PathEdit, Buttons, Status;
			PageView.HSplitTop(28.0f, &Header, &PageView);
			Ui()->DoLabel(&Header, "BKW — Фон", 22.0f, TEXTALIGN_ML);
			PageView.HSplitTop(8.0f, nullptr, &PageView);
			PageView.HSplitTop(28.0f, &Toggle, &PageView);
			if(DoButton_CheckBox(&s_MediaBackgroundToggleId, "Пользовательский фон главного меню", g_Config.m_BcMenuMediaBackground, &Toggle))
			{
				const bool Enable = g_Config.m_BcMenuMediaBackground == 0;
				if(Enable)
					str_copy(g_Config.m_BcMenuMediaBackgroundPath, s_MediaBackgroundPathInput.GetString());
				g_Config.m_BcMenuMediaBackground = Enable ? 1 : 0;
				GameClient()->m_MenuBackground.ReloadBkwMediaBackground();
			}

			PageView.HSplitTop(10.0f, nullptr, &PageView);
			PageView.HSplitTop(66.0f, &Info, &PageView);
			Info.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
			Info.Margin(10.0f, &Info);
			Ui()->DoLabel(&Info, "Укажите путь к картинке или видео. Поддержка формата зависит от media decoder сборки.\nПри ошибке BKW автоматически оставит обычный .map-фон.", 11.0f, TEXTALIGN_ML);

			CUIRect RaceOptions;
			PageView.HSplitTop(28.0f, &RaceOptions, &PageView);
			CUIRect RaceTimeOpt, PbDeltaOpt;
			RaceOptions.VSplitMid(&RaceTimeOpt, &PbDeltaOpt, 6.0f);
			if(DoButton_CheckBox(&s_MinimalHudRaceTimeToggleId, "Race time", g_Config.m_BkwMinimalHudRaceTime, &RaceTimeOpt))
				g_Config.m_BkwMinimalHudRaceTime ^= 1;
			if(DoButton_CheckBox(&s_MinimalHudPbDeltaToggleId, "PB delta", g_Config.m_BkwMinimalHudPbDelta, &PbDeltaOpt))
				g_Config.m_BkwMinimalHudPbDelta ^= 1;
			CUIRect MovementOptions;
			PageView.HSplitTop(28.0f, &MovementOptions, &PageView);
			CUIRect SpeedOpt, CheckpointOpt;
			MovementOptions.VSplitMid(&SpeedOpt, &CheckpointOpt, 6.0f);
			if(DoButton_CheckBox(&s_MinimalHudSpeedToggleId, "Speed", g_Config.m_BkwMinimalHudSpeed, &SpeedOpt))
				g_Config.m_BkwMinimalHudSpeed ^= 1;
			if(DoButton_CheckBox(&s_MinimalHudCheckpointToggleId, "Checkpoint", g_Config.m_BkwMinimalHudCheckpoint, &CheckpointOpt))
				g_Config.m_BkwMinimalHudCheckpoint ^= 1;
			PageView.HSplitTop(8.0f, nullptr, &PageView);
			CUIRect LayoutLabel, LayoutButtons;
			PageView.HSplitTop(20.0f, &LayoutLabel, &PageView);
			Ui()->DoLabel(&LayoutLabel, "Режим HUD:", 12.0f, TEXTALIGN_ML);
			PageView.HSplitTop(30.0f, &LayoutButtons, &PageView);
			CUIRect HorizontalBtn, CompactBtn;
			LayoutButtons.VSplitMid(&HorizontalBtn, &CompactBtn, 6.0f);
			if(DoButton_Menu(&s_MinimalHudLayoutHorizontalButton, "Горизонтальный", g_Config.m_BkwMinimalHudLayout == 0, &HorizontalBtn))
				g_Config.m_BkwMinimalHudLayout = 0;
			if(DoButton_Menu(&s_MinimalHudLayoutCompactButton, "Двухстрочный", g_Config.m_BkwMinimalHudLayout == 1, &CompactBtn))
				g_Config.m_BkwMinimalHudLayout = 1;
			PageView.HSplitTop(12.0f, nullptr, &PageView);
			PageView.HSplitTop(20.0f, &PathLabel, &PageView);
			Ui()->DoLabel(&PathLabel, "Файл фона (PNG/JPG/GIF/WebP/MP4/MOV/WebM и др.):", 12.0f, TEXTALIGN_ML);
			PageView.HSplitTop(30.0f, &PathEdit, &PageView);
			// Keep edits local until the user explicitly applies them.
			// Updating g_Config on every keystroke made CMenuBackground::Render()
			// repeatedly unload/reload the decoder through SyncFromConfig().
			Ui()->DoEditBox(&s_MediaBackgroundPathInput, &PathEdit, 12.0f);

			PageView.HSplitTop(10.0f, nullptr, &PageView);
			PageView.HSplitTop(30.0f, &Buttons, &PageView);
			CUIRect ReloadButton, ClearButton;
			Buttons.VSplitMid(&ReloadButton, &ClearButton, 8.0f);
			if(DoButton_Menu(&s_MediaBackgroundReloadButton, "Загрузить / обновить", 0, &ReloadButton))
			{
				str_copy(g_Config.m_BcMenuMediaBackgroundPath, s_MediaBackgroundPathInput.GetString());
				g_Config.m_BcMenuMediaBackground = 1;
				GameClient()->m_MenuBackground.ReloadBkwMediaBackground();
			}
			if(DoButton_Menu(&s_MediaBackgroundClearButton, "Убрать фон", 0, &ClearButton))
			{
				g_Config.m_BcMenuMediaBackground = 0;
				g_Config.m_BcMenuMediaBackgroundPath[0] = '\\0';
				s_MediaBackgroundPathInput.Set("");
				GameClient()->m_MenuBackground.ReloadBkwMediaBackground();
			}

			PageView.HSplitTop(12.0f, nullptr, &PageView);
			PageView.HSplitTop(44.0f, &Status, &PageView);
			Status.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
			Status.Margin(8.0f, &Status);
			const char *pStatus = GameClient()->m_MenuBackground.BkwMediaBackgroundStatus();
			if(GameClient()->m_MenuBackground.BkwMediaBackgroundHasError())
				TextRender()->TextColor(ColorRGBA(1.0f, 0.55f, 0.55f, 1.0f));
			else if(GameClient()->m_MenuBackground.BkwMediaBackgroundLoaded())
				TextRender()->TextColor(ColorRGBA(0.55f, 1.0f, 0.55f, 1.0f));
			Ui()->DoLabel(&Status, pStatus[0] != '\\0' ? pStatus : "Фон не загружен.", 11.0f, TEXTALIGN_ML);
			TextRender()->TextColor(TextRender()->DefaultTextColor());
		}
		else if(s_BkwTab == BKW_TAB_EXIT)
		{
			CUIRect Header, Toggle;
			PageView.HSplitTop(28.0f, &Header, &PageView);
			Ui()->DoLabel(&Header, "BKW — Предупреждение о выходе", 22.0f, TEXTALIGN_ML);
			PageView.HSplitTop(8.0f, nullptr, &PageView);
			PageView.HSplitTop(28.0f, &Toggle, &PageView);
			if(DoButton_CheckBox(&s_SaveWarningToggleId, "Предупреждать о несохранённом прогрессе", SaveWarning.m_Enabled ? 1 : 0, &Toggle))
			{
				SaveWarning.m_Enabled = !SaveWarning.m_Enabled;
				SaveWarning.Save(Storage());
			}

			PageView.HSplitTop(10.0f, nullptr, &PageView);
			CUIRect Info;
			PageView.HSplitTop(58.0f, &Info, &PageView);
			Info.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
			Info.Margin(10.0f, &Info);
			Ui()->DoLabel(&Info, "Окно появляется только после реального пересечения старта и исчезает после финиша или BKW-сохранения.", 11.0f, TEXTALIGN_ML);

			PageView.HSplitTop(14.0f, nullptr, &PageView);
			auto DoExceptionInput = [&](const char *pLabel, CLineInputBuffered<256> &Input, std::string &Target) {
				CUIRect Label, Edit;
				PageView.HSplitTop(20.0f, &Label, &PageView);
				Ui()->DoLabel(&Label, pLabel, 12.0f, TEXTALIGN_ML);
				PageView.HSplitTop(28.0f, &Edit, &PageView);
				if(Ui()->DoEditBox(&Input, &Edit, 12.0f))
				{
					Target = Input.GetString();
					SaveWarning.Save(Storage());
				}
				PageView.HSplitTop(10.0f, nullptr, &PageView);
			};

			DoExceptionInput("Исключения по IP (через запятую):", s_SaveWarningIpsInput, SaveWarning.m_ExcludedIps);
			DoExceptionInput("Исключения по сообществам, например ddnet, kog:", s_SaveWarningCommunitiesInput, SaveWarning.m_ExcludedCommunities);
			DoExceptionInput("Исключения по типу игры, например DDRace, Gores:", s_SaveWarningGameTypesInput, SaveWarning.m_ExcludedGameTypes);
			CUIRect Hint;
			PageView.HSplitTop(36.0f, &Hint, &PageView);
			TextRender()->TextColor(ColorRGBA(0.7f, 0.7f, 0.7f, 1.0f));
			Ui()->DoLabel(&Hint, "Поддерживаются запятые и точки с запятой. Совпадение без учёта регистра.", 11.0f, TEXTALIGN_ML);
			TextRender()->TextColor(TextRender()->DefaultTextColor());
		}
	};

	if(g_Config.m_BcSettingsLayout == 0)
	{
		const bool NeedRestart = m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartUpdate;

		auto RenderSettingsPage = [&](CUIRect PageView) {
			if(g_Config.m_UiSettingsPage == SETTINGS_GENERAL)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_GENERAL);
				RenderSettingsGeneral(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_TEE)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_TEE);
				if(Client()->IsSixup())
					RenderSettingsTee7(PageView);
				else
					RenderSettingsTee(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_APPEARANCE)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_APPEARANCE);
				RenderSettingsAppearance(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_CONTROLS)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_CONTROLS);
				m_MenusSettingsControls.Render(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_GRAPHICS)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_GRAPHICS);
				RenderSettingsGraphics(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_SOUND)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_SOUND);
				RenderSettingsSound(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_DDNET)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_DDNET);
				RenderSettingsDDNet(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_ASSETS)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_ASSETS);
				RenderSettingsAssets(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_TCLIENT)
			{
				GameClient()->m_MenuBackground.ChangePosition(13);
				RenderSettingsTClient(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_PROFILES)
			{
				GameClient()->m_MenuBackground.ChangePosition(14);
				RenderSettingsTClientProfiles(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_CONFIGS)
			{
				GameClient()->m_MenuBackground.ChangePosition(15);
				RenderSettingsTClientConfigs(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_CREDITS)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_RESERVED0);
				RenderBkwPage(PageView);
			}
			else if(g_Config.m_UiSettingsPage == SETTINGS_BESTCLIENT)
			{
				GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_RESERVED0);
				RenderSettingsBestClient(PageView);
			}
			else
			{
				dbg_assert_failed("ui_settings_page invalid");
			}
		};

		auto RenderRestartWarning = [&](CUIRect RestartBar) {
			CUIRect RestartWarning, RestartButton;
			RestartBar.VSplitRight(125.0f, &RestartWarning, &RestartButton);
			RestartWarning.VSplitRight(10.0f, &RestartWarning, nullptr);
			if(m_NeedRestartUpdate)
			{
				TextRender()->TextColor(0.7f, 1.0f, 0.7f, 1.0f);
				Ui()->DoLabel(&RestartWarning, Localize("BestClient update downloaded! Restart to apply."), 14.0f, TEXTALIGN_ML);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
			else
			{
				Ui()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), 14.0f, TEXTALIGN_ML);
			}

			static CButtonContainer s_RestartButton;
			if(DoButton_Menu(&s_RestartButton, Localize("Restart"), 0, &RestartButton))
			{
#if defined(CONF_AUTOUPDATE)
				if(m_NeedRestartUpdate)
				{
					Updater()->ApplyUpdateAndRestart();
				}
				else
#endif
				if(Client()->State() == IClient::STATE_ONLINE || GameClient()->Editor()->HasUnsavedData())
				{
					m_Popup = POPUP_RESTART;
				}
				else
				{
					Client()->Restart();
				}
			}
		};

		auto RenderSettingsPageNewLayout = [&](CUIRect PageView) {
			const int Page = g_Config.m_UiSettingsPage;
			const bool NeedsAutoScroll = Page == SETTINGS_GENERAL || Page == SETTINGS_APPEARANCE || Page == SETTINGS_CREDITS;

			if(!NeedsAutoScroll)
			{
				RenderSettingsPage(PageView);
				return;
			}

			static CScrollRegion s_aNewLayoutScrollRegions[SETTINGS_LENGTH];
			static CScrollRegion s_aBkwScrollRegions[BKW_TAB_LENGTH];
			CScrollRegionParams ScrollParams;
			ScrollParams.m_ScrollUnit = 60.0f;
			ScrollParams.m_ScrollbarMargin = 5.0f;

			vec2 ScrollOffset(0.0f, 0.0f);
			CScrollRegion &ScrollRegion = Page == SETTINGS_CREDITS ? s_aBkwScrollRegions[s_BkwTab] : s_aNewLayoutScrollRegions[Page];
			ScrollRegion.Begin(&PageView, &ScrollParams);

			CUIRect ContentView = PageView;
			const float ContentStartY = ContentView.y;
			float VirtualHeightBoost = Page == SETTINGS_GENERAL ? 120.0f : 96.0f;
			if(Page == SETTINGS_CREDITS)
				VirtualHeightBoost = s_BkwTab == BKW_TAB_SAVES ? 2350.0f : 360.0f;
			ContentView.h = PageView.h + VirtualHeightBoost;

			RenderSettingsPage(ContentView);

			CUIRect ContentRect = ContentView;
			ContentRect.y = ContentStartY;
			ContentRect.h = ContentView.h;
			ScrollRegion.AddRect(ContentRect);
			ScrollRegion.End();
		};

		CUIRect ContentView, RootTabBar, RestartBar;
		MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
		MainView.Margin(20.0f, &MainView);
		ContentView = MainView;
		if(NeedRestart)
		{
			ContentView.HSplitBottom(20.0f, &ContentView, &RestartBar);
			ContentView.HSplitBottom(10.0f, &ContentView, nullptr);
		}

		enum
		{
			ROOT_TAB_GENERAL = 0,
			ROOT_TAB_APPEARANCE,
			ROOT_TAB_TCLIENT,
			ROOT_TAB_BESTCLIENT,
			ROOT_TAB_BKW,
			ROOT_TAB_LENGTH,
		};

		auto GetRootTabByPage = [&](int Page) {
			if(Page == SETTINGS_GENERAL || Page == SETTINGS_CONTROLS || Page == SETTINGS_TEE || Page == SETTINGS_DDNET)
				return ROOT_TAB_GENERAL;
			if(Page == SETTINGS_APPEARANCE || Page == SETTINGS_GRAPHICS || Page == SETTINGS_SOUND || Page == SETTINGS_ASSETS)
				return ROOT_TAB_APPEARANCE;
			if(Page == SETTINGS_TCLIENT || Page == SETTINGS_PROFILES || Page == SETTINGS_CONFIGS)
				return ROOT_TAB_TCLIENT;
			if(Page == SETTINGS_CREDITS)
				return ROOT_TAB_BKW;
			return ROOT_TAB_BESTCLIENT;
		};

		ContentView.HSplitTop(36.0f, &RootTabBar, &ContentView);
		RootTabBar.HMargin(6.0f, &RootTabBar);

		const char *apRootTabs[ROOT_TAB_LENGTH] = {
			Localize("General"),
			Localize("Appearance"),
			TCLocalize("TClient"),
			Localize("BestClient"),
			"BKW",
		};
		static CButtonContainer s_aRootTabButtons[ROOT_TAB_LENGTH];
		const int CurRootTab = GetRootTabByPage(g_Config.m_UiSettingsPage);
		const float RootTabWidth = RootTabBar.w / (float)ROOT_TAB_LENGTH;
		CUIRect RootTabs = RootTabBar;
		for(int i = 0; i < ROOT_TAB_LENGTH; ++i)
		{
			CUIRect RootTabButton;
			RootTabs.VSplitLeft(RootTabWidth, &RootTabButton, &RootTabs);
			const int Corners = i == 0 ? (IGraphics::CORNER_TL | IGraphics::CORNER_BL) : (i == ROOT_TAB_LENGTH - 1 ? (IGraphics::CORNER_TR | IGraphics::CORNER_BR) : IGraphics::CORNER_NONE);
			if(DoButton_MenuTab(&s_aRootTabButtons[i], apRootTabs[i], CurRootTab == i, &RootTabButton, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
			{
				if(i == ROOT_TAB_GENERAL)
					g_Config.m_UiSettingsPage = SETTINGS_GENERAL;
				else if(i == ROOT_TAB_APPEARANCE)
					g_Config.m_UiSettingsPage = SETTINGS_APPEARANCE;
				else if(i == ROOT_TAB_TCLIENT)
					g_Config.m_UiSettingsPage = SETTINGS_TCLIENT;
				else if(i == ROOT_TAB_BESTCLIENT)
					g_Config.m_UiSettingsPage = SETTINGS_BESTCLIENT;
				else
					g_Config.m_UiSettingsPage = SETTINGS_CREDITS;
			}
		}

		const int ActiveRootTab = GetRootTabByPage(g_Config.m_UiSettingsPage);
		ContentView.HSplitTop(6.0f, nullptr, &ContentView);

		if(ActiveRootTab == ROOT_TAB_GENERAL || ActiveRootTab == ROOT_TAB_APPEARANCE || ActiveRootTab == ROOT_TAB_TCLIENT)
		{
			CUIRect SubTabBar;
			ContentView.HSplitTop(24.0f, &SubTabBar, &ContentView);

			const char *apSubTabs[4] = {};
			int aSubTabPages[4] = {};
			int NumSubTabs = 0;
			if(ActiveRootTab == ROOT_TAB_GENERAL)
			{
				apSubTabs[0] = Localize("General");
				apSubTabs[1] = Localize("Controls");
				apSubTabs[2] = Client()->IsSixup() ? "Tee (0.7)" : Localize("Tee");
				apSubTabs[3] = Localize("DDNet");
				aSubTabPages[0] = SETTINGS_GENERAL;
				aSubTabPages[1] = SETTINGS_CONTROLS;
				aSubTabPages[2] = SETTINGS_TEE;
				aSubTabPages[3] = SETTINGS_DDNET;
				NumSubTabs = 4;
			}
			else if(ActiveRootTab == ROOT_TAB_APPEARANCE)
			{
				apSubTabs[0] = Localize("Appearance");
				apSubTabs[1] = Localize("Graphics");
				apSubTabs[2] = Localize("Sound");
				apSubTabs[3] = Localize("Assets");
				aSubTabPages[0] = SETTINGS_APPEARANCE;
				aSubTabPages[1] = SETTINGS_GRAPHICS;
				aSubTabPages[2] = SETTINGS_SOUND;
				aSubTabPages[3] = SETTINGS_ASSETS;
				NumSubTabs = 4;
			}
			else if(ActiveRootTab == ROOT_TAB_TCLIENT)
			{
				apSubTabs[0] = TCLocalize("TClient");
				apSubTabs[1] = Localize("Profiles");
				apSubTabs[2] = Localize("Configs");
				aSubTabPages[0] = SETTINGS_TCLIENT;
				aSubTabPages[1] = SETTINGS_PROFILES;
				aSubTabPages[2] = SETTINGS_CONFIGS;
				NumSubTabs = 3;
			}

			static CButtonContainer s_aGeneralSubTabButtons[4];
			static CButtonContainer s_aAppearanceSubTabButtons[4];
			static CButtonContainer s_aTClientSubTabButtons[3];
			CButtonContainer *pSubButtons = ActiveRootTab == ROOT_TAB_GENERAL ? s_aGeneralSubTabButtons : (ActiveRootTab == ROOT_TAB_APPEARANCE ? s_aAppearanceSubTabButtons : s_aTClientSubTabButtons);

			CUIRect SubTabs = SubTabBar;
			const float SubTabWidth = SubTabBar.w / (float)NumSubTabs;
			for(int i = 0; i < NumSubTabs; ++i)
			{
				CUIRect SubButton;
				SubTabs.VSplitLeft(SubTabWidth, &SubButton, &SubTabs);
				const int Corners = i == 0 ? (IGraphics::CORNER_TL | IGraphics::CORNER_BL) : (i == NumSubTabs - 1 ? (IGraphics::CORNER_TR | IGraphics::CORNER_BR) : IGraphics::CORNER_NONE);
				if(DoButton_MenuTab(&pSubButtons[i], apSubTabs[i], g_Config.m_UiSettingsPage == aSubTabPages[i], &SubButton, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
					g_Config.m_UiSettingsPage = aSubTabPages[i];
			}

			ContentView.HSplitTop(10.0f, nullptr, &ContentView);
		}

		RenderSettingsPageNewLayout(ContentView);
		if(NeedRestart)
			RenderRestartWarning(RestartBar);
		return;
	}

	CUIRect Button, TabBar, RestartBar;
	MainView.VSplitRight(120.0f, &MainView, &TabBar);
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
	MainView.Margin(20.0f, &MainView);

	const bool NeedRestart = m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartUpdate;
	if(NeedRestart)
	{
		MainView.HSplitBottom(20.0f, &MainView, &RestartBar);
		MainView.HSplitBottom(10.0f, &MainView, nullptr);
	}

	TabBar.HSplitTop(50.0f, &Button, &TabBar);
	Button.Draw(ms_ColorTabbarActive, IGraphics::CORNER_BR, 10.0f);

	const char *apTabs[SETTINGS_LENGTH] = {
		Localize("Language"),
		Localize("General"),
		Localize("Player"),
		Client()->IsSixup() ? "Tee 0.7" : Localize("Tee"),
		Localize("Appearance"),
		Localize("Controls"),
		Localize("Graphics"),
		Localize("Sound"),
		Localize("DDNet"),
		Localize("Assets"),
		TCLocalize("TClient"),
		Localize("BestClient"),
		Localize("Profiles"),
		Localize("Configs"),
		"BKW"};

	if(g_Config.m_UiSettingsPage == SETTINGS_LANGUAGE)
		g_Config.m_UiSettingsPage = SETTINGS_GENERAL;
	if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
		g_Config.m_UiSettingsPage = SETTINGS_TEE;

	static CButtonContainer s_aTabButtons[SETTINGS_LENGTH];

	for(int i = 0; i < SETTINGS_LENGTH; i++)
	{
		if(i == SETTINGS_LANGUAGE || i == SETTINGS_PLAYER)
			continue;
		TabBar.HSplitTop(10.0f, nullptr, &TabBar);
		TabBar.HSplitTop(26.0f, &Button, &TabBar);
		if(DoButton_MenuTab(&s_aTabButtons[i], apTabs[i], g_Config.m_UiSettingsPage == i, &Button, IGraphics::CORNER_R, &m_aAnimatorsSettingsTab[i]))
			g_Config.m_UiSettingsPage = i;
	}

	if(g_Config.m_UiSettingsPage == SETTINGS_LANGUAGE)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_LANGUAGE);
		RenderLanguageSettings(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GENERAL)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_GENERAL);
		RenderSettingsGeneral(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_PLAYER);
		RenderSettingsPlayer(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_TEE)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_TEE);
		if(Client()->IsSixup())
			RenderSettingsTee7(MainView);
		else
			RenderSettingsTee(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_APPEARANCE)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_APPEARANCE);
		RenderSettingsAppearance(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_CONTROLS)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_CONTROLS);
		m_MenusSettingsControls.Render(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GRAPHICS)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_GRAPHICS);
		RenderSettingsGraphics(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_SOUND)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_SOUND);
		RenderSettingsSound(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_DDNET)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_DDNET);
		RenderSettingsDDNet(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_ASSETS)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_ASSETS);
		RenderSettingsAssets(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_TCLIENT)
	{
		GameClient()->m_MenuBackground.ChangePosition(13);
		RenderSettingsTClient(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_PROFILES)
	{
		GameClient()->m_MenuBackground.ChangePosition(14);
		RenderSettingsTClientProfiles(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_CONFIGS)
	{
		GameClient()->m_MenuBackground.ChangePosition(15);
		RenderSettingsTClientConfigs(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_CREDITS)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_RESERVED0);
		RenderBkwPage(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_BESTCLIENT)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_RESERVED0);
		RenderSettingsBestClient(MainView);
	}
	else
	{
		dbg_assert_failed("ui_settings_page invalid");
	}

	if(NeedRestart)
	{
		CUIRect RestartWarning, RestartButton;
		RestartBar.VSplitRight(125.0f, &RestartWarning, &RestartButton);
		RestartWarning.VSplitRight(10.0f, &RestartWarning, nullptr);
		if(m_NeedRestartUpdate)
		{
			TextRender()->TextColor(0.7f, 1.0f, 0.7f, 1.0f);
			Ui()->DoLabel(&RestartWarning, Localize("BestClient update downloaded! Restart to apply."), 14.0f, TEXTALIGN_ML);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
		else
		{
			Ui()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), 14.0f, TEXTALIGN_ML);
		}

		static CButtonContainer s_RestartButton;
		if(DoButton_Menu(&s_RestartButton, Localize("Restart"), 0, &RestartButton))
		{
#if defined(CONF_AUTOUPDATE)
			if(m_NeedRestartUpdate)
			{
				Updater()->ApplyUpdateAndRestart();
			}
			else
#endif
			if(Client()->State() == IClient::STATE_ONLINE || GameClient()->Editor()->HasUnsavedData())
			{
				m_Popup = POPUP_RESTART;
			}
			else
			{
				Client()->Restart();
			}
		}
	}
}

CUi::EPopupMenuFunctionResult CMenus::PopupSettingsCountrySelection(void *pContext, CUIRect View, bool Active)
{
	SPopupSettingsCountrySelectionContext *pPopupContext = static_cast<SPopupSettingsCountrySelectionContext *>(pContext);
	CMenus *pMenus = pPopupContext->m_pMenus;

	static CListBox s_ListBox;
	s_ListBox.SetActive(Active);
	s_ListBox.DoStart(50.0f, pMenus->GameClient()->m_CountryFlags.Num(), 8, 1, -1, &View, false);

	if(pPopupContext->m_New)
	{
		pPopupContext->m_New = false;
		s_ListBox.ScrollToSelected();
	}

	for(size_t i = 0; i < pMenus->GameClient()->m_CountryFlags.Num(); ++i)
	{
		const CCountryFlags::CCountryFlag &Entry = pMenus->GameClient()->m_CountryFlags.GetByIndex(i);
		const CListboxItem Item = s_ListBox.DoNextItem(&Entry, Entry.m_CountryCode == pPopupContext->m_Selection);
		if(!Item.m_Visible)
			continue;

		CUIRect FlagRect, Label;
		Item.m_Rect.Margin(5.0f, &FlagRect);
		FlagRect.HSplitBottom(12.0f, &FlagRect, &Label);
		Label.HSplitTop(2.0f, nullptr, &Label);
		const float OldWidth = FlagRect.w;
		FlagRect.w = FlagRect.h * 2.0f;
		FlagRect.x += (OldWidth - FlagRect.w) / 2.0f;
		pMenus->GameClient()->m_CountryFlags.Render(Entry.m_CountryCode, ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f), FlagRect.x, FlagRect.y, FlagRect.w, FlagRect.h);
		pMenus->Ui()->DoLabel(&Label, Entry.m_aCountryCodeString, 10.0f, TEXTALIGN_MC);
	}

	const int NewSelected = s_ListBox.DoEnd();
	pPopupContext->m_Selection = NewSelected >= 0 ? pMenus->GameClient()->m_CountryFlags.GetByIndex(NewSelected).m_CountryCode : -1;
	if(s_ListBox.WasItemSelected() || s_ListBox.WasItemActivated())
	{
		if(pPopupContext->m_pCountry != nullptr)
		{
			*pPopupContext->m_pCountry = pPopupContext->m_Selection;
			pMenus->SetNeedSendInfo();
		}
		return CUi::POPUP_CLOSE_CURRENT;
	}

	return CUi::POPUP_KEEP_OPEN;
}

bool CMenus::RenderHslaScrollbars(CUIRect *pRect, unsigned int *pColor, bool Alpha, float DarkestLight)
{
	const unsigned PrevPackedColor = *pColor;
	ColorHSLA Color(*pColor, Alpha);
	const ColorHSLA OriginalColor = Color;
	const char *apLabels[] = {Localize("Hue"), Localize("Sat."), Localize("Lht."), Localize("Alpha")};
	const float SizePerEntry = 20.0f;
	const float MarginPerEntry = 5.0f;
	const float PreviewMargin = 2.5f;
	const float PreviewHeight = 40.0f + 2 * PreviewMargin;
	const float OffY = (SizePerEntry + MarginPerEntry) * (3 + (Alpha ? 1 : 0)) - PreviewHeight;

	CUIRect Preview;
	pRect->VSplitLeft(PreviewHeight, &Preview, pRect);
	Preview.HSplitTop(OffY / 2.0f, nullptr, &Preview);
	Preview.HSplitTop(PreviewHeight, &Preview, nullptr);

	Preview.Draw(ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f), IGraphics::CORNER_ALL, 4.0f + PreviewMargin);
	Preview.Margin(PreviewMargin, &Preview);
	Preview.Draw(color_cast<ColorRGBA>(Color.UnclampLighting(DarkestLight)), IGraphics::CORNER_ALL, 4.0f + PreviewMargin);

	auto &&RenderHueRect = [&](CUIRect *pColorRect) {
		float CurXOff = pColorRect->x;
		const float SizeColor = pColorRect->w / 6;

		{
			IGraphics::CColorVertex aColorVertices[] = {IGraphics::CColorVertex(0, 1, 0, 0, 1), IGraphics::CColorVertex(1, 1, 1, 0, 1), IGraphics::CColorVertex(2, 1, 0, 0, 1), IGraphics::CColorVertex(3, 1, 1, 0, 1)};
			Graphics()->SetColorVertex(aColorVertices, std::size(aColorVertices));
			IGraphics::CFreeformItem Freeform(CurXOff, pColorRect->y, CurXOff + SizeColor, pColorRect->y, CurXOff, pColorRect->y + pColorRect->h, CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}
		CurXOff += SizeColor;
		{
			IGraphics::CColorVertex aColorVertices[] = {IGraphics::CColorVertex(0, 1, 1, 0, 1), IGraphics::CColorVertex(1, 0, 1, 0, 1), IGraphics::CColorVertex(2, 1, 1, 0, 1), IGraphics::CColorVertex(3, 0, 1, 0, 1)};
			Graphics()->SetColorVertex(aColorVertices, std::size(aColorVertices));
			IGraphics::CFreeformItem Freeform(CurXOff, pColorRect->y, CurXOff + SizeColor, pColorRect->y, CurXOff, pColorRect->y + pColorRect->h, CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}
		CurXOff += SizeColor;
		{
			IGraphics::CColorVertex aColorVertices[] = {IGraphics::CColorVertex(0, 0, 1, 0, 1), IGraphics::CColorVertex(1, 0, 1, 1, 1), IGraphics::CColorVertex(2, 0, 1, 0, 1), IGraphics::CColorVertex(3, 0, 1, 1, 1)};
			Graphics()->SetColorVertex(aColorVertices, std::size(aColorVertices));
			IGraphics::CFreeformItem Freeform(CurXOff, pColorRect->y, CurXOff + SizeColor, pColorRect->y, CurXOff, pColorRect->y + pColorRect->h, CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}
		CurXOff += SizeColor;
		{
			IGraphics::CColorVertex aColorVertices[] = {IGraphics::CColorVertex(0, 0, 1, 1, 1), IGraphics::CColorVertex(1, 0, 0, 1, 1), IGraphics::CColorVertex(2, 0, 1, 1, 1), IGraphics::CColorVertex(3, 0, 0, 1, 1)};
			Graphics()->SetColorVertex(aColorVertices, std::size(aColorVertices));
			IGraphics::CFreeformItem Freeform(CurXOff, pColorRect->y, CurXOff + SizeColor, pColorRect->y, CurXOff, pColorRect->y + pColorRect->h, CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}
		CurXOff += SizeColor;
		{
			IGraphics::CColorVertex aColorVertices[] = {IGraphics::CColorVertex(0, 0, 0, 1, 1), IGraphics::CColorVertex(1, 1, 0, 1, 1), IGraphics::CColorVertex(2, 0, 0, 1, 1), IGraphics::CColorVertex(3, 1, 0, 1, 1)};
			Graphics()->SetColorVertex(aColorVertices, std::size(aColorVertices));
			IGraphics::CFreeformItem Freeform(CurXOff, pColorRect->y, CurXOff + SizeColor, pColorRect->y, CurXOff, pColorRect->y + pColorRect->h, CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}
		CurXOff += SizeColor;
		{
			IGraphics::CColorVertex aColorVertices[] = {IGraphics::CColorVertex(0, 1, 0, 1, 1), IGraphics::CColorVertex(1, 1, 0, 0, 1), IGraphics::CColorVertex(2, 1, 0, 1, 1), IGraphics::CColorVertex(3, 1, 0, 0, 1)};
			Graphics()->SetColorVertex(aColorVertices, std::size(aColorVertices));
			IGraphics::CFreeformItem Freeform(CurXOff, pColorRect->y, CurXOff + SizeColor, pColorRect->y, CurXOff, pColorRect->y + pColorRect->h, CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}
	};

	auto &&RenderSaturationRect = [&](CUIRect *pColorRect, const ColorRGBA &CurColor) {
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColor);
		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColor);
		LeftColor.s = 0.0f;
		RightColor.s = 1.0f;
		const ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);
		const ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);
		Graphics()->SetColor4(LeftColorRGBA, RightColorRGBA, RightColorRGBA, LeftColorRGBA);
		IGraphics::CFreeformItem Freeform(pColorRect->x, pColorRect->y, pColorRect->x + pColorRect->w, pColorRect->y, pColorRect->x, pColorRect->y + pColorRect->h, pColorRect->x + pColorRect->w, pColorRect->y + pColorRect->h);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	};

	auto &&RenderLightingRect = [&](CUIRect *pColorRect, const ColorRGBA &CurColor) {
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColor);
		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColor);
		LeftColor.l = DarkestLight;
		RightColor.l = 1.0f;
		const ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);
		const ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);
		Graphics()->SetColor4(LeftColorRGBA, RightColorRGBA, RightColorRGBA, LeftColorRGBA);
		IGraphics::CFreeformItem Freeform(pColorRect->x, pColorRect->y, pColorRect->x + pColorRect->w, pColorRect->y, pColorRect->x, pColorRect->y + pColorRect->h, pColorRect->x + pColorRect->w, pColorRect->y + pColorRect->h);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	};

	auto &&RenderAlphaRect = [&](CUIRect *pColorRect, const ColorRGBA &CurColorFull) {
		const ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(color_cast<ColorHSLA>(CurColorFull).WithAlpha(0.0f));
		const ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(color_cast<ColorHSLA>(CurColorFull).WithAlpha(1.0f));
		Graphics()->SetColor4(LeftColorRGBA, RightColorRGBA, RightColorRGBA, LeftColorRGBA);
		IGraphics::CFreeformItem Freeform(pColorRect->x, pColorRect->y, pColorRect->x + pColorRect->w, pColorRect->y, pColorRect->x, pColorRect->y + pColorRect->h, pColorRect->x + pColorRect->w, pColorRect->y + pColorRect->h);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	};

	for(int i = 0; i < 3 + Alpha; i++)
	{
		CUIRect Button, Label;
		pRect->HSplitTop(SizePerEntry, &Button, pRect);
		pRect->HSplitTop(MarginPerEntry, nullptr, pRect);
		Button.VSplitLeft(140.0f, &Label, &Button);
		Label.VMargin(10.0f, &Label);
		Button.Draw(ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f), IGraphics::CORNER_ALL, 1.0f);

		CUIRect Rail;
		Button.Margin(2.0f, &Rail);
		char aBuf[32];
		if(i == 0)
			str_format(aBuf, sizeof(aBuf), "%s: %.1f° (%03d)", apLabels[i], Color[i] * 360.0f, round_to_int(Color[i] * 255.0f));
		else if(i == 2)
		{
			float Lht = DarkestLight + Color[i] * (1.0f - DarkestLight);
			str_format(aBuf, sizeof(aBuf), "%s: %.1f%% (%03d)", apLabels[i], Lht * 100.0f, round_to_int(Color[i] * 255.0f));
		}
		else
			str_format(aBuf, sizeof(aBuf), "%s: %.1f%% (%03d)", apLabels[i], Color[i] * 100.0f, round_to_int(Color[i] * 255.0f));
		Ui()->DoLabel(&Label, aBuf, 12.0f, TEXTALIGN_ML);

		ColorRGBA HandleColor;
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();
		if(i == 0)
		{
			RenderHueRect(&Rail);
			HandleColor = color_cast<ColorRGBA>(ColorHSLA(Color.h, 1.0f, 0.5f, 1.0f));
		}
		else if(i == 1)
		{
			RenderSaturationRect(&Rail, color_cast<ColorRGBA>(ColorHSLA(Color.h, 1.0f, 0.5f, 1.0f)));
			HandleColor = color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, 0.5f, 1.0f));
		}
		else if(i == 2)
		{
			RenderLightingRect(&Rail, color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, Color.l, 1.0f)));
			HandleColor = color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, Color.l, 1.0f).UnclampLighting(DarkestLight));
		}
		else if(i == 3)
		{
			RenderAlphaRect(&Rail, color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, Color.l, 1.0f).UnclampLighting(DarkestLight)));
			HandleColor = color_cast<ColorRGBA>(Color.UnclampLighting(DarkestLight));
		}
		Graphics()->TrianglesEnd();
		Color[i] = Ui()->DoScrollbarH(&((char *)pColor)[i], &Button, Color[i], &HandleColor);
	}

	if(OriginalColor != Color)
		*pColor = Color.Pack(Alpha);
	return PrevPackedColor != *pColor;
}
