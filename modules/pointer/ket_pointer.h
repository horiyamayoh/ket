#pragma once

/**
 * @file ket_pointer.h
 * @brief raw pointerのnull不許可とpointer補助API。
 *
 * @details ownershipを持たないraw pointerのnull不許可、weak pointerのlock、overloadされた
 * operator&の回避を短いAPIへ集約する。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_pointer.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：null許容性と所有権の有無を型名や関数名で明示できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::pointer
 * 内部実装：ket::pointer::detail
 */

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ket
{
	namespace pointer
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief nullを保持しないraw pointer wrapper。
		 * @tparam T 参照先の型。
		 * @note 所有権は持たず、参照先のlifetimeは呼び出し側の責任。
		 * @note `void`は対象外。参照先型があるraw pointerに限定。
		 */
		template <typename T>
		class NotNull
		{
			// NOLINTBEGIN(modernize-type-traits): C++11最小要件の公開signature。
			static_assert(!std::is_void<T>::value,
						  "ket::pointer::NotNull does not support void pointees.");

			template <typename U>
			using EnableIfPointerConvertibleTo =
				typename std::enable_if<std::is_convertible<T*, U*>::value &&
										!std::is_void<U>::value>::type;

			/**
			 * @brief NotNull間変換でnull再検査を省く内部tag。
			 */
			struct AlreadyChecked
			{
			};
			// NOLINTEND(modernize-type-traits)

			template <typename U>
			friend class NotNull;

		  public:
			/**
			 * @brief non-null raw pointerからの構築。
			 * @param[in] ptr 保持対象のraw pointer。
			 * @pre `ptr`はraw pointer。`nullptr`はstd::invalid_argumentとして扱う。
			 * @post 構築成功時、`Get()`は`ptr`と同じpointer値を返す。
			 * @code
			 * int value = 42;
			 * ket::pointer::NotNull<int> ptr(&value);
			 * // *ptr == 42
			 * @endcode
			 */
			constexpr explicit NotNull(T* ptr);

			/**
			 * @brief pointer互換のNotNullへの変換。
			 * @tparam U 変換先の参照先型。
			 * @retval value 同じraw pointerを保持する変換先NotNull。
			 * @pre `T*`は`U*`へ暗黙変換可能。参照先lifetimeが有効。
			 * @post 変換元と参照先の状態変更なし。
			 * @code
			 * int value = 42;
			 * ket::pointer::NotNull<int> mutable_ptr(&value);
			 * ket::pointer::NotNull<const int> const_ptr = mutable_ptr;
			 * // *const_ptr == 42
			 * @endcode
			 */
			template <typename U, typename = EnableIfPointerConvertibleTo<U>>
			constexpr operator NotNull<U>() const noexcept;

			/**
			 * @brief 保持中のraw pointer取得。
			 * @retval ptr 構築時に渡したnon-null raw pointer。
			 * @pre 構築でnullを拒否済み。
			 * @post objectと参照先の状態変更なし。
			 * @code
			 * int value = 42;
			 * ket::pointer::NotNull<int> ptr(&value);
			 * // ptr.Get() == &value
			 * @endcode
			 */
			constexpr T* Get() const noexcept // NOLINT(modernize-use-nodiscard): C++11 signature.
			{
				return ptr_;
			}

			/**
			 * @brief 保持中のraw pointerの参照先取得。
			 * @retval reference 保持中のraw pointerが指すobject。
			 * @pre 参照先objectのlifetimeが有効。
			 * @post wrapperの状態変更なし。参照先の変更可否は`T`に従う。
			 * @code
			 * int value = 42;
			 * ket::pointer::NotNull<int> ptr(&value);
			 * *ptr = 7;
			 * // value == 7
			 * @endcode
			 */
			constexpr typename std::add_lvalue_reference<T>::type // NOLINT(modernize-type-traits)
			operator*() const noexcept
			{
				return *ptr_;
			}

			/**
			 * @brief 保持中のraw pointerのmember access。
			 * @retval ptr 構築時に渡したnon-null raw pointer。
			 * @pre 参照先objectのlifetimeが有効。
			 * @post wrapperと参照先の状態変更なし。
			 * @code
			 * struct Entry
			 * {
			 *     int value;
			 * };
			 * Entry entry{42};
			 * ket::pointer::NotNull<Entry> ptr(&entry);
			 * // ptr->value == 42
			 * @endcode
			 */
			constexpr T* operator->() const noexcept
			{
				return ptr_;
			}

		  private:
			constexpr NotNull(T* ptr, AlreadyChecked /*already_checked*/) noexcept : ptr_(ptr) {}

			T* ptr_;
		};

		/**
		 * @brief weak pointerのlock結果取得。
		 * @param[in] weak lock対象のweak pointer。
		 * @retval value `weak.lock()`で得たshared pointer。
		 * @retval empty `weak`がexpiredのときの空shared pointer。
		 * @pre `weak`は有効なstd::weak_ptr object。
		 * @post `weak`と外部状態の変更なし。
		 * @code
		 * const auto locked = ket::pointer::LockWeak(weak);
		 * // expiredなら空std::shared_ptr
		 * @endcode
		 */
		template <typename T>
		std::shared_ptr<T> LockWeak(const std::weak_ptr<T>& weak) noexcept;

		/**
		 * @brief overloadされたoperator&を回避した実address取得。
		 * @param[in] value address取得対象のlvalue。
		 * @retval ptr `value`の実address。
		 * @pre `value`は有効なlvalue。
		 * @post `value`と外部状態の変更なし。
		 * @code
		 * const auto ptr = ket::pointer::AddressOf(value);
		 * // operator& overloadを呼ばない
		 * @endcode
		 */
		template <typename T>
		T* AddressOf(T& value) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief NotNull構築時のnull拒否。
			 * @param[in] ptr 検証対象のraw pointer。
			 * @retval ptr `nullptr`ではない入力pointer。
			 * @pre なし。`nullptr`はstd::invalid_argumentとして扱う。
			 * @post non-null時は引数と外部状態の変更なし。null時は例外送出。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename T>
			constexpr T* RequireNotNull(T* ptr)
			{
				return ptr != nullptr ? ptr : throw std::invalid_argument("null pointer");
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename T>
		constexpr NotNull<T>::NotNull(T* ptr) : ptr_(detail::RequireNotNull(ptr))
		{
		}

		template <typename T>
		template <typename U, typename>
		constexpr NotNull<T>::operator NotNull<U>() const noexcept
		{
			return NotNull<U>(ptr_, typename NotNull<U>::AlreadyChecked());
		}

		template <typename T>
		std::shared_ptr<T> LockWeak(const std::weak_ptr<T>& weak) noexcept
		{
			return weak.lock();
		}

		template <typename T>
		T* AddressOf(T& value) noexcept
		{
			return std::addressof(value);
		}

	} // namespace pointer

} // namespace ket
