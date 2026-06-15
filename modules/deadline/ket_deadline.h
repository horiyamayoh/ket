#pragma once

/**
 * @file ket_deadline.h
 * @brief steady_clockベースの経過時間と期限判定API。
 *
 * @details timeout、deadline、elapsed timeを`std::chrono::steady_clock`へ限定し、
 * `system_clock`との混在を避ける。drop-in時はヘッダと実装を同じ単位で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_deadline.h` と `ket_deadline.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：`std::chrono::steady_clock` と timeout の扱いを明示し、`system_clock`混在を避けられる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::deadline
 * 内部実装：ket::deadline::detail
 */

#include <chrono> // IWYU pragma: export
// IWYU pragma: no_include <bits/chrono.h>

namespace ket
{
	namespace deadline
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief steady_clockで経過時間を測る小さいstopwatch。
		 */
		class Stopwatch
		{
		  public:
			/**
			 * @brief 現在時刻を開始点にしたstopwatch生成。
			 * @retval value 開始点を現在の`std::chrono::steady_clock::now()`にしたstopwatch。
			 * @pre なし。
			 * @post 外部状態の変更なし。
			 */
			static Stopwatch StartNew() noexcept;

			/**
			 * @brief 開始点を現在時刻へ戻す。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post 以後の`Elapsed()`はrestart後の経過時間を返す。
			 */
			void Restart() noexcept;

			/**
			 * @brief 開始点から現在までの経過時間取得。
			 * @retval value `std::chrono::steady_clock::duration`で表す経過時間。
			 * @pre `StartNew()`で生成済みのobject。
			 * @post 引数と外部状態の変更なし。
			 */
			std::chrono::steady_clock::duration Elapsed() const noexcept; // NOLINT

			/**
			 * @brief 開始点から現在までの経過時間をmilliseconds単位で取得。
			 * @retval value `std::chrono::milliseconds`へ切り捨て変換した経過時間。
			 * @pre `StartNew()`で生成済みのobject。
			 * @post 引数と外部状態の変更なし。
			 */
			std::chrono::milliseconds ElapsedMilliseconds() const noexcept; // NOLINT

		  private:
			/**
			 * @brief 開始点を指定したstopwatch生成。
			 * @param[in] started_at 保持する開始時刻。
			 * @retval value `started_at`を開始点にしたstopwatch。
			 * @pre `started_at`は`std::chrono::steady_clock`由来の時刻。
			 * @post 引数と外部状態の変更なし。
			 */
			explicit Stopwatch(std::chrono::steady_clock::time_point started_at) noexcept;

			std::chrono::steady_clock::time_point started_at_;
		};

		/**
		 * @brief steady_clockで表す期限時刻。
		 */
		class Deadline
		{
		  public:
			/**
			 * @brief 現在時刻からtimeout後のdeadline生成。
			 * @param[in] timeout 現在時刻へ加算する`steady_clock` duration。
			 * @retval value timeout後のdeadline。zero以下のtimeoutは即時期限切れ。
			 * @pre なし。負timeoutは即時期限切れとして扱う。
			 * @post 引数と外部状態の変更なし。
			 */
			static Deadline After(std::chrono::steady_clock::duration timeout) noexcept;

			/**
			 * @brief 指定したsteady_clock時刻のdeadline生成。
			 * @param[in] time_point 期限として保持する`steady_clock`時刻。
			 * @retval value `time_point`を期限にしたdeadline。
			 * @pre `time_point`は`std::chrono::steady_clock`由来の時刻。
			 * @post 引数と外部状態の変更なし。
			 */
			static Deadline At(std::chrono::steady_clock::time_point time_point) noexcept;

			/**
			 * @brief 期限切れ判定。
			 * @retval true 現在時刻がdeadline以上。
			 * @retval false 現在時刻がdeadline未満。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			bool Expired() const noexcept; // NOLINT

			/**
			 * @brief 期限までの残り時間取得。
			 * @retval value 現在時刻からdeadlineまでのduration。
			 * @retval zero 期限切れ、または期限ちょうど。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。戻り値は負にならない。
			 */
			std::chrono::steady_clock::duration Remaining() const noexcept; // NOLINT

			/**
			 * @brief 保持している期限時刻取得。
			 * @retval value `std::chrono::steady_clock::time_point`の期限時刻。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 */
			std::chrono::steady_clock::time_point TimePoint() const noexcept; // NOLINT

		  private:
			/**
			 * @brief 期限時刻を指定したdeadline生成。
			 * @param[in] time_point 保持する期限時刻。
			 * @retval value `time_point`を期限にしたdeadline。
			 * @pre `time_point`は`std::chrono::steady_clock`由来の時刻。
			 * @post 引数と外部状態の変更なし。
			 */
			explicit Deadline(std::chrono::steady_clock::time_point time_point) noexcept;

			std::chrono::steady_clock::time_point time_point_;
		};

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

	} // namespace deadline

} // namespace ket
