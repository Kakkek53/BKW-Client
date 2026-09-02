void RenderBkwAccountSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, IClient *pClient, IInput *pInput, IHttp *pHttp, CUIRect PageView)
{
	static CButtonContainer s_LoginButton;
	static CButtonContainer s_OpenLoginButton;
	static CButtonContainer s_LogoutButton;
	static CButtonContainer s_MainSaveButton;
	static CButtonContainer s_MainLoadButton;
	static CButtonContainer s_NewSaveButton;
	static CButtonContainer s_RefreshSavesButton;
	static CButtonContainer s_SaveLoadButtons[3];
	static CButtonContainer s_SaveOverwriteButtons[3];
	static CButtonContainer s_SaveDeleteButtons[3];
	static CButtonContainer s_ShareButton;
	static CButtonContainer s_RefreshSharesButton;
	static CButtonContainer s_ShareCopyButtons[3];
	static CButtonContainer s_ShareOpenButtons[3];
	static CButtonContainer s_ShareDeleteButtons[3];
	static CButtonContainer s_ImportClipboardButton;

	auto &Cloud = Bkw::CloudAccountState();

	PageView.VMargin(8.0f, &PageView);
	PageView.HSplitTop(8.0f, nullptr, &PageView);
	CUIRect Left, Right;
	PageView.VSplitMid(&Left, &Right, 24.0f);

	Left.Draw(ColorRGBA(0.075f, 0.08f, 0.10f, 0.72f), IGraphics::CORNER_ALL, 10.0f);
	Right.Draw(ColorRGBA(0.075f, 0.08f, 0.10f, 0.72f), IGraphics::CORNER_ALL, 10.0f);
	Left.Margin(14.0f, &Left);
	Right.Margin(14.0f, &Right);

	CUIRect Row;
	Left.HSplitTop(28.0f, &Row, &Left);
	pUi->DoLabel(&Row, "BKW Account", 20.0f, TEXTALIGN_ML);
	Left.HSplitTop(6.0f, nullptr, &Left);

	Left.HSplitTop(42.0f, &Row, &Left);
	pUi->DoLabel(&Row, "Discord используется только для аккаунта BKW Cloud. Discord access token не сохраняется в клиенте.", 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
	Left.HSplitTop(10.0f, nullptr, &Left);

	if(Cloud.LoggedIn())
	{
		char aProfile[192];
		if(Cloud.ProfileLoaded())
			str_format(aProfile, sizeof(aProfile), "Discord: %s%s%s", Cloud.GlobalName(), str_comp(Cloud.GlobalName(), Cloud.UserName()) != 0 ? "  (@" : "", str_comp(Cloud.GlobalName(), Cloud.UserName()) != 0 ? Cloud.UserName() : "");
		else
			str_copy(aProfile, "Discord: проверяю сессию…");
		if(Cloud.ProfileLoaded() && str_comp(Cloud.GlobalName(), Cloud.UserName()) != 0)
			str_append(aProfile, ")");
		Left.HSplitTop(30.0f, &Row, &Left);
		pUi->DoLabel(&Row, aProfile, 14.0f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
		Left.HSplitTop(7.0f, nullptr, &Left);
		Left.HSplitTop(29.0f, &Row, &Left);
		if(pMenus->DoButton_Menu(&s_LogoutButton, "Выйти из аккаунта", 0, &Row) && !Cloud.Busy())
			Cloud.Logout(pHttp);
	}
	else
	{
		Left.HSplitTop(32.0f, &Row, &Left);
		const char *pLoginText = Cloud.Busy() ? "Подключаю Discord…" : (Cloud.LoginWaiting() ? "Ожидаю Discord…" : "Войти через Discord");
		if(pMenus->DoButton_Menu(&s_LoginButton, pLoginText, 0, &Row) && !Cloud.Busy() && !Cloud.LoginWaiting())
			Cloud.StartLogin(pHttp);
		if(Cloud.LoginWaiting())
		{
			Left.HSplitTop(7.0f, nullptr, &Left);
			Left.HSplitTop(22.0f, &Row, &Left);
			char aCode[128];
			str_format(aCode, sizeof(aCode), "Код входа: %s", Cloud.UserCode()[0] ? Cloud.UserCode() : "…");
			pUi->DoLabel(&Row, aCode, 11.5f, TEXTALIGN_ML);
			Left.HSplitTop(4.0f, nullptr, &Left);
			Left.HSplitTop(28.0f, &Row, &Left);
			if(pMenus->DoButton_Menu(&s_OpenLoginButton, "Открыть Discord ещё раз", 0, &Row))
				Cloud.OpenVerification(pClient);
		}
	}

	Left.HSplitTop(14.0f, nullptr, &Left);
	Left.HSplitTop(44.0f, &Row, &Left);
	Row.Draw(ColorRGBA(0.035f, 0.04f, 0.055f, 0.8f), IGraphics::CORNER_ALL, 7.0f);
	Row.Margin(7.0f, &Row);
	pUi->DoLabel(&Row, Cloud.Status(), 10.0f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});

	Right.HSplitTop(25.0f, &Row, &Right);
	pUi->DoLabel(&Row, "Сохранения всего клиента", 18.0f, TEXTALIGN_ML);
	Right.HSplitTop(4.0f, nullptr, &Right);
	Right.HSplitTop(32.0f, &Row, &Right);
	pUi->DoLabel(&Row, "DDNet + TClient + BestClient + BKW. Пароли, токены и TipoCheat остаются локальными.", 9.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
	Right.HSplitTop(6.0f, nullptr, &Right);

	CUIRect A, B;
	Right.HSplitTop(28.0f, &Row, &Right);
	Row.VSplitMid(&A, &B, 7.0f);
	if(pMenus->DoButton_Menu(&s_MainSaveButton, "Сохранить основное", 0, &A) && !Cloud.Busy() && Cloud.LoggedIn())
		Cloud.SaveSettings(pHttp);
	if(pMenus->DoButton_Menu(&s_MainLoadButton, "Загрузить основное", 0, &B) && !Cloud.Busy() && Cloud.LoggedIn())
		Cloud.LoadSettings(pHttp);
	Right.HSplitTop(5.0f, nullptr, &Right);
	Right.HSplitTop(27.0f, &Row, &Right);
	Row.VSplitMid(&A, &B, 7.0f);
	if(pMenus->DoButton_Menu(&s_NewSaveButton, "+ Новое сохранение", 0, &A) && !Cloud.Busy() && Cloud.LoggedIn())
		Cloud.CreateSave(pHttp);
	if(pMenus->DoButton_Menu(&s_RefreshSavesButton, "Обновить список", 0, &B) && !Cloud.Busy() && Cloud.LoggedIn())
		Cloud.ListSaves(pHttp);

	const auto &Saves = Cloud.Saves();
	for(size_t i = 0; i < Saves.size() && i < 3; ++i)
	{
		Right.HSplitTop(5.0f, nullptr, &Right);
		Right.HSplitTop(26.0f, &Row, &Right);
		CUIRect Label, Buttons;
		Row.VSplitLeft(Row.w * 0.34f, &Label, &Buttons);
		pUi->DoLabel(&Label, Saves[i].m_Name.c_str(), 9.5f, TEXTALIGN_ML, {.m_MaxWidth = Label.w - 4.0f});
		CUIRect Load, Overwrite, Delete;
		Buttons.VSplitLeft(Buttons.w * 0.34f, &Load, &Buttons);
		Buttons.VSplitLeft(Buttons.w * 0.52f, &Overwrite, &Delete);
		if(pMenus->DoButton_Menu(&s_SaveLoadButtons[i], "Загрузить", 0, &Load) && !Cloud.Busy())
			Cloud.LoadSave(pHttp, Saves[i].m_Id.c_str());
		if(pMenus->DoButton_Menu(&s_SaveOverwriteButtons[i], "Обновить", 0, &Overwrite) && !Cloud.Busy())
			Cloud.UpdateSave(pHttp, Saves[i].m_Id.c_str());
		if(pMenus->DoButton_Menu(&s_SaveDeleteButtons[i], "×", 0, &Delete) && !Cloud.Busy())
			Cloud.DeleteSave(pHttp, Saves[i].m_Id.c_str());
	}

	Right.HSplitTop(11.0f, nullptr, &Right);
	Right.HSplitTop(22.0f, &Row, &Right);
	pUi->DoLabel(&Row, "Мои HTTPS-ссылки", 13.0f, TEXTALIGN_ML);
	Right.HSplitTop(4.0f, nullptr, &Right);
	Right.HSplitTop(27.0f, &Row, &Right);
	Row.VSplitMid(&A, &B, 7.0f);
	if(pMenus->DoButton_Menu(&s_ShareButton, "Создать HTTPS-ссылку", 0, &A) && !Cloud.Busy() && Cloud.LoggedIn())
		Cloud.CreateShare(pHttp);
	if(pMenus->DoButton_Menu(&s_RefreshSharesButton, "Обновить ссылки", 0, &B) && !Cloud.Busy() && Cloud.LoggedIn())
		Cloud.ListShares(pHttp);

	const auto &Shares = Cloud.Shares();
	for(size_t i = 0; i < Shares.size() && i < 3; ++i)
	{
		Right.HSplitTop(5.0f, nullptr, &Right);
		Right.HSplitTop(26.0f, &Row, &Right);
		CUIRect Link, Buttons;
		Row.VSplitLeft(Row.w * 0.43f, &Link, &Buttons);
		pUi->DoLabel(&Link, Shares[i].m_Url.c_str(), 8.0f, TEXTALIGN_ML, {.m_MaxWidth = Link.w - 4.0f});
		CUIRect Copy, Open, Delete;
		Buttons.VSplitLeft(Buttons.w * 0.38f, &Copy, &Buttons);
		Buttons.VSplitLeft(Buttons.w * 0.58f, &Open, &Delete);
		if(pMenus->DoButton_Menu(&s_ShareCopyButtons[i], "Копия", 0, &Copy))
			pInput->SetClipboardText(Shares[i].m_Url.c_str());
		if(pMenus->DoButton_Menu(&s_ShareOpenButtons[i], "Открыть", 0, &Open))
			pClient->ViewLink(Shares[i].m_Url.c_str());
		if(pMenus->DoButton_Menu(&s_ShareDeleteButtons[i], "×", 0, &Delete) && !Cloud.Busy())
			Cloud.DeleteShare(pHttp, Shares[i].m_Id.c_str());
	}

	Right.HSplitTop(10.0f, nullptr, &Right);
	Right.HSplitTop(26.0f, &Row, &Right);
	if(pMenus->DoButton_Menu(&s_ImportClipboardButton, "Импортировать HTTPS-ссылку из буфера", 0, &Row) && !Cloud.Busy())
	{
		const std::string Clipboard = pInput->GetClipboardText();
		Cloud.ImportShare(pHttp, Clipboard.c_str());
	}
}
