#pragma once

/**
 * @file ket_ascii.h
 * @brief ASCII前提の小さい文字列処理API。
 *
 * @details Unicode、locale、正規化を扱わないASCII限定のtrim、case変換、split、
 * prefix/suffix処理を集約する。drop-in時は宣言と実装を同じ単位で持ち出す。
 * viewを返すAPIは元文字列を参照するnon-owning viewを返し、lifetimeは呼び出し側が保持する。
 *
 * @par プロジェクトへの適用方法
 * `ket_ascii.h` と `ket_ascii.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view`でnon-owningな文字列処理を短く扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::ascii
 * 内部実装：ket::ascii::detail
 */

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ket
{
	namespace ascii
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 文字列が指定prefixで始まるかの判定。
		 * @param[in] text 判定対象の文字列view。
		 * @param[in] prefix 先頭一致を確認する文字列view。
		 * @retval true `text`が`prefix`で始まる。空prefixは常に一致。
		 * @retval false `text`が`prefix`で始まらない。
		 * @pre 非空viewのdataは、それぞれsizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。
		 * @note C++17での不足補完とASCII module内の命名統一が目的。
		 * @code
		 * const auto matched = ket::ascii::StartsWith("mode=auto", "mode=");
		 * // matched == true
		 * @endcode
		 */
		bool StartsWith(std::string_view text, std::string_view prefix) noexcept;

		/**
		 * @brief 文字列が指定suffixで終わるかの判定。
		 * @param[in] text 判定対象の文字列view。
		 * @param[in] suffix 末尾一致を確認する文字列view。
		 * @retval true `text`が`suffix`で終わる。空suffixは常に一致。
		 * @retval false `text`が`suffix`で終わらない。
		 * @pre 非空viewのdataは、それぞれsizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。
		 * @note C++17での不足補完とASCII module内の命名統一が目的。
		 * @code
		 * const auto matched = ket::ascii::EndsWith("path.txt", ".txt");
		 * // matched == true
		 * @endcode
		 */
		bool EndsWith(std::string_view text, std::string_view suffix) noexcept;

		/**
		 * @brief 文字列が指定needleを含むかの判定。
		 * @param[in] text 判定対象の文字列view。
		 * @param[in] needle 検索対象の文字列view。
		 * @retval true `text`が`needle`を含む。空needleは常に一致。
		 * @retval false `text`が`needle`を含まない。
		 * @pre 非空viewのdataは、それぞれsizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。
		 * @note byte列として比較し、Unicode正規化やcase変換は行わない。
		 * @code
		 * const auto matched = ket::ascii::Contains("alpha,beta", "ha,");
		 * // matched == true
		 * @endcode
		 */
		bool Contains(std::string_view text, std::string_view needle) noexcept;

		/**
		 * @brief 文字列両端のASCII whitespace除去。
		 * @param[in] text trim対象の文字列view。
		 * @retval value 両端のASCII whitespaceを除いたnon-owning view。
		 * @pre 非空viewのdataは、sizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。戻りviewは`text`と同じ元文字列を参照。
		 * @note ASCII whitespaceはspace、tab、CR、LF、FF、VTのみ。
		 * @code
		 * const auto value = ket::ascii::Trim(" \tname\r\n");
		 * // value == "name"
		 * @endcode
		 */
		std::string_view Trim(std::string_view text) noexcept;

		/**
		 * @brief 文字列左端のASCII whitespace除去。
		 * @param[in] text trim対象の文字列view。
		 * @retval value 左端のASCII whitespaceを除いたnon-owning view。
		 * @pre 非空viewのdataは、sizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。戻りviewは`text`と同じ元文字列を参照。
		 * @note ASCII whitespaceはspace、tab、CR、LF、FF、VTのみ。
		 * @code
		 * const auto value = ket::ascii::TrimLeft("\n  name");
		 * // value == "name"
		 * @endcode
		 */
		std::string_view TrimLeft(std::string_view text) noexcept;

		/**
		 * @brief 文字列右端のASCII whitespace除去。
		 * @param[in] text trim対象の文字列view。
		 * @retval value 右端のASCII whitespaceを除いたnon-owning view。
		 * @pre 非空viewのdataは、sizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。戻りviewは`text`と同じ元文字列を参照。
		 * @note ASCII whitespaceはspace、tab、CR、LF、FF、VTのみ。
		 * @code
		 * const auto value = ket::ascii::TrimRight("name \r\n");
		 * // value == "name"
		 * @endcode
		 */
		std::string_view TrimRight(std::string_view text) noexcept;

		/**
		 * @brief 一致時だけprefixを除いたviewの取得。
		 * @param[in] text 処理対象の文字列view。
		 * @param[in] prefix 取り除くprefix。
		 * @retval value `prefix`一致時はprefixを除いたview。不一致時は元view。
		 * @pre 非空viewのdataは、それぞれsizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。戻りviewは`text`と同じ元文字列を参照。
		 * @note `std::string_view::remove_prefix`と異なり、入力viewを変更しない。
		 * @code
		 * const auto value = ket::ascii::StripPrefix("id:42", "id:");
		 * // value == "42"
		 * @endcode
		 */
		std::string_view StripPrefix(std::string_view text, std::string_view prefix) noexcept;

		/**
		 * @brief 一致時だけsuffixを除いたviewの取得。
		 * @param[in] text 処理対象の文字列view。
		 * @param[in] suffix 取り除くsuffix。
		 * @retval value `suffix`一致時はsuffixを除いたview。不一致時は元view。
		 * @pre 非空viewのdataは、それぞれsizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。戻りviewは`text`と同じ元文字列を参照。
		 * @note `std::string_view::remove_suffix`と異なり、入力viewを変更しない。
		 * @code
		 * const auto value = ket::ascii::StripSuffix("name.txt", ".txt");
		 * // value == "name"
		 * @endcode
		 */
		std::string_view StripSuffix(std::string_view text, std::string_view suffix) noexcept;

		/**
		 * @brief delimiterで分割したnon-owning view列の取得。
		 * @param[in] text 分割対象の文字列view。
		 * @param[in] delimiter 分割に使う1 byte delimiter。
		 * @retval value 分割後のview列。空要素を保持し、各viewは元文字列を参照。
		 * @pre 非空viewのdataは、sizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。戻りviewの有効期間は`text`の元文字列の有効期間以下。
		 * @note 結果vectorの確保があるためnoexceptなし。
		 * @code
		 * const auto parts = ket::ascii::SplitViews("a,,b,", ',');
		 * // parts == {"a", "", "b", ""}
		 * @endcode
		 */
		std::vector<std::string_view> SplitViews(std::string_view text, char delimiter);

		/**
		 * @brief delimiterで分割したowning文字列列の取得。
		 * @param[in] text 分割対象の文字列view。
		 * @param[in] delimiter 分割に使う1 byte delimiter。
		 * @retval value 分割後の文字列列。空要素を保持し、各要素は入力byte列を複製。
		 * @pre 非空viewのdataは、sizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。戻り値は`text`の元文字列に依存しない。
		 * @note 結果vectorと各stringの確保があるためnoexceptなし。
		 * @code
		 * const auto parts = ket::ascii::Split("a,,b,", ',');
		 * // parts == {"a", "", "b", ""}
		 * @endcode
		 */
		std::vector<std::string> Split(std::string_view text, char delimiter);

		/**
		 * @brief 文字列view列のdelimiter付き連結。
		 * @param[in] parts 連結対象の文字列view列。
		 * @param[in] delimiter 要素間へ挿入する文字列view。
		 * @retval value `parts`を入力順にdelimiterで連結した文字列。
		 * @pre 各非空viewのdataは、それぞれsizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。戻り値は入力byte列を複製。
		 * @note 出力stringの確保があるためnoexceptなし。
		 * @code
		 * const auto text = ket::ascii::Join({"a", "b", ""}, ",");
		 * // text == "a,b,"
		 * @endcode
		 */
		std::string Join(const std::vector<std::string_view>& parts, std::string_view delimiter);

		/**
		 * @brief 全ての非重複一致部分の置換。
		 * @param[in] text 置換対象の文字列view。
		 * @param[in] from 置換元の文字列view。
		 * @param[in] to 置換後の文字列view。
		 * @retval value `from`に一致した全ての非重複部分を`to`へ置換した文字列。
		 * @pre `from`は空でない。各非空viewのdataはsizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。ASCII以外のbyteも一致判定と出力で保持。
		 * @note
		 * `from`が空の場合はstd::invalid_argument送出。出力stringの確保があるためnoexceptなし。
		 * @code
		 * const auto text = ket::ascii::ReplaceAll("a--b--", "--", ":");
		 * // text == "a:b:"
		 * @endcode
		 */
		std::string ReplaceAll(std::string_view text, std::string_view from, std::string_view to);

		/**
		 * @brief ASCII英大文字だけを小文字化した文字列の取得。
		 * @param[in] text 変換対象の文字列view。
		 * @retval value ASCII英大文字を小文字化した文字列。
		 * @pre 非空viewのdataは、sizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。ASCII英大文字以外のbyteは変更なし。
		 * @note Unicode、locale、正規化を扱わない。
		 * @code
		 * const auto text = ket::ascii::ToLower("A-Z_09");
		 * // text == "a-z_09"
		 * @endcode
		 */
		std::string ToLower(std::string_view text);

		/**
		 * @brief ASCII英小文字だけを大文字化した文字列の取得。
		 * @param[in] text 変換対象の文字列view。
		 * @retval value ASCII英小文字を大文字化した文字列。
		 * @pre 非空viewのdataは、sizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。ASCII英小文字以外のbyteは変更なし。
		 * @note Unicode、locale、正規化を扱わない。
		 * @code
		 * const auto text = ket::ascii::ToUpper("a-z_09");
		 * // text == "A-Z_09"
		 * @endcode
		 */
		std::string ToUpper(std::string_view text);

		/**
		 * @brief ASCII英字だけをcase-insensitiveにした等価判定。
		 * @param[in] a 比較対象の文字列view。
		 * @param[in] b 比較対象の文字列view。
		 * @retval true ASCII英字の大小だけを無視すると同じbyte列。
		 * @retval false 長さ不一致、またはASCII英字の大小以外で異なるbyte列。
		 * @pre 非空viewのdataは、それぞれsizeバイト以上読み取り可能な領域を指す。
		 * @post 引数と外部状態の変更なし。
		 * @note ASCII範囲外のbyteは変換せず、完全一致だけを等価として扱う。
		 * @code
		 * const auto matched = ket::ascii::EqualsIgnoreCase("Mode", "mode");
		 * // matched == true
		 * @endcode
		 */
		bool EqualsIgnoreCase(std::string_view a, std::string_view b) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief std::stringのrvalueだけを検出するtrait。
			 */
			template <typename T>
			struct IsStringRvalue
				: std::integral_constant<
					  bool,
					  std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::string> &&
						  std::is_rvalue_reference_v<T&&>>
			{
			};

			/**
			 * @brief std::stringのrvalueでのみ有効なSFINAE helper。
			 */
			template <typename T>
			using EnableIfStringRvalue = std::enable_if_t<IsStringRvalue<T>::value>;

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		/**
		 * @brief 一時std::stringからのdangling view生成を拒否。
		 * @param[in] text 拒否対象の一時std::string。
		 * @retval value 戻り値なし。このoverloadは常にdelete。
		 * @pre `text`はstd::stringのrvalue。
		 * @post 呼び出しはcompile時に拒否され、runtime副作用なし。
		 */
		template <typename T, typename = detail::EnableIfStringRvalue<T>>
		std::string_view Trim(T&& text) = delete;

		/**
		 * @brief 一時std::stringからのdangling view生成を拒否。
		 * @param[in] text 拒否対象の一時std::string。
		 * @retval value 戻り値なし。このoverloadは常にdelete。
		 * @pre `text`はstd::stringのrvalue。
		 * @post 呼び出しはcompile時に拒否され、runtime副作用なし。
		 */
		template <typename T, typename = detail::EnableIfStringRvalue<T>>
		std::string_view TrimLeft(T&& text) = delete;

		/**
		 * @brief 一時std::stringからのdangling view生成を拒否。
		 * @param[in] text 拒否対象の一時std::string。
		 * @retval value 戻り値なし。このoverloadは常にdelete。
		 * @pre `text`はstd::stringのrvalue。
		 * @post 呼び出しはcompile時に拒否され、runtime副作用なし。
		 */
		template <typename T, typename = detail::EnableIfStringRvalue<T>>
		std::string_view TrimRight(T&& text) = delete;

		/**
		 * @brief 一時std::stringからのdangling view生成を拒否。
		 * @param[in] text 拒否対象の一時std::string。
		 * @param[in] prefix 参照しないprefix。
		 * @retval value 戻り値なし。このoverloadは常にdelete。
		 * @pre `text`はstd::stringのrvalue。
		 * @post 呼び出しはcompile時に拒否され、runtime副作用なし。
		 */
		template <typename T, typename = detail::EnableIfStringRvalue<T>>
		std::string_view StripPrefix(T&& text, std::string_view prefix) = delete;

		/**
		 * @brief 一時std::stringからのdangling view生成を拒否。
		 * @param[in] text 拒否対象の一時std::string。
		 * @param[in] suffix 参照しないsuffix。
		 * @retval value 戻り値なし。このoverloadは常にdelete。
		 * @pre `text`はstd::stringのrvalue。
		 * @post 呼び出しはcompile時に拒否され、runtime副作用なし。
		 */
		template <typename T, typename = detail::EnableIfStringRvalue<T>>
		std::string_view StripSuffix(T&& text, std::string_view suffix) = delete;

		/**
		 * @brief 一時std::stringからのdangling view生成を拒否。
		 * @param[in] text 拒否対象の一時std::string。
		 * @param[in] delimiter 参照しないdelimiter。
		 * @retval value 戻り値なし。このoverloadは常にdelete。
		 * @pre `text`はstd::stringのrvalue。
		 * @post 呼び出しはcompile時に拒否され、runtime副作用なし。
		 */
		template <typename T, typename = detail::EnableIfStringRvalue<T>>
		std::vector<std::string_view> SplitViews(T&& text, char delimiter) = delete;

	} // namespace ascii

} // namespace ket
