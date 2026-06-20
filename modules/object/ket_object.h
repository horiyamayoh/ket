#pragma once

/**
 * @file ket_object.h
 * @brief copy/move 意図を型定義へ集約する object 補助API。
 *
 * @details 継承用 mixin と move 後リセット helper を、標準ライブラリのみで薄く提供する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。比較演算は宣言せず、
 * 利用側の object 比較方針へ介入しない。
 *
 * @par プロジェクトへの適用方法
 * `ket_object.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：copy/move意図を型定義の近くへ集約できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::object
 * 内部実装：ket::object::detail
 */

#include <type_traits>
#include <utility>

namespace ket
{
	namespace object
	{
		// NOLINTBEGIN(modernize-type-traits, modernize-use-nodiscard)

		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 派生型のcopyを禁止する継承用mixin。
		 * @note 空baseとして使うため、data memberと比較演算は持たない。
		 */
		class NonCopyable
		{
		  protected:
			NonCopyable() = default;
			~NonCopyable() = default;

		  public:
			NonCopyable(const NonCopyable&) = delete;
			NonCopyable& operator=(const NonCopyable&) = delete;
		};

		/**
		 * @brief 派生型のcopyとmoveを禁止する継承用mixin。
		 * @note 空baseとして使うため、data memberと比較演算は持たない。
		 */
		class NonMovable
		{
		  protected:
			NonMovable() = default;
			~NonMovable() = default;

		  public:
			NonMovable(const NonMovable&) = delete;
			NonMovable& operator=(const NonMovable&) = delete;
			NonMovable(NonMovable&&) = delete;
			NonMovable& operator=(NonMovable&&) = delete;
		};

		/**
		 * @brief 派生型をmove専用にする継承用mixin。
		 * @note 空baseとして使うため、data memberと比較演算は持たない。
		 */
		class MoveOnly
		{
		  protected:
			MoveOnly() = default;
			~MoveOnly() = default;

		  public:
			MoveOnly(const MoveOnly&) = delete;
			MoveOnly& operator=(const MoveOnly&) = delete;
			MoveOnly(MoveOnly&&) = default;
			MoveOnly& operator=(MoveOnly&&) = default;
		};

		/**
		 * @brief move 後のsourceを値初期化状態へ戻す小値 wrapper。
		 * @tparam T 保持する値の型。
		 * @note `T`はdefault constructible、move constructible、move assignableである必要あり。
		 * @note 比較演算は宣言せず、比較方針は利用側の型へ委ねる。
		 */
		template <typename T>
		class ResetOnMove
		{
			static_assert(std::is_default_constructible<T>::value,
						  "ket::object::ResetOnMove<T> requires default constructible T.");
			static_assert(std::is_move_constructible<T>::value,
						  "ket::object::ResetOnMove<T> requires move constructible T.");
			static_assert(std::is_move_assignable<T>::value,
						  "ket::object::ResetOnMove<T> requires move assignable T.");

		  public:
			/**
			 * @brief 値初期化した保持値で構築。
			 * @retval value 値初期化済みのwrapper。
			 * @pre `T{}`が有効。
			 * @post `Get()`は値初期化された`T`への参照を返す。
			 * @code
			 * ket::object::ResetOnMove<int> value;
			 * // value.Get() == 0
			 * @endcode
			 */
			ResetOnMove() = default;

			/**
			 * @brief 初期値を保持して構築。
			 * @param[in] value 保持する初期値。
			 * @retval value `value`からmove構築したwrapper。
			 * @pre `T`は`value`からmove構築可能。
			 * @post `Get()`は渡された値からmove構築した`T`への参照を返す。
			 * @code
			 * ket::object::ResetOnMove<int> value(42);
			 * // value.Get() == 42
			 * @endcode
			 */
			explicit ResetOnMove(T value);

			/**
			 * @brief move 構築後にsourceを値初期化状態へ戻す。
			 * @param[in,out] other move元wrapper。
			 * @retval value `other`の保持値からmove構築したwrapper。
			 * @pre `other`は有効なwrapper。`T{}`によるresetが有効。
			 * @post
			 * 構築先はmove前の`other.Get()`の値を保持し、`other.Get()`は`T{}`で代入された状態。
			 * @code
			 * ket::object::ResetOnMove<int> source(42);
			 * ket::object::ResetOnMove<int> moved(std::move(source));
			 * // moved.Get() == 42, source.Get() == 0
			 * @endcode
			 */
			ResetOnMove(ResetOnMove&& other) noexcept(
				std::is_nothrow_move_constructible<T>::value &&
				std::is_nothrow_default_constructible<T>::value &&
				std::is_nothrow_move_assignable<T>::value);

			/**
			 * @brief move 代入後にsourceを値初期化状態へ戻す。
			 * @param[in,out] other move元wrapper。
			 * @retval value `*this`への参照。
			 * @pre `*this`と`other`は有効なwrapper。`T{}`によるresetが有効。
			 * @post
			 * `*this`はmove前の`other.Get()`の値を保持し、`other.Get()`は`T{}`で代入された状態。
			 * @code
			 * ket::object::ResetOnMove<int> source(42);
			 * ket::object::ResetOnMove<int> target;
			 * target = std::move(source);
			 * // target.Get() == 42, source.Get() == 0
			 * @endcode
			 */
			ResetOnMove& operator=(ResetOnMove&& other) noexcept(
				std::is_nothrow_move_assignable<T>::value &&
				std::is_nothrow_default_constructible<T>::value);

			/**
			 * @brief 保持値のmutable参照取得。
			 * @retval value 保持値へのmutable参照。
			 * @pre `*this`は有効なwrapper。
			 * @post wrapperの保持値と外部状態の変更なし。
			 * @code
			 * ket::object::ResetOnMove<int> value;
			 * value.Get() = 42;
			 * // value.Get() == 42
			 * @endcode
			 */
			T& Get() noexcept;

			/**
			 * @brief 保持値のconst参照取得。
			 * @retval value 保持値へのconst参照。
			 * @pre `*this`は有効なwrapper。
			 * @post wrapperの保持値と外部状態の変更なし。
			 * @code
			 * const ket::object::ResetOnMove<int> value(42);
			 * const auto current = value.Get();
			 * // current == 42
			 * @endcode
			 */
			const T& Get() const noexcept;

		  private:
			T value_ = T{};
		};

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief objectを値初期化状態へ戻す。
			 * @tparam T reset対象の型。
			 * @param[in,out] value reset対象の値。
			 * @retval void 戻り値なし。
			 * @pre `T{}`を`value`へmove代入可能。
			 * @post `value`は`T{}`で代入された状態。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename T>
			void
			ResetToDefault(T& value) noexcept(std::is_nothrow_default_constructible<T>::value &&
											  std::is_nothrow_move_assignable<T>::value)
			{
				value = T{};
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename T>
		ResetOnMove<T>::ResetOnMove(T value) : value_(std::move(value))
		{
		}

		template <typename T>
		ResetOnMove<T>::ResetOnMove(ResetOnMove&& other) noexcept(
			std::is_nothrow_move_constructible<T>::value &&
			std::is_nothrow_default_constructible<T>::value &&
			std::is_nothrow_move_assignable<T>::value)
			: value_{std::move(other.value_)}
		{
			detail::ResetToDefault(other.value_);
		}

		template <typename T>
		ResetOnMove<T>& ResetOnMove<T>::operator=(ResetOnMove&& other) noexcept(
			std::is_nothrow_move_assignable<T>::value &&
			std::is_nothrow_default_constructible<T>::value)
		{
			const auto self_assignment = this == &other;
			if (self_assignment)
			{
				return *this;
			}

			value_ = std::move(other.value_);
			detail::ResetToDefault(other.value_);
			return *this;
		}

		template <typename T>
		T& ResetOnMove<T>::Get() noexcept
		{
			return value_;
		}

		template <typename T>
		const T& ResetOnMove<T>::Get() const noexcept
		{
			return value_;
		}

		// NOLINTEND(modernize-type-traits, modernize-use-nodiscard)

	} // namespace object

} // namespace ket
