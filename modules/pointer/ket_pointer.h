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
		 */
		template <typename T>
		class NotNull
		{
		  public:
			/**
			 * @brief non-null raw pointerからの構築。
			 * @param[in] ptr 保持対象のraw pointer。
			 * @retval void 構築成功。
			 * @pre `ptr`はraw pointer。`nullptr`はstd::invalid_argumentとして扱う。
			 * @post 構築成功時、`Get()`は`ptr`と同じpointer値を返す。
			 */
			explicit NotNull(T* ptr);

			/**
			 * @brief 保持中のraw pointer取得。
			 * @retval ptr 構築時に渡したnon-null raw pointer。
			 * @pre 構築でnullを拒否済み。
			 * @post objectと参照先の状態変更なし。
			 */
			T* Get() const noexcept // NOLINT(modernize-use-nodiscard): C++11 signature.
			{
				return ptr_;
			}

			/**
			 * @brief 保持中のraw pointerの参照先取得。
			 * @retval reference 保持中のraw pointerが指すobject。
			 * @pre 参照先objectのlifetimeが有効。
			 * @post wrapperの状態変更なし。参照先の変更可否は`T`に従う。
			 */
			T& operator*() const noexcept
			{
				return *ptr_;
			}

			/**
			 * @brief 保持中のraw pointerのmember access。
			 * @retval ptr 構築時に渡したnon-null raw pointer。
			 * @pre 参照先objectのlifetimeが有効。
			 * @post wrapperと参照先の状態変更なし。
			 */
			T* operator->() const noexcept
			{
				return ptr_;
			}

		  private:
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
			T* RequireNotNull(T* ptr)
			{
				if (ptr == nullptr)
				{
					throw std::invalid_argument(
						"ket::pointer::NotNull requires a non-null pointer.");
				}

				return ptr;
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename T>
		NotNull<T>::NotNull(T* ptr) : ptr_(detail::RequireNotNull(ptr))
		{
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
