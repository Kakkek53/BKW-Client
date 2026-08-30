void RenderBkwAccountSettings(CMenus *pMenus, CUi *pUi, CGameClient *pGameClient, IClient *pClient, IInput *pInput, IHttp *pHttp, CUIRect PageView)
{
	static CButtonContainer s_LoginButton;
	static CButtonContainer s_OpenLoginButton;
	static CButtonContainer s_LogoutButton;
	static CButtonContainer s_SaveButton;
	static CButtonContainer s_LoadButton;
	static CButtonContainer s_ShareButton;
	static CButtonContainer s_CopyShareButton;
	static CButtonContainer s_OpenShareButton;
	static CButtonContainer s_ImportClipboardButton;

	auto &Cloud = Bkw::CloudAccountState();
	Cloud.Poll(pHttp, pClient);

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
	pUi->DoLabel(&Row, "Вход выполняется через Discord в браузере. Discord access token никогда не сохраняется в клиенте — BKW Cloud выдаёт отдельную сессию.", 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
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
		Left.HSplitTop(8.0f, nullptr, &Left);
		Left.HSplitTop(30.0f, &Row, &Left);
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
			Left.HSplitTop(8.0f, nullptr, &Left);
			Left.HSplitTop(24.0f, &Row, &Left);
			char aCode[128];
			str_format(aCode, sizeof(aCode), "Код входа: %s", Cloud.UserCode()[0] ? Cloud.UserCode() : "…");
			pUi->DoLabel(&Row, aCode, 11.5f, TEXTALIGN_ML);
			Left.HSplitTop(5.0f, nullptr, &Left);
			Left.HSplitTop(28.0f, &Row, &Left);
			if(pMenus->DoButton_Menu(&s_OpenLoginButton, "Открыть Discord ещё раз", 0, &Row))
				Cloud.OpenVerification(pClient);
		}
	}

	Left.HSplitTop(14.0f, nullptr, &Left);
	Left.HSplitTop(48.0f, &Row, &Left);
	Row.Draw(ColorRGBA(0.035f, 0.04f, 0.055f, 0.8f), IGraphics::CORNER_ALL, 7.0f);
	Row.Margin(8.0f, &Row);
	pUi->DoLabel(&Row, Cloud.Status(), 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});

	Right.HSplitTop(28.0f, &Row, &Right);
	pUi->DoLabel(&Row, "Облачные настройки", 20.0f, TEXTALIGN_ML);
	Right.HSplitTop(6.0f, nullptr, &Right);
	Right.HSplitTop(42.0f, &Row, &Right);
	pUi->DoLabel(&Row, "В облако и share-ссылку попадают только параметры bkw_*. Токен аккаунта bkw_cloud_token автоматически исключён.", 10.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
	Right.HSplitTop(10.0f, nullptr, &Right);

	Right.HSplitTop(30.0f, &Row, &Right);
	if(pMenus->DoButton_Menu(&s_SaveButton, "Сохранить мои BKW настройки в облако", 0, &Row) && !Cloud.Busy() && Cloud.LoggedIn())
		Cloud.SaveSettings(pHttp);
	Right.HSplitTop(7.0f, nullptr, &Right);
	Right.HSplitTop(30.0f, &Row, &Right);
	if(pMenus->DoButton_Menu(&s_LoadButton, "Загрузить мои настройки из облака", 0, &Row) && !Cloud.Busy() && Cloud.LoggedIn())
		Cloud.LoadSettings(pHttp);
	Right.HSplitTop(7.0f, nullptr, &Right);
	Right.HSplitTop(30.0f, &Row, &Right);
	if(pMenus->DoButton_Menu(&s_ShareButton, "Создать обычную HTTPS-ссылку", 0, &Row) && !Cloud.Busy() && Cloud.LoggedIn())
		Cloud.CreateShare(pHttp);

	if(Cloud.LastShareUrl()[0] != '\0')
	{
		Right.HSplitTop(10.0f, nullptr, &Right);
		Right.HSplitTop(42.0f, &Row, &Right);
		Row.Draw(ColorRGBA(0.035f, 0.04f, 0.055f, 0.8f), IGraphics::CORNER_ALL, 7.0f);
		Row.Margin(7.0f, &Row);
		pUi->DoLabel(&Row, Cloud.LastShareUrl(), 9.5f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
		Right.HSplitTop(6.0f, nullptr, &Right);
		CUIRect CopyButton, OpenButton;
		Right.HSplitTop(28.0f, &Row, &Right);
		Row.VSplitMid(&CopyButton, &OpenButton, 8.0f);
		if(pMenus->DoButton_Menu(&s_CopyShareButton, "Копировать", 0, &CopyButton))
			pInput->SetClipboardText(Cloud.LastShareUrl());
		if(pMenus->DoButton_Menu(&s_OpenShareButton, "Открыть", 0, &OpenButton))
			Cloud.OpenShare(pClient);
	}

	Right.HSplitTop(16.0f, nullptr, &Right);
	Right.HSplitTop(22.0f, &Row, &Right);
	pUi->DoLabel(&Row, "Импорт чужих настроек", 13.0f, TEXTALIGN_ML);
	Right.HSplitTop(4.0f, nullptr, &Right);
	Right.HSplitTop(38.0f, &Row, &Right);
	pUi->DoLabel(&Row, "Скопируйте ссылку https://bkw-client.onrender.com/share/... в буфер обмена и нажмите кнопку ниже.", 10.0f, TEXTALIGN_ML, {.m_MaxWidth = Row.w});
	Right.HSplitTop(5.0f, nullptr, &Right);
	Right.HSplitTop(30.0f, &Row, &Right);
	if(pMenus->DoButton_Menu(&s_ImportClipboardButton, "Импортировать ссылку из буфера", 0, &Row) && !Cloud.Busy())
	{
		const std::string Clipboard = pInput->GetClipboardText();
		Cloud.ImportShare(pHttp, Clipboard.c_str());
	}
}
