bool s_BkwTfMenuOverlayActive = false;
bool s_BkwTfMenuOverlaySuppressReopen = false;
bool s_BkwTfMenuOverlayPulseSent = false;
int s_BkwTfMenuSelectedTab = 0;

void BkwTfMenuOverlayReset()
{
	s_BkwTfMenuOverlayActive = false;
	s_BkwTfMenuOverlayPulseSent = false;
	s_BkwTfMenuSelectedTab = 0;
}

const CVoteOptionClient *BkwTfVoteOptionAt(const CVoting &Voting, int WantedIndex)
{
	int Index = 0;
	for(const CVoteOptionClient *pOption = Voting.FirstOption(); pOption; pOption = pOption->m_pNext, ++Index)
	{
		if(Index == WantedIndex)
			return pOption;
	}
	return nullptr;
}

void BkwTfCopyValueAt(const CVoting &Voting, int Index, char *pBuffer, size_t BufferSize)
{
	if(!pBuffer || BufferSize == 0)
		return;
	pBuffer[0] = '\0';

	const CVoteOptionClient *pOption = BkwTfVoteOptionAt(Voting, Index);
	if(!pOption)
	{
		str_copy(pBuffer, "—", BufferSize);
		return;
	}

	const char *pValue = str_find(pOption->m_aDescription, ":");
	if(!pValue)
	{
		str_copy(pBuffer, pOption->m_aDescription, BufferSize);
		return;
	}
	++pValue;
	while(*pValue == ' ' || *pValue == '\t' || *pValue == '-' || *pValue == '|')
		++pValue;

	str_copy(pBuffer, *pValue ? pValue : "—", BufferSize);
	int Length = str_length(pBuffer);
	while(Length > 0 && (pBuffer[Length - 1] == ' ' || pBuffer[Length - 1] == '\t' || pBuffer[Length - 1] == '\r' || pBuffer[Length - 1] == '\n' || pBuffer[Length - 1] == '|'))
		pBuffer[--Length] = '\0';
}

void BkwTfCopyTabAt(const CVoting &Voting, int Index, const char *pFallback, char *pBuffer, size_t BufferSize)
{
	if(!pBuffer || BufferSize == 0)
		return;
	pBuffer[0] = '\0';

	const CVoteOptionClient *pOption = BkwTfVoteOptionAt(Voting, Index);
	if(!pOption || pOption->m_aDescription[0] == '\0')
	{
		str_copy(pBuffer, pFallback, BufferSize);
		return;
	}

	const char *pText = pOption->m_aDescription;
	const char *pFirstSpace = str_find(pText, " ");
	if(pFirstSpace && pFirstSpace[1] != '\0')
		pText = pFirstSpace + 1;
	while(*pText == ' ' || *pText == '\t')
		++pText;
	str_copy(pBuffer, *pText ? pText : pFallback, BufferSize);
}

void RenderBkwTfMenuOverlay(CUi *pUi, const CVoting &Voting)
{
	pUi->MapScreen();
	CUIRect Screen = *pUi->Screen();

	// Darken the running game first. The TF card itself uses only neutral
	// black/graphite/white shades so it fits the native DDNet/BestClient UI.
	Screen.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.68f), IGraphics::CORNER_NONE, 0.0f);

	const float CardW = minimum(Screen.w - 26.0f, 560.0f);
	const float CardH = minimum(Screen.h - 22.0f, 272.0f);
	CUIRect Card{Screen.x + (Screen.w - CardW) * 0.5f, Screen.y + (Screen.h - CardH) * 0.5f, CardW, CardH};
	Card.Draw(ColorRGBA(0.52f, 0.54f, 0.58f, 0.42f), IGraphics::CORNER_ALL, 14.0f);
	Card.Margin(1.2f, &Card);
	Card.Draw(ColorRGBA(0.035f, 0.038f, 0.045f, 0.985f), IGraphics::CORNER_ALL, 13.0f);

	CUIRect Header, Body;
	Card.Margin(10.0f, &Card);
	Card.HSplitTop(35.0f, &Header, &Body);

	CUIRect Title, Subtitle;
	Header.VSplitLeft(190.0f, &Title, &Subtitle);
	pUi->DoLabel(&Title, "TeeFusion", 21.0f, TEXTALIGN_ML);
	Subtitle.VSplitRight(4.0f, &Subtitle, nullptr);
	pUi->DoLabel(&Subtitle, "TF Menu  •  ESC — закрыть", 10.0f, TEXTALIGN_MR);

	CUIRect Divider;
	Body.HSplitTop(1.0f, &Divider, &Body);
	Divider.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.10f), IGraphics::CORNER_NONE, 0.0f);
	Body.HSplitTop(7.0f, nullptr, &Body);

	CUIRect Nav, Content;
	Body.VSplitLeft(142.0f, &Nav, &Content);
	Nav.VSplitRight(8.0f, &Nav, nullptr);
	Content.VSplitLeft(4.0f, nullptr, &Content);

	Nav.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.035f), IGraphics::CORNER_ALL, 9.0f);
	Content.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.055f), IGraphics::CORNER_ALL, 9.0f);

	static CButtonContainer s_aTabButtons[13];
	static const char *s_apFallbackTabs[12] = {
		"Правила", "Дуэли", "Почта", "Награда", "Рейтинг", "Уровень",
		"Клан", "Магазин", "Инвентарь", "VIP", "Статистика", "Настройки"};

	CUIRect NavInner;
	Nav.Margin(5.0f, &NavInner);
	const float TabSpacing = 1.0f;
	const float TabH = maximum(13.0f, minimum(17.0f, (NavInner.h - TabSpacing * 12.0f) / 13.0f));
	for(int Tab = 0; Tab < 13; ++Tab)
	{
		CUIRect Row;
		NavInner.HSplitTop(TabH, &Row, &NavInner);
		if(Tab != 12)
			NavInner.HSplitTop(TabSpacing, nullptr, &NavInner);

		const bool Active = s_BkwTfMenuSelectedTab == Tab;
		const bool Hovered = pUi->MouseInside(&Row);
		if(Active)
			Row.Draw(ColorRGBA(0.86f, 0.87f, 0.89f, 0.18f), IGraphics::CORNER_ALL, 5.0f);
		else if(Hovered)
			Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.075f), IGraphics::CORNER_ALL, 5.0f);

		char aTab[96];
		if(Tab == 0)
			str_copy(aTab, "Главная", sizeof(aTab));
		else
			BkwTfCopyTabAt(Voting, 10 + Tab, s_apFallbackTabs[Tab - 1], aTab, sizeof(aTab));

		CUIRect Label = Row;
		Label.VMargin(6.0f, &Label);
		pUi->DoLabel(&Label, aTab, Active ? 11.0f : 10.5f, TEXTALIGN_ML);
		if(pUi->DoButtonLogic(&s_aTabButtons[Tab], Active, &Row, BUTTONFLAG_LEFT))
			s_BkwTfMenuSelectedTab = Tab;
	}

	CUIRect ContentInner;
	Content.Margin(10.0f, &ContentInner);
	CUIRect ContentTitle;
	ContentInner.HSplitTop(24.0f, &ContentTitle, &ContentInner);

	if(s_BkwTfMenuSelectedTab == 0)
	{
		pUi->DoLabel(&ContentTitle, "Главная", 18.0f, TEXTALIGN_ML);
		ContentInner.HSplitTop(3.0f, nullptr, &ContentInner);

		if(Voting.NumOptions() <= 0)
		{
			pUi->DoLabel(&ContentInner, "Получаю данные TeeFusion…", 12.0f, TEXTALIGN_MC);
			return;
		}

		static const char *s_apInfoLabels[8] = {
			"Имя пользователя", "Discord", "Пин-код", "Язык",
			"Рейтинг", "Уровень", "Опыт", "Валюты"};

		const float InfoSpacing = 2.0f;
		const float RowH = minimum(21.0f, maximum(14.0f, (ContentInner.h - InfoSpacing * 7.0f) / 8.0f));
		for(int Info = 0; Info < 8; ++Info)
		{
			CUIRect Row;
			ContentInner.HSplitTop(RowH, &Row, &ContentInner);
			if(Info != 7)
				ContentInner.HSplitTop(InfoSpacing, nullptr, &ContentInner);
			Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Info % 2 == 0 ? 0.055f : 0.035f), IGraphics::CORNER_ALL, 5.0f);
			Row.VMargin(7.0f, &Row);

			CUIRect Key, Value;
			Row.VSplitLeft(minimum(132.0f, Row.w * 0.43f), &Key, &Value);
			pUi->DoLabel(&Key, s_apInfoLabels[Info], 10.5f, TEXTALIGN_ML);

			char aValue[160];
			BkwTfCopyValueAt(Voting, Info + 1, aValue, sizeof(aValue));
			pUi->DoLabel(&Value, aValue, 11.0f, TEXTALIGN_ML);
		}
	}
	else
	{
		char aTab[96];
		BkwTfCopyTabAt(Voting, 10 + s_BkwTfMenuSelectedTab, s_apFallbackTabs[s_BkwTfMenuSelectedTab - 1], aTab, sizeof(aTab));
		pUi->DoLabel(&ContentTitle, aTab, 18.0f, TEXTALIGN_ML);

		CUIRect Message = ContentInner;
		Message.HMargin(24.0f, &Message);
		Message.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.035f), IGraphics::CORNER_ALL, 8.0f);
		pUi->DoLabel(&Message, "Раздел подготовлен как вкладка. Содержимое подключим следующим шагом.", 11.0f, TEXTALIGN_MC);
	}
}
