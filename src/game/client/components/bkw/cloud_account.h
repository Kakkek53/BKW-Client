#ifndef GAME_CLIENT_COMPONENTS_BKW_CLOUD_ACCOUNT_H
#define GAME_CLIENT_COMPONENTS_BKW_CLOUD_ACCOUNT_H

#include <base/fs.h>
#include <base/hash.h>
#include <base/secure.h>
#include <base/str.h>
#include <base/time.h>

#if defined(CONF_FAMILY_WINDOWS)
#include <base/windows.h>
#endif


#include <engine/client.h>
#include <engine/http.h>
#include <engine/external/json-parser/json.h>
#include <engine/shared/config.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Bkw
{
class CCloudAccount
{
public:
	static constexpr const char *BASE_URL = "https://bkw-client.onrender.com";
	static constexpr size_t MAX_RESPONSE_SIZE = 2 * 1024 * 1024;

	struct SSave
	{
		std::string m_Id;
		std::string m_Name;
		std::string m_UpdatedAt;
	};

	struct SShare
	{
		std::string m_Id;
		std::string m_Url;
		std::string m_CreatedAt;
	};

private:
	enum class ERequest
	{
		NONE,
		DEVICE_START,
		DEVICE_POLL,
		ME,
		LOGOUT,
		SAVE_SETTINGS,
		LOAD_SETTINGS,
		LIST_SAVES,
		CREATE_SAVE,
		LOAD_SAVE,
		UPDATE_SAVE,
		DELETE_SAVE,
		CREATE_SHARE,
		LIST_SHARES,
		DELETE_SHARE,
		IMPORT_SHARE,
	};

	std::shared_ptr<IHttpRequest> m_pRequest;
	ERequest m_RequestType = ERequest::NONE;
	std::string m_DeviceCode;
	std::string m_UserCode;
	std::string m_VerificationUrl;
	std::string m_CodeVerifier;
	std::string m_UserName;
	std::string m_GlobalName;
	std::string m_LastShareUrl;
	std::string m_PendingId;
	std::vector<SSave> m_vSaves;
	std::vector<SShare> m_vShares;
	std::string m_Status = "BKW Cloud не подключён";
	bool m_LoginWaiting = false;
	bool m_ProfileLoaded = false;
	bool m_ProfileRequestStarted = false;
	bool m_ImportedThisSession = false;
	int m_HttpStatus = 0;
	int64_t m_NextDevicePoll = 0;

	static bool IsSharedKey(const char *pKey, int Flags)
	{
		if((Flags & CFGFLAG_SAVE) == 0 || (Flags & CFGFLAG_CLIENT) == 0)
			return false;
		if(str_comp(pKey, "bkw_cloud_token") == 0 || str_comp(pKey, "bkw_pending_deep_link") == 0 || str_startswith(pKey, "bkw_tipo_cheat"))
			return false;
		// Never upload credentials even if another client module accidentally marks them as saved settings.
		if(str_find(pKey, "password") || str_find(pKey, "token") || str_find(pKey, "secret") || str_find(pKey, "api_key") || str_find(pKey, "apikey"))
			return false;
		return true;
	}

	static bool IsValidSessionToken(const char *pToken)
	{
		if(!pToken)
			return false;
		const size_t Length = str_length(pToken);
		if(Length < 32 || Length >= sizeof(g_Config.m_BkwCloudToken))
			return false;
		for(const unsigned char *p = reinterpret_cast<const unsigned char *>(pToken); *p; ++p)
		{
			if(!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_' || *p == '-'))
				return false;
		}
		return true;
	}

	static std::string JsonEscape(const char *pText)
	{
		std::string Result;
		if(!pText)
			return Result;
		for(const unsigned char *p = reinterpret_cast<const unsigned char *>(pText); *p; ++p)
		{
			switch(*p)
			{
			case '"': Result += "\\\""; break;
			case '\\': Result += "\\\\"; break;
			case '\b': Result += "\\b"; break;
			case '\f': Result += "\\f"; break;
			case '\n': Result += "\\n"; break;
			case '\r': Result += "\\r"; break;
			case '\t': Result += "\\t"; break;
			default:
				if(*p < 0x20)
				{
					char aBuf[8];
					str_format(aBuf, sizeof(aBuf), "\\u%04x", (int)*p);
					Result += aBuf;
				}
				else
					Result.push_back((char)*p);
				break;
			}
		}
		return Result;
	}

	static std::string Base64Url(const unsigned char *pData, size_t Size)
	{
		static constexpr char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
		std::string Result;
		Result.reserve((Size * 4 + 2) / 3);
		for(size_t i = 0; i < Size; i += 3)
		{
			const uint32_t A = pData[i];
			const uint32_t B = i + 1 < Size ? pData[i + 1] : 0;
			const uint32_t C = i + 2 < Size ? pData[i + 2] : 0;
			const uint32_t Triple = (A << 16) | (B << 8) | C;
			Result.push_back(ALPHABET[(Triple >> 18) & 0x3f]);
			Result.push_back(ALPHABET[(Triple >> 12) & 0x3f]);
			if(i + 1 < Size)
				Result.push_back(ALPHABET[(Triple >> 6) & 0x3f]);
			if(i + 2 < Size)
				Result.push_back(ALPHABET[Triple & 0x3f]);
		}
		return Result;
	}

	static std::string PkceChallenge(const std::string &Verifier)
	{
		const SHA256_DIGEST Digest = sha256(Verifier.data(), Verifier.size());
		return Base64Url(Digest.data, sizeof(Digest.data));
	}

	static const char *JsonString(const json_value &Value)
	{
		return Value.type == json_string && Value.u.string.ptr ? Value.u.string.ptr : "";
	}

	static std::string BuildSettingsSnapshot()
	{
		std::string Result = "{\"schema\":2,\"client\":\"BKW\",\"settings\":{";
		bool First = true;
		auto AddKey = [&](const char *pKey) {
			if(!First)
				Result += ',';
			First = false;
			Result += '\"';
			Result += pKey;
			Result += "\":";
		};

#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName, Flags)) \
			{ \
				AddKey(#ScriptName); \
				Result += std::to_string(g_Config.m_##Name); \
			} \
		} while(false);
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName, Flags)) \
			{ \
				AddKey(#ScriptName); \
				Result += std::to_string((uint64_t)g_Config.m_##Name); \
			} \
		} while(false);
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName, Flags)) \
			{ \
				AddKey(#ScriptName); \
				Result += '\"'; \
				Result += JsonEscape(g_Config.m_##Name); \
				Result += '\"'; \
			} \
		} while(false);
#define SET_CONFIG_DOMAIN(CONFIGDOMAIN)
#include <engine/shared/config_includes.h>
#undef SET_CONFIG_DOMAIN
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_COL
#undef MACRO_CONFIG_STR

		Result += "}}";
		return Result;
	}

	static bool ApplySettingsSnapshot(const json_value &Snapshot)
	{
		if(Snapshot.type != json_object)
			return false;
		const json_value &Settings = Snapshot["settings"];
		if(Settings.type != json_object)
			return false;

#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName, Flags)) \
			{ \
				const json_value &Value = Settings[#ScriptName]; \
				if(Value.type == json_integer) \
					g_Config.m_##Name = std::clamp((int)Value.u.integer, Min, Max); \
			} \
		} while(false);
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName, Flags)) \
			{ \
				const json_value &Value = Settings[#ScriptName]; \
				if(Value.type == json_integer) \
					g_Config.m_##Name = (unsigned)Value.u.integer; \
			} \
		} while(false);
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName, Flags)) \
			{ \
				const json_value &Value = Settings[#ScriptName]; \
				if(Value.type == json_string && Value.u.string.ptr) \
					str_copy(g_Config.m_##Name, Value.u.string.ptr); \
			} \
		} while(false);
#define SET_CONFIG_DOMAIN(CONFIGDOMAIN)
#include <engine/shared/config_includes.h>
#undef SET_CONFIG_DOMAIN
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_COL
#undef MACRO_CONFIG_STR
		return true;
	}

	void ParseSaves(const json_value &Root)
	{
		m_vSaves.clear();
		const json_value &Items = Root["saves"];
		if(Items.type != json_array)
			return;
		for(unsigned i = 0; i < Items.u.array.length; ++i)
		{
			const json_value &Item = Items[i];
			if(Item.type != json_object)
				continue;
			SSave Save;
			Save.m_Id = JsonString(Item["id"]);
			Save.m_Name = JsonString(Item["name"]);
			Save.m_UpdatedAt = JsonString(Item["updated_at"]);
			if(!Save.m_Id.empty())
				m_vSaves.push_back(std::move(Save));
		}
	}

	void ParseShares(const json_value &Root)
	{
		m_vShares.clear();
		const json_value &Items = Root["shares"];
		if(Items.type != json_array)
			return;
		for(unsigned i = 0; i < Items.u.array.length; ++i)
		{
			const json_value &Item = Items[i];
			if(Item.type != json_object)
				continue;
			SShare Share;
			Share.m_Id = JsonString(Item["share_id"]);
			Share.m_Url = JsonString(Item["url"]);
			Share.m_CreatedAt = JsonString(Item["created_at"]);
			if(!Share.m_Id.empty() && !Share.m_Url.empty())
				m_vShares.push_back(std::move(Share));
		}
	}

	void HandlePendingDeepLink(IHttp *pHttp)
	{
		if(m_pRequest || g_Config.m_BkwPendingDeepLink[0] == '\0')
			return;
		std::string Link = g_Config.m_BkwPendingDeepLink;
		g_Config.m_BkwPendingDeepLink[0] = '\0';
		const std::string SharePrefix = "bkw://share/";
		if(Link.rfind(SharePrefix, 0) == 0)
		{
			const std::string Id = Link.substr(SharePrefix.size());
			const std::string Https = std::string(BASE_URL) + "/share/" + Id;
			ImportShare(pHttp, Https.c_str());
		}
		else if(Link == "bkw://auth/complete" || Link == "bkw:auth/complete")
		{
			m_NextDevicePoll = 0;
			m_Status = "Браузер подтвердил вход — завершаю подключение…";
		}
	}

	void ConfigureRequest(const std::shared_ptr<IHttpRequest> &pRequest, bool Authenticated)
	{
		pRequest->Timeout(CTimeout{8000, 20000, 1024, 8});
		pRequest->MaxResponseSize(MAX_RESPONSE_SIZE);
		pRequest->FailOnErrorStatus(false);
		pRequest->LogProgress(HTTPLOG::NONE);
		if(Authenticated && IsValidSessionToken(g_Config.m_BkwCloudToken))
		{
			std::string Authorization = "Bearer ";
			Authorization += g_Config.m_BkwCloudToken;
			pRequest->HeaderString("Authorization", Authorization.c_str());
		}
	}

	void Run(IHttp *pHttp, const std::shared_ptr<IHttpRequest> &pRequest, ERequest Type, bool Authenticated = false)
	{
		if(!pHttp || !pRequest)
			return;
		if(m_pRequest)
			m_pRequest->Abort();
		ConfigureRequest(pRequest, Authenticated);
		m_pRequest = pRequest;
		m_RequestType = Type;
		m_HttpStatus = 0;
		pHttp->Run(pRequest);
	}

	void LoadUser(const json_value &Root)
	{
		const json_value &User = Root["user"];
		if(User.type != json_object)
			return;
		m_UserName = JsonString(User["username"]);
		m_GlobalName = JsonString(User["global_name"]);
		m_ProfileLoaded = !m_UserName.empty();
		m_ProfileRequestStarted = false;
	}

	void StartDevicePoll(IHttp *pHttp)
	{
		if(!pHttp || m_DeviceCode.empty() || m_CodeVerifier.empty())
			return;
		std::string Body = "{\"device_code\":\"";
		Body += JsonEscape(m_DeviceCode.c_str());
		Body += "\",\"code_verifier\":\"";
		Body += JsonEscape(m_CodeVerifier.c_str());
		Body += "\"}";
		std::shared_ptr<IHttpRequest> pPost = HttpPostJson("https://bkw-client.onrender.com/api/auth/device/exchange", Body.c_str());
		Run(pHttp, pPost, ERequest::DEVICE_POLL, false);
	}

	void HandleCompleted(IClient *pClient)
	{
		if(!m_pRequest)
			return;
		const ERequest CompletedType = m_RequestType;
		const bool Done = m_pRequest->State() == EHttpState::DONE;
		m_HttpStatus = Done ? m_pRequest->StatusCode() : -1;

		unsigned char *pResult = nullptr;
		size_t ResultSize = 0;
		if(Done)
			m_pRequest->Result(&pResult, &ResultSize);

		json_value *pRoot = nullptr;
		if(pResult && ResultSize > 0)
			pRoot = json_parse(reinterpret_cast<const char *>(pResult), ResultSize);

		m_pRequest = nullptr;
		m_RequestType = ERequest::NONE;

		if(!Done || m_HttpStatus < 200 || m_HttpStatus >= 300 || !pRoot || pRoot->type != json_object)
		{
			if(m_HttpStatus == 401)
			{
				g_Config.m_BkwCloudToken[0] = '\0';
				m_ProfileLoaded = false;
				m_ProfileRequestStarted = false;
				m_Status = "Сессия BKW Cloud истекла — войдите снова";
			}
			else if(CompletedType == ERequest::DEVICE_POLL && m_LoginWaiting && m_HttpStatus == 404)
			{
				m_LoginWaiting = false;
				m_CodeVerifier.clear();
				m_Status = "Код входа истёк — начните вход заново";
			}
			else if(CompletedType == ERequest::DEVICE_POLL && m_LoginWaiting && m_HttpStatus == 403)
			{
				m_LoginWaiting = false;
				m_CodeVerifier.clear();
				m_Status = "PKCE-проверка входа не пройдена";
			}
			else
			{
				char aBuf[160];
				str_format(aBuf, sizeof(aBuf), "Ошибка BKW Cloud (HTTP %d)", m_HttpStatus);
				m_Status = aBuf;
			}
			if(pRoot)
				json_value_free(pRoot);
			return;
		}

		switch(CompletedType)
		{
		case ERequest::DEVICE_START:
		{
			m_DeviceCode = JsonString((*pRoot)["device_code"]);
			m_UserCode = JsonString((*pRoot)["user_code"]);
			m_VerificationUrl = JsonString((*pRoot)["verification_uri"]);
			if(m_DeviceCode.empty() || m_VerificationUrl.empty())
			{
				m_Status = "BKW Cloud вернул неполный код входа";
				break;
			}
			m_LoginWaiting = true;
			m_NextDevicePoll = time_get() + time_freq() * 2;
			m_Status = "Подтвердите вход Discord в браузере";
			if(pClient && str_startswith(m_VerificationUrl.c_str(), BASE_URL))
				pClient->ViewLink(m_VerificationUrl.c_str());
			break;
		}
		case ERequest::DEVICE_POLL:
		{
			const char *pStatus = JsonString((*pRoot)["status"]);
			if(str_comp(pStatus, "approved") == 0)
			{
				const char *pToken = JsonString((*pRoot)["token"]);
				if(IsValidSessionToken(pToken))
				{
					str_copy(g_Config.m_BkwCloudToken, pToken);
					m_LoginWaiting = false;
					m_ProfileRequestStarted = false;
					m_CodeVerifier.clear();
					m_DeviceCode.clear();
					LoadUser(*pRoot);
					m_Status = "Discord подключён к BKW Client через PKCE";
				}
				else
				{
					m_LoginWaiting = false;
					m_CodeVerifier.clear();
					m_DeviceCode.clear();
					m_Status = "BKW Cloud вернул некорректный session token";
				}
			}
			else
			{
				m_NextDevicePoll = time_get() + time_freq() * 2;
				m_Status = "Ожидаю подтверждение Discord…";
			}
			break;
		}
		case ERequest::ME:
			LoadUser(*pRoot);
			if(m_ProfileLoaded)
				m_Status = "BKW Cloud подключён";
			break;
		case ERequest::LOGOUT:
			m_Status = "Вы вышли из BKW Cloud";
			break;
		case ERequest::SAVE_SETTINGS:
			m_Status = "Основное сохранение обновлено";
			break;
		case ERequest::LOAD_SETTINGS:
		case ERequest::LOAD_SAVE:
		{
			const json_value &Snapshot = (*pRoot)["settings"];
			if(ApplySettingsSnapshot(Snapshot))
				m_Status = "Настройки всего клиента применены";
			else
				m_Status = "Сохранение не содержит совместимых настроек";
			break;
		}
		case ERequest::LIST_SAVES:
			ParseSaves(*pRoot);
			m_Status = "Список сохранений обновлён";
			break;
		case ERequest::CREATE_SAVE:
		{
			const json_value &Save = (*pRoot)["save"];
			if(Save.type == json_object)
			{
				SSave Item;
				Item.m_Id = JsonString(Save["id"]);
				Item.m_Name = JsonString(Save["name"]);
				Item.m_UpdatedAt = JsonString(Save["updated_at"]);
				if(!Item.m_Id.empty())
					m_vSaves.insert(m_vSaves.begin(), std::move(Item));
			}
			m_Status = "Новое сохранение создано";
			break;
		}
		case ERequest::UPDATE_SAVE:
			m_Status = "Сохранение перезаписано";
			break;
		case ERequest::DELETE_SAVE:
			m_vSaves.erase(std::remove_if(m_vSaves.begin(), m_vSaves.end(), [&](const SSave &Save) { return Save.m_Id == m_PendingId; }), m_vSaves.end());
			m_Status = "Сохранение удалено";
			m_PendingId.clear();
			break;
		case ERequest::CREATE_SHARE:
		{
			m_LastShareUrl = JsonString((*pRoot)["url"]);
			const char *pId = JsonString((*pRoot)["share_id"]);
			if(pId[0] && !m_LastShareUrl.empty())
				m_vShares.insert(m_vShares.begin(), SShare{pId, m_LastShareUrl, ""});
			m_Status = m_LastShareUrl.empty() ? "Не удалось получить ссылку" : "HTTPS-ссылка создана";
			break;
		}
		case ERequest::LIST_SHARES:
			ParseShares(*pRoot);
			m_Status = "Список HTTPS-ссылок обновлён";
			break;
		case ERequest::DELETE_SHARE:
			m_vShares.erase(std::remove_if(m_vShares.begin(), m_vShares.end(), [&](const SShare &Share) { return Share.m_Id == m_PendingId; }), m_vShares.end());
			m_Status = "HTTPS-ссылка удалена";
			m_PendingId.clear();
			break;
		case ERequest::IMPORT_SHARE:
		{
			const json_value &Snapshot = (*pRoot)["settings"];
			m_ImportedThisSession = ApplySettingsSnapshot(Snapshot);
			m_Status = m_ImportedThisSession ? "Настройки из ссылки применены" : "Ссылка не содержит совместимых BKW настроек";
			break;
		}
		case ERequest::NONE:
			break;
		}
		json_value_free(pRoot);
	}

public:
	void Poll(IHttp *pHttp, IClient *pClient)
	{
		if(m_pRequest && m_pRequest->Done())
			HandleCompleted(pClient);
		HandlePendingDeepLink(pHttp);

		if(!m_pRequest && m_LoginWaiting && !m_DeviceCode.empty() && !m_CodeVerifier.empty() && time_get() >= m_NextDevicePoll)
			StartDevicePoll(pHttp);

		if(!m_pRequest && !m_LoginWaiting && IsValidSessionToken(g_Config.m_BkwCloudToken) && !m_ProfileLoaded && !m_ProfileRequestStarted)
			RequestMe(pHttp);
	}

	void StartLogin(IHttp *pHttp)
	{
		if(!pHttp || m_pRequest)
			return;
		m_LoginWaiting = false;
		m_DeviceCode.clear();
		m_UserCode.clear();
		m_VerificationUrl.clear();
		m_CodeVerifier.clear();

		char aVerifier[65];
		secure_random_password(aVerifier, sizeof(aVerifier), 64);
		m_CodeVerifier = aVerifier;
		const std::string Challenge = PkceChallenge(m_CodeVerifier);
		if(Challenge.size() != 43)
		{
			m_CodeVerifier.clear();
			m_Status = "Не удалось создать PKCE challenge";
			return;
		}

		m_Status = "Запрашиваю защищённый вход Discord…";
		std::string Body = "{\"code_challenge\":\"";
		Body += Challenge;
		Body += "\"}";
		std::shared_ptr<IHttpRequest> pPost = HttpPostJson("https://bkw-client.onrender.com/api/auth/device/start", Body.c_str());
		Run(pHttp, pPost, ERequest::DEVICE_START, false);
	}

	void RequestMe(IHttp *pHttp)
	{
		if(!pHttp || m_pRequest || !IsValidSessionToken(g_Config.m_BkwCloudToken))
			return;
		m_ProfileRequestStarted = true;
		std::shared_ptr<IHttpRequest> pGet = HttpGet("https://bkw-client.onrender.com/api/me");
		Run(pHttp, pGet, ERequest::ME, true);
	}

	void Logout(IHttp *pHttp)
	{
		if(g_Config.m_BkwCloudToken[0] == '\0')
			return;
		if(pHttp && !m_pRequest && IsValidSessionToken(g_Config.m_BkwCloudToken))
		{
			std::shared_ptr<IHttpRequest> pPost = HttpPostJson("https://bkw-client.onrender.com/api/logout", "{}");
			Run(pHttp, pPost, ERequest::LOGOUT, true);
		}
		g_Config.m_BkwCloudToken[0] = '\0';
		m_ProfileLoaded = false;
		m_ProfileRequestStarted = false;
		m_UserName.clear();
		m_GlobalName.clear();
		m_LoginWaiting = false;
		m_CodeVerifier.clear();
		m_DeviceCode.clear();
		m_Status = "Вы вышли из BKW Cloud";
	}

	void SaveSettings(IHttp *pHttp)
	{
		if(!pHttp || m_pRequest || !LoggedIn())
			return;
		std::string Body = "{\"settings\":";
		Body += BuildSettingsSnapshot();
		Body += '}';
		std::shared_ptr<IHttpRequest> pPost = HttpPostJson("https://bkw-client.onrender.com/api/settings", Body.c_str());
		Run(pHttp, pPost, ERequest::SAVE_SETTINGS, true);
		m_Status = "Сохраняю настройки всего клиента…";
	}

	void LoadSettings(IHttp *pHttp)
	{
		if(!pHttp || m_pRequest || !LoggedIn())
			return;
		std::shared_ptr<IHttpRequest> pGet = HttpGet("https://bkw-client.onrender.com/api/settings");
		Run(pHttp, pGet, ERequest::LOAD_SETTINGS, true);
		m_Status = "Загружаю основное сохранение…";
	}

	void ListSaves(IHttp *pHttp)
	{
		if(!pHttp || m_pRequest || !LoggedIn())
			return;
		Run(pHttp, HttpGet("https://bkw-client.onrender.com/api/saves"), ERequest::LIST_SAVES, true);
		m_Status = "Получаю список сохранений…";
	}

	void CreateSave(IHttp *pHttp)
	{
		if(!pHttp || m_pRequest || !LoggedIn())
			return;
		char aName[64];
		str_format(aName, sizeof(aName), "Сохранение %d", (int)m_vSaves.size() + 1);
		std::string Body = "{\"name\":\"";
		Body += JsonEscape(aName);
		Body += "\",\"settings\":";
		Body += BuildSettingsSnapshot();
		Body += '}';
		Run(pHttp, HttpPostJson("https://bkw-client.onrender.com/api/saves", Body.c_str()), ERequest::CREATE_SAVE, true);
		m_Status = "Создаю отдельное сохранение…";
	}

	void LoadSave(IHttp *pHttp, const char *pId)
	{
		if(!pHttp || m_pRequest || !LoggedIn() || !pId || !pId[0])
			return;
		std::string Url = std::string(BASE_URL) + "/api/saves/" + pId;
		Run(pHttp, HttpGet(Url.c_str()), ERequest::LOAD_SAVE, true);
		m_Status = "Загружаю выбранное сохранение…";
	}

	void UpdateSave(IHttp *pHttp, const char *pId)
	{
		if(!pHttp || m_pRequest || !LoggedIn() || !pId || !pId[0])
			return;
		std::string Url = std::string(BASE_URL) + "/api/saves/" + pId;
		std::string Body = "{\"settings\":";
		Body += BuildSettingsSnapshot();
		Body += '}';
		Run(pHttp, HttpPostJson(Url.c_str(), Body.c_str()), ERequest::UPDATE_SAVE, true);
		m_Status = "Перезаписываю сохранение…";
	}

	void DeleteSave(IHttp *pHttp, const char *pId)
	{
		if(!pHttp || m_pRequest || !LoggedIn() || !pId || !pId[0])
			return;
		m_PendingId = pId;
		std::string Url = std::string(BASE_URL) + "/api/saves/" + pId + "/delete";
		Run(pHttp, HttpPostJson(Url.c_str(), "{}"), ERequest::DELETE_SAVE, true);
		m_Status = "Удаляю сохранение…";
	}

	void ListShares(IHttp *pHttp)
	{
		if(!pHttp || m_pRequest || !LoggedIn())
			return;
		Run(pHttp, HttpGet("https://bkw-client.onrender.com/api/shares"), ERequest::LIST_SHARES, true);
		m_Status = "Получаю мои HTTPS-ссылки…";
	}

	void DeleteShare(IHttp *pHttp, const char *pId)
	{
		if(!pHttp || m_pRequest || !LoggedIn() || !pId || !pId[0])
			return;
		m_PendingId = pId;
		std::string Url = std::string(BASE_URL) + "/api/share/" + pId + "/delete";
		Run(pHttp, HttpPostJson(Url.c_str(), "{}"), ERequest::DELETE_SHARE, true);
		m_Status = "Удаляю HTTPS-ссылку…";
	}

	bool RegisterBkwProtocol()
	{
#if defined(CONF_FAMILY_WINDOWS)
		char aPath[IO_MAX_PATH_LENGTH];
		if(fs_executable_path(aPath, sizeof(aPath)) != 0)
		{
			m_Status = "Не удалось определить путь к BKW Client";
			return false;
		}
		bool Updated = false;
		if(!windows_shell_register_protocol("bkw", aPath, &Updated))
		{
			m_Status = "Windows не разрешила зарегистрировать bkw://";
			return false;
		}
		if(Updated)
			windows_shell_update();
		m_Status = "Протокол bkw:// включён по вашему запросу";
		return true;
#else
		m_Status = "Автоматическая регистрация bkw:// сейчас доступна только на Windows";
		return false;
#endif
	}

	void CreateShare(IHttp *pHttp)
	{
		if(!pHttp || m_pRequest || !LoggedIn())
			return;
		std::string Body = "{\"settings\":";
		Body += BuildSettingsSnapshot();
		Body += '}';
		std::shared_ptr<IHttpRequest> pPost = HttpPostJson("https://bkw-client.onrender.com/api/share", Body.c_str());
		Run(pHttp, pPost, ERequest::CREATE_SHARE, true);
		m_Status = "Создаю HTTPS-ссылку…";
	}

	bool ImportShare(IHttp *pHttp, const char *pText)
	{
		if(!pHttp || m_pRequest || !pText)
			return false;
		std::string Text = pText;
		while(!Text.empty() && (Text.back() == '\r' || Text.back() == '\n' || Text.back() == ' ' || Text.back() == '\t'))
			Text.pop_back();
		while(!Text.empty() && (Text.front() == ' ' || Text.front() == '\t'))
			Text.erase(Text.begin());

		const std::string Prefix1 = std::string(BASE_URL) + "/share/";
		const std::string Prefix2 = std::string(BASE_URL) + "/api/share/";
		std::string Id;
		if(Text.rfind(Prefix1, 0) == 0)
			Id = Text.substr(Prefix1.size());
		else if(Text.rfind(Prefix2, 0) == 0)
			Id = Text.substr(Prefix2.size());
		else
		{
			m_Status = "В буфере нет ссылки bkw-client.onrender.com/share/...";
			return false;
		}
		if(Id.empty() || Id.size() > 64 || Id.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") != std::string::npos)
		{
			m_Status = "Некорректный ID BKW share-ссылки";
			return false;
		}

		std::string Url = std::string(BASE_URL) + "/api/share/" + Id;
		std::shared_ptr<IHttpRequest> pGet = HttpGet(Url.c_str());
		Run(pHttp, pGet, ERequest::IMPORT_SHARE, false);
		m_Status = "Загружаю настройки по ссылке…";
		return true;
	}

	bool OpenVerification(IClient *pClient) const
	{
		return pClient && !m_VerificationUrl.empty() && str_startswith(m_VerificationUrl.c_str(), BASE_URL) && pClient->ViewLink(m_VerificationUrl.c_str());
	}

	bool OpenShare(IClient *pClient) const
	{
		return pClient && !m_LastShareUrl.empty() && str_startswith(m_LastShareUrl.c_str(), BASE_URL) && pClient->ViewLink(m_LastShareUrl.c_str());
	}

	bool Busy() const { return m_pRequest != nullptr; }
	bool LoginWaiting() const { return m_LoginWaiting; }
	bool LoggedIn() const { return IsValidSessionToken(g_Config.m_BkwCloudToken); }
	bool ProfileLoaded() const { return m_ProfileLoaded; }
	const char *UserName() const { return m_UserName.c_str(); }
	const char *GlobalName() const { return m_GlobalName.empty() ? m_UserName.c_str() : m_GlobalName.c_str(); }
	const char *Status() const { return m_Status.c_str(); }
	const char *UserCode() const { return m_UserCode.c_str(); }
	const char *VerificationUrl() const { return m_VerificationUrl.c_str(); }
	const char *LastShareUrl() const { return m_LastShareUrl.c_str(); }
	const std::vector<SSave> &Saves() const { return m_vSaves; }
	const std::vector<SShare> &Shares() const { return m_vShares; }
	int HttpStatus() const { return m_HttpStatus; }
	bool ImportedThisSession() const { return m_ImportedThisSession; }
};

inline CCloudAccount &CloudAccountState()
{
	static CCloudAccount s_State;
	return s_State;
}
} // namespace Bkw

#endif // GAME_CLIENT_COMPONENTS_BKW_CLOUD_ACCOUNT_H