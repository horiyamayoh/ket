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

	class ThrowingConstruction
	{
	  public:
		explicit ThrowingConstruction(int value)
		{
			static_cast<void>(value);
			throw std::runtime_error("construction failure");
		}
	};

	class ThrowingTemporary
	{
	  public:
		ThrowingTemporary(int* construct_count,
						  int* value_destroy_count,
						  int* temporary_destroy_count) noexcept
			: construct_count_(construct_count), value_destroy_count_(value_destroy_count),
			  temporary_destroy_count_(temporary_destroy_count)
		{
		}

		ThrowingTemporary(const ThrowingTemporary&) = delete;
		ThrowingTemporary& operator=(const ThrowingTemporary&) = delete;
		ThrowingTemporary(ThrowingTemporary&&) = delete;
		ThrowingTemporary& operator=(ThrowingTemporary&&) = delete;

		~ThrowingTemporary() noexcept(false)
		{
			++(*temporary_destroy_count_);
			// cppcheck-suppress exceptThrowInDestructor
			throw std::runtime_error("temporary destructor failure");
		}

		[[nodiscard]]
		int* ConstructCount() const noexcept
		{
			return construct_count_;
		}

		[[nodiscard]]
		int* ValueDestroyCount() const noexcept
		{
			return value_destroy_count_;
		}

	  private:
		int* construct_count_;
		int* value_destroy_count_;
		int* temporary_destroy_count_;
	};

	class ValueFromThrowingTemporary
	{
	  public:
		explicit ValueFromThrowingTemporary(const ThrowingTemporary& source) noexcept
			: destroy_count_(source.ValueDestroyCount())
		{
			++(*source.ConstructCount());
		}

		~ValueFromThrowingTemporary() noexcept
		{
			++(*destroy_count_);
		}

		ValueFromThrowingTemporary(const ValueFromThrowingTemporary&) = delete;
		ValueFromThrowingTemporary& operator=(const ValueFromThrowingTemporary&) = delete;

	  private:
		int* destroy_count_;
	};

	class MoveOnlyIntFactory
	{
	  public:
		explicit MoveOnlyIntFactory(int* call_count) noexcept : call_count_(call_count) {}

		MoveOnlyIntFactory(const MoveOnlyIntFactory&) = delete;
		MoveOnlyIntFactory& operator=(const MoveOnlyIntFactory&) = delete;

		MoveOnlyIntFactory(MoveOnlyIntFactory&&) = delete;
		MoveOnlyIntFactory& operator=(MoveOnlyIntFactory&&) = delete;

		int operator()() const noexcept
		{
			++(*call_count_);
			return 123;
		}

	  private:
		int* call_count_;
	};

	void CreateLazyWithThrowingDestructor()
	{
		ket::cache::Lazy<ThrowingDestructor> value;
		const ThrowingDestructor& created = value.GetOrCreate(
			[]
			{
				return 7;
			});
		const auto stored_value = created.Value();
		static_cast<void>(stored_value);
	}

	void ReenterDuringFactory()
	{
		ket::cache::Lazy<int> value;

		const auto created = value.GetOrCreate(
			[&value]
			{
				return value.GetOrCreate(
					[]
					{
						return 1;
					});
			});
		static_cast<void>(created);
	}

	void ResetDuringFactory()
	{
		ket::cache::Lazy<int> value;

		const auto created = value.GetOrCreate(
			[&value]
			{
				value.Reset();
				return 1;
			});
		static_cast<void>(created);
	}

	class ReentrantDestructor
	{
	  public:
		ReentrantDestructor(ket::cache::Lazy<ReentrantDestructor>* owner,
							bool get_or_create) noexcept
			: owner_(owner), get_or_create_(get_or_create)
		{
		}

		~ReentrantDestructor() noexcept // NOLINT(misc-no-recursion)
		{
			if (get_or_create_)
			{
				// cppcheck-suppress throwInNoexceptFunction
				const ReentrantDestructor& created = owner_->GetOrCreate(
					[this]
					{
						return ReentrantDestructor(owner_, false);
					});
				static_cast<void>(created);
				return;
			}

			owner_->Reset();
		}

		ReentrantDestructor(const ReentrantDestructor&) = delete;
		ReentrantDestructor& operator=(const ReentrantDestructor&) = delete;

	  private:
		ket::cache::Lazy<ReentrantDestructor>* owner_;
		bool get_or_create_;
	};

	void ReenterDuringReset()
	{
		ket::cache::Lazy<ReentrantDestructor> value;
		const ReentrantDestructor& created = value.GetOrCreate(
			[&value]
			{
				return ReentrantDestructor(&value, false);
			});
		static_cast<void>(created);

		value.Reset();
	}

	void GetOrCreateDuringReset()
	{
		ket::cache::Lazy<ReentrantDestructor> value;
		const ReentrantDestructor& created = value.GetOrCreate(
			[&value]
			{
				return ReentrantDestructor(&value, true);
			});
		static_cast<void>(created);

		value.Reset();
	}

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
 * @brief empty状態のReset no-op確認。
 * @details 保持値なしのLazyへResetを呼び出してもemptyのままで、以後の生成に影響しないことを確認。
 * @pre C++17以降。
 * @post `value`は生成後の保持値あり。Reset前のfactory呼び出しなし。
 */
TEST(KetCacheTest, ResetEmptyDoesNotCreateValue)
{
	ket::cache::Lazy<int> value;
	int factory_count = 0;

	value.Reset();
	const auto has_value_after_reset = value.HasValue();

	const int& created = value.GetOrCreate(
		[&factory_count]
		{
			++factory_count;
			return 31;
		});
	const auto has_value_after_create = value.HasValue();

	EXPECT_EQ(has_value_after_reset, false);
	EXPECT_EQ(has_value_after_create, true);
	EXPECT_EQ(factory_count, 1);
	EXPECT_EQ(created, 31);
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

	const auto create_throwing = [&throwing_factory_count, &value]
	{
		static_cast<void>(value.GetOrCreate(
			[&throwing_factory_count]() -> int
			{
				++throwing_factory_count;
				throw std::runtime_error("factory failure");
			}));
	};

	EXPECT_THROW(create_throwing(), std::runtime_error);

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
 * @brief 保持値構築例外後のempty維持確認。
 * @details factoryは正常に値を返すがTの構築が例外を投げる場合、Lazyがemptyのまま残ることを確認。
 * @pre C++17以降。
 * @post `value`はemptyのまま。factoryは失敗した生成試行の回数だけ呼び出し済み。
 */
TEST(KetCacheTest, StaysEmptyAfterValueConstructionException)
{
	ket::cache::Lazy<ThrowingConstruction> value;
	int factory_count = 0;

	const auto create_throwing = [&factory_count, &value]
	{
		static_cast<void>(value.GetOrCreate(
			[&factory_count]
			{
				++factory_count;
				return 1;
			}));
	};

	EXPECT_THROW(create_throwing(), std::runtime_error);
	const auto has_value_after_first_exception = value.HasValue();

	EXPECT_THROW(create_throwing(), std::runtime_error);
	const auto has_value_after_second_exception = value.HasValue();

	EXPECT_EQ(factory_count, 2);
	EXPECT_EQ(has_value_after_first_exception, false);
	EXPECT_EQ(has_value_after_second_exception, false);
}

/**
 * @test
 * @brief 変換一時objectのdestructor例外後のempty維持確認。
 * @details placement new成功後にfactory戻り値の一時object破棄が例外を投げた場合、
 *          保持値を破棄してemptyへ戻すことを確認。
 * @pre C++17以降。
 * @post `value`はemptyのまま。構築済み保持値は1回破棄済み。
 */
TEST(KetCacheTest, DestroysConstructedValueWhenTemporaryDestructorThrows)
{
	ket::cache::Lazy<ValueFromThrowingTemporary> value;
	int construct_count = 0;
	int value_destroy_count = 0;
	int temporary_destroy_count = 0;

	const auto create_throwing =
		[&construct_count, &temporary_destroy_count, &value, &value_destroy_count]
	{
		static_cast<void>(value.GetOrCreate(
			[&construct_count, &temporary_destroy_count, &value_destroy_count]
			{
				return ThrowingTemporary(
					&construct_count, &value_destroy_count, &temporary_destroy_count);
			}));
	};

	EXPECT_THROW(create_throwing(), std::runtime_error);
	const auto has_value_after_exception = value.HasValue();

	EXPECT_EQ(construct_count, 1);
	EXPECT_EQ(value_destroy_count, 1);
	EXPECT_EQ(temporary_destroy_count, 1);
	EXPECT_EQ(has_value_after_exception, false);
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
 * @brief copy不可factoryのlvalue受け付け確認。
 * @details GetOrCreateがfactoryを不要copyせず、copy/move不可のlvalue
 * callableを必要時だけ呼び出すことを確認。
 * @pre C++17以降。
 * @post `value`は保持値あり。factory呼び出し回数は1。
 */
TEST(KetCacheTest, AcceptsMoveOnlyLvalueFactory)
{
	ket::cache::Lazy<int> value;
	int factory_count = 0;
	const MoveOnlyIntFactory factory(&factory_count);

	const int& first = value.GetOrCreate(factory);
	const int& second = value.GetOrCreate(factory);

	const auto same_address = &first == &second;

	EXPECT_EQ(factory_count, 1);
	EXPECT_EQ(first, 123);
	EXPECT_EQ(second, 123);
	EXPECT_EQ(same_address, true);
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
	EXPECT_DEATH(CreateLazyWithThrowingDestructor(), ".*");
}

/**
 * @test
 * @brief factory実行中の再入terminate確認。
 * @details GetOrCreateのfactory内から同じLazyへGetOrCreateした場合、process
 * terminationになることを確認。
 * @pre C++17以降。death testを実行できるGoogleTest環境。
 * @post death testの子processだけ終了。親processの外部状態変更なし。
 */
TEST(KetCacheTest, TerminatesWhenFactoryReentersSameLazy)
{
	EXPECT_DEATH(ReenterDuringFactory(), ".*");
}

/**
 * @test
 * @brief factory実行中のReset再入terminate確認。
 * @details GetOrCreateのfactory内から同じLazyへResetした場合、process
 * terminationになることを確認。
 * @pre C++17以降。death testを実行できるGoogleTest環境。
 * @post death testの子processだけ終了。親processの外部状態変更なし。
 */
TEST(KetCacheTest, TerminatesWhenFactoryResetsSameLazy)
{
	EXPECT_DEATH(ResetDuringFactory(), ".*");
}

/**
 * @test
 * @brief 保持値destructor中の再入terminate確認。
 * @details Resetで保持値を破棄している最中に同じLazyへResetした場合、process
 * terminationになることを確認。
 * @pre C++17以降。death testを実行できるGoogleTest環境。
 * @post death testの子processだけ終了。親processの外部状態変更なし。
 */
TEST(KetCacheTest, TerminatesWhenDestructorReentersSameLazy)
{
	EXPECT_DEATH(ReenterDuringReset(), ".*");
}

/**
 * @test
 * @brief 保持値destructor中のGetOrCreate再入terminate確認。
 * @details Resetで保持値を破棄している最中に同じLazyへGetOrCreateした場合、
 * process terminationになることを確認。
 * @pre C++17以降。death testを実行できるGoogleTest環境。
 * @post death testの子processだけ終了。親processの外部状態変更なし。
 */
TEST(KetCacheTest, TerminatesWhenDestructorCreatesSameLazy)
{
	EXPECT_DEATH(GetOrCreateDuringReset(), ".*");
}
