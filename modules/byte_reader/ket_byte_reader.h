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
			 * @pre `data`は`size`byte以上読み取り可能な配列を指す。
			 * `data == nullptr && size > 0`はinvalid readerとして保持。
			 * @post `Offset()`は0。入力pointerとsizeを保持。
			 * @code
			 * const std::uint8_t data[] = {0x12U, 0x34U};
			 * ket::byte_reader::Reader reader(data, 2U);
			 * // reader.Size() == 2
			 * // reader.Offset() == 0
			 * @endcode
			 */
			Reader(const std::uint8_t* data, std::size_t size) noexcept;

			/**
			 * @brief reader状態のcopy構築。
			 * @param[in] other copy元reader。
			 * @pre なし。
			 * @post non-owning pointer、size、offsetを`other`と同じ値で保持。
			 * @code
			 * const std::uint8_t data[] = {0x12U};
			 * ket::byte_reader::Reader source(data, 1U);
			 * ket::byte_reader::Reader copied(source);
			 * // copied.Size() == 1
			 * // copied.Offset() == 0
			 * @endcode
			 */
			Reader(const Reader& other) noexcept = default;

			/**
			 * @brief reader状態のcopy代入。
			 * @param[in] other copy元reader。
			 * @retval reference `*this`。
			 * @pre なし。
			 * @post non-owning pointer、size、offsetを`other`と同じ値で保持。
			 * @code
			 * const std::uint8_t data[] = {0x12U};
			 * ket::byte_reader::Reader source(data, 1U);
			 * ket::byte_reader::Reader assigned(nullptr, 0U);
			 * assigned = source;
			 * // assigned.Size() == 1
			 * @endcode
			 */
			Reader& operator=(const Reader& other) noexcept = default;

			/**
			 * @brief reader状態のmove構築。
			 * @param[in,out] other move元reader。
			 * @pre なし。
			 * @post non-owning pointer、size、offsetをmove前の`other`と同じ値で保持。
			 * @code
			 * const std::uint8_t data[] = {0x12U};
			 * ket::byte_reader::Reader source(data, 1U);
			 * ket::byte_reader::Reader moved(static_cast<ket::byte_reader::Reader&&>(source));
			 * // moved.Size() == 1
			 * @endcode
			 */
			Reader(Reader&& other) noexcept = default;

			/**
			 * @brief reader状態のmove代入。
			 * @param[in,out] other move元reader。
			 * @retval reference `*this`。
			 * @pre なし。
			 * @post non-owning pointer、size、offsetをmove前の`other`と同じ値で保持。
			 * @code
			 * const std::uint8_t data[] = {0x12U};
			 * ket::byte_reader::Reader source(data, 1U);
			 * ket::byte_reader::Reader assigned(nullptr, 0U);
			 * assigned = static_cast<ket::byte_reader::Reader&&>(source);
			 * // assigned.Size() == 1
			 * @endcode
			 */
			Reader& operator=(Reader&& other) noexcept = default;

			/**
			 * @brief 構築時に渡したbuffer sizeの取得。
			 * @retval value 構築時に渡したbyte数。invalid readerでも入力sizeを返す。
			 * @pre なし。
			 * @post reader状態と外部状態の変更なし。
			 * @code
			 * const std::uint8_t data[] = {0x12U, 0x34U};
			 * ket::byte_reader::Reader reader(data, 2U);
			 * const auto size = reader.Size();
			 * // size == 2
			 * @endcode
			 */
			std::size_t Size() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 現在offsetの取得。
			 * @retval value 次に読み取るbyte位置。
			 * @pre なし。
			 * @post reader状態と外部状態の変更なし。
			 * @code
			 * const std::uint8_t data[] = {0x12U};
			 * ket::byte_reader::Reader reader(data, 1U);
			 * const auto offset = reader.Offset();
			 * // offset == 0
			 * @endcode
			 */
			std::size_t Offset() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 読み取り可能な残りbyte数の取得。
			 * @retval value valid readerでは`Size() - Offset()`。invalid readerでは0。
			 * @pre なし。
			 * @post reader状態と外部状態の変更なし。
			 * @code
			 * const std::uint8_t data[] = {0x12U, 0x34U};
			 * ket::byte_reader::Reader reader(data, 2U);
			 * const auto remaining = reader.Remaining();
			 * // remaining == 2
			 * @endcode
			 */
			std::size_t Remaining() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief valid readerのoffsetが末尾に到達したかの判定。
			 * @retval true readerがvalidで残りbyte数が0。
			 * @retval false invalid reader、内部不整合、または残りbyte数が1以上。
			 * @pre なし。
			 * @post reader状態と外部状態の変更なし。
			 * @code
			 * ket::byte_reader::Reader reader(nullptr, 0U);
			 * const auto empty = reader.Empty();
			 * // empty == true
			 * @endcode
			 */
			bool Empty() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 指定byte数の読み飛ばし。
			 * @param[in] size 読み飛ばすbyte数。
			 * @retval true `size`byteの読み飛ばし成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre 元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が`size`増加。失敗時はoffset不変。
			 * @code
			 * const std::uint8_t data[] = {0x12U, 0x34U};
			 * ket::byte_reader::Reader reader(data, 2U);
			 * const auto ok = reader.Skip(1U);
			 * // ok == true
			 * // reader.Offset() == 1
			 * @endcode
			 */
			bool Skip(std::size_t size) noexcept;

			/**
			 * @brief 1byte unsigned値の読み取り。
			 * @param[out] out 読み取った値。
			 * @retval true 1byte読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out`は有効な参照。元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が1増加し、`out`へ値を書き込む。失敗時はoffsetと`out`不変。
			 * @code
			 * const std::uint8_t data[] = {0x12U};
			 * ket::byte_reader::Reader reader(data, 1U);
			 * std::uint8_t value = 0U;
			 * const auto ok = reader.ReadU8(value);
			 * // ok == true, value == 0x12U, reader.Offset() == 1
			 * @endcode
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
			 * @code
			 * const std::uint8_t data[] = {0x12U, 0x34U, 0x56U, 0x78U};
			 * ket::byte_reader::Reader reader(data, 4U);
			 * std::uint32_t value = 0U;
			 * const auto ok = reader.ReadBe32(value);
			 * // ok == true, value == 0x12345678U, reader.Offset() == 4
			 * @endcode
			 */
			bool ReadBe32(std::uint32_t& out) noexcept;

			/**
			 * @brief little-endian 16bit unsigned値の読み取り。
			 * @param[out] out 読み取った値。
			 * @retval true 2byte読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out`は有効な参照。元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が2増加し、`out`へ値を書き込む。失敗時はoffsetと`out`不変。
			 * @code
			 * const std::uint8_t data[] = {0x12U, 0x34U};
			 * ket::byte_reader::Reader reader(data, 2U);
			 * std::uint16_t value = 0U;
			 * const auto ok = reader.ReadLe16(value);
			 * // ok == true, value == 0x3412U, reader.Offset() == 2
			 * @endcode
			 */
			bool ReadLe16(std::uint16_t& out) noexcept;

			/**
			 * @brief little-endian 32bit unsigned値の読み取り。
			 * @param[out] out 読み取った値。
			 * @retval true 4byte読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out`は有効な参照。元buffer lifetimeはreader利用中保持。
			 * @post 成功時だけ`Offset()`が4増加し、`out`へ値を書き込む。失敗時はoffsetと`out`不変。
			 * @code
			 * const std::uint8_t data[] = {0x12U, 0x34U, 0x56U, 0x78U};
			 * ket::byte_reader::Reader reader(data, 4U);
			 * std::uint32_t value = 0U;
			 * const auto ok = reader.ReadLe32(value);
			 * // ok == true, value == 0x78563412U, reader.Offset() == 4
			 * @endcode
			 */
			bool ReadLe32(std::uint32_t& out) noexcept;

			/**
			 * @brief 指定byte数のnon-owning範囲読み取り。
			 * @param[in] size 取り出すbyte数。
			 * @param[out] out_data 読み取り開始位置を指すnon-owning pointer。
			 * @retval true `size`byteの範囲読み取り成功。
			 * @retval false invalid reader、または残りbyte不足。
			 * @pre `out_data`は有効な参照。返されたpointerのlifetimeは元bufferに従う。
			 * @post 成功時だけ`Offset()`が`size`増加し、`out_data`へ元buffer内の位置を書き込む。
			 * 失敗時はoffsetと`out_data`不変。
			 * @note validな空readerから0byteを読む場合、`out_data`はnullptr。
			 * @code
			 * const std::uint8_t data[] = {0x10U, 0x20U, 0x30U};
			 * ket::byte_reader::Reader reader(data, 3U);
			 * const std::uint8_t* bytes = nullptr;
			 * const auto ok = reader.ReadBytes(2U, bytes);
			 * // ok == true, bytes == data, reader.Offset() == 2
			 * @endcode
			 */
			bool ReadBytes(std::size_t size, const std::uint8_t*& out_data) noexcept;

		  private:
			const std::uint8_t* data_ = nullptr;
			std::size_t size_ = 0U;
			std::size_t offset_ = 0U;
		};

	} // namespace byte_reader

} // namespace ket
