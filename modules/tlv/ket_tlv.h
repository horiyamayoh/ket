#pragma once

/**
 * @file ket_tlv.h
 * @brief 小さいTLV recordのencode/decode API。
 *
 * @details `type:uint16 big-endian`、`length:uint32 big-endian`、`value bytes`
 * の1 recordだけを扱う。schema languageや複数record iteratorではなく、
 * wire format境界とdecode失敗時の出力不変性を短いAPIへ集約する。
 * drop-in時は宣言と実装を同じ単位で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_tlv.h` と `ket_tlv.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::vector<std::uint8_t>` とbool+outでwire format境界を固定できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::tlv
 * 内部実装：.cpp の無名 namespace
 */

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ket
{
	namespace tlv
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief decode結果が参照するTLV value view。
		 * @note `value`は入力bufferを指すnon-owning pointer。呼び出し側が入力buffer
		 * lifetimeを保持。
		 */
		struct View
		{
			std::uint16_t type = 0U;
			const std::uint8_t* value = nullptr;
			std::uint32_t value_size = 0U;
		};

		/**
		 * @brief 先頭1 recordのdecode結果。
		 * @note `consumed`はTLV header 6 bytesと`view.value_size`の合計。
		 */
		struct DecodeResult
		{
			View view;
			std::size_t consumed = 0U;
		};

		/**
		 * @brief 1つのTLV recordをbyte列へencode。
		 * @param[in] type wire headerへbig-endianで格納するtype。
		 * @param[in] value 追加するvalue bytes。`value_size == 0`の場合のみ`nullptr`可。
		 * @param[in] value_size 追加するvalue byte数。wire headerへbig-endian uint32で格納。
		 * @retval value `type:uint16 big-endian` + `length:uint32 big-endian` + value bytes。
		 * @pre `value_size > 0`の場合、`value`は`value_size`バイト以上読み取り可能な配列を指す。
		 * `value == nullptr && value_size > 0`はprecondition違反。
		 * @post 引数と外部状態の変更なし。
		 * @note record sizeがstd::vector::max_size()を超える場合はstd::length_error送出。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @code
		 * const std::uint8_t value[] = {0xAAU};
		 * const auto record = ket::tlv::Encode(0x1234U, value, 1U);
		 * // record.size() == 7, record[0] == 0x12, record[6] == 0xAA
		 * @endcode
		 */
		std::vector<std::uint8_t>
		Encode(std::uint16_t type, const std::uint8_t* value, std::uint32_t value_size);

		/**
		 * @brief 1つのTLV recordを既存byte列の末尾へ追加。
		 * @param[in,out] dst 追加先のbyte列。
		 * @param[in] type wire headerへbig-endianで格納するtype。
		 * @param[in] value 追加するvalue bytes。`value_size == 0`の場合のみ`nullptr`可。
		 * @param[in] value_size 追加するvalue byte数。wire headerへbig-endian uint32で格納。
		 * @retval void 戻り値なし。
		 * @pre `dst`は有効なstd::vector。`value_size > 0`の場合、`value`は`value_size`バイト以上
		 * 読み取り可能な配列を指す。`value`は`dst`の内部storageを指していてもよい。
		 * `value == nullptr && value_size > 0`はprecondition違反。
		 * @post `dst`の既存内容を保持し、末尾にTLV recordを追加。
		 * @note record sizeまたは出力合計がstd::vector::max_size()を超える場合は
		 * std::length_error送出。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @note 例外時は`dst`を変更しない強い例外保証。
		 * @code
		 * std::vector<std::uint8_t> output;
		 * const std::uint8_t value[] = {0xAAU};
		 * ket::tlv::Append(output, 0x1234U, value, 1U);
		 * // output.size() == 7, output[0] == 0x12, output[6] == 0xAA
		 * @endcode
		 */
		void Append(std::vector<std::uint8_t>& dst,
					std::uint16_t type,
					const std::uint8_t* value,
					std::uint32_t value_size);

		/**
		 * @brief 入力先頭の1つのTLV recordをdecode。
		 * @param[in] data decode対象のbyte列。`nullptr`の場合は`size`によらず失敗。
		 * @param[in] size `data`のbyte数。
		 * @param[out] out decode成功時に先頭recordのviewと消費byte数を格納。
		 * @retval true 先頭1 recordをdecodeし、`out`を更新。
		 * @retval false `data == nullptr`、6 bytes未満、declared length過大、
		 * または消費byte数のstd::size_t overflow。`out`は不変。
		 * @pre `data != nullptr`の場合、`data`は`size`バイト以上読み取り可能な配列を指す。
		 * @post 成功時だけ`out`を更新。失敗時は`out`を変更なし。
		 * @note `out.view.value`は入力bufferを指すnon-owning pointer。
		 * decode後の参照有効性は入力buffer lifetimeに依存。
		 * @code
		 * const std::uint8_t record[] = {0x12U, 0x34U, 0x00U, 0x00U, 0x00U, 0x01U, 0xAAU};
		 * ket::tlv::DecodeResult result{};
		 * const auto ok = ket::tlv::TryDecode(record, sizeof(record), result);
		 * // ok == true, result.view.type == 0x1234, result.view.value_size == 1
		 * @endcode
		 */
		bool TryDecode(const std::uint8_t* data, std::size_t size, DecodeResult& out) noexcept;

	} // namespace tlv

} // namespace ket
