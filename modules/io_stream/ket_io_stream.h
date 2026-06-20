#pragma once

/**
 * @file ket_io_stream.h
 * @brief streamの確実な読み書きと書式状態保存API。
 *
 * @details iostreamのshort read/write判定、行末ASCII whitespace除去付き行読み、書式状態の
 * scope復元を小さいAPIへ集約する。drop-in時は宣言と実装を同じ単位で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_io_stream.h` と `ket_io_stream.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：iostreamのshort read/writeとstate復元を標準ライブラリのみで安全に薄く包める。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::io_stream
 * 内部実装：ket::io_stream::detail
 */

#include <cstddef>
#include <cstdint>
#include <ios>
#include <string>

namespace ket
{
	namespace io_stream
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 指定バイト数のstream読み切り。
		 * @param[in,out] stream 読み取り元stream。
		 * @param[out] data 読み取り先バッファ。
		 * @param[in] size 読み取るバイト数。
		 * @retval true `size`バイトを読み切った。
		 * @retval false short read、またはstream errorにより`size`バイトを読み切れなかった。
		 * @pre `size > 0`の場合、`data`は`size`バイト以上書き込み可能な領域を指す。
		 * `data == nullptr && size > 0`はprecondition違反。
		 * @post 成功時は`data[0, size)`へ読み取ったバイト列を格納。失敗時はstream状態と
		 * `data`の一部が標準istreamのread結果に従って変化する場合あり。
		 * @note streamのexception maskにより、読み取り中の例外は呼び出し側へ伝播。
		 * @code
		 * std::uint8_t bytes[4] = {};
		 * const auto ok = ket::io_stream::TryReadExactly(stream, bytes, 4U);
		 * // ok == true のとき bytes[0..3] が読み取り済み
		 * @endcode
		 */
		bool TryReadExactly(std::istream& stream, std::uint8_t* data, std::size_t size);

		/**
		 * @brief 指定バイト数のstream書き切り。
		 * @param[in,out] stream 書き込み先stream。
		 * @param[in] data 書き込み元バッファ。
		 * @param[in] size 書き込むバイト数。
		 * @retval true `size`バイトを書き切った。
		 * @retval false stream errorにより`size`バイトを書き切れなかった。
		 * @pre `size > 0`の場合、`data`は`size`バイト以上読み取り可能な領域を指す。
		 * `data == nullptr && size > 0`はprecondition違反。
		 * @post 成功時は`data[0, size)`のバイト列をstreamへ書き込み済み。失敗時はstream状態と
		 * 書き込み済み範囲が標準ostreamのwrite結果に従う。
		 * @note streamのexception maskにより、書き込み中の例外は呼び出し側へ伝播。
		 * @code
		 * const std::uint8_t bytes[] = {0x01U, 0x02U};
		 * const auto ok = ket::io_stream::TryWriteAll(stream, bytes, 2U);
		 * // ok == true のとき2バイトを書き込み済み
		 * @endcode
		 */
		bool TryWriteAll(std::ostream& stream, const std::uint8_t* data, std::size_t size);

		/**
		 * @brief 行末ASCII whitespace除去付きの1行読み取り。
		 * @param[in,out] stream 読み取り元stream。
		 * @param[out] out 読み取った行の格納先。
		 * @retval true 1行を読み取った。空行も成功として扱う。
		 * @retval false EOFで1文字も読めない、またはstream error。
		 * @pre `out`は有効なstd::string object。
		 * @post 成功時だけ`out`を更新し、末尾のASCII whitespaceを除去した行を格納。失敗時は
		 * `out`を変更なし。
		 * @note 除去対象はASCIIのspace、tab、CR、LF、form feed、vertical tabのみ。
		 * @note std::stringのallocationとstream exceptionの可能性があるためnoexceptなし。
		 * @code
		 * std::string line;
		 * const auto ok = ket::io_stream::TryReadLineTrimmedAscii(stream, line);
		 * // 入力 "value \t\r\n" では ok == true, line == "value"
		 * @endcode
		 */
		bool TryReadLineTrimmedAscii(std::istream& stream, std::string& out);

		/**
		 * @brief streamの書式状態をscope終了時に復元するRAII object。
		 * @note 保存対象はflags、precision、fill。copy/moveは禁止。
		 */
		class StateSaver
		{
		  public:
			/**
			 * @brief stream書式状態の保存開始。
			 * @param[in,out] stream 保存と復元の対象stream。
			 * @retval StateSaver 保存した状態を保持するobject。
			 * @pre `stream`はこのobjectのdestructor完了まで同じobjectとして生存。
			 * @post `stream`のflags、precision、fillを記録。構築直後のstream状態は変更なし。
			 * @code
			 * std::ostringstream stream;
			 * const ket::io_stream::StateSaver saver(stream);
			 * stream.precision(9);
			 * @endcode
			 */
			explicit StateSaver(std::ios& stream);

			/**
			 * @brief 保存したstream書式状態の復元。
			 * @retval void 戻り値なし。
			 * @pre 対象streamはconstructor呼び出し時と同じobjectとして生存。
			 * @post 対象streamのflags、precision、fillはconstructor呼び出し時の値へ復元。
			 * @code
			 * std::ostringstream stream;
			 * stream.precision(3);
			 * {
			 *     const ket::io_stream::StateSaver saver(stream);
			 *     stream.precision(9);
			 * }
			 * // stream.precision() == 3
			 * @endcode
			 */
			~StateSaver() noexcept;

			/**
			 * @brief copy構築禁止。
			 * @param[in] other copy元。使用不可。
			 * @retval void 戻り値なし。
			 * @pre copy操作は利用不可。
			 * @post 状態変更なし。
			 * @code
			 * const auto copyable = std::is_copy_constructible<ket::io_stream::StateSaver>::value;
			 * // copyable == false
			 * @endcode
			 */
			StateSaver(const StateSaver&) = delete;

			/**
			 * @brief copy代入禁止。
			 * @param[in] other copy元。使用不可。
			 * @retval value 使用不可。
			 * @pre copy操作は利用不可。
			 * @post 状態変更なし。
			 * @code
			 * const auto assignable = std::is_copy_assignable<ket::io_stream::StateSaver>::value;
			 * // assignable == false
			 * @endcode
			 */
			StateSaver& operator=(const StateSaver&) = delete;

			/**
			 * @brief move構築禁止。
			 * @param[in] other move元。使用不可。
			 * @retval void 戻り値なし。
			 * @pre move操作は利用不可。
			 * @post 状態変更なし。
			 * @code
			 * const auto movable = std::is_move_constructible<ket::io_stream::StateSaver>::value;
			 * // movable == false
			 * @endcode
			 */
			StateSaver(StateSaver&&) = delete;

			/**
			 * @brief move代入禁止。
			 * @param[in] other move元。使用不可。
			 * @retval value 使用不可。
			 * @pre move操作は利用不可。
			 * @post 状態変更なし。
			 * @code
			 * const auto movable = std::is_move_assignable<ket::io_stream::StateSaver>::value;
			 * // movable == false
			 * @endcode
			 */
			StateSaver& operator=(StateSaver&&) = delete;

		  private:
			std::ios& stream_;
			std::ios::fmtflags flags_;
			std::streamsize precision_;
			char fill_;
		};

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief ASCII whitespace判定。
			 * @param[in] value 判定対象の文字。
			 * @retval true ASCIIのspace、tab、CR、LF、form feed、vertical tab。
			 * @retval false ASCII whitespace以外。
			 * @pre なし。任意のchar値を単一byteとして扱う。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr bool IsAsciiWhitespace(char value) noexcept
			{
				return value == ' ' || value == '\t' || value == '\r' || value == '\n' ||
					value == '\f' || value == '\v';
			}

		} // namespace detail

	} // namespace io_stream

} // namespace ket
