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

std::string FileNameFromPath(const char *pPath)
{
	if(pPath == nullptr)
		return {};
	std::string Result(pPath);
	const size_t Slash = Result.find_last_of("/\\");
	if(Slash != std::string::npos)
		Result.erase(0, Slash + 1);
	return Result;
}

const char *TeedataEndpoint(ESkinShopCategory Category)
{
	switch(Category)
	{
	case ESkinShopCategory::GAMESKIN: return "gameskin";
	case ESkinShopCategory::PARTICLE: return "particle";
	case ESkinShopCategory::ENTITY: return "entity";
	case ESkinShopCategory::EMOTICON: return "emoticon";
	case ESkinShopCategory::CURSOR: return "cursor";
	case ESkinShopCategory::HUD:
	case ESkinShopCategory::NUM_CATEGORIES: break;
	}
	return nullptr;
}

const char *TeedataPathPrefix(ESkinShopCategory Category)
{
	switch(Category)
	{
	case ESkinShopCategory::GAMESKIN: return "/gameskins/";
	case ESkinShopCategory::PARTICLE: return "/particles/";
	case ESkinShopCategory::ENTITY: return "/entities/";
	case ESkinShopCategory::EMOTICON: return "/emoticons/";
	case ESkinShopCategory::CURSOR: return "/cursors/";
	case ESkinShopCategory::HUD:
	case ESkinShopCategory::NUM_CATEGORIES: break;
	}
	return nullptr;
}

bool SafeRemotePath(const char *pPath)
{
	return pPath != nullptr && pPath[0] != '\0' &&
		str_find(pPath, "..") == nullptr &&
		str_find(pPath, "\\") == nullptr;
}

int SourceIndex(ESkinShopSource Source)
{
	return (int)Source;
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
	if(m_pHttp != nullptr && m_pRequest == nullptr && HasMore())
		LoadNextPage();
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

const char *CSkinShop::SourceName(ESkinShopSource Source)
{
	switch(Source)
	{
	case ESkinShopSource::CHERYDATA: return "CheryData";
	case ESkinShopSource::TEEDATA: return "Teedata";
	case ESkinShopSource::NUM_SOURCES: break;
	}
	return "Unknown";
}

bool CSkinShop::SourceEnabled(ESkinShopSource Source) const
{
	const int Index = SourceIndex(Source);
	return Index >= 0 && Index < SOURCE_COUNT && m_aSourceEnabled[Index];
}

bool CSkinShop::SourceAvailable(ESkinShopSource Source) const
{
	if(Source == ESkinShopSource::CHERYDATA)
		return true;
	if(Source == ESkinShopSource::TEEDATA)
		return m_Category != ESkinShopCategory::HUD;
	return false;
}

bool CSkinShop::HasMore() const
{
	for(int i = 0; i < SOURCE_COUNT; ++i)
	{
		const ESkinShopSource Source = (ESkinShopSource)i;
		if(SourceEnabled(Source) && SourceAvailable(Source) && m_aHasMore[i])
			return true;
	}
	return false;
}

bool CSkinShop::Error() const
{
	bool HasActiveSource = false;
	bool HasHealthySource = false;
	for(int i = 0; i < SOURCE_COUNT; ++i)
	{
		const ESkinShopSource Source = (ESkinShopSource)i;
		if(!SourceEnabled(Source) || !SourceAvailable(Source))
			continue;
		HasActiveSource = true;
		if(!m_aSourceError[i])
			HasHealthySource = true;
	}
	return HasActiveSource && !HasHealthySource;
}

int CSkinShop::LoadedPage() const
{
	int Page = 0;
	for(int i = 0; i < SOURCE_COUNT; ++i)
	{
		const ESkinShopSource Source = (ESkinShopSource)i;
		if(SourceEnabled(Source) && SourceAvailable(Source))
			Page = std::max(Page, m_aLoadedPage[i]);
	}
	return Page;
}

void CSkinShop::BuildPageUrl(char *pBuffer, size_t BufferSize, ESkinShopSource Source, ESkinShopCategory Category, int Page)
{
	if(Page < 1)
		Page = 1;

	if(Source == ESkinShopSource::CHERYDATA)
	{
		str_format(pBuffer, BufferSize, "https://teeworlds.xyz/api/skins?page=%d&limit=%d&type=%s&sort=newest", Page, PAGE_LIMIT, CategoryType(Category));
		return;
	}

	if(Source == ESkinShopSource::TEEDATA)
	{
		const char *pEndpoint = TeedataEndpoint(Category);
		if(pEndpoint != nullptr)
		{
			str_format(pBuffer, BufferSize, "https://teedata.net/api/%s/read?limit=1000000000000000000", pEndpoint);
			return;
		}
	}

	pBuffer[0] = '\0';
}

std::string CSkinShop::BuildImageUrl(ESkinShopSource Source, ESkinShopCategory Category, const char *pImageUrl)
{
	if(!SafeRemotePath(pImageUrl))
		return {};

	if(Source == ESkinShopSource::CHERYDATA)
	{
		if(str_startswith(pImageUrl, "https://teeworlds.xyz/"))
			return pImageUrl;
		if(str_startswith(pImageUrl, "/uploads/"))
			return std::string("https://teeworlds.xyz") + pImageUrl;
		return {};
	}

	if(Source == ESkinShopSource::TEEDATA)
	{
		const char *pExpectedPrefix = TeedataPathPrefix(Category);
		if(pExpectedPrefix == nullptr)
			return {};

		if(str_startswith(pImageUrl, pExpectedPrefix))
			return std::string("https://teedata.net") + pImageUrl;

		constexpr const char *pTeedataRoot = "https://teedata.net";
		if(str_startswith(pImageUrl, pTeedataRoot))
		{
			const char *pPath = pImageUrl + str_length(pTeedataRoot);
			if(str_startswith(pPath, pExpectedPrefix))
				return pImageUrl;
		}
	}

	return {};
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
	if(Id.size() > 16)
		Id.erase(0, Id.size() - 16);
	const std::string Suffix = "_" + Id;
	// cl_asset_* buffers contain 50 bytes including the terminator.
	const size_t MaxBaseLength = 49 - Suffix.size();
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
	return m_FailedPreviews.count(Item.m_Id) != 0;
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
	const std::string &PreviewUrl = Item.m_PreviewUrl.empty() ? Item.m_ImageUrl : Item.m_PreviewUrl;
	if(m_pHttp == nullptr || m_pStorage == nullptr || PreviewUrl.empty() || PreviewReady(Item))
		return;

	// Visible cards request previews each frame. Finish one before starting another.
	if(m_pPreviewRequest != nullptr || PreviewError(Item))
		return;

	m_PreviewItemId = Item.m_Id;
	m_PreviewRequestPath = CachedPreviewPath(Item);
	m_PreviewError = false;
	m_pPreviewRequest = HttpGetFile(PreviewUrl.c_str(), m_pStorage, m_PreviewRequestPath.c_str(), IStorage::TYPE_SAVE);
	m_pPreviewRequest->Timeout(CTimeout{10000, 30000, 500, 5});
	m_pPreviewRequest->LogProgress(HTTPLOG::FAILURE);
	m_pHttp->Run(m_pPreviewRequest);
}

void CSkinShop::Download(const SSkinShopItem &Item)
{
	if(m_pHttp == nullptr || m_pStorage == nullptr || Item.m_ImageUrl.empty() || Installed(Item))
		return;

	if(m_pDownloadRequest != nullptr)
		return;

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

void CSkinShop::ResetSourceState()
{
	for(int i = 0; i < SOURCE_COUNT; ++i)
	{
		m_aLoadedPage[i] = 0;
		m_aRequestPage[i] = 0;
		m_aHasMore[i] = true;
		m_aSourceError[i] = false;
	}
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
	ResetSourceState();
	LoadNextPage();
}

void CSkinShop::SetSourceEnabled(ESkinShopSource Source, bool Enabled)
{
	const int Index = SourceIndex(Source);
	if(Index < 0 || Index >= SOURCE_COUNT || Source == ESkinShopSource::NUM_SOURCES)
		return;
	if(!SourceAvailable(Source) && Enabled)
		return;
	if(m_aSourceEnabled[Index] == Enabled)
		return;

	if(!Enabled && SourceAvailable(Source))
	{
		int ActiveSources = 0;
		for(int i = 0; i < SOURCE_COUNT; ++i)
		{
			const ESkinShopSource Other = (ESkinShopSource)i;
			if(SourceEnabled(Other) && SourceAvailable(Other))
				++ActiveSources;
		}
		if(ActiveSources <= 1)
			return;
	}

	m_aSourceEnabled[Index] = Enabled;
	Refresh();
}

void CSkinShop::Refresh()
{
	Abort();
	m_vItems.clear();
	m_ItemIds.clear();
	ResetSourceState();
	LoadNextPage();
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
	for(int i = 0; i < SOURCE_COUNT; ++i)
		m_aRequestPage[i] = 0;
	m_PreviewError = false;
	m_DownloadError = false;
	m_FailedPreviews.clear();
}

void CSkinShop::StartPage(ESkinShopSource Source, int Page)
{
	if(m_pHttp == nullptr || m_pRequest != nullptr || Page < 1 || !SourceEnabled(Source) || !SourceAvailable(Source))
		return;

	const int Index = SourceIndex(Source);
	if(!m_aHasMore[Index])
		return;

	char aUrl[256];
	BuildPageUrl(aUrl, sizeof(aUrl), Source, m_Category, Page);
	if(aUrl[0] == '\0')
	{
		m_aHasMore[Index] = false;
		m_aSourceError[Index] = true;
		return;
	}

	m_pRequest = HttpGet(aUrl);
	m_pRequest->Timeout(CTimeout{10000, 20000, 500, 5});
	m_pRequest->LogProgress(HTTPLOG::FAILURE);
	m_RequestSource = Source;
	m_aRequestPage[Index] = Page;
	m_pHttp->Run(m_pRequest);
}

void CSkinShop::LoadNextPage()
{
	if(m_pRequest != nullptr)
		return;

	for(int i = 0; i < SOURCE_COUNT; ++i)
	{
		const ESkinShopSource Source = (ESkinShopSource)i;
		if(SourceEnabled(Source) && SourceAvailable(Source) && m_aHasMore[i] && m_aLoadedPage[i] == 0)
		{
			StartPage(Source, 1);
			return;
		}
	}

	int BestSource = -1;
	for(int i = 0; i < SOURCE_COUNT; ++i)
	{
		const ESkinShopSource Source = (ESkinShopSource)i;
		if(!SourceEnabled(Source) || !SourceAvailable(Source) || !m_aHasMore[i])
			continue;
		if(BestSource < 0 || m_aLoadedPage[i] < m_aLoadedPage[BestSource])
			BestSource = i;
	}
	if(BestSource >= 0)
		StartPage((ESkinShopSource)BestSource, m_aLoadedPage[BestSource] + 1);
}

void CSkinShop::Update()
{
	if(m_pRequest != nullptr && m_pRequest->Done())
	{
		std::shared_ptr<IHttpRequest> pRequest;
		std::swap(m_pRequest, pRequest);
		const ESkinShopSource RequestSource = m_RequestSource;
		const int Source = SourceIndex(RequestSource);
		const int RequestPage = m_aRequestPage[Source];
		m_aRequestPage[Source] = 0;

		if(pRequest->State() != EHttpState::DONE)
		{
			m_aSourceError[Source] = true;
			m_aHasMore[Source] = false;
		}
		else
		{
			json_value *pJson = pRequest->ResultJson();
			std::vector<SSkinShopItem> vPageItems;
			bool HasMoreItems = false;
			const bool Success = pJson != nullptr && ParsePage(RequestSource, RequestPage, pJson, vPageItems, HasMoreItems);
			if(pJson != nullptr)
				json_value_free(pJson);

			if(!Success)
			{
				m_aSourceError[Source] = true;
				m_aHasMore[Source] = false;
			}
			else
			{
				for(SSkinShopItem &Item : vPageItems)
				{
					if(m_ItemIds.insert(Item.m_Id).second)
						m_vItems.emplace_back(std::move(Item));
				}
				m_aLoadedPage[Source] = RequestPage;
				m_aHasMore[Source] = HasMoreItems;
				m_aSourceError[Source] = false;
			}
		}
	}

	if(m_pPreviewRequest != nullptr && m_pPreviewRequest->Done())
	{
		const bool Success = m_pPreviewRequest->State() == EHttpState::DONE && m_pStorage != nullptr && m_pStorage->FileExists(m_PreviewRequestPath.c_str(), IStorage::TYPE_SAVE);
		m_pPreviewRequest.reset();
		m_PreviewError = !Success;
		if(!Success)
			m_FailedPreviews.insert(m_PreviewItemId);
	}

	if(m_pDownloadRequest != nullptr && m_pDownloadRequest->Done())
	{
		const bool Success = m_pDownloadRequest->State() == EHttpState::DONE && m_pStorage != nullptr && m_pStorage->FileExists(m_DownloadRequestPath.c_str(), IStorage::TYPE_SAVE);
		m_pDownloadRequest.reset();
		m_DownloadError = !Success;
	}
}

bool CSkinShop::ParsePage(ESkinShopSource Source, int /*Page*/, json_value *pJson, std::vector<SSkinShopItem> &vItems, bool &HasMoreItems) const
{
	if(pJson == nullptr || pJson->type != json_object)
		return false;

	vItems.clear();

	if(Source == ESkinShopSource::CHERYDATA)
	{
		const json_value &Skins = (*pJson)["skins"];
		if(Skins.type != json_array)
			return false;

		vItems.reserve(Skins.u.array.length);
		for(unsigned int i = 0; i < Skins.u.array.length; ++i)
		{
			const json_value &Skin = Skins[i];
			if(Skin.type != json_object)
				continue;

			const std::string RawId = JsonId(Skin);
			const char *pName = JsonString(Skin, "name");
			const char *pFileName = JsonString(Skin, "filename");
			const char *pType = JsonString(Skin, "type");
			const char *pImageUrl = JsonString(Skin, "imageUrl");
			if(pImageUrl == nullptr)
				pImageUrl = JsonString(Skin, "image_url");

			if(RawId.empty() || pName == nullptr || pFileName == nullptr || pType == nullptr || pImageUrl == nullptr)
				continue;
			if(str_comp(pType, CategoryType(m_Category)) != 0)
				continue;

			const std::string ImageUrl = BuildImageUrl(Source, m_Category, pImageUrl);
			if(ImageUrl.empty())
				continue;

			const char *pAuthor = JsonString(Skin, "author_name");
			if(pAuthor == nullptr || pAuthor[0] == '\0')
				pAuthor = JsonString(Skin, "author_username");
			if(pAuthor == nullptr || pAuthor[0] == '\0')
				pAuthor = JsonString(Skin, "username");
			if(pAuthor == nullptr || pAuthor[0] == '\0')
				pAuthor = JsonString(Skin, "author");

			SSkinShopItem Item;
			Item.m_Id = "cherydata:" + RawId;
			Item.m_Name = pName;
			Item.m_FileName = pFileName;
			Item.m_Type = pType;
			Item.m_Author = pAuthor != nullptr ? pAuthor : "";
			Item.m_ImageUrl = ImageUrl;
			Item.m_PreviewUrl = ImageUrl;
			Item.m_Source = Source;
			Item.m_Width = JsonInt(Skin, "width");
			Item.m_Height = JsonInt(Skin, "height");
			Item.m_Downloads = JsonInt(Skin, "downloads");
			vItems.emplace_back(std::move(Item));
		}

		HasMoreItems = Skins.u.array.length >= PAGE_LIMIT;
		return true;
	}

	if(Source == ESkinShopSource::TEEDATA)
	{
		if(!SourceAvailable(Source))
			return false;

		const json_value &Result = (*pJson)["result"];
		if(Result.type != json_object)
			return false;

		const json_value &Items = Result["items"];
		if(Items.type != json_array)
			return false;

		vItems.reserve(Items.u.array.length);
		for(unsigned int i = 0; i < Items.u.array.length; ++i)
		{
			const json_value &Data = Items[i];
			if(Data.type != json_object)
				continue;

			const std::string RawId = JsonId(Data);
			const char *pName = JsonString(Data, "name");
			const char *pFilePath = JsonString(Data, "file_path");
			const char *pThumbnailPath = JsonString(Data, "thumbnail_path");
			if(RawId.empty() || pName == nullptr || pFilePath == nullptr)
				continue;

			const std::string ImageUrl = BuildImageUrl(Source, m_Category, pFilePath);
			if(ImageUrl.empty())
				continue;
			std::string PreviewUrl = BuildImageUrl(Source, m_Category, pThumbnailPath);
			if(PreviewUrl.empty())
				PreviewUrl = ImageUrl;

			const json_value &Author = Data["author"];
			const char *pAuthor = Author.type == json_object ? JsonString(Author, "name") : nullptr;
			const json_value &Counts = Data["_count"];

			SSkinShopItem Item;
			Item.m_Id = "teedata:" + RawId;
			Item.m_Name = pName;
			Item.m_FileName = FileNameFromPath(pFilePath);
			Item.m_Type = CategoryType(m_Category);
			Item.m_Author = pAuthor != nullptr ? pAuthor : "";
			Item.m_ImageUrl = ImageUrl;
			Item.m_PreviewUrl = PreviewUrl;
			Item.m_Source = Source;
			Item.m_Width = JsonInt(Data, "width");
			Item.m_Height = JsonInt(Data, "height");
			Item.m_Downloads = Counts.type == json_object ? JsonInt(Counts, "downloads") : 0;
			vItems.emplace_back(std::move(Item));
		}

		HasMoreItems = false;
		return true;
	}

	return false;
}
