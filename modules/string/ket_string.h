#pragma once

/**
 * @file ket_string.h
 * @brief 複数文字列片の連結と追記API。
 *
 * @details formatではない文字列片の連結と既存文字列への追記を短いAPIへ集約する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。標準ライブラリのstring append処理を
 * 薄く包み、入力順、NUL保持、自己参照時の安全性を明示する。
 *
 * @par プロジェクトへの適用方法
 * `ket_string.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view`を利用でき、文字列片連結を標準ライブラリのみで安全に薄く包める。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::string
 * 内部実装：ket::string::detail
 */

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace ket
{
	namespace string
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 複数文字列片の連結。
		 * @param[in] parts 連結対象の文字列片。
		 * @retval value 連結後の文字列。
		 * @pre 各要素はstd::string_viewに変換可能な文字列片、またはchar。raw C
		 * stringは非nullかつnull終端。
		 * @post 引数と外部状態の変更なし。出力文字列は入力順と入力内容を保持。
		 * @note 数値、enum、任意stream変換は対象外。必要な変換は呼び出し側で明示。
		 * @note 合計長がstd::string::max_size()を超える場合はstd::length_error送出。
		 * @note std::stringのreserveとappendがallocationを伴うためnoexceptなし。
		 * @code
		 * const auto text = ket::string::Cat("id=", id_text, ", mode=", mode_text);
		 * // text == "id=42, mode=run"
		 * @endcode
		 */
		template <typename... Parts>
		std::string Cat(const Parts&... parts);

		/**
		 * @brief 複数文字列片の末尾追記。
		 * @param[in,out] destination 追記先の文字列。
		 * @param[in] parts 追記対象の文字列片。
		 * @retval void 戻り値なし。
		 * @pre destinationは有効なstd::string。各要素はstd::string_viewへ変換可能、またはchar。
		 * raw C stringは非nullかつnull終端。
		 * @post `destination`の既存内容を保持し、末尾に`parts`の連結結果を追加。
		 * @note `destination`自身やそのviewをpartsに含む場合は、suffixを生成してから追記。
		 * @note 合計長がstd::string::max_size()を超える場合はstd::length_error送出。
		 * @note std::stringのreserve、生成、appendがallocationを伴うためnoexceptなし。
		 * @code
		 * std::string text = "id=";
		 * ket::string::Append(text, id_text, ", mode=", mode_text);
		 * // text == "id=42, mode=run"
		 * @endcode
		 */
		template <typename... Parts>
		void Append(std::string& destination, const Parts&... parts);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief charとして扱う文字列片の型判定。
			 * @tparam Part 判定対象の型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Part>
			inline constexpr bool kIsCharStringPart = std::is_same_v<std::decay_t<Part>, char>;

			/**
			 * @brief std::string_viewとして扱う文字列片の型判定。
			 * @tparam Part 判定対象の型。
			 * @note nullptrはstd::string_view変換候補から除外。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Part>
			inline constexpr bool kIsStringViewStringPart =
				!std::is_same_v<std::decay_t<Part>, std::nullptr_t> &&
				std::is_convertible_v<Part, std::string_view>;

			/**
			 * @brief StrCatとStrAppendが受け付ける文字列片の型判定。
			 * @tparam Part 判定対象の型。
			 * @note char、またはstd::string_viewに変換可能な型を受け付ける。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Part>
			inline constexpr bool kIsStringPart =
				kIsCharStringPart<Part> || kIsStringViewStringPart<Part>;

			/**
			 * @brief 文字列片の長さ取得。
			 * @param[in] part 長さ取得対象の文字列片。
			 * @retval value `part`のバイト数。
			 * @pre `part`は有効なstd::string_view。
			 * @post 引数と外部状態の変更なし。
			 */
			inline std::size_t StringPartSize(std::string_view part) noexcept
			{
				return part.size();
			}

			/**
			 * @brief 1文字の文字列片長さ取得。
			 * @param[in] part 長さ取得対象の文字。
			 * @retval value 常に1。
			 * @pre なし。任意のchar値を1文字の文字列片として扱う。
			 * @post 引数と外部状態の変更なし。
			 */
			constexpr std::size_t StringPartSize(char part) noexcept
			{
				static_cast<void>(part);
				return 1U;
			}

			/**
			 * @brief 文字列片の合計長加算。
			 * @param[in] total_size 加算前の合計バイト数。
			 * @param[in] part_size 加算対象のバイト数。
			 * @param[in] max_size 許容する最大合計バイト数。
			 * @retval value 加算後の合計バイト数。
			 * @pre `total_size`は`max_size`以下。
			 * @post 引数と外部状態の変更なし。`max_size`を超える場合はstd::length_error送出。
			 */
			inline std::size_t
			AddPartSize(std::size_t total_size, std::size_t part_size, std::size_t max_size)
			{
				const auto size_would_exceed = part_size > (max_size - total_size);
				if (size_would_exceed)
				{
					throw std::length_error("ket string result exceeds std::string::max_size().");
				}

				return total_size + part_size;
			}

			/**
			 * @brief 文字列片の合計長累積終端。
			 * @param[in] total_size 累積済みの合計バイト数。
			 * @param[in] max_size 許容する最大合計バイト数。
			 * @retval value 累積済みの合計バイト数。
			 * @pre `total_size`は`max_size`以下。
			 * @post 引数と外部状態の変更なし。
			 */
			inline std::size_t AccumulateStringPartSize(std::size_t total_size,
														std::size_t max_size) noexcept
			{
				static_cast<void>(max_size);
				return total_size;
			}

			/**
			 * @brief 文字列片の合計長累積。
			 * @param[in] total_size 累積済みの合計バイト数。
			 * @param[in] max_size 許容する最大合計バイト数。
			 * @param[in] first 次に加算する文字列片。
			 * @param[in] rest 後続の文字列片。
			 * @retval value 全文字列片の合計バイト数。
			 * @pre `total_size`は`max_size`以下。各要素はstd::string_viewへ変換可能、またはchar。
			 * @post 引数と外部状態の変更なし。`max_size`を超える場合はstd::length_error送出。
			 */
			template <typename First, typename... Rest>
			std::size_t AccumulateStringPartSize(std::size_t total_size,
												 std::size_t max_size,
												 const First& first,
												 const Rest&... rest)
			{
				const auto first_size = StringPartSize(first);
				const auto next_size = AddPartSize(total_size, first_size, max_size);

				return AccumulateStringPartSize(next_size, max_size, rest...);
			}

			/**
			 * @brief 複数文字列片の合計長取得。
			 * @param[in] max_size 許容する最大合計バイト数。
			 * @param[in] parts 合計長取得対象の文字列片。
			 * @retval value 全文字列片の合計バイト数。
			 * @pre 各要素はstd::string_viewに変換可能な文字列片、またはchar。
			 * @post 引数と外部状態の変更なし。`max_size`を超える場合はstd::length_error送出。
			 */
			template <typename... Parts>
			std::size_t TotalStringPartSize(std::size_t max_size, const Parts&... parts)
			{
				return AccumulateStringPartSize(0U, max_size, parts...);
			}

			/**
			 * @brief 文字列片の追記。
			 * @param[in,out] destination 追記先の文字列。
			 * @param[in] part 追記対象の文字列片。
			 * @retval void 戻り値なし。
			 * @pre `destination`は有効なstd::stringオブジェクト。`part`は有効なstd::string_view。
			 * @post `destination`の末尾に`part`の内容を同じ順序で追加。
			 */
			inline void AppendStringPart(std::string& destination, std::string_view part)
			{
				const auto part_is_empty = part.empty();
				if (part_is_empty)
				{
					return;
				}

				destination.append(part.data(), part.size());
			}

			/**
			 * @brief 1文字の追記。
			 * @param[in,out] destination 追記先の文字列。
			 * @param[in] part 追記対象の文字。
			 * @retval void 戻り値なし。
			 * @pre `destination`は有効なstd::stringオブジェクト。
			 * @post `destination`の末尾に`part`を1文字として追加。
			 */
			inline void AppendStringPart(std::string& destination, char part)
			{
				destination.push_back(part);
			}

			/**
			 * @brief 複数文字列片の追記。
			 * @param[in,out] destination 追記先の文字列。
			 * @param[in] parts 追記対象の文字列片。
			 * @retval void 戻り値なし。
			 * @pre destinationは有効なstd::string。各要素はstd::string_viewへ変換可能、またはchar。
			 * @post `destination`の末尾に`parts`の内容を同じ順序で追加。
			 */
			template <typename... Parts>
			void AppendStringParts(std::string& destination, const Parts&... parts)
			{
				(AppendStringPart(destination, parts), ...);
			}

			/**
			 * @brief 2つの文字列範囲の重なり判定。
			 * @param[in] first_data 1つ目の範囲先頭。
			 * @param[in] first_size 1つ目の範囲バイト数。
			 * @param[in] second_data 2つ目の範囲先頭。
			 * @param[in] second_size 2つ目の範囲バイト数。
			 * @retval true 2つの非空範囲に重なりあり。
			 * @retval false いずれかが空、または範囲に重なりなし。
			 * @pre 非空範囲では各dataがsizeバイト以上読み取り可能な領域を指す。
			 * @post 引数と外部状態の変更なし。
			 */
			inline bool RangesOverlap(const char* first_data,
									  std::size_t first_size,
									  const char* second_data,
									  std::size_t second_size) noexcept
			{
				const auto first_is_empty = first_size == 0U;
				const auto second_is_empty = second_size == 0U;
				if (first_is_empty || second_is_empty)
				{
					return false;
				}

				const char* const first_end = first_data + first_size;
				const char* const second_end = second_data + second_size;
				const auto less = std::less<>();
				const auto first_starts_before_second_ends = less(first_data, second_end);
				const auto second_starts_before_first_ends = less(second_data, first_end);

				return first_starts_before_second_ends && second_starts_before_first_ends;
			}

			/**
			 * @brief 文字列片と追記先の重なり判定。
			 * @param[in] destination 追記先の文字列。
			 * @param[in] part 判定対象の文字列片。
			 * @retval true `part`の範囲が`destination`の現在範囲と重なる。
			 * @retval false `part`の範囲が`destination`の現在範囲と重ならない。
			 * @pre `destination`は有効なstd::string。`part`は有効なstd::string_view。
			 * @post 引数と外部状態の変更なし。
			 */
			inline bool PartAliasesDestination(const std::string& destination,
											   std::string_view part) noexcept
			{
				return RangesOverlap(
					destination.data(), destination.size(), part.data(), part.size());
			}

			/**
			 * @brief 1文字片と追記先の重なり判定。
			 * @param[in] destination 追記先の文字列。
			 * @param[in] part 判定対象の文字。
			 * @retval false charは単一値として受け取るため重なりなし。
			 * @pre なし。任意のchar値を1文字の文字列片として扱う。
			 * @post 引数と外部状態の変更なし。
			 */
			inline bool PartAliasesDestination(const std::string& destination, char part) noexcept
			{
				static_cast<void>(destination);
				static_cast<void>(part);
				return false;
			}

			/**
			 * @brief 複数文字列片と追記先の重なり判定。
			 * @param[in] destination 追記先の文字列。
			 * @param[in] parts 判定対象の文字列片。
			 * @retval true いずれかの文字列片が`destination`の現在範囲と重なる。
			 * @retval false すべての文字列片が`destination`の現在範囲と重ならない。
			 * @pre
			 * `destination`は有効なstd::string。各要素はstd::string_viewへ変換可能、またはchar。
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename... Parts>
			bool AnyPartAliasesDestination(const std::string& destination, const Parts&... parts)
			{
				return (false || ... || PartAliasesDestination(destination, parts));
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename... Parts>
		std::string Cat(const Parts&... parts)
		{
			static_assert(
				(detail::kIsStringPart<Parts> && ...),
				"ket::string::Cat accepts char and types convertible to std::string_view only.");

			std::string result;

			const auto max_size = result.max_size();
			const auto total_size = detail::TotalStringPartSize(max_size, parts...);
			result.reserve(total_size);
			detail::AppendStringParts(result, parts...);

			return result;
		}

		template <typename... Parts>
		void Append(std::string& destination, const Parts&... parts)
		{
			static_assert(
				(detail::kIsStringPart<Parts> && ...),
				"ket::string::Append accepts char and types convertible to std::string_view only.");

			const auto destination_size = destination.size();
			const auto max_append_size = destination.max_size() - destination_size;
			const auto append_size = detail::TotalStringPartSize(max_append_size, parts...);
			const auto append_is_empty = append_size == 0U;
			if (append_is_empty)
			{
				return;
			}

			const auto has_alias = detail::AnyPartAliasesDestination(destination, parts...);
			if (has_alias)
			{
				const auto suffix = Cat(parts...);
				destination.append(suffix.data(), suffix.size());
				return;
			}

			destination.reserve(destination_size + append_size);
			detail::AppendStringParts(destination, parts...);
		}

	} // namespace string

} // namespace ket
