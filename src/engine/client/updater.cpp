/* Copyright © 2026 BestProject Team */
#include "updater.h"
#include "bkw_update_version.h"
#include <engine/shared/bkw_version.h>

#include <base/math.h>
#include <base/process.h>
#include <base/str.h>
#include <base/time.h>
#include <base/types.h>

#include <engine/client.h>
#include <engine/config.h>
#include <engine/external/json-parser/json.h>
#include <engine/http.h>
#include <engine/shared/config.h>
#include <engine/shared/json.h>
#include <engine/storage.h>

#include <game/client/components/bestclient/version.h>
#include <game/version.h>

#if defined(CONF_PLATFORM_ANDROID)
#include <android/android_main.h>
#endif

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>
#include <vector>

static bool StrEndsWithNoCase(const char *pStr, const char *pSuffix)
{
	if(!pStr || !pSuffix)
		return false;
	int StrLen = str_length(pStr);
	int SuffixLen = str_length(pSuffix);
	if(SuffixLen > StrLen)
		return false;
	return str_comp_nocase(pStr + StrLen - SuffixLen, pSuffix) == 0;
}

static constexpr const char *GITHUB_RELEASES_URL = "https://api.github.com/repos/Kakkek53/test/releases?per_page=100";
static constexpr const char *GITHUB_LATEST_RELEASE_URL = "https://github.com/Kakkek53/test/releases";
#if defined(CONF_PLATFORM_ANDROID)
static constexpr const char *UPDATE_ARCHIVE_PATH = "update/bestclient-release.apk";
#else
static constexpr const char *UPDATE_ARCHIVE_PATH = "update/bestclient-release.zip";
#endif


static std::string ToLowerAscii(const char *pStr)
{
	std::string Lower;
	if(!pStr)
		return Lower;

	for(const unsigned char *p = reinterpret_cast<const unsigned char *>(pStr); *p != '\0'; ++p)
		Lower.push_back(static_cast<char>(std::tolower(*p)));
	return Lower;
}

static const char *GetReleaseVersionString(const json_value *pJson)
{
	if(!pJson || pJson->type != json_object || json_boolean_get(json_object_get(pJson, "draft")))
		return nullptr;
	const char *pVersion = json_string_get(json_object_get(pJson, "name"));
	BkwUpdate::CVersion Version;
	return pVersion && Version.Parse(pVersion) && Version.m_Channel == g_Config.m_BkwUpdateChannel ? pVersion : nullptr;
}

static int ScoreArchiveAsset(const char *pAssetName)
{
	if(!pAssetName)
		return -1;

	const std::string Lower = ToLowerAscii(pAssetName);
	if(Lower.find("bestclient") == std::string::npos && Lower.find("bkw") == std::string::npos)
		return -1;

#if defined(CONF_FAMILY_WINDOWS)
	if(!StrEndsWithNoCase(pAssetName, ".zip"))
		return -1;
	if(Lower.find("windows") == std::string::npos && Lower.find("win") == std::string::npos)
		return -1;
#elif defined(CONF_PLATFORM_ANDROID)
	if(!StrEndsWithNoCase(pAssetName, ".apk"))
		return -1;
	if(Lower.find("android") == std::string::npos)
		return -1;
#elif defined(CONF_PLATFORM_LINUX)
	if(!StrEndsWithNoCase(pAssetName, ".tar.xz"))
		return -1;
	if(Lower.find("linux") == std::string::npos)
		return -1;
#else
	return -1;
#endif

	if(Lower.find("debug") != std::string::npos || Lower.find("symbols") != std::string::npos || Lower.find("source") != std::string::npos)
		return -1;

	// Never install an archive for another CPU architecture.
#if defined(CONF_ARCH_AMD64)
	if(Lower.find("arm") != std::string::npos || Lower.find("win32") != std::string::npos || Lower.find("i686") != std::string::npos)
		return -1;
#elif defined(CONF_ARCH_IA32)
	if(Lower.find("64") != std::string::npos || Lower.find("arm") != std::string::npos)
		return -1;
#endif
	int Score = 100;

#if defined(CONF_FAMILY_WINDOWS)
	if(Lower == "bestclient-windows.zip")
		Score += 200;
	if(Lower.find("x64") != std::string::npos || Lower.find("64") != std::string::npos || Lower.find("amd64") != std::string::npos)
		Score += 20;
#elif defined(CONF_PLATFORM_ANDROID)
	if(Lower == "bestclient-android.apk")
		Score += 200;
#elif defined(CONF_PLATFORM_LINUX)
	if(Lower == "bestclient-linux.tar.xz")
		Score += 200;
#endif

#if defined(CONF_ARCH_AMD64)
	if(Lower.find("x64") != std::string::npos || Lower.find("64") != std::string::npos || Lower.find("amd64") != std::string::npos)
		Score += 10;
#elif defined(CONF_ARCH_IA32)
	if(Lower.find("x86") != std::string::npos || Lower.find("32") != std::string::npos)
		Score += 10;
#endif

	return Score;
}

static bool ParseReleaseObject(const json_value *pJson, char *pVersion, int VersionSize, char *pArchiveName, int ArchiveNameSize, char *pArchiveUrl, int ArchiveUrlSize)
{
	if(!pJson || pJson->type != json_object)
		return false;

	const char *pReleaseVersion = GetReleaseVersionString(pJson);
	if(!pReleaseVersion)
		return false;

	const json_value *pAssets = json_object_get(pJson, "assets");
	if(!pAssets || pAssets->type != json_array)
		return false;

	int BestScore = -1;
	char aBestName[128] = "";
	char aBestUrl[2048] = "";

	for(int i = 0; i < json_array_length(pAssets); ++i)
	{
		const json_value *pAsset = json_array_get(pAssets, i);
		if(!pAsset || pAsset->type != json_object)
			continue;

		const char *pName = json_string_get(json_object_get(pAsset, "name"));
		const char *pUrl = json_string_get(json_object_get(pAsset, "browser_download_url"));
		const int Score = ScoreArchiveAsset(pName);
		if(!pName || !pUrl || Score < 0 || Score <= BestScore || !str_startswith(pUrl, "https://github.com/Kakkek53/test/releases/download/"))
			continue;

		BestScore = Score;
		str_copy(aBestName, pName, sizeof(aBestName));
		str_copy(aBestUrl, pUrl, sizeof(aBestUrl));
	}

	if(BestScore < 0)
		return false;

	str_copy(pVersion, pReleaseVersion, VersionSize);
	str_copy(pArchiveName, aBestName, ArchiveNameSize);
	str_copy(pArchiveUrl, aBestUrl, ArchiveUrlSize);
	return true;
}

static bool ParseLatestRelease(json_value *pJson, char *pVersion, int VersionSize, char *pArchiveName, int ArchiveNameSize, char *pArchiveUrl, int ArchiveUrlSize)
{
	if(!pJson)
		return false;

	if(pJson->type == json_object)
		return ParseReleaseObject(pJson, pVersion, VersionSize, pArchiveName, ArchiveNameSize, pArchiveUrl, ArchiveUrlSize);

	if(pJson->type == json_array)
	{
		const json_value *pBestRelease = nullptr;
		char aBestVersion[64] = "";
		for(int i = 0; i < json_array_length(pJson); ++i)
		{
			const json_value *pRelease = json_array_get(pJson, i);
			const char *pReleaseVersion = GetReleaseVersionString(pRelease);
			if(!pReleaseVersion)
				continue;

			char aCandidateVersion[64], aCandidateName[128], aCandidateUrl[2048];
			if(!ParseReleaseObject(pRelease, aCandidateVersion, sizeof(aCandidateVersion), aCandidateName, sizeof(aCandidateName), aCandidateUrl, sizeof(aCandidateUrl)))
				continue;
			BkwUpdate::CVersion Candidate, Best;
			Candidate.Parse(pReleaseVersion);
			Best.Parse(aBestVersion);
			if(!pBestRelease || Candidate.m_Numbers > Best.m_Numbers)
			{
				pBestRelease = pRelease;
				str_copy(aBestVersion, pReleaseVersion, sizeof(aBestVersion));
			}
		}

		if(pBestRelease)
			return ParseReleaseObject(pBestRelease, pVersion, VersionSize, pArchiveName, ArchiveNameSize, pArchiveUrl, ArchiveUrlSize);
	}

	return false;
}

static void StripFilename(char *pPath)
{
	if(!pPath)
		return;

	for(int i = str_length(pPath) - 1; i >= 0; --i)
	{
		if(pPath[i] == '/' || pPath[i] == '\\')
		{
			pPath[i] = '\0';
			return;
		}
	}
	pPath[0] = '\0';
}

CUpdater::CUpdater()
{
	m_pClient = nullptr;
	m_pStorage = nullptr;
	m_pHttp = nullptr;

	m_State = CLEAN;
	m_aStatus[0] = '\0';
	m_Percent = 0;
	m_aLatestVersion[0] = '\0';
	m_aArchiveName[0] = '\0';
	m_aArchiveUrl[0] = '\0';
	str_copy(m_aArchivePath, UPDATE_ARCHIVE_PATH, sizeof(m_aArchivePath));
}

void CUpdater::Init()
{
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pHttp = Kernel()->RequestInterface<IHttp>();

	// Check BKW releases on startup, without replacing a running game automatically.
	m_bAutoCheckPending = true;
}

void CUpdater::SetCurrentState(EUpdaterState NewState)
{
	const CLockScope LockScope(m_Lock);
	m_State = NewState;
}

void CUpdater::SetStatus(const char *pStatus)
{
	const CLockScope LockScope(m_Lock);
	str_copy(m_aStatus, pStatus ? pStatus : "", sizeof(m_aStatus));
}

void CUpdater::SetPercent(int Percent)
{
	const CLockScope LockScope(m_Lock);
	m_Percent = std::clamp(Percent, 0, 100);
}

IUpdater::EUpdaterState CUpdater::GetCurrentState()
{
	const CLockScope LockScope(m_Lock);
	return m_State;
}

void CUpdater::GetCurrentFile(char *pBuf, int BufSize)
{
	const CLockScope LockScope(m_Lock);
	str_copy(pBuf, m_aStatus, BufSize);
}

int CUpdater::GetCurrentPercent()
{
	const CLockScope LockScope(m_Lock);
	return m_Percent;
}

const char *CUpdater::GetLatestVersionString()
{
	return m_aLatestVersion;
}

void CUpdater::ResetTask()
{
	if(m_pCurrentTask)
	{
		m_pCurrentTask->Abort();
		m_pCurrentTask = nullptr;
	}
	m_TaskKind = ETaskKind::NONE;
}

void CUpdater::StartReleaseFetch(bool List)
{
	ResetTask();
	SetStatus("Checking latest release");
	SetPercent(0);
	SetCurrentState(IUpdater::GETTING_MANIFEST);

	char aUrl[2304];
	m_FetchReleaseList = List;
	m_CheckedChannel = g_Config.m_BkwUpdateChannel;
	str_copy(aUrl, List ? GITHUB_RELEASES_URL : "https://api.github.com/repos/Kakkek53/test/releases/latest");
	m_TaskKind = ETaskKind::FETCH_RELEASE;
	m_pCurrentTask = HttpGet(aUrl);
	// A repository with only prereleases legitimately returns 404 for /latest.
	// Keep the HTTP response available so the updater can fall back quietly.
	m_pCurrentTask->FailOnErrorStatus(false);
	m_pCurrentTask->HeaderString("Accept", "application/vnd.github+json");
	m_pCurrentTask->HeaderString("User-Agent", CLIENT_NAME);
	m_pCurrentTask->HeaderString("X-GitHub-Api-Version", "2022-11-28");
	m_pCurrentTask->HeaderString("Cache-Control", "no-cache");
	m_pCurrentTask->HeaderString("Pragma", "no-cache");
	m_pCurrentTask->Timeout(CTimeout{10000, 60000, 500, 10});
	m_pCurrentTask->IpResolve(IPRESOLVE::V4);
	m_pHttp->Run(m_pCurrentTask);
}

void CUpdater::ParseReleaseTask()
{
	json_value *pJson = m_pCurrentTask ? m_pCurrentTask->ResultJson() : nullptr;
	if(!pJson)
	{
		SetStatus("Failed to parse release info");
		SetCurrentState(IUpdater::FAIL);
		return;
	}

	char aVersion[64] = "";
	char aArchiveName[128] = "";
	char aArchiveUrl[2048] = "";

	const bool Parsed = ParseLatestRelease(pJson, aVersion, sizeof(aVersion), aArchiveName, sizeof(aArchiveName), aArchiveUrl, sizeof(aArchiveUrl));
	json_value_free(pJson);

	if(!Parsed || !BkwUpdate::IsNewer(aVersion, BKW_VERSION, m_CheckedChannel))
	{
		m_aLatestVersion[0] = '\0';
		m_aArchiveName[0] = '\0';
		m_aArchiveUrl[0] = '\0';
		SetStatus("No update available");
		SetCurrentState(IUpdater::CLEAN);
		return;
	}

	str_copy(m_aLatestVersion, aVersion, sizeof(m_aLatestVersion));
	str_copy(m_aArchiveName, aArchiveName, sizeof(m_aArchiveName));
	str_copy(m_aArchiveUrl, aArchiveUrl, sizeof(m_aArchiveUrl));
	SetStatus("Update available");
	SetCurrentState(IUpdater::VERSION_AVAILABLE);
}

void CUpdater::StartArchiveDownload()
{
	ResetTask();
	char aUpdateDir[IO_MAX_PATH_LENGTH];
	m_pStorage->GetBinaryPathAbsolute("update", aUpdateDir, sizeof(aUpdateDir));
	m_pStorage->CreateFolder(aUpdateDir, IStorage::TYPE_ABSOLUTE);
	m_pStorage->GetBinaryPathAbsolute(UPDATE_ARCHIVE_PATH, m_aArchivePath, sizeof(m_aArchivePath));
	m_pStorage->RemoveFile(m_aArchivePath, IStorage::TYPE_ABSOLUTE);

	SetStatus(m_aArchiveName);
	SetPercent(0);
	SetCurrentState(IUpdater::DOWNLOADING);

	m_TaskKind = ETaskKind::DOWNLOAD_ARCHIVE;
	m_pCurrentTask = HttpGetFile(m_aArchiveUrl, m_pStorage, m_aArchivePath, IStorage::TYPE_ABSOLUTE);
	m_pCurrentTask->HeaderString("User-Agent", CLIENT_NAME);
	m_pCurrentTask->Timeout(CTimeout{10000, 0, 500, 10});
	m_pCurrentTask->IpResolve(IPRESOLVE::V4);
	m_pHttp->Run(m_pCurrentTask);
}

bool CUpdater::LaunchApplyScriptAndQuit()
{
#if defined(CONF_FAMILY_WINDOWS)
	char aArchivePath[IO_MAX_PATH_LENGTH];
	char aUpdaterPath[IO_MAX_PATH_LENGTH];
	char aInstallDir[IO_MAX_PATH_LENGTH];
	char aExePath[IO_MAX_PATH_LENGTH];
	char aPid[32];

	str_copy(aArchivePath, m_aArchivePath, sizeof(aArchivePath));
	if(!m_pStorage->FileExists(aArchivePath, IStorage::TYPE_ABSOLUTE))
	{
		SetStatus("Downloaded archive missing");
		return false;
	}

	m_pStorage->GetBinaryPathAbsolute("bestclient-updater.exe", aUpdaterPath, sizeof(aUpdaterPath));
	m_pStorage->GetBinaryPathAbsolute(PLAT_CLIENT_EXEC, aExePath, sizeof(aExePath));
	str_copy(aInstallDir, aExePath, sizeof(aInstallDir));
	StripFilename(aInstallDir);

	str_format(aPid, sizeof(aPid), "%d", process_id());
	const char *apArguments[] = {aPid, aArchivePath, aInstallDir, aExePath};

	if(process_execute(aUpdaterPath, EShellExecuteWindowState::FOREGROUND, apArguments, std::size(apArguments)) == INVALID_PROCESS)
	{
		SetStatus("Failed to launch updater");
		return false;
	}

	m_pClient->Quit();
	return true;
#elif defined(CONF_PLATFORM_ANDROID)
	char aArchivePath[IO_MAX_PATH_LENGTH];

	str_copy(aArchivePath, m_aArchivePath, sizeof(aArchivePath));
	if(!m_pStorage->FileExists(aArchivePath, IStorage::TYPE_ABSOLUTE))
	{
		SetStatus("Downloaded archive missing");
		return false;
	}

	char aAbsoluteArchivePath[IO_MAX_PATH_LENGTH];
	str_copy(aAbsoluteArchivePath, m_aArchivePath, sizeof(aAbsoluteArchivePath));

	if(!InstallAndroidApk(aAbsoluteArchivePath))
	{
		SetStatus("Failed to launch installer");
		return false;
	}

	// The OS replaces the running process once the user confirms the install,
	// so the client must keep running instead of quitting like on Windows/Linux.
	return true;
#elif defined(CONF_PLATFORM_LINUX)
	char aArchivePath[IO_MAX_PATH_LENGTH];
	char aUpdaterPath[IO_MAX_PATH_LENGTH];
	char aInstallDir[IO_MAX_PATH_LENGTH];
	char aExePath[IO_MAX_PATH_LENGTH];
	char aPid[32];

	str_copy(aArchivePath, m_aArchivePath, sizeof(aArchivePath));
	if(!m_pStorage->FileExists(aArchivePath, IStorage::TYPE_ABSOLUTE))
	{
		SetStatus("Downloaded archive missing");
		return false;
	}

	m_pStorage->GetBinaryPathAbsolute("bestclient-updater", aUpdaterPath, sizeof(aUpdaterPath));
	m_pStorage->GetBinaryPathAbsolute(PLAT_CLIENT_EXEC, aExePath, sizeof(aExePath));
	str_copy(aInstallDir, aExePath, sizeof(aInstallDir));
	StripFilename(aInstallDir);

	str_format(aPid, sizeof(aPid), "%d", process_id());
	const char *apArguments[] = {aPid, aArchivePath, aInstallDir, aExePath};

	if(process_execute(aUpdaterPath, EShellExecuteWindowState::FOREGROUND, apArguments, std::size(apArguments)) == INVALID_PROCESS)
	{
		SetStatus("Failed to launch updater");
		return false;
	}

	m_pClient->Quit();
	return true;
#else
	SetStatus("Archive updater is only available on Windows and Linux");
	return false;
#endif
}

void CUpdater::CheckForUpdate()
{
	const EUpdaterState State = GetCurrentState();
	if(State == IUpdater::GETTING_MANIFEST || State == IUpdater::DOWNLOADING)
		return;

#if !defined(CONF_FAMILY_WINDOWS) && !defined(CONF_PLATFORM_LINUX) && !defined(CONF_PLATFORM_ANDROID)
	if(m_pClient)
		m_pClient->ViewLink(GITHUB_LATEST_RELEASE_URL);
	return;
#endif

	m_aLatestVersion[0] = '\0';
	m_aArchiveName[0] = '\0';
	m_aArchiveUrl[0] = '\0';
	StartReleaseFetch();
}

void CUpdater::InitiateUpdate()
{
	const EUpdaterState State = GetCurrentState();
	if(State == IUpdater::GETTING_MANIFEST || State == IUpdater::DOWNLOADING)
		return;

	if((State == IUpdater::VERSION_AVAILABLE || State == IUpdater::FAIL) && m_aArchiveUrl[0] != '\0')
	{
		StartArchiveDownload();
		return;
	}

	CheckForUpdate();
}

void CUpdater::ApplyUpdateAndRestart()
{
	if(GetCurrentState() != IUpdater::NEED_RESTART)
		return;

	if(!LaunchApplyScriptAndQuit())
		SetCurrentState(IUpdater::FAIL);
}

void CUpdater::Update()
{
	if(m_CheckedChannel >= 0 && m_CheckedChannel != g_Config.m_BkwUpdateChannel)
	{
		ResetTask();
		m_aLatestVersion[0] = '\0';
		m_aArchiveUrl[0] = '\0';
		m_aArchiveName[0] = '\0';
		SetCurrentState(CLEAN);
		m_bAutoCheckPending = true;
	}

	if(g_Config.m_BcAutoUpdate != 0)
	{
		const EUpdaterState State = GetCurrentState();
		if(State == IUpdater::VERSION_AVAILABLE)
			InitiateUpdate();
	}

	if(m_bAutoCheckPending && m_pHttp && GetCurrentState() == CLEAN)
	{
		m_bAutoCheckPending = false;
		CheckForUpdate();
	}

	if(!m_pCurrentTask)
		return;

	if(!m_pCurrentTask->Done())
	{
		if(GetCurrentState() == IUpdater::DOWNLOADING)
			SetPercent(m_pCurrentTask->Progress());
		return;
	}

	// /latest omits GitHub prereleases and libcurl reports its 404 as ERROR.
	// Do not call StatusCode or ResultJson for that request: both assert unless
	// the request state is DONE. The channel-aware list is authoritative.
	if(m_TaskKind == ETaskKind::FETCH_RELEASE && !m_FetchReleaseList)
	{
		StartReleaseFetch(true);
		return;
	}

	if(m_pCurrentTask->State() != EHttpState::DONE || m_pCurrentTask->StatusCode() >= 400)
	{
		ResetTask();
		SetStatus("Update check failed");
		SetCurrentState(IUpdater::FAIL);
		return;
	}

	if(m_TaskKind == ETaskKind::FETCH_RELEASE)
	{
		ParseReleaseTask();
		ResetTask();
		return;
	}

	if(m_TaskKind == ETaskKind::DOWNLOAD_ARCHIVE)
	{
		ResetTask();
		SetPercent(100);
		SetStatus(m_aArchiveName[0] != '\0' ? m_aArchiveName : "update");
		SetCurrentState(IUpdater::NEED_RESTART);
	}
}
