#pragma once

/**
 * @file ket_bytes.h
 * @brief 可変長byte payloadの構築API。
 *
 * @details `std::vector<std::uint8_t>`を所有するpayload builderと、既存vectorへの
 * endian固定幅追記を短いAPIへ集約する。ヘッダオンリーmoduleのため、drop-in時は
 * ヘッダ単体で持ち出す。protocol固有のschemaやchecksumは扱わず、byte列構築の小さい儀式だけを扱う。
 *
 * @par プロジェクトへの適用方法
 * `ket_bytes.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：`std::vector<std::uint8_t>`と`std::string_view`を利用でき、
 * payload構築を標準ライブラリのみで薄く包める。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::bytes
 * 内部実装：ket::bytes::detail
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

namespace ket
{
	namespace bytes
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 1byte値の末尾追記。
		 * @param[in,out] dst 追記先のbyte列。
		 * @param[in] value 追記する1byte値。
		 * @retval void 戻り値なし。
		 * @pre `dst`は有効なstd::vectorオブジェクト。
		 * @post `dst`の既存内容を保持し、末尾に`value`を1byte追加。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @code
		 * std::vector<std::uint8_t> bytes;
		 * ket::bytes::AppendU8(bytes, std::uint8_t{0x12U});
		 * // bytes == {0x12}
		 * @endcode
		 */
		inline void AppendU8(std::vector<std::uint8_t>& dst, std::uint8_t value);

		/**
		 * @brief 16bit値のbig-endian末尾追記。
		 * @param[in,out] dst 追記先のbyte列。
		 * @param[in] value 追記する16bit値。
		 * @retval void 戻り値なし。
		 * @pre `dst`は有効なstd::vectorオブジェクト。
		 * @post `dst`の既存内容を保持し、末尾に上位byteから2byte追加。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @code
		 * std::vector<std::uint8_t> bytes;
		 * ket::bytes::AppendBe16(bytes, std::uint16_t{0x1234U});
		 * // bytes == {0x12, 0x34}
		 * @endcode
		 */
		inline void AppendBe16(std::vector<std::uint8_t>& dst, std::uint16_t value);

		/**
		 * @brief 32bit値のbig-endian末尾追記。
		 * @param[in,out] dst 追記先のbyte列。
		 * @param[in] value 追記する32bit値。
		 * @retval void 戻り値なし。
		 * @pre `dst`は有効なstd::vectorオブジェクト。
		 * @post `dst`の既存内容を保持し、末尾に上位byteから4byte追加。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @code
		 * std::vector<std::uint8_t> bytes;
		 * ket::bytes::AppendBe32(bytes, std::uint32_t{0x12345678U});
		 * // bytes == {0x12, 0x34, 0x56, 0x78}
		 * @endcode
		 */
		inline void AppendBe32(std::vector<std::uint8_t>& dst, std::uint32_t value);

		/**
		 * @brief 16bit値のlittle-endian末尾追記。
		 * @param[in,out] dst 追記先のbyte列。
		 * @param[in] value 追記する16bit値。
		 * @retval void 戻り値なし。
		 * @pre `dst`は有効なstd::vectorオブジェクト。
		 * @post `dst`の既存内容を保持し、末尾に下位byteから2byte追加。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @code
		 * std::vector<std::uint8_t> bytes;
		 * ket::bytes::AppendLe16(bytes, std::uint16_t{0x1234U});
		 * // bytes == {0x34, 0x12}
		 * @endcode
		 */
		inline void AppendLe16(std::vector<std::uint8_t>& dst, std::uint16_t value);

		/**
		 * @brief 32bit値のlittle-endian末尾追記。
		 * @param[in,out] dst 追記先のbyte列。
		 * @param[in] value 追記する32bit値。
		 * @retval void 戻り値なし。
		 * @pre `dst`は有効なstd::vectorオブジェクト。
		 * @post `dst`の既存内容を保持し、末尾に下位byteから4byte追加。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @code
		 * std::vector<std::uint8_t> bytes;
		 * ket::bytes::AppendLe32(bytes, std::uint32_t{0x12345678U});
		 * // bytes == {0x78, 0x56, 0x34, 0x12}
		 * @endcode
		 */
		inline void AppendLe32(std::vector<std::uint8_t>& dst, std::uint32_t value);

		/**
		 * @brief 任意byte列の末尾追記。
		 * @param[in,out] dst 追記先のbyte列。
		 * @param[in] data 追記するbyte列の先頭。`size == 0`の場合だけnullptr可。
		 * @param[in] size 追記するbyte数。
		 * @retval void 戻り値なし。
		 * @pre `dst`は有効なstd::vectorオブジェクト。`data`は`size`バイト以上読み取り可能な
		 * 配列を指す。`Append(nullptr, 0)`はno-op、`Append(nullptr, size > 0)`はprecondition違反。
		 * @post `dst`の既存内容を保持し、`size`が0でなければ末尾に`data[0..size)`を同じ順序で追加。
		 * @note raw byte列を扱うC API境界に近い入力のため、nullable条件付きのポインタを使う。
		 * @note std::vectorの確保があるためnoexceptなし。
		 * @code
		 * const std::uint8_t data[] = {0x01U, 0x02U};
		 * std::vector<std::uint8_t> bytes{0xF0U};
		 * ket::bytes::Append(bytes, data, 2U);
		 * // bytes == {0xF0, 0x01, 0x02}
		 * @endcode
		 */
		inline void
		Append(std::vector<std::uint8_t>& dst, const std::uint8_t* data, std::size_t size);

		/**
		 * @brief 可変長byte payload builder。
		 * @note 内部bufferはstd::vector<std::uint8_t>で所有。
		 */
		class Builder
		{
		  public:
			/**
			 * @brief 空のbuilderを作る。
			 * @pre なし。
			 * @post `Buffer()`は空のbyte列を参照。
			 * @code
			 * ket::bytes::Builder builder;
			 * // builder.Buffer().empty() == true
			 * @endcode
			 */
			Builder() = default;

			/**
			 * @brief reserve済みbuilderを作る。
			 * @param[in] reserve_size 内部bufferへ事前確保するbyte数。
			 * @retval value reserve済みbuilder。
			 * @pre `reserve_size`はstd::vector<std::uint8_t>でreserve可能な値。
			 * @post `Buffer().capacity()`は`reserve_size`以上。
			 * @note std::vectorの確保があるためnoexceptなし。
			 * @code
			 * ket::bytes::Builder builder(8U);
			 * // builder.Buffer().capacity() >= 8
			 * @endcode
			 */
			explicit Builder(std::size_t reserve_size)
			{
				buffer_.reserve(reserve_size);
			}

			/**
			 * @brief 1byte値の末尾追記。
			 * @param[in] value 追記する1byte値。
			 * @retval *this 追記後のbuilder。
			 * @pre なし。
			 * @post 内部bufferの既存内容を保持し、末尾に`value`を1byte追加。
			 * @code
			 * ket::bytes::Builder builder;
			 * auto& same = builder.AppendU8(std::uint8_t{0xA5U});
			 * // &same == &builder
			 * // builder.Buffer() == {0xA5}
			 * @endcode
			 */
			Builder& AppendU8(std::uint8_t value)
			{
				ket::bytes::AppendU8(buffer_, value);
				return *this;
			}

			/**
			 * @brief 16bit値のbig-endian末尾追記。
			 * @param[in] value 追記する16bit値。
			 * @retval *this 追記後のbuilder。
			 * @pre なし。
			 * @post 内部bufferの既存内容を保持し、末尾に上位byteから2byte追加。
			 * @code
			 * ket::bytes::Builder builder;
			 * builder.AppendBe16(std::uint16_t{0x1234U});
			 * // builder.Buffer() == {0x12, 0x34}
			 * @endcode
			 */
			Builder& AppendBe16(std::uint16_t value)
			{
				ket::bytes::AppendBe16(buffer_, value);
				return *this;
			}

			/**
			 * @brief 32bit値のbig-endian末尾追記。
			 * @param[in] value 追記する32bit値。
			 * @retval *this 追記後のbuilder。
			 * @pre なし。
			 * @post 内部bufferの既存内容を保持し、末尾に上位byteから4byte追加。
			 * @code
			 * ket::bytes::Builder builder;
			 * builder.AppendBe32(std::uint32_t{0x12345678U});
			 * // builder.Buffer() == {0x12, 0x34, 0x56, 0x78}
			 * @endcode
			 */
			Builder& AppendBe32(std::uint32_t value)
			{
				ket::bytes::AppendBe32(buffer_, value);
				return *this;
			}

			/**
			 * @brief 16bit値のlittle-endian末尾追記。
			 * @param[in] value 追記する16bit値。
			 * @retval *this 追記後のbuilder。
			 * @pre なし。
			 * @post 内部bufferの既存内容を保持し、末尾に下位byteから2byte追加。
			 * @code
			 * ket::bytes::Builder builder;
			 * builder.AppendLe16(std::uint16_t{0x1234U});
			 * // builder.Buffer() == {0x34, 0x12}
			 * @endcode
			 */
			Builder& AppendLe16(std::uint16_t value)
			{
				ket::bytes::AppendLe16(buffer_, value);
				return *this;
			}

			/**
			 * @brief 32bit値のlittle-endian末尾追記。
			 * @param[in] value 追記する32bit値。
			 * @retval *this 追記後のbuilder。
			 * @pre なし。
			 * @post 内部bufferの既存内容を保持し、末尾に下位byteから4byte追加。
			 * @code
			 * ket::bytes::Builder builder;
			 * builder.AppendLe32(std::uint32_t{0x12345678U});
			 * // builder.Buffer() == {0x78, 0x56, 0x34, 0x12}
			 * @endcode
			 */
			Builder& AppendLe32(std::uint32_t value)
			{
				ket::bytes::AppendLe32(buffer_, value);
				return *this;
			}

			/**
			 * @brief 任意byte列の末尾追記。
			 * @param[in] data 追記するbyte列の先頭。`size == 0`の場合だけnullptr可。
			 * @param[in] size 追記するbyte数。
			 * @retval *this 追記後のbuilder。
			 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`Append(nullptr, 0)`は
			 * no-op、`Append(nullptr, size > 0)`はprecondition違反。
			 * @post 内部bufferの既存内容を保持し、`size`が0でなければ末尾に`data[0..size)`を
			 * 同じ順序で追加。
			 * @code
			 * const std::uint8_t data[] = {0x01U, 0x02U};
			 * ket::bytes::Builder builder;
			 * builder.Append(data, 2U);
			 * // builder.Buffer() == {0x01, 0x02}
			 * @endcode
			 */
			Builder& Append(const std::uint8_t* data, std::size_t size)
			{
				ket::bytes::Append(buffer_, data, size);
				return *this;
			}

			/**
			 * @brief ASCII byte列として扱う文字列片の末尾追記。
			 * @param[in] text 追記するASCII文字列片。
			 * @retval *this 追記後のbuilder。
			 * @pre `text`の各要素はASCII byte列。UTF-8 validationやencoding変換は行わない。
			 * @post 内部bufferの既存内容を保持し、末尾に`text`の各byteを同じ順序で追加。
			 * @note std::vectorの確保があるためnoexceptなし。
			 * @code
			 * ket::bytes::Builder builder;
			 * builder.AppendAscii("AZ");
			 * // builder.Buffer() == {0x41, 0x5A}
			 * @endcode
			 */
			Builder& AppendAscii(std::string_view text)
			{
				std::transform(text.begin(),
							   text.end(),
							   std::back_inserter(buffer_),
							   [](char value)
							   {
								   return static_cast<std::uint8_t>(value);
							   });

				return *this;
			}

			/**
			 * @brief 構築途中bufferの参照取得。
			 * @retval value 内部bufferへのconst参照。
			 * @pre なし。
			 * @post builderと内部bufferの変更なし。
			 * @code
			 * ket::bytes::Builder builder;
			 * builder.AppendU8(std::uint8_t{0xA5U});
			 * const auto& buffer = builder.Buffer();
			 * // buffer == {0xA5}
			 * @endcode
			 */
			[[nodiscard]] const std::vector<std::uint8_t>& Buffer() const noexcept
			{
				return buffer_;
			}

			/**
			 * @brief 構築済みbyte列のmove取得。
			 * @retval value Build呼び出し時点の内部buffer内容。
			 * @pre rvalueのbuilderに対して呼び出す。
			 * @post 戻り値へ内部bufferをmove。対象builderはvalidだが内部buffer状態は規定しない。
			 * @code
			 * ket::bytes::Builder builder;
			 * builder.AppendU8(std::uint8_t{0xA5U});
			 * auto payload = std::move(builder).Build();
			 * // payload == {0xA5}
			 * @endcode
			 */
			std::vector<std::uint8_t> Build() &&
			{
				return std::move(buffer_);
			}

			/**
			 * @brief 内部bufferの全要素削除。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post `Buffer().empty()`はtrue。内部bufferのcapacityはstd::vector::clearに従い保持。
			 * @code
			 * ket::bytes::Builder builder;
			 * builder.AppendU8(std::uint8_t{0xA5U});
			 * builder.Clear();
			 * // builder.Buffer().empty() == true
			 * @endcode
			 */
			void Clear() noexcept
			{
				buffer_.clear();
			}

		  private:
			std::vector<std::uint8_t> buffer_;
		};

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief 指定shift位置の1byte取得。
			 * @param[in] value 取得対象の32bit値。
			 * @param[in] shift 右shiftするbit数。
			 * @retval value `shift`位置の下位8bit。
			 * @pre `shift`は0、8、16、24のいずれか。
			 * @post 引数と外部状態の変更なし。
			 */
			constexpr std::uint8_t ByteAt(std::uint32_t value, unsigned shift) noexcept
			{
				return static_cast<std::uint8_t>((value >> shift) & 0xFFU);
			}

			/**
			 * @brief 任意byte列の末尾追記実体。
			 * @param[in,out] dst 追記先のbyte列。
			 * @param[in] data 追記するbyte列の先頭。`size == 0`の場合だけnullptr可。
			 * @param[in] size 追記するbyte数。
			 * @retval void 戻り値なし。
			 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`size == 0`ではno-op。
			 * @post `dst`の既存内容を保持し、`size`が0でなければ末尾にbyte列を追加。
			 */
			inline void
			AppendBytes(std::vector<std::uint8_t>& dst, const std::uint8_t* data, std::size_t size)
			{
				const auto size_is_zero = size == 0U;
				if (size_is_zero)
				{
					return;
				}

				dst.insert(dst.end(), data, data + size);
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		inline void AppendU8(std::vector<std::uint8_t>& dst, std::uint8_t value)
		{
			dst.push_back(value);
		}

		inline void AppendBe16(std::vector<std::uint8_t>& dst, std::uint16_t value)
		{
			dst.push_back(detail::ByteAt(value, 8U));
			dst.push_back(detail::ByteAt(value, 0U));
		}

		inline void AppendBe32(std::vector<std::uint8_t>& dst, std::uint32_t value)
		{
			dst.push_back(detail::ByteAt(value, 24U));
			dst.push_back(detail::ByteAt(value, 16U));
			dst.push_back(detail::ByteAt(value, 8U));
			dst.push_back(detail::ByteAt(value, 0U));
		}

		inline void AppendLe16(std::vector<std::uint8_t>& dst, std::uint16_t value)
		{
			dst.push_back(detail::ByteAt(value, 0U));
			dst.push_back(detail::ByteAt(value, 8U));
		}

		inline void AppendLe32(std::vector<std::uint8_t>& dst, std::uint32_t value)
		{
			dst.push_back(detail::ByteAt(value, 0U));
			dst.push_back(detail::ByteAt(value, 8U));
			dst.push_back(detail::ByteAt(value, 16U));
			dst.push_back(detail::ByteAt(value, 24U));
		}

		inline void
		Append(std::vector<std::uint8_t>& dst, const std::uint8_t* data, std::size_t size)
		{
			detail::AppendBytes(dst, data, size);
		}

	} // namespace bytes

} // namespace ket
