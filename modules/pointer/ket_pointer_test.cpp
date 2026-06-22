#include "ket_pointer.h"

#include <memory>
#include <stdexcept>

#include <gtest/gtest.h>

namespace
{
	struct ArrowTarget
	{
		explicit ArrowTarget(int value) noexcept : value_(value) {}

		[[nodiscard]]
		int Value() const noexcept
		{
			return value_;
		}

		void SetValue(int value) noexcept
		{
			value_ = value;
		}

	  private:
		int value_;
	};

	class BaseTarget
	{
	  public:
		explicit BaseTarget(int value) noexcept : value_(value) {}

		[[nodiscard]]
		int Value() const noexcept
		{
			return value_;
		}

	  private:
		int value_;
	};

	class DerivedTarget : public BaseTarget
	{
	  public:
		explicit DerivedTarget(int value) noexcept : BaseTarget(value) {}
	};

	class OverloadedAddress
	{
	  public:
		OverloadedAddress(int value, int& address_call_count) noexcept
			: value_(value), address_call_count_(&address_call_count)
		{
		}

		OverloadedAddress* operator&() noexcept
		{
			++(*address_call_count_);
			return nullptr;
		}

		const OverloadedAddress* operator&() const noexcept
		{
			++(*address_call_count_);
			return nullptr;
		}

		[[nodiscard]]
		int Value() const noexcept
		{
			return value_;
		}

	  private:
		int value_;
		int* address_call_count_;
	};

} // namespace

/**
 * @test
 * @brief NotNullのnullptr拒否確認。
 * @details nullptrを渡した構築がstd::invalid_argumentを送出し、null保持objectを作らないことを確認。
 * @pre C++17以降。
 * @post 外部状態の変更なし。
 */
TEST(KetPointerTest, RejectsNullPointer)
{
	int* const ptr = nullptr;
	const auto construct_null = [ptr]()
	{
		static_cast<void>(ket::pointer::NotNull<int>{ptr});
	};

	EXPECT_THROW(construct_null(), std::invalid_argument);
}

/**
 * @test
 * @brief NotNullのdereference確認。
 * @details 有効なint pointerを保持し、Getとoperator*が同じ参照先を返すことを確認。
 * @pre C++17以降。
 * @post valueはoperator*経由で更新された値を保持。
 */
TEST(KetPointerTest, DereferencesStoredPointer)
{
	int value = 42;
	const ket::pointer::NotNull<int> ptr(&value);

	auto* const raw = ptr.Get();
	auto* const expected_raw = &value;
	EXPECT_EQ(raw, expected_raw);

	*ptr = 7;

	EXPECT_EQ(value, 7);
}

/**
 * @test
 * @brief NotNullのconst参照先変換確認。
 * @details NotNull<int>からNotNull<const
 * int>へ暗黙変換し、同じ参照先をconst参照先として読めることを確認。
 * @pre C++17以降。
 * @post valueと外部状態の変更なし。
 */
TEST(KetPointerTest, ConvertsMutablePointeeToConstPointee)
{
	int value = 42;
	const ket::pointer::NotNull<int> mutable_ptr(&value);

	const ket::pointer::NotNull<const int> const_ptr = mutable_ptr;

	const auto* const raw = const_ptr.Get();
	const auto* const expected_raw = &value;
	const auto dereferenced = *const_ptr;

	EXPECT_EQ(raw, expected_raw);
	EXPECT_EQ(dereferenced, 42);
}

/**
 * @test
 * @brief NotNullのderived-to-base変換確認。
 * @details NotNull<Derived>からNotNull<Base>へ暗黙変換し、base
 * interfaceから同じobjectを読めることを確認。
 * @pre C++17以降。
 * @post objectと外部状態の変更なし。
 */
TEST(KetPointerTest, ConvertsDerivedPointeeToBasePointee)
{
	DerivedTarget target(42);
	const ket::pointer::NotNull<DerivedTarget> derived_ptr(&target);

	const ket::pointer::NotNull<BaseTarget> base_ptr = derived_ptr;

	auto* const raw = base_ptr.Get();
	BaseTarget* const expected_raw = &target;
	const auto value = base_ptr->Value();

	EXPECT_EQ(raw, expected_raw);
	EXPECT_EQ(value, 42);
}

/**
 * @test
 * @brief NotNullのoperator arrow確認。
 * @details member関数とmember変数をoperator->経由で使い、参照先objectに到達できることを確認。
 * @pre C++17以降。
 * @post targetはoperator->経由で更新された値を保持。
 */
TEST(KetPointerTest, AccessesMembersThroughArrow)
{
	ArrowTarget target(42);
	const ket::pointer::NotNull<ArrowTarget> ptr(&target);

	const auto initial_value = ptr->Value();
	EXPECT_EQ(initial_value, 42);

	ptr->SetValue(9);
	const auto updated_value = target.Value();

	EXPECT_EQ(updated_value, 9);
}

/**
 * @test
 * @brief LockWeakの有効weak pointer確認。
 * @details 所有中のshared pointerから作ったweak pointerをlockし、同じ参照先のshared
 * pointerを返すことを確認。
 * @pre C++17以降。
 * @post ownerとweak objectへの代入なし。lockedは参照先objectの共有所有権を持つ。
 */
TEST(KetPointerTest, LocksAliveWeakPointer)
{
	const auto owner = std::make_shared<int>(42);
	const std::weak_ptr<int> weak = owner;

	const auto locked = ket::pointer::LockWeak(weak);
	const auto* const locked_raw = locked.get();
	const auto* const owner_raw = owner.get();
	ASSERT_NE(locked_raw, nullptr);

	EXPECT_EQ(locked_raw, owner_raw);
	const auto locked_value = *locked;
	EXPECT_EQ(locked_value, 42);
}

/**
 * @test
 * @brief LockWeakのexpired確認。
 * @details 所有者を破棄したweak pointerをlockし、空shared pointerを返すことを確認。
 * @pre C++17以降。
 * @post weakはexpired状態を保持。
 */
TEST(KetPointerTest, ReturnsEmptySharedPointerForExpiredWeakPointer)
{
	std::weak_ptr<int> weak;
	{
		const auto owner = std::make_shared<int>(42);
		weak = owner;
	}

	const auto locked = ket::pointer::LockWeak(weak);
	const auto* const locked_raw = locked.get();

	EXPECT_EQ(locked_raw, nullptr);
}

/**
 * @test
 * @brief LockWeakの空weak pointer確認。
 * @details default constructed weak pointerをlockし、空shared pointerを返すことを確認。
 * @pre C++17以降。
 * @post weakは空状態を保持。
 */
TEST(KetPointerTest, ReturnsEmptySharedPointerForDefaultWeakPointer)
{
	const std::weak_ptr<int> weak;

	const auto locked = ket::pointer::LockWeak(weak);
	const auto* const locked_raw = locked.get();

	EXPECT_EQ(locked_raw, nullptr);
}

/**
 * @test
 * @brief AddressOfのoperator&回避確認。
 * @details operator&をoverloadした型に対し、overload結果ではなく実object addressを返すことを確認。
 * @pre C++17以降。
 * @post valueと外部状態の変更なし。
 */
TEST(KetPointerTest, BypassesOverloadedAddressOperator)
{
	int mutable_address_call_count = 0;
	OverloadedAddress value(42, mutable_address_call_count);

	auto* const overloaded_address = &value;
	const auto calls_after_overloaded_address = mutable_address_call_count;
	auto* const actual_address = ket::pointer::AddressOf(value);
	const auto calls_after_address_of = mutable_address_call_count;
	ASSERT_NE(actual_address, nullptr);
	const auto actual_value = actual_address->Value();

	EXPECT_EQ(overloaded_address, nullptr);
	EXPECT_NE(actual_address, overloaded_address);
	EXPECT_EQ(calls_after_overloaded_address, 1);
	EXPECT_EQ(calls_after_address_of, calls_after_overloaded_address);
	EXPECT_EQ(actual_value, 42);
}

/**
 * @test
 * @brief AddressOfのconst operator&回避確認。
 * @details const objectのoperator& overloadを呼ばず、const実object addressを返すことを確認。
 * @pre C++17以降。
 * @post valueと外部状態の変更なし。
 */
TEST(KetPointerTest, BypassesConstOverloadedAddressOperator)
{
	int address_call_count = 0;
	const OverloadedAddress value(42, address_call_count);

	const auto* const overloaded_address = &value;
	const auto calls_after_overloaded_address = address_call_count;
	const auto* const actual_address = ket::pointer::AddressOf(value);
	const auto calls_after_address_of = address_call_count;
	ASSERT_NE(actual_address, nullptr);
	const auto actual_value = actual_address->Value();

	EXPECT_EQ(overloaded_address, nullptr);
	EXPECT_NE(actual_address, overloaded_address);
	EXPECT_EQ(calls_after_overloaded_address, 1);
	EXPECT_EQ(calls_after_address_of, calls_after_overloaded_address);
	EXPECT_EQ(actual_value, 42);
}
