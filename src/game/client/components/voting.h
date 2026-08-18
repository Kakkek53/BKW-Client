/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_VOTING_H
#define GAME_CLIENT_COMPONENTS_VOTING_H

#include <base/str.h>

#include <engine/client.h>
#include <engine/console.h>
#include <engine/keys.h>
#include <engine/shared/memheap.h>

#include <game/client/component.h>
#include <game/client/ui_rect.h>
#include <game/voting.h>

class CVoting : public CComponent
{
	CHeap m_Heap;

	static void ConCallvote(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);

	int64_t m_Opentime;
	int64_t m_Closetime;
	char m_aDescription[VOTE_DESC_LENGTH];
	char m_aReason[VOTE_REASON_LENGTH];
	int m_Voted;
	int m_Yes, m_No, m_Pass, m_Total;
	bool m_ReceivingOptions;

	int m_NumVoteOptions;
	CVoteOptionClient *m_pFirst;
	CVoteOptionClient *m_pLast;

	CVoteOptionClient *m_pRecycleFirst;
	CVoteOptionClient *m_pRecycleLast;

	void RemoveOption(const char *pDescription);
	void ClearOptions();
	void Callvote(const char *pType, const char *pValue, const char *pReason);

	void RenderBars(CUIRect Bars) const;

public:
	friend class CTClient; // TClient

	CVoting();
	int Sizeof() const override { return sizeof(*this); }
	void OnReset() override;
	void OnConsoleInit() override;
	void OnMessage(int MsgType, void *pRawMsg) override;
	bool OnInput(const IInput::CEvent &Event) override
	{
		if(Event.m_Key != KEY_F6 || !(Event.m_Flags & IInput::FLAG_PRESS) || (Event.m_Flags & IInput::FLAG_REPEAT))
			return false;

		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", "===== TeeFusion vote options dump =====");

		char aUserName[64] = "";
		int Index = 0;
		for(const CVoteOptionClient *pOption = m_pFirst; pOption; pOption = pOption->m_pNext, ++Index)
		{
			char aLine[VOTE_DESC_LENGTH + 32];
			str_format(aLine, sizeof(aLine), "[%d] %s", Index, pOption->m_aDescription);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", aLine);

			if(aUserName[0] == '\0')
			{
				const char *pLabel = str_utf8_find_nocase(pOption->m_aDescription, "Имя пользователя");
				if(!pLabel)
					pLabel = str_utf8_find_nocase(pOption->m_aDescription, "Username");
				if(pLabel)
				{
					const char *pValue = str_find(pLabel, ":");
					if(pValue)
					{
						++pValue;
						while(*pValue == ' ' || *pValue == '\t' || *pValue == '-' || *pValue == '|')
							++pValue;
						if(*pValue != '\0')
						{
							str_copy(aUserName, pValue, sizeof(aUserName));
							int Length = str_length(aUserName);
							while(Length > 0 && (aUserName[Length - 1] == ' ' || aUserName[Length - 1] == '\t' || aUserName[Length - 1] == '\r' || aUserName[Length - 1] == '\n' || aUserName[Length - 1] == '|'))
								aUserName[--Length] = '\0';
						}
					}
				}
			}
		}

		char aSummary[256];
		if(aUserName[0] != '\0')
			str_format(aSummary, sizeof(aSummary), "Получено vote options: %d. Имя пользователя: %s", m_NumVoteOptions, aUserName);
		else
			str_format(aSummary, sizeof(aSummary), "Получено vote options: %d. Строка имени не найдена. Откройте локальную консоль для полного списка.", m_NumVoteOptions);

		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", aSummary);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tf-vote", m_ReceivingOptions ? "Сервер еще отправляет группу vote options." : "Группа vote options сейчас завершена.");
		Client()->Notify("TeeFusion vote parser", aSummary);
		return true;
	}

	void Render(bool ForcePreview = false);
	CUIRect GetHudRect(float HudWidth, float HudHeight, bool ForcePreview = false) const;

	void CallvoteSpectate(int ClientId, const char *pReason, bool ForceVote = false);
	void CallvoteKick(int ClientId, const char *pReason, bool ForceVote = false);
	void CallvoteOption(int OptionId, const char *pReason, bool ForceVote = false);
	void RemovevoteOption(int OptionId);
	void AddvoteOption(const char *pDescription, const char *pCommand);
	void AddOption(const char *pDescription);

	void Vote(int v); // -1 = no, 1 = yes

	int SecondsLeft() const;
	bool IsVoting() const { return m_Closetime != 0; }
	int TakenChoice() const { return m_Voted; }
	const char *VoteDescription() const { return m_aDescription; }
	const char *VoteReason() const { return m_aReason; }
	bool IsReceivingOptions() const { return m_ReceivingOptions; }
	int NumOptions() const { return m_NumVoteOptions; }
	const CVoteOptionClient *FirstOption() const { return m_pFirst; }
};

#endif