#include "ket_cache.h"

#include <memory>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

namespace
{
	class DestructionCounter
	{
	  public:
		explicit DestructionCounter(int* destroy_count) noexcept : destroy_count_(destroy_count) {}

		~DestructionCounter() noexcept
		{
			++(*destroy_count_);
		}

		DestructionCounter(const DestructionCounter&) = delete;
		DestructionCounter& operator=(const DestructionCounter&) = delete;

	  private:
		int* destroy_count_;
	};

	class ThrowingDestructor
	{
	  public:
		explicit ThrowingDestructor(int value) noexcept : value_(value) {}

		~ThrowingDestructor() noexcept(false)
		{
			// cppcheck-suppress exceptThrowInDestructor
			throw std::runtime_error("destructor failure");
		}

		[[nodiscard]] int Value() const noexcept
		{
			return value_;
		}

	  private:
		int value_;
	};

	static_assert(!std::is_copy_constructible_v<ket::cache::Lazy<int>>,
				  "ket::cache::Lazy is not copy constructible.");
	static_assert(!std::is_copy_assignable_v<ket::cache::Lazy<int>>,
				  "ket::cache::Lazy is not copy assignable.");
	static_assert(!std::is_move_constructible_v<ket::cache::Lazy<int>>,
				  "ket::cache::Lazy is not move constructible.");
	static_assert(!std::is_move_assignable_v<ket::cache::Lazy<int>>,
				  "ket::cache::Lazy is not move assignable.");

} // namespace

/**
 * @test
 * @brief default構築時の空状態確認。
 * @details `Lazy<int>`をdefault構築し、保持値を持たないことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetCacheTest, DefaultConstructsEmpty)
{
	const ket::cache::Lazy<int> value;

	const auto has_value = value.HasValue();

	EXPECT_EQ(has_value, false);
}

/**
 * @test
 * @brief factory呼び出し回数と保持値再利用の確認。
 * @details 2回のGetOrCreateを呼び出し、factoryが1回だけ実行され同じ保持値参照を返すことを確認。
 * @pre C++17以降。
 * @post `value`は保持値あり。外部状態はfactory呼び出し回数だけ変化。
 */
TEST(KetCacheTest, CreatesValueOnlyOnce)
{
	ket::cache::Lazy<int> value;
	int factory_count = 0;

	int& first = value.GetOrCreate(
		[&factory_count]
		{
			++factory_count;
			return 42;
		});
	int& second = value.GetOrCreate(
		[&factory_count]
		{
			++factory_count;
			return 7;
		});

	const auto has_value = value.HasValue();
	const auto same_address = &first == &second;

	EXPECT_EQ(has_value, true);
	EXPECT_EQ(factory_count, 1);
	EXPECT_EQ(first, 42);
	EXPECT_EQ(second, 42);
	EXPECT_EQ(same_address, true);
}

/**
 * @test
 * @brief Reset後の破棄と再生成確認。
 * @details Resetで保持値を破棄してemptyへ戻し、次のGetOrCreateで新しい値を生成することを確認。
 * @pre C++17以降。
 * @post `value`は再生成後の保持値あり。destroy_countはResetで1増加。
 */
TEST(KetCacheTest, ResetsAndRecreatesValue)
{
	int factory_count = 0;
	int destroy_count = 0;
	ket::cache::Lazy<DestructionCounter> value;

	const DestructionCounter& first = value.GetOrCreate(
		[&factory_count, &destroy_count]
		{
			++factory_count;
			return &destroy_count;
		});
	static_cast<void>(first);

	const auto has_value_before_reset = value.HasValue();
	value.Reset();
	const auto has_value_after_reset = value.HasValue();

	const DestructionCounter& second = value.GetOrCreate(
		[&factory_count, &destroy_count]
		{
			++factory_count;
			return &destroy_count;
		});
	static_cast<void>(second);

	const auto has_value_after_recreate = value.HasValue();

	EXPECT_EQ(has_value_before_reset, true);
	EXPECT_EQ(has_value_after_reset, false);
	EXPECT_EQ(has_value_after_recreate, true);
	EXPECT_EQ(factory_count, 2);
	EXPECT_EQ(destroy_count, 1);
}

/**
 * @test
 * @brief destructorによる保持値破棄確認。
 * @details 保持値ありのLazyをscope exitさせ、保持値destructorが1回呼び出されることを確認。
 * @pre C++17以降。
 * @post destroy_countはscope exitで1増加。
 */
TEST(KetCacheTest, DestroysValueOnLazyDestruction)
{
	int destroy_count = 0;

	{
		ket::cache::Lazy<DestructionCounter> value;
		const DestructionCounter& created = value.GetOrCreate(
			[&destroy_count]
			{
				return &destroy_count;
			});
		static_cast<void>(created);
	}

	EXPECT_EQ(destroy_count, 1);
}

/**
 * @test
 * @brief factory例外後のempty維持確認。
 * @details factory例外を伝播し、例外後もemptyのまま再生成できることを確認。
 * @pre C++17以降。
 * @post `value`は再生成後の保持値あり。失敗factoryの呼び出し回数は1。
 */
TEST(KetCacheTest, StaysEmptyAfterFactoryException)
{
	ket::cache::Lazy<int> value;
	int throwing_factory_count = 0;

	EXPECT_THROW(static_cast<void>(value.GetOrCreate(
					 [&throwing_factory_count]() -> int
					 {
						 ++throwing_factory_count;
						 throw std::runtime_error("factory failure");
					 })),
				 std::runtime_error);

	const auto has_value_after_exception = value.HasValue();
	const int& created = value.GetOrCreate(
		[]
		{
			return 99;
		});
	const auto has_value_after_success = value.HasValue();

	EXPECT_EQ(throwing_factory_count, 1);
	EXPECT_EQ(has_value_after_exception, false);
	EXPECT_EQ(has_value_after_success, true);
	EXPECT_EQ(created, 99);
}

/**
 * @test
 * @brief move-only値の保持確認。
 * @details std::unique_ptrをfactoryから返し、copy不可の値を保持できることを確認。
 * @pre C++17以降。
 * @post `value`はmove-only値を保持。外部状態の変更なし。
 */
TEST(KetCacheTest, StoresMoveOnlyValue)
{
	ket::cache::Lazy<std::unique_ptr<int>> value;

	const std::unique_ptr<int>& created = value.GetOrCreate(
		[]
		{
			return std::make_unique<int>(42);
		});

	const auto has_value = value.HasValue();

	ASSERT_NE(created, nullptr);
	EXPECT_EQ(has_value, true);
	EXPECT_EQ(*created, 42);
}

/**
 * @test
 * @brief 保持値address stability確認。
 * @details Resetしない限り複数回のGetOrCreateが同じ保持値addressを返すことを確認。
 * @pre C++17以降。
 * @post `value`は保持値あり。factoryは初回のみ実行。
 */
TEST(KetCacheTest, KeepsValueAddressStableUntilReset)
{
	ket::cache::Lazy<int> value;
	int factory_count = 0;

	const int& first = value.GetOrCreate(
		[&factory_count]
		{
			++factory_count;
			return 5;
		});
	const int* const first_address = &first;

	const int& second = value.GetOrCreate(
		[&factory_count]
		{
			++factory_count;
			return 6;
		});
	const int* const second_address = &second;

	const auto same_address = first_address == second_address;

	EXPECT_EQ(factory_count, 1);
	EXPECT_EQ(same_address, true);
}

/**
 * @test
 * @brief 保持値destructor例外時のterminate確認。
 * @details destructorが例外を投げる保持値をscope exitで破棄し、process
 * terminationになることを確認。
 * @pre C++17以降。death testを実行できるGoogleTest環境。
 * @post death testの子processだけ終了。親processの外部状態変更なし。
 */
TEST(KetCacheTest, TerminatesWhenStoredValueDestructorThrows)
{
	EXPECT_DEATH(
		{
			ket::cache::Lazy<ThrowingDestructor> value;
			const ThrowingDestructor& created = value.GetOrCreate(
				[]
				{
					return 7;
				});
			const auto stored_value = created.Value();
			static_cast<void>(stored_value);
		},
		".*");
}
