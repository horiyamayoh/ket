#include "ket_concurrency.h"

#include <atomic>
#include <chrono> // IWYU pragma: keep
#include <future>
#include <string> // IWYU pragma: keep
#include <thread>
#include <utility>

#include <gtest/gtest.h>

/**
 * @test
 * @brief default構築時の非joinable確認。
 * @details default構築したJoiningThreadがthreadを所有せず、Joinableでfalseを返すことを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetConcurrencyTest, DefaultConstructsAsNotJoinable)
{
	const ket::concurrency::JoiningThread thread;

	const auto thread_is_joinable = thread.Joinable();

	EXPECT_FALSE(thread_is_joinable);
}

/**
 * @test
 * @brief destructorによるjoin確認。
 * @details joinableなthreadを所有させ、scope退出時にthread完了までjoinされることを確認。
 * @pre C++17以降。生成したthreadはself-joinにならない。
 * @post thread完了フラグはtrue。その他の外部状態の変更なし。
 */
TEST(KetConcurrencyTest, JoinsOwnedThreadOnDestruction)
{
	std::atomic<bool> thread_finished(false);
	std::promise<void> thread_started_promise;
	std::future<void> thread_started = thread_started_promise.get_future();
	std::promise<void> release_thread_promise;
	std::shared_future<void> release_thread = release_thread_promise.get_future().share();
	std::promise<void> thread_done_promise;
	const std::future<void> thread_done = thread_done_promise.get_future();

	std::thread releaser(
		[&thread_started, &release_thread_promise]()
		{
			thread_started.wait();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			release_thread_promise.set_value();
		});

	{
		const ket::concurrency::JoiningThread thread(std::thread(
			[&thread_started_promise, &release_thread, &thread_done_promise, &thread_finished]()
			{
				thread_started_promise.set_value();
				release_thread.wait();
				thread_finished.store(true);
				thread_done_promise.set_value();
			}));

		const auto thread_is_joinable = thread.Joinable();

		EXPECT_TRUE(thread_is_joinable);
	}

	const auto thread_done_status = thread_done.wait_for(std::chrono::seconds(0));
	const auto thread_done_is_ready = thread_done_status == std::future_status::ready;
	const auto finished = thread_finished.load();

	releaser.join();

	EXPECT_TRUE(thread_done_is_ready);
	EXPECT_TRUE(finished);
}

/**
 * @test
 * @brief 非joinableなstd::threadの所有確認。
 * @details default構築されたstd::threadを渡し、JoiningThreadが非joinable状態を保つことを確認。
 * @pre C++17以降。
 * @post 所有threadはjoinableではなく、外部状態の変更なし。
 */
TEST(KetConcurrencyTest, AcceptsNonJoinableThread)
{
	std::thread raw_thread;
	const ket::concurrency::JoiningThread thread(std::move(raw_thread));

	const auto thread_is_joinable = thread.Joinable();

	EXPECT_FALSE(thread_is_joinable);
}

/**
 * @test
 * @brief move構築時の所有権移動確認。
 * @details joinableなthreadをmove構築で移し、移動先がjoinableになることを確認。
 * @pre C++17以降。生成したthreadはself-joinにならない。
 * @post move先destructorによりthread完了フラグはtrue。
 */
TEST(KetConcurrencyTest, MoveConstructsByTransferringThreadOwnership)
{
	std::atomic<bool> thread_finished(false);

	{
		ket::concurrency::JoiningThread source(std::thread(
			[&thread_finished]()
			{
				thread_finished.store(true);
			}));

		const auto source_was_joinable = source.Joinable();
		const ket::concurrency::JoiningThread moved(std::move(source));

		const auto moved_is_joinable = moved.Joinable();
		// cppcheck-suppress accessMoved
		// NOLINTNEXTLINE(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
		const auto source_is_joinable_after_move = source.Joinable();

		EXPECT_TRUE(source_was_joinable);
		EXPECT_TRUE(moved_is_joinable);
		EXPECT_FALSE(source_is_joinable_after_move);
	}

	const auto finished = thread_finished.load();

	EXPECT_TRUE(finished);
}

/**
 * @test
 * @brief self-move代入のno-op確認。
 * @details 同一objectへのmove代入を行い、所有threadのjoinable状態が保持されることを確認。
 * @pre C++17以降。生成したthreadはself-joinにならない。
 * @post destructorによりthread完了フラグはtrue。
 */
TEST(KetConcurrencyTest, SelfMoveAssignmentKeepsThreadOwnership)
{
	std::atomic<bool> thread_finished(false);

	{
		ket::concurrency::JoiningThread thread(std::thread(
			[&thread_finished]()
			{
				thread_finished.store(true);
			}));

		ket::concurrency::JoiningThread& same_thread = thread;
		thread = std::move(same_thread);

		const auto thread_is_joinable = thread.Joinable();

		EXPECT_TRUE(thread_is_joinable);
	}

	const auto finished = thread_finished.load();

	EXPECT_TRUE(finished);
}

/**
 * @test
 * @brief move代入時の旧thread join確認。
 * @details move代入前の旧threadを完了までjoinしてから、新しいthread所有権を受け取ることを確認。
 * @pre C++17以降。旧threadは代入元threadを所有する前に終了可能な状態。
 * @post 旧thread完了フラグはtrue。代入先はjoinable。
 */
TEST(KetConcurrencyTest, MoveAssignmentJoinsPreviousThreadBeforeTakingNewOwnership)
{
	std::promise<void> old_started_promise;
	const std::future<void> old_started = old_started_promise.get_future();
	std::promise<void> old_release_promise;
	std::shared_future<void> old_release = old_release_promise.get_future().share();
	std::atomic<bool> old_thread_finished(false);

	ket::concurrency::JoiningThread target(std::thread(
		[&old_started_promise, &old_release, &old_thread_finished]()
		{
			old_started_promise.set_value();
			old_release.wait();
			old_thread_finished.store(true);
		}));

	old_started.wait();

	ket::concurrency::JoiningThread source{std::thread(
		[]()
		{
		})};

	const auto source_was_joinable = source.Joinable();

	old_release_promise.set_value();
	target = std::move(source);

	const auto old_finished = old_thread_finished.load();
	const auto target_is_joinable = target.Joinable();

	EXPECT_TRUE(source_was_joinable);
	EXPECT_TRUE(old_finished);
	EXPECT_TRUE(target_is_joinable);
}

/**
 * @test
 * @brief readyなfutureとshared_futureの判定確認。
 * @details 値設定済みのstd::futureとstd::shared_futureを渡し、IsReadyがtrueを返すことを確認。
 * @pre C++17以降。対象futureはvalid。
 * @post futureの値は取り出されず、共有状態は保持。
 */
TEST(KetConcurrencyTest, DetectsReadyFutureAndSharedFuture)
{
	std::promise<int> future_promise;
	const std::future<int> future = future_promise.get_future();
	future_promise.set_value(42);

	std::promise<int> shared_promise;
	const std::shared_future<int> shared_future = shared_promise.get_future().share();
	shared_promise.set_value(7);

	const auto future_is_ready = ket::concurrency::IsReady(future);
	const auto shared_future_is_ready = ket::concurrency::IsReady(shared_future);

	EXPECT_TRUE(future_is_ready);
	EXPECT_TRUE(shared_future_is_ready);
}

/**
 * @test
 * @brief 未完了futureのtimeout判定確認。
 * @details 値未設定のvalidなstd::futureを渡し、IsReadyがfalseを返すことを確認。
 * @pre C++17以降。対象futureはvalid。
 * @post futureの共有状態は保持。
 */
TEST(KetConcurrencyTest, ReturnsFalseForPendingFuture)
{
	std::promise<int> promise;
	const std::future<int> future = promise.get_future();
	std::promise<int> shared_promise;
	const std::shared_future<int> shared_future = shared_promise.get_future().share();

	const auto future_is_ready = ket::concurrency::IsReady(future);
	const auto shared_future_is_ready = ket::concurrency::IsReady(shared_future);

	EXPECT_FALSE(future_is_ready);
	EXPECT_FALSE(shared_future_is_ready);
}

/**
 * @test
 * @brief deferred futureの非ready確認。
 * @details std::launch::deferredで作ったfutureを渡し、IsReadyがfalseを返すことを確認。
 * @pre C++17以降。対象futureはvalid。
 * @post deferred callableは実行されない。
 */
TEST(KetConcurrencyTest, ReturnsFalseForDeferredFuture)
{
	const std::future<int> future = std::async(std::launch::deferred,
											   []()
											   {
												   return 42;
											   });
	const std::shared_future<int> shared_future = std::async(std::launch::deferred,
															 []()
															 {
																 return 7;
															 })
													  .share();

	const auto future_is_ready = ket::concurrency::IsReady(future);
	const auto shared_future_is_ready = ket::concurrency::IsReady(shared_future);

	EXPECT_FALSE(future_is_ready);
	EXPECT_FALSE(shared_future_is_ready);
}

/**
 * @test
 * @brief invalid future preconditionの確認。
 * @details invalid futureはIsReadyのprecondition違反であり、APIへ渡さないことをテストで明示。
 * @pre C++17以降。
 * @post invalid futureはinvalidなまま。
 */
TEST(KetConcurrencyTest, DocumentsInvalidFuturePrecondition)
{
	const std::future<int> invalid_future;

	const auto invalid_future_is_valid = invalid_future.valid();

	EXPECT_FALSE(invalid_future_is_valid);
}
