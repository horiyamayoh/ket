#pragma once

/**
 * @file ket_optional.h
 * @brief std::optionalの小さい合成API。
 *
 * @details `std::optional`の値変換、optionalを返す合成、遅延fallback評価を短いAPIへ集約する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。標準の値保持制約に従い、
 * 参照保持が必要な場合は`std::reference_wrapper`を明示的に返す。mapperとfactoryは
 * `std::invoke`互換のcallableとして扱う。
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

#include <functional>
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
		 * @brief mutable optional保持値の変換。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in,out] value 変換対象のoptional。値を持つ場合はmapperへmutable参照を渡す。
		 * @param[in] f 保持値を変換する`std::invoke`互換mapper。
		 * @retval value `value`が値を持つ場合、`std::invoke(f,
		 * *value)`を`std::decay_t`で保持するoptional。
		 * @retval std::nullopt `value`が空の場合。
		 * @pre `std::invoke(f, *value)`はvoidではないobjectを値として返す。直接の参照戻り値は不可。
		 * @post 空の場合は`f`を呼ばない。値ありの場合、mapperは`T&`を通じて保持値を変更できる。
		 * @note 参照を保持したい場合は、mapperが`std::reference_wrapper<T>`を明示的に返す。
		 * @note mapperの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @note C++23以降では`std::optional::transform`が標準代替。
		 * @code
		 * auto source = std::optional<int>(2);
		 * const auto value = ket::optional::Map(source, [](int& x) {
		 *     x += 1;
		 *     return x * 3;
		 * });
		 * // source == std::optional<int>(3), value == std::optional<int>(9)
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr auto Map(std::optional<T>& value, F&& f);

		/**
		 * @brief const optional保持値の変換。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 変換対象のoptional。
		 * @param[in] f 保持値を変換する`std::invoke`互換mapper。
		 * @retval value `value`が値を持つ場合、`std::invoke(f,
		 * *value)`を`std::decay_t`で保持するoptional。
		 * @retval std::nullopt `value`が空の場合。
		 * @pre `std::invoke(f, *value)`はvoidではないobjectを値として返す。直接の参照戻り値は不可。
		 * @post 空の場合は`f`を呼ばない。API自身は`value`の保持状態を変更しない。
		 * @note 参照を保持したい場合は、mapperが`std::reference_wrapper<T>`を明示的に返す。
		 * @note mapperの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @note C++23以降では`std::optional::transform`が標準代替。
		 * @code
		 * const auto value = ket::optional::Map(std::optional<int>(2), [](int x) {
		 *     return x * 3;
		 * });
		 * // value == std::optional<int>(6)
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr auto Map(const std::optional<T>& value, F&& f);

		/**
		 * @brief optional保持値のmove変換。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 変換対象のoptional。値を持つ場合は保持値をmapperへmove可能。
		 * @param[in] f 保持値を変換する`std::invoke`互換mapper。
		 * @retval value
		 * `value`が値を持つ場合、`std::invoke(f,
		 * std::move(*value))`を`std::decay_t`で保持するoptional。
		 * @retval std::nullopt `value`が空の場合。
		 * @pre `std::invoke(f,
		 * std::move(*value))`はvoidではないobjectを値として返す。直接の参照戻り値は不可。
		 * @post 空の場合は`f`を呼ばない。値ありの場合は`value`の保持値がmove元になり得る。
		 * @note 参照を保持したい場合は、mapperが`std::reference_wrapper<T>`を明示的に返す。
		 * @note mapperの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @note C++23以降では`std::optional::transform`が標準代替。
		 * @code
		 * auto text = ket::optional::Map(std::optional<std::string>("id"), [](std::string x) {
		 *     return x.size();
		 * });
		 * // text == std::optional<std::size_t>(2)
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr auto Map(std::optional<T>&& value, F&& f);

		/**
		 * @brief const rvalue optional保持値の変換。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 変換対象のoptional。値を持つ場合は保持値をconst
		 * rvalueとしてmapperへ渡す。
		 * @param[in] f 保持値を変換する`std::invoke`互換mapper。
		 * @retval value
		 * `value`が値を持つ場合、`std::invoke(f,
		 * std::move(*value))`を`std::decay_t`で保持するoptional。
		 * @retval std::nullopt `value`が空の場合。
		 * @pre `std::invoke(f,
		 * std::move(*value))`はvoidではないobjectを値として返す。直接の参照戻り値は不可。
		 * @post 空の場合は`f`を呼ばない。API自身は`value`の保持状態を変更しない。
		 * @note const rvalueを区別するmapperへ、標準optionalのvalue categoryに沿って値を渡す。
		 * @note mapperの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @note C++23以降では`std::optional::transform`が標準代替。
		 * @code
		 * const auto source = std::optional<int>(2);
		 * const auto value = ket::optional::Map(std::move(source), [](const int&& x) {
		 *     return x * 3;
		 * });
		 * // value == std::optional<int>(6)
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr auto Map(const std::optional<T>&& value, F&& f);

		/**
		 * @brief mutable optionalを返すmapperによるoptional合成。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in,out] value 合成対象のoptional。値を持つ場合はmapperへmutable参照を渡す。
		 * @param[in] f 保持値からoptionalを返す`std::invoke`互換mapper。
		 * @retval value `value`が値を持つ場合、`std::invoke(f, *value)`が返したoptional。
		 * @retval empty optional `value`が空の場合の空optional。
		 * @pre `std::invoke(f, *value)`は`std::optional<U>`を値として返す。
		 * @post 空の場合は`f`を呼ばない。値ありの場合、mapperは`T&`を通じて保持値を変更できる。
		 * @note 戻り値はunwrapせず、mapperが返したoptional型をそのまま返す。
		 * @note mapperの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @note C++23以降では`std::optional::and_then`が標準代替。
		 * @code
		 * auto source = std::optional<int>(2);
		 * const auto value = ket::optional::AndThen(source, [](int& x) {
		 *     x += 1;
		 *     return std::optional<int>(x * 3);
		 * });
		 * // source == std::optional<int>(3), value == std::optional<int>(9)
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr auto AndThen(std::optional<T>& value, F&& f);

		/**
		 * @brief const optionalを返すmapperによるoptional合成。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 合成対象のoptional。
		 * @param[in] f 保持値からoptionalを返す`std::invoke`互換mapper。
		 * @retval value `value`が値を持つ場合、`std::invoke(f, *value)`が返したoptional。
		 * @retval empty optional `value`が空の場合の空optional。
		 * @pre `std::invoke(f, *value)`は`std::optional<U>`を値として返す。
		 * @post 空の場合は`f`を呼ばない。API自身は`value`の保持状態を変更しない。
		 * @note 戻り値はunwrapせず、mapperが返したoptional型をそのまま返す。
		 * @note mapperの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @note C++23以降では`std::optional::and_then`が標準代替。
		 * @code
		 * const auto value = ket::optional::AndThen(std::optional<int>(2), [](int x) {
		 *     return std::optional<int>(x * 3);
		 * });
		 * // value == std::optional<int>(6)
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr auto AndThen(const std::optional<T>& value, F&& f);

		/**
		 * @brief optionalを返すmapperによるoptionalのmove合成。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 合成対象のoptional。値を持つ場合は保持値をmapperへmove可能。
		 * @param[in] f 保持値からoptionalを返す`std::invoke`互換mapper。
		 * @retval value `value`が値を持つ場合、`std::invoke(f,
		 * std::move(*value))`が返したoptional。
		 * @retval empty optional `value`が空の場合の空optional。
		 * @pre `std::invoke(f, std::move(*value))`は`std::optional<U>`を値として返す。
		 * @post 空の場合は`f`を呼ばない。値ありの場合は`value`の保持値がmove元になり得る。
		 * @note 戻り値はunwrapせず、mapperが返したoptional型をそのまま返す。
		 * @note mapperの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @note C++23以降では`std::optional::and_then`が標準代替。
		 * @code
		 * auto value = ket::optional::AndThen(std::optional<std::string>("id"), [](std::string x) {
		 *     return std::optional<std::size_t>(x.size());
		 * });
		 * // value == std::optional<std::size_t>(2)
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr auto AndThen(std::optional<T>&& value, F&& f);

		/**
		 * @brief optionalを返すmapperによるconst rvalue optionalの合成。
		 * @tparam T optionalの保持型。
		 * @tparam F mapperの型。
		 * @param[in] value 合成対象のoptional。値を持つ場合は保持値をconst
		 * rvalueとしてmapperへ渡す。
		 * @param[in] f 保持値からoptionalを返す`std::invoke`互換mapper。
		 * @retval value `value`が値を持つ場合、`std::invoke(f,
		 * std::move(*value))`が返したoptional。
		 * @retval empty optional `value`が空の場合の空optional。
		 * @pre `std::invoke(f, std::move(*value))`は`std::optional<U>`を値として返す。
		 * @post 空の場合は`f`を呼ばない。API自身は`value`の保持状態を変更しない。
		 * @note 戻り値はunwrapせず、mapperが返したoptional型をそのまま返す。
		 * @note mapperの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @note C++23以降では`std::optional::and_then`が標準代替。
		 * @code
		 * const auto source = std::optional<int>(2);
		 * const auto value = ket::optional::AndThen(std::move(source), [](const int&& x) {
		 *     return std::optional<int>(x * 3);
		 * });
		 * // value == std::optional<int>(6)
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr auto AndThen(const std::optional<T>&& value, F&& f);

		/**
		 * @brief optional保持値、または遅延評価fallbackの値取得。
		 * @tparam T optionalの保持型。
		 * @tparam F fallback factoryの型。
		 * @param[in] value 値取得対象のoptional。
		 * @param[in] fallback_factory 空の場合だけ評価する`std::invoke`互換fallback factory。
		 * @retval value
		 * `value`が値を持つ場合はその保持値のcopy、空の場合は`fallback_factory()`の結果。
		 * @pre Tはcopy constructible。`std::invoke(fallback_factory)`はTへ変換可能な値を返す。
		 * @post
		 * 値ありの場合は`fallback_factory`を呼ばない。API自身は`value`の保持状態を変更しない。
		 * @note `std::optional::value_or`と異なり、fallbackは必要時だけ評価。
		 * @note fallback factoryの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @code
		 * const auto value = ket::optional::ValueOrEval(std::optional<int>(), [] { return 42; });
		 * // value == 42
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr T ValueOrEval(const std::optional<T>& value, F&& fallback_factory);

		/**
		 * @brief optional保持値のmove、または遅延評価fallbackの値取得。
		 * @tparam T optionalの保持型。
		 * @tparam F fallback factoryの型。
		 * @param[in] value 値取得対象のoptional。値を持つ場合は保持値を戻り値へmove可能。
		 * @param[in] fallback_factory 空の場合だけ評価する`std::invoke`互換fallback factory。
		 * @retval value
		 * `value`が値を持つ場合はその保持値のmove、空の場合は`fallback_factory()`の結果。
		 * @pre Tはmove constructible。`std::invoke(fallback_factory)`はTへ変換可能な値を返す。
		 * @post
		 * 値ありの場合は`value`の保持値がmove元になり得る。値ありの場合は`fallback_factory`を呼ばない。
		 * @note `std::optional::value_or`と異なり、fallbackは必要時だけ評価。
		 * @note fallback factoryの副作用と例外は、呼び出し元が渡したcallableの規約に従う。
		 * @code
		 * auto value = ket::optional::ValueOrEval(std::optional<std::string>(), [] {
		 *     return std::string("fallback");
		 * });
		 * // value == "fallback"
		 * @endcode
		 */
		template <typename T, typename F>
		constexpr T ValueOrEval(std::optional<T>&& value, F&& fallback_factory);

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
			 * @brief AndThenのmapper戻り型として直接返せるoptional値の判定。
			 * @tparam Value 判定対象の型。
			 * @note optionalの参照戻り値を拒否し、値として返るoptionalだけを許容。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Value>
			struct IsValidAndThenResult
				: std::integral_constant<bool,
										 !std::is_reference_v<Value> &&
											 IsOptional<std::decay_t<Value>>::value>
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

			/**
			 * @brief std::reference_wrapper型の判定。
			 * @tparam Value 判定対象の型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Value>
			struct IsReferenceWrapper : std::false_type
			{
			};

			/**
			 * @brief std::reference_wrapper型の判定specialization。
			 * @tparam Value reference_wrapperの保持型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename Value>
			struct IsReferenceWrapper<std::reference_wrapper<Value>> : std::true_type
			{
			};

			/**
			 * @brief pointer-to-memberをconstexpr文脈で呼び出す内部helper。
			 * @tparam Class memberを持つclass型。
			 * @tparam Pointed member pointerの指す型。
			 * @tparam Object 呼び出し対象objectの型。
			 * @tparam Args member functionへ渡す引数型。
			 * @param[in] member 呼び出すmember pointer。
			 * @param[in] object 呼び出し対象object、reference_wrapper、またはpointer。
			 * @param[in] args member functionへ渡す引数。
			 * @retval value member functionの戻り値、またはdata member参照。
			 * @pre `member`は`object`に対して有効なmember pointer。data member
			 * pointerでは追加引数なし。
			 * @post 呼び出し結果以外の副作用は、対象memberの規約に従う。
			 */
			template <typename Class, typename Pointed, typename Object, typename... Args>
			constexpr decltype(auto)
			InvokeMemberPointer(Pointed Class::*member, Object&& object, Args&&... args)
			{
				if constexpr (std::is_function_v<Pointed>)
				{
					if constexpr (std::is_base_of_v<Class, std::decay_t<Object>>)
					{
						return (std::forward<Object>(object).*member)(std::forward<Args>(args)...);
					}
					else
					{
						if constexpr (IsReferenceWrapper<std::decay_t<Object>>::value)
						{
							return (object.get().*member)(std::forward<Args>(args)...);
						}
						else
						{
							return ((*std::forward<Object>(object)).*
									member)(std::forward<Args>(args)...);
						}
					}
				}
				else
				{
					static_assert(sizeof...(Args) == 0,
								  "ket::optional internal data member invocation takes no extra "
								  "arguments.");
					if constexpr (std::is_base_of_v<Class, std::decay_t<Object>>)
					{
						return std::forward<Object>(object).*member;
					}
					else
					{
						if constexpr (IsReferenceWrapper<std::decay_t<Object>>::value)
						{
							return object.get().*member;
						}
						else
						{
							return (*std::forward<Object>(object)).*member;
						}
					}
				}
			}

			/**
			 * @brief C++17 constexpr文脈で使えるstd::invoke相当の内部helper。
			 * @tparam F callableの型。
			 * @tparam Args callableへ渡す引数型。
			 * @param[in] f 呼び出すcallable。
			 * @param[in] args callableへ渡す引数。
			 * @retval value callableの戻り値。
			 * @pre `std::invoke_result_t<F, Args...>`が成立するcallable。
			 * @post 呼び出し結果以外の副作用は、callableの規約に従う。
			 */
			template <typename F, typename... Args>
			constexpr decltype(auto) Invoke(F&& f, Args&&... args)
			{
				if constexpr (std::is_member_pointer_v<std::decay_t<F>>)
				{
					return InvokeMemberPointer(std::forward<F>(f), std::forward<Args>(args)...);
				}
				else
				{
					return std::forward<F>(f)(std::forward<Args>(args)...);
				}
			}

			/**
			 * @brief Mapのmapper呼び出しが公開契約を満たすかの判定。
			 * @tparam F mapperの型。
			 * @tparam Arg mapperへ渡す保持値の参照型。
			 * @tparam Enable SFINAE用の内部引数。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename F, typename Arg, typename Enable = void>
			struct IsValidMapInvocation : std::false_type
			{
			};

			/**
			 * @brief Mapのmapper呼び出し結果に基づく公開契約判定。
			 * @tparam F mapperの型。
			 * @tparam Arg mapperへ渡す保持値の参照型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename F, typename Arg>
			struct IsValidMapInvocation<F, Arg, std::void_t<std::invoke_result_t<F, Arg>>>
				: IsValidMapResult<std::invoke_result_t<F, Arg>>
			{
			};

			/**
			 * @brief Mapの公開戻り型を構築するhelper。
			 * @tparam F mapperの型。
			 * @tparam Arg mapperへ渡す保持値の参照型。
			 * @tparam IsValid mapper呼び出しが公開契約を満たすか。
			 * @note 無効なmapperでは、`std::optional<void>`などを作る前に診断を出す。
			 */
			template <typename F, typename Arg, bool IsValid>
			struct MapResult
			{
				static_assert(IsValidMapInvocation<F, Arg>::value,
							  "ket::optional::Map mapper must be invocable and return a non-void "
							  "object by value.");
			};

			/**
			 * @brief 有効なMap mapper呼び出しから公開戻り型を構築するspecialization。
			 * @tparam F mapperの型。
			 * @tparam Arg mapperへ渡す保持値の参照型。
			 */
			template <typename F, typename Arg>
			struct MapResult<F, Arg, true>
			{
				using Type = std::optional<std::decay_t<std::invoke_result_t<F, Arg>>>;
			};

			template <typename F, typename Arg>
			using MapResultT =
				typename MapResult<F, Arg, IsValidMapInvocation<F, Arg>::value>::Type;

			/**
			 * @brief AndThenのmapper呼び出しが公開契約を満たすかの判定。
			 * @tparam F mapperの型。
			 * @tparam Arg mapperへ渡す保持値の参照型。
			 * @tparam Enable SFINAE用の内部引数。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename F, typename Arg, typename Enable = void>
			struct IsValidAndThenInvocation : std::false_type
			{
			};

			/**
			 * @brief AndThenのmapper呼び出し結果に基づく公開契約判定。
			 * @tparam F mapperの型。
			 * @tparam Arg mapperへ渡す保持値の参照型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename F, typename Arg>
			struct IsValidAndThenInvocation<F, Arg, std::void_t<std::invoke_result_t<F, Arg>>>
				: IsValidAndThenResult<std::invoke_result_t<F, Arg>>
			{
			};

			/**
			 * @brief AndThenの公開戻り型を構築するhelper。
			 * @tparam F mapperの型。
			 * @tparam Arg mapperへ渡す保持値の参照型。
			 * @tparam IsValid mapper呼び出しが公開契約を満たすか。
			 * @note optional以外やoptional参照のmapper戻り値では、公開戻り型を作る前に診断を出す。
			 */
			template <typename F, typename Arg, bool IsValid>
			struct AndThenResult
			{
				static_assert(IsValidAndThenInvocation<F, Arg>::value,
							  "ket::optional::AndThen mapper must be invocable and return "
							  "std::optional<U> by value.");
			};

			/**
			 * @brief 有効なAndThen mapper呼び出しから公開戻り型を構築するspecialization。
			 * @tparam F mapperの型。
			 * @tparam Arg mapperへ渡す保持値の参照型。
			 */
			template <typename F, typename Arg>
			struct AndThenResult<F, Arg, true>
			{
				using Type = std::invoke_result_t<F, Arg>;
			};

			template <typename F, typename Arg>
			using AndThenResultT =
				typename AndThenResult<F, Arg, IsValidAndThenInvocation<F, Arg>::value>::Type;

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename T, typename F>
		constexpr auto Map(std::optional<T>& value, F&& f)
		{
			using optional_result_type = detail::MapResultT<F&&, T&>;

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return optional_result_type{};
			}

			return optional_result_type(detail::Invoke(std::forward<F>(f), *value));
		}

		template <typename T, typename F>
		constexpr auto Map(const std::optional<T>& value, F&& f)
		{
			using optional_result_type = detail::MapResultT<F&&, const T&>;

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return optional_result_type{};
			}

			return optional_result_type(detail::Invoke(std::forward<F>(f), *value));
		}

		template <typename T, typename F>
		constexpr auto Map(std::optional<T>&& value, F&& f)
		{
			using optional_result_type = detail::MapResultT<F&&, T&&>;

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return optional_result_type{};
			}

			return optional_result_type(detail::Invoke(std::forward<F>(f), std::move(*value)));
		}

		template <typename T, typename F>
		constexpr auto Map(const std::optional<T>&& value, F&& f)
		{
			using optional_result_type = detail::MapResultT<F&&, const T&&>;

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return optional_result_type{};
			}

			return optional_result_type(detail::Invoke(std::forward<F>(f), std::move(*value)));
		}

		template <typename T, typename F>
		constexpr auto AndThen(std::optional<T>& value, F&& f)
		{
			using result_type = detail::AndThenResultT<F&&, T&>;

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return result_type{};
			}

			return detail::Invoke(std::forward<F>(f), *value);
		}

		template <typename T, typename F>
		constexpr auto AndThen(const std::optional<T>& value, F&& f)
		{
			using result_type = detail::AndThenResultT<F&&, const T&>;

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return result_type{};
			}

			return detail::Invoke(std::forward<F>(f), *value);
		}

		template <typename T, typename F>
		constexpr auto AndThen(std::optional<T>&& value, F&& f)
		{
			using result_type = detail::AndThenResultT<F&&, T&&>;

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return result_type{};
			}

			return detail::Invoke(std::forward<F>(f), std::move(*value));
		}

		template <typename T, typename F>
		constexpr auto AndThen(const std::optional<T>&& value, F&& f)
		{
			using result_type = detail::AndThenResultT<F&&, const T&&>;

			const auto value_has_value = value.has_value();
			if (!value_has_value)
			{
				return result_type{};
			}

			return detail::Invoke(std::forward<F>(f), std::move(*value));
		}

		template <typename T, typename F>
		constexpr T ValueOrEval(const std::optional<T>& value, F&& fallback_factory)
		{
			static_assert(std::is_copy_constructible_v<T>,
						  "ket::optional::ValueOrEval(const optional<T>&) requires copy "
						  "constructible T.");

			const auto value_has_value = value.has_value();
			if (value_has_value)
			{
				return *value;
			}

			return detail::Invoke(std::forward<F>(fallback_factory));
		}

		template <typename T, typename F>
		constexpr T ValueOrEval(std::optional<T>&& value, F&& fallback_factory)
		{
			static_assert(std::is_move_constructible_v<T>,
						  "ket::optional::ValueOrEval(optional<T>&&) requires move "
						  "constructible T.");

			const auto value_has_value = value.has_value();
			if (value_has_value)
			{
				return std::move(*value);
			}

			return detail::Invoke(std::forward<F>(fallback_factory));
		}

	} // namespace optional

} // namespace ket
