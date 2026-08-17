#include "menus.h"

#include <base/str.h>

#include <engine/console.h>
#include <engine/shared/config.h>

#include <game/client/ui_listbox.h>
#include <game/localization.h>

#include <algorithm>
#include <string>
#include <vector>

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

bool MatchesSearch(const SSkinShopItem &Item, const char *pSearch)
{
	if(pSearch == nullptr || pSearch[0] == '\0')
		return true;
	if(str_find_nocase(Item.m_Name.c_str(), pSearch) != nullptr)
		return true;
	if(str_find_nocase(Item.m_FileName.c_str(), pSearch) != nullptr)
		return true;
	if(!Item.m_Author.empty() && str_find_nocase(Item.m_Author.c_str(), pSearch) != nullptr)
		return true;
	return str_find_nocase(CSkinShop::SourceName(Item.m_Source), pSearch) != nullptr;
}
} // namespace

void CMenus::RenderSettingsAssetsShop(CUIRect MainView)
{
	m_SkinShop.Init(Http(), Storage());
	m_SkinShop.Update();

	if(!m_SkinShop.Loading() && m_SkinShop.HasMore())
		m_SkinShop.LoadNextPage();

	CUIRect Header, SourceBar, CategoryBar, SearchBar, ListView, Details;
	MainView.HSplitTop(24.0f, &Header, &MainView);
	Ui()->DoLabel(&Header, Localize("Texture shop"), 18.0f, TEXTALIGN_ML);
	MainView.HSplitTop(5.0f, nullptr, &MainView);

	MainView.HSplitTop(22.0f, &SourceBar, &MainView);
	CUIRect SourceLabel, CheryDataButton, TeedataButton;
	SourceBar.VSplitLeft(80.0f, &SourceLabel, &SourceBar);
	Ui()->DoLabel(&SourceLabel, Localize("Sources:"), 13.0f, TEXTALIGN_ML);
	SourceBar.VSplitLeft(135.0f, &CheryDataButton, &SourceBar);
	SourceBar.VSplitLeft(150.0f, &TeedataButton, &SourceBar);

	static int s_CheryDataSourceId;
	static int s_TeedataSourceId;
	const bool CheryDataEnabled = m_SkinShop.SourceEnabled(ESkinShopSource::CHERYDATA);
	if(DoButton_CheckBox(&s_CheryDataSourceId, "CheryData", CheryDataEnabled, &CheryDataButton))
		m_SkinShop.SetSourceEnabled(ESkinShopSource::CHERYDATA, !CheryDataEnabled);

	const bool TeedataAvailable = m_SkinShop.SourceAvailable(ESkinShopSource::TEEDATA);
	if(TeedataAvailable)
	{
		const bool TeedataEnabled = m_SkinShop.SourceEnabled(ESkinShopSource::TEEDATA);
		if(DoButton_CheckBox(&s_TeedataSourceId, "Teedata", TeedataEnabled, &TeedataButton))
			m_SkinShop.SetSourceEnabled(ESkinShopSource::TEEDATA, !TeedataEnabled);
	}
	else
	{
		TextRender()->TextColor(0.55f, 0.55f, 0.55f, 1.0f);
		Ui()->DoLabel(&TeedataButton, "Teedata (no HUD)", 12.0f, TEXTALIGN_ML);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}

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

	MainView.HSplitTop(6.0f, nullptr, &MainView);
	MainView.HSplitTop(24.0f, &SearchBar, &MainView);
	CUIRect SearchLabel, SearchBox;
	SearchBar.VSplitLeft(62.0f, &SearchLabel, &SearchBox);
	Ui()->DoLabel(&SearchLabel, Localize("Search:"), 13.0f, TEXTALIGN_ML);
	static CLineInputBuffered<128> s_SearchInput;
	Ui()->DoClearableEditBox(&s_SearchInput, &SearchBox, 12.0f);

	MainView.HSplitTop(8.0f, nullptr, &MainView);
	MainView.HSplitBottom(210.0f, &ListView, &Details);
	Details.HSplitTop(8.0f, nullptr, &Details);

	const auto &vItems = m_SkinShop.Items();
	std::vector<const SSkinShopItem *> vpFilteredItems;
	vpFilteredItems.reserve(vItems.size());
	const char *pSearch = s_SearchInput.GetString();
	for(const SSkinShopItem &Item : vItems)
	{
		if(MatchesSearch(Item, pSearch))
			vpFilteredItems.push_back(&Item);
	}

	static CListBox s_ShopListBox;
	static int s_Selected = -1;
	static ESkinShopCategory s_SelectedCategory = ESkinShopCategory::NUM_CATEGORIES;
	static int s_SourceMask = -1;
	static std::string s_LastSearch;
	static IGraphics::CTextureHandle s_PreviewTexture;
	static std::string s_PreviewPath;

	auto ClearPreview = [&]() {
		if(s_PreviewTexture.IsValid() && !s_PreviewTexture.IsNullTexture())
			Graphics()->UnloadTexture(&s_PreviewTexture);
		s_PreviewTexture.Invalidate();
		s_PreviewPath.clear();
	};

	const int SourceMask =
		(m_SkinShop.SourceEnabled(ESkinShopSource::CHERYDATA) ? 1 : 0) |
		(m_SkinShop.SourceEnabled(ESkinShopSource::TEEDATA) ? 2 : 0);
	if(s_SelectedCategory != m_SkinShop.Category() || s_SourceMask != SourceMask || s_LastSearch != pSearch)
	{
		s_SelectedCategory = m_SkinShop.Category();
		s_SourceMask = SourceMask;
		s_LastSearch = pSearch;
		s_Selected = -1;
		ClearPreview();
	}

	if(s_Selected >= (int)vpFilteredItems.size())
		s_Selected = -1;

	if(vpFilteredItems.empty())
	{
		ClearPreview();
		const char *pMessage;
		if(m_SkinShop.Loading())
			pMessage = Localize("Loading shop items...");
		else if(m_SkinShop.Error())
			pMessage = Localize("Could not load the shop. Try opening the tab again.");
		else if(pSearch[0] != '\0')
			pMessage = Localize("No textures match your search.");
		else
			pMessage = Localize("No textures found.");
		Ui()->DoLabel(&ListView, pMessage, 16.0f, TEXTALIGN_MC);
	}
	else
	{
		s_ShopListBox.DoStart(48.0f, vpFilteredItems.size(), 1, 1, s_Selected, &ListView, false);
		int Hovered = -1;
		for(size_t i = 0; i < vpFilteredItems.size(); ++i)
		{
			const SSkinShopItem &ItemData = *vpFilteredItems[i];
			const CListboxItem Item = s_ShopListBox.DoNextItem(&ItemData, s_Selected == (int)i);
			if(!Item.m_Visible)
				continue;

			if(Ui()->MouseInside(&Item.m_Rect))
				Hovered = (int)i;

			CUIRect Row = Item.m_Rect;
			Row.Margin(5.0f, &Row);
			CUIRect Name, Meta;
			Row.HSplitTop(20.0f, &Name, &Meta);
			Ui()->DoLabel(&Name, ItemData.m_Name.c_str(), 15.0f, TEXTALIGN_ML);

			char aMeta[320];
			const char *pSourceName = CSkinShop::SourceName(ItemData.m_Source);
			if(!ItemData.m_Author.empty())
				str_format(aMeta, sizeof(aMeta), "%s  |  %s  |  %dx%d  |  %s", pSourceName, ItemData.m_Author.c_str(), ItemData.m_Width, ItemData.m_Height, ItemData.m_Type.c_str());
			else
				str_format(aMeta, sizeof(aMeta), "%s  |  %dx%d  |  %s", pSourceName, ItemData.m_Width, ItemData.m_Height, ItemData.m_Type.c_str());
			Ui()->DoLabel(&Meta, aMeta, 12.0f, TEXTALIGN_ML);
		}

		const int ClickedSelected = s_ShopListBox.DoEnd();
		if(ClickedSelected >= 0)
			s_Selected = ClickedSelected;
		if(Hovered >= 0)
			s_Selected = Hovered;
		if(s_Selected < 0)
			s_Selected = 0;
	}

	if(s_Selected < 0 || s_Selected >= (int)vpFilteredItems.size())
	{
		char aStatus[192];
		str_format(aStatus, sizeof(aStatus), "%zu / %zu items%s", vpFilteredItems.size(), vItems.size(), m_SkinShop.Loading() ? "  |  loading more..." : "");
		Ui()->DoLabel(&Details, aStatus, 13.0f, TEXTALIGN_ML);
		return;
	}

	const SSkinShopItem &Selected = *vpFilteredItems[s_Selected];
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
	else if(!s_PreviewPath.empty())
	{
		ClearPreview();
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
	char aMeta[320];
	str_format(aMeta, sizeof(aMeta), "%s  |  %s%s%s  |  %dx%d",
		CSkinShop::SourceName(Selected.m_Source),
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
	char aStatus[224];
	if(m_SkinShop.DownloadLoading(Selected))
		str_format(aStatus, sizeof(aStatus), Localize("Downloading... %d%%"), m_SkinShop.DownloadProgress(Selected));
	else if(m_SkinShop.DownloadError(Selected))
		str_copy(aStatus, Localize("Download failed. You can try again."));
	else if(IsSelected)
		str_copy(aStatus, Localize("Downloaded and selected"));
	else if(IsInstalled)
		str_copy(aStatus, Localize("Downloaded"));
	else
		str_format(aStatus, sizeof(aStatus), "%s  |  %s%s", Selected.m_FileName.c_str(), CSkinShop::SourceName(Selected.m_Source), m_SkinShop.Loading() ? "  |  loading more..." : "");
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
