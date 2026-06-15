#pragma once

/**
 * @file ket_optional.h
 * @brief std::optionalの小さい合成API。
 *
 * @details `std::optional`の値変換、optionalを返す合成、遅延fallback評価を短いAPIへ集約する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。標準の値保持制約に従い、
 * 参照保持が必要な場合は`std::reference_wrapper`を明示的に返す。
 *
 * @par プロジェクトへの適用方法
 * `ket_optional.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::optional`の小さい合成を、標準の値保持制約に従って扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：API別。
 * 非推奨理由：APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::optional
 * 内部実装：ket::optional::detail
 */

#include <optional>
#include <type_traits>
#include <utility>

namespace ket
{
	namespace optional
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief optional保持値の変換。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 変換対象のoptional。
		 * @param[in] f 保持値を変換するmapper。
		 * @retval value `value`が値を持つ場合、`f(*value)`を`std::decay_t`で保持するoptional。
		 * @retval std::nullopt `value`が空の場合。
		 * @pre `f(*value)`はvoidではないobjectを値として返す。直接の参照戻り値は不可。
		 * @post `value`と外部状態の変更なし。空の場合は`f`を呼ばない。
		 * @note 参照を保持したい場合は、mapperが`std::reference_wrapper<T>`を明示的に返す。
		 * @note C++23以降では`std::optional::transform`が標準代替。
		 * @code
		 * const auto value = ket::optional::Map(std::optional<int>(2), [](int x) { return x * 3;
		 * });
		 * // value == std::optional<int>(6)
		 * @endcode
		 */
		template <typename T, typename F>
		auto Map(const std::optional<T>& value,
				 F f) -> std::optional<std::decay_t<decltype(f(*value))>>;

		/**
		 * @brief optional保持値のmove変換。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 変換対象のoptional。値を持つ場合は保持値をmapperへmove可能。
		 * @param[in] f 保持値を変換するmapper。
		 * @retval value
		 * `value`が値を持つ場合、`f(std::move(*value))`を`std::decay_t`で保持するoptional。
		 * @retval std::nullopt `value`が空の場合。
		 * @pre `f(std::move(*value))`はvoidではないobjectを値として返す。直接の参照戻り値は不可。
		 * @post 空の場合は`f`を呼ばない。値ありの場合は`value`の保持値がmove元になり得る。
		 * @note 参照を保持したい場合は、mapperが`std::reference_wrapper<T>`を明示的に返す。
		 * @note C++23以降では`std::optional::transform`が標準代替。
		 * @code
		 * auto text = ket::optional::Map(std::optional<std::string>("id"), [](std::string x) {
		 *     return x.size();
		 * });
		 * // text == std::optional<std::size_t>(2)
		 * @endcode
		 */
		template <typename T, typename F>
		auto Map(std::optional<T>&& value,
				 F f) -> std::optional<std::decay_t<decltype(f(std::move(*value)))>>;

		/**
		 * @brief optionalを返すmapperによるoptional合成。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 合成対象のoptional。
		 * @param[in] f 保持値からoptionalを返すmapper。
		 * @retval value `value`が値を持つ場合、`f(*value)`が返したoptional。
		 * @retval empty optional `value`が空の場合の空optional。
		 * @pre `f(*value)`は`std::optional<U>`を値として返す。
		 * @post `value`と外部状態の変更なし。空の場合は`f`を呼ばない。
		 * @note 戻り値はunwrapせず、mapperが返したoptional型をそのまま返す。
		 * @note C++23以降では`std::optional::and_then`が標準代替。
		 * @code
		 * const auto value = ket::optional::AndThen(std::optional<int>(2), [](int x) {
		 *     return std::optional<int>(x * 3);
		 * });
		 * // value == std::optional<int>(6)
		 * @endcode
		 */
		template <typename T, typename F>
		auto AndThen(const std::optional<T>& value, F f) -> decltype(f(*value));

		/**
		 * @brief optionalを返すmapperによるoptionalのmove合成。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 合成対象のoptional。値を持つ場合は保持値をmapperへmove可能。
		 * @param[in] f 保持値からoptionalを返すmapper。
		 * @retval value `value`が値を持つ場合、`f(std::move(*value))`が返したoptional。
		 * @retval empty optional `value`が空の場合の空optional。
		 * @pre `f(std::move(*value))`は`std::optional<U>`を値として返す。
		 * @post 空の場合は`f`を呼ばない。値ありの場合は`value`の保持値がmove元になり得る。
		 * @note 戻り値はunwrapせず、mapperが返したoptional型をそのまま返す。
		 * @note C++23以降では`std::optional::and_then`が標準代替。
		 * @code
		 * auto value = ket::optional::AndThen(std::optional<std::string>("id"), [](std::string x) {
		 *     return std::optional<std::size_t>(x.size());
		 * });
		 * // value == std::optional<std::size_t>(2)
		 * @endcode
		 */
		template <typename T, typename F>
		auto AndThen(std::optional<T>&& value, F f) -> decltype(f(std::move(*value)));

		/**
		 * @brief optional保持値、または遅延評価fallbackの値取得。
		 * @tparam T optionalの保持型。
		 * @tparam F fallback factoryの型。
		 * @param[in] value 値取得対象のoptional。
		 * @param[in] fallback_factory 空の場合だけ評価するfallback factory。
		 * @retval value
		 * `value`が値を持つ場合はその保持値のcopy、空の場合は`fallback_factory()`の結果。
		 * @pre Tはcopy constructible。`fallback_factory()`はTへ変換可能な値を返す。
		 * @post `value`と外部状態の変更なし。値ありの場合は`fallback_factory`を呼ばない。
		 * @note `std::optional::value_or`と異なり、fallbackは必要時だけ評価。
		 * @code
		 * const auto value = ket::optional::ValueOrEval(std::optional<int>(), [] { return 42; });
		 * // value == 42
		 * @endcode
		 */
		template <typename T, typename F>
		T ValueOrEval(const std::optional<T>& value, F fallback_factory);

		/**
		 * @brief optional保持値のmove、または遅延評価fallbackの値取得。
		 * @tparam T optionalの保持型。
		 * @tparam F fallback factoryの型。
		 * @param[in] value 値取得対象のoptional。値を持つ場合は保持値を戻り値へmove可能。
		 * @param[in] fallback_factory 空の場合だけ評価するfallback factory。
		 * @retval value
		 * `value`が値を持つ場合はその保持値のmove、空の場合は`fallback_factory()`の結果。
		 * @pre Tはmove constructible。`fallback_factory()`はTへ変換可能な値を返す。
		 * @post
		 * 値ありの場合は`value`の保持値がmove元になり得る。値ありの場合は`fallback_factory`を呼ばない。
		 * @note `std::optional::value_or`と異なり、fallbackは必要時だけ評価。
		 * @code
		 * auto value = ket::optional::ValueOrEval(std::optional<std::string>(), [] {
		 *     return std::string("fallback");
		 * });
		 * // value == "fallback"
		 * @endcode
		 */
		template <typename T, typename F>
		T ValueOrEval(std::optional<T>&& value, F fallback_factory);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief std::optional型の判定。
			 * @tparam Value 判定対象の型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Value>
			struct IsOptional : std::false_type
			{
			};

			/**
			 * @brief std::optional型の判定specialization。
			 * @tparam Value optionalの保持型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Value>
			struct IsOptional<std::optional<Value>> : std::true_type
			{
			};

			/**
			 * @brief Mapのmapper戻り型として直接保持可能な型の判定。
			 * @tparam Value 判定対象の型。
			 * @note direct referenceとvoidを拒否し、`std::decay_t`後のobject保持だけを許容。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Value>
			struct IsValidMapResult
				: std::integral_constant<bool,
										 !std::is_void_v<Value> && !std::is_reference_v<Value> &&
											 std::is_object_v<std::decay_t<Value>>>
			{
			};

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename T, typename F>
		auto Map(const std::optional<T>& value,
				 F f) -> std::optional<std::decay_t<decltype(f(*value))>>
		{
			using mapper_result_type = decltype(f(*value));
			using optional_result_type = std::optional<std::decay_t<mapper_result_type>>;
			static_assert(detail::IsValidMapResult<mapper_result_type>::value,
						  "ket::optional::Map mapper must return a non-void object by value.");

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return std::nullopt;
			}

			return optional_result_type(f(*value));
		}

		template <typename T, typename F>
		auto Map(std::optional<T>&& value,
				 F f) -> std::optional<std::decay_t<decltype(f(std::move(*value)))>>
		{
			using mapper_result_type = decltype(f(std::move(*value)));
			using optional_result_type = std::optional<std::decay_t<mapper_result_type>>;
			static_assert(detail::IsValidMapResult<mapper_result_type>::value,
						  "ket::optional::Map mapper must return a non-void object by value.");

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return std::nullopt;
			}

			return optional_result_type(f(std::move(*value)));
		}

		template <typename T, typename F>
		auto AndThen(const std::optional<T>& value, F f) -> decltype(f(*value))
		{
			using result_type = decltype(f(*value));
			static_assert(detail::IsOptional<result_type>::value,
						  "ket::optional::AndThen mapper must return std::optional<U>.");

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return result_type{};
			}

			return f(*value);
		}

		template <typename T, typename F>
		auto AndThen(std::optional<T>&& value, F f) -> decltype(f(std::move(*value)))
		{
			using result_type = decltype(f(std::move(*value)));
			static_assert(detail::IsOptional<result_type>::value,
						  "ket::optional::AndThen mapper must return std::optional<U>.");

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return result_type{};
			}

			return f(std::move(*value));
		}

		template <typename T, typename F>
		T ValueOrEval(const std::optional<T>& value, F fallback_factory)
		{
			static_assert(std::is_copy_constructible_v<T>,
						  "ket::optional::ValueOrEval(const optional<T>&) requires copy "
						  "constructible T.");

			const auto value_has_value = value.has_value();
			if (value_has_value)
			{
				return *value;
			}

			return fallback_factory();
		}

		template <typename T, typename F>
		T ValueOrEval(std::optional<T>&& value, F fallback_factory)
		{
			static_assert(std::is_move_constructible_v<T>,
						  "ket::optional::ValueOrEval(optional<T>&&) requires move "
						  "constructible T.");

			const auto value_has_value = value.has_value();
			if (value_has_value)
			{
				return std::move(*value);
			}

			return fallback_factory();
		}

	} // namespace optional

} // namespace ket
