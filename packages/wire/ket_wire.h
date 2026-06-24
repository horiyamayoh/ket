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
 * `ket_wire.h` と依存する ket module のheader/sourceを対象プロジェクトへコピー。
 * `ket_byte_reader.cpp`、`ket_byte_writer.cpp`、`ket_endian.cpp`、`ket_bcd.cpp`
 * もbuild対象に含める。
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
 * 標準ライブラリ、および ket::byte_view、ket::byte_reader、ket::byte_writer、ket::endian、
 * ket::bits、ket::numeric、ket::bcd、ket::bytes。
 * `ket-wire` は ordinary module ではなく package runtime であり、複数 ket module を合成可能。
 *
 * @par namespace
 * 公開API：ket::wire
 * 内部実装：ket::wire::detail
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits> // IWYU pragma: keep
#include <utility>
#include <vector>

#include "ket_bcd.h"
#include "ket_bits.h"
#include "ket_byte_reader.h"
#include "ket_byte_view.h"
#include "ket_byte_writer.h"
#include "ket_bytes.h"
#include "ket_endian.h"
#include "ket_numeric.h"

namespace ket
{
	namespace wire
	{
		namespace detail
		{
			/**
			 * @brief BitMember private state access helper.
			 */
			template <typename T>
			struct BitMemberAccess; // IWYU pragma: keep

			/**
			 * @brief Field private state access helper.
			 */
			template <typename T>
			struct FieldAccess; // IWYU pragma: keep

			/**
			 * @brief Schema private state access helper.
			 */
			template <typename T, std::size_t FieldCount>
			struct SchemaAccess; // IWYU pragma: keep

		} // namespace detail

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
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] bool Ok() const noexcept;
		};

		/**
		 * @brief grouped bit descriptor内のlogical bit member.
		 * @tparam T schema対象型。
		 */
		template <typename T>
		class BitMember
		{
		  public:
			using GetFunction = bool (*)(const T& value,
										 const BitMember& member,
										 std::uint64_t& out,
										 Status& status) noexcept;
			using SetFunction = void (*)(T& value, std::uint64_t raw) noexcept;

			/**
			 * @brief logical bit名。
			 * @retval value descriptor生成時に指定した名前。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::string_view Name() const noexcept;

			/**
			 * @brief storage内の開始bit位置。
			 * @retval value least significant bitを0とするshift。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr unsigned Shift() const noexcept;

			/**
			 * @brief logical bit幅。
			 * @retval value bit幅。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr unsigned Width() const noexcept;

			/**
			 * @brief reserved bitで期待される値。
			 * @retval value expected raw value。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::uint64_t Expected() const noexcept;

			/**
			 * @brief object memberへ対応するlogical bitかを判定。
			 * @retval true object member対応。
			 * @retval false reserved bit。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr bool HasMember() const noexcept;

			/**
			 * @brief descriptor生成時の妥当性。
			 * @retval true runtimeで利用可能。
			 * @retval false schema構築時にschema errorとして扱う。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr bool IsValid() const noexcept;

		  private:
			template <typename>
			friend struct detail::BitMemberAccess;

			std::string_view name_;
			unsigned shift_ = 0U;
			unsigned width_ = 0U;
			std::uint64_t expected_ = 0U;
			bool has_member_ = false;
			bool is_valid_ = false;
			GetFunction get_ = nullptr;
			SetFunction set_ = nullptr;
		};

		/**
		 * @brief schema field descriptor.
		 * @tparam T schema対象型。
		 */
		template <typename T>
		class Field
		{
		  public:
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

			/**
			 * @brief field名。
			 * @retval value descriptor生成時に指定した名前。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::string_view Name() const noexcept;

			/**
			 * @brief bit group名。
			 * @retval value grouped bitsの場合はgroup名、それ以外は空。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::string_view Group() const noexcept;

			/**
			 * @brief field category取得。
			 * @retval value field kind。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr FieldKind Kind() const noexcept;

			/**
			 * @brief fixed fieldのencoded byte数。
			 * @retval value encoded byte数。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::size_t EncodedSize() const noexcept;

			/**
			 * @brief field単体の最大encoded byte数。
			 * @retval value 最大encoded byte数。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::size_t MaxEncodedSize() const noexcept;

			/**
			 * @brief field sizeがvalue非依存かを判定。
			 * @retval true fixed-size field。
			 * @retval false value依存size field。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr bool IsFixedSize() const noexcept;

			/**
			 * @brief descriptor生成時の妥当性。
			 * @retval true runtimeで利用可能。
			 * @retval false schema構築時にschema errorとして扱う。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr bool IsValid() const noexcept;

			/**
			 * @brief repeated expected byte値。
			 * @retval value repeated reserved/padding byte。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::uint64_t Expected() const noexcept;

			/**
			 * @brief const expected byte列。
			 * @retval value expected byte列先頭。byte列を持たないfieldではnullptr。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr const std::uint8_t* ExpectedBytes() const noexcept;

			/**
			 * @brief const expected byte列の長さ。
			 * @retval value expected byte列長。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::size_t ExpectedBytesSize() const noexcept;

			/**
			 * @brief grouped bit descriptors。
			 * @retval value 最大16個のbit member descriptor配列。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr const std::array<BitMember<T>, 16U>&
			BitMembers() const noexcept;

			/**
			 * @brief grouped bit descriptorをindexで取得。
			 * @param[in] index bit member index。
			 * @retval value descriptor pointer。
			 * @retval nullptr `index >= BitMemberCount()`。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr const BitMember<T>*
			BitMemberAt(std::size_t index) const noexcept;

			/**
			 * @brief 有効なgrouped bit descriptor数。
			 * @retval value bit member descriptor数。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::size_t BitMemberCount() const noexcept;

		  private:
			template <typename>
			friend struct detail::FieldAccess;

			std::string_view name_;
			std::string_view group_;
			FieldKind kind_ = FieldKind::kValidation;
			std::size_t encoded_size_ = 0U;
			std::size_t max_encoded_size_ = 0U;
			bool is_fixed_size_ = true;
			bool is_valid_ = false;
			std::uint64_t expected_ = 0U;
			const std::uint8_t* expected_bytes_ = nullptr;
			std::size_t expected_bytes_size_ = 0U;
			bool owns_expected_bytes_ = false;
			std::array<std::uint8_t, 32U> owned_expected_bytes_{};
			std::array<BitMember<T>, 16U> bit_members_{};
			std::size_t bit_member_count_ = 0U;
			DecodeFunction decode_ = nullptr;
			PreflightFunction preflight_ = nullptr;
			EncodeFunction encode_ = nullptr;
			MeasureFunction measure_ = nullptr;
			ValidationFunction validation_ = nullptr;
		};

		/**
		 * @brief schema descriptor owning its field array.
		 * @tparam T schema対象型。
		 * @tparam FieldCount field descriptor数。
		 */
		template <typename T, std::size_t FieldCount>
		class Schema
		{
		  public:
			/**
			 * @brief schema名。
			 * @retval value descriptor生成時に指定した名前。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::string_view Name() const noexcept;

			/**
			 * @brief schemaが所有するfield descriptor列。
			 * @retval value field descriptor配列。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr const std::array<Field<T>, FieldCount>& Fields() const noexcept;

			/**
			 * @brief field descriptorをindexで取得。
			 * @param[in] index field index。
			 * @retval value descriptor pointer。
			 * @retval nullptr `index >= FieldCountValue()`。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr const Field<T>* FieldAt(std::size_t index) const noexcept;

			/**
			 * @brief field descriptor数。
			 * @retval value schema内field数。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::size_t FieldCountValue() const noexcept;

			/**
			 * @brief schema encoded sizeがvalue非依存かを判定。
			 * @retval true fixed-size schema。
			 * @retval false value依存size schema。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr bool IsFixedSize() const noexcept;

			/**
			 * @brief fixed-size schemaのencoded byte数。
			 * @retval value fixed encoded byte数。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::size_t FixedSize() const noexcept;

			/**
			 * @brief schema最大encoded byte数。
			 * @retval value 最大encoded byte数。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr std::size_t MaxSize() const noexcept;

			/**
			 * @brief schema構築時status。
			 * @retval value schema構築時status。
			 * @pre なし。
			 * @post descriptorと外部状態の変更なし。
			 * @code
			 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
			 * @endcode
			 */
			[[nodiscard]] constexpr const Status& SchemaStatus() const noexcept;

		  private:
			template <typename, std::size_t>
			friend struct detail::SchemaAccess;

			std::string_view name_;
			std::array<Field<T>, FieldCount> fields_{};
			std::size_t field_count_ = FieldCount;
			bool is_fixed_size_ = true;
			std::size_t fixed_size_ = 0U;
			std::size_t max_size_ = 0U;
			Status status_{};
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @retval std::nullopt schema errorまたはpreflight失敗時。
		 * @pre schema内のview fieldは参照元byte列のlifetimeを呼び出し中維持。
		 * @post 成功時だけbytesを保持。`value`の変更なし。
		 * @note owning buffer確保に失敗した場合は標準例外が送出され、EncodeResultは返らない。
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @note fixed-size schemaで明らかなoutput不足の場合、validation hookは実行しない。
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
		 */
		template <typename T>
		Field<T> ConstU8(std::string_view name, std::uint8_t expected) noexcept;

		/**
		 * @brief constant fixed bytes field descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name field名。
		 * @param[in] expected 期待するbyte列先頭。`size == 0`の場合のみnullptr可。非所有参照。
		 * @param[in] size 期待するbyte数。
		 * @retval value field descriptor。
		 * @pre `expected`はschema利用中に参照可能なstatic storageまたはcaller管理storage。
		 * @post encode時は`expected[0..size)`を書き込み。
		 * @note stack配列など短命storageを渡す場合は、所有copyを作る`std::array` overloadを使う。
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
		 */
		template <typename T>
		Field<T>
		ConstBytes(std::string_view name, const std::uint8_t* expected, std::size_t size) noexcept;

		/**
		 * @brief constant fixed bytes field descriptorを`std::array`から所有copyで生成。
		 * @tparam T schema対象型。
		 * @tparam Size 期待するbyte数。32byte以下。
		 * @param[in] name field名。
		 * @param[in] expected 期待するbyte列。descriptor内へcopy。
		 * @retval value field descriptor。
		 * @pre `Size <= 32`。大きいconstant byte列は長命storageのpointer overloadを使う。
		 * @post schemaへcopyされた後も`expected`引数のlifetimeに依存しない。
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
		 */
		template <typename T, std::size_t Size>
		Field<T> ConstBytes(std::string_view name,
							const std::array<std::uint8_t, Size>& expected) noexcept;

		/**
		 * @brief repeated reserved bytes field descriptor生成。
		 * @tparam T schema対象型。
		 * @param[in] name field名。
		 * @param[in] size reserved byte数。
		 * @param[in] expected 各byteの期待値。
		 * @retval value field descriptor。
		 * @pre `name`はschema利用中に参照可能なstorage。
		 * @post decodeは全byte一致を検査し、encodeは期待値を埋める。
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @pre `members`は16要素以下で、1byte storageの全bitを重複なしに覆う。未使用bitは
		 * `ReservedBits`で明示。
		 * @post decode/encodeはstorage unitを1回だけ消費し、未被覆bitを持つdescriptorはschema
		 * errorとして扱う。
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @pre `members`は16要素以下で、2byte storageの全bitを重複なしに覆う。未使用bitは
		 * `ReservedBits`で明示。
		 * @post decode/encodeはstorage unitを1回だけ消費し、未被覆bitを持つdescriptorはschema
		 * errorとして扱う。
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @pre `members`は16要素以下で、2byte storageの全bitを重複なしに覆う。未使用bitは
		 * `ReservedBits`で明示。
		 * @post decode/encodeはstorage unitを1回だけ消費し、未被覆bitを持つdescriptorはschema
		 * errorとして扱う。
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @post decode/encode/EncodeTo/measure時にbyteを消費せずcallbackを実行。
		 * @note callbackがfalseを返して`Status::offset`を設定した場合、その値はvalidation
		 * field位置からの相対byte offsetとして扱い、result statusではschema先頭基準へ補正。
		 * callbackがStatusをOKのままfalseを返した場合、validation field位置をfailure offsetにする。
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
		 * @code
		 * // See packages/wire/ket_wire_test.cpp for executable usage of this API.
		 * @endcode
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
			 * @brief BitMember private state access helper.
			 * @tparam T schema対象型。
			 */
			template <typename T>
			struct BitMemberAccess
			{
				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::string_view Name(const BitMember<T>& member) noexcept
				{
					return member.name_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetName(BitMember<T>& member, std::string_view name) noexcept
				{
					member.name_ = name;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr unsigned Shift(const BitMember<T>& member) noexcept
				{
					return member.shift_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetShift(BitMember<T>& member, unsigned shift) noexcept
				{
					member.shift_ = shift;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr unsigned Width(const BitMember<T>& member) noexcept
				{
					return member.width_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetWidth(BitMember<T>& member, unsigned width) noexcept
				{
					member.width_ = width;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::uint64_t Expected(const BitMember<T>& member) noexcept
				{
					return member.expected_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetExpected(BitMember<T>& member, std::uint64_t expected) noexcept
				{
					member.expected_ = expected;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr bool HasMember(const BitMember<T>& member) noexcept
				{
					return member.has_member_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetHasMember(BitMember<T>& member, bool has_member) noexcept
				{
					member.has_member_ = has_member;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr bool IsValid(const BitMember<T>& member) noexcept
				{
					return member.is_valid_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetIsValid(BitMember<T>& member, bool is_valid) noexcept
				{
					member.is_valid_ = is_valid;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr typename BitMember<T>::GetFunction
				Get(const BitMember<T>& member) noexcept
				{
					return member.get_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetGet(BitMember<T>& member,
								   typename BitMember<T>::GetFunction get) noexcept
				{
					member.get_ = get;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr typename BitMember<T>::SetFunction
				Set(const BitMember<T>& member) noexcept
				{
					return member.set_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetSetter(BitMember<T>& member,
									  typename BitMember<T>::SetFunction setter) noexcept
				{
					member.set_ = setter;
				}
			};

			/**
			 * @brief Field private state access helper.
			 * @tparam T schema対象型。
			 */
			template <typename T>
			struct FieldAccess
			{
				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::string_view Name(const Field<T>& field) noexcept
				{
					return field.name_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetName(Field<T>& field, std::string_view name) noexcept
				{
					field.name_ = name;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::string_view Group(const Field<T>& field) noexcept
				{
					return field.group_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetGroup(Field<T>& field, std::string_view group) noexcept
				{
					field.group_ = group;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr FieldKind Kind(const Field<T>& field) noexcept
				{
					return field.kind_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetKind(Field<T>& field, FieldKind kind) noexcept
				{
					field.kind_ = kind;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::size_t EncodedSize(const Field<T>& field) noexcept
				{
					return field.encoded_size_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetEncodedSize(Field<T>& field, std::size_t encoded_size) noexcept
				{
					field.encoded_size_ = encoded_size;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::size_t MaxEncodedSize(const Field<T>& field) noexcept
				{
					return field.max_encoded_size_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetMaxEncodedSize(Field<T>& field,
											  std::size_t max_encoded_size) noexcept
				{
					field.max_encoded_size_ = max_encoded_size;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr bool IsFixedSize(const Field<T>& field) noexcept
				{
					return field.is_fixed_size_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetIsFixedSize(Field<T>& field, bool is_fixed_size) noexcept
				{
					field.is_fixed_size_ = is_fixed_size;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr bool IsValid(const Field<T>& field) noexcept
				{
					return field.is_valid_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetIsValid(Field<T>& field, bool is_valid) noexcept
				{
					field.is_valid_ = is_valid;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::uint64_t Expected(const Field<T>& field) noexcept
				{
					return field.expected_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetExpected(Field<T>& field, std::uint64_t expected) noexcept
				{
					field.expected_ = expected;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr const std::uint8_t* ExpectedBytes(const Field<T>& field) noexcept
				{
					return field.expected_bytes_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetExpectedBytes(Field<T>& field, const std::uint8_t* expected) noexcept
				{
					field.expected_bytes_ = expected;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::size_t ExpectedBytesSize(const Field<T>& field) noexcept
				{
					return field.expected_bytes_size_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetExpectedBytesSize(Field<T>& field, std::size_t size) noexcept
				{
					field.expected_bytes_size_ = size;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr bool OwnsExpectedBytes(const Field<T>& field) noexcept
				{
					return field.owns_expected_bytes_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre `size <= 32`。
				 * @post 対象descriptorのowned expected bytesを更新。
				 */
				static void SetOwnedExpectedBytes(Field<T>& field,
												  const std::uint8_t* expected,
												  std::size_t size) noexcept
				{
					field.owns_expected_bytes_ = true;
					field.expected_bytes_ = nullptr;
					field.expected_bytes_size_ = size;
					for (std::size_t index = 0U; index < size; ++index)
					{
						field.owned_expected_bytes_[index] = expected[index];
					}
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr const std::array<BitMember<T>, 16U>&
				BitMembers(const Field<T>& field) noexcept
				{
					return field.bit_members_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetBitMember(Field<T>& field,
										 std::size_t index,
										 const BitMember<T>& member) noexcept
				{
					field.bit_members_[index] = member;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::size_t BitMemberCount(const Field<T>& field) noexcept
				{
					return field.bit_member_count_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetBitMemberCount(Field<T>& field, std::size_t count) noexcept
				{
					field.bit_member_count_ = count;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr typename Field<T>::DecodeFunction
				Decode(const Field<T>& field) noexcept
				{
					return field.decode_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetDecode(Field<T>& field,
									  typename Field<T>::DecodeFunction decode) noexcept
				{
					field.decode_ = decode;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr typename Field<T>::PreflightFunction
				Preflight(const Field<T>& field) noexcept
				{
					return field.preflight_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetPreflight(Field<T>& field,
										 typename Field<T>::PreflightFunction preflight) noexcept
				{
					field.preflight_ = preflight;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr typename Field<T>::EncodeFunction
				Encode(const Field<T>& field) noexcept
				{
					return field.encode_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetEncode(Field<T>& field,
									  typename Field<T>::EncodeFunction encode) noexcept
				{
					field.encode_ = encode;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr typename Field<T>::MeasureFunction
				Measure(const Field<T>& field) noexcept
				{
					return field.measure_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetMeasure(Field<T>& field,
									   typename Field<T>::MeasureFunction measure) noexcept
				{
					field.measure_ = measure;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr typename Field<T>::ValidationFunction
				Validation(const Field<T>& field) noexcept
				{
					return field.validation_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetValidation(Field<T>& field,
										  typename Field<T>::ValidationFunction validation) noexcept
				{
					field.validation_ = validation;
				}
			};

			/**
			 * @brief Schema private state access helper.
			 * @tparam T schema対象型。
			 * @tparam FieldCount field descriptor数。
			 */
			template <typename T, std::size_t FieldCount>
			struct SchemaAccess
			{
				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::string_view Name(const Schema<T, FieldCount>& schema) noexcept
				{
					return schema.name_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetName(Schema<T, FieldCount>& schema, std::string_view name) noexcept
				{
					schema.name_ = name;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr const std::array<Field<T>, FieldCount>&
				Fields(const Schema<T, FieldCount>& schema) noexcept
				{
					return schema.fields_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetFields(Schema<T, FieldCount>& schema,
									  const std::array<Field<T>, FieldCount>& fields) noexcept
				{
					schema.fields_ = fields;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::size_t
				FieldCountValue(const Schema<T, FieldCount>& schema) noexcept
				{
					return schema.field_count_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr bool IsFixedSize(const Schema<T, FieldCount>& schema) noexcept
				{
					return schema.is_fixed_size_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetIsFixedSize(Schema<T, FieldCount>& schema,
										   bool is_fixed_size) noexcept
				{
					schema.is_fixed_size_ = is_fixed_size;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::size_t FixedSize(const Schema<T, FieldCount>& schema) noexcept
				{
					return schema.fixed_size_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetFixedSize(Schema<T, FieldCount>& schema, std::size_t size) noexcept
				{
					schema.fixed_size_ = size;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr std::size_t MaxSize(const Schema<T, FieldCount>& schema) noexcept
				{
					return schema.max_size_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetMaxSize(Schema<T, FieldCount>& schema, std::size_t size) noexcept
				{
					schema.max_size_ = size;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static constexpr const Status&
				SchemaStatus(const Schema<T, FieldCount>& schema) noexcept
				{
					return schema.status_;
				}

				/**
				 * @brief descriptor private state access.
				 * @param[in,out] object 対象descriptor。
				 * @retval value 取得値、またはvoid。
				 * @pre なし。
				 * @post getterは変更なし、setterは対象descriptorを更新。
				 */
				static void SetSchemaStatus(Schema<T, FieldCount>& schema,
											const Status& status) noexcept
				{
					schema.status_ = status;
				}
			};

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
			 * @brief field-relative offsetをschema先頭からのoffsetへ変換。
			 * @param[in,out] status 変換対象status。
			 * @param[in] field_start field開始offset。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post overflowしない場合だけ`status.offset`を更新。
			 */
			inline void AddFieldStartToStatus(Status& status, std::size_t field_start) noexcept
			{
				std::size_t adjusted_offset = 0U;
				const auto offset_adjusted =
					ket::numeric::TryAdd<std::size_t>(field_start, status.offset, adjusted_offset);
				if (offset_adjusted)
				{
					status.offset = adjusted_offset;
				}
			}

			/**
			 * @brief validation callback失敗statusをfield文脈で補完。
			 * @param[in,out] status callbackが返したstatus。
			 * @param[in] field_start validation field開始offset。
			 * @param[in] field field名。
			 * @param[in] group group名。
			 * @retval void 戻り値なし。
			 * @pre callbackがfalseを返した直後。
			 * @post
			 * statusが未設定ならkCallbackFailed、設定済みならoffsetをfield-relativeとして補正。
			 */
			inline void CompleteValidationFailureStatus(Status& status,
														std::size_t field_start,
														std::string_view field,
														std::string_view group) noexcept
			{
				const auto status_ok = status.Ok();
				if (status_ok)
				{
					status = MakeStatus(Error::kCallbackFailed, field_start, field, group);
					return;
				}

				AddFieldStartToStatus(status, field_start);
				const auto field_is_empty = status.field.empty();
				if (field_is_empty)
				{
					status.field = field;
				}
				const auto group_is_empty = status.group.empty();
				if (group_is_empty)
				{
					status.group = group;
				}
			}

			/**
			 * @brief input view validity判定。
			 * @param[in] data input view。
			 * @retval true valid view。
			 * @retval false invalid view。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			constexpr bool IsValidInput(ket::byte_view::View data) noexcept
			{
				return ket::byte_view::IsValid(data);
			}

			/**
			 * @brief output view validity判定。
			 * @param[in] out output view。
			 * @retval true valid view。
			 * @retval false invalid view。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			constexpr bool IsValidOutput(ket::byte_view::MutableView out) noexcept
			{
				return ket::byte_view::IsValid(out);
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
			Unsigned LoadInteger(const std::uint8_t* data, ByteOrder order) noexcept
			{
				static_assert(std::is_unsigned_v<Unsigned>, "Unsigned must be unsigned");
				if constexpr (sizeof(Unsigned) == sizeof(std::uint8_t))
				{
					return static_cast<Unsigned>(data[0]);
				}
				else
				{
					if constexpr (sizeof(Unsigned) == sizeof(std::uint16_t))
					{
						if (order == ByteOrder::kBig)
						{
							const auto raw = ket::endian::LoadBe16(data);
							return static_cast<Unsigned>(raw);
						}

						const auto raw = ket::endian::LoadLe16(data);
						return static_cast<Unsigned>(raw);
					}
					else
					{
						if constexpr (sizeof(Unsigned) == sizeof(std::uint32_t))
						{
							if (order == ByteOrder::kBig)
							{
								const auto raw = ket::endian::LoadBe32(data);
								return static_cast<Unsigned>(raw);
							}

							const auto raw = ket::endian::LoadLe32(data);
							return static_cast<Unsigned>(raw);
						}
						else
						{
							static_assert(sizeof(Unsigned) == sizeof(std::uint64_t),
										  "Unsupported unsigned integer width");
							if (order == ByteOrder::kBig)
							{
								const auto raw = ket::endian::LoadBe64(data);
								return static_cast<Unsigned>(raw);
							}

							const auto raw = ket::endian::LoadLe64(data);
							return static_cast<Unsigned>(raw);
						}
					}
				}
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
			void StoreInteger(std::uint8_t* data, Unsigned value, ByteOrder order) noexcept
			{
				static_assert(std::is_unsigned_v<Unsigned>, "Unsigned must be unsigned");
				if constexpr (sizeof(Unsigned) == sizeof(std::uint8_t))
				{
					data[0] = static_cast<std::uint8_t>(value);
					return;
				}
				else
				{
					if constexpr (sizeof(Unsigned) == sizeof(std::uint16_t))
					{
						if (order == ByteOrder::kBig)
						{
							ket::endian::StoreBe16(data, static_cast<std::uint16_t>(value));
						}
						else
						{
							ket::endian::StoreLe16(data, static_cast<std::uint16_t>(value));
						}
						return;
					}
					else
					{
						if constexpr (sizeof(Unsigned) == sizeof(std::uint32_t))
						{
							if (order == ByteOrder::kBig)
							{
								ket::endian::StoreBe32(data, static_cast<std::uint32_t>(value));
							}
							else
							{
								ket::endian::StoreLe32(data, static_cast<std::uint32_t>(value));
							}
							return;
						}
						else
						{
							static_assert(sizeof(Unsigned) == sizeof(std::uint64_t),
										  "Unsupported unsigned integer width");
							if (order == ByteOrder::kBig)
							{
								ket::endian::StoreBe64(data, static_cast<std::uint64_t>(value));
							}
							else
							{
								ket::endian::StoreLe64(data, static_cast<std::uint64_t>(value));
							}
						}
					}
				}
			}

			/**
			 * @brief field byte列をoffset位置から参照。
			 * @tparam T schema対象型。
			 * @param[in] data input view。
			 * @param[in] offset field開始offset。
			 * @param[in] field field descriptor。
			 * @param[out] status failure status。
			 * @param[out] out field byte列先頭。
			 * @retval true 参照成功。
			 * @retval false input不足。
			 * @pre `out`は有効な参照。
			 * @post 成功時だけ`out`を更新。
			 */
			template <typename T>
			bool PeekFieldBytes(ket::byte_view::View data,
								std::size_t offset,
								const Field<T>& field,
								Status& status,
								const std::uint8_t*& out) noexcept
			{
				const auto field_start = offset;
				const auto input_valid = IsValidInput(data);
				const auto offset_valid = input_valid && offset <= data.Size();
				const auto available = offset_valid ? ket::byte_view::Remaining(data, offset) : 0U;
				const auto* data_ptr = data.Data();
				const auto has_data = data_ptr != nullptr;
				const std::uint8_t* start = nullptr;
				if (offset_valid && has_data)
				{
					start = data_ptr + offset;
				}

				auto reader = ket::byte_reader::Reader(start, available);
				const auto read = offset_valid && reader.ReadBytes(field.EncodedSize(), out);
				if (!read)
				{
					status = MakeSizeStatus(Error::kShortInput,
											field_start,
											field.Name(),
											field.Group(),
											field.EncodedSize(),
											available);
					return false;
				}

				return true;
			}

			/**
			 * @brief field byte列をoffset位置へ書き込み。
			 * @tparam T schema対象型。
			 * @param[in,out] out output buffer先頭。
			 * @param[in,out] offset current offset。
			 * @param[in] field field descriptor。
			 * @param[in] source field byte列先頭。
			 * @retval void 戻り値なし。
			 * @pre `out`は十分な容量を持つbuffer。
			 * @post `offset`を実書き込みbyte数だけ更新。
			 */
			template <typename T>
			void WriteFieldBytes(std::uint8_t* out,
								 std::size_t& offset,
								 const Field<T>& field,
								 const std::uint8_t* source) noexcept
			{
				const auto encoded_size = field.EncodedSize();
				std::uint8_t* start = nullptr;
				const auto has_output = encoded_size > 0U;
				if (has_output)
				{
					start = out + offset;
				}

				auto writer = ket::byte_writer::Writer(start, encoded_size);
				const auto wrote = writer.WriteBytes(source, encoded_size);
				(void)wrote;
				offset += writer.Offset();
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
				const std::uint8_t* bytes = nullptr;
				const auto read = PeekFieldBytes(data, offset, field, status, bytes);
				if (!read)
				{
					return false;
				}

				const auto raw = LoadInteger<Unsigned>(bytes, Order);
				if constexpr (std::is_signed_v<MemberType>)
				{
					value.*Member = DecodeSigned<MemberType, Unsigned>(raw);
				}
				else
				{
					value.*Member = static_cast<MemberType>(raw);
				}
				offset += field.EncodedSize();
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
				const auto added =
					ket::numeric::TryAdd<std::size_t>(encoded_size, field.EncodedSize(), next_size);
				if (!added)
				{
					status = MakeSizeStatus(Error::kSizeOverflow,
											encoded_size,
											field.Name(),
											field.Group(),
											field.EncodedSize(),
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

				std::array<std::uint8_t, sizeof(Unsigned)> encoded{};
				StoreInteger<Unsigned>(encoded.data(), raw, Order);
				WriteFieldBytes(out, offset, field, encoded.data());
			}

			/**
			 * @brief integer field descriptor生成。
			 * @tparam T schema対象型。
			 * @tparam Member member pointer.
			 * @tparam Unsigned wire unsigned type.
			 * @tparam ExpectedMember exact object member type.
			 * @tparam Order byte order.
			 * @param[in] name field名。
			 * @retval value field descriptor。
			 * @pre member type width matches `Unsigned`.
			 * @post allocationなし。
			 */
			template <typename T,
					  auto Member,
					  typename Unsigned,
					  typename ExpectedMember,
					  ByteOrder Order>
			Field<T> MakeIntegerField(std::string_view name) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				CheckMemberClass<T, Member>();
				static_assert(std::is_same_v<MemberType, ExpectedMember>,
							  "integer member must be the exact fixed-width signedness type");
				static_assert(sizeof(ExpectedMember) == sizeof(Unsigned),
							  "integer member and wire size mismatch");

				Field<T> field{};
				FieldAccess<T>::SetName(field, name);
				FieldAccess<T>::SetKind(field, FieldKind::kInteger);
				FieldAccess<T>::SetEncodedSize(field, IntegerByteSize<Unsigned>());
				FieldAccess<T>::SetMaxEncodedSize(field, field.EncodedSize());
				FieldAccess<T>::SetIsValid(field, true);
				FieldAccess<T>::SetDecode(field, &DecodeInteger<T, Member, Unsigned, Order>);
				FieldAccess<T>::SetPreflight(field, &PreflightNoop<T>);
				FieldAccess<T>::SetEncode(field, &EncodeInteger<T, Member, Unsigned, Order>);
				FieldAccess<T>::SetMeasure(field, &MeasureFixed<T>);
				return field;
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
				const std::uint8_t* bytes = nullptr;
				const auto read = PeekFieldBytes(data, offset, field, status, bytes);
				if (!read)
				{
					return false;
				}

				const auto validation = ket::bcd::Validate(bytes, field.EncodedSize());
				const auto bcd_is_valid = validation.Ok();
				if (!bcd_is_valid)
				{
					status = MakeExpectedStatus(Error::kInvalidBcd,
												field_start + validation.byte_offset,
												field.Name(),
												field.Group(),
												9U,
												validation.actual);
					return false;
				}

				const auto raw = static_cast<Unsigned>(LoadInteger<Unsigned>(bytes, Order));
				const auto decoded = ket::bcd::ToInt(raw);
				const auto decoded_has_value = decoded.has_value();
				if (!decoded_has_value)
				{
					status = MakeExpectedStatus(
						Error::kInvalidBcd, field_start, field.Name(), field.Group(), 9U, 0U);
					return false;
				}

				value.*Member = *decoded;
				offset += field.EncodedSize();
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
			template <typename T, auto Member, typename Unsigned>
			bool PreflightBcdInteger(const T& value, const Field<T>& field, Status& status) noexcept
			{
				using MemberType = typename MemberPointerTraits<decltype(Member)>::member_type;
				static_assert(std::is_same_v<MemberType, int>, "BCD decoded member must be int");
				const auto encoded = ket::bcd::FromInt<Unsigned>(value.*Member);
				const auto built = encoded.has_value();
				if (!built)
				{
					const auto actual =
						value.*Member < 0 ? 0U : static_cast<std::uint64_t>(value.*Member);
					status =
						MakeExpectedStatus(Error::kValueOutOfRange,
										   0U,
										   field.Name(),
										   field.Group(),
										   static_cast<std::uint64_t>(ket::bcd::MaxInt<Unsigned>()),
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
				const auto encoded = ket::bcd::FromInt<Unsigned>(value.*Member);
				const auto raw = encoded.value_or(Unsigned{0});
				std::array<std::uint8_t, sizeof(Unsigned)> bytes{};
				StoreInteger<Unsigned>(bytes.data(), raw, Order);
				WriteFieldBytes(out, offset, field, bytes.data());
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
				FieldAccess<T>::SetName(field, name);
				FieldAccess<T>::SetKind(field, FieldKind::kBcdInteger);
				FieldAccess<T>::SetEncodedSize(field, IntegerByteSize<Unsigned>());
				FieldAccess<T>::SetMaxEncodedSize(field, field.EncodedSize());
				FieldAccess<T>::SetIsValid(field, true);
				FieldAccess<T>::SetDecode(field, &DecodeBcdInteger<T, Member, Unsigned, Order>);
				FieldAccess<T>::SetPreflight(field, &PreflightBcdInteger<T, Member, Unsigned>);
				FieldAccess<T>::SetEncode(field, &EncodeBcdInteger<T, Member, Unsigned, Order>);
				FieldAccess<T>::SetMeasure(field, &MeasureFixed<T>);
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
				const std::uint8_t* bytes = nullptr;
				const auto read = PeekFieldBytes(data, offset, field, status, bytes);
				if (!read)
				{
					return false;
				}

				if constexpr (ValidateBcd)
				{
					const auto validation = ket::bcd::Validate(bytes, field.EncodedSize());
					const auto bcd_is_valid = validation.Ok();
					if (!bcd_is_valid)
					{
						status = MakeExpectedStatus(Error::kInvalidBcd,
													field_start + validation.byte_offset,
													field.Name(),
													field.Group(),
													9U,
													validation.actual);
						return false;
					}
				}

				const auto encoded_size = field.EncodedSize();
				const auto has_bytes = encoded_size > 0U;
				if (has_bytes)
				{
					auto* destination = MutableByteStorageData(value.*Member);
					for (std::size_t index = 0U; index < encoded_size; ++index)
					{
						destination[index] = bytes[index];
					}
				}

				offset += encoded_size;
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
					const auto validation = ket::bcd::Validate(source, field.EncodedSize());
					const auto bcd_is_valid = validation.Ok();
					if (!bcd_is_valid)
					{
						status = MakeExpectedStatus(Error::kInvalidBcd,
													validation.byte_offset,
													field.Name(),
													field.Group(),
													9U,
													validation.actual);
						return false;
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
				const auto encoded_size = field.EncodedSize();
				const auto has_bytes = encoded_size > 0U;
				if (has_bytes)
				{
					const auto* source = ByteStorageData(value.*Member);
					WriteFieldBytes(out, offset, field, source);
				}
				else
				{
					WriteFieldBytes(out, offset, field, nullptr);
				}
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
				FieldAccess<T>::SetName(field, name);
				FieldAccess<T>::SetKind(field,
										ValidateBcd ? FieldKind::kRawBcdBytes : FieldKind::kBytes);
				FieldAccess<T>::SetEncodedSize(field, Traits::kSize);
				FieldAccess<T>::SetMaxEncodedSize(field, field.EncodedSize());
				FieldAccess<T>::SetIsValid(field, true);
				FieldAccess<T>::SetDecode(field, &DecodeBytes<T, Member, ValidateBcd>);
				FieldAccess<T>::SetPreflight(field, &PreflightBytes<T, Member, ValidateBcd>);
				FieldAccess<T>::SetEncode(field, &EncodeBytes<T, Member>);
				FieldAccess<T>::SetMeasure(field, &MeasureFixed<T>);
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
				const std::uint8_t* bytes = nullptr;
				const auto read = PeekFieldBytes(data, offset, field, status, bytes);
				if (!read)
				{
					return false;
				}

				const std::uint8_t* view_data = nullptr;
				const auto encoded_size = field.EncodedSize();
				const auto has_bytes = encoded_size > 0U;
				if (has_bytes)
				{
					view_data = bytes;
				}

				value.*Member = ket::byte_view::View(view_data, encoded_size);
				offset += encoded_size;
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
				const auto is_valid = IsValidInput(view);
				if (!is_valid)
				{
					status = MakeSizeStatus(Error::kInvalidInputView,
											0U,
											field.Name(),
											field.Group(),
											field.EncodedSize(),
											view.Size());
					return false;
				}

				const auto size_matches = view.Size() == field.EncodedSize();
				if (!size_matches)
				{
					status = MakeSizeStatus(Error::kLengthMismatch,
											0U,
											field.Name(),
											field.Group(),
											field.EncodedSize(),
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
				const auto encoded_size = field.EncodedSize();
				const auto has_bytes = encoded_size > 0U;
				if (has_bytes)
				{
					const auto view = value.*Member;
					WriteFieldBytes(out, offset, field, view.Data());
				}
				else
				{
					WriteFieldBytes(out, offset, field, nullptr);
				}
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
				const std::uint8_t* bytes = nullptr;
				const auto read = PeekFieldBytes(data, offset, field, status, bytes);
				if (!read)
				{
					return false;
				}

				for (std::size_t index = 0U; index < field.EncodedSize(); ++index)
				{
					const auto actual = bytes[index];
					auto expected = static_cast<std::uint8_t>(field.Expected());
					const auto* expected_bytes = field.ExpectedBytes();
					const auto has_expected_bytes = expected_bytes != nullptr;
					if (has_expected_bytes)
					{
						expected = expected_bytes[index];
					}

					const auto matches = actual == expected;
					if (!matches)
					{
						status = MakeExpectedStatus(Error::kReservedMismatch,
													field_start + index,
													field.Name(),
													field.Group(),
													expected,
													actual);
						return false;
					}
				}

				offset += field.EncodedSize();
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
				const auto* expected_bytes = field.ExpectedBytes();
				const auto has_expected_bytes = expected_bytes != nullptr;
				if (has_expected_bytes)
				{
					WriteFieldBytes(out, offset, field, expected_bytes);
					return;
				}

				std::array<std::uint8_t, 1U> expected{static_cast<std::uint8_t>(field.Expected())};
				const auto encoded_size = field.EncodedSize();
				const auto has_bytes = encoded_size > 0U;
				std::uint8_t* start = nullptr;
				if (has_bytes)
				{
					start = out + offset;
				}

				auto writer = ket::byte_writer::Writer(start, encoded_size);
				for (std::size_t index = 0U; index < encoded_size; ++index)
				{
					const auto wrote = writer.WriteBytes(expected.data(), expected.size());
					(void)wrote;
				}
				offset += writer.Offset();
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
				FieldAccess<T>::SetName(field, name);
				FieldAccess<T>::SetKind(field, kind);
				FieldAccess<T>::SetEncodedSize(field, size);
				FieldAccess<T>::SetMaxEncodedSize(field, size);
				FieldAccess<T>::SetIsValid(field, true);
				FieldAccess<T>::SetExpected(field, expected);
				FieldAccess<T>::SetDecode(field, &DecodeExpectedBytes<T>);
				FieldAccess<T>::SetPreflight(field, &PreflightNoop<T>);
				FieldAccess<T>::SetEncode(field, &EncodeExpectedBytes<T>);
				FieldAccess<T>::SetMeasure(field, &MeasureFixed<T>);
				return field;
			}

			/**
			 * @brief bit mask取得。
			 * @param[in] width bit幅。
			 * @param[out] out lower width bits mask。
			 * @retval true mask取得成功。
			 * @retval false widthがstd::uint64_tのbit幅を超過。
			 * @pre `out`は有効な参照。
			 * @post 成功時だけ`out`を更新。
			 */
			inline bool TryBitMask(unsigned width, std::uint64_t& out) noexcept
			{
				return ket::bits::TryMask<std::uint64_t>(width, out);
			}

			/**
			 * @brief bit memberがstorage bit幅に収まるか判定。
			 * @tparam Unsigned storage unsigned type.
			 * @tparam T schema対象型。
			 * @param[in] member bit member descriptor。
			 * @retval true storage bit幅内。
			 * @retval false shiftまたはwidthがstorage bit幅外。
			 * @pre `Unsigned`はunsigned integer type。
			 * @post 外部状態の変更なし。
			 */
			template <typename Unsigned, typename T>
			bool BitMemberFitsStorage(const BitMember<T>& member) noexcept
			{
				const auto storage_width = ket::bits::TypeBitWidth<Unsigned>();
				const auto shift_fits = member.Shift() < storage_width;
				if (!shift_fits)
				{
					return false;
				}

				return member.Width() <= (storage_width - member.Shift());
			}

			/**
			 * @brief bit memberが占有するstorage mask取得。
			 * @tparam T schema対象型。
			 * @param[in] member bit member descriptor。
			 * @param[out] out shifted coverage mask。
			 * @retval true mask取得成功。
			 * @retval false widthがmask生成範囲外。
			 * @pre `member`はstorage bit幅内。
			 * @post 成功時だけ`out`を更新。
			 */
			template <typename T>
			bool TryBitMemberCoverage(const BitMember<T>& member, std::uint64_t& out) noexcept
			{
				std::uint64_t mask = 0U;
				const auto mask_ok = TryBitMask(member.Width(), mask);
				if (!mask_ok)
				{
					return false;
				}

				out = mask << member.Shift();
				return true;
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
				const std::uint8_t* bytes = nullptr;
				const auto read = PeekFieldBytes(data, offset, field, status, bytes);
				if (!read)
				{
					return false;
				}

				const auto raw_storage =
					static_cast<std::uint64_t>(LoadInteger<Unsigned>(bytes, Order));
				for (std::size_t index = 0U; index < field.BitMemberCount(); ++index)
				{
					const auto& member = field.BitMembers()[index];
					std::uint64_t mask = 0U;
					const auto mask_ok = TryBitMask(member.Width(), mask);
					(void)mask_ok;
					const auto actual = (raw_storage >> member.Shift()) & mask;
					const auto has_member = member.HasMember();
					if (!has_member)
					{
						const auto matches = actual == member.Expected();
						if (!matches)
						{
							status = MakeExpectedStatus(Error::kReservedMismatch,
														field_start,
														member.Name(),
														field.Group(),
														member.Expected(),
														actual);
							return false;
						}
					}
					else
					{
						BitMemberAccess<T>::Set(member)(value, actual);
					}
				}

				offset += field.EncodedSize();
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
				for (std::size_t index = 0U; index < field.BitMemberCount(); ++index)
				{
					const auto& member = field.BitMembers()[index];
					const auto has_member = member.HasMember();
					if (!has_member)
					{
						continue;
					}

					std::uint64_t actual = 0U; // NOLINT(misc-const-correctness)
					const auto got_value =
						BitMemberAccess<T>::Get(member)(value, member, actual, status);
					if (!got_value)
					{
						return false;
					}

					std::uint64_t max_value = 0U;
					const auto mask_ok = TryBitMask(member.Width(), max_value);
					(void)mask_ok;
					const auto fits = actual <= max_value;
					if (!fits)
					{
						status = MakeExpectedStatus(Error::kValueOutOfRange,
													0U,
													member.Name(),
													field.Group(),
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
				for (std::size_t index = 0U; index < field.BitMemberCount(); ++index)
				{
					const auto& member = field.BitMembers()[index];
					std::uint64_t actual = member.Expected(); // NOLINT(misc-const-correctness)
					const auto has_member = member.HasMember();
					if (has_member)
					{
						const auto got_value =
							BitMemberAccess<T>::Get(member)(value, member, actual, ignored_status);
						(void)got_value;
					}

					std::uint64_t mask = 0U;
					const auto mask_ok = TryBitMask(member.Width(), mask);
					(void)mask_ok;
					raw_storage |= (actual & mask) << member.Shift();
				}

				std::array<std::uint8_t, sizeof(Unsigned)> bytes{};
				StoreInteger<Unsigned>(bytes.data(), static_cast<Unsigned>(raw_storage), Order);
				WriteFieldBytes(out, offset, field, bytes.data());
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
			 * @pre `BitCount <= 16` and members cover every storage bit without overlap.
			 * @post allocationなし。
			 */
			template <typename T, typename Unsigned, ByteOrder Order, std::size_t BitCount>
			Field<T> MakeBitsField(std::string_view group,
								   std::array<BitMember<T>, BitCount> members) noexcept
			{
				Field<T> field{};
				FieldAccess<T>::SetName(field, group);
				FieldAccess<T>::SetGroup(field, group);
				FieldAccess<T>::SetKind(field, FieldKind::kBits);
				FieldAccess<T>::SetEncodedSize(field, IntegerByteSize<Unsigned>());
				FieldAccess<T>::SetMaxEncodedSize(field, field.EncodedSize());
				FieldAccess<T>::SetIsValid(field, BitCount <= field.BitMembers().size());
				FieldAccess<T>::SetDecode(field, &DecodeBits<T, Unsigned, Order>);
				FieldAccess<T>::SetPreflight(field, &PreflightBits<T>);
				FieldAccess<T>::SetEncode(field, &EncodeBits<T, Unsigned, Order>);
				FieldAccess<T>::SetMeasure(field, &MeasureFixed<T>);
				const auto field_is_valid = field.IsValid();
				if (!field_is_valid)
				{
					return field;
				}

				std::uint64_t used_mask = 0U;
				for (std::size_t index = 0U; index < BitCount; ++index)
				{
					const auto& member = members[index];
					const auto member_valid = member.IsValid();
					const auto member_fits_storage = BitMemberFitsStorage<Unsigned>(member);
					std::uint64_t member_coverage = 0U;
					bool coverage_ok = false;
					if (member_fits_storage)
					{
						coverage_ok = TryBitMemberCoverage(member, member_coverage);
					}
					const auto member_overlaps =
						coverage_ok && ((used_mask & member_coverage) != 0U);
					if (!member_valid || !member_fits_storage || !coverage_ok || member_overlaps)
					{
						FieldAccess<T>::SetIsValid(field, false);
						return field;
					}

					used_mask |= member_coverage;
					FieldAccess<T>::SetBitMember(field, index, member);
				}

				std::uint64_t storage_mask = 0U;
				const auto storage_mask_ok =
					TryBitMask(ket::bits::TypeBitWidth<Unsigned>(), storage_mask);
				const auto storage_is_fully_covered = storage_mask_ok && used_mask == storage_mask;
				if (!storage_is_fully_covered)
				{
					FieldAccess<T>::SetIsValid(field, false);
					return field;
				}

				FieldAccess<T>::SetBitMemberCount(field, BitCount);
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
				const auto callback_ok = FieldAccess<T>::Validation(field)(value, status);
				if (!callback_ok)
				{
					CompleteValidationFailureStatus(status, offset, field.Name(), field.Group());
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
				const auto callback_ok = FieldAccess<T>::Validation(field)(value, status);
				if (!callback_ok)
				{
					CompleteValidationFailureStatus(status, 0U, field.Name(), field.Group());
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
			 * @brief validation field size measure no-op。
			 * @tparam T schema対象型。
			 * @param[in] value 測定対象object。
			 * @param[in] field field descriptor。
			 * @param[in,out] encoded_size 現在までのencoded size。
			 * @param[out] status failure status。
			 * @retval true 常に成功。
			 * @retval false 未使用。
			 * @pre callback is valid.
			 * @post encoded_size、status、外部状態の変更なし。
			 */
			template <typename T>
			bool MeasureValidation(const T& value,
								   const Field<T>& field,
								   std::size_t& encoded_size,
								   Status& status) noexcept
			{
				(void)value;
				(void)field;
				(void)encoded_size;
				(void)status;
				return true;
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
				const auto schema_ok = schema.SchemaStatus().Ok();
				if (!schema_ok)
				{
					status = schema.SchemaStatus();
					return false;
				}

				return true;
			}

			/**
			 * @brief output bufferがschema field列を収容できるか検査。
			 * @tparam T schema対象型。
			 * @tparam FieldCount field descriptor数。
			 * @param[in] schema 対象schema。
			 * @param[in] out output view。
			 * @param[out] status failure status。
			 * @retval true output capacity sufficient.
			 * @retval false short output、またはsize overflow。
			 * @pre `schema`はvalid、`out`はvalid view。
			 * @post 引数と外部状態の変更なし。
			 */
			template <typename T, std::size_t FieldCount>
			bool CheckOutputCapacity(const Schema<T, FieldCount>& schema,
									 ket::byte_view::MutableView out,
									 Status& status) noexcept
			{
				std::size_t offset = 0U;
				for (const auto& field : schema.Fields())
				{
					const auto field_size = field.EncodedSize();
					const auto available = ket::byte_view::Remaining(out, offset);
					const auto enough = field_size <= available;
					if (!enough)
					{
						status = MakeSizeStatus(Error::kShortOutput,
												offset,
												field.Name(),
												field.Group(),
												field_size,
												available);
						return false;
					}

					std::size_t next_offset = 0U;
					const auto added =
						ket::numeric::TryAdd<std::size_t>(offset, field_size, next_offset);
					if (!added)
					{
						status = MakeSizeStatus(Error::kSizeOverflow,
												offset,
												field.Name(),
												field.Group(),
												field_size,
												offset);
						return false;
					}

					offset = next_offset;
				}

				return true;
			}

			/**
			 * @brief measured encoded sizeがoutput容量に収まるか検査。
			 * @param[in] encoded_size 測定済みencoded size。
			 * @param[in] schema_name schema名。
			 * @param[in] out output view。
			 * @param[out] status failure status。
			 * @retval true output capacity sufficient.
			 * @retval false short output。
			 * @pre `out`はvalid view。
			 * @post 引数と外部状態の変更なし。
			 */
			inline bool CheckMeasuredOutputCapacity(std::size_t encoded_size,
													std::string_view schema_name,
													ket::byte_view::MutableView out,
													Status& status) noexcept
			{
				const auto enough_output = encoded_size <= out.Size();
				if (enough_output)
				{
					return true;
				}

				status = MakeSizeStatus(
					Error::kShortOutput, out.Size(), {}, schema_name, encoded_size, out.Size());
				return false;
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
				for (const auto& field : schema.Fields())
				{
					const auto field_start = measured;
					const auto measured_field =
						FieldAccess<T>::Measure(field)(value, field, measured, status);
					if (!measured_field)
					{
						return false;
					}

					const auto preflight_ok =
						FieldAccess<T>::Preflight(field)(value, field, status);
					if (!preflight_ok)
					{
						AddFieldStartToStatus(status, field_start);
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
				for (const auto& field : schema.Fields())
				{
					FieldAccess<T>::Encode(field)(value, out, offset, field);
				}
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename T>
		constexpr std::string_view BitMember<T>::Name() const noexcept
		{
			return name_;
		}

		template <typename T>
		constexpr unsigned BitMember<T>::Shift() const noexcept
		{
			return shift_;
		}

		template <typename T>
		constexpr unsigned BitMember<T>::Width() const noexcept
		{
			return width_;
		}

		template <typename T>
		constexpr std::uint64_t BitMember<T>::Expected() const noexcept
		{
			return expected_;
		}

		template <typename T>
		constexpr bool BitMember<T>::HasMember() const noexcept
		{
			return has_member_;
		}

		template <typename T>
		constexpr bool BitMember<T>::IsValid() const noexcept
		{
			return is_valid_;
		}

		template <typename T>
		constexpr std::string_view Field<T>::Name() const noexcept
		{
			return name_;
		}

		template <typename T>
		constexpr std::string_view Field<T>::Group() const noexcept
		{
			return group_;
		}

		template <typename T>
		constexpr FieldKind Field<T>::Kind() const noexcept
		{
			return kind_;
		}

		template <typename T>
		constexpr std::size_t Field<T>::EncodedSize() const noexcept
		{
			return encoded_size_;
		}

		template <typename T>
		constexpr std::size_t Field<T>::MaxEncodedSize() const noexcept
		{
			return max_encoded_size_;
		}

		template <typename T>
		constexpr bool Field<T>::IsFixedSize() const noexcept
		{
			return is_fixed_size_;
		}

		template <typename T>
		constexpr bool Field<T>::IsValid() const noexcept
		{
			return is_valid_;
		}

		template <typename T>
		constexpr std::uint64_t Field<T>::Expected() const noexcept
		{
			return expected_;
		}

		template <typename T>
		constexpr const std::uint8_t* Field<T>::ExpectedBytes() const noexcept
		{
			if (owns_expected_bytes_)
			{
				return owned_expected_bytes_.data();
			}

			return expected_bytes_;
		}

		template <typename T>
		constexpr std::size_t Field<T>::ExpectedBytesSize() const noexcept
		{
			return expected_bytes_size_;
		}

		template <typename T>
		constexpr const std::array<BitMember<T>, 16U>& Field<T>::BitMembers() const noexcept
		{
			return bit_members_;
		}

		template <typename T>
		constexpr const BitMember<T>* Field<T>::BitMemberAt(std::size_t index) const noexcept
		{
			if (index >= bit_member_count_)
			{
				return nullptr;
			}

			return &bit_members_[index];
		}

		template <typename T>
		constexpr std::size_t Field<T>::BitMemberCount() const noexcept
		{
			return bit_member_count_;
		}

		template <typename T, std::size_t FieldCount>
		constexpr std::string_view Schema<T, FieldCount>::Name() const noexcept
		{
			return name_;
		}

		template <typename T, std::size_t FieldCount>
		constexpr const std::array<Field<T>, FieldCount>&
		Schema<T, FieldCount>::Fields() const noexcept
		{
			return fields_;
		}

		template <typename T, std::size_t FieldCount>
		constexpr const Field<T>* Schema<T, FieldCount>::FieldAt(std::size_t index) const noexcept
		{
			if (index >= field_count_)
			{
				return nullptr;
			}

			return &fields_[index];
		}

		template <typename T, std::size_t FieldCount>
		constexpr std::size_t Schema<T, FieldCount>::FieldCountValue() const noexcept
		{
			return field_count_;
		}

		template <typename T, std::size_t FieldCount>
		constexpr bool Schema<T, FieldCount>::IsFixedSize() const noexcept
		{
			return is_fixed_size_;
		}

		template <typename T, std::size_t FieldCount>
		constexpr std::size_t Schema<T, FieldCount>::FixedSize() const noexcept
		{
			return fixed_size_;
		}

		template <typename T, std::size_t FieldCount>
		constexpr std::size_t Schema<T, FieldCount>::MaxSize() const noexcept
		{
			return max_size_;
		}

		template <typename T, std::size_t FieldCount>
		constexpr const Status& Schema<T, FieldCount>::SchemaStatus() const noexcept
		{
			return status_;
		}

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
			detail::SchemaAccess<T, FieldCount>::SetName(schema, name);
			detail::SchemaAccess<T, FieldCount>::SetFields(schema, fields);
			detail::SchemaAccess<T, FieldCount>::SetSchemaStatus(schema, detail::OkStatus());

			std::size_t fixed_size = 0U;
			std::size_t max_size = 0U;
			for (std::size_t index = 0U; index < FieldCount; ++index)
			{
				const auto& field = schema.Fields()[index];
				const auto field_is_valid = field.IsValid();
				const auto decode_is_valid = detail::FieldAccess<T>::Decode(field) != nullptr;
				const auto preflight_is_valid = detail::FieldAccess<T>::Preflight(field) != nullptr;
				const auto encode_is_valid = detail::FieldAccess<T>::Encode(field) != nullptr;
				const auto measure_is_valid = detail::FieldAccess<T>::Measure(field) != nullptr;
				const auto callbacks_are_valid =
					decode_is_valid && preflight_is_valid && encode_is_valid && measure_is_valid;
				if (!field_is_valid || !callbacks_are_valid)
				{
					const auto status =
						detail::MakeStatus(Error::kSchemaError, index, field.Name(), field.Group());
					detail::SchemaAccess<T, FieldCount>::SetSchemaStatus(schema, status);
					return schema;
				}

				const auto field_is_fixed_size = field.IsFixedSize();
				if (!field_is_fixed_size)
				{
					detail::SchemaAccess<T, FieldCount>::SetIsFixedSize(schema, false);
				}
				else
				{
					std::size_t next_size = 0U;
					const auto added = ket::numeric::TryAdd<std::size_t>(
						fixed_size, field.EncodedSize(), next_size);
					if (!added)
					{
						const auto status = detail::MakeSizeStatus(Error::kSizeOverflow,
																   fixed_size,
																   field.Name(),
																   field.Group(),
																   field.EncodedSize(),
																   fixed_size);
						detail::SchemaAccess<T, FieldCount>::SetSchemaStatus(schema, status);
						return schema;
					}

					fixed_size = next_size;
				}

				std::size_t next_max_size = 0U;
				const auto max_added = ket::numeric::TryAdd<std::size_t>(
					max_size, field.MaxEncodedSize(), next_max_size);
				if (!max_added)
				{
					const auto status = detail::MakeSizeStatus(Error::kSizeOverflow,
															   max_size,
															   field.Name(),
															   field.Group(),
															   field.MaxEncodedSize(),
															   max_size);
					detail::SchemaAccess<T, FieldCount>::SetSchemaStatus(schema, status);
					return schema;
				}

				max_size = next_max_size;
			}

			detail::SchemaAccess<T, FieldCount>::SetFixedSize(schema, fixed_size);
			detail::SchemaAccess<T, FieldCount>::SetMaxSize(schema, max_size);
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
													   schema.Name(),
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
			const auto input_valid = detail::IsValidInput(data);
			if (!input_valid)
			{
				result.status = detail::MakeSizeStatus(
					Error::kInvalidInputView, 0U, {}, schema.Name(), 0U, data.Size());
				return result;
			}

			const auto schema_valid = detail::ValidateSchema(schema, result.status);
			if (!schema_valid)
			{
				return result;
			}

			T decoded{};
			std::size_t offset = 0U; // NOLINT(misc-const-correctness)
			for (const auto& field : schema.Fields())
			{
				const auto decoded_field = detail::FieldAccess<T>::Decode(field)(
					data, offset, decoded, field, result.status);
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

			auto builder = ket::bytes::Builder(encoded_size);
			builder.AppendFill(std::uint8_t{0U}, encoded_size);
			auto bytes = std::move(builder).Build();
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
			const auto output_valid = detail::IsValidOutput(out);
			if (!output_valid)
			{
				result.status = detail::MakeSizeStatus(
					Error::kInvalidOutputView, 0U, {}, schema.Name(), 0U, out.Size());
				return result;
			}

			const auto schema_valid = detail::ValidateSchema(schema, result.status);
			if (!schema_valid)
			{
				return result;
			}

			const auto fixed_size_schema = schema.IsFixedSize();
			if (fixed_size_schema)
			{
				const auto fixed_capacity_ok =
					detail::CheckOutputCapacity(schema, out, result.status);
				if (!fixed_capacity_ok)
				{
					return result;
				}
			}

			std::size_t encoded_size = 0U;
			const auto measured =
				detail::MeasureAndPreflight(value, schema, encoded_size, result.status);
			if (!measured)
			{
				return result;
			}

			const auto enough_output = detail::CheckMeasuredOutputCapacity(
				encoded_size, schema.Name(), out, result.status);
			if (!enough_output)
			{
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
			const auto schema_ok = schema.SchemaStatus().Ok();
			if (!schema_ok)
			{
				return std::nullopt;
			}

			const auto schema_is_fixed_size = schema.IsFixedSize();
			if (!schema_is_fixed_size)
			{
				return std::nullopt;
			}

			return schema.FixedSize();
		}

		template <typename T, std::size_t FieldCount>
		std::optional<std::size_t> MaxEncodedSize(const Schema<T, FieldCount>& schema) noexcept
		{
			const auto schema_ok = schema.SchemaStatus().Ok();
			if (!schema_ok)
			{
				return std::nullopt;
			}

			return schema.MaxSize();
		}

		template <typename T, std::size_t FieldCount>
		bool IsFixedSize(const Schema<T, FieldCount>& schema) noexcept
		{
			const auto schema_ok = schema.SchemaStatus().Ok();
			return schema_ok && schema.IsFixedSize();
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
			return detail::
				MakeIntegerField<T, Member, std::uint8_t, std::uint8_t, detail::ByteOrder::kSingle>(
					name);
		}

		template <typename T, auto Member>
		Field<T> U16Be(std::string_view name) noexcept
		{
			return detail::
				MakeIntegerField<T, Member, std::uint16_t, std::uint16_t, detail::ByteOrder::kBig>(
					name);
		}

		template <typename T, auto Member>
		Field<T> U16Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T,
											Member,
											std::uint16_t,
											std::uint16_t,
											detail::ByteOrder::kLittle>(name);
		}

		template <typename T, auto Member>
		Field<T> U32Be(std::string_view name) noexcept
		{
			return detail::
				MakeIntegerField<T, Member, std::uint32_t, std::uint32_t, detail::ByteOrder::kBig>(
					name);
		}

		template <typename T, auto Member>
		Field<T> U32Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T,
											Member,
											std::uint32_t,
											std::uint32_t,
											detail::ByteOrder::kLittle>(name);
		}

		template <typename T, auto Member>
		Field<T> U64Be(std::string_view name) noexcept
		{
			return detail::
				MakeIntegerField<T, Member, std::uint64_t, std::uint64_t, detail::ByteOrder::kBig>(
					name);
		}

		template <typename T, auto Member>
		Field<T> U64Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T,
											Member,
											std::uint64_t,
											std::uint64_t,
											detail::ByteOrder::kLittle>(name);
		}

		template <typename T, auto Member>
		Field<T> I8(std::string_view name) noexcept
		{
			return detail::
				MakeIntegerField<T, Member, std::uint8_t, std::int8_t, detail::ByteOrder::kSingle>(
					name);
		}

		template <typename T, auto Member>
		Field<T> I16Be(std::string_view name) noexcept
		{
			return detail::
				MakeIntegerField<T, Member, std::uint16_t, std::int16_t, detail::ByteOrder::kBig>(
					name);
		}

		template <typename T, auto Member>
		Field<T> I16Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T,
											Member,
											std::uint16_t,
											std::int16_t,
											detail::ByteOrder::kLittle>(name);
		}

		template <typename T, auto Member>
		Field<T> I32Be(std::string_view name) noexcept
		{
			return detail::
				MakeIntegerField<T, Member, std::uint32_t, std::int32_t, detail::ByteOrder::kBig>(
					name);
		}

		template <typename T, auto Member>
		Field<T> I32Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T,
											Member,
											std::uint32_t,
											std::int32_t,
											detail::ByteOrder::kLittle>(name);
		}

		template <typename T, auto Member>
		Field<T> I64Be(std::string_view name) noexcept
		{
			return detail::
				MakeIntegerField<T, Member, std::uint64_t, std::int64_t, detail::ByteOrder::kBig>(
					name);
		}

		template <typename T, auto Member>
		Field<T> I64Le(std::string_view name) noexcept
		{
			return detail::MakeIntegerField<T,
											Member,
											std::uint64_t,
											std::int64_t,
											detail::ByteOrder::kLittle>(name);
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
			detail::FieldAccess<T>::SetName(field, name);
			detail::FieldAccess<T>::SetKind(field, FieldKind::kViewBytes);
			detail::FieldAccess<T>::SetEncodedSize(field, size);
			detail::FieldAccess<T>::SetMaxEncodedSize(field, size);
			detail::FieldAccess<T>::SetIsValid(field, true);
			detail::FieldAccess<T>::SetDecode(field, &detail::DecodeViewBytes<T, Member>);
			detail::FieldAccess<T>::SetPreflight(field, &detail::PreflightViewBytes<T, Member>);
			detail::FieldAccess<T>::SetEncode(field, &detail::EncodeViewBytes<T, Member>);
			detail::FieldAccess<T>::SetMeasure(field, &detail::MeasureFixed<T>);
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
			detail::FieldAccess<T>::SetName(field, name);
			detail::FieldAccess<T>::SetKind(field, FieldKind::kConstBytes);
			detail::FieldAccess<T>::SetEncodedSize(field, size);
			detail::FieldAccess<T>::SetMaxEncodedSize(field, size);
			const auto expected_view = ket::byte_view::View(expected, size);
			const auto expected_is_valid = ket::byte_view::IsValid(expected_view);
			detail::FieldAccess<T>::SetIsValid(field, expected_is_valid);
			detail::FieldAccess<T>::SetExpectedBytes(field, expected);
			detail::FieldAccess<T>::SetExpectedBytesSize(field, size);
			detail::FieldAccess<T>::SetDecode(field, &detail::DecodeExpectedBytes<T>);
			detail::FieldAccess<T>::SetPreflight(field, &detail::PreflightNoop<T>);
			detail::FieldAccess<T>::SetEncode(field, &detail::EncodeExpectedBytes<T>);
			detail::FieldAccess<T>::SetMeasure(field, &detail::MeasureFixed<T>);
			return field;
		}

		template <typename T, std::size_t Size>
		Field<T> ConstBytes(std::string_view name,
							const std::array<std::uint8_t, Size>& expected) noexcept
		{
			static_assert(Size <= 32U, "owned ConstBytes supports up to 32 bytes");

			Field<T> field{};
			detail::FieldAccess<T>::SetName(field, name);
			detail::FieldAccess<T>::SetKind(field, FieldKind::kConstBytes);
			detail::FieldAccess<T>::SetEncodedSize(field, Size);
			detail::FieldAccess<T>::SetMaxEncodedSize(field, Size);
			detail::FieldAccess<T>::SetIsValid(field, true);
			detail::FieldAccess<T>::SetOwnedExpectedBytes(field, expected.data(), Size);
			detail::FieldAccess<T>::SetDecode(field, &detail::DecodeExpectedBytes<T>);
			detail::FieldAccess<T>::SetPreflight(field, &detail::PreflightNoop<T>);
			detail::FieldAccess<T>::SetEncode(field, &detail::EncodeExpectedBytes<T>);
			detail::FieldAccess<T>::SetMeasure(field, &detail::MeasureFixed<T>);
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
			static_assert(Width <= ket::bits::TypeBitWidth<MemberType>(),
						  "bit width must fit member type");

			BitMember<T> member{};
			detail::BitMemberAccess<T>::SetName(member, name);
			detail::BitMemberAccess<T>::SetShift(member, Shift);
			detail::BitMemberAccess<T>::SetWidth(member, Width);
			detail::BitMemberAccess<T>::SetHasMember(member, true);
			detail::BitMemberAccess<T>::SetIsValid(
				member, Width > 0U && Width <= ket::bits::TypeBitWidth<MemberType>());
			detail::BitMemberAccess<T>::SetGet(member, &detail::GetBitMember<T, Member>);
			detail::BitMemberAccess<T>::SetSetter(member, &detail::SetBitMember<T, Member>);
			return member;
		}

		template <typename T, unsigned Shift, unsigned Width>
		BitMember<T> ReservedBits(std::string_view name, std::uint64_t expected) noexcept
		{
			BitMember<T> member{};
			detail::BitMemberAccess<T>::SetName(member, name);
			detail::BitMemberAccess<T>::SetShift(member, Shift);
			detail::BitMemberAccess<T>::SetWidth(member, Width);
			detail::BitMemberAccess<T>::SetExpected(member, expected);
			detail::BitMemberAccess<T>::SetHasMember(member, false);
			std::uint64_t mask = 0U;
			const auto mask_ok = detail::TryBitMask(Width, mask);
			detail::BitMemberAccess<T>::SetIsValid(member,
												   Width > 0U && mask_ok && expected <= mask);
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
			detail::FieldAccess<T>::SetName(field, name);
			detail::FieldAccess<T>::SetKind(field, FieldKind::kValidation);
			detail::FieldAccess<T>::SetIsValid(field, callback != nullptr);
			detail::FieldAccess<T>::SetValidation(field, callback);
			detail::FieldAccess<T>::SetDecode(field, &detail::DecodeValidation<T>);
			detail::FieldAccess<T>::SetPreflight(field, &detail::PreflightValidation<T>);
			detail::FieldAccess<T>::SetEncode(field, &detail::EncodeValidation<T>);
			detail::FieldAccess<T>::SetMeasure(field, &detail::MeasureValidation<T>);
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
