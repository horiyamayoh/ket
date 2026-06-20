#pragma once

/**
 * @file ket_enums.h
 * @brief enum型のtable-based変換API。
 *
 * @details 利用者が明示したtableを正として、enum型の名前変換、文字列parse、
 * underlying値取得、flags操作を短いAPIへ集約する。reflectionや登録frameworkは持たず、
 * drop-in時はヘッダ単体で持ち出す。flags操作はunsigned underlying typeのbit mask
 * enumを対象にする。
 *
 * @par プロジェクトへの適用方法
 * `ket_enums.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::string_view` と class template argument deduction を利用でき、
 * table-based変換を標準ライブラリのみで短く書ける。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::enums
 * 内部実装：ket::enums::detail
 */

#include <cstddef>
#include <optional>
#include <string_view>
#include <type_traits>

namespace ket
{
	namespace enums
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief enum値のunderlying値取得。
		 * @tparam E 変換対象のenum型。
		 * @param[in] value 変換対象のenum値。
		 * @retval value `value`をunderlying typeへ変換した値。
		 * @pre `E`はenum型。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * enum class Mode : unsigned { kRun = 1U };
		 * const auto value = ket::enums::ToUnderlying(Mode::kRun);
		 * // value == 1
		 * @endcode
		 */
		template <typename E>
		constexpr std::underlying_type_t<E> ToUnderlying(E value) noexcept;

		/**
		 * @brief enum値と名前の対応表entry。
		 * @tparam E 対象enum型。
		 */
		template <typename E>
		struct Entry
		{
			static_assert(std::is_enum_v<E>, "ket::enums::Entry requires an enum type.");

			E value;
			std::string_view name;
		};

		template <typename E>
		Entry(E value, std::string_view name) -> Entry<E>;

		/**
		 * @brief enum値に対応する名前取得。
		 * @tparam E 対象enum型。
		 * @tparam N table entry数。
		 * @param[in] value 検索対象のenum値。
		 * @param[in] table 検索に使う対応表。
		 * @retval value `value`と最初に一致したentryの名前。
		 * @retval std::nullopt 一致するentryなし。
		 * @pre `table`の各`name`は参照中に有効な文字列を指す。
		 * @post 引数と外部状態の変更なし。重複valueでは先頭entryを返す。
		 * @code
		 * const ket::enums::Entry<Mode> table[] = {ket::enums::Entry{Mode::kRun, "run"}};
		 * const auto name = ket::enums::Name(Mode::kRun, table);
		 * // name == std::optional<std::string_view>("run")
		 * @endcode
		 */
		template <typename E, std::size_t N>
		constexpr std::optional<std::string_view> Name(E value,
													   const Entry<E> (&table)[N]) noexcept;

		/**
		 * @brief enum値に対応する名前またはfallback取得。
		 * @tparam E 対象enum型。
		 * @tparam N table entry数。
		 * @param[in] value 検索対象のenum値。
		 * @param[in] table 検索に使う対応表。
		 * @param[in] fallback 一致するentryがない場合に返す名前。
		 * @retval value `value`と最初に一致したentryの名前。
		 * @retval fallback 一致するentryなし。
		 * @pre `table`の各`name`と`fallback`は戻り値の参照中に有効な文字列を指す。
		 * @post 引数と外部状態の変更なし。重複valueでは先頭entryを返す。
		 * @code
		 * enum class Mode { kRun, kUnknown };
		 * const ket::enums::Entry<Mode> table[] = {ket::enums::Entry{Mode::kRun, "run"}};
		 * const auto name = ket::enums::NameOr(Mode::kUnknown, table, "unknown");
		 * // name == "unknown"
		 * @endcode
		 */
		template <typename E, std::size_t N>
		constexpr std::string_view
		NameOr(E value, const Entry<E> (&table)[N], std::string_view fallback) noexcept;

		/**
		 * @brief 名前に対応するenum値取得。
		 * @tparam E 対象enum型。
		 * @tparam N table entry数。
		 * @param[in] text 検索対象の名前。
		 * @param[in] table 検索に使う対応表。
		 * @retval value `text`と最初に完全一致したentryのenum値。
		 * @retval std::nullopt 完全一致するentryなし。
		 * @pre `text`と`table`の各`name`は参照中に有効な文字列を指す。
		 * @post 引数と外部状態の変更なし。重複nameでは先頭entryを返す。
		 * @note 比較はcase-sensitiveかつ完全一致。
		 * @code
		 * const ket::enums::Entry<Mode> table[] = {ket::enums::Entry{Mode::kRun, "run"}};
		 * const auto value = ket::enums::Parse<Mode>("run", table);
		 * // value == std::optional<Mode>(Mode::kRun)
		 * @endcode
		 */
		template <typename E, std::size_t N>
		constexpr std::optional<E> Parse(std::string_view text,
										 const Entry<E> (&table)[N]) noexcept;

		/**
		 * @brief enum値がtableに存在するか判定。
		 * @tparam E 対象enum型。
		 * @tparam N table entry数。
		 * @param[in] value 判定対象のenum値。
		 * @param[in] table 判定に使う対応表。
		 * @retval true `value`と一致するentryあり。
		 * @retval false `value`と一致するentryなし。
		 * @pre `table`は`N`個のentryを参照できる配列。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * enum class Mode { kRun };
		 * const ket::enums::Entry<Mode> table[] = {ket::enums::Entry{Mode::kRun, "run"}};
		 * const auto valid = ket::enums::IsValid(Mode::kRun, table);
		 * // valid == true
		 * @endcode
		 */
		template <typename E, std::size_t N>
		constexpr bool IsValid(E value, const Entry<E> (&table)[N]) noexcept;

		/**
		 * @brief 指定flagがすべて立っているか判定。
		 * @tparam E 対象enum型。
		 * @param[in] flags 判定対象のflag集合。
		 * @param[in] flag 判定するflag。
		 * @retval true `flag`の全bitが`flags`に含まれる。
		 * @retval false `flag`の少なくとも1bitが`flags`に含まれない。
		 * @pre `E`はunsigned underlying typeを持つbit mask enum型。
		 * @post 引数と外部状態の変更なし。`flag`が0の場合はtrueを返す。
		 * @note 0 maskは全bitが含まれる状態として扱う。
		 * @code
		 * enum class Permission : unsigned { kRead = 1U, kWrite = 2U, kReadWrite = 3U };
		 * const auto has_write = ket::enums::HasFlag(Permission::kReadWrite, Permission::kWrite);
		 * // has_write == true
		 * @endcode
		 */
		template <typename E>
		constexpr bool HasFlag(E flags, E flag) noexcept;

		/**
		 * @brief 指定flagを立てたflag集合取得。
		 * @tparam E 対象enum型。
		 * @param[in] flags 入力flag集合。
		 * @param[in] flag 立てるflag。
		 * @retval value `flags`に`flag`のbitを加えた値。
		 * @pre `E`はunsigned underlying typeを持つbit mask enum型。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * enum class Permission : unsigned { kRead = 1U, kWrite = 2U, kReadWrite = 3U };
		 * const auto flags = ket::enums::SetFlag(Permission::kRead, Permission::kWrite);
		 * // flags == Permission::kReadWrite
		 * @endcode
		 */
		template <typename E>
		constexpr E SetFlag(E flags, E flag) noexcept;

		/**
		 * @brief 指定flagを下ろしたflag集合取得。
		 * @tparam E 対象enum型。
		 * @param[in] flags 入力flag集合。
		 * @param[in] flag 下ろすflag。
		 * @retval value `flags`から`flag`のbitを除いた値。
		 * @pre `E`はunsigned underlying typeを持つbit mask enum型。
		 * @post 引数と外部状態の変更なし。
		 * @code
		 * enum class Permission : unsigned { kRead = 1U, kWrite = 2U, kReadWrite = 3U };
		 * const auto flags = ket::enums::ClearFlag(Permission::kReadWrite, Permission::kRead);
		 * // flags == Permission::kWrite
		 * @endcode
		 */
		template <typename E>
		constexpr E ClearFlag(E flags, E flag) noexcept;

		/**
		 * @brief mask内のいずれかのflagが立っているか判定。
		 * @tparam E 対象enum型。
		 * @param[in] flags 判定対象のflag集合。
		 * @param[in] mask 判定するflag mask。
		 * @retval true `mask`の少なくとも1bitが`flags`に含まれる。
		 * @retval false `mask`のbitが`flags`に含まれない。
		 * @pre `E`はunsigned underlying typeを持つbit mask enum型。
		 * @post 引数と外部状態の変更なし。`mask`が0の場合はfalseを返す。
		 * @note 0 maskは「いずれかのflag」の対象bitを持たない状態として扱う。
		 * @code
		 * enum class Permission : unsigned { kRead = 1U, kWrite = 2U, kReadWrite = 3U };
		 * const auto any = ket::enums::HasAnyFlag(Permission::kReadWrite, Permission::kWrite);
		 * // any == true
		 * @endcode
		 */
		template <typename E>
		constexpr bool HasAnyFlag(E flags, E mask) noexcept;

		/**
		 * @brief mask内のすべてのflagが立っているか判定。
		 * @tparam E 対象enum型。
		 * @param[in] flags 判定対象のflag集合。
		 * @param[in] mask 判定するflag mask。
		 * @retval true `mask`の全bitが`flags`に含まれる。
		 * @retval false `mask`の少なくとも1bitが`flags`に含まれない。
		 * @pre `E`はunsigned underlying typeを持つbit mask enum型。
		 * @post 引数と外部状態の変更なし。`mask`が0の場合はtrueを返す。
		 * @note 0 maskは全bitが含まれる状態として扱う。
		 * @code
		 * enum class Permission : unsigned { kRead = 1U, kWrite = 2U, kReadWrite = 3U };
		 * const auto all = ket::enums::HasAllFlags(Permission::kReadWrite, Permission::kReadWrite);
		 * // all == true
		 * @endcode
		 */
		template <typename E>
		constexpr bool HasAllFlags(E flags, E mask) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief enum型のunderlying type取得。
			 * @tparam E 対象enum型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename E>
			struct EnumTraits
			{
				static_assert(std::is_enum_v<E>, "ket::enums requires an enum type.");

				using Underlying = std::underlying_type_t<E>;
			};

			/**
			 * @brief flag操作対象enum型の制約確認。
			 * @tparam E 対象enum型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename E>
			struct FlagTraits
			{
				using Underlying = typename EnumTraits<E>::Underlying;

				static_assert(std::is_unsigned_v<Underlying>,
							  "ket::enums flag helpers require an unsigned underlying type.");
			};

			/**
			 * @brief enum値のunderlying値取得。
			 * @tparam E 変換対象のenum型。
			 * @param[in] value 変換対象のenum値。
			 * @retval value `value`をunderlying typeへ変換した値。
			 * @pre `E`はenum型。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename E>
			constexpr typename EnumTraits<E>::Underlying ToUnderlyingUnchecked(E value) noexcept
			{
				return static_cast<typename EnumTraits<E>::Underlying>(value);
			}

			/**
			 * @brief flag操作用のunderlying値取得。
			 * @tparam E 対象enum型。
			 * @param[in] value 変換対象のenum値。
			 * @retval value `value`をunsigned underlying typeへ変換した値。
			 * @pre `E`はunsigned underlying typeを持つbit mask enum型。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename E>
			constexpr typename FlagTraits<E>::Underlying ToFlagBits(E value) noexcept
			{
				return static_cast<typename FlagTraits<E>::Underlying>(value);
			}

			/**
			 * @brief mask内のすべてのflagが立っているか判定。
			 * @tparam E 対象enum型。
			 * @param[in] flags 判定対象のflag集合。
			 * @param[in] mask 判定するflag mask。
			 * @retval true `mask`の全bitが`flags`に含まれる。
			 * @retval false `mask`の少なくとも1bitが`flags`に含まれない。
			 * @pre `E`はunsigned underlying typeを持つbit mask enum型。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename E>
			constexpr bool ContainsAllFlagBits(E flags, E mask) noexcept
			{
				const auto flags_value = ToFlagBits(flags);
				const auto mask_value = ToFlagBits(mask);

				return (flags_value & mask_value) == mask_value;
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename E>
		constexpr std::underlying_type_t<E> ToUnderlying(E value) noexcept
		{
			return detail::ToUnderlyingUnchecked(value);
		}

		template <typename E, std::size_t N>
		constexpr std::optional<std::string_view> Name(E value, const Entry<E> (&table)[N]) noexcept
		{
			for (const auto& entry : table)
			{
				const auto value_matches = entry.value == value;
				if (value_matches)
				{
					return entry.name;
				}
			}

			return std::nullopt;
		}

		template <typename E, std::size_t N>
		constexpr std::string_view
		NameOr(E value, const Entry<E> (&table)[N], std::string_view fallback) noexcept
		{
			const auto name = Name(value, table);
			const auto name_has_value = name.has_value();
			if (name_has_value)
			{
				return *name;
			}

			return fallback;
		}

		template <typename E, std::size_t N>
		constexpr std::optional<E> Parse(std::string_view text, const Entry<E> (&table)[N]) noexcept
		{
			for (const auto& entry : table)
			{
				const auto text_matches = entry.name == text;
				if (text_matches)
				{
					return entry.value;
				}
			}

			return std::nullopt;
		}

		template <typename E, std::size_t N>
		constexpr bool IsValid(E value, const Entry<E> (&table)[N]) noexcept
		{
			const auto name = Name(value, table);
			return name.has_value();
		}

		template <typename E>
		constexpr bool HasFlag(E flags, E flag) noexcept
		{
			return detail::ContainsAllFlagBits(flags, flag);
		}

		template <typename E>
		constexpr E SetFlag(E flags, E flag) noexcept
		{
			const auto flags_value = detail::ToFlagBits(flags);
			const auto flag_value = detail::ToFlagBits(flag);

			return static_cast<E>(flags_value | flag_value);
		}

		template <typename E>
		constexpr E ClearFlag(E flags, E flag) noexcept
		{
			const auto flags_value = detail::ToFlagBits(flags);
			const auto flag_value = detail::ToFlagBits(flag);

			return static_cast<E>(flags_value & ~flag_value);
		}

		template <typename E>
		constexpr bool HasAnyFlag(E flags, E mask) noexcept
		{
			const auto flags_value = detail::ToFlagBits(flags);
			const auto mask_value = detail::ToFlagBits(mask);

			return (flags_value & mask_value) != 0;
		}

		template <typename E>
		constexpr bool HasAllFlags(E flags, E mask) noexcept
		{
			return detail::ContainsAllFlagBits(flags, mask);
		}

	} // namespace enums

} // namespace ket
