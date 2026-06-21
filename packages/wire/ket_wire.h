#pragma once

/**
 * @file ket_wire.h
 * @brief schema driven binary wire layout runtime.
 *
 * @details C++ struct and fixed or mostly fixed binary wire layouts are converted through a
 * schema that owns field descriptors. The runtime keeps byte order, field offsets, encoded size,
 * BCD validation, grouped bit packing, reserved bytes, exact/prefix decode, and named diagnostics
 * in one public API. It does not provide transport, IDL generation, arbitrary object serialization,
 * logging, checksum algorithms, or TLV tree handling.
 *
 * @par プロジェクトへの適用方法
 * `ket_wire.h` と依存する `ket_byte_view.h` を対象プロジェクトへコピー。ヘッダオンリー
 * package runtime。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::optional`、`std::array`、`std::string_view`、
 * `std::vector<std::uint8_t>`により、失敗値、field列所有、名前付きdiagnostics、所有byte列を
 * public APIで明示可能。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリ、および ket::byte_view。
 * `ket-wire` は ordinary module ではなく package runtime であり、複数 ket module を合成可能。
 *
 * @par namespace
 * 公開API：ket::wire
 * 内部実装：ket::wire::detail
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits> // IWYU pragma: keep
#include <utility>
#include <vector>

#include "ket_byte_view.h"

namespace ket
{
	namespace wire
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief wire operation failure kind.
		 */
		enum class Error : std::uint8_t
		{
			kNone,
			kInvalidInputView,
			kInvalidOutputView,
			kShortInput,
			kShortOutput,
			kTrailingBytes,
			kInvalidBcd,
			kValueOutOfRange,
			kReservedMismatch,
			kLengthMismatch,
			kChecksumMismatch,
			kCallbackFailed,
			kSizeOverflow,
			kSchemaError
		};

		/**
		 * @brief field descriptor category.
		 */
		enum class FieldKind : std::uint8_t
		{
			kInteger,
			kBcdInteger,
			kRawBcdBytes,
			kBytes,
			kViewBytes,
			kConstBytes,
			kReservedBytes,
			kPadBytes,
			kBits,
			kValidation
		};

		// NOLINTBEGIN(misc-non-private-member-variables-in-classes)

		/**
		 * @brief operation diagnostic without allocation.
		 */
		struct Status
		{
			Error error = Error::kNone;
			std::size_t offset = 0U;
			std::string_view field;
			std::string_view group;
			std::size_t required_size = 0U;
			std::size_t available_size = 0U;
			std::uint64_t expected = 0U;
			std::uint64_t actual = 0U;
			bool has_expected = false;
			bool has_actual = false;

			/**
			 * @brief success status判定。
			 * @retval true `error == Error::kNone`。
			 * @retval false failure kindを保持。
			 * @pre なし。
			 * @post statusと外部状態の変更なし。
			 */
			[[nodiscard]] bool Ok() const noexcept;
		};

		/**
		 * @brief grouped bit descriptor内のlogical bit member.
		 * @tparam T schema対象型。
		 */
		template <typename T>
		struct BitMember
		{
			using GetFunction = bool (*)(const T& value,
										 const BitMember& member,
										 std::uint64_t& out,
										 Status& status) noexcept;
			using SetFunction = void (*)(T& value, std::uint64_t raw) noexcept;

			std::string_view name;
			unsigned shift = 0U;
			unsigned width = 0U;
			std::uint64_t expected = 0U;
			bool has_member = false;
			bool is_valid = false;
			GetFunction get = nullptr;
			SetFunction set = nullptr;
		};

		/**
		 * @brief schema field descriptor.
		 * @tparam T schema対象型。
		 */
		template <typename T>
		struct Field
		{
			using DecodeFunction = bool (*)(ket::byte_view::View data,
											std::size_t& offset,
											T& value,
											const Field& field,
											Status& status) noexcept;
			using PreflightFunction = bool (*)(const T& value,
											   const Field& field,
											   Status& status) noexcept;
			using EncodeFunction = void (*)(const T& value,
											std::uint8_t* out,
											std::size_t& offset,
											const Field& field) noexcept;
			using MeasureFunction = bool (*)(const T& value,
											 const Field& field,
											 std::size_t& encoded_size,
											 Status& status) noexcept;
			using ValidationFunction = bool (*)(const T& value, Status& status) noexcept;

			std::string_view name;
			std::string_view group;
			FieldKind kind = FieldKind::kValidation;
			std::size_t encoded_size = 0U;
			std::size_t max_encoded_size = 0U;
			bool is_fixed_size = true;
			bool is_valid = false;
			std::uint64_t expected = 0U;
			const std::uint8_t* expected_bytes = nullptr;
			std::size_t expected_bytes_size = 0U;
			std::array<BitMember<T>, 16U> bit_members{};
			std::size_t bit_member_count = 0U;
			DecodeFunction decode = nullptr;
			PreflightFunction preflight = nullptr;
			EncodeFunction encode = nullptr;
			MeasureFunction measure = nullptr;
			ValidationFunction validation = nullptr;
		};

		/**
		 * @brief schema descriptor owning its field array.
		 * @tparam T schema対象型。
		 * @tparam FieldCount field descriptor数。
		 */
		template <typename T, std::size_t FieldCount>
		struct Schema
		{
			std::string_view name;
			std::array<Field<T>, FieldCount> fields{};
			std::size_t field_count = FieldCount;
			bool is_fixed_size = true;
			std::size_t fixed_size = 0U;
			std::size_t max_size = 0U;
			Status status{};
		};

		/**
		 * @brief decode operation result.
		 * @tparam T decode対象型。
		 */
		template <typename T>
		struct DecodeResult
		{
			std::optional<T> value;
			Status status{};
			std::size_t consumed = 0U;
		};

		/**
		 * @brief owning encode operation result.
		 */
		struct EncodeResult
		{
			std::optional<std::vector<std::uint8_t>> bytes;
			Status status{};
		};

		/**
		 * @brief fixed-buffer encode operation result.
		 */
		struct EncodeToResult
		{
			Status status{};
			std::size_t encoded_size = 0U;
		};

		/**
		 * @brief encoded size measurement result.
		 */
		struct SizeResult
		{
			std::optional<std::size_t> value;
			Status status{};
		};

		// NOLINTEND(misc-non-private-member-variables-in-classes)

		/**
		 * @brief validation hook function pointer.
		 * @tparam T schema対象型。
		 */
		template <typename T>
		using ValidationCallback = bool (*)(const T& value, Status& status) noexcept;

		/**
		 * @brief schemaをfield配列から構築。
		 * @tparam T schema対象型。
		 * @tparam FieldCount field descriptor数。
		 * @param[in] name message名。schema利用中に参照可能なstorageを要求。
		 * @param[in] fields schemaが所有するfield descriptor列。
		 * @retval value 構築済みschema。schema errorは`Schema::status`に保持。
		 * @pre `fields`内の各descriptorはfactoryから生成された値、または同じcontractを満たす値。
		 * @post `fields`はschema内へcopyされ、caller-held配列のlifetimeに依存しない。
		 */
		template <typename T, std::size_t FieldCount>
		Schema<T, FieldCount> MakeSchema(std::string_view name,
										 std::array<Field<T>, FieldCount> fields) noexcept;

		/**
		 * @brief schema全体を読み、input全消費を要求するdecode。
		 * @tparam T decode対象型。
		 * @tparam FieldCount field descriptor数。
		 * @param[in] data decode対象byte view。
		 * @param[in] schema decode schema。
		 * @retval value 成功時はdecode済みobject。
		 * @retval std::nullopt 失敗時。
		 * @pre `T`はdefault construction可能。`data`の元buffer lifetimeは呼び出し中維持。
		 * @post 成功時だけresult valueを保持。input bufferの変更なし。失敗時のconsumedは0。
		 */
		template <typename T, std::size_t FieldCount>
		DecodeResult<T> DecodeExact(ket::byte_view::View data, const Schema<T, FieldCount>& schema);

		/**
		 * @brief schemaで定義された先頭messageだけを読み、trailing bytesを許すdecode。
		 * @tparam T decode対象型。
		 * @tparam FieldCount field descriptor数。
		 * @param[in] data decode対象byte view。
		 * @param[in] schema decode schema。
		 * @retval value 成功時はdecode済みobject。
		 * @retval std::nullopt 失敗時。
		 * @pre `T`はdefault construction可能。`data`の元buffer lifetimeは呼び出し中維持。
		 * @post 成功時だけconsumedへ実消費byte数を保持。input bufferの変更なし。
		 */
		template <typename T, std::size_t FieldCount>
		DecodeResult<T> DecodePrefix(ket::byte_view::View data,
									 const Schema<T, FieldCount>& schema);

		/**
		 * @brief objectを所有byte列へencode。
		 * @tparam T encode対象型。
		 * @tparam FieldCount field descriptor数。
		 * @param[in] value encode対象object。
		 * @param[in] schema encode schema。
		 * @retval value 成功時はencoded bytes。
		 * @retval std::nullopt 失敗時。
		 * @pre schema内のview fieldは参照元byte列のlifetimeを呼び出し中維持。
		 * @post 成功時だけbytesを保持。`value`の変更なし。
		 */
		template <typename T, std::size_t FieldCount>
		EncodeResult Encode(const T& value, const Schema<T, FieldCount>& schema);

		/**
		 * @brief objectをcaller-owned fixed bufferへencode。
		 * @tparam T encode対象型。
		 * @tparam FieldCount field descriptor数。
		 * @param[in] value encode対象object。
		 * @param[in] schema encode schema。
		 * @param[in,out] out 書き込み先mutable byte view。
		 * @retval value `EncodeToResult::encoded_size`に書き込みbyte数を保持。
		 * @retval failure `EncodeToResult::status`に理由を保持。
		 * @pre `out`の元buffer lifetimeと書き込み可能性は呼び出し中維持。
		 * @post 成功時だけ先頭encoded_size byteを更新。失敗時はoutput bufferを変更しない。
		 */
		template <typename T, std::size_t FieldCount>
		EncodeToResult EncodeTo(const T& value,
								const Schema<T, FieldCount>& schema,
								ket::byte_view::MutableView out);

		/**
		 * @brief fixed-size schemaのencoded size取得。
		 * @tparam T schema対象型。
		 * @tparam FieldCount field descriptor数。
		 * @param[in] schema 対象schema。
		 * @retval value fixed encoded size。
		 * @retval std::nullopt schema error、またはvalue依存size。
		 * @pre なし。
		 * @post schemaと外部状態の変更なし。
		 */
		template <typename T, std::size_t FieldCount>
		std::optional<std::size_t> EncodedSize(const Schema<T, FieldCount>& schema) noexcept;

		/**
		 * @brief bounded schemaの最大encoded size取得。
		 * @tparam T schema対象型。
		 * @tparam FieldCount field descriptor数。
		 * @param[in] schema 対象schema。
		 * @retval value 最大encoded size。
		 * @retval std::nullopt schema error、または最大値が固定されないschema。
		 * @pre なし。
		 * @post schemaと外部状態の変更なし。
		 */
		template <typename T, std::size_t FieldCount>
		std::optional<std::size_t> MaxEncodedSize(const Schema<T, FieldCount>& schema) noexcept;

		/**
		 * @brief schema encoded sizeがvalue非依存かを判定。
		 * @tparam T schema対象型。
		 * @tparam FieldCount field descriptor数。
		 * @param[in] schema 対象schema。
		 * @retval true schemaがvalidでfixed-size。
		 * @retval false schema error、またはvalue依存size。
		 * @pre なし。
		 * @post schemaと外部状態の変更なし。
		 */
		template <typename T, std::size_t FieldCount>
		bool IsFixedSize(const Schema<T, FieldCount>& schema) noexcept;

		/**
		 * @brief valueを含めたencoded size測定。
		 * @tparam T schema対象型。
		 * @tparam FieldCount field descriptor数。
		 * @param[in] value 測定対象object。
		 * @param[in] schema 対象schema。
		 * @retval value encoded size。
		 * @retval std::nullopt schema error、size overflow、またはvalue検証失敗。
		 * @pre schema内のview fieldは参照元byte列のlifetimeを呼び出し中維持。
		 * @post `value`、schema、外部状態の変更なし。
		 */
		template <typename T, std::size_t FieldCount>
		SizeResult MeasureEncodedSize(const T& value, const Schema<T, FieldCount>& schema);

		/**
		 * @brief unsigned 8bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::uint8_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> U8(std::string_view name) noexcept;

		/**
		 * @brief big-endian unsigned 16bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::uint16_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> U16Be(std::string_view name) noexcept;

		/**
		 * @brief little-endian unsigned 16bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::uint16_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> U16Le(std::string_view name) noexcept;

		/**
		 * @brief big-endian unsigned 32bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::uint32_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> U32Be(std::string_view name) noexcept;

		/**
		 * @brief little-endian unsigned 32bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::uint32_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> U32Le(std::string_view name) noexcept;

		/**
		 * @brief big-endian unsigned 64bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::uint64_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> U64Be(std::string_view name) noexcept;

		/**
		 * @brief little-endian unsigned 64bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::uint64_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> U64Le(std::string_view name) noexcept;

		/**
		 * @brief signed 8bit two's complement field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::int8_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> I8(std::string_view name) noexcept;

		/**
		 * @brief big-endian signed 16bit two's complement field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::int16_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> I16Be(std::string_view name) noexcept;

		/**
		 * @brief little-endian signed 16bit two's complement field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::int16_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> I16Le(std::string_view name) noexcept;

		/**
		 * @brief big-endian signed 32bit two's complement field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::int32_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> I32Be(std::string_view name) noexcept;

		/**
		 * @brief little-endian signed 32bit two's complement field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::int32_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> I32Le(std::string_view name) noexcept;

		/**
		 * @brief big-endian signed 64bit two's complement field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::int64_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> I64Be(std::string_view name) noexcept;

		/**
		 * @brief little-endian signed 64bit two's complement field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::int64_t T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> I64Le(std::string_view name) noexcept;

		/**
		 * @brief packed BCD 1byte decoded integer field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `int T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> BcdU8(std::string_view name) noexcept;

		/**
		 * @brief big-endian packed BCD 2byte decoded integer field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `int T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> BcdU16Be(std::string_view name) noexcept;

		/**
		 * @brief little-endian packed BCD 2byte decoded integer field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `int T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> BcdU16Le(std::string_view name) noexcept;

		/**
		 * @brief big-endian packed BCD 4byte decoded integer field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `int T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> BcdU32Be(std::string_view name) noexcept;

		/**
		 * @brief little-endian packed BCD 4byte decoded integer field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `int T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post allocationなし。
		 */
		template <typename T, auto Member>
		Field<T> BcdU32Le(std::string_view name) noexcept;

		/**
		 * @brief raw packed BCD bytes copy field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::array<std::uint8_t, N> T::*` または `std::uint8_t[N] T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre member storageはfixed byte array。
		 * @post decodeはsource pointerを保存せず、raw bytesをcopy。
		 */
		template <typename T, auto Member>
		Field<T> RawBcdBytes(std::string_view name) noexcept;

		/**
		 * @brief fixed bytes copy field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `std::array<std::uint8_t, N> T::*` または `std::uint8_t[N] T::*`。
		 * @param[in] name field名。
		 * @retval value field descriptor。
		 * @pre member storageはfixed byte array。
		 * @post decodeはsource pointerを保存せず、bytesをcopy。
		 */
		template <typename T, auto Member>
		Field<T> Bytes(std::string_view name) noexcept;

		/**
		 * @brief fixed-size byte view field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member `ket::byte_view::View T::*`。
		 * @param[in] name field名。
		 * @param[in] size view fieldのencoded byte数。
		 * @retval value field descriptor。
		 * @pre decode後のview利用中はdecode元buffer lifetimeをcallerが保持。
		 * @post decodeはsource pointerを意図的にmember viewへ保持。
		 */
		template <typename T, auto Member>
		Field<T> ViewBytes(std::string_view name, std::size_t size) noexcept;

		/**
		 * @brief constant 1byte field descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name field名。
		 * @param[in] expected 期待するwire byte。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post encode時は`expected`を書き込み。
		 */
		template <typename T>
		Field<T> ConstU8(std::string_view name, std::uint8_t expected) noexcept;

		/**
		 * @brief constant fixed bytes field descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name field名。
		 * @param[in] expected 期待するbyte列先頭。`size == 0`の場合のみnullptr可。
		 * @param[in] size 期待するbyte数。
		 * @retval value field descriptor。
		 * @pre `expected`はschema利用中に参照可能なstorage。
		 * @post encode時は`expected[0..size)`を書き込み。
		 */
		template <typename T>
		Field<T>
		ConstBytes(std::string_view name, const std::uint8_t* expected, std::size_t size) noexcept;

		/**
		 * @brief repeated reserved bytes field descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name field名。
		 * @param[in] size reserved byte数。
		 * @param[in] expected 各byteの期待値。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post decodeは全byte一致を検査し、encodeは期待値を埋める。
		 */
		template <typename T>
		Field<T>
		ReservedBytes(std::string_view name, std::size_t size, std::uint8_t expected = 0U) noexcept;

		/**
		 * @brief repeated padding bytes field descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name field名。
		 * @param[in] size padding byte数。
		 * @param[in] value 各padding byteの値。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post decodeは全byte一致を検査し、encodeはpadding値を埋める。
		 */
		template <typename T>
		Field<T>
		PadBytes(std::string_view name, std::size_t size, std::uint8_t value = 0U) noexcept;

		/**
		 * @brief grouped bit member descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Member unsigned integral member pointer。
		 * @tparam Shift storage unit内のLSB基準shift。
		 * @tparam Width logical field bit幅。
		 * @param[in] name logical field名。
		 * @retval value bit member descriptor。
		 * @pre `Shift + Width`はstorage descriptorのbit幅以下。
		 * @post allocationなし。
		 */
		template <typename T, auto Member, unsigned Shift, unsigned Width>
		BitMember<T> Bit(std::string_view name) noexcept;

		/**
		 * @brief grouped reserved bit member descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam Shift storage unit内のLSB基準shift。
		 * @tparam Width reserved bit幅。
		 * @param[in] name logical reserved field名。
		 * @param[in] expected reserved bit値。
		 * @retval value bit member descriptor。
		 * @pre `expected`は`Width` bitに収まる値。
		 * @post allocationなし。
		 */
		template <typename T, unsigned Shift, unsigned Width>
		BitMember<T> ReservedBits(std::string_view name, std::uint64_t expected) noexcept;

		/**
		 * @brief 1byte storage grouped bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam BitCount logical bit descriptor数。
		 * @param[in] group bit group名。
		 * @param[in] members logical bit descriptor列。
		 * @retval value field descriptor。
		 * @pre `members`は16要素以下。
		 * @post decode/encodeはstorage unitを1回だけ消費。
		 */
		template <typename T, std::size_t BitCount>
		Field<T> BitsU8(std::string_view group,
						std::array<BitMember<T>, BitCount> members) noexcept;

		/**
		 * @brief big-endian 2byte storage grouped bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam BitCount logical bit descriptor数。
		 * @param[in] group bit group名。
		 * @param[in] members logical bit descriptor列。
		 * @retval value field descriptor。
		 * @pre `members`は16要素以下。
		 * @post decode/encodeはstorage unitを1回だけ消費。
		 */
		template <typename T, std::size_t BitCount>
		Field<T> BitsU16Be(std::string_view group,
						   std::array<BitMember<T>, BitCount> members) noexcept;

		/**
		 * @brief little-endian 2byte storage grouped bit field descriptor生成。
		 * @tparam T schema対象型。
		 * @tparam BitCount logical bit descriptor数。
		 * @param[in] group bit group名。
		 * @param[in] members logical bit descriptor列。
		 * @retval value field descriptor。
		 * @pre `members`は16要素以下。
		 * @post decode/encodeはstorage unitを1回だけ消費。
		 */
		template <typename T, std::size_t BitCount>
		Field<T> BitsU16Le(std::string_view group,
						   std::array<BitMember<T>, BitCount> members) noexcept;

		/**
		 * @brief computed validation field descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name validation名。
		 * @param[in] callback `bool(const T&, Status&) noexcept` callback。
		 * @retval value field descriptor。
		 * @pre `callback != nullptr`。失敗時はcallbackがStatusへ詳細を設定可能。
		 * @post decode/encode/measure時にbyteを消費せずcallbackを実行。
		 */
		template <typename T>
		Field<T> Validate(std::string_view name, ValidationCallback<T> callback) noexcept;

		/**
		 * @brief length validation hook descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name validation名。
		 * @param[in] callback length検査callback。
		 * @retval value field descriptor。
		 * @pre `callback != nullptr`。
		 * @post byteを消費せずcallbackを実行。
		 */
		template <typename T>
		Field<T> LengthValidation(std::string_view name, ValidationCallback<T> callback) noexcept;

		/**
		 * @brief checksum validation hook descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name validation名。
		 * @param[in] callback checksum検査callback。
		 * @retval value field descriptor。
		 * @pre `callback != nullptr`。
		 * @post byteを消費せずcallbackを実行。
		 */
		template <typename T>
		Field<T> ChecksumValidation(std::string_view name, ValidationCallback<T> callback) noexcept;

		/**
		 * @brief range validation hook descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name validation名。
		 * @param[in] callback range検査callback。
		 * @retval value field descriptor。
		 * @pre `callback != nullptr`。
		 * @post byteを消費せずcallbackを実行。
		 */
		template <typename T>
		Field<T> RangeValidation(std::string_view name, ValidationCallback<T> callback) noexcept;

		/**
		 * @brief cross-field validation hook descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name validation名。
		 * @param[in] callback cross-field検査callback。
		 * @retval value field descriptor。
		 * @pre `callback != nullptr`。
		 * @post byteを消費せずcallbackを実行。
		 */
		template <typename T>
		Field<T> CrossFieldValidation(std::string_view name,
									  ValidationCallback<T> callback) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief byte order for integer descriptors.
			 */
			enum class ByteOrder : std::uint8_t
			{
				kSingle,
				kBig,
				kLittle
			};

			/**
			 * @brief member pointer traits.
			 * @tparam Pointer member pointer type.
			 */
			template <typename Pointer>
			struct MemberPointerTraits;

			/**
			 * @brief member pointer traits specialization.
			 * @tparam Class owning class.
			 * @tparam Member member type.
			 */
			template <typename Class, typename Member>
			struct MemberPointerTraits<Member Class::*>
			{
				using class_type = Class;
				using member_type = Member;
			};

			/**
			 * @brief std::array byte storage traits.
			 * @tparam T candidate type.
			 */
			template <typename T>
			struct ByteArrayTraits
			{
				static constexpr bool kIsByteArray = false;
				static constexpr std::size_t kSize = 0U;
			};

			/**
			 * @brief std::array byte storage traits specialization.
			 * @tparam Size array size.
			 */
			template <std::size_t Size>
			struct ByteArrayTraits<std::array<std::uint8_t, Size>>
			{
				static constexpr bool kIsByteArray = true;
				static constexpr std::size_t kSize = Size;
			};

			/**
			 * @brief C byte array storage traits specialization.
			 * @tparam Size array size.
			 */
			template <std::size_t Size>
			struct ByteArrayTraits<std::uint8_t[Size]>
			{
				static constexpr bool kIsByteArray = true;
				static constexpr std::size_t kSize = Size;
			};

			/**
			 * @brief success Status生成。
			 * @retval value success status。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			inline Status OkStatus() noexcept
			{
				return Status{};
			}

			/**
			 * @brief basic failure Status生成。
			 * @param[in] error failure kind。
			 * @param[in] offset diagnostic offset。
			 * @param[in] field field名。
			 * @param[in] group group名。
			 * @retval value failure status。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			inline Status MakeStatus(Error error,
									 std::size_t offset,
									 std::string_view field,
									 std::string_view group) noexcept
			{
				Status status{};
				status.error = error;
				status.offset = offset;
				status.field = field;
				status.group = group;
				return status;
			}

			/**
			 * @brief required/available size付きfailure Status生成。
			 * @param[in] error failure kind。
			 * @param[in] offset diagnostic offset。
			 * @param[in] field field名。
			 * @param[in] group group名。
			 * @param[in] required_size 必要byte数。
			 * @param[in] available_size 利用可能byte数。
			 * @retval value failure status。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			inline Status MakeSizeStatus(Error error,
										 std::size_t offset,
										 std::string_view field,
										 std::string_view group,
										 std::size_t required_size,
										 std::size_t available_size) noexcept
			{
				auto status = MakeStatus(error, offset, field, group);
				status.required_size = required_size;
				status.available_size = available_size;
				return status;
			}

			/**
			 * @brief expected/actual付きfailure Status生成。
			 * @param[in] error failure kind。
			 * @param[in] offset diagnostic offset。
			 * @param[in] field field名。
			 * @param[in] group group名。
			 * @param[in] expected 期待値。
			 * @param[in] actual 実値。
			 * @retval value failure status。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			inline Status MakeExpectedStatus(Error error,
											 std::size_t offset,
											 std::string_view field,
											 std::string_view group,
											 std::uint64_t expected,
											 std::uint64_t actual) noexcept
			{
				auto status = MakeStatus(error, offset, field, group);
				status.expected = expected;
				status.actual = actual;
				status.has_expected = true;
				status.has_actual = true;
				return status;
			}

			/**
			 * @brief byte view storage validity判定。
			 * @param[in] data buffer pointer。
			 * @param[in] size buffer size。
			 * @retval true `data != nullptr`、または`size == 0`。
			 * @retval false `data == nullptr && size > 0`。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			constexpr bool IsValidStorage(const void* data, std::size_t size) noexcept
			{
				return data != nullptr || size == 0U;
			}

			/**
			 * @brief input view validity判定。
			 * @param[in] data input view。
			 * @retval true valid view。
			 * @retval false invalid view。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			constexpr bool IsValidInputView(ket::byte_view::View data) noexcept
			{
				return IsValidStorage(data.Data(), data.Size());
			}

			/**
			 * @brief output view validity判定。
			 * @param[in] out output view。
			 * @retval true valid view。
			 * @retval false invalid view。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			constexpr bool IsValidOutputView(ket::byte_view::MutableView out) noexcept
			{
				return IsValidStorage(out.Data(), out.Size());
			}

			/**
			 * @brief read可能範囲判定。
			 * @param[in] data input view。
			 * @param[in] offset 開始offset。
			 * @param[in] size 読み取りbyte数。
			 * @retval true `offset..offset+size`がview内。
			 * @retval false invalid view、またはsize不足。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			constexpr bool
			CanRead(ket::byte_view::View data, std::size_t offset, std::size_t size) noexcept
			{
				const auto is_valid = IsValidInputView(data);
				return is_valid && offset <= data.Size() && size <= (data.Size() - offset);
			}

			/**
			 * @brief size_t addition overflow検査。
			 * @param[in] lhs 左辺。
			 * @param[in] rhs 右辺。
			 * @param[out] out 成功時の和。
			 * @retval true overflowなし。
			 * @retval false overflowあり。`out`は変更なし。
			 * @pre `out`は有効な参照。
			 * @post 成功時だけ`out`を更新。
			 */
			inline bool TryAddSize(std::size_t lhs, std::size_t rhs, std::size_t& out) noexcept
			{
				const auto max_value = std::numeric_limits<std::size_t>::max();
				const auto overflows = rhs > (max_value - lhs);
				if (overflows)
				{
					return false;
				}

				out = lhs + rhs;
				return true;
			}

			/**
			 * @brief unsigned integer width in bytes.
			 * @tparam Unsigned unsigned integer type.
			 * @retval value byte width.
			 * @pre `Unsigned` is unsigned integral.
			 * @post 外部状態の変更なし。
			 */
			template <typename Unsigned>
			constexpr std::size_t IntegerByteSize() noexcept
			{
				return sizeof(Unsigned);
			}

			/**
			 * @brief unsigned integerをbyte列からload。
			 * @tparam Unsigned unsigned integer type.
			 * @param[in] data 読み取り対象先頭。
			 * @param[in] order byte order。
			 * @retval value decoded unsigned integer。
			 * @pre `data`は`sizeof(Unsigned)`byte以上読み取り可能。
			 * @post 外部状態の変更なし。
			 */
			template <typename Unsigned>
			Unsigned LoadUnsigned(const std::uint8_t* data, ByteOrder order) noexcept
			{
				static_assert(std::is_unsigned_v<Unsigned>, "Unsigned must be unsigned");
				if (order == ByteOrder::kSingle)
				{
					return static_cast<Unsigned>(data[0]);
				}

				Unsigned value = 0U;
				const auto width = IntegerByteSize<Unsigned>();
				for (std::size_t index = 0U; index < width; ++index)
				{
					const auto source_index =
						order == ByteOrder::kBig ? index : (width - index - 1U);
					value = static_cast<Unsigned>((value << 8U) |
												  static_cast<Unsigned>(data[source_index]));
				}

				return value;
			}

			/**
			 * @brief unsigned integerをbyte列へstore。
			 * @tparam Unsigned unsigned integer type.
			 * @param[in,out] data 書き込み対象先頭。
			 * @param[in] value 書き込む値。
			 * @param[in] order byte order。
			 * @retval void 戻り値なし。
			 * @pre `data`は`sizeof(Unsigned)`byte以上書き込み可能。
			 * @post `data[0..sizeof(Unsigned))`を更新。
			 */
			template <typename Unsigned>
			void StoreUnsigned(std::uint8_t* data, Unsigned value, ByteOrder order) noexcept
			{
				static_assert(std::is_unsigned_v<Unsigned>, "Unsigned must be unsigned");
				if (order == ByteOrder::kSingle)
				{
					data[0] = static_cast<std::uint8_t>(value);
					return;
				}

				const auto width = IntegerByteSize<Unsigned>();
				for (std::size_t index = 0U; index < width; ++index)
				{
					const auto shift = static_cast<unsigned>(index * 8U);
					const auto shifted = static_cast<std::uint64_t>(value) >> shift;
					const auto byte = static_cast<std::uint8_t>(shifted & std::uint64_t{0xFFU});
					const auto destination_index =
						order == ByteOrder::kBig ? (width - index - 1U) : index;
					data[destination_index] = byte;
				}
			}

			/**
			 * @brief unsigned bitsをsigned two's complement値へ変換。
			 * @tparam Signed signed integer type.
			 * @tparam Unsigned unsigned integer type.
			 * @param[in] raw raw two's complement bits。
			 * @retval value signed value。
			 * @pre `Signed` and `Unsigned` have the same size.
			 * @post 外部状態の変更なし。
			 */
			template <typename Signed, typename Unsigned>
			Signed DecodeSigned(Unsigned raw) noexcept
			{
				static_assert(std::is_signed_v<Signed>, "Signed must be signed");
				static_assert(std::is_unsigned_v<Unsigned>, "Unsigned must be unsigned");
				static_assert(sizeof(Signed) == sizeof(Unsigned), "integer sizes must match");
				const auto bit_count = static_cast<unsigned>(sizeof(Unsigned) * 8U);
				const auto sign_mask = static_cast<Unsigned>(Unsigned{1} << (bit_count - 1U));
				const auto sign_is_clear = (raw & sign_mask) == 0U;
				if (sign_is_clear)
				{
					return static_cast<Signed>(raw);
				}

				const auto inverted = static_cast<Unsigned>(~raw);
				const auto magnitude = static_cast<Unsigned>(inverted + Unsigned{1});
				const auto is_minimum = magnitude == sign_mask;
				if (is_minimum)
				{
					return std::numeric_limits<Signed>::min();
				}

				return static_cast<Signed>(-static_cast<Signed>(magnitude));
			}

			/**
			 * @brief signed値をtwo's complement bitsへ変換。
			 * @tparam Signed signed integer type.
			 * @tparam Unsigned unsigned integer type.
			 * @param[in] value signed value。
			 * @retval value raw two's complement bits。
			 * @pre `Signed` and `Unsigned` have the same size.
			 * @post 外部状態の変更なし。
			 */
			template <typename Signed, typename Unsigned>
			Unsigned EncodeSigned(Signed value) noexcept
			{
				static_assert(std::is_signed_v<Signed>, "Signed must be signed");
				static_assert(std::is_unsigned_v<Unsigned>, "Unsigned must be unsigned");
				static_assert(sizeof(Signed) == sizeof(Unsigned), "integer sizes must match");
				const auto is_non_negative = value >= 0;
				if (is_non_negative)
				{
					return static_cast<Unsigned>(value);
				}

				const auto minimum = std::numeric_limits<Signed>::min();
				const auto is_minimum = value == minimum;
				if (is_minimum)
				{
					const auto bit_count = static_cast<unsigned>(sizeof(Unsigned) * 8U);
					return static_cast<Unsigned>(Unsigned{1} << (bit_count - 1U));
				}

				const auto magnitude = static_cast<Unsigned>(-value);
				return static_cast<Unsigned>(static_cast<Unsigned>(~magnitude) + Unsigned{1});
			}

			/**
			 * @brief member pointer class一致をcompile-time検査。
			 * @tparam T expected class type.
			 * @tparam Member member pointer.
			 * @retval void 戻り値なし。
			 * @pre `Member` is a member pointer.
			 * @post 外部状態の変更なし。
			 */
			template <typename T, auto Member>
			void CheckMemberClass() noexcept
			{
				using Traits = MemberPointerTraits<decltype(Member)>;
				static_assert(std::is_same_v<typename Traits::class_type, T>,
							  "Member pointer must belong to T");
			}

			/**
			 * @brief integer field read処理。
			 * @tparam T schema対象型。
			 * @tparam Member member pointer.
			 * @tparam Unsigned wire unsigned type.
			 * @tparam Order byte order.
			 * @param[in] data input view。
			 * @param[in,out] offset current offset。
			 * @param[in,out] value decode中object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true decode成功。
			 * @retval false decode失敗。
			 * @pre `value`はtemporary object。
			 * @post 成功時だけoffsetとmemberを更新。
			 */
			template <typename T, auto Member, typename Unsigned, ByteOrder Order>
			bool DecodeInteger(ket::byte_view::View data,
							   std::size_t& offset,
							   T& value,
							   const Field<T>& field,
							   Status& status) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				CheckMemberClass<T, Member>();
				static_assert(sizeof(MemberType) == sizeof(Unsigned), "member size mismatch");
				const auto field_start = offset;
				const auto can_read = CanRead(data, offset, field.encoded_size);
				if (!can_read)
				{
					const auto available = offset <= data.Size() ? (data.Size() - offset) : 0U;
					status = MakeSizeStatus(Error::kShortInput,
											field_start,
											field.name,
											field.group,
											field.encoded_size,
											available);
					return false;
				}

				const auto raw = LoadUnsigned<Unsigned>(data.Data() + offset, Order);
				if constexpr (std::is_signed_v<MemberType>)
				{
					value.*Member = DecodeSigned<MemberType, Unsigned>(raw);
				}
				else
				{
					value.*Member = static_cast<MemberType>(raw);
				}
				offset += field.encoded_size;
				return true;
			}

			/**
			 * @brief field preflight no-op。
			 * @tparam T schema対象型。
			 * @param[in] value encode対象object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true 常に成功。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T>
			bool PreflightNoop(const T& value, const Field<T>& field, Status& status) noexcept
			{
				(void)value;
				(void)field;
				(void)status;
				return true;
			}

			/**
			 * @brief fixed field size measure。
			 * @tparam T schema対象型。
			 * @param[in] value 測定対象object。
			 * @param[in] field field descriptor。
			 * @param[in,out] encoded_size 現在までのencoded size。
			 * @param[out] status failure status。
			 * @retval true size加算成功。
			 * @retval false overflow。
			 * @pre `encoded_size`は現在までのbyte数。
			 * @post 成功時だけ`encoded_size`を更新。
			 */
			template <typename T>
			bool MeasureFixed(const T& value,
							  const Field<T>& field,
							  std::size_t& encoded_size,
							  Status& status) noexcept
			{
				(void)value;
				std::size_t next_size = 0U;
				const auto added = TryAddSize(encoded_size, field.encoded_size, next_size);
				if (!added)
				{
					status = MakeSizeStatus(Error::kSizeOverflow,
											encoded_size,
											field.name,
											field.group,
											field.encoded_size,
											encoded_size);
					return false;
				}

				encoded_size = next_size;
				return true;
			}

			/**
			 * @brief integer field write処理。
			 * @tparam T schema対象型。
			 * @tparam Member member pointer.
			 * @tparam Unsigned wire unsigned type.
			 * @tparam Order byte order.
			 * @param[in] value encode対象object。
			 * @param[in,out] out output buffer。
			 * @param[in,out] offset current offset。
			 * @param[in] field field descriptor。
			 * @retval void 戻り値なし。
			 * @pre output buffer has enough capacity.
			 * @post output buffer and offset updated.
			 */
			template <typename T, auto Member, typename Unsigned, ByteOrder Order>
			void EncodeInteger(const T& value,
							   std::uint8_t* out,
							   std::size_t& offset,
							   const Field<T>& field) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				Unsigned raw = 0U;
				if constexpr (std::is_signed_v<MemberType>)
				{
					raw = EncodeSigned<MemberType, Unsigned>(value.*Member);
				}
				else
				{
					raw = static_cast<Unsigned>(value.*Member);
				}

				StoreUnsigned<Unsigned>(out + offset, raw, Order);
				offset += field.encoded_size;
			}

			/**
			 * @brief integer field descriptor生成。
			 * @tparam T schema対象型。
			 * @tparam Member member pointer.
			 * @tparam Unsigned wire unsigned type.
			 * @tparam Order byte order.
			 * @param[in] name field名。
			 * @retval value field descriptor。
			 * @pre member type width matches `Unsigned`.
			 * @post allocationなし。
			 */
			template <typename T, auto Member, typename Unsigned, ByteOrder Order>
			Field<T> MakeIntegerField(std::string_view name) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				CheckMemberClass<T, Member>();
				static_assert(std::is_integral_v<MemberType>, "integer member required");
				static_assert(sizeof(MemberType) == sizeof(Unsigned),
							  "integer member size mismatch");

				Field<T> field{};
				field.name = name;
				field.kind = FieldKind::kInteger;
				field.encoded_size = IntegerByteSize<Unsigned>();
				field.max_encoded_size = field.encoded_size;
				field.is_valid = true;
				field.decode = &DecodeInteger<T, Member, Unsigned, Order>;
				field.preflight = &PreflightNoop<T>;
				field.encode = &EncodeInteger<T, Member, Unsigned, Order>;
				field.measure = &MeasureFixed<T>;
				return field;
			}

			/**
			 * @brief BCD nibble妥当性判定。
			 * @param[in] value nibble値。
			 * @retval true `value <= 9`。
			 * @retval false BCD範囲外。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			constexpr bool IsBcdNibble(std::uint64_t value) noexcept
			{
				return value <= 9U;
			}

			/**
			 * @brief BCD raw bitsのnibble検査。
			 * @param[in] raw BCD raw bits。
			 * @param[in] nibble_count nibble数。
			 * @param[out] bad_nibble 不正nibble。
			 * @retval true 全nibble valid。
			 * @retval false invalid nibbleあり。
			 * @pre `nibble_count <= 16`。
			 * @post 成功時は`bad_nibble`変更なし。失敗時は不正nibbleを格納。
			 */
			inline bool ValidateBcdNibbles(std::uint64_t raw,
										   unsigned nibble_count,
										   std::uint8_t& bad_nibble) noexcept
			{
				for (unsigned index = 0U; index < nibble_count; ++index)
				{
					const auto shift = index * 4U;
					const auto nibble = static_cast<std::uint8_t>((raw >> shift) & 0x0FU);
					const auto is_bcd = IsBcdNibble(nibble);
					if (!is_bcd)
					{
						bad_nibble = nibble;
						return false;
					}
				}

				return true;
			}

			/**
			 * @brief BCD raw bitsをintへ変換。
			 * @param[in] raw BCD raw bits。
			 * @param[in] nibble_count nibble数。
			 * @param[out] out decoded integer。
			 * @param[out] bad_nibble 不正nibble。
			 * @retval true 変換成功。
			 * @retval false invalid nibble、またはint overflow。
			 * @pre `out`と`bad_nibble`は有効な参照。
			 * @post 成功時だけ`out`を更新。
			 */
			inline bool ParseBcdInteger(std::uint64_t raw,
										unsigned nibble_count,
										int& out,
										std::uint8_t& bad_nibble) noexcept
			{
				int result = 0;
				for (unsigned remaining = nibble_count; remaining > 0U; --remaining)
				{
					const auto shift = (remaining - 1U) * 4U;
					const auto digit = static_cast<std::uint8_t>((raw >> shift) & 0x0FU);
					const auto is_bcd = IsBcdNibble(digit);
					if (!is_bcd)
					{
						bad_nibble = digit;
						return false;
					}

					const auto max_value = std::numeric_limits<int>::max();
					const auto max_before_append = (max_value - static_cast<int>(digit)) / 10;
					const auto overflows = result > max_before_append;
					if (overflows)
					{
						return false;
					}

					result = (result * 10) + static_cast<int>(digit);
				}

				out = result;
				return true;
			}

			/**
			 * @brief BCD digit数の10進上限取得。
			 * @param[in] nibble_count digit数。
			 * @retval value `10 ^ nibble_count`。
			 * @pre `nibble_count <= 9`。
			 * @post 外部状態の変更なし。
			 */
			constexpr int BcdDecimalLimit(unsigned nibble_count) noexcept
			{
				int limit = 1;
				for (unsigned index = 0U; index < nibble_count; ++index)
				{
					limit *= 10;
				}

				return limit;
			}

			/**
			 * @brief intをBCD raw bitsへ変換。
			 * @param[in] value encode対象値。
			 * @param[in] nibble_count BCD digit数。
			 * @param[out] out raw bits。
			 * @retval true 変換成功。
			 * @retval false 範囲外。
			 * @pre `out`は有効な参照。
			 * @post 成功時だけ`out`を更新。
			 */
			inline bool
			BuildBcdInteger(int value, unsigned nibble_count, std::uint64_t& out) noexcept
			{
				const auto limit = BcdDecimalLimit(nibble_count);
				const auto in_range = value >= 0 && value < limit;
				if (!in_range)
				{
					return false;
				}

				auto remaining = value;
				std::uint64_t raw = 0U;
				for (unsigned index = 0U; index < nibble_count; ++index)
				{
					const auto digit = static_cast<std::uint64_t>(remaining % 10);
					raw |= digit << (index * 4U);
					remaining /= 10;
				}

				out = raw;
				return true;
			}

			/**
			 * @brief BCD integer field read処理。
			 * @tparam T schema対象型。
			 * @tparam Member int member pointer.
			 * @tparam Unsigned wire unsigned type.
			 * @tparam Order byte order.
			 * @param[in] data input view。
			 * @param[in,out] offset current offset。
			 * @param[in,out] value decode中object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true decode成功。
			 * @retval false decode失敗。
			 * @pre member type is int.
			 * @post 成功時だけoffsetとmemberを更新。
			 */
			template <typename T, auto Member, typename Unsigned, ByteOrder Order>
			bool DecodeBcdInteger(ket::byte_view::View data,
								  std::size_t& offset,
								  T& value,
								  const Field<T>& field,
								  Status& status) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				static_assert(std::is_same_v<MemberType, int>, "BCD decoded member must be int");
				const auto field_start = offset;
				const auto can_read = CanRead(data, offset, field.encoded_size);
				if (!can_read)
				{
					const auto available = offset <= data.Size() ? (data.Size() - offset) : 0U;
					status = MakeSizeStatus(Error::kShortInput,
											field_start,
											field.name,
											field.group,
											field.encoded_size,
											available);
					return false;
				}

				const auto raw =
					static_cast<std::uint64_t>(LoadUnsigned<Unsigned>(data.Data() + offset, Order));
				int decoded = 0;
				std::uint8_t bad_nibble = 0U;
				const auto parsed = ParseBcdInteger(
					raw, static_cast<unsigned>(field.encoded_size * 2U), decoded, bad_nibble);
				if (!parsed)
				{
					status = MakeExpectedStatus(
						Error::kInvalidBcd, field_start, field.name, field.group, 9U, bad_nibble);
					return false;
				}

				value.*Member = decoded;
				offset += field.encoded_size;
				return true;
			}

			/**
			 * @brief BCD integer field preflight。
			 * @tparam T schema対象型。
			 * @tparam Member int member pointer.
			 * @param[in] value encode対象object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true value fits BCD digit count.
			 * @retval false value out of range.
			 * @pre member type is int.
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T, auto Member>
			bool PreflightBcdInteger(const T& value, const Field<T>& field, Status& status) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				static_assert(std::is_same_v<MemberType, int>, "BCD decoded member must be int");
				std::uint64_t raw = 0U;
				const auto nibble_count = static_cast<unsigned>(field.encoded_size * 2U);
				const auto built = BuildBcdInteger(value.*Member, nibble_count, raw);
				if (!built)
				{
					const auto limit = BcdDecimalLimit(nibble_count);
					const auto actual =
						value.*Member < 0 ? 0U : static_cast<std::uint64_t>(value.*Member);
					status = MakeExpectedStatus(Error::kValueOutOfRange,
												0U,
												field.name,
												field.group,
												static_cast<std::uint64_t>(limit - 1),
												actual);
					return false;
				}

				return true;
			}

			/**
			 * @brief BCD integer field write処理。
			 * @tparam T schema対象型。
			 * @tparam Member int member pointer.
			 * @tparam Unsigned wire unsigned type.
			 * @tparam Order byte order.
			 * @param[in] value encode対象object。
			 * @param[in,out] out output buffer。
			 * @param[in,out] offset current offset。
			 * @param[in] field field descriptor。
			 * @retval void 戻り値なし。
			 * @pre preflight済み、かつoutput capacity十分。
			 * @post output buffer and offset updated.
			 */
			template <typename T, auto Member, typename Unsigned, ByteOrder Order>
			void EncodeBcdInteger(const T& value,
								  std::uint8_t* out,
								  std::size_t& offset,
								  const Field<T>& field) noexcept
			{
				std::uint64_t raw = 0U;
				const auto nibble_count = static_cast<unsigned>(field.encoded_size * 2U);
				const auto built = BuildBcdInteger(value.*Member, nibble_count, raw);
				(void)built;
				StoreUnsigned<Unsigned>(out + offset, static_cast<Unsigned>(raw), Order);
				offset += field.encoded_size;
			}

			/**
			 * @brief BCD integer field descriptor生成。
			 * @tparam T schema対象型。
			 * @tparam Member int member pointer.
			 * @tparam Unsigned wire unsigned type.
			 * @tparam Order byte order.
			 * @param[in] name field名。
			 * @retval value field descriptor。
			 * @pre member type is int.
			 * @post allocationなし。
			 */
			template <typename T, auto Member, typename Unsigned, ByteOrder Order>
			Field<T> MakeBcdIntegerField(std::string_view name) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				CheckMemberClass<T, Member>();
				static_assert(std::is_same_v<MemberType, int>, "BCD decoded member must be int");

				Field<T> field{};
				field.name = name;
				field.kind = FieldKind::kBcdInteger;
				field.encoded_size = IntegerByteSize<Unsigned>();
				field.max_encoded_size = field.encoded_size;
				field.is_valid = true;
				field.decode = &DecodeBcdInteger<T, Member, Unsigned, Order>;
				field.preflight = &PreflightBcdInteger<T, Member>;
				field.encode = &EncodeBcdInteger<T, Member, Unsigned, Order>;
				field.measure = &MeasureFixed<T>;
				return field;
			}

			/**
			 * @brief std::array byte storage pointer取得。
			 * @tparam Array std::array type.
			 * @param[in] storage storage object。
			 * @retval value data pointer。
			 * @pre storage is std::array.
			 * @post 外部状態の変更なし。
			 */
			template <typename Array>
			const std::uint8_t* ByteStorageData(const Array& storage) noexcept
			{
				return storage.data();
			}

			/**
			 * @brief C array byte storage pointer取得。
			 * @tparam Size array size.
			 * @param[in] storage storage object。
			 * @retval value data pointer。
			 * @pre storage is C byte array.
			 * @post 外部状態の変更なし。
			 */
			template <std::size_t Size>
			const std::uint8_t* ByteStorageData(const std::uint8_t (&storage)[Size]) noexcept
			{
				return storage;
			}

			/**
			 * @brief std::array mutable byte storage pointer取得。
			 * @tparam Array std::array type.
			 * @param[in,out] storage storage object。
			 * @retval value data pointer。
			 * @pre storage is std::array.
			 * @post storage内容の変更なし。
			 */
			template <typename Array>
			std::uint8_t* MutableByteStorageData(Array& storage) noexcept
			{
				return storage.data();
			}

			/**
			 * @brief C array mutable byte storage pointer取得。
			 * @tparam Size array size.
			 * @param[in,out] storage storage object。
			 * @retval value data pointer。
			 * @pre storage is C byte array.
			 * @post storage内容の変更なし。
			 */
			template <std::size_t Size>
			std::uint8_t* MutableByteStorageData(std::uint8_t (&storage)[Size]) noexcept
			{
				return storage;
			}

			/**
			 * @brief raw bytes field read処理。
			 * @tparam T schema対象型。
			 * @tparam Member byte array member pointer.
			 * @tparam ValidateBcd trueならBCD nibble検査あり。
			 * @param[in] data input view。
			 * @param[in,out] offset current offset。
			 * @param[in,out] value decode中object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true decode成功。
			 * @retval false decode失敗。
			 * @pre member is fixed byte array.
			 * @post 成功時だけoffsetとmember bytesを更新。
			 */
			template <typename T, auto Member, bool ValidateBcd>
			bool DecodeBytes(ket::byte_view::View data,
							 std::size_t& offset,
							 T& value,
							 const Field<T>& field,
							 Status& status) noexcept
			{
				const auto field_start = offset;
				const auto can_read = CanRead(data, offset, field.encoded_size);
				if (!can_read)
				{
					const auto available = offset <= data.Size() ? (data.Size() - offset) : 0U;
					status = MakeSizeStatus(Error::kShortInput,
											field_start,
											field.name,
											field.group,
											field.encoded_size,
											available);
					return false;
				}

				if constexpr (ValidateBcd)
				{
					for (std::size_t index = 0U; index < field.encoded_size; ++index)
					{
						const auto byte = data.Data()[offset + index];
						std::uint8_t bad_nibble = 0U;
						const auto valid_bcd = ValidateBcdNibbles(byte, 2U, bad_nibble);
						if (!valid_bcd)
						{
							status = MakeExpectedStatus(Error::kInvalidBcd,
														field_start + index,
														field.name,
														field.group,
														9U,
														bad_nibble);
							return false;
						}
					}
				}

				if (field.encoded_size > 0U)
				{
					auto* destination = MutableByteStorageData(value.*Member);
					std::copy_n(data.Data() + offset, field.encoded_size, destination);
				}

				offset += field.encoded_size;
				return true;
			}

			/**
			 * @brief raw bytes field preflight。
			 * @tparam T schema対象型。
			 * @tparam Member byte array member pointer.
			 * @tparam ValidateBcd trueならBCD nibble検査あり。
			 * @param[in] value encode対象object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true preflight成功。
			 * @retval false BCD invalid。
			 * @pre member is fixed byte array.
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T, auto Member, bool ValidateBcd>
			bool PreflightBytes(const T& value, const Field<T>& field, Status& status) noexcept
			{
				if constexpr (ValidateBcd)
				{
					const auto* source = ByteStorageData(value.*Member);
					for (std::size_t index = 0U; index < field.encoded_size; ++index)
					{
						std::uint8_t bad_nibble = 0U;
						const auto valid_bcd = ValidateBcdNibbles(source[index], 2U, bad_nibble);
						if (!valid_bcd)
						{
							status = MakeExpectedStatus(
								Error::kInvalidBcd, index, field.name, field.group, 9U, bad_nibble);
							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief raw bytes field write処理。
			 * @tparam T schema対象型。
			 * @tparam Member byte array member pointer.
			 * @param[in] value encode対象object。
			 * @param[in,out] out output buffer。
			 * @param[in,out] offset current offset。
			 * @param[in] field field descriptor。
			 * @retval void 戻り値なし。
			 * @pre preflight済み、かつoutput capacity十分。
			 * @post output buffer and offset updated.
			 */
			template <typename T, auto Member>
			void EncodeBytes(const T& value,
							 std::uint8_t* out,
							 std::size_t& offset,
							 const Field<T>& field) noexcept
			{
				if (field.encoded_size > 0U)
				{
					const auto* source = ByteStorageData(value.*Member);
					std::copy_n(source, field.encoded_size, out + offset);
				}

				offset += field.encoded_size;
			}

			/**
			 * @brief bytes field descriptor生成。
			 * @tparam T schema対象型。
			 * @tparam Member byte array member pointer.
			 * @tparam ValidateBcd trueならBCD nibble検査あり。
			 * @param[in] name field名。
			 * @retval value field descriptor。
			 * @pre member is fixed byte array.
			 * @post allocationなし。
			 */
			template <typename T, auto Member, bool ValidateBcd>
			Field<T> MakeBytesField(std::string_view name) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				using Traits = ByteArrayTraits<MemberType>;
				CheckMemberClass<T, Member>();
				static_assert(Traits::kIsByteArray, "member must be a fixed byte array");

				Field<T> field{};
				field.name = name;
				field.kind = ValidateBcd ? FieldKind::kRawBcdBytes : FieldKind::kBytes;
				field.encoded_size = Traits::kSize;
				field.max_encoded_size = field.encoded_size;
				field.is_valid = true;
				field.decode = &DecodeBytes<T, Member, ValidateBcd>;
				field.preflight = &PreflightBytes<T, Member, ValidateBcd>;
				field.encode = &EncodeBytes<T, Member>;
				field.measure = &MeasureFixed<T>;
				return field;
			}

			/**
			 * @brief view bytes field read処理。
			 * @tparam T schema対象型。
			 * @tparam Member byte view member pointer.
			 * @param[in] data input view。
			 * @param[in,out] offset current offset。
			 * @param[in,out] value decode中object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true decode成功。
			 * @retval false decode失敗。
			 * @pre decode元buffer lifetimeはresult view利用中callerが保持。
			 * @post 成功時だけoffsetとmember viewを更新。
			 */
			template <typename T, auto Member>
			bool DecodeViewBytes(ket::byte_view::View data,
								 std::size_t& offset,
								 T& value,
								 const Field<T>& field,
								 Status& status) noexcept
			{
				const auto field_start = offset;
				const auto can_read = CanRead(data, offset, field.encoded_size);
				if (!can_read)
				{
					const auto available = offset <= data.Size() ? (data.Size() - offset) : 0U;
					status = MakeSizeStatus(Error::kShortInput,
											field_start,
											field.name,
											field.group,
											field.encoded_size,
											available);
					return false;
				}

				const std::uint8_t* view_data = nullptr;
				if (field.encoded_size > 0U)
				{
					view_data = data.Data() + offset;
				}

				value.*Member = ket::byte_view::View(view_data, field.encoded_size);
				offset += field.encoded_size;
				return true;
			}

			/**
			 * @brief view bytes field preflight。
			 * @tparam T schema対象型。
			 * @tparam Member byte view member pointer.
			 * @param[in] value encode対象object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true view validかつsize一致。
			 * @retval false invalid view、またはlength mismatch。
			 * @pre view fieldの元buffer lifetimeは呼び出し中維持。
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T, auto Member>
			bool PreflightViewBytes(const T& value, const Field<T>& field, Status& status) noexcept
			{
				const auto view = value.*Member;
				const auto is_valid = IsValidInputView(view);
				if (!is_valid)
				{
					status = MakeSizeStatus(Error::kInvalidInputView,
											0U,
											field.name,
											field.group,
											field.encoded_size,
											view.Size());
					return false;
				}

				const auto size_matches = view.Size() == field.encoded_size;
				if (!size_matches)
				{
					status = MakeSizeStatus(Error::kLengthMismatch,
											0U,
											field.name,
											field.group,
											field.encoded_size,
											view.Size());
					return false;
				}

				return true;
			}

			/**
			 * @brief view bytes field write処理。
			 * @tparam T schema対象型。
			 * @tparam Member byte view member pointer.
			 * @param[in] value encode対象object。
			 * @param[in,out] out output buffer。
			 * @param[in,out] offset current offset。
			 * @param[in] field field descriptor。
			 * @retval void 戻り値なし。
			 * @pre preflight済み、かつoutput capacity十分。
			 * @post output buffer and offset updated.
			 */
			template <typename T, auto Member>
			void EncodeViewBytes(const T& value,
								 std::uint8_t* out,
								 std::size_t& offset,
								 const Field<T>& field) noexcept
			{
				if (field.encoded_size > 0U)
				{
					const auto view = value.*Member;
					std::copy_n(view.Data(), field.encoded_size, out + offset);
				}

				offset += field.encoded_size;
			}

			/**
			 * @brief fixed bytes expectation read処理。
			 * @tparam T schema対象型。
			 * @param[in] data input view。
			 * @param[in,out] offset current offset。
			 * @param[in,out] value decode中object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true decode成功。
			 * @retval false size不足、またはexpected mismatch。
			 * @pre expected bytes pointer is valid when size > 0.
			 * @post 成功時だけoffsetを更新。
			 */
			template <typename T>
			bool DecodeExpectedBytes(ket::byte_view::View data,
									 std::size_t& offset,
									 T& value,
									 const Field<T>& field,
									 Status& status) noexcept
			{
				(void)value;
				const auto field_start = offset;
				const auto can_read = CanRead(data, offset, field.encoded_size);
				if (!can_read)
				{
					const auto available = offset <= data.Size() ? (data.Size() - offset) : 0U;
					status = MakeSizeStatus(Error::kShortInput,
											field_start,
											field.name,
											field.group,
											field.encoded_size,
											available);
					return false;
				}

				for (std::size_t index = 0U; index < field.encoded_size; ++index)
				{
					const auto actual = data.Data()[offset + index];
					auto expected = static_cast<std::uint8_t>(field.expected);
					if (field.expected_bytes != nullptr)
					{
						expected = field.expected_bytes[index];
					}

					const auto matches = actual == expected;
					if (!matches)
					{
						status = MakeExpectedStatus(Error::kReservedMismatch,
													field_start + index,
													field.name,
													field.group,
													expected,
													actual);
						return false;
					}
				}

				offset += field.encoded_size;
				return true;
			}

			/**
			 * @brief fixed bytes expectation write処理。
			 * @tparam T schema対象型。
			 * @param[in] value encode対象object。
			 * @param[in,out] out output buffer。
			 * @param[in,out] offset current offset。
			 * @param[in] field field descriptor。
			 * @retval void 戻り値なし。
			 * @pre output capacity十分。
			 * @post output buffer and offset updated.
			 */
			template <typename T>
			void EncodeExpectedBytes(const T& value,
									 std::uint8_t* out,
									 std::size_t& offset,
									 const Field<T>& field) noexcept
			{
				(void)value;
				for (std::size_t index = 0U; index < field.encoded_size; ++index)
				{
					auto expected = static_cast<std::uint8_t>(field.expected);
					if (field.expected_bytes != nullptr)
					{
						expected = field.expected_bytes[index];
					}

					out[offset + index] = expected;
				}

				offset += field.encoded_size;
			}

			/**
			 * @brief repeated expectation field descriptor生成。
			 * @tparam T schema対象型。
			 * @param[in] name field名。
			 * @param[in] size encoded byte数。
			 * @param[in] expected repeated expected byte。
			 * @param[in] kind field kind。
			 * @retval value field descriptor。
			 * @pre なし。
			 * @post allocationなし。
			 */
			template <typename T>
			Field<T> MakeRepeatedExpectedBytes(std::string_view name,
											   std::size_t size,
											   std::uint8_t expected,
											   FieldKind kind) noexcept
			{
				Field<T> field{};
				field.name = name;
				field.kind = kind;
				field.encoded_size = size;
				field.max_encoded_size = size;
				field.is_valid = true;
				field.expected = expected;
				field.decode = &DecodeExpectedBytes<T>;
				field.preflight = &PreflightNoop<T>;
				field.encode = &EncodeExpectedBytes<T>;
				field.measure = &MeasureFixed<T>;
				return field;
			}

			/**
			 * @brief bit mask生成。
			 * @param[in] width bit幅。
			 * @retval value lower width bits mask。
			 * @pre `width <= 64`。
			 * @post 外部状態の変更なし。
			 */
			constexpr std::uint64_t MaskForWidth(unsigned width) noexcept
			{
				return width >= 64U ? std::numeric_limits<std::uint64_t>::max()
									: ((std::uint64_t{1} << width) - 1U);
			}

			/**
			 * @brief bit member get処理。
			 * @tparam T schema対象型。
			 * @tparam Member unsigned member pointer.
			 * @param[in] value encode対象object。
			 * @param[in] member bit member descriptor。
			 * @param[out] out raw member value。
			 * @param[out] status failure status。
			 * @retval true get成功。
			 * @retval false 未使用。
			 * @pre member type is unsigned integral.
			 * @post 成功時だけ`out`を更新。
			 */
			template <typename T, auto Member>
			bool GetBitMember(const T& value,
							  const BitMember<T>& member,
							  std::uint64_t& out,
							  Status& status) noexcept
			{
				(void)member;
				(void)status;
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				static_assert(std::is_integral_v<MemberType> && std::is_unsigned_v<MemberType>,
							  "bit member must be unsigned integral");
				out = static_cast<std::uint64_t>(value.*Member);
				return true;
			}

			/**
			 * @brief bit member set処理。
			 * @tparam T schema対象型。
			 * @tparam Member unsigned member pointer.
			 * @param[in,out] value decode中object。
			 * @param[in] raw raw logical value。
			 * @retval void 戻り値なし。
			 * @pre raw fits member type.
			 * @post memberを更新。
			 */
			template <typename T, auto Member>
			void SetBitMember(T& value, std::uint64_t raw) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				value.*Member = static_cast<MemberType>(raw);
			}

			/**
			 * @brief bit group read処理。
			 * @tparam T schema対象型。
			 * @tparam Unsigned storage unsigned type.
			 * @tparam Order byte order.
			 * @param[in] data input view。
			 * @param[in,out] offset current offset。
			 * @param[in,out] value decode中object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true decode成功。
			 * @retval false size不足、またはreserved mismatch。
			 * @pre field has valid bit members.
			 * @post 成功時だけoffsetとlogical membersを更新。
			 */
			template <typename T, typename Unsigned, ByteOrder Order>
			bool DecodeBits(ket::byte_view::View data,
							std::size_t& offset,
							T& value,
							const Field<T>& field,
							Status& status) noexcept
			{
				const auto field_start = offset;
				const auto can_read = CanRead(data, offset, field.encoded_size);
				if (!can_read)
				{
					const auto available = offset <= data.Size() ? (data.Size() - offset) : 0U;
					status = MakeSizeStatus(Error::kShortInput,
											field_start,
											field.name,
											field.group,
											field.encoded_size,
											available);
					return false;
				}

				const auto raw_storage =
					static_cast<std::uint64_t>(LoadUnsigned<Unsigned>(data.Data() + offset, Order));
				for (std::size_t index = 0U; index < field.bit_member_count; ++index)
				{
					const auto& member = field.bit_members[index];
					const auto mask = MaskForWidth(member.width);
					const auto actual = (raw_storage >> member.shift) & mask;
					if (!member.has_member)
					{
						const auto matches = actual == member.expected;
						if (!matches)
						{
							status = MakeExpectedStatus(Error::kReservedMismatch,
														field_start,
														member.name,
														field.group,
														member.expected,
														actual);
							return false;
						}
					}
					else
					{
						member.set(value, actual);
					}
				}

				offset += field.encoded_size;
				return true;
			}

			/**
			 * @brief bit group preflight。
			 * @tparam T schema対象型。
			 * @param[in] value encode対象object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true all logical members fit.
			 * @retval false range overflow。
			 * @pre field has valid bit members.
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T>
			bool PreflightBits(const T& value, const Field<T>& field, Status& status) noexcept
			{
				for (std::size_t index = 0U; index < field.bit_member_count; ++index)
				{
					const auto& member = field.bit_members[index];
					if (!member.has_member)
					{
						continue;
					}

					std::uint64_t actual = 0U; // NOLINT(misc-const-correctness)
					const auto got_value = member.get(value, member, actual, status);
					if (!got_value)
					{
						return false;
					}

					const auto max_value = MaskForWidth(member.width);
					const auto fits = actual <= max_value;
					if (!fits)
					{
						status = MakeExpectedStatus(Error::kValueOutOfRange,
													0U,
													member.name,
													field.group,
													max_value,
													actual);
						return false;
					}
				}

				return true;
			}

			/**
			 * @brief bit group write処理。
			 * @tparam T schema対象型。
			 * @tparam Unsigned storage unsigned type.
			 * @tparam Order byte order.
			 * @param[in] value encode対象object。
			 * @param[in,out] out output buffer。
			 * @param[in,out] offset current offset。
			 * @param[in] field field descriptor。
			 * @retval void 戻り値なし。
			 * @pre preflight済み、かつoutput capacity十分。
			 * @post output buffer and offset updated.
			 */
			template <typename T, typename Unsigned, ByteOrder Order>
			void EncodeBits(const T& value,
							std::uint8_t* out,
							std::size_t& offset,
							const Field<T>& field) noexcept
			{
				std::uint64_t raw_storage = 0U;
				Status ignored_status{}; // NOLINT(misc-const-correctness)
				for (std::size_t index = 0U; index < field.bit_member_count; ++index)
				{
					const auto& member = field.bit_members[index];
					std::uint64_t actual = member.expected; // NOLINT(misc-const-correctness)
					if (member.has_member)
					{
						const auto got_value = member.get(value, member, actual, ignored_status);
						(void)got_value;
					}

					const auto mask = MaskForWidth(member.width);
					raw_storage |= (actual & mask) << member.shift;
				}

				StoreUnsigned<Unsigned>(out + offset, static_cast<Unsigned>(raw_storage), Order);
				offset += field.encoded_size;
			}

			/**
			 * @brief bit group field descriptor生成。
			 * @tparam T schema対象型。
			 * @tparam Unsigned storage unsigned type.
			 * @tparam Order byte order.
			 * @tparam BitCount logical bit descriptor数。
			 * @param[in] group group名。
			 * @param[in] members logical bit descriptor列。
			 * @retval value field descriptor。
			 * @pre `BitCount <= 16` and members valid.
			 * @post allocationなし。
			 */
			template <typename T, typename Unsigned, ByteOrder Order, std::size_t BitCount>
			Field<T> MakeBitsField(std::string_view group,
								   std::array<BitMember<T>, BitCount> members) noexcept
			{
				Field<T> field{};
				field.name = group;
				field.group = group;
				field.kind = FieldKind::kBits;
				field.encoded_size = IntegerByteSize<Unsigned>();
				field.max_encoded_size = field.encoded_size;
				field.is_valid = BitCount <= field.bit_members.size();
				field.decode = &DecodeBits<T, Unsigned, Order>;
				field.preflight = &PreflightBits<T>;
				field.encode = &EncodeBits<T, Unsigned, Order>;
				field.measure = &MeasureFixed<T>;
				if (!field.is_valid)
				{
					return field;
				}

				for (std::size_t index = 0U; index < BitCount; ++index)
				{
					const auto& member = members[index];
					const auto member_valid = member.is_valid;
					if (!member_valid)
					{
						field.is_valid = false;
						return field;
					}

					field.bit_members[index] = member;
				}

				field.bit_member_count = BitCount;
				return field;
			}

			/**
			 * @brief validation field read処理。
			 * @tparam T schema対象型。
			 * @param[in] data input view。
			 * @param[in,out] offset current offset。
			 * @param[in,out] value decode中object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true callback success.
			 * @retval false callback failure.
			 * @pre callback is valid.
			 * @post offsetとbyte列の変更なし。
			 */
			template <typename T>
			bool DecodeValidation(ket::byte_view::View data,
								  std::size_t& offset,
								  T& value,
								  const Field<T>& field,
								  Status& status) noexcept
			{
				(void)data;
				const auto callback_ok = field.validation(value, status);
				if (!callback_ok)
				{
					const auto status_ok = status.Ok();
					if (status_ok)
					{
						status =
							MakeStatus(Error::kCallbackFailed, offset, field.name, field.group);
					}
					return false;
				}

				return true;
			}

			/**
			 * @brief validation field preflight。
			 * @tparam T schema対象型。
			 * @param[in] value encode対象object。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @retval true callback success.
			 * @retval false callback failure.
			 * @pre callback is valid.
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T>
			bool PreflightValidation(const T& value, const Field<T>& field, Status& status) noexcept
			{
				const auto callback_ok = field.validation(value, status);
				if (!callback_ok)
				{
					const auto status_ok = status.Ok();
					if (status_ok)
					{
						status = MakeStatus(Error::kCallbackFailed, 0U, field.name, field.group);
					}
					return false;
				}

				return true;
			}

			/**
			 * @brief validation field write no-op。
			 * @tparam T schema対象型。
			 * @param[in] value encode対象object。
			 * @param[in,out] out output buffer。
			 * @param[in,out] offset current offset。
			 * @param[in] field field descriptor。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post output bufferとoffsetの変更なし。
			 */
			template <typename T>
			void EncodeValidation(const T& value,
								  std::uint8_t* out, // NOLINT(readability-non-const-parameter)
								  std::size_t& offset,
								  const Field<T>& field) noexcept
			{
				(void)value;
				(void)out;
				(void)offset;
				(void)field;
			}

			/**
			 * @brief validation field measure。
			 * @tparam T schema対象型。
			 * @param[in] value 測定対象object。
			 * @param[in] field field descriptor。
			 * @param[in,out] encoded_size 現在までのencoded size。
			 * @param[out] status failure status。
			 * @retval true callback success.
			 * @retval false callback failure.
			 * @pre callback is valid.
			 * @post encoded_sizeの変更なし。
			 */
			template <typename T>
			bool MeasureValidation(const T& value,
								   const Field<T>& field,
								   std::size_t& encoded_size,
								   Status& status) noexcept
			{
				(void)encoded_size;
				return PreflightValidation(value, field, status);
			}

			/**
			 * @brief schema status検査。
			 * @tparam T schema対象型。
			 * @tparam FieldCount field descriptor数。
			 * @param[in] schema 対象schema。
			 * @param[out] status failure status。
			 * @retval true schema valid.
			 * @retval false schema error.
			 * @pre なし。
			 * @post schemaと外部状態の変更なし。
			 */
			template <typename T, std::size_t FieldCount>
			bool ValidateSchema(const Schema<T, FieldCount>& schema, Status& status) noexcept
			{
				const auto schema_ok = schema.status.Ok();
				if (!schema_ok)
				{
					status = schema.status;
					return false;
				}

				return true;
			}

			/**
			 * @brief schema encoded size measurement and preflight.
			 * @tparam T schema対象型。
			 * @tparam FieldCount field descriptor数。
			 * @param[in] value 測定対象object。
			 * @param[in] schema 対象schema。
			 * @param[out] encoded_size measured size。
			 * @param[out] status failure status。
			 * @retval true measurement success.
			 * @retval false schema error、overflow、またはpreflight failure.
			 * @pre なし。
			 * @post 成功時だけ`encoded_size`を更新。
			 */
			template <typename T, std::size_t FieldCount>
			bool MeasureAndPreflight(const T& value,
									 const Schema<T, FieldCount>& schema,
									 std::size_t& encoded_size,
									 Status& status) noexcept
			{
				const auto schema_valid = ValidateSchema(schema, status);
				if (!schema_valid)
				{
					return false;
				}

				std::size_t measured = 0U; // NOLINT(misc-const-correctness)
				for (const auto& field : schema.fields)
				{
					const auto measured_field = field.measure(value, field, measured, status);
					if (!measured_field)
					{
						return false;
					}

					const auto preflight_ok = field.preflight(value, field, status);
					if (!preflight_ok)
					{
						return false;
					}
				}

				encoded_size = measured;
				return true;
			}

			/**
			 * @brief unchecked encode after preflight.
			 * @tparam T schema対象型。
			 * @tparam FieldCount field descriptor数。
			 * @param[in] value encode対象object。
			 * @param[in] schema encode schema。
			 * @param[in,out] out output buffer。
			 * @retval void 戻り値なし。
			 * @pre schema preflight済み、かつoutput capacity十分。
			 * @post output先頭encoded_size byteを更新。
			 */
			template <typename T, std::size_t FieldCount>
			void EncodeUnchecked(const T& value,
								 const Schema<T, FieldCount>& schema,
								 std::uint8_t* out) noexcept
			{
				std::size_t offset = 0U; // NOLINT(misc-const-correctness)
				for (const auto& field : schema.fields)
				{
					field.encode(value, out, offset, field);
				}
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		/**
		 * @brief success status判定。
		 * @retval true `error == Error::kNone`。
		 * @retval false failure kindを保持。
		 * @pre なし。
		 * @post statusと外部状態の変更なし。
		 */
		[[nodiscard]] inline bool Status::Ok() const noexcept
		{
			return error == Error::kNone;
		}

		template <typename T, std::size_t FieldCount>
		Schema<T, FieldCount> MakeSchema(std::string_view name,
										 std::array<Field<T>, FieldCount> fields) noexcept
		{
			Schema<T, FieldCount> schema{};
			schema.name = name;
			schema.fields = fields;
			schema.status = detail::OkStatus();

			std::size_t fixed_size = 0U;
			std::size_t max_size = 0U;
			for (std::size_t index = 0U; index < FieldCount; ++index)
			{
				const auto& field = schema.fields[index];
				if (!field.is_valid || field.decode == nullptr || field.preflight == nullptr ||
					field.encode == nullptr || field.measure == nullptr)
				{
					schema.status =
						detail::MakeStatus(Error::kSchemaError, index, field.name, field.group);
					return schema;
				}

				if (!field.is_fixed_size)
				{
					schema.is_fixed_size = false;
				}
				else
				{
					std::size_t next_size = 0U;
					const auto added =
						detail::TryAddSize(fixed_size, field.encoded_size, next_size);
					if (!added)
					{
						schema.status = detail::MakeSizeStatus(Error::kSizeOverflow,
															   fixed_size,
															   field.name,
															   field.group,
															   field.encoded_size,
															   fixed_size);
						return schema;
					}

					fixed_size = next_size;
				}

				std::size_t next_max_size = 0U;
				const auto max_added =
					detail::TryAddSize(max_size, field.max_encoded_size, next_max_size);
				if (!max_added)
				{
					schema.status = detail::MakeSizeStatus(Error::kSizeOverflow,
														   max_size,
														   field.name,
														   field.group,
														   field.max_encoded_size,
														   max_size);
					return schema;
				}

				max_size = next_max_size;
			}

			schema.fixed_size = fixed_size;
			schema.max_size = max_size;
			return schema;
		}

		template <typename T, std::size_t FieldCount>
		DecodeResult<T> DecodeExact(ket::byte_view::View data, const Schema<T, FieldCount>& schema)
		{
			auto result = DecodePrefix(data, schema);
			const auto has_value = result.value.has_value();
			if (!has_value)
			{
				return result;
			}

			const auto fully_consumed = result.consumed == data.Size();
			if (!fully_consumed)
			{
				result.value = std::nullopt;
				result.status = detail::MakeSizeStatus(Error::kTrailingBytes,
													   result.consumed,
													   {},
													   schema.name,
													   result.consumed,
													   data.Size());
				result.consumed = 0U;
				return result;
			}

			return result;
		}

		template <typename T, std::size_t FieldCount>
		DecodeResult<T> DecodePrefix(ket::byte_view::View data, const Schema<T, FieldCount>& schema)
		{
			DecodeResult<T> result{};
			const auto input_valid = detail::IsValidInputView(data);
			if (!input_valid)
			{
				result.status = detail::MakeSizeStatus(
					Error::kInvalidInputView, 0U, {}, schema.name, 0U, data.Size());
				return result;
			}

			const auto schema_valid = detail::ValidateSchema(schema, result.status);
			if (!schema_valid)
			{
				return result;
			}

			T decoded{};
			std::size_t offset = 0U; // NOLINT(misc-const-correctness)
			for (const auto& field : schema.fields)
			{
				const auto decoded_field =
					field.decode(data, offset, decoded, field, result.status);
				if (!decoded_field)
				{
					result.consumed = 0U;
					return result;
				}
			}

			result.value = std::move(decoded);
			result.status = detail::OkStatus();
			result.consumed = offset;
			return result;
		}

		template <typename T, std::size_t FieldCount>
		EncodeResult Encode(const T& value, const Schema<T, FieldCount>& schema)
		{
			EncodeResult result{};
			std::size_t encoded_size = 0U;
			const auto measured =
				detail::MeasureAndPreflight(value, schema, encoded_size, result.status);
			if (!measured)
			{
				return result;
			}

			std::vector<std::uint8_t> bytes{};
			bytes.resize(encoded_size);
			std::uint8_t* data = nullptr;
			if (encoded_size > 0U)
			{
				data = bytes.data();
			}

			detail::EncodeUnchecked(value, schema, data);
			result.bytes = std::move(bytes);
			result.status = detail::OkStatus();
			return result;
		}

		template <typename T, std::size_t FieldCount>
		EncodeToResult EncodeTo(const T& value,
								const Schema<T, FieldCount>& schema,
								ket::byte_view::MutableView out)
		{
			EncodeToResult result{};
			const auto output_valid = detail::IsValidOutputView(out);
			if (!output_valid)
			{
				result.status = detail::MakeSizeStatus(
					Error::kInvalidOutputView, 0U, {}, schema.name, 0U, out.Size());
				return result;
			}

			std::size_t encoded_size = 0U;
			const auto measured =
				detail::MeasureAndPreflight(value, schema, encoded_size, result.status);
			if (!measured)
			{
				return result;
			}

			const auto enough_output = encoded_size <= out.Size();
			if (!enough_output)
			{
				result.status = detail::MakeSizeStatus(
					Error::kShortOutput, 0U, {}, schema.name, encoded_size, out.Size());
				return result;
			}

			std::uint8_t* data = nullptr;
			if (encoded_size > 0U)
			{
				data = out.Data();
			}

			detail::EncodeUnchecked(value, schema, data);
			result.status = detail::OkStatus();
			result.encoded_size = encoded_size;
			return result;
		}

		template <typename T, std::size_t FieldCount>
		std::optional<std::size_t> EncodedSize(const Schema<T, FieldCount>& schema) noexcept
		{
			const auto schema_ok = schema.status.Ok();
			if (!schema_ok)
			{
				return std::nullopt;
			}

			if (!schema.is_fixed_size)
			{
				return std::nullopt;
			}

			return schema.fixed_size;
		}

		template <typename T, std::size_t FieldCount>
		std::optional<std::size_t> MaxEncodedSize(const Schema<T, FieldCount>& schema) noexcept
		{
			const auto schema_ok = schema.status.Ok();
			if (!schema_ok)
			{
				return std::nullopt;
			}

			return schema.max_size;
		}

		template <typename T, std::size_t FieldCount>
		bool IsFixedSize(const Schema<T, FieldCount>& schema) noexcept
		{
			const auto schema_ok = schema.status.Ok();
			return schema_ok && schema.is_fixed_size;
		}

		template <typename T, std::size_t FieldCount>
		SizeResult MeasureEncodedSize(const T& value, const Schema<T, FieldCount>& schema)
		{
			SizeResult result{};
			std::size_t encoded_size = 0U;
			const auto measured =
				detail::MeasureAndPreflight(value, schema, encoded_size, result.status);
			if (!measured)
			{
				return result;
			}

			result.value = encoded_size;
			result.status = detail::OkStatus();
			return result;
		}

		template <typename T, auto Member>
		Field<T> U8(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint8_t, detail::ByteOrder::kSingle>(
				name);
		}

		template <typename T, auto Member>
		Field<T> U16Be(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint16_t, detail::ByteOrder::kBig>(
				name);
		}

		template <typename T, auto Member>
		Field<T> U16Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint16_t, detail::ByteOrder::kLittle>(
				name);
		}

		template <typename T, auto Member>
		Field<T> U32Be(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint32_t, detail::ByteOrder::kBig>(
				name);
		}

		template <typename T, auto Member>
		Field<T> U32Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint32_t, detail::ByteOrder::kLittle>(
				name);
		}

		template <typename T, auto Member>
		Field<T> U64Be(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint64_t, detail::ByteOrder::kBig>(
				name);
		}

		template <typename T, auto Member>
		Field<T> U64Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint64_t, detail::ByteOrder::kLittle>(
				name);
		}

		template <typename T, auto Member>
		Field<T> I8(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint8_t, detail::ByteOrder::kSingle>(
				name);
		}

		template <typename T, auto Member>
		Field<T> I16Be(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint16_t, detail::ByteOrder::kBig>(
				name);
		}

		template <typename T, auto Member>
		Field<T> I16Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint16_t, detail::ByteOrder::kLittle>(
				name);
		}

		template <typename T, auto Member>
		Field<T> I32Be(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint32_t, detail::ByteOrder::kBig>(
				name);
		}

		template <typename T, auto Member>
		Field<T> I32Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint32_t, detail::ByteOrder::kLittle>(
				name);
		}

		template <typename T, auto Member>
		Field<T> I64Be(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint64_t, detail::ByteOrder::kBig>(
				name);
		}

		template <typename T, auto Member>
		Field<T> I64Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T, Member, std::uint64_t, detail::ByteOrder::kLittle>(
				name);
		}

		template <typename T, auto Member>
		Field<T> BcdU8(std::string_view name) noexcept
		{
			return detail::MakeBcdIntegerField<T, Member, std::uint8_t, detail::ByteOrder::kSingle>(
				name);
		}

		template <typename T, auto Member>
		Field<T> BcdU16Be(std::string_view name) noexcept
		{
			return detail::MakeBcdIntegerField<T, Member, std::uint16_t, detail::ByteOrder::kBig>(
				name);
		}

		template <typename T, auto Member>
		Field<T> BcdU16Le(std::string_view name) noexcept
		{
			return detail::
				MakeBcdIntegerField<T, Member, std::uint16_t, detail::ByteOrder::kLittle>(name);
		}

		template <typename T, auto Member>
		Field<T> BcdU32Be(std::string_view name) noexcept
		{
			return detail::MakeBcdIntegerField<T, Member, std::uint32_t, detail::ByteOrder::kBig>(
				name);
		}

		template <typename T, auto Member>
		Field<T> BcdU32Le(std::string_view name) noexcept
		{
			return detail::
				MakeBcdIntegerField<T, Member, std::uint32_t, detail::ByteOrder::kLittle>(name);
		}

		template <typename T, auto Member>
		Field<T> RawBcdBytes(std::string_view name) noexcept
		{
			return detail::MakeBytesField<T, Member, true>(name);
		}

		template <typename T, auto Member>
		Field<T> Bytes(std::string_view name) noexcept
		{
			return detail::MakeBytesField<T, Member, false>(name);
		}

		template <typename T, auto Member>
		Field<T> ViewBytes(std::string_view name, std::size_t size) noexcept
		{
			using MemberType = typename detail::MemberPointerTraits<decltype(Member)>::member_type;
			detail::CheckMemberClass<T, Member>();
			static_assert(std::is_same_v<MemberType, ket::byte_view::View>,
						  "view bytes member must be ket::byte_view::View");

			Field<T> field{};
			field.name = name;
			field.kind = FieldKind::kViewBytes;
			field.encoded_size = size;
			field.max_encoded_size = size;
			field.is_valid = true;
			field.decode = &detail::DecodeViewBytes<T, Member>;
			field.preflight = &detail::PreflightViewBytes<T, Member>;
			field.encode = &detail::EncodeViewBytes<T, Member>;
			field.measure = &detail::MeasureFixed<T>;
			return field;
		}

		template <typename T>
		Field<T> ConstU8(std::string_view name, std::uint8_t expected) noexcept
		{
			return detail::MakeRepeatedExpectedBytes<T>(name, 1U, expected, FieldKind::kConstBytes);
		}

		template <typename T>
		Field<T>
		ConstBytes(std::string_view name, const std::uint8_t* expected, std::size_t size) noexcept
		{
			Field<T> field{};
			field.name = name;
			field.kind = FieldKind::kConstBytes;
			field.encoded_size = size;
			field.max_encoded_size = size;
			field.is_valid = detail::IsValidStorage(expected, size);
			field.expected_bytes = expected;
			field.expected_bytes_size = size;
			field.decode = &detail::DecodeExpectedBytes<T>;
			field.preflight = &detail::PreflightNoop<T>;
			field.encode = &detail::EncodeExpectedBytes<T>;
			field.measure = &detail::MeasureFixed<T>;
			return field;
		}

		/**
		 * @brief repeated reserved bytes field descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name field名。
		 * @param[in] size reserved byte数。
		 * @param[in] expected 各byteの期待値。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post decodeは全byte一致を検査し、encodeは期待値を埋める。
		 */
		template <typename T>
		Field<T>
		ReservedBytes(std::string_view name, std::size_t size, std::uint8_t expected) noexcept
		{
			return detail::MakeRepeatedExpectedBytes<T>(
				name, size, expected, FieldKind::kReservedBytes);
		}

		/**
		 * @brief repeated padding bytes field descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name field名。
		 * @param[in] size padding byte数。
		 * @param[in] value 各padding byteの値。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post decodeは全byte一致を検査し、encodeはpadding値を埋める。
		 */
		template <typename T>
		Field<T> PadBytes(std::string_view name, std::size_t size, std::uint8_t value) noexcept
		{
			return detail::MakeRepeatedExpectedBytes<T>(name, size, value, FieldKind::kPadBytes);
		}

		template <typename T, auto Member, unsigned Shift, unsigned Width>
		BitMember<T> Bit(std::string_view name) noexcept
		{
			using MemberType = typename detail::MemberPointerTraits<decltype(Member)>::member_type;
			detail::CheckMemberClass<T, Member>();
			static_assert(std::is_integral_v<MemberType> && std::is_unsigned_v<MemberType>,
						  "bit member must be unsigned integral");
			static_assert(Width > 0U, "bit width must be positive");
			static_assert(Width <= sizeof(MemberType) * 8U, "bit width must fit member type");

			BitMember<T> member{};
			member.name = name;
			member.shift = Shift;
			member.width = Width;
			member.has_member = true;
			member.is_valid = Width > 0U && Width <= 64U;
			member.get = &detail::GetBitMember<T, Member>;
			member.set = &detail::SetBitMember<T, Member>;
			return member;
		}

		template <typename T, unsigned Shift, unsigned Width>
		BitMember<T> ReservedBits(std::string_view name, std::uint64_t expected) noexcept
		{
			BitMember<T> member{};
			member.name = name;
			member.shift = Shift;
			member.width = Width;
			member.expected = expected;
			member.has_member = false;
			member.is_valid = Width > 0U && Width <= 64U && expected <= detail::MaskForWidth(Width);
			return member;
		}

		template <typename T, std::size_t BitCount>
		Field<T> BitsU8(std::string_view group, std::array<BitMember<T>, BitCount> members) noexcept
		{
			return detail::MakeBitsField<T, std::uint8_t, detail::ByteOrder::kSingle>(group,
																					  members);
		}

		template <typename T, std::size_t BitCount>
		Field<T> BitsU16Be(std::string_view group,
						   std::array<BitMember<T>, BitCount> members) noexcept
		{
			return detail::MakeBitsField<T, std::uint16_t, detail::ByteOrder::kBig>(group, members);
		}

		template <typename T, std::size_t BitCount>
		Field<T> BitsU16Le(std::string_view group,
						   std::array<BitMember<T>, BitCount> members) noexcept
		{
			return detail::MakeBitsField<T, std::uint16_t, detail::ByteOrder::kLittle>(group,
																					   members);
		}

		template <typename T>
		Field<T> Validate(std::string_view name, ValidationCallback<T> callback) noexcept
		{
			Field<T> field{};
			field.name = name;
			field.kind = FieldKind::kValidation;
			field.is_valid = callback != nullptr;
			field.validation = callback;
			field.decode = &detail::DecodeValidation<T>;
			field.preflight = &detail::PreflightValidation<T>;
			field.encode = &detail::EncodeValidation<T>;
			field.measure = &detail::MeasureValidation<T>;
			return field;
		}

		template <typename T>
		Field<T> LengthValidation(std::string_view name, ValidationCallback<T> callback) noexcept
		{
			return Validate<T>(name, callback);
		}

		template <typename T>
		Field<T> ChecksumValidation(std::string_view name, ValidationCallback<T> callback) noexcept
		{
			return Validate<T>(name, callback);
		}

		template <typename T>
		Field<T> RangeValidation(std::string_view name, ValidationCallback<T> callback) noexcept
		{
			return Validate<T>(name, callback);
		}

		template <typename T>
		Field<T> CrossFieldValidation(std::string_view name,
									  ValidationCallback<T> callback) noexcept
		{
			return Validate<T>(name, callback);
		}

	} // namespace wire

} // namespace ket
