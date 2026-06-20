#pragma once

/**
 * @file ket_file.h
 * @brief ファイル全読み、全書き、基本query API。
 *
 * @details 小さいファイルI/Oの定型処理を、全体読み込み、全体書き込み、存在確認、サイズ取得へ
 * 集約する。text encoding変換、path正規化、再帰処理は扱わず、標準ライブラリのfilesystemと
 * streamを薄く包む。
 *
 * @par プロジェクトへの適用方法
 * `ket_file.h` と `ket_file.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::filesystem`と`std::string_view`を利用でき、pathとbytes/textの扱いを標準型へ寄せられる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::file
 * 内部実装：ket::file::detail
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace ket
{
	namespace file
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief ファイル全体のtext読み込み。
		 * @param[in] path 読み込み対象path。
		 * @param[out] out 読み込んだbytesを格納する文字列。
		 * @param[out] error 失敗詳細のoptional出力。`nullptr`の場合は詳細を破棄。
		 * @retval true ファイル全体を読み込み、`out`を更新。
		 * @retval false path不在、directory指定、permission、短いread、またはサイズ過大。
		 * @pre `out`は有効なstd::string。`error`は`nullptr`または有効なstd::error_code。
		 * @post
		 * 成功時だけ`out`を置換。失敗時は`out`不変。成功時は`error`をclearし、失敗時は可能な範囲で原因を設定。
		 * @note text encoding変換なし。ファイルbytesをそのままstd::stringへ保持。
		 * @note std::stringのallocation例外は呼び出し側へ伝播。
		 * @code
		 * std::string text;
		 * std::error_code error;
		 * const auto ok = ket::file::TryReadAllText("config.txt", text, &error);
		 * @endcode
		 */
		bool TryReadAllText(const std::filesystem::path& path,
							std::string& out,
							std::error_code* error = nullptr);

		/**
		 * @brief ファイル全体のbytes読み込み。
		 * @param[in] path 読み込み対象path。
		 * @param[out] out 読み込んだbytesを格納するvector。
		 * @param[out] error 失敗詳細のoptional出力。`nullptr`の場合は詳細を破棄。
		 * @retval true ファイル全体を読み込み、`out`を更新。
		 * @retval false path不在、directory指定、permission、短いread、またはサイズ過大。
		 * @pre `out`は有効なstd::vector。`error`は`nullptr`または有効なstd::error_code。
		 * @post
		 * 成功時だけ`out`を置換。失敗時は`out`不変。成功時は`error`をclearし、失敗時は可能な範囲で原因を設定。
		 * @note binary/textを区別せず、ファイルbytesをそのままstd::uint8_t列へ保持。
		 * @note std::vectorのallocation例外は呼び出し側へ伝播。
		 * @code
		 * std::vector<std::uint8_t> bytes;
		 * const auto ok = ket::file::TryReadAllBytes("payload.bin", bytes);
		 * @endcode
		 */
		bool TryReadAllBytes(const std::filesystem::path& path,
							 std::vector<std::uint8_t>& out,
							 std::error_code* error = nullptr);

		/**
		 * @brief text内容のファイル全体書き込み。
		 * @param[in] path 書き込み対象path。
		 * @param[in] text 書き込むbytesを指す文字列view。
		 * @param[out] error 失敗詳細のoptional出力。`nullptr`の場合は詳細を破棄。
		 * @retval true `text`全体を書き込み、stream closeまで成功。
		 * @retval false pathを開けない、permission、directory指定、短いwrite、またはclose失敗。
		 * @pre
		 * `text`は`text.size()`バイト以上読み取り可能な範囲を指す。`error`は`nullptr`または有効なstd::error_code。
		 * @post
		 * 成功時は対象pathの内容を`text`で置換。失敗時の部分書き込み有無はplatformとstream状態に従う。
		 * @note text encoding変換なし。`text`のbytesをそのまま書き込む。
		 * @code
		 * std::error_code error;
		 * const auto ok = ket::file::TryWriteAllText("config.txt", "mode=run", &error);
		 * @endcode
		 */
		bool TryWriteAllText(const std::filesystem::path& path,
							 std::string_view text,
							 std::error_code* error = nullptr);

		/**
		 * @brief bytes内容のファイル全体書き込み。
		 * @param[in] path 書き込み対象path。
		 * @param[in] data 書き込むbytesの先頭。`size == 0`の場合だけ`nullptr`可。
		 * @param[in] size 書き込むbyte数。
		 * @param[out] error 失敗詳細のoptional出力。`nullptr`の場合は詳細を破棄。
		 * @retval true `data[0, size)`全体を書き込み、stream closeまで成功。
		 * @retval false
		 * pathを開けない、permission、directory指定、短いwrite、close失敗、またはnonnull条件違反。
		 * @pre `size >
		 * 0`の場合、`data`は`size`バイト以上読み取り可能な配列を指す。`error`は`nullptr`または有効なstd::error_code。
		 * @post
		 * 成功時は対象pathの内容を入力bytesで置換。失敗時の部分書き込み有無はplatformとstream状態に従う。
		 * @note raw pointerはbytes列のC API境界として扱うため採用。
		 * @code
		 * const std::uint8_t payload[] = {0x01U, 0x02U};
		 * const auto ok = ket::file::TryWriteAllBytes("payload.bin", payload, 2U);
		 * @endcode
		 */
		bool TryWriteAllBytes(const std::filesystem::path& path,
							  const std::uint8_t* data,
							  std::size_t size,
							  std::error_code* error = nullptr);

		/**
		 * @brief path存在確認。
		 * @param[in] path 確認対象path。
		 * @retval true pathが存在。
		 * @retval false path不在、またはfilesystem query失敗。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto exists = ket::file::Exists("config.txt");
		 * // exists == true if config.txt exists
		 * @endcode
		 */
		bool Exists(const std::filesystem::path& path) noexcept;

		/**
		 * @brief directory判定。
		 * @param[in] path 確認対象path。
		 * @retval true pathが存在し、directory。
		 * @retval false path不在、directory以外、またはfilesystem query失敗。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto directory = ket::file::IsDirectory("logs");
		 * // directory == true if logs is an existing directory
		 * @endcode
		 */
		bool IsDirectory(const std::filesystem::path& path) noexcept;

		/**
		 * @brief 通常ファイルサイズ取得。
		 * @param[in] path 取得対象path。
		 * @retval value ファイルサイズbyte数。
		 * @retval std::nullopt path不在、directory指定、またはfilesystem query失敗。
		 * @pre なし。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto size = ket::file::Size("payload.bin");
		 * // size == std::optional<std::uintmax_t>(file byte count)
		 * @endcode
		 */
		std::optional<std::uintmax_t> Size(const std::filesystem::path& path) noexcept;

	} // namespace file

} // namespace ket
