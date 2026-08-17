#include "skinshop.h"

#include <base/str.h>
#include <engine/http.h>
#include <engine/shared/json.h>
#include <engine/storage.h>

#include <algorithm>
#include <cctype>
#include <utility>

namespace
{
const char *JsonString(const json_value &Object, const char *pKey)
{
	const json_value &Value = Object[pKey];
	return Value.type == json_string ? json_string_get(&Value) : nullptr;
}

int JsonInt(const json_value &Object, const char *pKey)
{
	const json_value &Value = Object[pKey];
	return Value.type == json_integer ? json_int_get(&Value) : 0;
}

std::string JsonId(const json_value &Object)
{
	const json_value &Value = Object["id"];
	if(Value.type == json_string)
		return json_string_get(&Value);
	if(Value.type == json_integer)
		return std::to_string(json_int_get(&Value));
	return {};
}

std::string SafePart(const std::string &Value)
{
	std::string Result;
	Result.reserve(Value.size());
	for(unsigned char C : Value)
	{
		if(std::isalnum(C) || C == '-' || C == '_' || C == '.' || C == ' ')
			Result.push_back((char)C);
		else
			Result.push_back('_');
	}
	while(!Result.empty() && (Result.front() == '.' || Result.front() == ' '))
		Result.erase(Result.begin());
	while(!Result.empty() && (Result.back() == '.' || Result.back() == ' '))
		Result.pop_back();
	return Result;
}

bool EndsWithPng(const std::string &Value)
{
	if(Value.size() < 4)
		return false;
	const size_t Pos = Value.size() - 4;
	return Value[Pos] == '.' &&
		std::tolower((unsigned char)Value[Pos + 1]) == 'p' &&
		std::tolower((unsigned char)Value[Pos + 2]) == 'n' &&
		std::tolower((unsigned char)Value[Pos + 3]) == 'g';
}
} // namespace

CSkinShop::~CSkinShop()
{
	Abort();
}

void CSkinShop::Init(IHttp *pHttp, IStorage *pStorage)
{
	m_pHttp = pHttp;
	m_pStorage = pStorage;
	PrepareStorage();
	if(m_pHttp != nullptr && m_vItems.empty() && m_pRequest == nullptr)
		StartPage(1);
}

void CSkinShop::PrepareStorage()
{
	if(m_StoragePrepared || m_pStorage == nullptr)
		return;
	if(!m_pStorage->FolderExists("skinshop", IStorage::TYPE_SAVE))
		m_pStorage->CreateFolder("skinshop", IStorage::TYPE_SAVE);
	if(!m_pStorage->FolderExists("skinshop/previews", IStorage::TYPE_SAVE))
		m_pStorage->CreateFolder("skinshop/previews", IStorage::TYPE_SAVE);
	if(!m_pStorage->FolderExists("assets", IStorage::TYPE_SAVE))
		m_pStorage->CreateFolder("assets", IStorage::TYPE_SAVE);
	m_StoragePrepared = true;
}

const char *CSkinShop::CategoryType(ESkinShopCategory Category)
{
	switch(Category)
	{
	case ESkinShopCategory::GAMESKIN: return "gameskin";
	case ESkinShopCategory::PARTICLE: return "particle";
	case ESkinShopCategory::ENTITY: return "entity";
	case ESkinShopCategory::EMOTICON: return "emoticon";
	case ESkinShopCategory::HUD: return "hud";
	case ESkinShopCategory::CURSOR: return "cursor";
	case ESkinShopCategory::NUM_CATEGORIES: break;
	}
	return "gameskin";
}

const char *CSkinShop::CategoryName(ESkinShopCategory Category)
{
	switch(Category)
	{
	case ESkinShopCategory::GAMESKIN: return "Gun packs";
	case ESkinShopCategory::PARTICLE: return "Particles";
	case ESkinShopCategory::ENTITY: return "Tiles";
	case ESkinShopCategory::EMOTICON: return "Emoticons";
	case ESkinShopCategory::HUD: return "HUD";
	case ESkinShopCategory::CURSOR: return "Cursors";
	case ESkinShopCategory::NUM_CATEGORIES: break;
	}
	return "Gun packs";
}

const char *CSkinShop::CategoryAssetFolder(ESkinShopCategory Category)
{
	switch(Category)
	{
	case ESkinShopCategory::GAMESKIN: return "game";
	case ESkinShopCategory::PARTICLE: return "particles";
	case ESkinShopCategory::ENTITY: return "entities";
	case ESkinShopCategory::EMOTICON: return "emoticons";
	case ESkinShopCategory::HUD: return "hud";
	case ESkinShopCategory::CURSOR: return "cursor";
	case ESkinShopCategory::NUM_CATEGORIES: break;
	}
	return "game";
}

const char *CSkinShop::CategoryConfigCommand(ESkinShopCategory Category)
{
	switch(Category)
	{
	case ESkinShopCategory::GAMESKIN: return "cl_asset_game";
	case ESkinShopCategory::PARTICLE: return "cl_asset_particles";
	case ESkinShopCategory::ENTITY: return "cl_assets_entities";
	case ESkinShopCategory::EMOTICON: return "cl_asset_emoticons";
	case ESkinShopCategory::HUD: return "cl_asset_hud";
	case ESkinShopCategory::CURSOR: return "cl_asset_cursor";
	case ESkinShopCategory::NUM_CATEGORIES: break;
	}
	return "cl_asset_game";
}

void CSkinShop::BuildPageUrl(char *pBuffer, size_t BufferSize, ESkinShopCategory Category, int Page)
{
	if(Page < 1)
		Page = 1;
	str_format(pBuffer, BufferSize, "https://teeworlds.xyz/api/skins?page=%d&limit=%d&type=%s&sort=newest", Page, PAGE_LIMIT, CategoryType(Category));
}

std::string CSkinShop::BuildImageUrl(const char *pImageUrl)
{
	if(pImageUrl == nullptr || pImageUrl[0] == '\0')
		return {};
	if(pImageUrl[0] == '/')
		return std::string("https://teeworlds.xyz") + pImageUrl;
	return pImageUrl;
}

std::string CSkinShop::AssetName(const SSkinShopItem &Item)
{
	std::string Base = Item.m_Name.empty() ? Item.m_FileName : Item.m_Name;
	const size_t Slash = Base.find_last_of("/\\");
	if(Slash != std::string::npos)
		Base.erase(0, Slash + 1);
	if(EndsWithPng(Base))
		Base.resize(Base.size() - 4);
	Base = SafePart(Base);
	if(Base.empty())
		Base = "texture";

	std::string Id = SafePart(Item.m_Id);
	if(Id.empty())
		Id = "item";
	if(Id.size() > 8)
		Id.resize(8);
	const std::string Suffix = "_" + Id;
	const size_t MaxBaseLength = 48 > Suffix.size() ? 48 - Suffix.size() : 1;
	if(Base.size() > MaxBaseLength)
		Base.resize(MaxBaseLength);
	return Base + Suffix;
}

std::string CSkinShop::CachedPreviewPath(const SSkinShopItem &Item) const
{
	std::string Id = SafePart(Item.m_Id);
	if(Id.empty())
		Id = AssetName(Item);
	if(Id.size() > 80)
		Id.resize(80);
	return "skinshop/previews/" + Id + ".png";
}

std::string CSkinShop::InstallPath(const SSkinShopItem &Item) const
{
	return std::string("assets/") + CategoryAssetFolder(m_Category) + "/" + AssetName(Item) + ".png";
}

std::string CSkinShop::PreviewPath(const SSkinShopItem &Item) const
{
	if(Installed(Item))
		return InstallPath(Item);
	return CachedPreviewPath(Item);
}

bool CSkinShop::Installed(const SSkinShopItem &Item) const
{
	return m_pStorage != nullptr && m_pStorage->FileExists(InstallPath(Item).c_str(), IStorage::TYPE_SAVE);
}

bool CSkinShop::PreviewReady(const SSkinShopItem &Item) const
{
	return m_pStorage != nullptr && m_pStorage->FileExists(PreviewPath(Item).c_str(), IStorage::TYPE_SAVE);
}

bool CSkinShop::PreviewLoading(const SSkinShopItem &Item) const
{
	return m_pPreviewRequest != nullptr && m_PreviewItemId == Item.m_Id;
}

bool CSkinShop::PreviewError(const SSkinShopItem &Item) const
{
	return m_PreviewError && m_PreviewItemId == Item.m_Id;
}

bool CSkinShop::DownloadLoading(const SSkinShopItem &Item) const
{
	return m_pDownloadRequest != nullptr && m_DownloadItemId == Item.m_Id;
}

bool CSkinShop::DownloadError(const SSkinShopItem &Item) const
{
	return m_DownloadError && m_DownloadItemId == Item.m_Id;
}

int CSkinShop::DownloadProgress(const SSkinShopItem &Item) const
{
	if(DownloadLoading(Item))
		return m_pDownloadRequest->Progress();
	return Installed(Item) ? 100 : 0;
}

void CSkinShop::RequestPreview(const SSkinShopItem &Item)
{
	if(m_pHttp == nullptr || m_pStorage == nullptr || Item.m_ImageUrl.empty() || PreviewReady(Item))
		return;

	if(m_pPreviewRequest != nullptr)
	{
		if(m_PreviewItemId == Item.m_Id)
			return;
		m_pPreviewRequest->Abort();
		m_pPreviewRequest.reset();
	}

	m_PreviewItemId = Item.m_Id;
	m_PreviewRequestPath = CachedPreviewPath(Item);
	m_PreviewError = false;
	m_pPreviewRequest = HttpGetFile(Item.m_ImageUrl.c_str(), m_pStorage, m_PreviewRequestPath.c_str(), IStorage::TYPE_SAVE);
	m_pPreviewRequest->Timeout(CTimeout{10000, 30000, 500, 5});
	m_pPreviewRequest->LogProgress(HTTPLOG::FAILURE);
	m_pHttp->Run(m_pPreviewRequest);
}

void CSkinShop::Download(const SSkinShopItem &Item)
{
	if(m_pHttp == nullptr || m_pStorage == nullptr || Item.m_ImageUrl.empty() || Installed(Item))
		return;

	if(m_pDownloadRequest != nullptr)
	{
		if(m_DownloadItemId == Item.m_Id)
			return;
		m_pDownloadRequest->Abort();
		m_pDownloadRequest.reset();
	}

	PrepareStorage();
	const std::string Folder = std::string("assets/") + CategoryAssetFolder(m_Category);
	if(!m_pStorage->FolderExists(Folder.c_str(), IStorage::TYPE_SAVE))
		m_pStorage->CreateFolder(Folder.c_str(), IStorage::TYPE_SAVE);

	m_DownloadItemId = Item.m_Id;
	m_DownloadRequestPath = InstallPath(Item);
	m_DownloadError = false;
	m_pDownloadRequest = HttpGetFile(Item.m_ImageUrl.c_str(), m_pStorage, m_DownloadRequestPath.c_str(), IStorage::TYPE_SAVE);
	m_pDownloadRequest->Timeout(CTimeout{10000, 60000, 500, 5});
	m_pDownloadRequest->LogProgress(HTTPLOG::FAILURE);
	m_pHttp->Run(m_pDownloadRequest);
}

bool CSkinShop::DeleteInstalled(const SSkinShopItem &Item)
{
	if(m_pStorage == nullptr)
		return false;
	if(DownloadLoading(Item))
	{
		m_pDownloadRequest->Abort();
		m_pDownloadRequest.reset();
	}
	const std::string Path = InstallPath(Item);
	if(!m_pStorage->FileExists(Path.c_str(), IStorage::TYPE_SAVE))
		return true;
	return m_pStorage->RemoveFile(Path.c_str(), IStorage::TYPE_SAVE);
}

void CSkinShop::SetCategory(ESkinShopCategory Category)
{
	if(Category == ESkinShopCategory::NUM_CATEGORIES)
		return;
	if(Category == m_Category && (!m_vItems.empty() || m_pRequest != nullptr))
		return;

	Abort();
	m_Category = Category;
	m_vItems.clear();
	m_ItemIds.clear();
	m_LoadedPage = 0;
	m_RequestPage = 0;
	m_HasMore = true;
	m_Error = false;
	StartPage(1);
}

void CSkinShop::Refresh()
{
	Abort();
	m_vItems.clear();
	m_ItemIds.clear();
	m_LoadedPage = 0;
	m_RequestPage = 0;
	m_HasMore = true;
	m_Error = false;
	StartPage(1);
}

void CSkinShop::Abort()
{
	if(m_pRequest != nullptr)
	{
		m_pRequest->Abort();
		m_pRequest.reset();
	}
	if(m_pPreviewRequest != nullptr)
	{
		m_pPreviewRequest->Abort();
		m_pPreviewRequest.reset();
	}
	if(m_pDownloadRequest != nullptr)
	{
		m_pDownloadRequest->Abort();
		m_pDownloadRequest.reset();
	}
	m_RequestPage = 0;
	m_PreviewError = false;
	m_DownloadError = false;
}

void CSkinShop::StartPage(int Page)
{
	if(m_pHttp == nullptr || m_pRequest != nullptr || !m_HasMore || Page < 1)
		return;

	char aUrl[256];
	BuildPageUrl(aUrl, sizeof(aUrl), m_Category, Page);
	m_pRequest = HttpGet(aUrl);
	m_pRequest->Timeout(CTimeout{10000, 20000, 500, 5});
	m_pRequest->LogProgress(HTTPLOG::FAILURE);
	m_RequestPage = Page;
	m_pHttp->Run(m_pRequest);
}

void CSkinShop::LoadNextPage()
{
	if(m_pRequest != nullptr || !m_HasMore)
		return;
	StartPage(m_LoadedPage + 1);
}

void CSkinShop::Update()
{
	if(m_pRequest != nullptr && m_pRequest->Done())
	{
		std::shared_ptr<IHttpRequest> pRequest;
		std::swap(m_pRequest, pRequest);
		const int RequestPage = m_RequestPage;
		m_RequestPage = 0;

		if(pRequest->State() != EHttpState::DONE)
		{
			m_Error = true;
		}
		else
		{
			json_value *pJson = pRequest->ResultJson();
			std::vector<SSkinShopItem> vPageItems;
			bool HasMore = false;
			const bool Success = pJson != nullptr && ParsePage(pJson, vPageItems, HasMore);
			if(pJson != nullptr)
				json_value_free(pJson);

			if(!Success)
			{
				m_Error = true;
			}
			else
			{
				for(SSkinShopItem &Item : vPageItems)
				{
					if(m_ItemIds.insert(Item.m_Id).second)
						m_vItems.emplace_back(std::move(Item));
				}
				m_LoadedPage = RequestPage;
				m_HasMore = HasMore;
				m_Error = false;
			}
		}
	}

	if(m_pPreviewRequest != nullptr && m_pPreviewRequest->Done())
	{
		const bool Success = m_pPreviewRequest->State() == EHttpState::DONE && m_pStorage != nullptr && m_pStorage->FileExists(m_PreviewRequestPath.c_str(), IStorage::TYPE_SAVE);
		m_pPreviewRequest.reset();
		m_PreviewError = !Success;
	}

	if(m_pDownloadRequest != nullptr && m_pDownloadRequest->Done())
	{
		const bool Success = m_pDownloadRequest->State() == EHttpState::DONE && m_pStorage != nullptr && m_pStorage->FileExists(m_DownloadRequestPath.c_str(), IStorage::TYPE_SAVE);
		m_pDownloadRequest.reset();
		m_DownloadError = !Success;
	}
}

bool CSkinShop::ParsePage(json_value *pJson, std::vector<SSkinShopItem> &vItems, bool &HasMore) const
{
	if(pJson == nullptr || pJson->type != json_object)
		return false;

	const json_value &Skins = (*pJson)["skins"];
	if(Skins.type != json_array)
		return false;

	vItems.clear();
	vItems.reserve(Skins.u.array.length);
	for(unsigned int i = 0; i < Skins.u.array.length; ++i)
	{
		const json_value &Skin = Skins[i];
		if(Skin.type != json_object)
			continue;

		const std::string Id = JsonId(Skin);
		const char *pName = JsonString(Skin, "name");
		const char *pFileName = JsonString(Skin, "filename");
		const char *pType = JsonString(Skin, "type");
		const char *pImageUrl = JsonString(Skin, "imageUrl");
		if(pImageUrl == nullptr)
			pImageUrl = JsonString(Skin, "image_url");

		if(Id.empty() || pName == nullptr || pFileName == nullptr || pType == nullptr || pImageUrl == nullptr)
			continue;
		if(str_comp(pType, CategoryType(m_Category)) != 0)
			continue;

		const char *pAuthor = JsonString(Skin, "author_name");
		if(pAuthor == nullptr || pAuthor[0] == '\0')
			pAuthor = JsonString(Skin, "author_username");
		if(pAuthor == nullptr || pAuthor[0] == '\0')
			pAuthor = JsonString(Skin, "username");
		if(pAuthor == nullptr || pAuthor[0] == '\0')
			pAuthor = JsonString(Skin, "author");

		SSkinShopItem Item;
		Item.m_Id = Id;
		Item.m_Name = pName;
		Item.m_FileName = pFileName;
		Item.m_Type = pType;
		Item.m_Author = pAuthor != nullptr ? pAuthor : "";
		Item.m_ImageUrl = BuildImageUrl(pImageUrl);
		Item.m_Width = JsonInt(Skin, "width");
		Item.m_Height = JsonInt(Skin, "height");
		Item.m_Downloads = JsonInt(Skin, "downloads");
		vItems.emplace_back(std::move(Item));
	}

	HasMore = Skins.u.array.length >= PAGE_LIMIT;
	return true;
}
