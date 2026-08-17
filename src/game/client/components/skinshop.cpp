#include "skinshop.h"

#include <base/str.h>
#include <engine/http.h>
#include <engine/shared/json.h>

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
} // namespace

CSkinShop::~CSkinShop()
{
	Abort();
}

void CSkinShop::Init(IHttp *pHttp)
{
	m_pHttp = pHttp;
	if(m_pHttp != nullptr && m_vItems.empty() && m_pRequest == nullptr)
	{
		StartPage(1);
	}
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
	m_RequestPage = 0;
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
	if(m_pRequest == nullptr || !m_pRequest->Done())
		return;

	std::shared_ptr<IHttpRequest> pRequest;
	std::swap(m_pRequest, pRequest);
	const int RequestPage = m_RequestPage;
	m_RequestPage = 0;

	if(pRequest->State() != EHttpState::DONE)
	{
		m_Error = true;
		return;
	}

	json_value *pJson = pRequest->ResultJson();
	std::vector<SSkinShopItem> vPageItems;
	bool HasMore = false;
	const bool Success = pJson != nullptr && ParsePage(pJson, vPageItems, HasMore);
	if(pJson != nullptr)
		json_value_free(pJson);

	if(!Success)
	{
		m_Error = true;
		return;
	}

	for(SSkinShopItem &Item : vPageItems)
	{
		if(m_ItemIds.insert(Item.m_Id).second)
			m_vItems.emplace_back(std::move(Item));
	}

	m_LoadedPage = RequestPage;
	m_HasMore = HasMore;
	m_Error = false;
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

		const char *pId = JsonString(Skin, "id");
		const char *pName = JsonString(Skin, "name");
		const char *pFileName = JsonString(Skin, "filename");
		const char *pType = JsonString(Skin, "type");
		const char *pImageUrl = JsonString(Skin, "imageUrl");
		if(pImageUrl == nullptr)
			pImageUrl = JsonString(Skin, "image_base64");

		if(pId == nullptr || pName == nullptr || pFileName == nullptr || pType == nullptr || pImageUrl == nullptr)
			continue;
		if(str_comp(pType, CategoryType(m_Category)) != 0)
			continue;

		const char *pAuthor = JsonString(Skin, "author_name");
		if(pAuthor == nullptr || pAuthor[0] == '\0')
			pAuthor = JsonString(Skin, "author_username");
		if(pAuthor == nullptr || pAuthor[0] == '\0')
			pAuthor = JsonString(Skin, "username");

		SSkinShopItem Item;
		Item.m_Id = pId;
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
