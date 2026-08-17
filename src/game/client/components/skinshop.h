#ifndef GAME_CLIENT_COMPONENTS_SKINSHOP_H
#define GAME_CLIENT_COMPONENTS_SKINSHOP_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class IHttp;
class IHttpRequest;
typedef struct _json_value json_value;

enum class ESkinShopCategory
{
	GAMESKIN = 0,
	PARTICLE,
	ENTITY,
	EMOTICON,
	HUD,
	CURSOR,
	NUM_CATEGORIES,
};

struct SSkinShopItem
{
	std::string m_Id;
	std::string m_Name;
	std::string m_FileName;
	std::string m_Type;
	std::string m_Author;
	std::string m_ImageUrl;
	int m_Width = 0;
	int m_Height = 0;
	int m_Downloads = 0;
};

class CSkinShop
{
public:
	static constexpr int PAGE_LIMIT = 100;

	CSkinShop() = default;
	~CSkinShop();

	void Init(IHttp *pHttp);
	void SetCategory(ESkinShopCategory Category);
	void Refresh();
	void Update();
	void LoadNextPage();
	void Abort();

	ESkinShopCategory Category() const { return m_Category; }
	const std::vector<SSkinShopItem> &Items() const { return m_vItems; }
	bool Loading() const { return m_pRequest != nullptr; }
	bool HasMore() const { return m_HasMore; }
	bool Error() const { return m_Error; }
	int LoadedPage() const { return m_LoadedPage; }

	static const char *CategoryType(ESkinShopCategory Category);
	static const char *CategoryName(ESkinShopCategory Category);
	static void BuildPageUrl(char *pBuffer, size_t BufferSize, ESkinShopCategory Category, int Page);
	static std::string BuildImageUrl(const char *pImageUrl);

private:
	void StartPage(int Page);
	bool ParsePage(json_value *pJson, std::vector<SSkinShopItem> &vItems, bool &HasMore) const;

	IHttp *m_pHttp = nullptr;
	std::shared_ptr<IHttpRequest> m_pRequest;
	ESkinShopCategory m_Category = ESkinShopCategory::GAMESKIN;
	std::vector<SSkinShopItem> m_vItems;
	std::unordered_set<std::string> m_ItemIds;
	int m_LoadedPage = 0;
	int m_RequestPage = 0;
	bool m_HasMore = true;
	bool m_Error = false;
};

#endif // GAME_CLIENT_COMPONENTS_SKINSHOP_H
