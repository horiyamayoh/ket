#pragma once

/**
 * @file ket_platform.h
 * @brief errno、Windows error、environment variable の小さいplatform補助API。
 *
 * @details platform APIの差分を隠しすぎず、errno番号、Windows error code、
 * process environment variableを標準文字列で扱うためのAPIを集約する。
 * drop-in時は宣言と実装を同じ単位で持ち出す。Windows専用APIは`_WIN32`で宣言ごと隠す。
 *
 * @par プロジェクトへの適用方法
 * `ket_platform.h` と `ket_platform.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：platform APIの差分を隠しすぎず、標準文字列で結果を扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：標準ライブラリだけではerrno/Windows error messageの扱いが不十分。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリとplatform API。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::platform
 * 内部実装：ket::platform::detail
 */

#include <optional>
#include <string>
#include <string_view>

namespace ket
{
	namespace platform
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief errno番号のplatform message文字列化。
		 * @param[in] error_number 文字列化対象のerrno番号。
		 * @retval value platformのthread-safe APIから取得したmessage、またはASCII fallback。
		 * @pre なし。未知のerrno番号はfallback文字列として扱う。
		 * @post 引数と外部状態の変更なし。戻り値は空文字列にしない。
		 * @note message取得に失敗した場合は`Unknown error <n>`形式を返す。
		 * @code
		 * const auto text = ket::platform::FormatErrno(EINVAL);
		 * // !text.empty()
		 * @endcode
		 */
		std::string FormatErrno(int error_number);

		/**
		 * @brief process environment variableの取得。
		 * @param[in] name 取得対象のenvironment variable名。
		 * @retval value environment variableが存在する場合の値。
		 * @retval std::nullopt missing、空name、NULを含むname、またはplatform変換失敗。
		 * @pre なし。空nameとNULを含むnameは失敗値として扱う。
		 * @post process environmentは変更しない。environment同時変更時の一貫性はplatform
		 * API規約に従う。
		 * @note Windowsではwide APIからUTF-8 narrow stringへ変換する。
		 * @code
		 * const auto value = ket::platform::GetEnvironmentVariable("PATH");
		 * // value.has_value() はprocess environmentに依存
		 * @endcode
		 */
		std::optional<std::string> GetEnvironmentVariable(std::string_view name);

#ifdef _WIN32
		/**
		 * @brief Windows error codeの格納型。
		 */
		using WindowsErrorCode = unsigned long;

		/**
		 * @brief 現在threadのWindows last-error code取得。
		 * @retval value `GetLastError()`が返すWindows error code。
		 * @pre なし。呼び出し前にWindows APIが設定したlast-error codeを対象とする。
		 * @post last-error codeを変更しない。
		 * @code
		 * const auto code = ket::platform::GetLastErrorCode();
		 * // code は現在threadのWindows last-error code
		 * @endcode
		 */
		WindowsErrorCode GetLastErrorCode() noexcept;

		/**
		 * @brief Windows error codeのmessage文字列化。
		 * @param[in] code 文字列化対象のWindows error code。
		 * @retval value Windows wide APIから取得してUTF-8へ変換したmessage、またはASCII fallback。
		 * @pre なし。未知のWindows error codeはfallback文字列として扱う。
		 * @post 引数と外部状態の変更なし。戻り値は空文字列にしない。
		 * @note UTF-8変換に失敗した場合は`Unknown Windows error <n>`形式を返す。
		 * @code
		 * const auto text = ket::platform::FormatWindowsError(ERROR_FILE_NOT_FOUND);
		 * // !text.empty()
		 * @endcode
		 */
		std::string FormatWindowsError(WindowsErrorCode code);
#endif

	} // namespace platform

} // namespace ket
