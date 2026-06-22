#include "ket_state.h"

#include <array>
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

	constexpr MachineTransition kCompileTimeTable[] = {
		{MachineState::kIdle, MachineEvent::kStart, MachineState::kRunning},
		{MachineState::kRunning, MachineEvent::kPause, MachineState::kPaused},
	};
	static_assert(ket::state::IsAllowed(MachineState::kIdle,
										MachineEvent::kStart,
										kCompileTimeTable),
				  "ket::state::IsAllowed supports constexpr C array lookup");

	constexpr auto kCompileTimeNext =
		ket::state::Next(MachineState::kRunning, MachineEvent::kPause, kCompileTimeTable);
	static_assert(kCompileTimeNext.has_value(), "ket::state::Next finds a constexpr transition");
	static_assert(*kCompileTimeNext == MachineState::kPaused,
				  "ket::state::Next returns the constexpr target state");

	constexpr std::array<MachineTransition, 0U> kCompileTimeEmptyTable{};
	static_assert(!ket::state::IsAllowed(MachineState::kIdle,
										 MachineEvent::kStart,
										 kCompileTimeEmptyTable),
				  "ket::state::IsAllowed rejects an empty constexpr std::array table");

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

/**
 * @test
 * @brief 末尾行にある遷移のlookup確認。
 * @details 一致行を表の末尾だけに置き、線形走査が最後の行まで到達することを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStateTest, FindsLastRowTransition)
{
	const MachineTransition table[] = {
		{MachineState::kIdle, MachineEvent::kStop, MachineState::kStopped},
		{MachineState::kRunning, MachineEvent::kPause, MachineState::kPaused},
		{MachineState::kPaused, MachineEvent::kReset, MachineState::kIdle},
	};

	const auto allowed = ket::state::IsAllowed(MachineState::kPaused, MachineEvent::kReset, table);
	const auto next = ket::state::Next(MachineState::kPaused, MachineEvent::kReset, table);

	EXPECT_TRUE(allowed);
	EXPECT_EQ(next, std::optional<MachineState>(MachineState::kIdle));
}

/**
 * @test
 * @brief 表にないenum値の遷移失敗確認。
 * @details 表に含まれないenum classの状態とイベントを入力し、falseとstd::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStateTest, RejectsUnlistedEnumValues)
{
	const MachineTransition table[] = {
		{MachineState::kIdle, MachineEvent::kStart, MachineState::kRunning},
		{MachineState::kRunning, MachineEvent::kStop, MachineState::kStopped},
	};
	const auto unlisted_state = MachineState::kError;
	const auto unlisted_event = MachineEvent::kReset;

	const auto state_allowed = ket::state::IsAllowed(unlisted_state, MachineEvent::kStart, table);
	const auto state_next = ket::state::Next(unlisted_state, MachineEvent::kStart, table);
	const auto event_allowed = ket::state::IsAllowed(MachineState::kIdle, unlisted_event, table);
	const auto event_next = ket::state::Next(MachineState::kIdle, unlisted_event, table);

	EXPECT_FALSE(state_allowed);
	EXPECT_EQ(state_next, std::nullopt);
	EXPECT_FALSE(event_allowed);
	EXPECT_EQ(event_next, std::nullopt);
}

/**
 * @test
 * @brief scalar型の状態とイベントのlookup確認。
 * @details int状態とcharイベントの表を入力し、template APIがenum class専用ではないことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStateTest, SupportsScalarStateAndEventValues)
{
	using IntTransition = ket::state::Transition<int, char>;
	const IntTransition table[] = {
		{1, 'a', 2},
		{2, 'b', 3},
	};

	const auto allowed = ket::state::IsAllowed(2, 'b', table);
	const auto next = ket::state::Next(2, 'b', table);
	const auto missing_allowed = ket::state::IsAllowed(3, 'b', table);
	const auto missing_next = ket::state::Next(3, 'b', table);

	EXPECT_TRUE(allowed);
	EXPECT_EQ(next, std::optional<int>(3));
	EXPECT_FALSE(missing_allowed);
	EXPECT_EQ(missing_next, std::nullopt);
}

/**
 * @test
 * @brief 空std::array遷移表の失敗値確認。
 * @details 0行のstd::array表を入力し、falseとstd::nulloptを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStateTest, RejectsEmptyStdArrayTable)
{
	const std::array<MachineTransition, 0U> table{};

	const auto allowed = ket::state::IsAllowed(MachineState::kIdle, MachineEvent::kStart, table);
	const auto next = ket::state::Next(MachineState::kIdle, MachineEvent::kStart, table);

	EXPECT_FALSE(allowed);
	EXPECT_EQ(next, std::nullopt);
}

/**
 * @test
 * @brief Doxygen例のstd::array lookup確認。
 * @details Doxygen例と同じ形のstd::array表を入力し、許可判定と次状態取得を確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetStateTest, MatchesDocumentedArrayExamples)
{
	enum class Mode : std::uint8_t
	{
		kIdle,
		kRunning,
	};
	enum class Event : std::uint8_t
	{
		kStart,
	};
	const std::array<ket::state::Transition<Mode, Event>, 1U> table = {{
		{Mode::kIdle, Event::kStart, Mode::kRunning},
	}};

	const auto allowed = ket::state::IsAllowed(Mode::kIdle, Event::kStart, table);
	const auto next = ket::state::Next(Mode::kIdle, Event::kStart, table);

	EXPECT_TRUE(allowed);
	EXPECT_EQ(next, std::optional<Mode>(Mode::kRunning));
}
