from pathlib import Path

# Windows helpers: local named-pipe handoff + bring current process window forward.
p = Path('src/base/windows.h')
t = p.read_text(encoding='utf-8')
marker = 'void windows_shell_update();\n'
add = '''void windows_shell_update();

/** Send one UTF-8 message to a local Windows named pipe. */
bool windows_named_pipe_send(const char *pipe_name, const char *message);

/** Restore and focus the visible top-level window owned by this process. */
void windows_activate_current_process_window();
'''
if 'windows_named_pipe_send' not in t:
    if marker not in t: raise RuntimeError('windows.h marker not found')
    t = t.replace(marker, add, 1)
p.write_text(t, encoding='utf-8')

p = Path('src/base/windows.cpp')
t = p.read_text(encoding='utf-8')
insert = '''
bool windows_named_pipe_send(const char *pipe_name, const char *message)
{
	if(!pipe_name || !pipe_name[0] || !message)
		return false;
	std::string full_name = "\\\\\\\\.\\\\pipe\\\\";
	full_name += pipe_name;
	const std::wstring wide_name = windows_utf8_to_wide(full_name.c_str());
	HANDLE pipe = CreateFileW(wide_name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if(pipe == INVALID_HANDLE_VALUE)
		return false;
	const DWORD size = (DWORD)str_length(message);
	DWORD written = 0;
	const bool success = WriteFile(pipe, message, size, &written, nullptr) != FALSE && written == size;
	CloseHandle(pipe);
	return success;
}

static BOOL CALLBACK windows_activate_current_process_window_callback(HWND window, LPARAM parameter)
{
	DWORD process_id = 0;
	GetWindowThreadProcessId(window, &process_id);
	if(process_id != GetCurrentProcessId() || !IsWindowVisible(window) || GetWindow(window, GW_OWNER) != nullptr)
		return TRUE;
	ShowWindowAsync(window, SW_RESTORE);
	SetForegroundWindow(window);
	*reinterpret_cast<bool *>(parameter) = true;
	return FALSE;
}

void windows_activate_current_process_window()
{
	bool found = false;
	EnumWindows(windows_activate_current_process_window_callback, reinterpret_cast<LPARAM>(&found));
}
'''
if 'windows_named_pipe_send(' not in t:
    pos = t.rfind('\n#endif')
    if pos < 0: raise RuntimeError('windows.cpp endif not found')
    t = t[:pos] + insert + t[pos:]
p.write_text(t, encoding='utf-8')

# Dedicated local FIFO on the running client.
p = Path('src/engine/client/client.h')
t = p.read_text(encoding='utf-8')
if 'm_BkwDeepLinkFifo' not in t:
    if '\tCFifo m_Fifo;\n' not in t: raise RuntimeError('client.h fifo marker not found')
    t = t.replace('\tCFifo m_Fifo;\n', '\tCFifo m_Fifo;\n\tCFifo m_BkwDeepLinkFifo;\n', 1)
p.write_text(t, encoding='utf-8')

p = Path('src/engine/client/client.cpp')
t = p.read_text(encoding='utf-8')

# Console command executed by CFifo in the already-running instance.
reg_marker = 'void CClient::RegisterCommands()\n{\n'
reg_code = '''void CClient::RegisterCommands()
{
#if defined(CONF_FAMILY_WINDOWS)
	m_pConsole->Register("bkw_deep_link", "s[uri]", CFGFLAG_CLIENT, [](IConsole::IResult *pResult, void *pUserData) {
		CClient *pSelf = static_cast<CClient *>(pUserData);
		const char *pUri = pResult->GetString(0);
		if(!pUri || !str_startswith(pUri, "bkw:") || str_length(pUri) >= (int)sizeof(g_Config.m_BkwPendingDeepLink))
			return;
		str_copy(g_Config.m_BkwPendingDeepLink, pUri);
		windows_activate_current_process_window();
	}, this, "Handle a trusted local BKW deep link");
#endif
'''
if '"bkw_deep_link"' not in t:
    if reg_marker not in t: raise RuntimeError('RegisterCommands marker not found')
    t = t.replace(reg_marker, reg_code, 1)

init_marker = '\tm_Fifo.Init(m_pConsole, g_Config.m_ClInputFifo, CFGFLAG_CLIENT);\n'
if 'm_BkwDeepLinkFifo.Init' not in t:
    if init_marker not in t: raise RuntimeError('fifo init marker not found')
    t = t.replace(init_marker, init_marker + '#if defined(CONF_FAMILY_WINDOWS)\n\tm_BkwDeepLinkFifo.Init(m_pConsole, "bkw-client-deeplink", CFGFLAG_CLIENT);\n#endif\n', 1)

# Update and shutdown alongside the existing FIFO.
if 'm_BkwDeepLinkFifo.Update();' not in t:
    if '\tm_Fifo.Update();\n' in t:
        t = t.replace('\tm_Fifo.Update();\n', '\tm_Fifo.Update();\n#if defined(CONF_FAMILY_WINDOWS)\n\tm_BkwDeepLinkFifo.Update();\n#endif\n', 1)
    else:
        raise RuntimeError('fifo update marker not found')
if 'm_BkwDeepLinkFifo.Shutdown();' not in t:
    if '\tm_Fifo.Shutdown();\n' in t:
        t = t.replace('\tm_Fifo.Shutdown();\n', '\tm_Fifo.Shutdown();\n#if defined(CONF_FAMILY_WINDOWS)\n\tm_BkwDeepLinkFifo.Shutdown();\n#endif\n', 1)
    else:
        raise RuntimeError('fifo shutdown marker not found')

# A protocol-launched second process forwards bkw:// to the first process and exits.
main_marker = '''#else
int main(int argc, const char **argv)
#endif
{
	const int64_t MainStart = time_get();
'''
main_new = '''#else
int main(int argc, const char **argv)
#endif
{
#if defined(CONF_FAMILY_WINDOWS) && !defined(CONF_PLATFORM_ANDROID)
	for(int i = 1; i < argc; ++i)
	{
		const char *pArg = argv[i];
		if(!pArg || !str_startswith(pArg, "bkw:") || str_length(pArg) >= 240 || str_find(pArg, "\\\"") || str_find(pArg, "\\n") || str_find(pArg, "\\r"))
			continue;
		char aCommand[320];
		str_format(aCommand, sizeof(aCommand), "bkw_deep_link \\\"%s\\\"\\n", pArg);
		if(windows_named_pipe_send("bkw-client-deeplink", aCommand))
			return 0;
	}
#endif
	const int64_t MainStart = time_get();
'''
if 'windows_named_pipe_send("bkw-client-deeplink"' not in t:
    if main_marker not in t: raise RuntimeError('main marker not found')
    t = t.replace(main_marker, main_new, 1)

p.write_text(t, encoding='utf-8')
