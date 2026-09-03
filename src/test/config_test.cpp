#include "test.h"

#include <engine/client/menu_glass.h>
#include <engine/kernel.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <gtest/gtest.h>

#include <memory>

class CBkwConfigTest : public ::testing::Test
{
protected:
	CTestInfo m_TestInfo;
	std::unique_ptr<CConfig> m_pSavedConfig;
	std::unique_ptr<IStorage> m_pStorage;
	std::unique_ptr<IConsole> m_pConsole;
	std::unique_ptr<IConfigManager> m_pConfig;
	std::unique_ptr<IKernel> m_pKernel;

	void SetUp() override
	{
		m_pSavedConfig = std::make_unique<CConfig>(g_Config);
		m_pStorage = m_TestInfo.CreateTestStorage();
		ASSERT_NE(m_pStorage, nullptr);
		m_pConsole = CreateConsole(CFGFLAG_CLIENT);
		m_pConfig.reset(CreateConfigManager());
		m_pKernel.reset(IKernel::Create());
		m_pKernel->RegisterInterface<IStorage>(m_pStorage.get(), false);
		m_pKernel->RegisterInterface<IConsole>(m_pConsole.get(), false);
		m_pKernel->RegisterInterface<IConfigManager>(m_pConfig.get(), false);
		m_pConsole->Init();
		m_pConfig->Init();
		// Match the live client after startup has finished executing stored commands.
		m_pConsole->StoreCommands(false);
	}

	void TearDown() override
	{
		g_Config = *m_pSavedConfig;
	}

	void Execute(const char *pCommand)
	{
		m_pConsole->ExecuteLine(pCommand, IConsole::CLIENT_ID_UNSPECIFIED);
	}
};

TEST_F(CBkwConfigTest, AliasUsesOriginalChainAndClamp)
{
	int Calls = 0;
	m_pConsole->Chain("cl_showfps", [](IConsole::IResult *pResult, void *pUser, IConsole::FCommandCallback pfnCallback, void *pCallbackUser) {
		++*static_cast<int *>(pUser);
		pfnCallback(pResult, pCallbackUser);
	}, &Calls);
	g_Config.m_ClShowfps = 0;
	Execute("bkw_cl_showfps 10");
	EXPECT_EQ(g_Config.m_ClShowfps, 1);
	EXPECT_EQ(Calls, 1);
	Execute("cl_showfps 0");
	EXPECT_EQ(g_Config.m_ClShowfps, 0);
	EXPECT_EQ(Calls, 2);
	m_pConfig->SetReadOnly("cl_showfps", true);
	Execute("bkw_cl_showfps 1");
	EXPECT_EQ(g_Config.m_ClShowfps, 0);
}

TEST_F(CBkwConfigTest, StringAliasKeepsSemicolonsAndEscapesInsideValue)
{
	g_Config.m_ClShowfps = 0;
	Execute(R"(bkw_cl_languagefile "a; cl_showfps 1")");
	EXPECT_STREQ(g_Config.m_ClLanguagefile, "a; cl_showfps 1");
	EXPECT_EQ(g_Config.m_ClShowfps, 0);
	Execute(R"(bkw_cl_languagefile "a\\b\"c")");
	EXPECT_STREQ(g_Config.m_ClLanguagefile, "a\\b\"c");
}

TEST_F(CBkwConfigTest, AliasesSupportToggleResetAndGlassRanges)
{
	g_Config.m_ClShowfps = 0;
	Execute("toggle bkw_cl_showfps 0 1");
	EXPECT_EQ(g_Config.m_ClShowfps, 1);
	Execute("reset bkw_cl_showfps");
	EXPECT_EQ(g_Config.m_ClShowfps, 0);
	Execute("bkw_ui_glass_transparency 200");
	EXPECT_EQ(g_Config.m_BkwUiGlassTransparency, 100);
	Execute("bkw_ui_glass_blur -1");
	EXPECT_EQ(g_Config.m_BkwUiGlassBlur, 0);
	Execute("bkw_ui_glass_blur 150");
	EXPECT_EQ(g_Config.m_BkwUiGlassBlur, 100);
	Execute("bkw_ui_glass_blur 50");
	EXPECT_EQ(g_Config.m_BkwUiGlassBlur, 50);
	const auto *pOriginal = m_pConsole->GetCommandInfo("cl_languagefile", CFGFLAG_CLIENT, false);
	const auto *pAlias = m_pConsole->GetCommandInfo("bkw_cl_languagefile", CFGFLAG_CLIENT, false);
	ASSERT_NE(pOriginal, nullptr);
	ASSERT_NE(pAlias, nullptr);
	EXPECT_EQ(pOriginal->Flags(), pAlias->Flags());
}

TEST_F(CBkwConfigTest, BlurPercentageUsesIncreasingFootprintAndValidMipRegions)
{
	float PreviousScale = 0.0f;
	for(int Percent = 1; Percent <= 100; ++Percent)
	{
		const SMenuGlassBlur Blur(Percent);
		EXPECT_GT(Blur.m_Scale, PreviousScale);
		PreviousScale = Blur.m_Scale;
		EXPECT_GE(Blur.m_LastMip, 2);
		EXPECT_LT(Blur.m_LastMip, SMenuGlassBlur::MIP_LEVELS);
		for(int Size : {1, 17, 64, 719, 1080, 3840})
		{
			for(int Mip = 0; Mip <= Blur.m_LastMip; ++Mip)
			{
				EXPECT_GE(Blur.Size(Size, Mip), 1);
				EXPECT_LE(Blur.Size(Size, Mip), std::max(1, Size >> Mip));
			}
			EXPECT_EQ(Blur.Size(Size, 0), Size);
			EXPECT_EQ(Blur.Size(Size, 1), std::max(1, Size >> 1));
		}
	}
	EXPECT_FLOAT_EQ(SMenuGlassBlur(100).m_Scale, 64.0f);
}

TEST_F(CBkwConfigTest, GlassColorIsIndependentOfUiColor)
{
	const unsigned UiColor = g_Config.m_UiColor;
	Execute("bkw_ui_glass_color 65408");
	EXPECT_EQ(g_Config.m_UiColor, UiColor);
	const unsigned GlassColor = g_Config.m_BkwUiGlassColor;
	Execute("ui_color 123456");
	EXPECT_EQ(g_Config.m_BkwUiGlassColor, GlassColor);
}
