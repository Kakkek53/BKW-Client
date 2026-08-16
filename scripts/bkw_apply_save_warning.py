from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: str, old: str, new: str):
    p = ROOT / path
    text = p.read_text(encoding="utf-8")
    if old not in text:
        raise RuntimeError(f"pattern not found in {path}: {old[:120]!r}")
    text = text.replace(old, new, 1)
    p.write_text(text, encoding="utf-8")


def replace_last(path: str, old: str, new: str):
    p = ROOT / path
    text = p.read_text(encoding="utf-8")
    pos = text.rfind(old)
    if pos < 0:
        raise RuntimeError(f"last pattern not found in {path}: {old[:120]!r}")
    text = text[:pos] + new + text[pos + len(old):]
    p.write_text(text, encoding="utf-8")


# menus.h: expose the small BKW exit-warning helpers to menus_ingame.cpp.
replace_once(
    "src/game/client/components/menus.h",
    "\tvoid ShowQuitPopup();\n\tvoid JoinTutorial();",
    "\tvoid ShowQuitPopup();\n\tbool BkwShouldWarnUnsavedProgress();\n\tbool BkwCanSaveCurrentProgress();\n\tbool BkwSaveCurrentProgress();\n\tvoid BkwOpenUnsavedProgressWarning(bool QuitGame);\n\tvoid JoinTutorial();",
)

# menus.cpp includes.
replace_once(
    "src/game/client/components/menus.cpp",
    "#include <game/client/components/binds.h>\n#include <game/client/components/console.h>",
    "#include <game/client/components/binds.h>\n#include <game/client/components/bkw/save_store.h>\n#include <game/client/components/bkw/save_warning.h>\n#include <game/client/components/console.h>",
)
replace_once(
    "src/game/client/components/menus.cpp",
    "#include <chrono>\n#include <cmath>\n#include <vector>",
    "#include <chrono>\n#include <cmath>\n#include <ctime>\n#include <string>\n#include <vector>",
)

# Quit button: BKW warning gets priority, then original DDNet confirmations.
replace_once(
    "src/game/client/components/menus.cpp",
    "\tif(DoButton_MenuTab(&s_QuitButton, FontIcon::POWER_OFF, 0, &Button, IGraphics::CORNER_T, &m_aAnimatorsSmallPage[SMALL_TAB_QUIT], nullptr, nullptr, &QuitColor, 10.0f))\n\t{\n\t\tif(GameClient()->Editor()->HasUnsavedData() || (GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmQuitTime && g_Config.m_ClConfirmQuitTime >= 0) || m_MenusIngameTouchControls.UnsavedChanges() || GameClient()->m_TouchControls.HasEditingChanges())\n\t\t{\n\t\t\tm_Popup = POPUP_QUIT;\n\t\t}\n\t\telse\n\t\t{\n\t\t\tClient()->Quit();\n\t\t}\n\t}",
    "\tif(DoButton_MenuTab(&s_QuitButton, FontIcon::POWER_OFF, 0, &Button, IGraphics::CORNER_T, &m_aAnimatorsSmallPage[SMALL_TAB_QUIT], nullptr, nullptr, &QuitColor, 10.0f))\n\t{\n\t\tif(BkwShouldWarnUnsavedProgress())\n\t\t{\n\t\t\tBkwOpenUnsavedProgressWarning(true);\n\t\t}\n\t\telse if(GameClient()->Editor()->HasUnsavedData() || (GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmQuitTime && g_Config.m_ClConfirmQuitTime >= 0) || m_MenusIngameTouchControls.UnsavedChanges() || GameClient()->m_TouchControls.HasEditingChanges())\n\t\t{\n\t\t\tm_Popup = POPUP_QUIT;\n\t\t}\n\t\telse\n\t\t{\n\t\t\tClient()->Quit();\n\t\t}\n\t}",
)

# Add BKW helper implementations immediately before ShowQuitPopup.
replace_once(
    "src/game/client/components/menus.cpp",
    "void CMenus::ShowQuitPopup()\n{\n\tm_Popup = POPUP_QUIT;\n}",
    r'''bool CMenus::BkwShouldWarnUnsavedProgress()
{
	auto &Warning = Bkw::SaveWarningState();
	Warning.Load(Storage());
	if(Client()->State() != IClient::STATE_ONLINE || !Warning.HasUnsavedProgress())
		return false;

	const CServerInfo &Info = Client()->ServerInfo();
	if(Bkw::CSaveWarningState::ListMatches(Warning.m_ExcludedIps, Info.m_aAddress) ||
		Bkw::CSaveWarningState::ListMatches(Warning.m_ExcludedCommunities, Info.m_aCommunityId) ||
		Bkw::CSaveWarningState::ListMatches(Warning.m_ExcludedGameTypes, Info.m_aGameType))
		return false;

	if(const CCommunity *pCommunity = ServerBrowser()->Community(Info.m_aCommunityId))
	{
		if(Bkw::CSaveWarningState::ListMatches(Warning.m_ExcludedCommunities, pCommunity->Name()))
			return false;
	}
	return true;
}

bool CMenus::BkwCanSaveCurrentProgress()
{
	if(Client()->State() != IClient::STATE_ONLINE)
		return false;
	const int LocalClientId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalClientId < 0 || LocalClientId >= MAX_CLIENTS)
		return false;
	const int Team = GameClient()->m_Teams.Team(LocalClientId);
	return Team > TEAM_FLOCK && Team < GameClient()->m_Teams.TeamSuper();
}

bool CMenus::BkwSaveCurrentProgress()
{
	if(!BkwCanSaveCurrentProgress())
		return false;

	const int LocalClientId = GameClient()->m_Snap.m_LocalClientId;
	const int Team = GameClient()->m_Teams.Team(LocalClientId);
	std::vector<std::string> vPlayers;
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ++ClientId)
	{
		if(!GameClient()->m_aClients[ClientId].m_Active || GameClient()->m_Teams.Team(ClientId) != Team)
			continue;
		if(GameClient()->m_aClients[ClientId].m_aName[0] != '\0')
			vPlayers.emplace_back(GameClient()->m_aClients[ClientId].m_aName);
	}
	if(vPlayers.empty())
		return false;
	std::sort(vPlayers.begin(), vPlayers.end());

	Bkw::CSaveStore Store;
	Store.Load(Storage());
	Bkw::SSaveEntry Entry;
	Entry.m_Key = Store.NextFreeKey();
	Entry.m_ServerAddress = Client()->ServerInfo().m_aAddress;
	Entry.m_Map = Client()->ServerInfo().m_aMap;
	Entry.m_SavedAtUnix = (long long)std::time(nullptr);
	Entry.m_vPlayers = std::move(vPlayers);

	char aCommand[128];
	str_format(aCommand, sizeof(aCommand), "/save %s", Entry.m_Key.c_str());
	GameClient()->m_Chat.SendChat(0, aCommand);
	Store.Add(std::move(Entry));
	Store.Save(Storage());
	Bkw::SaveWarningState().MarkSaved();
	return true;
}

void CMenus::BkwOpenUnsavedProgressWarning(bool QuitGame)
{
	auto &Warning = Bkw::SaveWarningState();
	Warning.Load(Storage());
	Warning.m_PopupActive = true;
	Warning.m_ExitAction = QuitGame ? Bkw::ESaveWarningExitAction::QUIT : Bkw::ESaveWarningExitAction::DISCONNECT;
	Warning.m_ExitAfterSaveAt = 0;
	m_Popup = POPUP_CONFIRM;
	SetActive(true);
}

void CMenus::ShowQuitPopup()
{
	if(BkwShouldWarnUnsavedProgress())
		BkwOpenUnsavedProgressWarning(true);
	else
		m_Popup = POPUP_QUIT;
}''',
)

# Render a dedicated 3-button BKW popup before the normal fullscreen-popup switch.
replace_once(
    "src/game/client/components/menus.cpp",
    "void CMenus::RenderPopupFullscreen(CUIRect Screen)\n{\n\tchar aBuf[1536];",
    r'''void CMenus::RenderPopupFullscreen(CUIRect Screen)
{
	auto &BkwWarning = Bkw::SaveWarningState();
	if(BkwWarning.m_PopupActive)
	{
		if(BkwWarning.m_ExitAfterSaveAt > 0 && time_get() >= BkwWarning.m_ExitAfterSaveAt)
		{
			const Bkw::ESaveWarningExitAction Action = BkwWarning.m_ExitAction;
			BkwWarning.ResetRace();
			m_Popup = POPUP_NONE;
			SetActive(false);
			if(Action == Bkw::ESaveWarningExitAction::QUIT)
				Client()->Quit();
			else if(Action == Bkw::ESaveWarningExitAction::DISCONNECT)
				Client()->Disconnect();
			return;
		}

		Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.30f), IGraphics::CORNER_ALL, 0.0f);
		CUIRect Box = Screen;
		Box.VMargin(maximum(20.0f, Screen.w * 0.17f), &Box);
		Box.HMargin(maximum(20.0f, (Screen.h - 245.0f) * 0.5f), &Box);
		Box.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.97f), IGraphics::CORNER_ALL, 10.0f);
		Box.Margin(18.0f, &Box);

		CUIRect Title, Message, Buttons;
		Box.HSplitTop(34.0f, &Title, &Box);
		Ui()->DoLabel(&Title, "Несохранённый прогресс", 24.0f, TEXTALIGN_MC);
		Box.HSplitTop(14.0f, nullptr, &Box);
		Box.HSplitTop(88.0f, &Message, &Box);
		if(BkwWarning.m_ExitAfterSaveAt > 0)
		{
			Ui()->DoLabel(&Message, "Сохранение отправлено серверу. Выход...", 15.0f, TEXTALIGN_MC);
			return;
		}

		const bool CanSave = BkwCanSaveCurrentProgress();
		const char *pMessage = CanSave ?
			"Вы начали прохождение и ещё не сохранили прогресс.\nСохранить команду перед выходом?" :
			"Вы начали прохождение и ещё не сохранили прогресс.\nДля автосохранения нужно находиться в отдельной /team.";
		Ui()->DoLabel(&Message, pMessage, 14.0f, TEXTALIGN_MC);

		Box.HSplitBottom(38.0f, &Box, &Buttons);
		CUIRect ForceButton, SaveButton, CancelButton, Rest;
		Buttons.VSplitLeft(Buttons.w / 3.0f - 4.0f, &ForceButton, &Rest);
		Rest.VSplitLeft(6.0f, nullptr, &Rest);
		Rest.VSplitLeft(Rest.w / 2.0f - 3.0f, &SaveButton, &Rest);
		Rest.VSplitLeft(6.0f, nullptr, &CancelButton);

		static CButtonContainer s_BkwForceExitButton;
		static CButtonContainer s_BkwSaveExitButton;
		static CButtonContainer s_BkwCancelExitButton;

		const bool Escape = Input()->KeyPress(KEY_ESCAPE);
		if(DoButton_Menu(&s_BkwCancelExitButton, "Отмена", 0, &CancelButton) || Escape)
		{
			BkwWarning.m_PopupActive = false;
			BkwWarning.m_ExitAction = Bkw::ESaveWarningExitAction::NONE;
			BkwWarning.m_ExitAfterSaveAt = 0;
			m_Popup = POPUP_NONE;
			return;
		}
		if(DoButton_Menu(&s_BkwForceExitButton, "Всё равно выйти", 0, &ForceButton))
		{
			const Bkw::ESaveWarningExitAction Action = BkwWarning.m_ExitAction;
			BkwWarning.ResetRace();
			m_Popup = POPUP_NONE;
			SetActive(false);
			if(Action == Bkw::ESaveWarningExitAction::QUIT)
				Client()->Quit();
			else
				Client()->Disconnect();
			return;
		}
		if(CanSave)
		{
			if(DoButton_Menu(&s_BkwSaveExitButton, "Сохраниться и выйти", 0, &SaveButton) && BkwSaveCurrentProgress())
			{
				// Give the reliable chat command a short moment to reach the server before closing the connection.
				BkwWarning.m_ExitAfterSaveAt = time_get() + time_freq() * 3 / 4;
			}
		}
		else
		{
			SaveButton.Draw(ColorRGBA(0.18f, 0.18f, 0.18f, 0.55f), IGraphics::CORNER_ALL, 5.0f);
			TextRender()->TextColor(0.55f, 0.55f, 0.55f, 1.0f);
			Ui()->DoLabel(&SaveButton, "Сохраниться и выйти", 12.0f, TEXTALIGN_MC);
			TextRender()->TextColor(TextRender()->DefaultTextColor());
		}
		return;
	}

	char aBuf[1536];''',
)

# Disconnect button: BKW warning gets priority over DDNet's time/editor confirmation.
replace_once(
    "src/game/client/components/menus_ingame.cpp",
    "\tif(DoButton_Menu(&s_DisconnectButton, Localize(\"Disconnect\"), 0, &Button))\n\t{\n\t\tif((GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmDisconnectTime && g_Config.m_ClConfirmDisconnectTime >= 0) ||",
    "\tif(DoButton_Menu(&s_DisconnectButton, Localize(\"Disconnect\"), 0, &Button))\n\t{\n\t\tif(BkwShouldWarnUnsavedProgress())\n\t\t{\n\t\t\tBkwOpenUnsavedProgressWarning(false);\n\t\t}\n\t\telse if((GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmDisconnectTime && g_Config.m_ClConfirmDisconnectTime >= 0) ||",
)

# menus_settings.cpp: warning settings + integration with BKW saves.
replace_once(
    "src/game/client/components/menus_settings.cpp",
    "#include <game/client/components/bkw/save_store.h>\n#include <game/client/components/menu_background.h>",
    "#include <game/client/components/bkw/save_store.h>\n#include <game/client/components/bkw/save_warning.h>\n#include <game/client/components/menu_background.h>",
)
replace_once(
    "src/game/client/components/menus_settings.cpp",
    "\t\tBKW_TAB_CLANS,\n\t\tBKW_TAB_LENGTH,",
    "\t\tBKW_TAB_CLANS,\n\t\tBKW_TAB_EXIT,\n\t\tBKW_TAB_LENGTH,",
)
replace_once(
    "src/game/client/components/menus_settings.cpp",
    "\t\tstatic CLineInputBuffered<64> s_ClansCommandInput;\n\t\tstatic bool s_ClansCommandInitialized = false;\n\t\tstatic CButtonContainer s_aBkwTabButtons[BKW_TAB_LENGTH];",
    "\t\tstatic CLineInputBuffered<64> s_ClansCommandInput;\n\t\tstatic bool s_ClansCommandInitialized = false;\n\t\tstatic int s_SaveWarningToggleId;\n\t\tstatic CLineInputBuffered<256> s_SaveWarningIpsInput;\n\t\tstatic CLineInputBuffered<256> s_SaveWarningCommunitiesInput;\n\t\tstatic CLineInputBuffered<256> s_SaveWarningGameTypesInput;\n\t\tstatic bool s_SaveWarningInputsInitialized = false;\n\t\tstatic CButtonContainer s_aBkwTabButtons[BKW_TAB_LENGTH];",
)
replace_once(
    "src/game/client/components/menus_settings.cpp",
    "\t\ts_SaveStore.Load(Storage());\n\t\tif(!s_PlayerInfoCommandInitialized)",
    "\t\ts_SaveStore.Load(Storage());\n\t\tauto &SaveWarning = Bkw::SaveWarningState();\n\t\tSaveWarning.Load(Storage());\n\t\tif(!s_SaveWarningInputsInitialized)\n\t\t{\n\t\t\ts_SaveWarningIpsInput.Set(SaveWarning.m_ExcludedIps.c_str());\n\t\t\ts_SaveWarningCommunitiesInput.Set(SaveWarning.m_ExcludedCommunities.c_str());\n\t\t\ts_SaveWarningGameTypesInput.Set(SaveWarning.m_ExcludedGameTypes.c_str());\n\t\t\ts_SaveWarningInputsInitialized = true;\n\t\t}\n\t\tif(!s_PlayerInfoCommandInitialized)",
)
replace_once(
    "src/game/client/components/menus_settings.cpp",
    "\t\tconst char *apBkwTabs[BKW_TAB_LENGTH] = {\"Сохранение\", \"Чекпоинты\", \"Игрок\", \"Кланы\"};",
    "\t\tconst char *apBkwTabs[BKW_TAB_LENGTH] = {\"Сохранение\", \"Чекпоинты\", \"Игрок\", \"Кланы\", \"Выход\"};",
)
replace_once(
    "src/game/client/components/menus_settings.cpp",
    "\t\t\t\tGameClient()->m_Chat.SendChat(0, aCommand);\n\t\t\t\ts_SaveStore.Add(std::move(Entry));\n\t\t\t\ts_SaveStore.Save(Storage());",
    "\t\t\t\tGameClient()->m_Chat.SendChat(0, aCommand);\n\t\t\t\ts_SaveStore.Add(std::move(Entry));\n\t\t\t\ts_SaveStore.Save(Storage());\n\t\t\t\tBkw::SaveWarningState().MarkSaved();",
)
replace_once(
    "src/game/client/components/menus_settings.cpp",
    "\t\t\t\t\t\tGameClient()->m_Chat.SendChat(0, aCommand);\n\t\t\t\t\t\ts_SaveStore.Remove(Index);",
    "\t\t\t\t\t\tGameClient()->m_Chat.SendChat(0, aCommand);\n\t\t\t\t\t\tBkw::SaveWarningState().MarkLoaded();\n\t\t\t\t\t\ts_SaveStore.Remove(Index);",
)

# Add the fifth BKW page just before the RenderBkwPage lambda closes.
exit_page = r'''
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
'''
replace_last(
    "src/game/client/components/menus_settings.cpp",
    "\t\t}\n\t};\n\n\tif(g_Config.m_BcSettingsLayout == 0)",
    "\t\t}\n" + exit_page + "\t};\n\n\tif(g_Config.m_BcSettingsLayout == 0)",
)

print("BKW save warning patch applied")
