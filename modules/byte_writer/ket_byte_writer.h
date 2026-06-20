#pragma once

/**
 * @file ket_byte_writer.h
 * @brief fixed bufferへの逐次書き込みAPI。
 *
 * @details 所有しないfixed bufferへ、size確認後にbyte列と整数値を逐次書き込む。
 * 成功時だけoffsetとbufferを更新し、失敗時は既存bufferを保持する。drop-in時は宣言と実装を
 * 同じ単位で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_byte_writer.h` と `ket_byte_writer.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：fixed buffer書き込みのsize確認とoffset更新条件を小さいAPIで固定できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::byte_writer
 * 内部実装：.cpp の無名 namespace
 */

#include <cstddef>
#include <cstdint>

namespace ket
{
	namespace byte_writer
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief fixed bufferへ逐次書き込むnon-owning writer。
		 *
		 * `data == nullptr && size == 0`は有効な空writer。`data == nullptr && size > 0`は
		 * invalid writerとして扱い、書き込みとskipは失敗。
		 */
		class Writer
		{
		  public:
			/**
			 * @brief 書き込み先bufferを参照するwriter生成。
			 * @param[in,out] data 書き込み先buffer先頭。`size == 0`のときだけnullptr可。
			 * @param[in] size 書き込み先bufferのbyte数。
			 * @pre `data`は`size`バイト以上書き込み可能な配列を指す。`nullptr`と非0
			 * sizeの組はinvalid writerとして構築。
			 * @post offsetは0。buffer内容の変更なし。
			 * @code
			 * std::uint8_t buffer[] = {0x00U, 0x00U};
			 * ket::byte_writer::Writer writer(buffer, 2U);
			 * // writer.Size() == 2
			 * // writer.Offset() == 0
			 * @endcode
			 */
			Writer(std::uint8_t* data, std::size_t size) noexcept;

			/**
			 * @brief writer状態のcopy constructor。
			 * @param[in] other copy元writer。
			 * @pre `other`は有効またはinvalidなWriter object。
			 * @post non-owning pointer、size、offsetをそのまま複製。buffer内容の変更なし。
			 * @code
			 * std::uint8_t buffer[] = {0x00U};
			 * ket::byte_writer::Writer source(buffer, 1U);
			 * ket::byte_writer::Writer copied(source);
			 * // copied.Size() == 1
			 * // copied.Offset() == 0
			 * @endcode
			 */
			Writer(const Writer& other) noexcept = default;

			/**
			 * @brief writer状態のcopy assignment。
			 * @param[in] other copy元writer。
			 * @retval value `*this`。
			 * @pre `other`は有効またはinvalidなWriter object。
			 * @post non-owning pointer、size、offsetをそのまま複製。buffer内容の変更なし。
			 * @code
			 * std::uint8_t buffer[] = {0x00U};
			 * ket::byte_writer::Writer source(buffer, 1U);
			 * ket::byte_writer::Writer assigned(nullptr, 0U);
			 * assigned = source;
			 * // assigned.Size() == 1
			 * @endcode
			 */
			Writer& operator=(const Writer& other) noexcept = default;

			/**
			 * @brief writer状態のmove constructor。
			 * @param[in,out] other move元writer。
			 * @pre `other`は有効またはinvalidなWriter object。
			 * @post non-owning pointer、size、offsetをそのまま移動。buffer内容の変更なし。
			 * @code
			 * std::uint8_t buffer[] = {0x00U};
			 * ket::byte_writer::Writer source(buffer, 1U);
			 * ket::byte_writer::Writer moved(static_cast<ket::byte_writer::Writer&&>(source));
			 * // moved.Size() == 1
			 * @endcode
			 */
			Writer(Writer&& other) noexcept = default;

			/**
			 * @brief writer状態のmove assignment。
			 * @param[in,out] other move元writer。
			 * @retval value `*this`。
			 * @pre `other`は有効またはinvalidなWriter object。
			 * @post non-owning pointer、size、offsetをそのまま移動。buffer内容の変更なし。
			 * @code
			 * std::uint8_t buffer[] = {0x00U};
			 * ket::byte_writer::Writer source(buffer, 1U);
			 * ket::byte_writer::Writer assigned(nullptr, 0U);
			 * assigned = static_cast<ket::byte_writer::Writer&&>(source);
			 * // assigned.Size() == 1
			 * @endcode
			 */
			Writer& operator=(Writer&& other) noexcept = default;

			/**
			 * @brief 参照先bufferの総byte数取得。
			 * @retval value constructorで渡されたbuffer size。
			 * @pre なし。invalid writerでも参照先として渡されたsizeを返す。
			 * @post writer状態とbuffer内容の変更なし。
			 * @code
			 * std::uint8_t buffer[] = {0x00U, 0x00U};
			 * ket::byte_writer::Writer writer(buffer, 2U);
			 * const auto size = writer.Size();
			 * // size == 2
			 * @endcode
			 */
			std::size_t Size() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 次に書き込むoffset取得。
			 * @retval value 現在offset。
			 * @pre なし。
			 * @post writer状態とbuffer内容の変更なし。
			 * @code
			 * std::uint8_t buffer[] = {0x00U};
			 * ket::byte_writer::Writer writer(buffer, 1U);
			 * const auto offset = writer.Offset();
			 * // offset == 0
			 * @endcode
			 */
			std::size_t Offset() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 残り書き込み可能byte数取得。
			 * @retval value valid writerでは`Size() - Offset()`。invalid writerまたはoffsetがsizeを
			 * 超える内部不整合時は0。
			 * @pre なし。
			 * @post writer状態とbuffer内容の変更なし。
			 * @code
			 * std::uint8_t buffer[] = {0x00U, 0x00U};
			 * ket::byte_writer::Writer writer(buffer, 2U);
			 * const auto remaining = writer.Remaining();
			 * // remaining == 2
			 * @endcode
			 */
			std::size_t Remaining() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief valid writerのoffsetが末尾に到達したかの判定。
			 * @retval true writerがvalidで残りbyte数が0。
			 * @retval false invalid writer、内部不整合、または残りbyte数が1以上。
			 * @pre なし。
			 * @post writer状態とbuffer内容の変更なし。
			 * @code
			 * ket::byte_writer::Writer writer(nullptr, 0U);
			 * const auto full = writer.Full();
			 * // full == true
			 * @endcode
			 */
			bool Full() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 指定byte数だけoffsetを進める。
			 * @param[in] size skipするbyte数。
			 * @retval true 指定byte数のskipに成功。
			 * @retval false invalid writer、または残りbyte数不足。
			 * @pre なし。size不足は失敗値として扱う。
			 * @post 成功時だけoffsetを`size`だけ進める。buffer内容の変更なし。
			 * @code
			 * std::uint8_t buffer[] = {0x00U, 0x00U};
			 * ket::byte_writer::Writer writer(buffer, 2U);
			 * const auto ok = writer.Skip(1U);
			 * // ok == true
			 * // writer.Offset() == 1
			 * @endcode
			 */
			bool Skip(std::size_t size) noexcept;

			/**
			 * @brief 1byte値の書き込み。
			 * @param[in] value 書き込む値。
			 * @retval true 書き込みに成功。
			 * @retval false invalid writer、または残りbyte数不足。
			 * @pre なし。size不足は失敗値として扱う。
			 * @post 成功時だけoffsetを1進め、対象byteを更新。失敗時はoffsetとbuffer内容を保持。
			 * @code
			 * std::uint8_t buffer[] = {0x00U};
			 * ket::byte_writer::Writer writer(buffer, 1U);
			 * const auto ok = writer.WriteU8(0x12U);
			 * // ok == true, buffer[0] == 0x12U, writer.Offset() == 1
			 * @endcode
			 */
			bool WriteU8(std::uint8_t value) noexcept;

			/**
			 * @brief big-endian 16bit値の書き込み。
			 * @param[in] value 書き込む値。
			 * @retval true 書き込みに成功。
			 * @retval false invalid writer、または残りbyte数不足。
			 * @pre なし。size不足は失敗値として扱う。
			 * @post
			 * 成功時だけoffsetを2進め、対象byteをbig-endian順に更新。失敗時はoffsetとbuffer内容を保持。
			 * @code
			 * std::uint8_t buffer[] = {0x00U, 0x00U};
			 * ket::byte_writer::Writer writer(buffer, 2U);
			 * const auto ok = writer.WriteBe16(0x1234U);
			 * // ok == true, buffer[0] == 0x12U, buffer[1] == 0x34U
			 * @endcode
			 */
			bool WriteBe16(std::uint16_t value) noexcept;

			/**
			 * @brief big-endian 32bit値の書き込み。
			 * @param[in] value 書き込む値。
			 * @retval true 書き込みに成功。
			 * @retval false invalid writer、または残りbyte数不足。
			 * @pre なし。size不足は失敗値として扱う。
			 * @post
			 * 成功時だけoffsetを4進め、対象byteをbig-endian順に更新。失敗時はoffsetとbuffer内容を保持。
			 * @code
			 * std::uint8_t buffer[] = {0x00U, 0x00U, 0x00U, 0x00U};
			 * ket::byte_writer::Writer writer(buffer, 4U);
			 * const auto ok = writer.WriteBe32(0x12345678U);
			 * // ok == true, buffer[0] == 0x12U, buffer[3] == 0x78U
			 * @endcode
			 */
			bool WriteBe32(std::uint32_t value) noexcept;

			/**
			 * @brief little-endian 16bit値の書き込み。
			 * @param[in] value 書き込む値。
			 * @retval true 書き込みに成功。
			 * @retval false invalid writer、または残りbyte数不足。
			 * @pre なし。size不足は失敗値として扱う。
			 * @post
			 * 成功時だけoffsetを2進め、対象byteをlittle-endian順に更新。失敗時はoffsetとbuffer内容を保持。
			 * @code
			 * std::uint8_t buffer[] = {0x00U, 0x00U};
			 * ket::byte_writer::Writer writer(buffer, 2U);
			 * const auto ok = writer.WriteLe16(0x1234U);
			 * // ok == true, buffer[0] == 0x34U, buffer[1] == 0x12U
			 * @endcode
			 */
			bool WriteLe16(std::uint16_t value) noexcept;

			/**
			 * @brief little-endian 32bit値の書き込み。
			 * @param[in] value 書き込む値。
			 * @retval true 書き込みに成功。
			 * @retval false invalid writer、または残りbyte数不足。
			 * @pre なし。size不足は失敗値として扱う。
			 * @post
			 * 成功時だけoffsetを4進め、対象byteをlittle-endian順に更新。失敗時はoffsetとbuffer内容を保持。
			 * @code
			 * std::uint8_t buffer[] = {0x00U, 0x00U, 0x00U, 0x00U};
			 * ket::byte_writer::Writer writer(buffer, 4U);
			 * const auto ok = writer.WriteLe32(0x12345678U);
			 * // ok == true, buffer[0] == 0x78U, buffer[3] == 0x12U
			 * @endcode
			 */
			bool WriteLe32(std::uint32_t value) noexcept;

			/**
			 * @brief byte列の書き込み。
			 * @param[in] data 書き込むbyte列先頭。`size == 0`のときだけnullptr可。
			 * @param[in] size 書き込むbyte数。
			 * @retval true 書き込みに成功。valid writerの`size == 0`はno-op成功。
			 * @retval false null source、invalid writer、または残りbyte数不足。
			 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`nullptr`と非0
			 * sizeの組は失敗値として扱う。
			 * @post
			 * 成功時だけoffsetを`size`だけ進め、対象範囲を更新。失敗時はoffsetとbuffer内容を保持。
			 * @code
			 * std::uint8_t buffer[] = {0x00U, 0x00U};
			 * const std::uint8_t source[] = {0x10U, 0x20U};
			 * ket::byte_writer::Writer writer(buffer, 2U);
			 * const auto ok = writer.WriteBytes(source, 2U);
			 * // ok == true, buffer[0] == 0x10U, writer.Offset() == 2
			 * @endcode
			 */
			bool WriteBytes(const std::uint8_t* data, std::size_t size) noexcept;

		  private:
			std::uint8_t* data_;
			std::size_t size_;
			std::size_t offset_ = 0U;
		};

	} // namespace byte_writer

} // namespace ket
