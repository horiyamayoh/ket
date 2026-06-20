#pragma once

/**
 * @file ket_cache.h
 * @brief once/lazy valueの補助API。
 *
 * @details `Lazy<T>`で値の遅延生成、再利用、明示的な破棄を局所化する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。thread-safe化、
 * global registry、memoization frameworkは扱わない。
 *
 * @par プロジェクトへの適用方法
 * `ket_cache.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：lazy valueの非thread-safe方針と例外後状態を標準ライブラリのみで局所的に固定可能。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::cache
 * 内部実装：ket::cache::detail
 */

#include <exception>
#include <new>
#include <type_traits>
#include <utility>

namespace ket
{
	namespace cache
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 手動storage上に値を一度だけ遅延生成する非thread-safeなlazy value。
		 * @tparam T 保持する値の型。
		 * @note `Lazy`自体はcopy/move禁止。保持値のaddress stabilityを`Reset`まで維持。
		 * @note 再入、同時アクセス、thread-safe cacheは対象外。
		 */
		template <typename T>
		class Lazy
		{
			// NOLINTNEXTLINE(modernize-type-traits): C++11最小要件のため`_v` alias不使用。
			static_assert(std::is_object<T>::value, "ket::cache::Lazy<T> requires an object type.");
			// NOLINTNEXTLINE(modernize-type-traits): C++11最小要件のため`_v` alias不使用。
			static_assert(!std::is_array<T>::value,
						  "ket::cache::Lazy<T> does not support array types.");
			// NOLINTNEXTLINE(modernize-type-traits): C++11最小要件のため`_v` alias不使用。
			static_assert(std::is_destructible<T>::value,
						  "ket::cache::Lazy<T> requires a destructible type.");

		  public:
			/**
			 * @brief 空状態のlazy value構築。
			 * @pre `T`は`std::aligned_storage`で格納可能なobject type。
			 * @post 保持値なし。heap allocationなし。
			 * @code
			 * ket::cache::Lazy<int> value;
			 * // value.HasValue() == false
			 * @endcode
			 */
			Lazy() noexcept;

			/**
			 * @brief 保持値がある場合の破棄。
			 * @pre `T`のdestructorは例外を投げず、同じ`Lazy`へ再入しない。
			 * @post 保持値があった場合はそのlifetime終了。destructor例外は`std::terminate`。
			 * @code
			 * {
			 *     ket::cache::Lazy<int> value;
			 *     value.GetOrCreate([] { return 42; });
			 * }
			 * // held int lifetime has ended
			 * @endcode
			 */
			~Lazy() noexcept;

			/**
			 * @brief copy construction禁止。
			 * @param[in] other copy元候補。delete済みのため参照不可。
			 * @pre 呼び出し不可。
			 * @post `Lazy` objectは生成されない。
			 * @code
			 * ket::cache::Lazy<int> value;
			 * // ket::cache::Lazy<int> copy(value); // compile error
			 * @endcode
			 */
			Lazy(const Lazy&) = delete;

			/**
			 * @brief copy assignment禁止。
			 * @param[in] other copy元候補。delete済みのため参照不可。
			 * @retval self 到達不可。宣言上は左辺参照を返す。
			 * @pre 呼び出し不可。
			 * @post 左辺の`Lazy`状態変更なし。
			 * @code
			 * ket::cache::Lazy<int> lhs;
			 * ket::cache::Lazy<int> rhs;
			 * // lhs = rhs; // compile error
			 * @endcode
			 */
			Lazy& operator=(const Lazy&) = delete;

			/**
			 * @brief move construction禁止。
			 * @param[in] other move元候補。delete済みのため参照不可。
			 * @pre 呼び出し不可。
			 * @post `Lazy` objectは生成されない。
			 * @code
			 * ket::cache::Lazy<int> value;
			 * // ket::cache::Lazy<int> moved(std::move(value)); // compile error
			 * @endcode
			 */
			Lazy(Lazy&&) = delete;

			/**
			 * @brief move assignment禁止。
			 * @param[in] other move元候補。delete済みのため参照不可。
			 * @retval self 到達不可。宣言上は左辺参照を返す。
			 * @pre 呼び出し不可。
			 * @post 左辺の`Lazy`状態変更なし。
			 * @code
			 * ket::cache::Lazy<int> lhs;
			 * ket::cache::Lazy<int> rhs;
			 * // lhs = std::move(rhs); // compile error
			 * @endcode
			 */
			Lazy& operator=(Lazy&&) = delete;

			/**
			 * @brief 保持値の有無判定。
			 * @retval true 保持値あり。
			 * @retval false 保持値なし。
			 * @pre なし。
			 * @post `Lazy`と保持値の状態変更なし。
			 * @code
			 * ket::cache::Lazy<int> value;
			 * const auto empty = value.HasValue();
			 * value.GetOrCreate([] { return 42; });
			 * const auto created = value.HasValue();
			 * // empty == false && created == true
			 * @endcode
			 */
			bool HasValue() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 保持値の明示的な破棄。
			 * @retval void 戻り値なし。
			 * @pre `T`のdestructorは例外を投げず、同じ`Lazy`へ再入しない。
			 * @post 保持値があった場合はemptyへ戻る。保持値なしの場合は状態変更なし。
			 * @note 破棄中の例外または再入検出時は`std::terminate`。
			 * @code
			 * ket::cache::Lazy<int> value;
			 * value.GetOrCreate([] { return 42; });
			 * value.Reset();
			 * // value.HasValue() == false
			 * @endcode
			 */
			void Reset() noexcept;

			/**
			 * @brief 保持値の取得または遅延生成。
			 * @tparam Factory 保持値がない場合に呼び出すfactoryの型。
			 * @param[in] factory 必要時だけ呼び出すfactory object。
			 * @retval value 保持値への参照。
			 * @pre 同じ`Lazy`への`GetOrCreate`または`Reset`をfactory実行中に再入呼び出ししない。
			 * @post 成功時は保持値あり。factoryまたは`T`構築の例外時はemptyのまま。
			 * @note 値がある場合、factoryは呼び出しなし。factory objectは`Lazy`内に保持しない。
			 * @note factory実行中の再入検出時は`std::terminate`。
			 * @note 追加heap allocationなし。
			 * @code
			 * ket::cache::Lazy<int> value;
			 * int& first = value.GetOrCreate([] { return 42; });
			 * int& second = value.GetOrCreate([] { return 7; });
			 * // &first == &second && second == 42
			 * @endcode
			 */
			template <typename Factory>
			T& GetOrCreate(Factory&& factory);

		  private:
			// NOLINTNEXTLINE(modernize-type-traits): C++11最小要件のため`_t` alias不使用。
			typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
			bool has_value_ = false;
			bool is_constructing_ = false;
			bool is_destroying_ = false;
		};

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief placement new用のstorage address取得。
			 * @tparam Storage 手動storage型。
			 * @param[in,out] storage placement new対象のstorage。
			 * @retval value `storage`先頭のmutable address。
			 * @pre `storage`は有効なstorage object。
			 * @post `storage`の状態変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename Storage>
			void* MutableStorageAddress(Storage& storage) noexcept
			{
				return static_cast<void*>(&storage);
			}

			/**
			 * @brief lifetime中の保持値pointer取得。
			 * @tparam T 保持値の型。
			 * @tparam Storage 手動storage型。
			 * @param[in,out] storage `T`のlifetime中であるstorage。
			 * @retval value `storage`内の保持値pointer。
			 * @pre placement new成功後、かつdestructor呼び出し前のstorage。
			 * @post `storage`と保持値の状態変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename T, typename Storage>
			T* ConstructedValuePointer(Storage& storage) noexcept
			{
				T* const value = static_cast<T*>(MutableStorageAddress(storage));
#if defined(__cpp_lib_launder) && __cpp_lib_launder >= 201606L
				return std::launder(value);
#else
				return value;
#endif
			}

			/**
			 * @brief lifetime中の保持値const pointer取得。
			 * @tparam T 保持値の型。
			 * @tparam Storage 手動storage型。
			 * @param[in] storage `T`のlifetime中であるstorage。
			 * @retval value `storage`内の保持値const pointer。
			 * @pre placement new成功後、かつdestructor呼び出し前のstorage。
			 * @post `storage`と保持値の状態変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename T, typename Storage>
			const T* ConstructedValuePointer(const Storage& storage) noexcept
			{
				const T* const value = reinterpret_cast<const T*>(&storage);
#if defined(__cpp_lib_launder) && __cpp_lib_launder >= 201606L
				return std::launder(value);
#else
				return value;
#endif
			}

			/**
			 * @brief placement new成功直後に保持値あり状態へmark。
			 * @tparam T 保持値の型。
			 * @param[in] value placement newが返した保持値pointer。
			 * @param[in,out] has_value 保持値有無flag。
			 * @retval value `value`をそのまま返す。
			 * @pre `value`はplacement new成功直後の有効な保持値pointer。
			 * @post `has_value`はtrue。
			 * @note factory戻り値の一時object破棄より前にmarkするためのdetail helper。
			 */
			template <typename T>
			T* MarkConstructed(T* value, bool& has_value) noexcept
			{
				has_value = true;
				return value;
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename T>
		Lazy<T>::Lazy() noexcept = default;

		template <typename T>
		Lazy<T>::~Lazy() noexcept
		{
			// cppcheck-suppress throwInNoexceptFunction
			Reset();
		}

		template <typename T>
		bool Lazy<T>::HasValue() const noexcept
		{
			return has_value_;
		}

		template <typename T>
		void Lazy<T>::Reset() noexcept // NOLINT(bugprone-exception-escape,misc-no-recursion)
		{
			const auto operation_in_progress = is_constructing_ || is_destroying_;
			if (operation_in_progress)
			{
				std::terminate();
			}

			const auto value_exists = has_value_;
			if (!value_exists)
			{
				return;
			}

			T* const value = detail::ConstructedValuePointer<T>(storage_);
			has_value_ = false;
			is_destroying_ = true;
			// cppcheck-suppress nullPointer
			// cppcheck-suppress throwInNoexceptFunction
			value->~T();
			is_destroying_ = false;
		}

		template <typename T>
		template <typename Factory>
		T& Lazy<T>::GetOrCreate(Factory&& factory) // NOLINT(misc-no-recursion)
		{
			const auto operation_in_progress = is_constructing_ || is_destroying_;
			if (operation_in_progress)
			{
				std::terminate();
			}

			const auto value_exists = has_value_;
			if (value_exists)
			{
				return *detail::ConstructedValuePointer<T>(storage_);
			}

			void* const storage = detail::MutableStorageAddress(storage_);
			is_constructing_ = true;

			try
			{
				T* const value = detail::MarkConstructed(
					new (storage) T(std::forward<Factory>(factory)()), has_value_);
				is_constructing_ = false;

				// cppcheck-suppress nullPointer
				// cppcheck-suppress uninitdata
				return *value;
			}
			catch (...)
			{
				is_constructing_ = false;

				const auto constructed_before_exception = has_value_;
				// cppcheck-suppress knownConditionTrueFalse
				if (constructed_before_exception)
				{
					Reset();
				}

				throw;
			}
		}

	} // namespace cache

} // namespace ket
