from pathlib import Path
import re

# 1) Runtime-only deep-link handoff config.
p = Path('src/engine/shared/config_variables_bkw.inc')
t = p.read_text(encoding='utf-8')
needle = 'MACRO_CONFIG_STR(BkwCloudToken, bkw_cloud_token, 192, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "BKW Cloud session token")\n'
if 'BkwPendingDeepLink' not in t:
    if needle not in t:
        raise RuntimeError('cloud token config marker not found')
    t = t.replace(needle, needle + 'MACRO_CONFIG_STR(BkwPendingDeepLink, bkw_pending_deep_link, 256, "", CFGFLAG_CLIENT, "Runtime-only BKW deep link pending for the cloud component")\n', 1)
p.write_text(t, encoding='utf-8')

# 2) Recognize bkw:// command-line arguments without treating them as console errors.
p = Path('src/engine/client/client.cpp')
t = p.read_text(encoding='utf-8')
old = '''static bool UnknownArgumentCallback(const char *pCommand, void *pUser)
{
\tCClient *pClient = static_cast<CClient *>(pUser);
\tif(str_startswith(pCommand, CONNECTLINK_NO_SLASH))
'''
new = '''static bool UnknownArgumentCallback(const char *pCommand, void *pUser)
{
\tCClient *pClient = static_cast<CClient *>(pUser);
\tif(str_startswith(pCommand, "bkw:"))
\t{
\t\tstr_copy(g_Config.m_BkwPendingDeepLink, pCommand);
\t\treturn true;
\t}
\tif(str_startswith(pCommand, CONNECTLINK_NO_SLASH))
'''
if old not in t:
    raise RuntimeError('UnknownArgumentCallback marker not found')
t = t.replace(old, new, 1)
p.write_text(t, encoding='utf-8')

# 3) Upgrade cloud account implementation.
p = Path('src/game/client/components/bkw/cloud_account.h')
t = p.read_text(encoding='utf-8')

t = t.replace('#include <base/hash.h>\n', '#include <base/fs.h>\n#include <base/hash.h>\n', 1)
if '#include <base/windows.h>' not in t:
    t = t.replace('#include <base/time.h>\n', '#include <base/time.h>\n\n#if defined(CONF_FAMILY_WINDOWS)\n#include <base/windows.h>\n#endif\n', 1)
t = t.replace('#include <string>\n', '#include <string>\n#include <vector>\n', 1)

t = t.replace('''public:
\tstatic constexpr const char *BASE_URL = "https://bkw-client.onrender.com";
\tstatic constexpr size_t MAX_RESPONSE_SIZE = 2 * 1024 * 1024;

private:
''', '''public:
\tstatic constexpr const char *BASE_URL = "https://bkw-client.onrender.com";
\tstatic constexpr size_t MAX_RESPONSE_SIZE = 2 * 1024 * 1024;

\tstruct SSave
\t{
\t\tstd::string m_Id;
\t\tstd::string m_Name;
\t\tstd::string m_UpdatedAt;
\t};

\tstruct SShare
\t{
\t\tstd::string m_Id;
\t\tstd::string m_Url;
\t\tstd::string m_CreatedAt;
\t};

private:
''', 1)

t = t.replace('''\t\tSAVE_SETTINGS,
\t\tLOAD_SETTINGS,
\t\tCREATE_SHARE,
\t\tIMPORT_SHARE,
''', '''\t\tSAVE_SETTINGS,
\t\tLOAD_SETTINGS,
\t\tLIST_SAVES,
\t\tCREATE_SAVE,
\t\tLOAD_SAVE,
\t\tUPDATE_SAVE,
\t\tDELETE_SAVE,
\t\tCREATE_SHARE,
\t\tLIST_SHARES,
\t\tDELETE_SHARE,
\t\tIMPORT_SHARE,
''', 1)

t = t.replace('''\tstd::string m_LastShareUrl;
\tstd::string m_Status = "BKW Cloud не подключён";
''', '''\tstd::string m_LastShareUrl;
\tstd::string m_PendingId;
\tstd::vector<SSave> m_vSaves;
\tstd::vector<SShare> m_vShares;
\tstd::string m_Status = "BKW Cloud не подключён";
''', 1)

old_filter = '''\tstatic bool IsSharedKey(const char *pKey)
\t{
\t\treturn str_comp(pKey, "bkw_cloud_token") != 0 && !str_startswith(pKey, "bkw_tipo_cheat");
\t}
'''
new_filter = '''\tstatic bool IsSharedKey(const char *pKey, int Flags)
\t{
\t\tif((Flags & CFGFLAG_SAVE) == 0 || (Flags & CFGFLAG_CLIENT) == 0)
\t\t\treturn false;
\t\tif(str_comp(pKey, "bkw_cloud_token") == 0 || str_comp(pKey, "bkw_pending_deep_link") == 0 || str_startswith(pKey, "bkw_tipo_cheat"))
\t\t\treturn false;
\t\t// Never upload credentials even if another client module accidentally marks them as saved settings.
\t\tif(str_find(pKey, "password") || str_find(pKey, "token") || str_find(pKey, "secret") || str_find(pKey, "api_key") || str_find(pKey, "apikey"))
\t\t\treturn false;
\t\treturn true;
\t}
'''
if old_filter not in t:
    raise RuntimeError('IsSharedKey block not found')
t = t.replace(old_filter, new_filter, 1)

# Schema 2 and all config domains.
t = t.replace('std::string Result = "{\\\"schema\\\":1,\\\"client\\\":\\\"BKW\\\",\\\"settings\\\":{";', 'std::string Result = "{\\\"schema\\\":2,\\\"client\\\":\\\"BKW\\\",\\\"settings\\\":{";', 1)
t = t.replace('if(IsSharedKey(#ScriptName))', 'if(IsSharedKey(#ScriptName, Flags))')
t = t.replace('#include <engine/shared/config_variables_bkw.inc>\n#undef MACRO_CONFIG_INT\n#undef MACRO_CONFIG_COL\n#undef MACRO_CONFIG_STR', '#define SET_CONFIG_DOMAIN(CONFIGDOMAIN)\n#include <engine/shared/config_includes.h>\n#undef SET_CONFIG_DOMAIN\n#undef MACRO_CONFIG_INT\n#undef MACRO_CONFIG_COL\n#undef MACRO_CONFIG_STR')

# Insert helpers before ConfigureRequest.
marker = '\tvoid ConfigureRequest(const std::shared_ptr<IHttpRequest> &pRequest, bool Authenticated)\n'
if marker not in t:
    raise RuntimeError('ConfigureRequest marker not found')
helpers = '''\tvoid ParseSaves(const json_value &Root)
\t{
\t\tm_vSaves.clear();
\t\tconst json_value &Items = Root["saves"];
\t\tif(Items.type != json_array)
\t\t\treturn;
\t\tfor(unsigned i = 0; i < Items.u.array.length; ++i)
\t\t{
\t\t\tconst json_value &Item = Items[i];
\t\t\tif(Item.type != json_object)
\t\t\t\tcontinue;
\t\t\tSSave Save;
\t\t\tSave.m_Id = JsonString(Item["id"]);
\t\t\tSave.m_Name = JsonString(Item["name"]);
\t\t\tSave.m_UpdatedAt = JsonString(Item["updated_at"]);
\t\t\tif(!Save.m_Id.empty())
\t\t\t\tm_vSaves.push_back(std::move(Save));
\t\t}
\t}

\tvoid ParseShares(const json_value &Root)
\t{
\t\tm_vShares.clear();
\t\tconst json_value &Items = Root["shares"];
\t\tif(Items.type != json_array)
\t\t\treturn;
\t\tfor(unsigned i = 0; i < Items.u.array.length; ++i)
\t\t{
\t\t\tconst json_value &Item = Items[i];
\t\t\tif(Item.type != json_object)
\t\t\t\tcontinue;
\t\t\tSShare Share;
\t\t\tShare.m_Id = JsonString(Item["share_id"]);
\t\t\tShare.m_Url = JsonString(Item["url"]);
\t\t\tShare.m_CreatedAt = JsonString(Item["created_at"]);
\t\t\tif(!Share.m_Id.empty() && !Share.m_Url.empty())
\t\t\t\tm_vShares.push_back(std::move(Share));
\t\t}
\t}

\tvoid HandlePendingDeepLink(IHttp *pHttp)
\t{
\t\tif(m_pRequest || g_Config.m_BkwPendingDeepLink[0] == '\\0')
\t\t\treturn;
\t\tstd::string Link = g_Config.m_BkwPendingDeepLink;
\t\tg_Config.m_BkwPendingDeepLink[0] = '\\0';
\t\tconst std::string SharePrefix = "bkw://share/";
\t\tif(Link.rfind(SharePrefix, 0) == 0)
\t\t{
\t\t\tconst std::string Id = Link.substr(SharePrefix.size());
\t\t\tconst std::string Https = std::string(BASE_URL) + "/share/" + Id;
\t\t\tImportShare(pHttp, Https.c_str());
\t\t}
\t\telse if(Link == "bkw://auth/complete" || Link == "bkw:auth/complete")
\t\t{
\t\t\tm_NextDevicePoll = 0;
\t\t\tm_Status = "Браузер подтвердил вход — завершаю подключение…";
\t\t}
\t}

'''
t = t.replace(marker, helpers + marker, 1)

# Extend completion switch.
old = '''\t\tcase ERequest::SAVE_SETTINGS:
\t\t\tm_Status = "BKW настройки сохранены в облако";
\t\t\tbreak;
\t\tcase ERequest::LOAD_SETTINGS:
\t\t{
\t\t\tconst json_value &Snapshot = (*pRoot)["settings"];
\t\t\tif(ApplySettingsSnapshot(Snapshot))
\t\t\t\tm_Status = "Облачные BKW настройки применены";
\t\t\telse
\t\t\t\tm_Status = "В облаке пока нет совместимых настроек";
\t\t\tbreak;
\t\t}
\t\tcase ERequest::CREATE_SHARE:
\t\t\tm_LastShareUrl = JsonString((*pRoot)["url"]);
\t\t\tm_Status = m_LastShareUrl.empty() ? "Не удалось получить ссылку" : "HTTPS-ссылка создана и готова к копированию";
\t\t\tbreak;
'''
new = '''\t\tcase ERequest::SAVE_SETTINGS:
\t\t\tm_Status = "Основное сохранение обновлено";
\t\t\tbreak;
\t\tcase ERequest::LOAD_SETTINGS:
\t\tcase ERequest::LOAD_SAVE:
\t\t{
\t\t\tconst json_value &Snapshot = (*pRoot)["settings"];
\t\t\tif(ApplySettingsSnapshot(Snapshot))
\t\t\t\tm_Status = "Настройки всего клиента применены";
\t\t\telse
\t\t\t\tm_Status = "Сохранение не содержит совместимых настроек";
\t\t\tbreak;
\t\t}
\t\tcase ERequest::LIST_SAVES:
\t\t\tParseSaves(*pRoot);
\t\t\tm_Status = "Список сохранений обновлён";
\t\t\tbreak;
\t\tcase ERequest::CREATE_SAVE:
\t\t{
\t\t\tconst json_value &Save = (*pRoot)["save"];
\t\t\tif(Save.type == json_object)
\t\t\t{
\t\t\t\tSSave Item;
\t\t\t\tItem.m_Id = JsonString(Save["id"]);
\t\t\t\tItem.m_Name = JsonString(Save["name"]);
\t\t\t\tItem.m_UpdatedAt = JsonString(Save["updated_at"]);
\t\t\t\tif(!Item.m_Id.empty())
\t\t\t\t\tm_vSaves.insert(m_vSaves.begin(), std::move(Item));
\t\t\t}
\t\t\tm_Status = "Новое сохранение создано";
\t\t\tbreak;
\t\t}
\t\tcase ERequest::UPDATE_SAVE:
\t\t\tm_Status = "Сохранение перезаписано";
\t\t\tbreak;
\t\tcase ERequest::DELETE_SAVE:
\t\t\tm_vSaves.erase(std::remove_if(m_vSaves.begin(), m_vSaves.end(), [&](const SSave &Save) { return Save.m_Id == m_PendingId; }), m_vSaves.end());
\t\t\tm_Status = "Сохранение удалено";
\t\t\tm_PendingId.clear();
\t\t\tbreak;
\t\tcase ERequest::CREATE_SHARE:
\t\t{
\t\t\tm_LastShareUrl = JsonString((*pRoot)["url"]);
\t\t\tconst char *pId = JsonString((*pRoot)["share_id"]);
\t\t\tif(pId[0] && !m_LastShareUrl.empty())
\t\t\t\tm_vShares.insert(m_vShares.begin(), SShare{pId, m_LastShareUrl, ""});
\t\t\tm_Status = m_LastShareUrl.empty() ? "Не удалось получить ссылку" : "HTTPS-ссылка создана";
\t\t\tbreak;
\t\t}
\t\tcase ERequest::LIST_SHARES:
\t\t\tParseShares(*pRoot);
\t\t\tm_Status = "Список HTTPS-ссылок обновлён";
\t\t\tbreak;
\t\tcase ERequest::DELETE_SHARE:
\t\t\tm_vShares.erase(std::remove_if(m_vShares.begin(), m_vShares.end(), [&](const SShare &Share) { return Share.m_Id == m_PendingId; }), m_vShares.end());
\t\t\tm_Status = "HTTPS-ссылка удалена";
\t\t\tm_PendingId.clear();
\t\t\tbreak;
'''
if old not in t:
    raise RuntimeError('completion switch marker not found')
t = t.replace(old, new, 1)

# Poll deep links before regular work.
t = t.replace('''\tvoid Poll(IHttp *pHttp, IClient *pClient)
\t{
\t\tif(m_pRequest && m_pRequest->Done())
\t\t\tHandleCompleted(pClient);
''', '''\tvoid Poll(IHttp *pHttp, IClient *pClient)
\t{
\t\tif(m_pRequest && m_pRequest->Done())
\t\t\tHandleCompleted(pClient);
\t\tHandlePendingDeepLink(pHttp);
''', 1)

# Update main save wording and insert new APIs before CreateShare.
t = t.replace('m_Status = "Сохраняю BKW настройки…";', 'm_Status = "Сохраняю настройки всего клиента…";', 1)
t = t.replace('m_Status = "Загружаю BKW настройки…";', 'm_Status = "Загружаю основное сохранение…";', 1)
marker = '\tvoid CreateShare(IHttp *pHttp)\n'
if marker not in t:
    raise RuntimeError('CreateShare marker not found')
methods = '''\tvoid ListSaves(IHttp *pHttp)
\t{
\t\tif(!pHttp || m_pRequest || !LoggedIn())
\t\t\treturn;
\t\tRun(pHttp, HttpGet("https://bkw-client.onrender.com/api/saves"), ERequest::LIST_SAVES, true);
\t\tm_Status = "Получаю список сохранений…";
\t}

\tvoid CreateSave(IHttp *pHttp)
\t{
\t\tif(!pHttp || m_pRequest || !LoggedIn())
\t\t\treturn;
\t\tchar aName[64];
\t\tstr_format(aName, sizeof(aName), "Сохранение %d", (int)m_vSaves.size() + 1);
\t\tstd::string Body = "{\\\"name\\\":\\\"";
\t\tBody += JsonEscape(aName);
\t\tBody += "\\\",\\\"settings\\\":";
\t\tBody += BuildSettingsSnapshot();
\t\tBody += '}';
\t\tRun(pHttp, HttpPostJson("https://bkw-client.onrender.com/api/saves", Body.c_str()), ERequest::CREATE_SAVE, true);
\t\tm_Status = "Создаю отдельное сохранение…";
\t}

\tvoid LoadSave(IHttp *pHttp, const char *pId)
\t{
\t\tif(!pHttp || m_pRequest || !LoggedIn() || !pId || !pId[0])
\t\t\treturn;
\t\tstd::string Url = std::string(BASE_URL) + "/api/saves/" + pId;
\t\tRun(pHttp, HttpGet(Url.c_str()), ERequest::LOAD_SAVE, true);
\t\tm_Status = "Загружаю выбранное сохранение…";
\t}

\tvoid UpdateSave(IHttp *pHttp, const char *pId)
\t{
\t\tif(!pHttp || m_pRequest || !LoggedIn() || !pId || !pId[0])
\t\t\treturn;
\t\tstd::string Url = std::string(BASE_URL) + "/api/saves/" + pId;
\t\tstd::string Body = "{\\\"settings\\\":";
\t\tBody += BuildSettingsSnapshot();
\t\tBody += '}';
\t\tRun(pHttp, HttpPostJson(Url.c_str(), Body.c_str()), ERequest::UPDATE_SAVE, true);
\t\tm_Status = "Перезаписываю сохранение…";
\t}

\tvoid DeleteSave(IHttp *pHttp, const char *pId)
\t{
\t\tif(!pHttp || m_pRequest || !LoggedIn() || !pId || !pId[0])
\t\t\treturn;
\t\tm_PendingId = pId;
\t\tstd::string Url = std::string(BASE_URL) + "/api/saves/" + pId + "/delete";
\t\tRun(pHttp, HttpPostJson(Url.c_str(), "{}"), ERequest::DELETE_SAVE, true);
\t\tm_Status = "Удаляю сохранение…";
\t}

\tvoid ListShares(IHttp *pHttp)
\t{
\t\tif(!pHttp || m_pRequest || !LoggedIn())
\t\t\treturn;
\t\tRun(pHttp, HttpGet("https://bkw-client.onrender.com/api/shares"), ERequest::LIST_SHARES, true);
\t\tm_Status = "Получаю мои HTTPS-ссылки…";
\t}

\tvoid DeleteShare(IHttp *pHttp, const char *pId)
\t{
\t\tif(!pHttp || m_pRequest || !LoggedIn() || !pId || !pId[0])
\t\t\treturn;
\t\tm_PendingId = pId;
\t\tstd::string Url = std::string(BASE_URL) + "/api/share/" + pId + "/delete";
\t\tRun(pHttp, HttpPostJson(Url.c_str(), "{}"), ERequest::DELETE_SHARE, true);
\t\tm_Status = "Удаляю HTTPS-ссылку…";
\t}

\tbool RegisterBkwProtocol()
\t{
#if defined(CONF_FAMILY_WINDOWS)
\t\tchar aPath[IO_MAX_PATH_LENGTH];
\t\tif(fs_executable_path(aPath, sizeof(aPath)) != 0)
\t\t{
\t\t\tm_Status = "Не удалось определить путь к BKW Client";
\t\t\treturn false;
\t\t}
\t\tbool Updated = false;
\t\tif(!windows_shell_register_protocol("bkw", aPath, &Updated))
\t\t{
\t\t\tm_Status = "Windows не разрешила зарегистрировать bkw://";
\t\t\treturn false;
\t\t}
\t\tif(Updated)
\t\t\twindows_shell_update();
\t\tm_Status = "Протокол bkw:// включён по вашему запросу";
\t\treturn true;
#else
\t\tm_Status = "Автоматическая регистрация bkw:// сейчас доступна только на Windows";
\t\treturn false;
#endif
\t}

'''
t = t.replace(marker, methods + marker, 1)

# Public getters.
t = t.replace('''\tconst char *LastShareUrl() const { return m_LastShareUrl.c_str(); }
\tint HttpStatus() const { return m_HttpStatus; }
''', '''\tconst char *LastShareUrl() const { return m_LastShareUrl.c_str(); }
\tconst std::vector<SSave> &Saves() const { return m_vSaves; }
\tconst std::vector<SShare> &Shares() const { return m_vShares; }
\tint HttpStatus() const { return m_HttpStatus; }
''', 1)

p.write_text(t, encoding='utf-8')
