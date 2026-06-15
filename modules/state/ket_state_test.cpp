#include "ket_state.h"

#include <cstdint>
#include <optional>
#include <string> // NOLINT(misc-include-cleaner)
#include <type_traits>

#include <gtest/gtest.h>

namespace
{
	enum class MachineState : std::uint8_t
	{
		kIdle,
		kRunning,
		kPaused,
		kStopped,
		kError,
	};

	enum class MachineEvent : std::uint8_t
	{
		kStart,
		kPause,
		kStop,
		kReset,
	};

	using MachineTransition = ket::state::Transition<MachineState, MachineEvent>;

	static_assert(std::is_aggregate_v<MachineTransition>,
				  "ket::state::Transition remains aggregate-initializable");

} // namespace

/**
 * @test
 * @brief 定義済み遷移の許可判定と次状態取得確認。
 * @details enum classの状態とイベントを入力し、表に存在する遷移でtrueと次状態を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStateTest, FindsKnownEnumClassTransition)
{
	const MachineTransition table[] = {
		{MachineState::kIdle, MachineEvent::kStart, MachineState::kRunning},
		{MachineState::kRunning, MachineEvent::kPause, MachineState::kPaused},
		{MachineState::kPaused, MachineEvent::kStart, MachineState::kRunning},
	};

	const auto allowed = ket::state::IsAllowed(MachineState::kRunning, MachineEvent::kPause, table);
	const auto next = ket::state::Next(MachineState::kRunning, MachineEvent::kPause, table);

	EXPECT_TRUE(allowed);
	EXPECT_EQ(next, std::optional<MachineState>(MachineState::kPaused));
}

/**
 * @test
 * @brief 未定義遷移の失敗値確認。
 * @details 表にない状態とイベントの組み合わせを入力し、falseとstd::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStateTest, RejectsUnknownTransition)
{
	const MachineTransition table[] = {
		{MachineState::kIdle, MachineEvent::kStart, MachineState::kRunning},
		{MachineState::kRunning, MachineEvent::kStop, MachineState::kStopped},
	};

	const auto allowed = ket::state::IsAllowed(MachineState::kIdle, MachineEvent::kStop, table);
	const auto next = ket::state::Next(MachineState::kIdle, MachineEvent::kStop, table);

	EXPECT_FALSE(allowed);
	EXPECT_EQ(next, std::nullopt);
}

/**
 * @test
 * @brief duplicate遷移の先勝ち確認。
 * @details 同じ状態とイベントの行を複数持つ表を入力し、先頭一致行の次状態を返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStateTest, UsesFirstMatchingDuplicateTransition)
{
	const MachineTransition table[] = {
		{MachineState::kIdle, MachineEvent::kStart, MachineState::kRunning},
		{MachineState::kIdle, MachineEvent::kStart, MachineState::kError},
		{MachineState::kRunning, MachineEvent::kStop, MachineState::kStopped},
	};

	const auto allowed = ket::state::IsAllowed(MachineState::kIdle, MachineEvent::kStart, table);
	const auto next = ket::state::Next(MachineState::kIdle, MachineEvent::kStart, table);

	EXPECT_TRUE(allowed);
	EXPECT_EQ(next, std::optional<MachineState>(MachineState::kRunning));
}
