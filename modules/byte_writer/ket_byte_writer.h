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
 * 内部実装：ket::byte_writer::detail
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
		 * invalid writerとして扱い、非空の書き込みとskipは失敗。
		 */
		class Writer
		{
		  public:
			/**
			 * @brief 書き込み先bufferを参照するwriter生成。
			 * @param[in,out] data 書き込み先buffer先頭。`size == 0`のときだけnullptr可。
			 * @param[in] size 書き込み先bufferのbyte数。
			 * @retval void 戻り値なし。
			 * @pre `data`は`size`バイト以上書き込み可能な配列を指す。`nullptr`と非0
			 * sizeの組はinvalid writerとして構築。
			 * @post offsetは0。buffer内容の変更なし。
			 */
			Writer(std::uint8_t* data, std::size_t size) noexcept;

			/**
			 * @brief writer状態のcopy constructor。
			 * @param[in] other copy元writer。
			 * @retval void 戻り値なし。
			 * @pre `other`は有効またはinvalidなWriter object。
			 * @post non-owning pointer、size、offsetをそのまま複製。buffer内容の変更なし。
			 */
			Writer(const Writer& other) noexcept = default;

			/**
			 * @brief writer状態のcopy assignment。
			 * @param[in] other copy元writer。
			 * @retval value `*this`。
			 * @pre `other`は有効またはinvalidなWriter object。
			 * @post non-owning pointer、size、offsetをそのまま複製。buffer内容の変更なし。
			 */
			Writer& operator=(const Writer& other) noexcept = default;

			/**
			 * @brief writer状態のmove constructor。
			 * @param[in,out] other move元writer。
			 * @retval void 戻り値なし。
			 * @pre `other`は有効またはinvalidなWriter object。
			 * @post non-owning pointer、size、offsetをそのまま移動。buffer内容の変更なし。
			 */
			Writer(Writer&& other) noexcept = default;

			/**
			 * @brief writer状態のmove assignment。
			 * @param[in,out] other move元writer。
			 * @retval value `*this`。
			 * @pre `other`は有効またはinvalidなWriter object。
			 * @post non-owning pointer、size、offsetをそのまま移動。buffer内容の変更なし。
			 */
			Writer& operator=(Writer&& other) noexcept = default;

			/**
			 * @brief 参照先bufferの総byte数取得。
			 * @retval value constructorで渡されたbuffer size。
			 * @pre なし。invalid writerでも参照先として渡されたsizeを返す。
			 * @post writer状態とbuffer内容の変更なし。
			 */
			std::size_t Size() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 次に書き込むoffset取得。
			 * @retval value 現在offset。
			 * @pre なし。
			 * @post writer状態とbuffer内容の変更なし。
			 */
			std::size_t Offset() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 残り書き込み可能byte数取得。
			 * @retval value `Size() - Offset()`。offsetがsizeを超える内部不整合時は0。
			 * @pre なし。
			 * @post writer状態とbuffer内容の変更なし。
			 */
			std::size_t Remaining() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief offsetが末尾に到達したかの判定。
			 * @retval true 残りbyte数が0。
			 * @retval false 残りbyte数が1以上。
			 * @pre なし。
			 * @post writer状態とbuffer内容の変更なし。
			 */
			bool Full() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 指定byte数だけoffsetを進める。
			 * @param[in] size skipするbyte数。
			 * @retval true 指定byte数のskipに成功。
			 * @retval false invalid writer、または残りbyte数不足。
			 * @pre なし。size不足は失敗値として扱う。
			 * @post 成功時だけoffsetを`size`だけ進める。buffer内容の変更なし。
			 */
			bool Skip(std::size_t size) noexcept;

			/**
			 * @brief 1byte値の書き込み。
			 * @param[in] value 書き込む値。
			 * @retval true 書き込みに成功。
			 * @retval false invalid writer、または残りbyte数不足。
			 * @pre なし。size不足は失敗値として扱う。
			 * @post 成功時だけoffsetを1進め、対象byteを更新。失敗時はoffsetとbuffer内容を保持。
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
			 */
			bool WriteLe32(std::uint32_t value) noexcept;

			/**
			 * @brief byte列の書き込み。
			 * @param[in] data 書き込むbyte列先頭。`size == 0`のときだけnullptr可。
			 * @param[in] size 書き込むbyte数。
			 * @retval true 書き込みに成功。`size == 0`はno-op成功。
			 * @retval false null source、invalid writer、または残りbyte数不足。
			 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`nullptr`と非0
			 * sizeの組は失敗値として扱う。
			 * @post
			 * 成功時だけoffsetを`size`だけ進め、対象範囲を更新。失敗時はoffsetとbuffer内容を保持。
			 */
			bool WriteBytes(const std::uint8_t* data, std::size_t size) noexcept;

		  private:
			std::uint8_t* data_;
			std::size_t size_;
			std::size_t offset_ = 0U;
		};

	} // namespace byte_writer

} // namespace ket
