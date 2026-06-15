#pragma once

/**
 * @file ket_byte_reader.h
 * @brief 固定bufferからの逐次読み取りAPI。
 *
 * @details non-owning readerとしてbuffer、size、offsetを保持し、成功時だけoffsetを進める。
 * size不足やinvalid readerではoffsetと出力を変更しない。drop-in時は宣言と実装を同じ単位で
 * 持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_byte_reader.h` と `ket_byte_reader.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：buffer lifetimeとoffset更新条件を明示し、size不足時の状態不変を固定できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::byte_reader
 * 内部実装：ket::byte_reader::detail
 */

#include <cstddef>
#include <cstdint>

namespace ket
{
	namespace byte_reader
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 固定bufferを所有せずに読み進めるreader。
		 * @note `data == nullptr && size == 0`は有効な空reader。
		 * @note `data == nullptr && size > 0`はinvalid readerとして読み取り失敗。
		 */
		class Reader
		{
		  public:
			/**
			 * @brief non-owning buffer readerの構築。
			 * @param[in] data 読み取り対象buffer先頭。`size == 0`の場合のみnullptr可。
			 * @param[in] size `data`のbyte数。
			 * @retval void 戻り値なし。
			 * @pre `data`は`size`byte以上読み取り可能な配列を指す。
			 * `data == nullptr && size > 0`はinvalid readerとして保持。
			 * @post `Offset()`は0。入力pointerとsizeを保持。
			 */
			Reader(const std::uint8_t* data, std::size_t size) noexcept;

			/**
			 * @brief reader状態のcopy構築。
			 * @param[in] other copy元reader。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post non-owning pointer、size、offsetを`other`と同じ値で保持。
			 */
			Reader(const Reader& other) noexcept = default;

			/**
			 * @brief reader状態のcopy代入。
			 * @param[in] other copy元reader。
			 * @retval reference `*this`。
			 * @pre なし。
			 * @post non-owning pointer、size、offsetを`other`と同じ値で保持。
			 */
			Reader& operator=(const Reader& other) noexcept = default;

			/**
			 * @brief reader状態のmove構築。
			 * @param[in,out] other move元reader。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post non-owning pointer、size、offsetをmove前の`other`と同じ値で保持。
			 */
			Reader(Reader&& other) noexcept = default;

			/**
			 * @brief reader状態のmove代入。
			 * @param[in,out] other move元reader。
			 * @retval reference `*this`。
			 * @pre なし。
			 * @post non-owning pointer、size、offsetをmove前の`other`と同じ値で保持。
			 */
			Reader& operator=(Reader&& other) noexcept = default;

			/**
			 * @brief 構築時に渡したbuffer sizeの取得。
			 * @retval value 構築時に渡したbyte数。invalid readerでも入力sizeを返す。
			 * @pre なし。
			 * @post reader状態と外部状態の変更なし。
			 */
			std::size_t Size() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 現在offsetの取得。
			 * @retval value 次に読み取るbyte位置。
			 * @pre なし。
			 * @post reader状態と外部状態の変更なし。
			 */
			std::size_t Offset() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 読み取り可能な残りbyte数の取得。
			 * @retval value valid readerでは`Size() - Offset()`。invalid readerでは0。
			 * @pre なし。
			 * @post reader状態と外部状態の変更なし。
			 */
			std::size_t Remaining() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 読み取り可能byteの空判定。
			 * @retval true 読み取り可能な残りbyteなし。
			 * @retval false 1byte以上の読み取り可能byteあり。
			 * @pre なし。
			 * @post reader状態と外部状態の変更なし。
			 */
			bool Empty() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 指定byte数の読み飛ばし。
			 * @param[in] size 読み飛ばすbyte数。
			 * @retval true `size`byteの読み飛ばし成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre 元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が`size`増加。失敗時はoffset不変。
			 */
			bool Skip(std::size_t size) noexcept;

			/**
			 * @brief 1byte unsigned値の読み取り。
			 * @param[out] out 読み取った値。
			 * @retval true 1byte読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out`は有効な参照。元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が1増加し、`out`へ値を書き込む。失敗時はoffsetと`out`不変。
			 */
			bool ReadU8(std::uint8_t& out) noexcept;

			/**
			 * @brief big-endian 16bit unsigned値の読み取り。
			 * @param[out] out 読み取った値。
			 * @retval true 2byte読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out`は有効な参照。元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が2増加し、`out`へ値を書き込む。失敗時はoffsetと`out`不変。
			 * @code
			 * const std::uint8_t data[] = {0x12U, 0x34U};
			 * ket::byte_reader::Reader reader(data, 2U);
			 * std::uint16_t value = 0U;
			 * const auto ok = reader.ReadBe16(value);
			 * // ok == true, value == 0x1234U, reader.Offset() == 2
			 * @endcode
			 */
			bool ReadBe16(std::uint16_t& out) noexcept;

			/**
			 * @brief big-endian 32bit unsigned値の読み取り。
			 * @param[out] out 読み取った値。
			 * @retval true 4byte読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out`は有効な参照。元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が4増加し、`out`へ値を書き込む。失敗時はoffsetと`out`不変。
			 */
			bool ReadBe32(std::uint32_t& out) noexcept;

			/**
			 * @brief little-endian 16bit unsigned値の読み取り。
			 * @param[out] out 読み取った値。
			 * @retval true 2byte読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out`は有効な参照。元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が2増加し、`out`へ値を書き込む。失敗時はoffsetと`out`不変。
			 */
			bool ReadLe16(std::uint16_t& out) noexcept;

			/**
			 * @brief little-endian 32bit unsigned値の読み取り。
			 * @param[out] out 読み取った値。
			 * @retval true 4byte読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out`は有効な参照。元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が4増加し、`out`へ値を書き込む。失敗時はoffsetと`out`不変。
			 */
			bool ReadLe32(std::uint32_t& out) noexcept;

			/**
			 * @brief 指定byte数のnon-owning範囲読み取り。
			 * @param[out] out_data 読み取り開始位置を指すnon-owning pointer。
			 * @param[in] size 取り出すbyte数。
			 * @retval true `size`byteの範囲読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out_data`は有効な参照。返されたpointerのlifetimeは元bufferに従う。
			 * @post 成功時だけ`Offset()`が`size`増加し、`out_data`へ元buffer内の位置を書き込む。
			 * 失敗時はoffsetと`out_data`不変。
			 * @note validな空readerから0byteを読む場合、`out_data`はnullptr。
			 */
			bool ReadBytes(const std::uint8_t*& out_data, std::size_t size) noexcept;

		  private:
			const std::uint8_t* data_ = nullptr;
			std::size_t size_ = 0U;
			std::size_t offset_ = 0U;
		};

	} // namespace byte_reader

} // namespace ket
