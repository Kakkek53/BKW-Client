#include "menus.h"

#include <base/str.h>

#include <engine/console.h>
#include <engine/shared/config.h>

#include <game/client/ui_listbox.h>
#include <game/localization.h>

#include <algorithm>
#include <string>

namespace
{
const char *CurrentAssetName(ESkinShopCategory Category)
{
	switch(Category)
	{
	case ESkinShopCategory::GAMESKIN: return g_Config.m_ClAssetGame;
	case ESkinShopCategory::PARTICLE: return g_Config.m_ClAssetParticles;
	case ESkinShopCategory::ENTITY: return g_Config.m_ClAssetsEntities;
	case ESkinShopCategory::EMOTICON: return g_Config.m_ClAssetEmoticons;
	case ESkinShopCategory::HUD: return g_Config.m_ClAssetHud;
	case ESkinShopCategory::CURSOR: return g_Config.m_ClAssetCursor;
	case ESkinShopCategory::NUM_CATEGORIES: break;
	}
	return "default";
}

void ExecuteAssetCommand(IConsole *pConsole, ESkinShopCategory Category, const char *pAssetName)
{
	char aCommand[IConsole::CMDLINE_LENGTH];
	str_format(aCommand, sizeof(aCommand), "%s \"%s\"", CSkinShop::CategoryConfigCommand(Category), pAssetName);
	pConsole->ExecuteLine(aCommand, IConsole::CLIENT_ID_GAME);
}
} // namespace

void CMenus::RenderSettingsAssetsShop(CUIRect MainView)
{
	m_SkinShop.Init(Http(), Storage());
	m_SkinShop.Update();

	if(!m_SkinShop.Loading() && !m_SkinShop.Error() && m_SkinShop.HasMore() && m_SkinShop.LoadedPage() > 0)
		m_SkinShop.LoadNextPage();

	CUIRect Header, CategoryBar, ListView, Details;
	MainView.HSplitTop(24.0f, &Header, &MainView);
	Ui()->DoLabel(&Header, Localize("Texture shop"), 18.0f, TEXTALIGN_ML);
	MainView.HSplitTop(5.0f, nullptr, &MainView);
	MainView.HSplitTop(24.0f, &CategoryBar, &MainView);

	static CButtonContainer s_aCategoryButtons[(int)ESkinShopCategory::NUM_CATEGORIES];
	const float CategoryWidth = CategoryBar.w / (float)((int)ESkinShopCategory::NUM_CATEGORIES);
	for(int i = 0; i < (int)ESkinShopCategory::NUM_CATEGORIES; ++i)
	{
		const ESkinShopCategory Category = (ESkinShopCategory)i;
		CUIRect Button;
		CategoryBar.VSplitLeft(CategoryWidth, &Button, &CategoryBar);
		const int Corners = i == 0 ? IGraphics::CORNER_L : (i == (int)ESkinShopCategory::NUM_CATEGORIES - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE);
		if(DoButton_MenuTab(&s_aCategoryButtons[i], CSkinShop::CategoryName(Category), m_SkinShop.Category() == Category, &Button, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
			m_SkinShop.SetCategory(Category);
	}

	MainView.HSplitTop(8.0f, nullptr, &MainView);
	MainView.HSplitBottom(210.0f, &ListView, &Details);
	Details.HSplitTop(8.0f, nullptr, &Details);

	const auto &vItems = m_SkinShop.Items();
	static CListBox s_ShopListBox;
	static int s_Selected = -1;
	static ESkinShopCategory s_SelectedCategory = ESkinShopCategory::NUM_CATEGORIES;
	static IGraphics::CTextureHandle s_PreviewTexture;
	static std::string s_PreviewPath;

	auto ClearPreview = [&]() {
		if(s_PreviewTexture.IsValid() && !s_PreviewTexture.IsNullTexture())
			Graphics()->UnloadTexture(&s_PreviewTexture);
		s_PreviewTexture.Invalidate();
		s_PreviewPath.clear();
	};

	if(s_SelectedCategory != m_SkinShop.Category())
	{
		s_SelectedCategory = m_SkinShop.Category();
		s_Selected = -1;
		ClearPreview();
	}
	if(s_Selected >= (int)vItems.size())
		s_Selected = -1;

	if(vItems.empty())
	{
		const char *pMessage = m_SkinShop.Error() ? Localize("Could not load the shop. Try opening the tab again.") : Localize("Loading shop items...");
		Ui()->DoLabel(&ListView, pMessage, 16.0f, TEXTALIGN_MC);
	}
	else
	{
		s_ShopListBox.DoStart(48.0f, vItems.size(), 1, 1, s_Selected, &ListView, false);
		for(size_t i = 0; i < vItems.size(); ++i)
		{
			const SSkinShopItem &ItemData = vItems[i];
			const CListboxItem Item = s_ShopListBox.DoNextItem(&ItemData, s_Selected == (int)i);
			if(!Item.m_Visible)
				continue;

			CUIRect Row = Item.m_Rect;
			Row.Margin(5.0f, &Row);
			CUIRect Name, Meta;
			Row.HSplitTop(20.0f, &Name, &Meta);
			Ui()->DoLabel(&Name, ItemData.m_Name.c_str(), 15.0f, TEXTALIGN_ML);

			char aMeta[256];
			if(!ItemData.m_Author.empty())
				str_format(aMeta, sizeof(aMeta), "%s  |  %dx%d  |  %s", ItemData.m_Author.c_str(), ItemData.m_Width, ItemData.m_Height, ItemData.m_Type.c_str());
			else
				str_format(aMeta, sizeof(aMeta), "%dx%d  |  %s", ItemData.m_Width, ItemData.m_Height, ItemData.m_Type.c_str());
			Ui()->DoLabel(&Meta, aMeta, 12.0f, TEXTALIGN_ML);
		}
		s_Selected = s_ShopListBox.DoEnd();
	}

	if(s_Selected < 0 || s_Selected >= (int)vItems.size())
	{
		ClearPreview();
		char aStatus[160];
		str_format(aStatus, sizeof(aStatus), "%zu items%s", vItems.size(), m_SkinShop.Loading() ? "  |  loading more..." : "");
		Ui()->DoLabel(&Details, aStatus, 13.0f, TEXTALIGN_ML);
		return;
	}

	const SSkinShopItem &Selected = vItems[s_Selected];
	m_SkinShop.RequestPreview(Selected);

	const bool PreviewReady = m_SkinShop.PreviewReady(Selected);
	if(PreviewReady)
	{
		const std::string PreviewPath = m_SkinShop.PreviewPath(Selected);
		if(PreviewPath != s_PreviewPath || !s_PreviewTexture.IsValid() || s_PreviewTexture.IsNullTexture())
		{
			ClearPreview();
			s_PreviewPath = PreviewPath;
			s_PreviewTexture = Graphics()->LoadTexture(s_PreviewPath.c_str(), IStorage::TYPE_SAVE);
		}
	}

	CUIRect PreviewColumn, InfoColumn;
	Details.VSplitLeft(std::min(300.0f, Details.w * 0.42f), &PreviewColumn, &InfoColumn);
	PreviewColumn.VSplitRight(10.0f, &PreviewColumn, nullptr);
	InfoColumn.VSplitLeft(10.0f, nullptr, &InfoColumn);

	PreviewColumn.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.28f), IGraphics::CORNER_ALL, 6.0f);
	CUIRect PreviewRect = PreviewColumn;
	PreviewRect.Margin(8.0f, &PreviewRect);
	if(s_PreviewTexture.IsValid() && !s_PreviewTexture.IsNullTexture())
	{
		float ImageW = Selected.m_Width > 0 ? (float)Selected.m_Width : PreviewRect.w;
		float ImageH = Selected.m_Height > 0 ? (float)Selected.m_Height : PreviewRect.h;
		const float Scale = std::min(PreviewRect.w / ImageW, PreviewRect.h / ImageH);
		ImageW *= Scale;
		ImageH *= Scale;

		Graphics()->WrapClamp();
		Graphics()->TextureSet(s_PreviewTexture);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1, 1, 1, 1);
		IGraphics::CQuadItem Quad(PreviewRect.x + (PreviewRect.w - ImageW) / 2.0f, PreviewRect.y + (PreviewRect.h - ImageH) / 2.0f, ImageW, ImageH);
		Graphics()->QuadsDrawTL(&Quad, 1);
		Graphics()->QuadsEnd();
		Graphics()->WrapNormal();
	}
	else
	{
		const char *pPreviewStatus = m_SkinShop.PreviewError(Selected) ? Localize("Preview unavailable") : Localize("Loading preview...");
		Ui()->DoLabel(&PreviewRect, pPreviewStatus, 14.0f, TEXTALIGN_MC);
	}

	CUIRect Name, Meta, LocalName, Status, ButtonRow;
	InfoColumn.HSplitTop(28.0f, &Name, &InfoColumn);
	Ui()->DoLabel(&Name, Selected.m_Name.c_str(), 18.0f, TEXTALIGN_ML);
	InfoColumn.HSplitTop(22.0f, &Meta, &InfoColumn);
	char aMeta[256];
	str_format(aMeta, sizeof(aMeta), "%s%s%s  |  %dx%d",
		Selected.m_Author.empty() ? "" : Selected.m_Author.c_str(),
		Selected.m_Author.empty() ? "" : "  |  ",
		Selected.m_Type.c_str(), Selected.m_Width, Selected.m_Height);
	Ui()->DoLabel(&Meta, aMeta, 13.0f, TEXTALIGN_ML);

	InfoColumn.HSplitTop(22.0f, &LocalName, &InfoColumn);
	const std::string AssetName = CSkinShop::AssetName(Selected);
	char aLocalName[160];
	str_format(aLocalName, sizeof(aLocalName), "Local: %s", AssetName.c_str());
	Ui()->DoLabel(&LocalName, aLocalName, 12.0f, TEXTALIGN_ML);

	InfoColumn.HSplitTop(22.0f, &Status, &InfoColumn);
	const bool IsInstalled = m_SkinShop.Installed(Selected);
	const bool IsSelected = IsInstalled && str_comp(CurrentAssetName(m_SkinShop.Category()), AssetName.c_str()) == 0;
	char aStatus[192];
	if(m_SkinShop.DownloadLoading(Selected))
		str_format(aStatus, sizeof(aStatus), Localize("Downloading... %d%%"), m_SkinShop.DownloadProgress(Selected));
	else if(m_SkinShop.DownloadError(Selected))
		str_copy(aStatus, Localize("Download failed. You can try again."));
	else if(IsSelected)
		str_copy(aStatus, Localize("Downloaded and selected"));
	else if(IsInstalled)
		str_copy(aStatus, Localize("Downloaded"));
	else
		str_format(aStatus, sizeof(aStatus), "%s  |  page %d%s", Selected.m_FileName.c_str(), m_SkinShop.LoadedPage(), m_SkinShop.Loading() ? "  |  loading more..." : "");
	Ui()->DoLabel(&Status, aStatus, 12.0f, TEXTALIGN_ML);

	InfoColumn.HSplitBottom(34.0f, &InfoColumn, &ButtonRow);
	static CButtonContainer s_DownloadButton;
	static CButtonContainer s_DeleteButton;
	static CButtonContainer s_SelectButton;

	if(!IsInstalled)
	{
		const bool Downloading = m_SkinShop.DownloadLoading(Selected);
		const char *pLabel = Downloading ? Localize("Downloading...") : Localize("Download");
		if(DoButton_Menu(&s_DownloadButton, pLabel, 0, &ButtonRow) && !Downloading)
			m_SkinShop.Download(Selected);
	}
	else
	{
		CUIRect DeleteButton, SelectButton;
		ButtonRow.VSplitMid(&DeleteButton, &SelectButton, 8.0f);
		if(DoButton_Menu(&s_DeleteButton, Localize("Delete"), 0, &DeleteButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.8f, 0.25f, 0.25f, 0.65f)))
		{
			if(IsSelected)
				ExecuteAssetCommand(Console(), m_SkinShop.Category(), "default");
			m_SkinShop.DeleteInstalled(Selected);
		}

		if(DoButton_Menu(&s_SelectButton, IsSelected ? Localize("Selected") : Localize("Select texture"), IsSelected, &SelectButton) && !IsSelected)
			ExecuteAssetCommand(Console(), m_SkinShop.Category(), AssetName.c_str());
	}
}
