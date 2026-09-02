#include "test.h"

#include <base/windows.h>

#include <gtest/gtest.h>

#if defined(CONF_FAMILY_WINDOWS)

#include <base/str.h>
#include <engine/config.h>
#include <engine/console.h>
#include <engine/shared/fifo.h>

#include <atomic>
#include <thread>
#include <windows.h>

TEST(BkwDeepLink, DelayedReceiverAndRepeatedImports)
{
	auto pConsole = CreateConsole(CFGFLAG_CLIENT);
	std::string Received;
	pConsole->Register("bkw_deep_link", "s[uri]", CFGFLAG_CLIENT, [](IConsole::IResult *pResult, void *pUserData) {
		*static_cast<std::string *>(pUserData) = pResult->GetString(0);
	}, &Received, "Test deep-link receiver");
	char aPipeName[128];
	str_format(aPipeName, sizeof(aPipeName), "bkw-deeplink-test-%lu", GetCurrentProcessId());
	CFifo Fifo;
	Fifo.Init(pConsole.get(), aPipeName, CFGFLAG_CLIENT, true);

	for(const char *pUri : {"bkw://share/first1234567", "bkw://share/second123456", "bkw://auth/complete"})
	{
		Received.clear();
		std::atomic<bool> Done{false};
		bool Sent = false;
		std::thread Sender([&] {
			const std::string Command = std::string("bkw_deep_link \"") + pUri + "\"\n";
			Sent = windows_named_pipe_send(aPipeName, Command.c_str());
			Done = true;
		});
		// Simulate a minimized client which has not updated since the browser click.
		Sleep(100);
		const ULONGLONG Deadline = GetTickCount64() + 5000;
		while(!Done && GetTickCount64() < Deadline)
		{
			Fifo.Update();
			Sleep(1);
		}
		Sender.join();
		EXPECT_TRUE(Sent);
		EXPECT_EQ(Received, pUri);
		Fifo.Update(); // disconnect the acknowledged sender before the next import
	}
	Fifo.Shutdown();
}

TEST(ArgumentsToWide, SingleArgumentNoQuotes)
{
	const char *apArguments[] = {"change_map ctf5"};
	std::wstring Result = windows_args_to_wide(apArguments, std::size(apArguments));
	EXPECT_EQ(Result, LR"("change_map ctf5")");
}

TEST(ArgumentsToWide, SingleArgumentWithQuotes)
{
	const char *apArguments[] = {"change_map \"ctf5\""};
	std::wstring Result = windows_args_to_wide(apArguments, std::size(apArguments));
	EXPECT_EQ(Result, L"\"change_map \\\"ctf5\\\"\"");
}

TEST(ArgumentsToWide, MultipleArguments)
{
	const char *apArguments[] = {"change_map ctf5", "sv_register 0"};
	std::wstring Result = windows_args_to_wide(apArguments, std::size(apArguments));
	EXPECT_EQ(Result, LR"("change_map ctf5" "sv_register 0")");
}

TEST(ArgumentsToWide, SingleArgumentWithSlashes)
{
	const char *apArguments[] = {R"(sv_name te\st)"};
	std::wstring Result = windows_args_to_wide(apArguments, std::size(apArguments));
	EXPECT_EQ(Result, L"\"sv_name te\\st\"");
}

TEST(ArgumentsToWide, MultipleArgumentsWithSlashesAndQuotes)
{
	const char *apArguments[] = {R"(sv_name "te\\st")", R"(sv_motd "fo\\o")"};
	std::wstring Result = windows_args_to_wide(apArguments, std::size(apArguments));
	EXPECT_EQ(Result, L"\"sv_name \\\"te\\\\st\\\"\" \"sv_motd \\\"fo\\\\o\\\"\"");
}

TEST(ArgumentsToWide, MultipleArgumentsWithEndSlash)
{
	const char *apArguments[] = {R"(sv_name "te\\st\\")", R"(sv_motd foo\)"};
	std::wstring Result = windows_args_to_wide(apArguments, std::size(apArguments));
	EXPECT_EQ(Result, L"\"sv_name \\\"te\\\\st\\\\\\\\\\\"\" \"sv_motd foo\\\\\"");
}

#endif
