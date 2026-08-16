#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_BINDCHAT_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_BINDCHAT_H

#include <base/math.h>

#include <engine/console.h>

#include <game/client/component.h>
#include <game/client/lineinput.h>

#include <memory>
#include <string>

class IConfigManager;
class IHttpRequest;
struct _json_value;
typedef struct _json_value json_value;

enum
{
	BINDCHAT_MAX_NAME = 64,
	BINDCHAT_MAX_CMD = 1024,
	BINDCHAT_MAX_BINDS = 256,
	BKW_PLAYER_INFO_COMMAND_MAX = 64,
	BKW_PLAYER_INFO_NAME_MAX = 128,
	BKW_CLANS_COMMAND_MAX = 64,
	BKW_CLANS_NAME_MAX = 128,
};

class CBindChat : public CComponent
{
public:
	class CBind
	{
	public:
		char m_aName[BINDCHAT_MAX_NAME];
		char m_aCommand[BINDCHAT_MAX_CMD];
		CBind() = default;
		CBind(const char *pName, const char *pCommand);
		bool CompContent(const CBind &Other) const;
	};
	class CBindDefault
	{
	public:
		const char *m_pTitle;
		CBind m_Bind;
		CLineInput m_LineInput;
	};
	static std::vector<std::pair<const char *, std::vector<CBindDefault>>> BIND_DEFAULTS;

private:
	static void ConAddBindchat(IConsole::IResult *pResult, void *pUserData);
	static void ConBindchats(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveBindchat(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveBindchatAll(IConsole::IResult *pResult, void *pUserData);
	static void ConBindchatDefaults(IConsole::IResult *pResult, void *pUserData);
	static void ConBkwPlayerInfo(IConsole::IResult *pResult, void *pUserData);
	static void ConBkwPlayerInfoEnabled(IConsole::IResult *pResult, void *pUserData);
	static void ConBkwPlayerInfoChatCommand(IConsole::IResult *pResult, void *pUserData);
	static void ConBkwClans(IConsole::IResult *pResult, void *pUserData);
	static void ConBkwClansEnabled(IConsole::IResult *pResult, void *pUserData);
	static void ConBkwClansChatCommand(IConsole::IResult *pResult, void *pUserData);

	static void ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData);

	void ExecuteBind(const CBind &Bind, const char *pArgs);

	bool m_BkwPlayerInfoEnabled = true;
	char m_aBkwPlayerInfoChatCommand[BKW_PLAYER_INFO_COMMAND_MAX] = ".player";
	char m_aBkwPlayerInfoRequestedName[BKW_PLAYER_INFO_NAME_MAX] = "";
	std::shared_ptr<IHttpRequest> m_pBkwPlayerInfoRequest;

	bool m_BkwClansEnabled = true;
	char m_aBkwClansChatCommand[BKW_CLANS_COMMAND_MAX] = ".clans";
	char m_aBkwClansRequestedName[BKW_CLANS_NAME_MAX] = "";
	std::shared_ptr<IHttpRequest> m_pBkwClansRequest;

	bool MatchBkwPlayerInfoCommand(const char *pText, const char **ppPlayerName) const;
	void StartBkwPlayerInfoRequest(const char *pPlayerName);
	void FinishBkwPlayerInfoRequest();
	void PrintBkwPlayerInfo(const json_value &Root);
	void PrintBkwPlayerInfoLine(const char *pText);
	bool MatchBkwClansCommand(const char *pText, const char **ppPlayerName) const;
	void StartBkwClansRequest(const char *pPlayerName);
	void FinishBkwClansRequest();
	void PrintBkwClansHtml(const char *pHtml, size_t HtmlSize);
	static std::string BkwUrlEncode(const char *pText);

public:
	std::vector<CBind> m_vBinds; // TODO use map

	CBindChat();
	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;
	void OnRender() override;
	void OnStateChange(int NewState, int OldState) override;

	void AddBind(const CBind &Bind);

	bool RemoveBind(const char *pName);
	void RemoveAllBinds();

	CBind *GetBind(const char *pCommand);

	bool CheckBindChat(const char *pText);
	bool ChatDoBinds(const char *pText);
	bool ChatDoAutocomplete(bool ShiftPressed);

	bool BkwPlayerInfoEnabled() const { return m_BkwPlayerInfoEnabled; }
	const char *BkwPlayerInfoChatCommand() const { return m_aBkwPlayerInfoChatCommand; }
	void SetBkwPlayerInfoEnabled(bool Enabled);
	void SetBkwPlayerInfoChatCommand(const char *pCommand);
	bool BkwClansEnabled() const { return m_BkwClansEnabled; }
	const char *BkwClansChatCommand() const { return m_aBkwClansChatCommand; }
	void SetBkwClansEnabled(bool Enabled);
	void SetBkwClansChatCommand(const char *pCommand);
};

#endif
