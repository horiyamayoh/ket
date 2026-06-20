#include "ket_deadline.h"

#include <chrono> // IWYU pragma: keep
#include <thread>
// IWYU pragma: no_include <string>

#include <gtest/gtest.h>

/**
 * @test
 * @brief stopwatchの経過時間が非負で取得できることの確認。
 * @details StartNew直後に経過時間を取得し、durationとmillisecondsのどちらも負にならないことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDeadlineTest, StopwatchElapsedIsNonNegative)
{
	const auto stopwatch = ket::deadline::Stopwatch::StartNew();

	const auto elapsed = stopwatch.Elapsed();
	const auto elapsed_milliseconds = stopwatch.ElapsedMilliseconds();

	EXPECT_GE(elapsed, std::chrono::steady_clock::duration::zero());
	EXPECT_GE(elapsed_milliseconds, std::chrono::milliseconds::zero());
}

/**
 * @test
 * @brief stopwatchのrestartが開始点を更新することの確認。
 * @details
 * sleepで経過時間を作ってからRestartし、restart直後の経過時間がrestart前より短くなることを確認。
 * @pre C++17以降。
 * @post stopwatchはrestart後の開始点を保持。外部状態の変更なし。
 */
TEST(KetDeadlineTest, StopwatchRestartResetsStartTime)
{
	auto stopwatch = ket::deadline::Stopwatch::StartNew();
	std::this_thread::sleep_for(std::chrono::milliseconds(20));

	const auto before_restart = stopwatch.Elapsed();
	stopwatch.Restart();
	const auto after_restart = stopwatch.Elapsed();
	const auto restart_reset_elapsed = after_restart < before_restart;

	EXPECT_TRUE(restart_reset_elapsed);
}

/**
 * @test
 * @brief zero timeoutのdeadlineが期限切れになることの確認。
 * @details zero timeoutでAfterを呼び出し、ExpiredがtrueかつRemainingがzeroへ丸められることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDeadlineTest, ZeroTimeoutExpiresImmediately)
{
	const auto deadline =
		ket::deadline::Deadline::After(std::chrono::steady_clock::duration::zero());

	const auto expired = deadline.Expired();
	const auto remaining = deadline.Remaining();

	EXPECT_TRUE(expired);
	EXPECT_EQ(remaining, std::chrono::steady_clock::duration::zero());
}

/**
 * @test
 * @brief 負timeoutのdeadlineが期限切れになることの確認。
 * @details 負のdurationでAfterを呼び出し、ExpiredがtrueかつRemainingがzeroへ丸められることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDeadlineTest, NegativeTimeoutExpiresImmediately)
{
	const auto deadline = ket::deadline::Deadline::After(-std::chrono::milliseconds(1));

	const auto expired = deadline.Expired();
	const auto remaining = deadline.Remaining();

	EXPECT_TRUE(expired);
	EXPECT_EQ(remaining, std::chrono::steady_clock::duration::zero());
}

/**
 * @test
 * @brief 巨大timeoutのdeadlineが最大time_pointへ飽和することの確認。
 * @details
 * `duration::max()`でAfterを呼び出し、保持時刻が`time_point::max()`になり期限切れにならないことを確認。
 * @pre C++17以降。`steady_clock::now()`がepochより後の通常環境。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDeadlineTest, HugeTimeoutSaturatesAtMaxTimePoint)
{
	const auto deadline =
		ket::deadline::Deadline::After(std::chrono::steady_clock::duration::max());

	const auto stored_time_point = deadline.TimePoint();
	const auto expired = deadline.Expired();
	const auto remaining = deadline.Remaining();

	EXPECT_EQ(stored_time_point, std::chrono::steady_clock::time_point::max());
	EXPECT_FALSE(expired);
	EXPECT_GT(remaining, std::chrono::steady_clock::duration::zero());
}

/**
 * @test
 * @brief future deadlineが期限切れではないことの確認。
 * @details
 * 十分に長いtimeoutでAfterを呼び出し、ExpiredがfalseでRemainingが正値かつtimeout以下になることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDeadlineTest, FutureDeadlineIsNotExpired)
{
	const auto timeout = std::chrono::hours(1);
	const auto deadline = ket::deadline::Deadline::After(timeout);

	const auto expired = deadline.Expired();
	const auto remaining = deadline.Remaining();

	EXPECT_FALSE(expired);
	EXPECT_GT(remaining, std::chrono::steady_clock::duration::zero());
	EXPECT_LE(remaining, timeout);
}

/**
 * @test
 * @brief Remainingが負値を返さないことの確認。
 * @details 過去のtime_pointをAtで指定し、期限切れのRemainingがzeroへ丸められることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDeadlineTest, RemainingClampsExpiredDeadlineToZero)
{
	const auto past_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);
	const auto deadline = ket::deadline::Deadline::At(past_time);

	const auto expired = deadline.Expired();
	const auto remaining = deadline.Remaining();

	EXPECT_TRUE(expired);
	EXPECT_EQ(remaining, std::chrono::steady_clock::duration::zero());
}

/**
 * @test
 * @brief Atが指定time_pointを保持することの確認。
 * @details future time_pointをAtへ渡し、TimePointが同じ値を返して期限切れではないことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetDeadlineTest, AtKeepsSpecifiedTimePoint)
{
	const auto time_point = std::chrono::steady_clock::now() + std::chrono::hours(1);
	const auto deadline = ket::deadline::Deadline::At(time_point);

	const auto stored_time_point = deadline.TimePoint();
	const auto expired = deadline.Expired();

	EXPECT_EQ(stored_time_point, time_point);
	EXPECT_FALSE(expired);
}
