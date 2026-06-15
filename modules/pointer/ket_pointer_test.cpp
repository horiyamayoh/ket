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

	class OverloadedAddress
	{
	  public:
		explicit OverloadedAddress(int value) noexcept : value_(value) {}

		OverloadedAddress* operator&() noexcept
		{
			return nullptr;
		}

		const OverloadedAddress* operator&() const noexcept
		{
			return nullptr;
		}

		[[nodiscard]]
		int Value() const noexcept
		{
			return value_;
		}

	  private:
		int value_;
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

	EXPECT_THROW(static_cast<void>(ket::pointer::NotNull<int>{ptr}), std::invalid_argument);
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
	EXPECT_EQ(raw, &value);

	*ptr = 7;

	EXPECT_EQ(value, 7);
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
 * @post ownerとweakの状態変更なし。
 */
TEST(KetPointerTest, LocksAliveWeakPointer)
{
	const auto owner = std::make_shared<int>(42);
	const std::weak_ptr<int> weak = owner;

	const auto locked = ket::pointer::LockWeak(weak);

	EXPECT_EQ(locked.get(), owner.get());
	EXPECT_EQ(*locked, 42);
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

	EXPECT_EQ(locked.get(), nullptr);
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
	OverloadedAddress value(42);

	auto* const overloaded_address = &value;
	auto* const actual_address = ket::pointer::AddressOf(value);
	const auto actual_value = actual_address->Value();

	EXPECT_EQ(overloaded_address, nullptr);
	EXPECT_NE(actual_address, overloaded_address);
	EXPECT_EQ(actual_value, 42);
}
