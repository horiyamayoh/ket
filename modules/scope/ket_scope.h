#pragma once

/**
 * @file ket_scope.h
 * @brief scope exit cleanup と値復元の RAII API。
 *
 * @details cleanup 漏れと早期 return 時の復元漏れを、小さい header-only API で防ぐ。
 * callback と復元対象の保存値は guard object が直接保持し、追加 allocation は行わない。
 *
 * @par プロジェクトへの適用方法
 * `ket_scope.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：RAII cleanupを標準ライブラリのみで小さいAPIとして持ち出せる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::scope
 * 内部実装：ket::scope::detail
 */

#include <exception>
#include <utility>

#ifndef KET_SCOPE_NODISCARD
#if __cplusplus >= 201703L
#define KET_SCOPE_NODISCARD [[nodiscard]]
#else
#define KET_SCOPE_NODISCARD
#endif
#define KET_SCOPE_NODISCARD_DEFINED_BY_KET_SCOPE 1
#endif

namespace ket
{
	namespace scope
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief scope exit 時に callback を1回だけ実行する move-only guard。
		 * @tparam F 保持する callback 型。引数なしで呼び出し可能な型。
		 */
		template <typename F>
		class Exit;

		/**
		 * @brief callback 型から scope exit guard を生成。
		 * @tparam F 保持する callback 型。引数なしで呼び出し可能な型。
		 * @param[in] f scope exit 時に呼び出す callback。
		 * @retval value active 状態の `Exit<F>`。
		 * @pre `f` は引数なしで呼び出し可能。callback 例外は destructor 内で `std::terminate`。
		 * `F` は nothrow move constructible。callback の参照先は active guard
		 * の破棄まで生存するか、 その前に `Dismiss()` されている必要がある。
		 * @post `f` から move 構築した guard を返す。外部状態の変更は callback 実行時までなし。
		 * @code
		 * bool cleaned = false;
		 * auto guard = ket::scope::MakeExit([&] { cleaned = true; });
		 * // guard.Active() == true
		 * @endcode
		 */
		template <typename F>
		KET_SCOPE_NODISCARD Exit<F> MakeExit(F f) noexcept;

		/**
		 * @brief scope exit 時に対象値を構築時の値へ復元する guard。
		 * @tparam T 復元対象の型。構築時の値保存と復元代入が可能な型。
		 */
		template <typename T>
		class Restore;

		/**
		 * @brief 対象値から復元 guard を生成。
		 * @tparam T 復元対象の型。構築時の値保存と復元代入が可能な型。
		 * @param[in,out] target scope exit 時に構築時の値へ戻す対象。
		 * @retval value active 状態の `Restore<T>`。
		 * @pre `target` は guard より長く生存し、`T` は構築時の値保存と復元代入が可能。
		 * `T` の copy 構築例外はこの関数から伝播。
		 * @post `target` の現在値を保存した guard を返す。`target` の値は構築時点では変更なし。
		 * 例外送出時は guard を返さない。
		 * @code
		 * int mode = 1;
		 * auto restore = ket::scope::MakeRestore(mode);
		 * mode = 2;
		 * // restore破棄時にmode == 1
		 * @endcode
		 */
		template <typename T>
		KET_SCOPE_NODISCARD Restore<T>
		MakeRestore(T& target) noexcept(noexcept(T(std::declval<const T&>())));

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief scope exit callback の実行。
			 * @tparam F callback 型。
			 * @param[in,out] callback 呼び出す callback object。
			 * @retval void 戻り値なし。
			 * @pre `callback` は引数なしで呼び出し可能。
			 * @post callback の副作用だけが発生。例外送出時は `noexcept` により `std::terminate`。
			 */
			template <typename F>
			void Invoke(F& callback) noexcept
			{
				try
				{
					callback();
				}
				catch (...)
				{
					std::terminate();
				}
			}

			/**
			 * @brief Exit move constructor の noexcept 条件。
			 * @tparam F callback 型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename F>
			struct ExitMoveNoexcept
			{
				static const bool value = noexcept(F(std::declval<F&&>()));
			};

			/**
			 * @brief 保存値による復元代入。
			 * @tparam T 復元対象の型。
			 * @param[in,out] target 復元先。
			 * @param[in] value 復元する保存値。
			 * @retval void 戻り値なし。
			 * @pre `target = value` が有効な式。
			 * @post `target` は `value` と等価な状態。例外送出時は `noexcept` により
			 * `std::terminate`。
			 */
			template <typename T>
			void RestoreValue(T& target, const T& value) noexcept
			{
				try
				{
					target = value;
				}
				catch (...)
				{
					std::terminate();
				}
			}

			/**
			 * @brief Restore の保存値 copy に対する noexcept 条件。
			 * @tparam T 復元対象の型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct RestoreValueCopyNoexcept
			{
				static const bool value = noexcept(T(std::declval<const T&>()));
			};

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		/**
		 * @brief scope exit 時に callback を1回だけ実行する move-only guard。
		 * @tparam F 保持する callback 型。引数なしで呼び出し可能な型。
		 */
		template <typename F>
		class Exit
		{
		  public:
			/**
			 * @brief callback を保持して active guard を構築。
			 * @param[in] f scope exit 時に呼び出す callback。
			 * @pre `f` は引数なしで呼び出し可能。callback 例外は destructor 内で `std::terminate`。
			 * `F` は nothrow move constructible。callback の参照先は active guard の破棄まで
			 * 生存するか、その前に `Dismiss()` されている必要がある。
			 * @post guard は active。callback は value として保持。
			 * @code
			 * bool cleaned = false;
			 * auto callback = [&cleaned] { cleaned = true; };
			 * ket::scope::Exit<decltype(callback)> guard(callback);
			 * // guard.Active() == true
			 * @endcode
			 */
			explicit Exit(F f) noexcept : callback_(std::move(f))
			{
				static_assert(detail::ExitMoveNoexcept<F>::value,
							  "ket::scope::Exit callback must be nothrow move constructible.");
			}

			/**
			 * @brief guard の所有権を move。
			 * @param[in,out] other move 元 guard。
			 * @pre `F` は nothrow move constructible。
			 * @post move 先は move 前の `other` と同じ active 状態。`other` は inactive。
			 * @code
			 * auto guard = ket::scope::MakeExit([] {});
			 * auto moved = std::move(guard);
			 * // moved.Active() == true, guard.Active() == false
			 * @endcode
			 */
			Exit(Exit&& other) noexcept
				: callback_(std::move(other.callback_)), active_(other.active_)
			{
				static_assert(detail::ExitMoveNoexcept<F>::value,
							  "ket::scope::Exit callback must be nothrow move constructible.");
				other.active_ = false;
			}

			/**
			 * @brief copy 構築禁止。
			 * @param[in] other copy 元 guard。
			 * @pre copy は許可しない。
			 * @post この関数は利用不可。
			 * @code
			 * auto guard = ket::scope::MakeExit([] {});
			 * // auto copy = guard;  // compile error
			 * @endcode
			 */
			Exit(const Exit& other) = delete;

			/**
			 * @brief copy 代入禁止。
			 * @param[in] other copy 元 guard。
			 * @retval value 代入結果は提供しない。
			 * @pre copy 代入は許可しない。
			 * @post この関数は利用不可。
			 * @code
			 * auto callback = [] {};
			 * ket::scope::Exit<decltype(callback)> guard(callback);
			 * ket::scope::Exit<decltype(callback)> other(callback);
			 * // guard = other;  // compile error
			 * @endcode
			 */
			Exit& operator=(const Exit& other) = delete;

			/**
			 * @brief move 代入禁止。
			 * @param[in] other move 元 guard。
			 * @retval value 代入結果は提供しない。
			 * @pre move 代入は許可しない。
			 * @post この関数は利用不可。
			 * @code
			 * auto callback = [] {};
			 * ket::scope::Exit<decltype(callback)> guard(callback);
			 * ket::scope::Exit<decltype(callback)> other(callback);
			 * // guard = std::move(other);  // compile error
			 * @endcode
			 */
			Exit& operator=(Exit&& other) = delete;

			/**
			 * @brief active guard 破棄時の callback 実行。
			 * @pre active 状態では callback が引数なしで呼び出し可能。
			 * @post active なら callback を1回実行。inactive なら callback 実行なし。
			 * @code
			 * bool cleaned = false;
			 * {
			 *     auto guard = ket::scope::MakeExit([&] { cleaned = true; });
			 * }
			 * // cleaned == true
			 * @endcode
			 */
			~Exit() noexcept
			{
				const auto should_invoke = active_;
				if (should_invoke)
				{
					detail::Invoke(callback_);
				}
			}

			/**
			 * @brief callback 実行を解除。
			 * @retval void 戻り値なし。
			 * @pre guard は有効な object。
			 * @post guard は inactive。destructor で callback を呼ばない。
			 * @code
			 * bool cleaned = false;
			 * {
			 *     auto guard = ket::scope::MakeExit([&] { cleaned = true; });
			 *     guard.Dismiss();
			 * }
			 * // cleaned == false
			 * @endcode
			 */
			void Dismiss() noexcept
			{
				active_ = false;
			}

			/**
			 * @brief callback 実行予定の有無を取得。
			 * @retval true destructor で callback を呼び出す状態。
			 * @retval false dismiss 済み、または move 後の inactive 状態。
			 * @pre guard は有効な object。
			 * @post guard と外部状態の変更なし。
			 * @code
			 * auto guard = ket::scope::MakeExit([] {});
			 * const auto active = guard.Active();
			 * // active == true
			 * @endcode
			 */
			KET_SCOPE_NODISCARD bool Active() const noexcept
			{
				return active_;
			}

		  private:
			F callback_;
			bool active_ = true;
		};

		template <typename F>
		KET_SCOPE_NODISCARD Exit<F> MakeExit(F f) noexcept
		{
			return Exit<F>(std::move(f));
		}

		/**
		 * @brief scope exit 時に対象値を構築時の値へ復元する guard。
		 * @tparam T 復元対象の型。構築時の値保存と復元代入が可能な型。
		 */
		template <typename T>
		class Restore
		{
		  public:
			/**
			 * @brief 対象値の現在値を保存して active guard を構築。
			 * @param[in,out] target scope exit 時に構築時の値へ戻す対象。
			 * @pre `target` は guard より長く生存し、`T` は構築時の値保存と復元代入が可能。
			 * `T` の copy 構築例外はこの constructor から伝播。
			 * @post `target` の値は変更せず、guard 内に構築時の値を保存。例外送出時は guard
			 * を構築しない。
			 * @code
			 * int value = 1;
			 * ket::scope::Restore<int> restore(value);
			 * value = 2;
			 * // restore破棄時にvalue == 1
			 * @endcode
			 */
			explicit Restore(T& target) noexcept(detail::RestoreValueCopyNoexcept<T>::value)
				: target_(target), value_(target)
			{
			}

			/**
			 * @brief 復元責務を move。
			 * @param[in,out] other move 元 guard。
			 * @pre `T` は copy 構築可能。C++11で`MakeRestore`の戻り値を実用にするための移送。
			 * @post move 先は move 前の `other` と同じ active 状態。`other` は
			 * inactive。例外送出時の `other` は active のまま。
			 * @note C++11で`MakeRestore`の戻り値を実用にするための責務移送。
			 * @code
			 * int value = 1;
			 * auto restore = ket::scope::MakeRestore(value);
			 * auto moved = std::move(restore);
			 * value = 2;
			 * // moved破棄時にvalue == 1
			 * @endcode
			 */
			Restore(Restore&& other) noexcept(detail::RestoreValueCopyNoexcept<T>::value)
				: target_(other.target_), value_(other.value_), active_(other.active_)
			{
				other.active_ = false;
			}

			/**
			 * @brief copy 構築禁止。
			 * @param[in] other copy 元 guard。
			 * @pre copy は許可しない。
			 * @post この関数は利用不可。
			 * @code
			 * int value = 1;
			 * auto restore = ket::scope::MakeRestore(value);
			 * // auto copy = restore;  // compile error
			 * @endcode
			 */
			Restore(const Restore& other) = delete;

			/**
			 * @brief copy 代入禁止。
			 * @param[in] other copy 元 guard。
			 * @retval value 代入結果は提供しない。
			 * @pre copy 代入は許可しない。
			 * @post この関数は利用不可。
			 * @code
			 * int value = 1;
			 * auto restore = ket::scope::MakeRestore(value);
			 * auto other = ket::scope::MakeRestore(value);
			 * // restore = other;  // compile error
			 * @endcode
			 */
			Restore& operator=(const Restore& other) = delete;

			/**
			 * @brief move 代入禁止。
			 * @param[in] other move 元 guard。
			 * @retval value 代入結果は提供しない。
			 * @pre move 代入は許可しない。
			 * @post この関数は利用不可。
			 * @code
			 * int value = 1;
			 * auto restore = ket::scope::MakeRestore(value);
			 * auto other = ket::scope::MakeRestore(value);
			 * // restore = std::move(other);  // compile error
			 * @endcode
			 */
			Restore& operator=(Restore&& other) = delete;

			/**
			 * @brief active guard 破棄時の値復元。
			 * @pre active 状態では保存値の復元代入が有効。
			 * @post active なら対象値を構築時の値へ復元。inactive なら対象値を変更しない。
			 * @code
			 * int value = 1;
			 * {
			 *     auto restore = ket::scope::MakeRestore(value);
			 *     value = 2;
			 * }
			 * // value == 1
			 * @endcode
			 */
			~Restore() noexcept
			{
				const auto should_restore = active_;
				if (should_restore)
				{
					detail::RestoreValue(target_, value_);
				}
			}

			/**
			 * @brief 値復元を解除。
			 * @retval void 戻り値なし。
			 * @pre guard は有効な object。
			 * @post guard は inactive。destructor で対象値を変更しない。
			 * @code
			 * int value = 1;
			 * {
			 *     auto restore = ket::scope::MakeRestore(value);
			 *     value = 2;
			 *     restore.Dismiss();
			 * }
			 * // value == 2
			 * @endcode
			 */
			inline void Dismiss() noexcept // NOLINT(readability-redundant-inline-specifier)
			{
				active_ = false;
			}

			// NOLINTBEGIN(readability-redundant-inline-specifier)
			/**
			 * @brief 値復元予定の有無を取得。
			 * @retval true destructor で対象値を構築時の値へ戻す状態。
			 * @retval false dismiss 済み、または move 後の inactive 状態。
			 * @pre guard は有効な object。
			 * @post guard と対象値の変更なし。
			 * @code
			 * int value = 1;
			 * auto restore = ket::scope::MakeRestore(value);
			 * const auto active = restore.Active();
			 * // active == true
			 * @endcode
			 */
			KET_SCOPE_NODISCARD inline bool
			Active() const noexcept // NOLINT(readability-redundant-inline-specifier)
			{
				return active_;
			}
			// NOLINTEND(readability-redundant-inline-specifier)

		  private:
			T& target_;
			T value_;
			bool active_ = true;
		};

		template <typename T>
		KET_SCOPE_NODISCARD Restore<T>
		MakeRestore(T& target) noexcept(noexcept(T(std::declval<const T&>())))
		{
			return Restore<T>(target);
		}

	} // namespace scope

} // namespace ket

#ifdef KET_SCOPE_NODISCARD_DEFINED_BY_KET_SCOPE
#undef KET_SCOPE_NODISCARD
#undef KET_SCOPE_NODISCARD_DEFINED_BY_KET_SCOPE
#endif
