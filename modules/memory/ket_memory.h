#pragma once

/**
 * @file ket_memory.h
 * @brief alignmentとobject representation読み取りの補助API。
 *
 * @details pointer alignmentの判定、aligned pointer計算、byte zeroing、object representationの
 * 読み取り入口を小さいAPIへ集約する。object lifetime操作、type punning、serializationには
 * 踏み込まない。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_memory.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：pointer alignmentとobject representationの意図を小さいAPIへ分離できる。
 * `std::uintptr_t`を提供し、byte address alignmentを整数下位bitで判定できる処理系を前提とする。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。pointer alignment APIは`std::uintptr_t`を使用する。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::memory
 * 内部実装：ket::memory::detail
 */

#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <type_traits>

namespace ket
{
	namespace memory
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief pointerが指定alignment境界上にあるかの判定。
		 * @param[in] ptr 判定対象のpointer。
		 * @param[in] alignment 要求alignment。power-of-twoだけを有効値として扱う。
		 * @retval true `ptr`が非nullで、`alignment`がpower-of-twoかつ境界上。
		 * @retval false `nullptr`、`alignment == 0`、非power-of-two、または境界外。
		 * @pre なし。null pointerと不正alignmentはfalseとして扱う。
		 * @post 引数と外部状態の変更なし。
		 * @note pointer値は`std::uintptr_t`へ変換して判定する。
		 * @code
		 * alignas(16) unsigned char buffer[16] = {};
		 * const auto aligned = ket::memory::IsAligned(buffer, 16U);
		 * // aligned == true
		 * @endcode
		 */
		inline bool IsAligned(const void* ptr, std::size_t alignment) noexcept;

		/**
		 * @brief pointerを指定alignment境界へ切り上げる。
		 * @param[in] ptr 計算対象のpointer。
		 * @param[in] alignment 要求alignment。power-of-twoだけを有効値として扱う。
		 * @param[out] out 成功時のaligned pointer。失敗時は変更なし。
		 * @retval true `ptr`が非nullで、`alignment`がpower-of-twoかつ切り上げ結果を表現可能。
		 * @retval false `nullptr`、`alignment == 0`、非power-of-two、またはaddress計算overflow。
		 * @pre `out`は有効な参照。null pointerと不正alignmentは失敗値として扱う。
		 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
		 * @note pointer値は`std::uintptr_t`へ変換し、結果を`const void*`へ戻す。
		 * 返されたpointerのobject bounds、lifetime、provenance、dereference可能性は保証しない。
		 * @code
		 * alignas(8) unsigned char buffer[16] = {};
		 * const void* out = nullptr;
		 * const auto ok = ket::memory::TryAlignUp(buffer + 1U, 8U, out);
		 * // ok == true, out == buffer + 8
		 * @endcode
		 */
		inline bool TryAlignUp(const void* ptr, std::size_t alignment, const void*& out) noexcept;

		/**
		 * @brief mutable pointerを指定alignment境界へ切り上げる。
		 * @param[in] ptr 計算対象のmutable pointer。
		 * @param[in] alignment 要求alignment。power-of-twoだけを有効値として扱う。
		 * @param[out] out 成功時のaligned mutable pointer。失敗時は変更なし。
		 * @retval true `ptr`が非nullで、`alignment`がpower-of-twoかつ切り上げ結果を表現可能。
		 * @retval false `nullptr`、`alignment == 0`、非power-of-two、またはaddress計算overflow。
		 * @pre `out`は有効な参照。null pointerと不正alignmentは失敗値として扱う。
		 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
		 * @note pointer値は`std::uintptr_t`へ変換し、結果を`void*`へ戻す。
		 * 返されたpointerのobject bounds、lifetime、provenance、dereference可能性は保証しない。
		 * @code
		 * alignas(8) unsigned char buffer[16] = {};
		 * void* out = nullptr;
		 * const auto ok = ket::memory::TryAlignUp(buffer + 1U, 8U, out);
		 * // ok == true, out == buffer + 8
		 * @endcode
		 */
		inline bool TryAlignUp(void* ptr, std::size_t alignment, void*& out) noexcept;

		/**
		 * @brief object pointerを指定alignment境界へ切り上げ、入力pointer型で返す。
		 * @tparam T 計算対象pointerのpointee型。object型またはcv修飾voidだけを受け付ける。
		 * @param[in] ptr 計算対象のtyped pointer。
		 * @param[in] alignment 要求alignment。power-of-twoだけを有効値として扱う。
		 * @param[out] out 成功時のaligned typed pointer。失敗時は変更なし。
		 * @retval true `ptr`が非nullで、`alignment`がpower-of-twoかつ切り上げ結果を表現可能。
		 * @retval false `nullptr`、`alignment == 0`、非power-of-two、またはaddress計算overflow。
		 * @pre `out`は有効な参照。null pointerと不正alignmentは失敗値として扱う。
		 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
		 * @note pointer値は`std::uintptr_t`へ変換し、結果を`T*`へ戻す。
		 * 返されたpointerのobject bounds、lifetime、provenance、dereference可能性は保証しない。
		 * @code
		 * alignas(8) unsigned char buffer[16] = {};
		 * unsigned char* out = nullptr;
		 * const auto ok = ket::memory::TryAlignUp(buffer + 1U, 8U, out);
		 * // ok == true, out == buffer + 8
		 * @endcode
		 */
		template <typename T>
		bool TryAlignUp(T* ptr, std::size_t alignment, T*& out) noexcept;

		/**
		 * @brief pointerを指定alignment境界へ切り下げる。
		 * @param[in] ptr 計算対象のpointer。
		 * @param[in] alignment 要求alignment。power-of-twoだけを有効値として扱う。
		 * @param[out] out 成功時のaligned pointer。失敗時は変更なし。
		 * @retval true `ptr`が非nullで、`alignment`がpower-of-two。
		 * @retval false `nullptr`、`alignment == 0`、または非power-of-two。
		 * @pre `out`は有効な参照。null pointerと不正alignmentは失敗値として扱う。
		 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
		 * @note pointer値は`std::uintptr_t`へ変換し、結果を`const void*`へ戻す。
		 * 返されたpointerのobject bounds、lifetime、provenance、dereference可能性は保証しない。
		 * @code
		 * alignas(8) unsigned char buffer[16] = {};
		 * const void* out = nullptr;
		 * const auto ok = ket::memory::TryAlignDown(buffer + 7U, 8U, out);
		 * // ok == true, out == buffer
		 * @endcode
		 */
		inline bool TryAlignDown(const void* ptr, std::size_t alignment, const void*& out) noexcept;

		/**
		 * @brief mutable pointerを指定alignment境界へ切り下げる。
		 * @param[in] ptr 計算対象のmutable pointer。
		 * @param[in] alignment 要求alignment。power-of-twoだけを有効値として扱う。
		 * @param[out] out 成功時のaligned mutable pointer。失敗時は変更なし。
		 * @retval true `ptr`が非nullで、`alignment`がpower-of-two。
		 * @retval false `nullptr`、`alignment == 0`、または非power-of-two。
		 * @pre `out`は有効な参照。null pointerと不正alignmentは失敗値として扱う。
		 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
		 * @note pointer値は`std::uintptr_t`へ変換し、結果を`void*`へ戻す。
		 * 返されたpointerのobject bounds、lifetime、provenance、dereference可能性は保証しない。
		 * @code
		 * alignas(8) unsigned char buffer[16] = {};
		 * void* out = nullptr;
		 * const auto ok = ket::memory::TryAlignDown(buffer + 7U, 8U, out);
		 * // ok == true, out == buffer
		 * @endcode
		 */
		inline bool TryAlignDown(void* ptr, std::size_t alignment, void*& out) noexcept;

		/**
		 * @brief object pointerを指定alignment境界へ切り下げ、入力pointer型で返す。
		 * @tparam T 計算対象pointerのpointee型。object型またはcv修飾voidだけを受け付ける。
		 * @param[in] ptr 計算対象のtyped pointer。
		 * @param[in] alignment 要求alignment。power-of-twoだけを有効値として扱う。
		 * @param[out] out 成功時のaligned typed pointer。失敗時は変更なし。
		 * @retval true `ptr`が非nullで、`alignment`がpower-of-two。
		 * @retval false `nullptr`、`alignment == 0`、または非power-of-two。
		 * @pre `out`は有効な参照。null pointerと不正alignmentは失敗値として扱う。
		 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
		 * @note pointer値は`std::uintptr_t`へ変換し、結果を`T*`へ戻す。
		 * 返されたpointerのobject bounds、lifetime、provenance、dereference可能性は保証しない。
		 * @code
		 * alignas(8) unsigned char buffer[16] = {};
		 * unsigned char* out = nullptr;
		 * const auto ok = ket::memory::TryAlignDown(buffer + 7U, 8U, out);
		 * // ok == true, out == buffer
		 * @endcode
		 */
		template <typename T>
		bool TryAlignDown(T* ptr, std::size_t alignment, T*& out) noexcept;

		/**
		 * @brief 指定byte範囲の通常zeroing。
		 * @param[in,out] ptr zeroing対象の先頭pointer。
		 * @param[in] size zeroing対象のbyte数。
		 * @retval void 戻り値なし。
		 * @pre `size > 0`の場合、`ptr`は`size`バイト以上書き込み可能な領域を指す。
		 * `ptr == nullptr && size > 0`はprecondition違反。
		 * @post `ptr != nullptr && size > 0`の場合、対象範囲のbyteを0へ変更。
		 * `ptr == nullptr && size == 0`はno-op。
		 * @note `std::memset`による通常のzeroing。最適化除去への対策は`SecureZero`を使用。
		 * `ptr == nullptr && size > 0`は呼び出し側の契約違反で、`std::terminate`を呼ぶ。
		 * @code
		 * unsigned char bytes[4] = {1U, 2U, 3U, 4U};
		 * ket::memory::Zero(bytes, 4U);
		 * // bytes == {0, 0, 0, 0}
		 * @endcode
		 */
		inline void Zero(void* ptr, std::size_t size) noexcept;

		/**
		 * @brief 指定byte範囲のbest-effort secure zeroing。
		 * @param[in,out] ptr zeroing対象の先頭pointer。
		 * @param[in] size zeroing対象のbyte数。
		 * @retval void 戻り値なし。
		 * @pre `size > 0`の場合、`ptr`は`size`バイト以上書き込み可能な領域を指す。
		 * `ptr == nullptr && size > 0`はprecondition違反。
		 * @post `ptr != nullptr && size > 0`の場合、対象範囲のbyteを0へ変更。
		 * `ptr == nullptr && size == 0`はno-op。
		 * @note `volatile unsigned char*`経由のwriteで最適化除去を防ぐbest-effort。
		 * `ptr == nullptr && size > 0`は呼び出し側の契約違反で、`std::terminate`を呼ぶ。
		 * 暗号学的な完全消去保証なし。
		 * @code
		 * unsigned char secret[4] = {1U, 2U, 3U, 4U};
		 * ket::memory::SecureZero(secret, 4U);
		 * // secret == {0, 0, 0, 0}
		 * @endcode
		 */
		inline void SecureZero(void* ptr, std::size_t size) noexcept;

		/**
		 * @brief trivially copyable objectのobject representation先頭pointer取得。
		 * @tparam T 読み取り対象objectの型。trivially copyable型だけを受け付ける。
		 * @param[in] object 読み取り対象object。
		 * @retval value `object`のobject representation先頭を指す`const unsigned char*`。
		 * @pre `T`はtrivially copyable。`object`のlifetime中だけ戻り値を利用可能。
		 * @post 引数と外部状態の変更なし。object representationを読む入口だけを提供。
		 * @note object lifetime操作、placement new、type punningは行わない。
		 * padding、endianness、layoutを含む処理系依存のbyte列であり、serializationや安定比較には使わない。
		 * @code
		 * const auto value = std::uint32_t{0x01020304U};
		 * const auto* bytes = ket::memory::ObjectBytes(value);
		 * const auto size = ket::memory::ObjectByteSize(value);
		 * @endcode
		 */
		template <typename T>
		const unsigned char* ObjectBytes(const T& object) noexcept;

		/**
		 * @brief trivially copyable objectのobject representation byte数取得。
		 * @tparam T 対象objectの型。trivially copyable型だけを受け付ける。
		 * @param[in] object byte数取得対象object。
		 * @retval value `sizeof(T)`。
		 * @pre `T`はtrivially copyable。
		 * @post 引数と外部状態の変更なし。
		 * @note 引数は型推論のために受け取り、値の読み取りなし。
		 * byte列の意味はpadding、endianness、layoutに依存する。
		 * @code
		 * const auto value = std::uint32_t{0x01020304U};
		 * const auto size = ket::memory::ObjectByteSize(value);
		 * // size == sizeof(std::uint32_t)
		 * @endcode
		 */
		template <typename T>
		constexpr std::size_t ObjectByteSize(const T& object) noexcept;

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief power-of-two判定。
			 * @param[in] value 判定対象値。
			 * @retval true `value`が0以外のpower-of-two。
			 * @retval false 0、またはpower-of-twoではない値。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline bool IsPowerOfTwo(std::size_t value) noexcept
			{
				const auto value_is_zero = value == 0U;
				if (value_is_zero)
				{
					return false;
				}

				const auto mask = value - std::size_t{1U};
				return (value & mask) == 0U;
			}

			/**
			 * @brief alignment値がaddress演算型へ収まるかの判定。
			 * @param[in] alignment 判定対象alignment。
			 * @retval true `std::uintptr_t`で表現可能。
			 * @retval false `std::uintptr_t`で表現不可。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline bool AlignmentFitsAddress(std::size_t alignment) noexcept
			{
				const auto max_address = (std::numeric_limits<std::uintptr_t>::max)();
				const auto max_alignment = static_cast<std::size_t>(max_address);
				return alignment <= max_alignment;
			}

			/**
			 * @brief pointer alignment演算に使える入力かの判定。
			 * @param[in] ptr 判定対象pointer。
			 * @param[in] alignment 判定対象alignment。
			 * @retval true 非null pointer、power-of-two
			 * alignment、かつaddress演算型へ収まるalignment。
			 * @retval false null pointer、不正alignment、または表現不可alignment。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline bool CanAlignPointer(const void* ptr, std::size_t alignment) noexcept
			{
				const auto ptr_exists = ptr != nullptr;
				const auto alignment_is_power_of_two = IsPowerOfTwo(alignment);
				const auto alignment_fits_address = AlignmentFitsAddress(alignment);
				return ptr_exists && alignment_is_power_of_two && alignment_fits_address;
			}

			/**
			 * @brief pointer値のaddress整数変換。
			 * @param[in] ptr 変換対象pointer。
			 * @retval value `ptr`のaddress表現。
			 * @pre `ptr != nullptr`。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline std::uintptr_t PointerToAddress(const void* ptr) noexcept
			{
				return reinterpret_cast<std::uintptr_t>(ptr);
			}

			/**
			 * @brief pointerのalignment切り上げaddress計算。
			 * @param[in] ptr 計算対象pointer。
			 * @param[in] alignment 要求alignment。
			 * @param[out] out 成功時のaligned address。失敗時は変更なし。
			 * @retval true 入力がalignment計算可能で、切り上げ結果を表現可能。
			 * @retval false null pointer、不正alignment、またはaddress計算overflow。
			 * @pre `out`は有効な参照。
			 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline bool
			TryAlignUpAddress(const void* ptr, std::size_t alignment, std::uintptr_t& out) noexcept
			{
				const auto input_can_be_aligned = CanAlignPointer(ptr, alignment);
				if (!input_can_be_aligned)
				{
					return false;
				}

				const auto address = PointerToAddress(ptr);
				const auto alignment_value = static_cast<std::uintptr_t>(alignment);
				const auto mask = alignment_value - std::uintptr_t{1U};
				const auto max_address = (std::numeric_limits<std::uintptr_t>::max)();
				const auto address_overflows = address > (max_address - mask);
				if (address_overflows)
				{
					return false;
				}

				out = (address + mask) & ~mask;
				return true;
			}

			/**
			 * @brief pointerのalignment切り下げaddress計算。
			 * @param[in] ptr 計算対象pointer。
			 * @param[in] alignment 要求alignment。
			 * @param[out] out 成功時のaligned address。失敗時は変更なし。
			 * @retval true 入力がalignment計算可能。
			 * @retval false null pointer、または不正alignment。
			 * @pre `out`は有効な参照。
			 * @post 成功時だけ`out`を更新。失敗時は`out`と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline bool TryAlignDownAddress(const void* ptr,
											std::size_t alignment,
											std::uintptr_t& out) noexcept
			{
				const auto input_can_be_aligned = CanAlignPointer(ptr, alignment);
				if (!input_can_be_aligned)
				{
					return false;
				}

				const auto address = PointerToAddress(ptr);
				const auto alignment_value = static_cast<std::uintptr_t>(alignment);
				const auto mask = alignment_value - std::uintptr_t{1U};
				out = address & ~mask;
				return true;
			}

			/**
			 * @brief address整数のpointer変換。
			 * @param[in] address 変換対象address。
			 * @retval value `address`の`const void*`表現。
			 * @pre `address`はpointer値から得たaddressと同じaddress空間の値。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline const void* AddressToPointer(std::uintptr_t address) noexcept
			{
				return reinterpret_cast<const void*>(address); // NOLINT(performance-no-int-to-ptr)
			}

			/**
			 * @brief address整数のmutable pointer変換。
			 * @param[in] address 変換対象address。
			 * @retval value `address`の`void*`表現。
			 * @pre `address`はpointer値から得たaddressと同じaddress空間の値。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			inline void* AddressToMutablePointer(std::uintptr_t address) noexcept
			{
				return reinterpret_cast<void*>(address); // NOLINT(performance-no-int-to-ptr)
			}

			/**
			 * @brief address整数のtyped pointer変換。
			 * @tparam T 変換後pointerのpointee型。
			 * @param[in] address 変換対象address。
			 * @retval value `address`の`T*`表現。
			 * @pre `address`はpointer値から得たaddressと同じaddress空間の値。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename T>
			T* AddressToTypedPointer(std::uintptr_t address) noexcept
			{
				return reinterpret_cast<T*>(address); // NOLINT(performance-no-int-to-ptr)
			}

			/**
			 * @brief typed pointer alignment対象型の判定。
			 * @tparam T 判定対象pointee型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct IsPointerAlignableTarget
			{
				using Unqualified =
					typename std::remove_cv<T>::type; // NOLINT(modernize-type-traits)
				static const bool kValue =
					std::is_object<Unqualified>() || std::is_void<Unqualified>();
			};

			/**
			 * @brief object representation読み取り対象型の判定。
			 * @tparam T 判定対象型。
			 * @note detail配下の型は公開APIではない。
			 */
			template <typename T>
			struct IsObjectByteReadable
			{
				static const bool kValue = std::is_trivially_copyable<T>();
			};

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		inline bool IsAligned(const void* ptr, std::size_t alignment) noexcept
		{
			const auto input_can_be_checked = detail::CanAlignPointer(ptr, alignment);
			if (!input_can_be_checked)
			{
				return false;
			}

			const auto address = detail::PointerToAddress(ptr);
			const auto alignment_value = static_cast<std::uintptr_t>(alignment);
			const auto mask = alignment_value - std::uintptr_t{1U};

			return (address & mask) == std::uintptr_t{0U};
		}

		inline bool TryAlignUp(const void* ptr, std::size_t alignment, const void*& out) noexcept
		{
			std::uintptr_t aligned_address = 0U;
			const auto address_aligned = detail::TryAlignUpAddress(ptr, alignment, aligned_address);
			if (!address_aligned)
			{
				return false;
			}

			out = detail::AddressToPointer(aligned_address);
			return true;
		}

		// cppcheck-suppress constParameterPointer
		inline bool TryAlignUp(void* ptr, std::size_t alignment, void*& out) noexcept
		{
			std::uintptr_t aligned_address = 0U;
			const auto address_aligned = detail::TryAlignUpAddress(ptr, alignment, aligned_address);
			if (!address_aligned)
			{
				return false;
			}

			out = detail::AddressToMutablePointer(aligned_address);
			return true;
		}

		// cppcheck-suppress-begin constParameterPointer
		template <typename T>
		bool TryAlignUp(T* ptr, std::size_t alignment, T*& out) noexcept
		{
			static_assert(detail::IsPointerAlignableTarget<T>::kValue,
						  "ket::memory::TryAlignUp requires object pointer or void pointer.");

			std::uintptr_t aligned_address = 0U;
			const auto address_aligned = detail::TryAlignUpAddress(ptr, alignment, aligned_address);
			if (!address_aligned)
			{
				return false;
			}

			out = detail::AddressToTypedPointer<T>(aligned_address);
			return true;
		}
		// cppcheck-suppress-end constParameterPointer

		inline bool TryAlignDown(const void* ptr, std::size_t alignment, const void*& out) noexcept
		{
			std::uintptr_t aligned_address = 0U;
			const auto address_aligned =
				detail::TryAlignDownAddress(ptr, alignment, aligned_address);
			if (!address_aligned)
			{
				return false;
			}

			out = detail::AddressToPointer(aligned_address);
			return true;
		}

		// cppcheck-suppress constParameterPointer
		inline bool TryAlignDown(void* ptr, std::size_t alignment, void*& out) noexcept
		{
			std::uintptr_t aligned_address = 0U;
			const auto address_aligned =
				detail::TryAlignDownAddress(ptr, alignment, aligned_address);
			if (!address_aligned)
			{
				return false;
			}

			out = detail::AddressToMutablePointer(aligned_address);
			return true;
		}

		// cppcheck-suppress-begin constParameterPointer
		template <typename T>
		bool TryAlignDown(T* ptr, std::size_t alignment, T*& out) noexcept
		{
			static_assert(detail::IsPointerAlignableTarget<T>::kValue,
						  "ket::memory::TryAlignDown requires object pointer or void pointer.");

			std::uintptr_t aligned_address = 0U;
			const auto address_aligned =
				detail::TryAlignDownAddress(ptr, alignment, aligned_address);
			if (!address_aligned)
			{
				return false;
			}

			out = detail::AddressToTypedPointer<T>(aligned_address);
			return true;
		}
		// cppcheck-suppress-end constParameterPointer

		inline void Zero(void* ptr, std::size_t size) noexcept
		{
			const auto size_is_zero = size == 0U;
			if (size_is_zero)
			{
				return;
			}

			const auto ptr_is_null = ptr == nullptr;
			if (ptr_is_null)
			{
				std::terminate();
				return;
			}

			std::memset(ptr, 0, size);
		}

		inline void SecureZero(void* ptr, std::size_t size) noexcept
		{
			const auto size_is_zero = size == 0U;
			if (size_is_zero)
			{
				return;
			}

			const auto ptr_is_null = ptr == nullptr;
			if (ptr_is_null)
			{
				std::terminate();
				return;
			}

			auto* const bytes = static_cast<volatile unsigned char*>(ptr);
			for (std::size_t index = 0U; index < size; ++index)
			{
				bytes[index] = static_cast<unsigned char>(0U);
			}
		}

		template <typename T>
		const unsigned char* ObjectBytes(const T& object) noexcept
		{
			static_assert(detail::IsObjectByteReadable<T>::kValue,
						  "ket::memory::ObjectBytes requires trivially copyable T.");

			return reinterpret_cast<const unsigned char*>(&object);
		}

		template <typename T>
		constexpr std::size_t ObjectByteSize(const T& object) noexcept
		{
			static_assert(detail::IsObjectByteReadable<T>::kValue,
						  "ket::memory::ObjectByteSize requires trivially copyable T.");

			return sizeof(object);
		}

	} // namespace memory

} // namespace ket
