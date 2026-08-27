#include <game/client/components/bkw/ddstats_hours.h>

void RenderBkwHoursSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, IClient *pClient, IHttp *pHttp, CUIRect PageView)
{
	static CButtonContainer s_RefreshButton;
	static char s_aLastAutoPlayer[MAX_NAME_LENGTH] = "";

	auto &Hours = Bkw::DdStatsHoursState();
	Hours.Poll();

	const char *pCurrentPlayer = "";
	const int LocalClientId = pGameClient->m_Snap.m_LocalClientId;
	if(pClient->State() == IClient::STATE_ONLINE && LocalClientId >= 0 && LocalClientId < MAX_CLIENTS && pGameClient->m_aClients[LocalClientId].m_Active)
		pCurrentPlayer = pGameClient->m_aClients[LocalClientId].m_aName;

	if(pCurrentPlayer[0] != '\0' && str_comp(s_aLastAutoPlayer, pCurrentPlayer) != 0)
	{
		str_copy(s_aLastAutoPlayer, pCurrentPlayer);
		Hours.RequestIfStale(pHttp, pCurrentPlayer);
	}
	else if(pCurrentPlayer[0] == '\0')
	{
		s_aLastAutoPlayer[0] = '\0';
	}

	CUIRect Header;
	PageView.HSplitTop(26.0f, &Header, &PageView);
	pUi->DoLabel(&Header, "Игровое время", 20.0f, TEXTALIGN_ML);
	PageView.HSplitTop(6.0f, nullptr, &PageView);

	CUIRect PlayerRow;
	PageView.HSplitTop(30.0f, &PlayerRow, &PageView);
	CUIRect PlayerLabel, Refresh;
	PlayerRow.VSplitRight(112.0f, &PlayerLabel, &Refresh);
	PlayerLabel.VSplitRight(8.0f, &PlayerLabel, nullptr);
	char aPlayer[128];
	if(pCurrentPlayer[0] != '\0')
		str_format(aPlayer, sizeof(aPlayer), "Ник из игры: %s", pCurrentPlayer);
	else
		str_copy(aPlayer, "Ник из игры: —");
	pUi->DoLabel(&PlayerLabel, aPlayer, 12.0f, TEXTALIGN_ML);
	if(pMenus->DoButton_Menu(&s_RefreshButton, Hours.Loading() ? "Загрузка..." : "Обновить", 0, &Refresh) && !Hours.Loading() && pCurrentPlayer[0] != '\0')
		Hours.Request(pHttp, pCurrentPlayer, true);

	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect StatsCard;
	PageView.HSplitTop(152.0f, &StatsCard, &PageView);
	StatsCard.Draw(ColorRGBA(0.08f, 0.08f, 0.08f, 0.42f), IGraphics::CORNER_ALL, 6.0f);
	StatsCard.Margin(9.0f, &StatsCard);

	if(pCurrentPlayer[0] == '\0')
	{
		pUi->DoLabel(&StatsCard, "Подключитесь к серверу — часы автоматически загрузятся для текущего игрового ника.", 12.0f, TEXTALIGN_MC, {.m_MaxWidth = StatsCard.w});
		return;
	}

	if(Hours.Loading() && str_comp(Hours.Player(), pCurrentPlayer) == 0)
	{
		pUi->DoLabel(&StatsCard, "Загрузка статистики DDStats...", 12.0f, TEXTALIGN_MC);
		return;
	}

	if(Hours.Error() && str_comp(Hours.Player(), pCurrentPlayer) == 0)
	{
		char aError[160];
		if(Hours.HttpStatus() > 0)
			str_format(aError, sizeof(aError), "Не удалось загрузить DDStats (HTTP %d). Нажмите «Обновить».", Hours.HttpStatus());
		else
			str_copy(aError, "Не удалось загрузить DDStats. Нажмите «Обновить».");
		pUi->DoLabel(&StatsCard, aError, 12.0f, TEXTALIGN_MC, {.m_MaxWidth = StatsCard.w});
		return;
	}

	if(Hours.Loaded() && str_comp(Hours.Player(), pCurrentPlayer) == 0)
	{
		CUIRect BigHours, ExactLine, StartLine, AverageLine, SourceLine;
		StatsCard.HSplitTop(42.0f, &BigHours, &StatsCard);
		char aBigHours[96];
		str_format(aBigHours, sizeof(aBigHours), "%.1f ч", Hours.TotalHours());
		pUi->DoLabel(&BigHours, aBigHours, 28.0f, TEXTALIGN_MC);

		StatsCard.HSplitTop(20.0f, &ExactLine, &StatsCard);
		char aExact[128];
		Bkw::CDdStatsHours::FormatDuration(Hours.TotalSecondsPlayed(), aExact, sizeof(aExact), true);
		pUi->DoLabel(&ExactLine, aExact, 11.5f, TEXTALIGN_MC);

		StatsCard.HSplitTop(20.0f, &StartLine, &StatsCard);
		char aStart[160];
		str_format(aStart, sizeof(aStart), "Статистика с: %s", Hours.StartOfPlaytime()[0] != '\0' ? Hours.StartOfPlaytime() : "неизвестно");
		pUi->DoLabel(&StartLine, aStart, 10.5f, TEXTALIGN_MC);

		StatsCard.HSplitTop(20.0f, &AverageLine, &StatsCard);
		char aAverageDuration[96];
		Bkw::CDdStatsHours::FormatDuration(Hours.AverageSecondsPlayed(), aAverageDuration, sizeof(aAverageDuration));
		char aAverage[160];
		str_format(aAverage, sizeof(aAverage), "Среднее игровое время: %s", aAverageDuration);
		pUi->DoLabel(&AverageLine, aAverage, 10.5f, TEXTALIGN_MC);

		StatsCard.HSplitTop(20.0f, &SourceLine, &StatsCard);
		pUi->DoLabel(&SourceLine, "Источник: DDStats", 10.0f, TEXTALIGN_MC);
		return;
	}

	pUi->DoLabel(&StatsCard, "Статистика ещё не загружена.", 12.0f, TEXTALIGN_MC);
}
