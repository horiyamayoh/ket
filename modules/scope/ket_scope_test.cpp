#include "ket_scope.h"

#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string> // NOLINT(misc-include-cleaner)
#include <utility>

#include <gtest/gtest.h>

namespace
{
	constexpr int kTerminateExitCode = 86;

	void ExitFromTerminate() noexcept
	{
		std::_Exit(kTerminateExitCode);
	}

	int ReturnAfterScopeExit(int& count)
	{
		const auto guard = ket::scope::MakeExit(
			[&count]
			{
				++count;
			});
		static_cast<void>(guard);

		return 7;
	}

	int ReturnAfterRestore(int& value)
	{
		const auto restore = ket::scope::MakeRestore(value);
		static_cast<void>(restore);

		value = 42;
		return 7;
	}

	class ThrowingAssignment
	{
	  public:
		explicit ThrowingAssignment(int value) : value_(value) {}

		ThrowingAssignment(const ThrowingAssignment&) = default;

		ThrowingAssignment& operator=(const ThrowingAssignment& other)
		{
			const auto should_throw = other.value_ == 1;
			if (should_throw)
			{
				throw std::runtime_error("restore assignment");
			}

			value_ = other.value_;
			return *this;
		}

	  private:
		int value_;
	};

} // namespace

/**
 * @test
 * @brief scope exit callback の実行確認。
 * @details block を抜ける時に active guard の callback が1回だけ呼ばれることを確認。
 * @pre C++17以降。
 * @post count 以外の外部状態の変更なし。
 */
TEST(KetScopeTest, RunsExitCallbackOnScopeExit)
{
	int count = 0;

	{
		const auto guard = ket::scope::MakeExit(
			[&count]
			{
				++count;
			});
		const auto guard_is_active = guard.Active();

		EXPECT_TRUE(guard_is_active);
		EXPECT_EQ(count, 0);
	}

	EXPECT_EQ(count, 1);
}

/**
 * @test
 * @brief dismiss 済み guard の callback 抑止確認。
 * @details Dismiss後に block を抜けても callback が呼ばれないことを確認。
 * @pre C++17以降。
 * @post count は入力時の値を保持。
 */
TEST(KetScopeTest, DismissPreventsExitCallback)
{
	int count = 0;

	{
		auto guard = ket::scope::MakeExit(
			[&count]
			{
				++count;
			});
		guard.Dismiss();
		const auto guard_is_active = guard.Active();

		EXPECT_FALSE(guard_is_active);
	}

	EXPECT_EQ(count, 0);
}

/**
 * @test
 * @brief scope exit guard の move 確認。
 * @details guard を move し、source が inactive、destination が active になり callback
 * が1回だけ走ることを確認。
 * @pre C++17以降。
 * @post count 以外の外部状態の変更なし。
 */
TEST(KetScopeTest, MoveTransfersExitResponsibility)
{
	int count = 0;

	{
		auto source = ket::scope::MakeExit(
			[&count]
			{
				++count;
			});
		auto destination = std::move(source);
		// cppcheck-suppress accessMoved
		// NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
		const auto source_is_active = source.Active();
		const auto destination_is_active = destination.Active();

		EXPECT_FALSE(source_is_active);
		EXPECT_TRUE(destination_is_active);
	}

	EXPECT_EQ(count, 1);
}

/**
 * @test
 * @brief move 後 source と dismiss による二重実行防止確認。
 * @details move 後 source を破棄しても callback が走らず、destination を Dismiss
 * すると実行回数が0のままになることを確認。
 * @pre C++17以降。
 * @post count は入力時の値を保持。
 */
TEST(KetScopeTest, DoesNotRunMovedOrDismissedExitGuard)
{
	int count = 0;

	{
		auto source = ket::scope::MakeExit(
			[&count]
			{
				++count;
			});
		auto destination = std::move(source);
		destination.Dismiss();
		const auto destination_is_active = destination.Active();

		EXPECT_FALSE(destination_is_active);
	}

	EXPECT_EQ(count, 0);
}

/**
 * @test
 * @brief 早期 return 相当の scope exit 確認。
 * @details return で block を抜ける経路でも scope exit callback
 * が実行され、戻り値は保持されることを確認。
 * @pre C++17以降。
 * @post count 以外の外部状態の変更なし。
 */
TEST(KetScopeTest, RunsExitCallbackDuringEarlyReturn)
{
	int count = 0;

	const auto result = ReturnAfterScopeExit(count);

	EXPECT_EQ(result, 7);
	EXPECT_EQ(count, 1);
}

/**
 * @test
 * @brief scope exit callback 例外の terminate 確認。
 * @details destructor 内で callback が例外を投げた場合、例外を外へ出さず process termination
 * になることを確認。
 * @pre C++17以降。GoogleTest death test が利用可能。
 * @post 親 process の外部状態の変更なし。
 */
TEST(KetScopeTest, TerminatesWhenExitCallbackThrows)
{
	EXPECT_EXIT(
		{
			std::set_terminate(ExitFromTerminate);
			const auto guard = ket::scope::MakeExit(
				[]
				{
					throw std::runtime_error("cleanup");
				});
			static_cast<void>(guard);
		},
		::testing::ExitedWithCode(kTerminateExitCode),
		"");
}

/**
 * @test
 * @brief scope exit 時の値復元確認。
 * @details block 内で変更した対象値が、Restore の destructor により構築時の値へ戻ることを確認。
 * @pre C++17以降。
 * @post value は構築時の値へ復元。
 */
TEST(KetScopeTest, RestoresValueOnScopeExit)
{
	int value = 3;

	{
		const auto restore = ket::scope::MakeRestore(value);
		static_cast<void>(restore);
		value = 9;
	}

	EXPECT_EQ(value, 3);
}

/**
 * @test
 * @brief dismiss 済み restore guard の復元抑止確認。
 * @details Dismiss後に block を抜けても対象値が変更後の値を保持することを確認。
 * @pre C++17以降。
 * @post value は block 内で設定した値を保持。
 */
TEST(KetScopeTest, DismissPreventsRestore)
{
	int value = 3;

	{
		auto restore = ket::scope::MakeRestore(value);
		value = 9;
		restore.Dismiss();
		const auto restore_is_active = restore.Active();

		EXPECT_FALSE(restore_is_active);
	}

	EXPECT_EQ(value, 9);
}

/**
 * @test
 * @brief restore guard の move による二重復元防止確認。
 * @details Restore を move し、source 破棄では復元せず destination
 * 破棄時に1回だけ構築時の値へ戻ることを確認。
 * @pre C++17以降。
 * @post value は構築時の値へ復元。
 */
TEST(KetScopeTest, MoveTransfersRestoreResponsibility)
{
	int value = 3;

	{
		auto source = ket::scope::MakeRestore(value);
		value = 9;
		EXPECT_EQ(value, 9);
		auto destination = std::move(source);
		// cppcheck-suppress accessMoved
		// NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
		const auto source_is_active = source.Active();
		const auto destination_is_active = destination.Active();
		value = 12;

		EXPECT_FALSE(source_is_active);
		EXPECT_TRUE(destination_is_active);
	}

	EXPECT_EQ(value, 3);
}

/**
 * @test
 * @brief 早期 return 相当の値復元確認。
 * @details return で block を抜ける経路でも対象値が構築時の値へ戻り、戻り値は保持されることを確認。
 * @pre C++17以降。
 * @post value は構築時の値へ復元。
 */
TEST(KetScopeTest, RestoresValueDuringEarlyReturn)
{
	int value = 3;

	const auto result = ReturnAfterRestore(value);

	EXPECT_EQ(result, 7);
	EXPECT_EQ(value, 3);
}

/**
 * @test
 * @brief restore 代入例外の terminate 確認。
 * @details destructor 内の復元代入が例外を投げた場合、例外を外へ出さず process termination
 * になることを確認。
 * @pre C++17以降。GoogleTest death test が利用可能。
 * @post 親 process の外部状態の変更なし。
 */
TEST(KetScopeTest, TerminatesWhenRestoreAssignmentThrows)
{
	EXPECT_EXIT(
		{
			std::set_terminate(ExitFromTerminate);
			ThrowingAssignment value(1);
			const auto restore = ket::scope::MakeRestore(value);
			static_cast<void>(restore);
		},
		::testing::ExitedWithCode(kTerminateExitCode),
		"");
}
