#include "menus.h"

#include <base/str.h>

#include <engine/console.h>
#include <engine/shared/config.h>

#include <game/client/ui_scrollregion.h>
#include <engine/image.h>
#include <engine/gfx/image_manipulation.h>
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
	// This is a trusted local settings action, not a map command.
	pConsole->ExecuteLine(aCommand, IConsole::CLIENT_ID_NO_GAME);
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

	CUIRect Header, SourceBar, CategoryBar, SearchBar;
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

	CUIRect Footer;
	MainView.HSplitBottom(22.0f, &MainView, &Footer);
	static CButtonContainer s_RefreshButton;
	CUIRect Refresh;
	Footer.VSplitRight(90.0f, &Footer, &Refresh);
	if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &Refresh))
		m_SkinShop.Refresh();

	std::vector<const SSkinShopItem *> vpItems;
	for(const auto &Item : m_SkinShop.Items())
		if(MatchesSearch(Item, s_SearchInput.GetString()))
			vpItems.push_back(&Item);

	char aCount[128];
	str_format(aCount, sizeof(aCount), "%zu textures%s", vpItems.size(), m_SkinShop.Loading() ? " • loading..." : "");
	Ui()->DoLabel(&Footer, aCount, 11.0f, TEXTALIGN_ML);
	static CScrollRegion s_Scroll;
	static std::string s_Filter;
	const std::string Filter = std::to_string((int)m_SkinShop.Category()) + ":" +
		std::to_string(m_SkinShop.SourceEnabled(ESkinShopSource::CHERYDATA)) +
		std::to_string(m_SkinShop.SourceEnabled(ESkinShopSource::TEEDATA)) + s_SearchInput.GetString();
	if(Filter != s_Filter)
	{
		s_Filter = Filter;
		s_Scroll.Reset();
		Ui()->SetActiveItem(nullptr);
	}
	for(auto &[Key, Card] : m_ShopCards)
		Card.m_Visible = false;

	CScrollRegionParams Params;
	Params.m_ScrollUnit = 220.0f;
	Params.m_ScrollbarThickness = 12.0f;
	s_Scroll.Begin(&MainView, &Params);
	const int Columns = std::clamp((int)(MainView.w / 185.0f), 1, 5);
	const float Width = MainView.w / Columns;
	for(size_t i = 0; i < vpItems.size(); ++i)
	{
		const auto &Item = *vpItems[i];
		CUIRect CardRect{MainView.x + (i % Columns) * Width, MainView.y + (i / Columns) * 220.0f, Width, 220.0f};
		if(!s_Scroll.AddRect(CardRect))
			continue;
		auto &Card = m_ShopCards[std::to_string((int)m_SkinShop.Category()) + ":" + Item.m_Id];
		Card.m_Visible = true;
		CardRect.Margin(4.0f, &CardRect);
		CardRect.Draw(ColorRGBA(0.08f, 0.09f, 0.12f, 0.75f), IGraphics::CORNER_ALL, 9.0f);
		CardRect.Margin(8.0f, &CardRect);
		CUIRect Preview, Name, Author, Status, Buttons;
		CardRect.HSplitTop(112.0f, &Preview, &CardRect);
		CardRect.HSplitTop(25.0f, &Name, &CardRect);
		CardRect.HSplitTop(18.0f, &Author, &CardRect);
		CardRect.HSplitTop(18.0f, &Status, &Buttons);
		Ui()->DoLabel(&Name, Item.m_Name.c_str(), 13.0f, TEXTALIGN_ML, {.m_MaxWidth = Name.w});
		const std::string Meta = std::string(CSkinShop::SourceName(Item.m_Source)) + " • " + Item.m_Author;
		Ui()->DoLabel(&Author, Meta.c_str(), 10.0f, TEXTALIGN_ML, {.m_MaxWidth = Author.w});

		m_SkinShop.RequestPreview(Item);
		const std::string Path = m_SkinShop.PreviewPath(Item);
		if(m_SkinShop.PreviewReady(Item) && Card.m_Path != Path)
		{
			if(Card.m_Texture.IsValid())
				Graphics()->UnloadTexture(&Card.m_Texture);
			Card.m_Path = Path;
			CImageInfo Image;
			if(Graphics()->LoadPng(Image, Path.c_str(), IStorage::TYPE_SAVE))
			{
				Card.m_Aspect = (float)Image.m_Width / maximum(1.0f, (float)Image.m_Height);
				const float Scale = minimum(1.0f, 384.0f / maximum((float)Image.m_Width, (float)Image.m_Height));
				if(Scale < 1.0f)
					ResizeImage(Image, maximum(1, (int)(Image.m_Width * Scale)), maximum(1, (int)(Image.m_Height * Scale)));
				Card.m_Texture = Graphics()->LoadTextureRawMove(Image, 0, Path.c_str());
			}
		}
		// A neutral checkerboard makes transparent parts of the actual texture visible.
		{
			const CUIRect::CScopedGlass NoGlass(-1.0f);
			Preview.Draw(ColorRGBA(0.16f, 0.16f, 0.16f, 1.0f), 0, 0);
			for(float y = 0; y < Preview.h; y += 12.0f)
				for(float x = 0; x < Preview.w; x += 12.0f)
					if(((int)(x / 12) + (int)(y / 12)) % 2 == 0)
						CUIRect{Preview.x + x, Preview.y + y, minimum(12.0f, Preview.w - x), minimum(12.0f, Preview.h - y)}.Draw(ColorRGBA(0.23f, 0.23f, 0.23f, 1), 0, 0);
		}
		if(Card.m_Texture.IsValid() && !Card.m_Texture.IsNullTexture())
		{
			const float W = minimum(Preview.w, Preview.h * Card.m_Aspect);
			const float H = W / Card.m_Aspect;
			Graphics()->WrapClamp();
			Graphics()->TextureSet(Card.m_Texture);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1, 1, 1, 1);
			IGraphics::CQuadItem Quad(Preview.x + (Preview.w - W) / 2, Preview.y + (Preview.h - H) / 2, W, H);
			Graphics()->QuadsDrawTL(&Quad, 1);
			Graphics()->QuadsEnd();
			Graphics()->WrapNormal();
		}
		else
			Ui()->DoLabel(&Preview, m_SkinShop.PreviewError(Item) || !Card.m_Path.empty() ? Localize("Preview unavailable") : Localize("Loading preview..."), 11.0f, TEXTALIGN_MC);

		const bool Installed = m_SkinShop.Installed(Item);
		const std::string AssetName = CSkinShop::AssetName(Item);
		const bool Applied = Installed && str_comp(CurrentAssetName(m_SkinShop.Category()), AssetName.c_str()) == 0;
		char aStatus[96];
		if(m_SkinShop.DownloadLoading(Item))
			str_format(aStatus, sizeof(aStatus), Localize("Downloading... %d%%"), m_SkinShop.DownloadProgress(Item));
		else
			str_copy(aStatus, m_SkinShop.DownloadError(Item) ? Localize("Download failed") : Applied ? Localize("Applied") : Installed ? Localize("Downloaded") : "");
		Ui()->DoLabel(&Status, aStatus, 10.0f, TEXTALIGN_ML, {.m_MaxWidth = Status.w});
		if(Installed)
		{
			CUIRect Apply, Delete;
			Buttons.VSplitRight(30.0f, &Apply, &Delete);
			Apply.VSplitRight(4.0f, &Apply, nullptr);
			if(DoButton_Menu(&Card.m_ApplyButton, Applied ? Localize("Applied") : Localize("Apply"), Applied, &Apply) && !Applied)
				ExecuteAssetCommand(Console(), m_SkinShop.Category(), AssetName.c_str());
			if(DoButton_Menu(&Card.m_DeleteButton, "×", 0, &Delete))
			{
				if(Applied)
					ExecuteAssetCommand(Console(), m_SkinShop.Category(), "default");
				m_SkinShop.DeleteInstalled(Item);
				if(Card.m_Texture.IsValid())
					Graphics()->UnloadTexture(&Card.m_Texture);
				Card.m_Path.clear();
			}
		}
		else if(m_SkinShop.DownloadLoading(Item))
			Ui()->DoLabel(&Buttons, Localize("Downloading..."), 11.0f, TEXTALIGN_MC);
		else if(m_SkinShop.DownloadBusy())
			Ui()->DoLabel(&Buttons, Localize("Please wait..."), 11.0f, TEXTALIGN_MC);
		else if(DoButton_Menu(&Card.m_DownloadButton, Localize("Download"), 0, &Buttons))
			m_SkinShop.Download(Item);
	}
	if(vpItems.empty())
		Ui()->DoLabel(&MainView, m_SkinShop.Loading() ? Localize("Loading shop items...") : m_SkinShop.Error() ? Localize("Could not load the shop. Try refreshing.") : Localize("No textures found."), 14.0f, TEXTALIGN_MC);
	s_Scroll.End();
	// Keep GPU memory bounded to visible cards, not the entire marketplace.
	for(auto &[Key, Card] : m_ShopCards)
		if(!Card.m_Visible)
		{
			if(Card.m_Texture.IsValid())
				Graphics()->UnloadTexture(&Card.m_Texture);
			Card.m_Path.clear();
		}
}
