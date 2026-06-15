#pragma once

/**
 * @file ket_cli.h
 * @brief argc/argvから小さいCLI optionを取得するAPI。
 *
 * @details 小さい社内CLIで繰り返し書く`--key value`、`--key=value`、flag、positional
 * argument取得を短いAPIへ集約する。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で
 * 持ち出す。戻り値のstd::string_viewはargvの文字列領域を参照し、所有しない。
 *
 * @par プロジェクトへの適用方法
 * `ket_cli.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view`で`argv` lifetimeに依存する値を明示しながら扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::cli
 * 内部実装：ket::cli::detail
 */

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ket
{
	namespace cli
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief argc/argvのnon-owning view。
		 * @note 各argumentはnull終端C文字列として扱う。`argv[i] == nullptr`は空argument。
		 */
		class ArgvView
		{
		  public:
			/**
			 * @brief argc/argv viewの構築。
			 * @param[in] argc argv要素数。
			 * @param[in] argv argv配列先頭。`argc > 0 && argv == nullptr`は空view。
			 * @retval value 構築済みview。
			 * @pre `argv`が非nullの場合、`argc`個のポインタを読み取り可能。
			 * @post `argc < 0`または`argv == nullptr`の場合は空view。文字列領域の所有権は取得なし。
			 */
			ArgvView(int argc, const char* const* argv) noexcept
				: size_(argc > 0 && argv != nullptr ? static_cast<std::size_t>(argc) : 0U),
				  argv_(argc > 0 && argv != nullptr ? argv : nullptr)
			{
			}

			/**
			 * @brief argv viewのコピー構築。
			 * @param[in] other コピー元view。
			 * @retval value コピーされたview。
			 * @pre なし。コピー元viewは有効なArgvView。
			 * @post コピー元viewと外部状態の変更なし。文字列領域の所有権は取得なし。
			 */
			ArgvView(const ArgvView& other) noexcept = default;

			/**
			 * @brief argv viewのコピー代入。
			 * @param[in] other コピー元view。
			 * @retval value コピー代入後の`*this`。
			 * @pre なし。コピー元viewは有効なArgvView。
			 * @post コピー元viewと外部状態の変更なし。文字列領域の所有権は取得なし。
			 */
			ArgvView& operator=(const ArgvView& other) noexcept = default;

			/**
			 * @brief argv viewのmove構築。
			 * @param[in] other move元view。
			 * @retval value move構築されたview。
			 * @pre なし。move元viewは有効なArgvView。
			 * @post 文字列領域の所有権は取得なし。move元viewは破棄または再代入可能。
			 */
			ArgvView(ArgvView&& other) noexcept = default;

			/**
			 * @brief argv viewのmove代入。
			 * @param[in] other move元view。
			 * @retval value move代入後の`*this`。
			 * @pre なし。move元viewは有効なArgvView。
			 * @post 文字列領域の所有権は取得なし。move元viewは破棄または再代入可能。
			 */
			ArgvView& operator=(ArgvView&& other) noexcept = default;

			/**
			 * @brief argv要素数の取得。
			 * @retval value view内のargv要素数。
			 * @pre なし。
			 * @post viewと外部状態の変更なし。
			 */
			[[nodiscard]] std::size_t Size() const noexcept
			{
				return size_;
			}

			/**
			 * @brief 指定indexのargument取得。
			 * @param[in] index 取得対象index。
			 * @retval value 指定indexのargument。
			 * @retval empty 範囲外、空view、または`argv[index] == nullptr`。
			 * @pre なし。範囲外とnull要素は空argumentとして扱う。
			 * @post viewと外部状態の変更なし。戻り値はargv文字列領域を参照。
			 */
			[[nodiscard]] std::string_view AtOrEmpty(std::size_t index) const noexcept
			{
				const auto index_is_out_of_range = index >= size_;
				if (index_is_out_of_range)
				{
					return {};
				}

				const char* const argument = argv_[index];
				if (argument == nullptr)
				{
					return {};
				}

				return {argument};
			}

			/**
			 * @brief program nameの取得。
			 * @retval value `argv[0]`。
			 * @retval empty 空view、または`argv[0] == nullptr`。
			 * @pre なし。
			 * @post viewと外部状態の変更なし。戻り値はargv文字列領域を参照。
			 */
			[[nodiscard]] std::string_view ProgramNameOrEmpty() const noexcept
			{
				return AtOrEmpty(0U);
			}

		  private:
			std::size_t size_ = 0U;
			const char* const* argv_ = nullptr;
		};

		/**
		 * @brief optionの有無確認。
		 * @param[in] args 検索対象argv view。
		 * @param[in] name 検索するoption名。`"--"`で始まる名前のみ有効。
		 * @retval true `name`または`name=value`に一致するoptionが存在。
		 * @retval false option不在、または`name`が`"--"`で始まらない。
		 * @pre なし。program nameは検索対象外。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const char* argv[] = {"tool", "--help"};
		 * const auto args = ket::cli::ArgvView(2, argv);
		 * const auto has_help = ket::cli::HasOption(args, "--help");
		 * // has_help == true
		 * @endcode
		 */
		[[nodiscard]] inline bool HasOption(ArgvView args, std::string_view name) noexcept;

		/**
		 * @brief option値の取得。
		 * @param[in] args 検索対象argv view。
		 * @param[in] name 検索するoption名。`"--"`で始まる名前のみ有効。
		 * @retval value `--key value`または`--key=value`の値。
		 * @retval std::nullopt option不在、`name`不正、値なし、または次要素が別option。
		 * @pre なし。program nameは検索対象外。
		 * @post 引数と外部状態の変更なし。戻り値はargv文字列領域を参照。
		 * @note 重複optionは先に現れた値を返す。shell quote展開は扱わない。
		 * @code
		 * const char* argv[] = {"tool", "--id", "123"};
		 * const auto args = ket::cli::ArgvView(3, argv);
		 * const auto id = ket::cli::OptionValue(args, "--id");
		 * // id == std::optional<std::string_view>("123")
		 * @endcode
		 */
		[[nodiscard]] inline std::optional<std::string_view>
		OptionValue(ArgvView args, std::string_view name) noexcept;

		/**
		 * @brief option値またはfallbackの取得。
		 * @param[in] args 検索対象argv view。
		 * @param[in] name 検索するoption名。`"--"`で始まる名前のみ有効。
		 * @param[in] fallback option値が取得できない場合に返す値。
		 * @retval value `--key value`または`--key=value`の値。
		 * @retval fallback option不在、`name`不正、値なし、または次要素が別option。
		 * @pre なし。program nameは検索対象外。
		 * @post 引数と外部状態の変更なし。戻り値はargv文字列領域またはfallbackを参照。
		 */
		[[nodiscard]] inline std::string_view
		OptionValueOr(ArgvView args, std::string_view name, std::string_view fallback) noexcept;

		/**
		 * @brief positional argumentの取得。
		 * @param[in] args 取得対象argv view。
		 * @retval value program nameを除く、`"--"`で始まらないargumentの列。
		 * @pre なし。option schemaは持たず、`"--"`で始まるかどうかだけで判定。
		 * @post 引数と外部状態の変更なし。戻り値の各viewはargv文字列領域を参照。
		 * @note 結果vectorの確保があるためnoexceptなし。
		 */
		[[nodiscard]] inline std::vector<std::string_view> PositionalArguments(ArgvView args);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief 文字列prefix判定。
			 * @param[in] text 判定対象文字列。
			 * @param[in] prefix 期待するprefix。
			 * @retval true `text`が`prefix`で始まる。
			 * @retval false `prefix`不一致。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			[[nodiscard]] constexpr bool StartsWith(std::string_view text,
													std::string_view prefix) noexcept
			{
				const auto prefix_is_too_long = prefix.size() > text.size();
				if (prefix_is_too_long)
				{
					return false;
				}

				return text.substr(0U, prefix.size()) == prefix;
			}

			/**
			 * @brief option名として扱える文字列か判定。
			 * @param[in] name 判定対象option名。
			 * @retval true `name`が`"--"`で始まる。
			 * @retval false `name`が`"--"`で始まらない。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			[[nodiscard]] constexpr bool IsOptionName(std::string_view name) noexcept
			{
				return StartsWith(name, "--");
			}

			/**
			 * @brief argv要素がoption形か判定。
			 * @param[in] argument 判定対象argument。
			 * @retval true `argument`が`"--"`で始まる。
			 * @retval false `argument`が`"--"`で始まらない。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			[[nodiscard]] constexpr bool IsOptionArgument(std::string_view argument) noexcept
			{
				return StartsWith(argument, "--");
			}

			/**
			 * @brief inline値付きoptionとの一致判定。
			 * @param[in] argument 判定対象argument。
			 * @param[in] name 検索するoption名。
			 * @retval true `argument`が`name=value`形。
			 * @retval false inline値付きoption不一致。
			 * @pre `name`は`"--"`で始まるoption名。
			 * @post 引数と外部状態の変更なし。
			 */
			[[nodiscard]] constexpr bool IsInlineValueOption(std::string_view argument,
															 std::string_view name) noexcept
			{
				const auto has_prefix = StartsWith(argument, name);
				if (!has_prefix)
				{
					return false;
				}

				const auto has_separator =
					argument.size() > name.size() && argument[name.size()] == '=';
				return has_separator;
			}

			/**
			 * @brief option argumentが検索名に一致するか判定。
			 * @param[in] argument 判定対象argument。
			 * @param[in] name 検索するoption名。
			 * @retval true `argument`が`name`または`name=value`に一致。
			 * @retval false option不一致。
			 * @pre `name`は`"--"`で始まるoption名。
			 * @post 引数と外部状態の変更なし。
			 */
			[[nodiscard]] constexpr bool MatchesOption(std::string_view argument,
													   std::string_view name) noexcept
			{
				const auto exact_match = argument == name;
				if (exact_match)
				{
					return true;
				}

				return IsInlineValueOption(argument, name);
			}

			/**
			 * @brief inline値付きoptionから値を切り出す。
			 * @param[in] argument `name=value`形のargument。
			 * @param[in] name 検索するoption名。
			 * @retval value `=`より後ろの値。
			 * @pre `argument`は`name=value`形。
			 * @post 引数と外部状態の変更なし。戻り値は`argument`と同じ文字列領域を参照。
			 */
			[[nodiscard]] constexpr std::string_view
			InlineOptionValue(std::string_view argument, std::string_view name) noexcept
			{
				return argument.substr(name.size() + 1U);
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		[[nodiscard]] inline bool HasOption(ArgvView args, std::string_view name) noexcept
		{
			const auto name_is_option = detail::IsOptionName(name);
			if (!name_is_option)
			{
				return false;
			}

			const auto size = args.Size();
			for (std::size_t index = 1U; index < size; ++index)
			{
				const auto argument = args.AtOrEmpty(index);
				const auto matches = detail::MatchesOption(argument, name);
				if (matches)
				{
					return true;
				}
			}

			return false;
		}

		[[nodiscard]] inline std::optional<std::string_view>
		OptionValue(ArgvView args, std::string_view name) noexcept
		{
			const auto name_is_option = detail::IsOptionName(name);
			if (!name_is_option)
			{
				return std::nullopt;
			}

			const auto size = args.Size();
			for (std::size_t index = 1U; index < size; ++index)
			{
				const auto argument = args.AtOrEmpty(index);

				const auto has_inline_value = detail::IsInlineValueOption(argument, name);
				if (has_inline_value)
				{
					return detail::InlineOptionValue(argument, name);
				}

				const auto exact_match = argument == name;
				if (exact_match)
				{
					const auto next_index = index + 1U;
					const auto next_exists = next_index < size;
					if (!next_exists)
					{
						return std::nullopt;
					}

					const auto next_argument = args.AtOrEmpty(next_index);
					const auto next_is_option = detail::IsOptionArgument(next_argument);
					if (next_is_option)
					{
						return std::nullopt;
					}

					return next_argument;
				}
			}

			return std::nullopt;
		}

		[[nodiscard]] inline std::string_view
		OptionValueOr(ArgvView args, std::string_view name, std::string_view fallback) noexcept
		{
			const auto value = OptionValue(args, name);
			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return fallback;
			}

			return *value;
		}

		[[nodiscard]] inline std::vector<std::string_view> PositionalArguments(ArgvView args)
		{
			std::vector<std::string_view> result;

			const auto size = args.Size();
			for (std::size_t index = 1U; index < size; ++index)
			{
				const auto argument = args.AtOrEmpty(index);
				const auto argument_is_option = detail::IsOptionArgument(argument);
				if (argument_is_option)
				{
					continue;
				}

				result.push_back(argument);
			}

			return result;
		}

	} // namespace cli

} // namespace ket
