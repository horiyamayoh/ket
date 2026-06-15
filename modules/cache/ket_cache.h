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
 * 推奨理由：lazy valueのthread-safetyと例外後状態を標準ライブラリのみで局所的に固定可能。
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

#include <new>
#include <type_traits>

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
		  public:
			/**
			 * @brief 空状態のlazy value構築。
			 * @retval void 戻り値なし。
			 * @pre `T`は`std::aligned_storage`で格納可能なobject type。
			 * @post 保持値なし。heap allocationなし。
			 */
			Lazy() noexcept;

			/**
			 * @brief 保持値がある場合の破棄。
			 * @retval void 戻り値なし。
			 * @pre `T`のdestructorは例外を投げない。
			 * @post 保持値があった場合はそのlifetime終了。destructor例外は`std::terminate`。
			 */
			~Lazy() noexcept;

			Lazy(const Lazy&) = delete;
			Lazy& operator=(const Lazy&) = delete;
			Lazy(Lazy&&) = delete;
			Lazy& operator=(Lazy&&) = delete;

			/**
			 * @brief 保持値の有無判定。
			 * @retval true 保持値あり。
			 * @retval false 保持値なし。
			 * @pre なし。
			 * @post `Lazy`と保持値の状態変更なし。
			 */
			bool HasValue() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 保持値の明示的な破棄。
			 * @retval void 戻り値なし。
			 * @pre `T`のdestructorは例外を投げない。
			 * @post 保持値があった場合はemptyへ戻る。保持値なしの場合は状態変更なし。
			 * @note 破棄中の例外は`std::terminate`。
			 */
			void Reset() noexcept;

			/**
			 * @brief 保持値の取得または遅延生成。
			 * @tparam Factory 保持値がない場合に呼び出すfactoryの型。
			 * @param[in] factory `T`へ直接初期化可能な値を返す呼び出し可能object。
			 * @retval value 保持値への参照。
			 * @pre 同じ`Lazy`への`GetOrCreate`または`Reset`をfactory実行中に再入呼び出ししない。
			 * @post 成功時は保持値あり。factoryまたは`T`構築の例外時はemptyのまま。
			 * @note 値がある場合、factoryは呼び出しなし。追加heap allocationなし。
			 * @code
			 * ket::cache::Lazy<int> value;
			 * int& first = value.GetOrCreate([] { return 42; });
			 * int& second = value.GetOrCreate([] { return 7; });
			 * // &first == &second && second == 42
			 * @endcode
			 */
			template <typename Factory>
			T& GetOrCreate(Factory factory);

		  private:
			// NOLINTNEXTLINE(modernize-type-traits): C++11最小要件のため`_t` alias不使用。
			typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
			bool has_value_ = false;
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
				return reinterpret_cast<T*>(&storage);
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
				return reinterpret_cast<const T*>(&storage);
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
		void Lazy<T>::Reset() noexcept // NOLINT(bugprone-exception-escape)
		{
			const auto value_exists = has_value_;
			if (!value_exists)
			{
				return;
			}

			T* const value = detail::ConstructedValuePointer<T>(storage_);
			// cppcheck-suppress nullPointer
			// cppcheck-suppress throwInNoexceptFunction
			value->~T();
			has_value_ = false;
		}

		template <typename T>
		template <typename Factory>
		T& Lazy<T>::GetOrCreate(Factory factory)
		{
			const auto value_exists = has_value_;
			if (value_exists)
			{
				return *detail::ConstructedValuePointer<T>(storage_);
			}

			void* const storage = detail::MutableStorageAddress(storage_);
			T* const value = new (storage) T(factory());
			has_value_ = true;

			// cppcheck-suppress nullPointer
			// cppcheck-suppress uninitdata
			return *value;
		}

	} // namespace cache

} // namespace ket
