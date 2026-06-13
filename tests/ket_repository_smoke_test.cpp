#include <string> // IWYU pragma: keep

#include <gtest/gtest.h>

/**
 * @test
 * @brief GoogleTest harnessの起動確認。
 * @details 最小の真値判定を実行し、CTestからGoogleTestのtest caseが検出されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetRepositorySmokeTest, TestHarnessIsReady)
{
	const auto harness_is_ready = true;

	EXPECT_TRUE(harness_is_ready);
}
