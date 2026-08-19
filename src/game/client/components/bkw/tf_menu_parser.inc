#ifndef GAME_CLIENT_COMPONENTS_BKW_TF_MENU_PARSER_INC
#define GAME_CLIENT_COMPONENTS_BKW_TF_MENU_PARSER_INC

#include <base/str.h>

#include <game/client/components/bkw/tf_menu_state.inc>
#include <game/client/components/voting.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

namespace BkwTfMenu
{
enum class EItemKind
{
	SECTION,
	INFO,
	NOTE,
	ACTION,
	TOGGLE,
};

enum class EIconHint
{
	GENERIC,
	HOME,
	BACK,
	USER,
	SHOP,
	GIFT,
	SETTINGS,
	TROPHY,
	CHART,
	LEVEL,
	INVENTORY,
	CLAN,
	CROWN,
	MAIL,
	GAME,
	CHECK,
	INFO,
	WARNING,
	COINS,
};

struct SItem
{
	EItemKind m_Kind = EItemKind::INFO;
	EIconHint m_Icon = EIconHint::GENERIC;
	int m_VoteIndex = -1;
	std::string m_Text;
	std::string m_Detail;
	bool m_Checked = false;
	bool m_Negative = false;
	bool m_Warning = false;
	bool m_NeedsReason = false;
};

struct SPage
{
	std::string m_Title = "TeeFusion";
	EIconHint m_Icon = EIconHint::GENERIC;
	std::vector<SItem> m_vItems;
	int m_HomeVoteIndex = -1;
	int m_BackVoteIndex = -1;
	bool m_IsRoot = false;
	bool m_RequiresReason = false;
};

inline std::string Trim(std::string Text)
{
	while(!Text.empty() && (Text.front() == ' ' || Text.front() == '\t' || Text.front() == '\r' || Text.front() == '\n'))
		Text.erase(Text.begin());
	while(!Text.empty() && (Text.back() == ' ' || Text.back() == '\t' || Text.back() == '\r' || Text.back() == '\n'))
		Text.pop_back();
	return Text;
}

inline bool StartsWith(const std::string &Text, const char *pPrefix)
{
	const size_t PrefixLength = std::char_traits<char>::length(pPrefix);
	return Text.size() >= PrefixLength && Text.compare(0, PrefixLength, pPrefix) == 0;
}

inline bool Contains(const std::string &Text, const char *pNeedle)
{
	return Text.find(pNeedle) != std::string::npos;
}

inline void ReplaceAll(std::string &Text, const std::string &From, const std::string &To = "")
{
	if(From.empty())
		return;
	size_t Pos = 0;
	while((Pos = Text.find(From, Pos)) != std::string::npos)
	{
		Text.replace(Pos, From.size(), To);
		Pos += To.size();
	}
}

inline std::string CleanText(const char *pRaw)
{
	std::string Text = Trim(pRaw ? pRaw : "");

	for(;;)
	{
		const size_t Before = Text.size();
		if(StartsWith(Text, "│"))
			Text.erase(0, std::char_traits<char>::length("│"));
		if(StartsWith(Text, "►"))
			Text.erase(0, std::char_traits<char>::length("►"));
		if(StartsWith(Text, "◄"))
			Text.erase(0, std::char_traits<char>::length("◄"));
		if(StartsWith(Text, "╭─"))
			Text.erase(0, std::char_traits<char>::length("╭─"));
		Text = Trim(Text);
		if(Text.size() == Before)
			break;
	}

	ReplaceAll(Text, "[✔]");
	ReplaceAll(Text, "[✘]");
	ReplaceAll(Text, "[  ]");
	ReplaceAll(Text, "[ ]");
	ReplaceAll(Text, "⚠️");
	ReplaceAll(Text, "⚠");
	ReplaceAll(Text, "◄");
	Text = Trim(Text);

	if(StartsWith(Text, "-"))
	{
		Text.erase(0, 1);
		Text = Trim(Text);
	}
	return Text;
}

inline bool IsBoxStart(const std::string &Raw) { return StartsWith(Trim(Raw), "╭"); }
inline bool IsBoxEnd(const std::string &Raw) { return StartsWith(Trim(Raw), "╰"); }
inline bool IsBoxLine(const std::string &Raw) { return StartsWith(Trim(Raw), "│"); }
inline bool IsArrow(const std::string &Raw) { return StartsWith(Trim(Raw), "►"); }
inline bool IsBackGlyph(const std::string &Raw) { return StartsWith(Trim(Raw), "◄"); }
inline bool IsChecked(const std::string &Raw) { return Contains(Raw, "[✔]"); }
inline bool IsNegative(const std::string &Raw) { return Contains(Raw, "[✘]"); }
inline bool IsUnchecked(const std::string &Raw) { return Contains(Raw, "[ ]") || Contains(Raw, "[  ]"); }
inline bool IsToggle(const std::string &Raw) { return IsChecked(Raw) || IsUnchecked(Raw) || IsNegative(Raw); }
inline bool IsWarning(const std::string &Raw) { return Contains(Raw, "⚠"); }

inline bool IsNumbered(const std::string &Text)
{
	size_t Pos = 0;
	while(Pos < Text.size() && Text[Pos] >= '0' && Text[Pos] <= '9')
		++Pos;
	return Pos > 0 && Pos < Text.size() && Text[Pos] == '.';
}

inline bool HasPlaceholder(const std::string &Text)
{
	const size_t Open = Text.find('<');
	const size_t Close = Text.find('>', Open == std::string::npos ? 0 : Open + 1);
	return Open != std::string::npos && Close != std::string::npos;
}

inline bool ParseUserName(const CVoting &Voting, char *pBuffer, size_t BufferSize)
{
	if(!pBuffer || BufferSize == 0)
		return false;
	pBuffer[0] = '\0';

	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext)
	{
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

inline EIconHint GuessIcon(const std::string &Text)
{
	if(Contains(Text, "/clan_") || Contains(Text, "Cʟᴀɴ") || Contains(Text, "Kлaн") || Contains(Text, "клaн"))
		return EIconHint::CLAN;
	if(Contains(Text, "VIP"))
		return EIconHint::CROWN;
	if(Contains(Text, "мaгaз") || Contains(Text, "Kyпить") || Contains(Text, "пoкyп"))
		return EIconHint::SHOP;
	if(Contains(Text, "Нaгpaд") || Contains(Text, "Day ") || Contains(Text, "reward"))
		return EIconHint::GIFT;
	if(Contains(Text, "/rating_") || Contains(Text, "Peйтинг") || Contains(Text, "rating"))
		return EIconHint::TROPHY;
	if(Contains(Text, "Уpoвeнь") || Contains(Text, "Oпыт"))
		return EIconHint::LEVEL;
	if(Contains(Text, "Discord") || Contains(Text, "Mail") || Contains(Text, "Пoч"))
		return EIconHint::MAIL;
	if(Contains(Text, "Имя") || Contains(Text, "Username"))
		return EIconHint::USER;
	if(Contains(Text, "Дeньги") || Contains(Text, "Baлюты") || Contains(Text, "$") || Contains(Text, "Cash"))
		return EIconHint::COINS;
	if(Contains(Text, "Фильтpы") || Contains(Text, "пpeдмeтoв]") || Contains(Text, "Шляпы") || Contains(Text, "Keйcы"))
		return EIconHint::INVENTORY;
	if(Contains(Text, "Roulette") || Contains(Text, "Crash") || Contains(Text, "1vs1") || Contains(Text, "Vodka") || Contains(Text, "Apeнa") || Contains(Text, "KOH"))
		return EIconHint::GAME;
	return EIconHint::GENERIC;
}

inline std::string FriendlyCommand(const std::string &Command)
{
	if(StartsWith(Command, "/clan_invite"))
		return "Пригласить участника";
	if(StartsWith(Command, "/clan_kick_id"))
		return "Выгнать по ID";
	if(StartsWith(Command, "/clan_kick"))
		return "Выгнать по нику";
	if(StartsWith(Command, "/clan_members_offset"))
		return "Показать участников со смещением";
	if(StartsWith(Command, "/clan_members"))
		return "Показать участников";
	if(StartsWith(Command, "/clan_leave"))
		return "Выйти из клана";
	if(StartsWith(Command, "/clan_set_role_id owner"))
		return "Передать владение по ID";
	if(StartsWith(Command, "/clan_set_role owner"))
		return "Передать владение";
	if(StartsWith(Command, "/clan_set_role_id moder"))
		return "Назначить модератором по ID";
	if(StartsWith(Command, "/clan_set_role moder"))
		return "Назначить модератором";
	if(StartsWith(Command, "/clan_set_role_id member"))
		return "Назначить участником по ID";
	if(StartsWith(Command, "/clan_set_role member"))
		return "Назначить участником";
	if(StartsWith(Command, "/clan_rename"))
		return "Переименовать клан";
	if(StartsWith(Command, "/clan_delete"))
		return "Удалить клан";
	if(StartsWith(Command, "/rating_ranks"))
		return "Показать ранги";
	if(StartsWith(Command, "/rating_search"))
		return "Начать поиск";
	return Command;
}

inline void SplitLabelValue(const std::string &Text, std::string &Label, std::string &Value)
{
	const size_t Colon = Text.find(':');
	if(Colon == std::string::npos)
	{
		Label.clear();
		Value = Text;
		return;
	}
	Label = Trim(Text.substr(0, Colon));
	Value = Trim(Text.substr(Colon + 1));
}

inline std::string AggregateText(const CVoting &Voting)
{
	std::string Result;
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext)
	{
		Result += pOption->m_aDescription;
		Result += '\n';
	}
	return Result;
}

inline void ClassifyPage(const CVoting &Voting, SPage &Page)
{
	char aUserName[64];
	Page.m_IsRoot = ParseUserName(Voting, aUserName, sizeof(aUserName));
	if(Page.m_IsRoot)
	{
		Page.m_Title = "Главная";
		Page.m_Icon = EIconHint::HOME;
		return;
	}

	const std::string All = AggregateText(Voting);
	int ToggleCount = 0;
	int ArrowCount = 0;
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext)
	{
		const std::string Raw = pOption->m_aDescription;
		ToggleCount += IsToggle(Raw) ? 1 : 0;
		ArrowCount += IsArrow(Raw) ? 1 : 0;
	}

	if(Contains(All, "Bзнocы клaнa") || (Contains(All, "Last Deposits") && Contains(All, "ago")))
	{
		Page.m_Title = "Взносы клана";
		Page.m_Icon = EIconHint::CLAN;
		return;
	}
	if(Contains(All, "/clan_") || Contains(All, "Cʟᴀɴ Iɴғᴏ"))
	{
		if(Contains(All, "Last Deposits") || Contains(All, "Bзнocы клaнa"))
			Page.m_Title = "Взносы клана";
		else if(Contains(All, "/clan_invite") || Contains(All, "/clan_set_role") || Contains(All, "/clan_delete"))
			Page.m_Title = "Управление кланом";
		else
			Page.m_Title = "Клан";
		Page.m_Icon = EIconHint::CLAN;
		return;
	}
	if(Contains(All, "Kaтeгopии мaгaзинa"))
	{
		Page.m_Title = "Магазин";
		Page.m_Icon = EIconHint::SHOP;
		return;
	}
	if(Contains(All, "Kyпить") && Contains(All, "Инфopмaция o пpeдмeтe"))
	{
		Page.m_Title = "Предмет магазина";
		Page.m_Icon = EIconHint::SHOP;
		return;
	}
	if(Contains(All, "Нaгpaды зa eжeднeвный вxoд") || Contains(All, "Day 1"))
	{
		Page.m_Title = "Награды за вход";
		Page.m_Icon = EIconHint::GIFT;
		return;
	}
	if(Contains(All, "Нaгpaды зa eжeднeвныe квecты") || Contains(All, "Eжeднeвнaя нaгpaдa"))
	{
		Page.m_Title = "Ежедневные квесты";
		Page.m_Icon = EIconHint::GIFT;
		return;
	}
	if(Contains(All, "Пoлyчить peйтингoвyю нaгpaдy"))
	{
		Page.m_Title = "Рейтинговая награда";
		Page.m_Icon = EIconHint::GIFT;
		return;
	}
	if(ArrowCount >= 3 && Contains(All, "Нaгpaды зa"))
	{
		Page.m_Title = "Награды";
		Page.m_Icon = EIconHint::GIFT;
		return;
	}
	if(Contains(All, "/rating_"))
	{
		Page.m_Title = "Рейтинг";
		Page.m_Icon = EIconHint::TROPHY;
		return;
	}
	if(ToggleCount >= 10 && Contains(All, "Язык"))
	{
		Page.m_Title = "Настройки";
		Page.m_Icon = EIconHint::SETTINGS;
		return;
	}
	if(Contains(All, "Фильтpы") && Contains(All, "пpeдмeтoв]"))
	{
		Page.m_Title = "Инвентарь";
		Page.m_Icon = EIconHint::INVENTORY;
		return;
	}
	if(Contains(All, "Уpoвeнь:") && Contains(All, "Oпыт"))
	{
		Page.m_Title = "Уровень";
		Page.m_Icon = EIconHint::LEVEL;
		return;
	}
	if(ArrowCount >= 8 && Contains(All, "Roulette") && Contains(All, "Crash"))
	{
		Page.m_Title = "Статистика";
		Page.m_Icon = EIconHint::CHART;
		return;
	}
	if(Contains(All, "Bы нe VIP") || Contains(All, "Discord: discord.gg"))
	{
		Page.m_Title = "VIP";
		Page.m_Icon = EIconHint::CROWN;
		return;
	}
	if(Contains(All, "Vodka") && !Contains(All, "Kyпить"))
	{
		Page.m_Title = "Развлечения";
		Page.m_Icon = EIconHint::GAME;
		return;
	}

	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext)
	{
		const std::string Raw = Trim(pOption->m_aDescription);
		if(IsBoxStart(Raw))
		{
			const std::string Title = CleanText(Raw.c_str());
			if(!Title.empty())
			{
				Page.m_Title = Title;
				Page.m_Icon = GuessIcon(Title);
				return;
			}
		}
	}
}

inline bool LooksLikeShopCategoryBox(const std::string &Header)
{
	return Contains(Header, "Kaтeгopии") || Contains(Header, "мaгaзин");
}

inline bool LooksLikeInventoryItem(const std::string &Text)
{
	return StartsWith(Text, "[") && Contains(Text, "]") && Contains(Text, "пpeдмeт");
}

inline bool LooksLikePurchase(const std::string &Text)
{
	return IsNumbered(Text) && (Contains(Text, "Kyпить") || Contains(Text, "Buy"));
}

inline bool NextMeaningfulLooksInteractive(const std::vector<std::pair<int, std::string>> &vRaw, size_t Position)
{
	for(size_t i = Position + 1; i < vRaw.size(); ++i)
	{
		const std::string Next = Trim(vRaw[i].second);
		if(Next.empty())
			continue;
		if(IsBoxEnd(Next))
			return false;
		return IsArrow(Next) || IsToggle(Next);
	}
	return false;
}

inline SPage ParsePage(const CVoting &Voting)
{
	SPage Page;
	ClassifyPage(Voting, Page);

	std::vector<std::pair<int, std::string>> vRaw;
	vRaw.reserve(Voting.NumOptions());
	int Index = 0;
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
		vRaw.emplace_back(Index, pOption->m_aDescription);

	bool InBox = false;
	bool BoxContainsActions = false;
	std::string BoxHeader;

	for(size_t Pos = 0; Pos < vRaw.size(); ++Pos)
	{
		const int VoteIndex = vRaw[Pos].first;
		const std::string Raw = Trim(vRaw[Pos].second);
		if(Raw.empty())
			continue;

		if(IsBackGlyph(Raw))
		{
			if(VoteIndex == 0 || Contains(Raw, "гл") || Contains(Raw, "main"))
				Page.m_HomeVoteIndex = VoteIndex;
			else if(Page.m_BackVoteIndex < 0)
				Page.m_BackVoteIndex = VoteIndex;
			continue;
		}

		if(IsBoxStart(Raw))
		{
			InBox = true;
			BoxHeader = CleanText(Raw.c_str());
			BoxContainsActions = LooksLikeShopCategoryBox(BoxHeader);
			if(!BoxHeader.empty())
			{
				SItem Item;
				Item.m_Kind = EItemKind::SECTION;
				Item.m_Icon = GuessIcon(BoxHeader);
				Item.m_Text = BoxHeader;
				Page.m_vItems.push_back(std::move(Item));
			}
			continue;
		}
		if(IsBoxEnd(Raw))
		{
			InBox = false;
			BoxContainsActions = false;
			BoxHeader.clear();
			continue;
		}

		SItem Item;
		Item.m_VoteIndex = VoteIndex;
		Item.m_Checked = IsChecked(Raw);
		Item.m_Negative = IsNegative(Raw);
		Item.m_Warning = IsWarning(Raw);
		Item.m_NeedsReason = HasPlaceholder(Raw);
		Item.m_Text = CleanText(Raw.c_str());
		Item.m_Icon = GuessIcon(Item.m_Text);

		if(Item.m_NeedsReason)
			Page.m_RequiresReason = true;

		if(InBox && IsBoxLine(Raw))
		{
			if(IsToggle(Raw))
				Item.m_Kind = EItemKind::TOGGLE;
			else if(BoxContainsActions)
				Item.m_Kind = EItemKind::ACTION;
			else
			{
				Item.m_Kind = Item.m_Warning ? EItemKind::NOTE : EItemKind::INFO;
				Item.m_VoteIndex = -1;
			}
		}
		else if(IsArrow(Raw))
		{
			Item.m_Kind = IsToggle(Raw) ? EItemKind::TOGGLE : EItemKind::ACTION;
			if(StartsWith(Item.m_Text, "/"))
			{
				Item.m_Detail = Item.m_Text;
				Item.m_Text = FriendlyCommand(Item.m_Detail);
				Item.m_Icon = GuessIcon(Item.m_Detail);
			}
		}
		else if(IsToggle(Raw))
		{
			if(Page.m_Title == "Уровень" && Contains(Item.m_Text, "Oпыт"))
			{
				Item.m_Kind = EItemKind::INFO;
				Item.m_VoteIndex = -1;
			}
			else
				Item.m_Kind = EItemKind::TOGGLE;
		}
		else if(StartsWith(Raw, "-"))
		{
			Item.m_Kind = EItemKind::NOTE;
			Item.m_VoteIndex = -1;
		}
		else if(LooksLikePurchase(Item.m_Text))
		{
			Item.m_Kind = EItemKind::ACTION;
			Item.m_Icon = EIconHint::SHOP;
			Item.m_NeedsReason = Contains(Item.m_Text, " X ") || Contains(Raw, "пpичин");
			Page.m_RequiresReason = Page.m_RequiresReason || Item.m_NeedsReason;
		}
		else if(LooksLikeInventoryItem(Item.m_Text))
		{
			Item.m_Kind = EItemKind::ACTION;
			Item.m_Icon = EIconHint::INVENTORY;
		}
		else if(IsNumbered(Item.m_Text))
		{
			Item.m_Kind = EItemKind::NOTE;
			Item.m_VoteIndex = -1;
		}
		else if(Contains(Item.m_Text, "<пapaмeтp>") || Contains(Item.m_Text, "<parameter>"))
		{
			Item.m_Kind = EItemKind::NOTE;
			Item.m_Icon = EIconHint::INFO;
			Item.m_VoteIndex = -1;
			Page.m_RequiresReason = true;
		}
		else if(NextMeaningfulLooksInteractive(vRaw, Pos) || (!Item.m_Text.empty() && Item.m_Text.back() == ':' && !Contains(Item.m_Text, "Discord:")))
		{
			Item.m_Kind = EItemKind::SECTION;
			Item.m_VoteIndex = -1;
		}
		else
		{
			Item.m_Kind = EItemKind::INFO;
			Item.m_VoteIndex = -1;
		}

		if(Item.m_Kind == EItemKind::TOGGLE)
			Item.m_Icon = EIconHint::CHECK;
		else if(Item.m_Warning)
			Item.m_Icon = EIconHint::WARNING;
		else if(Item.m_Kind == EItemKind::INFO && Item.m_Icon == EIconHint::GENERIC)
			Item.m_Icon = EIconHint::INFO;

		Page.m_vItems.push_back(std::move(Item));
	}

	if(Page.m_BackVoteIndex < 0)
	{
		for(auto It = vRaw.rbegin(); It != vRaw.rend(); ++It)
		{
			if(IsBackGlyph(It->second) && It->first != Page.m_HomeVoteIndex)
			{
				Page.m_BackVoteIndex = It->first;
				break;
			}
		}
	}

	if(Page.m_RequiresReason)
	{
		for(SItem &Item : Page.m_vItems)
		{
			if(Item.m_Kind == EItemKind::ACTION && HasPlaceholder(Item.m_Detail.empty() ? Item.m_Text : Item.m_Detail))
				Item.m_NeedsReason = true;
		}
	}

	return Page;
}

} // namespace BkwTfMenu

#endif
