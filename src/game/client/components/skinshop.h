#ifndef GAME_CLIENT_COMPONENTS_SKINSHOP_H
#define GAME_CLIENT_COMPONENTS_SKINSHOP_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class IHttp;
class IHttpRequest;
class IStorage;
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

	void Init(IHttp *pHttp, IStorage *pStorage);
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

	void RequestPreview(const SSkinShopItem &Item);
	bool PreviewReady(const SSkinShopItem &Item) const;
	bool PreviewLoading(const SSkinShopItem &Item) const;
	bool PreviewError(const SSkinShopItem &Item) const;
	std::string PreviewPath(const SSkinShopItem &Item) const;

	void Download(const SSkinShopItem &Item);
	bool Installed(const SSkinShopItem &Item) const;
	bool DownloadLoading(const SSkinShopItem &Item) const;
	bool DownloadError(const SSkinShopItem &Item) const;
	int DownloadProgress(const SSkinShopItem &Item) const;
	bool DeleteInstalled(const SSkinShopItem &Item);
	std::string InstallPath(const SSkinShopItem &Item) const;

	static const char *CategoryType(ESkinShopCategory Category);
	static const char *CategoryName(ESkinShopCategory Category);
	static const char *CategoryAssetFolder(ESkinShopCategory Category);
	static const char *CategoryConfigCommand(ESkinShopCategory Category);
	static void BuildPageUrl(char *pBuffer, size_t BufferSize, ESkinShopCategory Category, int Page);
	static std::string BuildImageUrl(const char *pImageUrl);
	static std::string AssetName(const SSkinShopItem &Item);

private:
	void StartPage(int Page);
	bool ParsePage(json_value *pJson, std::vector<SSkinShopItem> &vItems, bool &HasMore) const;
	void PrepareStorage();
	std::string CachedPreviewPath(const SSkinShopItem &Item) const;

	IHttp *m_pHttp = nullptr;
	IStorage *m_pStorage = nullptr;
	std::shared_ptr<IHttpRequest> m_pRequest;
	std::shared_ptr<IHttpRequest> m_pPreviewRequest;
	std::shared_ptr<IHttpRequest> m_pDownloadRequest;
	ESkinShopCategory m_Category = ESkinShopCategory::GAMESKIN;
	std::vector<SSkinShopItem> m_vItems;
	std::unordered_set<std::string> m_ItemIds;
	std::string m_PreviewItemId;
	std::string m_PreviewRequestPath;
	std::string m_DownloadItemId;
	std::string m_DownloadRequestPath;
	int m_LoadedPage = 0;
	int m_RequestPage = 0;
	bool m_HasMore = true;
	bool m_Error = false;
	bool m_PreviewError = false;
	bool m_DownloadError = false;
	bool m_StoragePrepared = false;
};

#endif // GAME_CLIENT_COMPONENTS_SKINSHOP_H
