#ifndef GAME_CLIENT_COMPONENTS_BKW_CLOUD_ACCOUNT_H
#define GAME_CLIENT_COMPONENTS_BKW_CLOUD_ACCOUNT_H

#include <base/hash.h>
#include <base/secure.h>
#include <base/str.h>
#include <base/time.h>

#if defined(CONF_FAMILY_WINDOWS)
#include <base/windows.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <engine/client.h>
#include <engine/http.h>
#include <engine/external/json-parser/json.h>
#include <engine/shared/config.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

namespace Bkw
{
class CCloudAccount
{
public:
	static constexpr const char *BASE_URL = "https://bkw-client.onrender.com";
	static constexpr size_t MAX_RESPONSE_SIZE = 2 * 1024 * 1024;

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
		CREATE_SHARE,
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
	std::string m_Status = "BKW Cloud не подключён";
	bool m_LoginWaiting = false;
	bool m_ProfileLoaded = false;
	bool m_ProfileRequestStarted = false;
	bool m_ImportedThisSession = false;
	int m_HttpStatus = 0;
	int64_t m_NextDevicePoll = 0;

	static bool IsSharedKey(const char *pKey)
	{
		return str_comp(pKey, "bkw_cloud_token") != 0 && !str_startswith(pKey, "bkw_tipo_cheat");
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

	static bool EnsureDeepLinkProtocol()
	{
#if defined(CONF_FAMILY_WINDOWS)
		wchar_t aWidePath[32768];
		const DWORD Length = GetModuleFileNameW(nullptr, aWidePath, (DWORD)(sizeof(aWidePath) / sizeof(aWidePath[0])));
		if(Length == 0 || Length >= sizeof(aWidePath) / sizeof(aWidePath[0]))
			return false;
		aWidePath[Length] = L'\0';
		const std::optional<std::string> Path = windows_wide_to_utf8(aWidePath);
		if(!Path || Path->empty())
			return false;
		bool Updated = false;
		if(!windows_shell_register_protocol("bkw-discord", Path->c_str(), &Updated))
			return false;
		if(Updated)
			windows_shell_update();
		return true;
#else
		return false;
#endif
	}

	static std::string BuildSettingsSnapshot()
	{
		std::string Result = "{\"schema\":1,\"client\":\"BKW\",\"settings\":{";
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
			if(IsSharedKey(#ScriptName)) \
			{ \
				AddKey(#ScriptName); \
				Result += std::to_string(g_Config.m_##Name); \
			} \
		} while(false);
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName)) \
			{ \
				AddKey(#ScriptName); \
				Result += std::to_string((uint64_t)g_Config.m_##Name); \
			} \
		} while(false);
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName)) \
			{ \
				AddKey(#ScriptName); \
				Result += '\"'; \
				Result += JsonEscape(g_Config.m_##Name); \
				Result += '\"'; \
			} \
		} while(false);
#include <engine/shared/config_variables_bkw.inc>
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
			if(IsSharedKey(#ScriptName)) \
			{ \
				const json_value &Value = Settings[#ScriptName]; \
				if(Value.type == json_integer) \
					g_Config.m_##Name = std::clamp((int)Value.u.integer, Min, Max); \
			} \
		} while(false);
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName)) \
			{ \
				const json_value &Value = Settings[#ScriptName]; \
				if(Value.type == json_integer) \
					g_Config.m_##Name = (unsigned)Value.u.integer; \
			} \
		} while(false);
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Flags, Desc) \
		do \
		{ \
			if(IsSharedKey(#ScriptName)) \
			{ \
				const json_value &Value = Settings[#ScriptName]; \
				if(Value.type == json_string && Value.u.string.ptr) \
					str_copy(g_Config.m_##Name, Value.u.string.ptr); \
			} \
		} while(false);
#include <engine/shared/config_variables_bkw.inc>
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_COL
#undef MACRO_CONFIG_STR
		return true;
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
			m_Status = "BKW настройки сохранены в облако";
			break;
		case ERequest::LOAD_SETTINGS:
		{
			const json_value &Snapshot = (*pRoot)["settings"];
			if(ApplySettingsSnapshot(Snapshot))
				m_Status = "Облачные BKW настройки применены";
			else
				m_Status = "В облаке пока нет совместимых настроек";
			break;
		}
		case ERequest::CREATE_SHARE:
			m_LastShareUrl = JsonString((*pRoot)["url"]);
			m_Status = m_LastShareUrl.empty() ? "Не удалось получить ссылку" : "HTTPS-ссылка создана и готова к копированию";
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

		const bool DeepLinkReady = EnsureDeepLinkProtocol();
		m_Status = DeepLinkReady ? "Запрашиваю защищённый вход Discord…" : "Запрашиваю вход Discord… (deep-link не зарегистрирован)";
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
		m_Status = "Сохраняю BKW настройки…";
	}

	void LoadSettings(IHttp *pHttp)
	{
		if(!pHttp || m_pRequest || !LoggedIn())
			return;
		std::shared_ptr<IHttpRequest> pGet = HttpGet("https://bkw-client.onrender.com/api/settings");
		Run(pHttp, pGet, ERequest::LOAD_SETTINGS, true);
		m_Status = "Загружаю BKW настройки…";
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
