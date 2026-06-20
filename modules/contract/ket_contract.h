#pragma once

/**
 * @file ket_contract.h
 * @brief precondition、postcondition、invariantを明示する契約API。
 *
 * @details 契約違反時のprocess termination方針を小さいAPIへ閉じ込める。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。debug/releaseや
 * `NDEBUG`に連動せず、契約式は常時評価する。
 *
 * @par プロジェクトへの適用方法
 * `ket_contract.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：契約違反時のプロジェクト方針を小さいAPIへ閉じ込められる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：C++ contracts は標準化状況と利用可能性が安定していない。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::contract
 * 内部実装：ket::contract::detail
 */

#include <cstddef>
#include <exception>

namespace ket
{
	namespace contract
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 契約違反の種類。
		 */
		enum class Kind // NOLINT(performance-enum-size)
		{
			kExpects,
			kEnsures,
			kInvariant
		};

		/**
		 * @brief 契約違反としてprocess terminationを実行。
		 * @param[in] kind 契約違反の種類。
		 * @param[in] expression 文字列化済みの契約式。`nullptr`でもtermination方針は不変。
		 * @param[in] file 呼び出し元file名。`nullptr`でもtermination方針は不変。
		 * @param[in] line 呼び出し元line番号。
		 * @retval none 戻らない。
		 * @pre なし。診断文字列の生成や出力には依存しない。
		 * @post `std::terminate`を呼び出し、呼び出し元へ戻らない。
		 * @note `kind`、`expression`、`file`、`line`は将来の診断拡張用に受け取るが、
		 * 初回APIではhandler差し替えやloggingを提供しない。
		 * @code
		 * const auto kind = ket::contract::Kind::kExpects;
		 * ket::contract::Fail(kind, "count <= capacity", "file.cpp", 42);
		 * // 呼び出し元へ戻らない。
		 * @endcode
		 */
		[[noreturn]] inline void
		Fail(Kind kind, const char* expression, const char* file, int line) noexcept;

		/**
		 * @brief preconditionの成否確認。
		 * @param[in] condition preconditionの評価結果。
		 * @param[in] expression 文字列化済みの契約式。`nullptr`でも失敗時はterminate。
		 * @param[in] file 呼び出し元file名。`nullptr`でも失敗時はterminate。
		 * @param[in] line 呼び出し元line番号。
		 * @retval void 戻り値なし。
		 * @pre なし。`condition == false`は契約違反として扱う。
		 * @post `condition == true`なら外部状態の変更なし。`condition == false`なら戻らない。
		 * @code
		 * KET_EXPECTS(count <= capacity);
		 * @endcode
		 */
		inline void
		Expects(bool condition, const char* expression, const char* file, int line) noexcept;

		/**
		 * @brief postconditionの成否確認。
		 * @param[in] condition postconditionの評価結果。
		 * @param[in] expression 文字列化済みの契約式。`nullptr`でも失敗時はterminate。
		 * @param[in] file 呼び出し元file名。`nullptr`でも失敗時はterminate。
		 * @param[in] line 呼び出し元line番号。
		 * @retval void 戻り値なし。
		 * @pre なし。`condition == false`は契約違反として扱う。
		 * @post `condition == true`なら外部状態の変更なし。`condition == false`なら戻らない。
		 * @code
		 * KET_ENSURES(result != nullptr);
		 * @endcode
		 */
		inline void
		Ensures(bool condition, const char* expression, const char* file, int line) noexcept;

		/**
		 * @brief invariantの成否確認。
		 * @param[in] condition invariantの評価結果。
		 * @param[in] expression 文字列化済みの契約式。`nullptr`でも失敗時はterminate。
		 * @param[in] file 呼び出し元file名。`nullptr`でも失敗時はterminate。
		 * @param[in] line 呼び出し元line番号。
		 * @retval void 戻り値なし。
		 * @pre なし。`condition == false`は契約違反として扱う。
		 * @post `condition == true`なら外部状態の変更なし。`condition == false`なら戻らない。
		 * @code
		 * KET_ASSERT_INVARIANT(size_ <= capacity_);
		 * @endcode
		 */
		inline void AssertInvariant(bool condition,
									const char* expression,
									const char* file,
									int line) noexcept;

		/**
		 * @brief non-null pointerの取得。
		 * @tparam T pointerの指す型。
		 * @param[in] ptr 検査対象pointer。
		 * @param[in] expression 文字列化済みのpointer式。`nullptr`でも失敗時はterminate。
		 * @param[in] file 呼び出し元file名。`nullptr`でも失敗時はterminate。
		 * @param[in] line 呼び出し元line番号。
		 * @retval ptr `ptr != nullptr`の場合、入力と同じpointer値。
		 * @retval none `ptr == nullptr`の場合、戻らない。
		 * @pre `ptr`はraw pointer。smart pointerやoptional pointerのunwrapは呼び出し側の責務。
		 * @post 成功時は引数と外部状態の変更なし。失敗時は`std::terminate`を呼び出す。
		 * @code
		 * int* const value = KET_REQUIRE_NON_NULL(ptr);
		 * @endcode
		 */
		template <typename T>
		T* RequireNonNull(T* ptr, const char* expression, const char* file, int line) noexcept;

		/**
		 * @brief indexがsize境界内かを判定。
		 * @param[in] index 判定対象index。
		 * @param[in] size 有効範囲の要素数。
		 * @retval true `index < size`。
		 * @retval false `index >= size`。
		 * @pre なし。`size == 0`の場合は常にfalse。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * KET_EXPECTS(ket::contract::IsInBounds(index, size));
		 * @endcode
		 */
		inline bool IsInBounds(std::size_t index, std::size_t size) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief 契約違反kindの未使用抑止。
			 * @param[in] kind 契約違反の種類。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline void IgnoreKind(Kind kind) noexcept
			{
				static_cast<void>(kind);
			}

			/**
			 * @brief 契約違反位置情報の未使用抑止。
			 * @param[in] expression 文字列化済みの契約式。
			 * @param[in] file 呼び出し元file名。
			 * @param[in] line 呼び出し元line番号。
			 * @retval void 戻り値なし。
			 * @pre なし。`nullptr`は許容。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline void IgnoreLocation(const char* expression, const char* file, int line) noexcept
			{
				static_cast<void>(expression);
				static_cast<void>(file);
				static_cast<void>(line);
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		[[noreturn]] inline void
		Fail(Kind kind, const char* expression, const char* file, int line) noexcept
		{
			detail::IgnoreKind(kind);
			detail::IgnoreLocation(expression, file, line);
			std::terminate();
		}

		inline void
		Expects(bool condition, const char* expression, const char* file, int line) noexcept
		{
			const auto violated = !condition;
			if (violated)
			{
				Fail(Kind::kExpects, expression, file, line);
			}
		}

		inline void
		Ensures(bool condition, const char* expression, const char* file, int line) noexcept
		{
			const auto violated = !condition;
			if (violated)
			{
				Fail(Kind::kEnsures, expression, file, line);
			}
		}

		inline void
		AssertInvariant(bool condition, const char* expression, const char* file, int line) noexcept
		{
			const auto violated = !condition;
			if (violated)
			{
				Fail(Kind::kInvariant, expression, file, line);
			}
		}

		template <typename T>
		T* RequireNonNull(T* ptr, const char* expression, const char* file, int line) noexcept
		{
			const auto is_null = ptr == nullptr;
			if (is_null)
			{
				Fail(Kind::kExpects, expression, file, line);
			}

			return ptr;
		}

		inline bool IsInBounds(std::size_t index, std::size_t size) noexcept
		{
			return index < size;
		}

	} // namespace contract

} // namespace ket

/**
 * @brief preconditionを常時評価するstatement macro。
 * @param condition 評価対象のprecondition式。
 * @code
 * KET_EXPECTS(count <= capacity);
 * @endcode
 */
#define KET_EXPECTS(condition) \
	::ket::contract::Expects(!!(condition), #condition, __FILE__, __LINE__)

/**
 * @brief postconditionを常時評価するstatement macro。
 * @param condition 評価対象のpostcondition式。
 * @code
 * KET_ENSURES(result != nullptr);
 * @endcode
 */
#define KET_ENSURES(condition) \
	::ket::contract::Ensures(!!(condition), #condition, __FILE__, __LINE__)

/**
 * @brief invariantを常時評価するstatement macro。
 * @param condition 評価対象のinvariant式。
 * @code
 * KET_ASSERT_INVARIANT(size_ <= capacity_);
 * @endcode
 */
#define KET_ASSERT_INVARIANT(condition) \
	::ket::contract::AssertInvariant(!!(condition), #condition, __FILE__, __LINE__)

/**
 * @brief non-null pointerを要求し、成功時に同じpointer型で返すmacro。
 * @param ptr 評価対象のpointer式。
 * @code
 * int* const value = KET_REQUIRE_NON_NULL(ptr);
 * @endcode
 */
#define KET_REQUIRE_NON_NULL(ptr) ::ket::contract::RequireNonNull((ptr), #ptr, __FILE__, __LINE__)
