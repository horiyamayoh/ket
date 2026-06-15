#pragma once

/**
 * @file ket_concurrency.h
 * @brief thread joinとfuture ready判定の小さい補助API。
 *
 * @details join忘れを防ぐ所有threadと、futureのready判定を短いAPIへ集約する。
 * ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。thread poolやexecutorではなく、
 * 局所的なjoin管理とtimeoutなしのready確認だけを扱う。
 *
 * @par プロジェクトへの適用方法
 * `ket_concurrency.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：thread joinとfuture ready判定の小さい儀式を標準ライブラリのみで局所化できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：API別。
 * 非推奨理由：APIごとに標準代替の有無が異なるため、module単位では非推奨にしない。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::concurrency
 * 内部実装：ket::concurrency::detail
 */

#include <chrono> // IWYU pragma: keep
// IWYU pragma: no_include <bits/chrono.h>
#include <future>
#include <thread>
#include <utility>

namespace ket
{
	namespace concurrency
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief joinableなstd::threadをdestructorでjoinする所有型。
		 * @note C++20以降で同じ用途を満たせる場合はstd::jthreadを優先。
		 */
		class JoiningThread
		{
		  public:
			/**
			 * @brief threadを所有しない状態の構築。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post `Joinable()`はfalse。
			 */
			JoiningThread() noexcept = default;

			/**
			 * @brief std::threadの所有権取得。
			 * @param[in] thread 所有対象のstd::thread。joinableでないthreadも受け付ける。
			 * @retval void 戻り値なし。
			 * @pre `thread`がjoinableな場合、destructorや代入でself-joinにならない寿命関係。
			 * @post `thread`の移動元状態を所有。引数のthread objectは移動元として破棄。
			 */
			explicit JoiningThread(std::thread thread) noexcept;

			/**
			 * @brief 所有threadのjoin。
			 * @retval void 戻り値なし。
			 * @pre 所有threadがjoinableな場合、destructorを実行するthread自身ではない。
			 * @post joinableなthreadを所有していた場合は完了まで待機。
			 * @note joinできない状態やjoin例外はnoexcept destructorによりstd::terminate。
			 */
			~JoiningThread() noexcept;

			/**
			 * @brief copy構築禁止。
			 * @param[in] other copy元。使用不可。
			 * @retval void 戻り値なし。
			 * @pre copy操作は利用不可。
			 * @post 状態変更なし。
			 */
			JoiningThread(const JoiningThread& other) = delete;

			/**
			 * @brief copy代入禁止。
			 * @param[in] other copy元。使用不可。
			 * @retval value 使用不可。
			 * @pre copy操作は利用不可。
			 * @post 状態変更なし。
			 */
			JoiningThread& operator=(const JoiningThread& other) = delete;

			/**
			 * @brief move構築。
			 * @param[in,out] other 移動元のJoiningThread。
			 * @retval void 戻り値なし。
			 * @pre なし。移動元は有効なJoiningThread。
			 * @post `other`はjoinableなthreadを所有しない。
			 */
			JoiningThread(JoiningThread&& other) noexcept;

			/**
			 * @brief move代入。
			 * @param[in,out] other 移動元のJoiningThread。
			 * @retval value `*this`への参照。
			 * @pre 既存threadがjoinableな場合、代入を実行するthread自身ではない。
			 * @post self-moveでは状態を保持。それ以外では既存threadをjoinしてから所有権を移す。
			 * @note 既存threadのjoinで例外が発生した場合はnoexceptによりstd::terminate。
			 */
			JoiningThread& operator=(JoiningThread&& other) noexcept;

			/**
			 * @brief 内部std::threadへの参照取得。
			 * @retval value 内部std::threadへの参照。
			 * @pre なし。返却参照で直接操作したjoinable状態は利用者責任。
			 * @post `*this`の所有thread object自体は同じ。
			 */
			std::thread& Get() noexcept;

			/**
			 * @brief 所有threadのjoin可能状態確認。
			 * @retval true 所有threadがjoinable。
			 * @retval false 所有threadがjoinableでない。
			 * @pre なし。
			 * @post `*this`と外部状態の変更なし。
			 */
			bool Joinable() const noexcept; // NOLINT(modernize-use-nodiscard)

		  private:
			std::thread thread_;
		};

		/**
		 * @brief future-like objectのready判定。
		 * @tparam Future `wait_for(std::chrono::seconds(0))` を持つfuture-like型。
		 * @param[in,out] future ready判定対象のfuture-like object。
		 * @retval true `wait_for(0)`が`std::future_status::ready`。
		 * @retval false `wait_for(0)`がtimeoutまたはdeferred。
		 * @pre `future.valid()`がtrue。invalid futureはprecondition違反。
		 * @post futureの共有状態を消費せず、値や例外は取り出さない。
		 * @note `std::future_status::deferred` はreadyではないためfalse。
		 * @code
		 * std::promise<int> promise;
		 * auto future = promise.get_future();
		 * promise.set_value(42);
		 * const auto ready = ket::concurrency::IsReady(future);
		 * // ready == true
		 * @endcode
		 */
		template <typename Future>
		bool IsReady(Future& future) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief joinableなstd::threadのjoin実行。
			 * @param[in,out] thread join対象のstd::thread。
			 * @retval void 戻り値なし。
			 * @pre `thread`がjoinableな場合、呼び出し元thread自身ではない。
			 * @post joinableだった場合はjoin済み。joinableでなかった場合は状態変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline void JoinIfJoinable(std::thread& thread) noexcept
			{
				const auto thread_is_joinable = thread.joinable();
				if (thread_is_joinable)
				{
					thread.join();
				}
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		/**
		 * @brief std::thread所有権取得の定義。
		 * @param[in] thread 所有対象のstd::thread。
		 * @retval void 戻り値なし。
		 * @pre `thread`がjoinableな場合、destructorや代入でself-joinにならない寿命関係。
		 * @post `thread`の移動元状態を所有。
		 */
		inline JoiningThread::JoiningThread(std::thread thread) noexcept
			: thread_(std::move(thread))
		{
		}

		/**
		 * @brief 所有thread joinの定義。
		 * @retval void 戻り値なし。
		 * @pre 所有threadがjoinableな場合、destructorを実行するthread自身ではない。
		 * @post joinableなthreadを所有していた場合は完了まで待機。
		 */
		inline JoiningThread::~JoiningThread() noexcept
		{
			detail::JoinIfJoinable(thread_);
		}

		/**
		 * @brief move構築の定義。
		 * @param[in,out] other 移動元のJoiningThread。
		 * @retval void 戻り値なし。
		 * @pre なし。移動元は有効なJoiningThread。
		 * @post `other`はjoinableなthreadを所有しない。
		 */
		inline JoiningThread::JoiningThread(JoiningThread&& other) noexcept
			: thread_(std::move(other.thread_))
		{
		}

		inline JoiningThread& JoiningThread::operator=(JoiningThread&& other) noexcept
		{
			const auto is_self_assignment = this == &other;
			if (is_self_assignment)
			{
				return *this;
			}

			detail::JoinIfJoinable(thread_);
			thread_ = std::move(other.thread_);

			return *this;
		}

		/**
		 * @brief 内部std::thread参照取得の定義。
		 * @retval value 内部std::threadへの参照。
		 * @pre なし。返却参照で直接操作したjoinable状態は利用者責任。
		 * @post `*this`の所有thread object自体は同じ。
		 */
		inline std::thread& JoiningThread::Get() noexcept
		{
			return thread_;
		}

		/**
		 * @brief join可能状態確認の定義。
		 * @retval true 所有threadがjoinable。
		 * @retval false 所有threadがjoinableでない。
		 * @pre なし。
		 * @post `*this`と外部状態の変更なし。
		 */
		inline bool JoiningThread::Joinable() const noexcept // NOLINT(modernize-use-nodiscard)
		{
			return thread_.joinable();
		}

		template <typename Future>
		bool IsReady(Future& future) noexcept
		{
			const auto status = future.wait_for(std::chrono::seconds(0));

			return status == std::future_status::ready;
		}

	} // namespace concurrency

} // namespace ket
