#pragma once

/**
 * @file ket_endian.h
 * @brief byte orderを明示した固定幅整数の読み書きAPI。
 *
 * @details unaligned accessやstrict aliasingに頼らず、byte列と16/32/64bit整数を相互変換する。
 * drop-in時は宣言と実装を同じ単位で持ち出す。plain Load/Storeはpreconditionを満たす短い経路、
 * Try Load/Storeはnullとsize不足を失敗値で扱う経路。
 *
 * @par プロジェクトへの適用方法
 * `ket_endian.h` と `ket_endian.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：endianとunaligned accessの意図を名前に出し、strict aliasing依存を避けられる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：C++20 `std::endian` は判定であり、byte列読み書きの直接代替ではない。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::endian
 * 内部実装：ket::endian::detail
 */

#include <cstddef>
#include <cstdint>

namespace ket
{
	namespace endian
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 16bit整数のbyte順反転。
		 * @param[in] value 反転対象の値。
		 * @retval value 上位byteと下位byteを入れ替えた値。
		 * @pre なし。任意のstd::uint16_t値を受け付ける。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::endian::ByteSwap16(static_cast<std::uint16_t>(0x1234U));
		 * // value == 0x3412
		 * @endcode
		 */
		constexpr std::uint16_t ByteSwap16(std::uint16_t value) noexcept;

		/**
		 * @brief 32bit整数のbyte順反転。
		 * @param[in] value 反転対象の値。
		 * @retval value 4byteの並びを逆順にした値。
		 * @pre なし。任意のstd::uint32_t値を受け付ける。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::endian::ByteSwap32(std::uint32_t{0x12345678U});
		 * // value == 0x78563412
		 * @endcode
		 */
		constexpr std::uint32_t ByteSwap32(std::uint32_t value) noexcept;

		/**
		 * @brief 64bit整数のbyte順反転。
		 * @param[in] value 反転対象の値。
		 * @retval value 8byteの並びを逆順にした値。
		 * @pre なし。任意のstd::uint64_t値を受け付ける。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * const auto value = ket::endian::ByteSwap64(std::uint64_t{0x0123456789ABCDEFULL});
		 * // value == 0xEFCDAB8967452301
		 * @endcode
		 */
		constexpr std::uint64_t ByteSwap64(std::uint64_t value) noexcept;

		/**
		 * @brief big-endian 2byte列の16bit整数読み取り。
		 * @param[in] data 読み取り対象の先頭byte。
		 * @retval value `data[0]`を上位byte、`data[1]`を下位byteとして組み立てた値。
		 * @pre `data`は2byte以上読み取り可能な領域を指す。nullptrはprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note 実装はbyte単位の読み取りとshift/orのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * const std::uint8_t data[] = {0x12U, 0x34U};
		 * const auto value = ket::endian::LoadBe16(data);
		 * // value == 0x1234
		 * @endcode
		 */
		std::uint16_t LoadBe16(const std::uint8_t* data) noexcept;

		/**
		 * @brief big-endian 4byte列の32bit整数読み取り。
		 * @param[in] data 読み取り対象の先頭byte。
		 * @retval value `data[0]`を最上位byteとして組み立てた値。
		 * @pre `data`は4byte以上読み取り可能な領域を指す。nullptrはprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note 実装はbyte単位の読み取りとshift/orのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * const std::uint8_t data[] = {0x12U, 0x34U, 0x56U, 0x78U};
		 * const auto value = ket::endian::LoadBe32(data);
		 * // value == 0x12345678
		 * @endcode
		 */
		std::uint32_t LoadBe32(const std::uint8_t* data) noexcept;

		/**
		 * @brief big-endian 8byte列の64bit整数読み取り。
		 * @param[in] data 読み取り対象の先頭byte。
		 * @retval value `data[0]`を最上位byteとして組み立てた値。
		 * @pre `data`は8byte以上読み取り可能な領域を指す。nullptrはprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note 実装はbyte単位の読み取りとshift/orのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * const std::uint8_t data[] = {0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU};
		 * const auto value = ket::endian::LoadBe64(data);
		 * // value == 0x0123456789ABCDEF
		 * @endcode
		 */
		std::uint64_t LoadBe64(const std::uint8_t* data) noexcept;

		/**
		 * @brief little-endian 2byte列の16bit整数読み取り。
		 * @param[in] data 読み取り対象の先頭byte。
		 * @retval value `data[0]`を下位byte、`data[1]`を上位byteとして組み立てた値。
		 * @pre `data`は2byte以上読み取り可能な領域を指す。nullptrはprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note 実装はbyte単位の読み取りとshift/orのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * const std::uint8_t data[] = {0x34U, 0x12U};
		 * const auto value = ket::endian::LoadLe16(data);
		 * // value == 0x1234
		 * @endcode
		 */
		std::uint16_t LoadLe16(const std::uint8_t* data) noexcept;

		/**
		 * @brief little-endian 4byte列の32bit整数読み取り。
		 * @param[in] data 読み取り対象の先頭byte。
		 * @retval value `data[0]`を最下位byteとして組み立てた値。
		 * @pre `data`は4byte以上読み取り可能な領域を指す。nullptrはprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note 実装はbyte単位の読み取りとshift/orのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * const std::uint8_t data[] = {0x78U, 0x56U, 0x34U, 0x12U};
		 * const auto value = ket::endian::LoadLe32(data);
		 * // value == 0x12345678
		 * @endcode
		 */
		std::uint32_t LoadLe32(const std::uint8_t* data) noexcept;

		/**
		 * @brief little-endian 8byte列の64bit整数読み取り。
		 * @param[in] data 読み取り対象の先頭byte。
		 * @retval value `data[0]`を最下位byteとして組み立てた値。
		 * @pre `data`は8byte以上読み取り可能な領域を指す。nullptrはprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note 実装はbyte単位の読み取りとshift/orのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * const std::uint8_t data[] = {0xEFU, 0xCDU, 0xABU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U};
		 * const auto value = ket::endian::LoadLe64(data);
		 * // value == 0x0123456789ABCDEF
		 * @endcode
		 */
		std::uint64_t LoadLe64(const std::uint8_t* data) noexcept;

		/**
		 * @brief 16bit整数のbig-endian 2byte列書き込み。
		 * @param[in,out] data 書き込み対象の先頭byte。
		 * @param[in] value 書き込む値。
		 * @retval void 戻り値なし。
		 * @pre `data`は2byte以上書き込み可能な領域を指す。nullptrはprecondition違反。
		 * @post `data[0]`と`data[1]`に`value`のbig-endian表現を書き込む。
		 * @note 実装はbyte単位の書き込みとshift/maskのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * std::uint8_t data[2] = {};
		 * ket::endian::StoreBe16(data, static_cast<std::uint16_t>(0x1234U));
		 * // data[0] == 0x12 && data[1] == 0x34
		 * @endcode
		 */
		void StoreBe16(std::uint8_t* data, std::uint16_t value) noexcept;

		/**
		 * @brief 32bit整数のbig-endian 4byte列書き込み。
		 * @param[in,out] data 書き込み対象の先頭byte。
		 * @param[in] value 書き込む値。
		 * @retval void 戻り値なし。
		 * @pre `data`は4byte以上書き込み可能な領域を指す。nullptrはprecondition違反。
		 * @post `data[0]`から`data[3]`に`value`のbig-endian表現を書き込む。
		 * @note 実装はbyte単位の書き込みとshift/maskのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * std::uint8_t data[4] = {};
		 * ket::endian::StoreBe32(data, std::uint32_t{0x12345678U});
		 * // data[0] == 0x12 && data[3] == 0x78
		 * @endcode
		 */
		void StoreBe32(std::uint8_t* data, std::uint32_t value) noexcept;

		/**
		 * @brief 64bit整数のbig-endian 8byte列書き込み。
		 * @param[in,out] data 書き込み対象の先頭byte。
		 * @param[in] value 書き込む値。
		 * @retval void 戻り値なし。
		 * @pre `data`は8byte以上書き込み可能な領域を指す。nullptrはprecondition違反。
		 * @post `data[0]`から`data[7]`に`value`のbig-endian表現を書き込む。
		 * @note 実装はbyte単位の書き込みとshift/maskのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * std::uint8_t data[8] = {};
		 * ket::endian::StoreBe64(data, std::uint64_t{0x0123456789ABCDEFULL});
		 * // data[0] == 0x01 && data[7] == 0xEF
		 * @endcode
		 */
		void StoreBe64(std::uint8_t* data, std::uint64_t value) noexcept;

		/**
		 * @brief 16bit整数のlittle-endian 2byte列書き込み。
		 * @param[in,out] data 書き込み対象の先頭byte。
		 * @param[in] value 書き込む値。
		 * @retval void 戻り値なし。
		 * @pre `data`は2byte以上書き込み可能な領域を指す。nullptrはprecondition違反。
		 * @post `data[0]`と`data[1]`に`value`のlittle-endian表現を書き込む。
		 * @note 実装はbyte単位の書き込みとshift/maskのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * std::uint8_t data[2] = {};
		 * ket::endian::StoreLe16(data, static_cast<std::uint16_t>(0x1234U));
		 * // data[0] == 0x34 && data[1] == 0x12
		 * @endcode
		 */
		void StoreLe16(std::uint8_t* data, std::uint16_t value) noexcept;

		/**
		 * @brief 32bit整数のlittle-endian 4byte列書き込み。
		 * @param[in,out] data 書き込み対象の先頭byte。
		 * @param[in] value 書き込む値。
		 * @retval void 戻り値なし。
		 * @pre `data`は4byte以上書き込み可能な領域を指す。nullptrはprecondition違反。
		 * @post `data[0]`から`data[3]`に`value`のlittle-endian表現を書き込む。
		 * @note 実装はbyte単位の書き込みとshift/maskのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * std::uint8_t data[4] = {};
		 * ket::endian::StoreLe32(data, std::uint32_t{0x12345678U});
		 * // data[0] == 0x78 && data[3] == 0x12
		 * @endcode
		 */
		void StoreLe32(std::uint8_t* data, std::uint32_t value) noexcept;

		/**
		 * @brief 64bit整数のlittle-endian 8byte列書き込み。
		 * @param[in,out] data 書き込み対象の先頭byte。
		 * @param[in] value 書き込む値。
		 * @retval void 戻り値なし。
		 * @pre `data`は8byte以上書き込み可能な領域を指す。nullptrはprecondition違反。
		 * @post `data[0]`から`data[7]`に`value`のlittle-endian表現を書き込む。
		 * @note 実装はbyte単位の書き込みとshift/maskのみを使い、reinterpret_castとunaligned
		 * accessに依存しない。
		 * @code
		 * std::uint8_t data[8] = {};
		 * ket::endian::StoreLe64(data, std::uint64_t{0x0123456789ABCDEFULL});
		 * // data[0] == 0xEF && data[7] == 0x01
		 * @endcode
		 */
		void StoreLe64(std::uint8_t* data, std::uint64_t value) noexcept;

		/**
		 * @brief big-endian 2byte列の16bit整数読み取り試行。
		 * @param[in] data 読み取り対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`から読み取り可能なbyte数。
		 * @param[out] out 読み取り成功時の出力先。
		 * @retval true 読み取り成功。
		 * @retval false `data`がnullptr、または`size < 2`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上読み取り可能な領域を指す。
		 * @post 成功時だけ`out`を変更。失敗時は`out`と外部状態の変更なし。
		 * @code
		 * const std::uint8_t data[] = {0x12U, 0x34U};
		 * std::uint16_t value = 0U;
		 * const auto ok = ket::endian::TryLoadBe16(data, sizeof(data), value);
		 * // ok == true && value == 0x1234
		 * @endcode
		 */
		bool TryLoadBe16(const std::uint8_t* data, std::size_t size, std::uint16_t& out) noexcept;

		/**
		 * @brief big-endian 4byte列の32bit整数読み取り試行。
		 * @param[in] data 読み取り対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`から読み取り可能なbyte数。
		 * @param[out] out 読み取り成功時の出力先。
		 * @retval true 読み取り成功。
		 * @retval false `data`がnullptr、または`size < 4`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上読み取り可能な領域を指す。
		 * @post 成功時だけ`out`を変更。失敗時は`out`と外部状態の変更なし。
		 * @code
		 * const std::uint8_t data[] = {0x12U, 0x34U, 0x56U, 0x78U};
		 * std::uint32_t value = 0U;
		 * const auto ok = ket::endian::TryLoadBe32(data, sizeof(data), value);
		 * // ok == true && value == 0x12345678
		 * @endcode
		 */
		bool TryLoadBe32(const std::uint8_t* data, std::size_t size, std::uint32_t& out) noexcept;

		/**
		 * @brief big-endian 8byte列の64bit整数読み取り試行。
		 * @param[in] data 読み取り対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`から読み取り可能なbyte数。
		 * @param[out] out 読み取り成功時の出力先。
		 * @retval true 読み取り成功。
		 * @retval false `data`がnullptr、または`size < 8`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上読み取り可能な領域を指す。
		 * @post 成功時だけ`out`を変更。失敗時は`out`と外部状態の変更なし。
		 * @code
		 * const std::uint8_t data[] = {0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU};
		 * std::uint64_t value = 0U;
		 * const auto ok = ket::endian::TryLoadBe64(data, sizeof(data), value);
		 * // ok == true && value == 0x0123456789ABCDEF
		 * @endcode
		 */
		bool TryLoadBe64(const std::uint8_t* data, std::size_t size, std::uint64_t& out) noexcept;

		/**
		 * @brief little-endian 2byte列の16bit整数読み取り試行。
		 * @param[in] data 読み取り対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`から読み取り可能なbyte数。
		 * @param[out] out 読み取り成功時の出力先。
		 * @retval true 読み取り成功。
		 * @retval false `data`がnullptr、または`size < 2`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上読み取り可能な領域を指す。
		 * @post 成功時だけ`out`を変更。失敗時は`out`と外部状態の変更なし。
		 * @code
		 * const std::uint8_t data[] = {0x34U, 0x12U};
		 * std::uint16_t value = 0U;
		 * const auto ok = ket::endian::TryLoadLe16(data, sizeof(data), value);
		 * // ok == true && value == 0x1234
		 * @endcode
		 */
		bool TryLoadLe16(const std::uint8_t* data, std::size_t size, std::uint16_t& out) noexcept;

		/**
		 * @brief little-endian 4byte列の32bit整数読み取り試行。
		 * @param[in] data 読み取り対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`から読み取り可能なbyte数。
		 * @param[out] out 読み取り成功時の出力先。
		 * @retval true 読み取り成功。
		 * @retval false `data`がnullptr、または`size < 4`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上読み取り可能な領域を指す。
		 * @post 成功時だけ`out`を変更。失敗時は`out`と外部状態の変更なし。
		 * @code
		 * const std::uint8_t data[] = {0x78U, 0x56U, 0x34U, 0x12U};
		 * std::uint32_t value = 0U;
		 * const auto ok = ket::endian::TryLoadLe32(data, sizeof(data), value);
		 * // ok == true && value == 0x12345678
		 * @endcode
		 */
		bool TryLoadLe32(const std::uint8_t* data, std::size_t size, std::uint32_t& out) noexcept;

		/**
		 * @brief little-endian 8byte列の64bit整数読み取り試行。
		 * @param[in] data 読み取り対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`から読み取り可能なbyte数。
		 * @param[out] out 読み取り成功時の出力先。
		 * @retval true 読み取り成功。
		 * @retval false `data`がnullptr、または`size < 8`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上読み取り可能な領域を指す。
		 * @post 成功時だけ`out`を変更。失敗時は`out`と外部状態の変更なし。
		 * @code
		 * const std::uint8_t data[] = {0xEFU, 0xCDU, 0xABU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U};
		 * std::uint64_t value = 0U;
		 * const auto ok = ket::endian::TryLoadLe64(data, sizeof(data), value);
		 * // ok == true && value == 0x0123456789ABCDEF
		 * @endcode
		 */
		bool TryLoadLe64(const std::uint8_t* data, std::size_t size, std::uint64_t& out) noexcept;

		/**
		 * @brief 16bit整数のbig-endian 2byte列書き込み試行。
		 * @param[in,out] data 書き込み対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`へ書き込み可能なbyte数。
		 * @param[in] value 書き込む値。
		 * @retval true 書き込み成功。
		 * @retval false `data`がnullptr、または`size < 2`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上書き込み可能な領域を指す。
		 * @post 成功時だけbufferを変更。失敗時はbufferと外部状態の変更なし。
		 * @code
		 * std::uint8_t data[2] = {};
		 * const auto ok = ket::endian::TryStoreBe16(data, sizeof(data), 0x1234U);
		 * // ok == true && data[0] == 0x12 && data[1] == 0x34
		 * @endcode
		 */
		bool TryStoreBe16(std::uint8_t* data, std::size_t size, std::uint16_t value) noexcept;

		/**
		 * @brief 32bit整数のbig-endian 4byte列書き込み試行。
		 * @param[in,out] data 書き込み対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`へ書き込み可能なbyte数。
		 * @param[in] value 書き込む値。
		 * @retval true 書き込み成功。
		 * @retval false `data`がnullptr、または`size < 4`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上書き込み可能な領域を指す。
		 * @post 成功時だけbufferを変更。失敗時はbufferと外部状態の変更なし。
		 * @code
		 * std::uint8_t data[4] = {};
		 * const auto ok = ket::endian::TryStoreBe32(data, sizeof(data), 0x12345678U);
		 * // ok == true && data[0] == 0x12 && data[3] == 0x78
		 * @endcode
		 */
		bool TryStoreBe32(std::uint8_t* data, std::size_t size, std::uint32_t value) noexcept;

		/**
		 * @brief 64bit整数のbig-endian 8byte列書き込み試行。
		 * @param[in,out] data 書き込み対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`へ書き込み可能なbyte数。
		 * @param[in] value 書き込む値。
		 * @retval true 書き込み成功。
		 * @retval false `data`がnullptr、または`size < 8`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上書き込み可能な領域を指す。
		 * @post 成功時だけbufferを変更。失敗時はbufferと外部状態の変更なし。
		 * @code
		 * std::uint8_t data[8] = {};
		 * const auto ok = ket::endian::TryStoreBe64(data, sizeof(data), 0x0123456789ABCDEFULL);
		 * // ok == true && data[0] == 0x01 && data[7] == 0xEF
		 * @endcode
		 */
		bool TryStoreBe64(std::uint8_t* data, std::size_t size, std::uint64_t value) noexcept;

		/**
		 * @brief 16bit整数のlittle-endian 2byte列書き込み試行。
		 * @param[in,out] data 書き込み対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`へ書き込み可能なbyte数。
		 * @param[in] value 書き込む値。
		 * @retval true 書き込み成功。
		 * @retval false `data`がnullptr、または`size < 2`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上書き込み可能な領域を指す。
		 * @post 成功時だけbufferを変更。失敗時はbufferと外部状態の変更なし。
		 * @code
		 * std::uint8_t data[2] = {};
		 * const auto ok = ket::endian::TryStoreLe16(data, sizeof(data), 0x1234U);
		 * // ok == true && data[0] == 0x34 && data[1] == 0x12
		 * @endcode
		 */
		bool TryStoreLe16(std::uint8_t* data, std::size_t size, std::uint16_t value) noexcept;

		/**
		 * @brief 32bit整数のlittle-endian 4byte列書き込み試行。
		 * @param[in,out] data 書き込み対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`へ書き込み可能なbyte数。
		 * @param[in] value 書き込む値。
		 * @retval true 書き込み成功。
		 * @retval false `data`がnullptr、または`size < 4`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上書き込み可能な領域を指す。
		 * @post 成功時だけbufferを変更。失敗時はbufferと外部状態の変更なし。
		 * @code
		 * std::uint8_t data[4] = {};
		 * const auto ok = ket::endian::TryStoreLe32(data, sizeof(data), 0x12345678U);
		 * // ok == true && data[0] == 0x78 && data[3] == 0x12
		 * @endcode
		 */
		bool TryStoreLe32(std::uint8_t* data, std::size_t size, std::uint32_t value) noexcept;

		/**
		 * @brief 64bit整数のlittle-endian 8byte列書き込み試行。
		 * @param[in,out] data 書き込み対象の先頭byte。nullptrは失敗値として扱う。
		 * @param[in] size `data`へ書き込み可能なbyte数。
		 * @param[in] value 書き込む値。
		 * @retval true 書き込み成功。
		 * @retval false `data`がnullptr、または`size < 8`。
		 * @pre `data`がnullptrでない場合、`data`は`size`byte以上書き込み可能な領域を指す。
		 * @post 成功時だけbufferを変更。失敗時はbufferと外部状態の変更なし。
		 * @code
		 * std::uint8_t data[8] = {};
		 * const auto ok = ket::endian::TryStoreLe64(data, sizeof(data), 0x0123456789ABCDEFULL);
		 * // ok == true && data[0] == 0xEF && data[7] == 0x01
		 * @endcode
		 */
		bool TryStoreLe64(std::uint8_t* data, std::size_t size, std::uint64_t value) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/** @brief 32bit演算用の1byte mask。 */
			constexpr std::uint32_t kByteMask32 = 0x000000FFU;

			/** @brief 64bit演算用の1byte mask。 */
			constexpr std::uint64_t kByteMask64 = 0x00000000000000FFULL;

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		constexpr std::uint16_t ByteSwap16(std::uint16_t value) noexcept
		{
			return static_cast<std::uint16_t>(
				((static_cast<std::uint32_t>(value) & detail::kByteMask32) << 8U) |
				((static_cast<std::uint32_t>(value) >> 8U) & detail::kByteMask32));
		}

		constexpr std::uint32_t ByteSwap32(std::uint32_t value) noexcept
		{
			return ((value & detail::kByteMask32) << 24U) | ((value & 0x0000FF00U) << 8U) |
				((value >> 8U) & 0x0000FF00U) | ((value >> 24U) & detail::kByteMask32);
		}

		constexpr std::uint64_t ByteSwap64(std::uint64_t value) noexcept
		{
			return ((value & detail::kByteMask64) << 56U) |
				((value & 0x000000000000FF00ULL) << 40U) |
				((value & 0x0000000000FF0000ULL) << 24U) | ((value & 0x00000000FF000000ULL) << 8U) |
				((value >> 8U) & 0x00000000FF000000ULL) | ((value >> 24U) & 0x0000000000FF0000ULL) |
				((value >> 40U) & 0x000000000000FF00ULL) | ((value >> 56U) & detail::kByteMask64);
		}

	} // namespace endian

} // namespace ket
