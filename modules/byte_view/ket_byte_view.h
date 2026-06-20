#pragma once

/**
 * @file ket_byte_view.h
 * @brief non-owning byte span API。
 *
 * @details C++11からC++17までの環境で、所有権を持たないbyte列viewと境界確認付きaccessを
 * 小さく表現する。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で持ち出す。
 * `nullptr + 0`は空view、`nullptr + 非0`はinvalid viewとして扱う。
 *
 * @par プロジェクトへの適用方法
 * `ket_byte_view.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11〜17。
 * 推奨理由：C++17以前でnon-owning byte spanの寿命と境界確認を標準ライブラリのみで薄く包める。
 * 本ライブラリの適用を推奨しない C++ バージョン：C++20以降。
 * 非推奨理由：C++20以降は標準ライブラリの`std::span`で容易かつ明確に代替可能。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::byte_view
 * 内部実装：ket::byte_view::detail
 */

#include <cstddef>
#include <cstdint>

namespace ket
{
	namespace byte_view
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 読み取り専用のnon-owning byte view。
		 * @note viewは元bufferを所有せず、利用者がbuffer lifetimeを保持。
		 */
		class View
		{
		  public:
			/**
			 * @brief 空の読み取り専用byte view構築。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post `Data() == nullptr`、`Size() == 0`、`Empty() == true`。
			 * @code
			 * ket::byte_view::View view;
			 * // view.Data() == nullptr
			 * // view.Size() == 0
			 * // view.Empty() == true
			 * @endcode
			 */
			View() noexcept : data_(nullptr), size_(0U) {}

			/**
			 * @brief 読み取り専用byte列を参照するview構築。
			 * @param[in] data 参照対象bufferの先頭。`nullptr`は`size == 0`の場合のみ空view。
			 * @param[in] size 参照対象bufferのbyte数。
			 * @retval void 戻り値なし。
			 * @pre `data`は`size`byte以上読み取り可能な配列を指す。`nullptr + 非0`はinvalid
			 * viewとして保持。
			 * @post `data`と`size`を保持し、bufferの所有権は取得なし。
			 * @code
			 * const std::uint8_t data[] = {0x10U, 0x20U};
			 * ket::byte_view::View view(data, 2U);
			 * // view.Data() == data
			 * // view.Size() == 2
			 * @endcode
			 */
			View(const std::uint8_t* data, std::size_t size) noexcept : data_(data), size_(size) {}

			/**
			 * @brief 参照対象buffer先頭の取得。
			 * @retval value 構築時に渡されたbuffer先頭。空またはinvalid
			 * viewでは`nullptr`もあり得る。
			 * @pre なし。
			 * @post viewと外部状態の変更なし。
			 * @code
			 * const std::uint8_t data[] = {0x10U};
			 * ket::byte_view::View view(data, 1U);
			 * const auto pointer = view.Data();
			 * // pointer == data
			 * @endcode
			 */
			const std::uint8_t* Data() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 参照対象buffer byte数の取得。
			 * @retval value 構築時に渡されたbyte数。
			 * @pre なし。
			 * @post viewと外部状態の変更なし。
			 * @code
			 * const std::uint8_t data[] = {0x10U, 0x20U};
			 * ket::byte_view::View view(data, 2U);
			 * const auto size = view.Size();
			 * // size == 2
			 * @endcode
			 */
			std::size_t Size() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 空view判定。
			 * @retval true `Size() == 0`。
			 * @retval false `Size() != 0`。
			 * @pre なし。
			 * @post viewと外部状態の変更なし。
			 * @code
			 * ket::byte_view::View view;
			 * const auto empty = view.Empty();
			 * // empty == true
			 * @endcode
			 */
			bool Empty() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 指定indexのbyte取得。
			 * @param[in] index 取得対象の0始まりindex。
			 * @param[out] out 取得したbyteの格納先。
			 * @retval true viewがvalidかつ`index < Size()`で、`out`へbyteを格納。
			 * @retval false invalid view、または範囲外index。`out`は変更なし。
			 * @pre `out`は有効な参照。viewの元buffer lifetimeは呼び出し中維持。
			 * @post 成功時のみ`out`を変更。viewと元bufferの変更なし。
			 * @code
			 * const std::uint8_t data[] = {0x10U, 0x20U};
			 * ket::byte_view::View view(data, 2U);
			 * std::uint8_t value = 0U;
			 * const bool ok = view.TryAt(1U, value);
			 * // ok == true, value == 0x20
			 * @endcode
			 */
			bool TryAt(std::size_t index, std::uint8_t& out) const noexcept;

			/**
			 * @brief 指定範囲の読み取り専用subview取得。
			 * @param[in] offset subview開始位置。
			 * @param[in] count subview byte数。
			 * @param[out] out 取得したsubviewの格納先。
			 * @retval true viewがvalidかつ`offset`から`count`byteが範囲内で、`out`へsubviewを格納。
			 * @retval false invalid view、または範囲外slice。`out`は変更なし。
			 * @pre `out`は有効な参照。viewの元buffer lifetimeはsubview利用中も維持。
			 * @post 成功時のみ`out`を変更。viewと元bufferの変更なし。
			 * @note `offset == Size()`かつ`count == 0`の空subviewは成功。
			 * @code
			 * const std::uint8_t data[] = {0x10U, 0x20U, 0x30U};
			 * ket::byte_view::View view(data, 3U);
			 * ket::byte_view::View slice;
			 * const auto ok = view.TrySlice(1U, 2U, slice);
			 * // ok == true, slice.Data() == data + 1, slice.Size() == 2
			 * @endcode
			 */
			bool TrySlice(std::size_t offset, std::size_t count, View& out) const noexcept;

		  private:
			const std::uint8_t* data_;
			std::size_t size_;
		};

		/**
		 * @brief 書き込み可能なnon-owning byte view。
		 * @note viewは元bufferを所有せず、利用者がbuffer lifetimeと書き込み可能性を保持。
		 */
		class MutableView
		{
		  public:
			/**
			 * @brief 空の書き込み可能byte view構築。
			 * @retval void 戻り値なし。
			 * @pre なし。
			 * @post `Data() == nullptr`、`Size() == 0`、`Empty() == true`。
			 * @code
			 * ket::byte_view::MutableView view;
			 * // view.Data() == nullptr
			 * // view.Size() == 0
			 * // view.Empty() == true
			 * @endcode
			 */
			MutableView() noexcept : data_(nullptr), size_(0U) {}

			/**
			 * @brief 書き込み可能byte列を参照するview構築。
			 * @param[in] data 参照対象bufferの先頭。`nullptr`は`size == 0`の場合のみ空view。
			 * @param[in] size 参照対象bufferのbyte数。
			 * @retval void 戻り値なし。
			 * @pre `data`は`size`byte以上読み書き可能な配列を指す。`nullptr + 非0`はinvalid
			 * viewとして保持。
			 * @post `data`と`size`を保持し、bufferの所有権は取得なし。
			 * @code
			 * std::uint8_t data[] = {0x10U, 0x20U};
			 * ket::byte_view::MutableView view(data, 2U);
			 * // view.Data() == data
			 * // view.Size() == 2
			 * @endcode
			 */
			MutableView(std::uint8_t* data, std::size_t size) noexcept : data_(data), size_(size) {}

			/**
			 * @brief 参照対象buffer先頭の取得。
			 * @retval value 構築時に渡されたbuffer先頭。空またはinvalid
			 * viewでは`nullptr`もあり得る。
			 * @pre なし。
			 * @post viewと外部状態の変更なし。
			 * @code
			 * std::uint8_t data[] = {0x10U};
			 * ket::byte_view::MutableView view(data, 1U);
			 * const auto pointer = view.Data();
			 * // pointer == data
			 * @endcode
			 */
			std::uint8_t* Data() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 参照対象buffer byte数の取得。
			 * @retval value 構築時に渡されたbyte数。
			 * @pre なし。
			 * @post viewと外部状態の変更なし。
			 * @code
			 * std::uint8_t data[] = {0x10U, 0x20U};
			 * ket::byte_view::MutableView view(data, 2U);
			 * const auto size = view.Size();
			 * // size == 2
			 * @endcode
			 */
			std::size_t Size() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 空view判定。
			 * @retval true `Size() == 0`。
			 * @retval false `Size() != 0`。
			 * @pre なし。
			 * @post viewと外部状態の変更なし。
			 * @code
			 * ket::byte_view::MutableView view;
			 * const auto empty = view.Empty();
			 * // empty == true
			 * @endcode
			 */
			bool Empty() const noexcept; // NOLINT(modernize-use-nodiscard)

			/**
			 * @brief 指定indexのbyte取得。
			 * @param[in] index 取得対象の0始まりindex。
			 * @param[out] out 取得したbyteの格納先。
			 * @retval true viewがvalidかつ`index < Size()`で、`out`へbyteを格納。
			 * @retval false invalid view、または範囲外index。`out`は変更なし。
			 * @pre `out`は有効な参照。viewの元buffer lifetimeは呼び出し中維持。
			 * @post 成功時のみ`out`を変更。viewと元bufferの変更なし。
			 * @code
			 * std::uint8_t data[] = {0x10U, 0x20U};
			 * ket::byte_view::MutableView view(data, 2U);
			 * std::uint8_t value = 0U;
			 * const auto ok = view.TryAt(1U, value);
			 * // ok == true, value == 0x20
			 * @endcode
			 */
			bool TryAt(std::size_t index, std::uint8_t& out) const noexcept;

			/**
			 * @brief 指定indexのbyte更新。
			 * @param[in] index 更新対象の0始まりindex。
			 * @param[in] value 書き込むbyte値。
			 * @retval true viewがvalidかつ`index < Size()`で、元bufferへ`value`を書き込み。
			 * @retval false invalid view、または範囲外index。元bufferは変更なし。
			 * @pre viewの元buffer lifetimeと書き込み可能性は呼び出し中維持。
			 * @post 成功時のみ元bufferの`index`位置を変更。view自体の状態変更なし。
			 * @code
			 * std::uint8_t data[] = {0x10U, 0x20U};
			 * ket::byte_view::MutableView view(data, 2U);
			 * const bool ok = view.TrySet(1U, 0x30U);
			 * // ok == true, data[1] == 0x30
			 * @endcode
			 */
			bool TrySet(std::size_t index, std::uint8_t value) noexcept;

			/**
			 * @brief 指定範囲の書き込み可能subview取得。
			 * @param[in] offset subview開始位置。
			 * @param[in] count subview byte数。
			 * @param[out] out 取得したsubviewの格納先。
			 * @retval true viewがvalidかつ`offset`から`count`byteが範囲内で、`out`へsubviewを格納。
			 * @retval false invalid view、または範囲外slice。`out`は変更なし。
			 * @pre `out`は有効な参照。viewの元buffer
			 * lifetimeと書き込み可能性はsubview利用中も維持。
			 * @post 成功時のみ`out`を変更。viewと元bufferの変更なし。
			 * @note `offset == Size()`かつ`count == 0`の空subviewは成功。
			 * @code
			 * std::uint8_t data[] = {0x10U, 0x20U, 0x30U};
			 * ket::byte_view::MutableView view(data, 3U);
			 * ket::byte_view::MutableView slice;
			 * const auto ok = view.TrySlice(1U, 2U, slice);
			 * // ok == true, slice.Data() == data + 1, slice.Size() == 2
			 * @endcode
			 */
			bool TrySlice(std::size_t offset, std::size_t count, MutableView& out) const noexcept;

		  private:
			std::uint8_t* data_;
			std::size_t size_;
		};

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief pointerとsizeのvalid性判定。
			 * @param[in] data 判定対象buffer先頭。
			 * @param[in] size 判定対象buffer byte数。
			 * @retval true `data != nullptr`、または`size == 0`。
			 * @retval false `data == nullptr`かつ`size != 0`。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr bool IsValidStorage(const void* data, std::size_t size) noexcept
			{
				return data != nullptr || size == 0U;
			}

			/**
			 * @brief 指定indexのaccess可否判定。
			 * @param[in] data 判定対象buffer先頭。
			 * @param[in] size 判定対象buffer byte数。
			 * @param[in] index 判定対象index。
			 * @retval true storageがvalidかつ`index < size`。
			 * @retval false invalid storage、または範囲外index。
			 * @pre なし。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr bool CanAccess(const void* data, std::size_t size, std::size_t index) noexcept
			{
				return IsValidStorage(data, size) && index < size;
			}

			/**
			 * @brief 指定範囲のslice可否判定。
			 * @param[in] data 判定対象buffer先頭。
			 * @param[in] size 判定対象buffer byte数。
			 * @param[in] offset slice開始位置。
			 * @param[in] count slice byte数。
			 * @retval true storageがvalidかつ`offset`から`count`byteが範囲内。
			 * @retval false invalid storage、または範囲外slice。
			 * @pre なし。`offset > size`は失敗値として扱い、減算underflowを避ける。
			 * @post 引数と外部状態の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			constexpr bool CanSlice(const void* data,
									std::size_t size,
									std::size_t offset,
									std::size_t count) noexcept
			{
				return IsValidStorage(data, size) && offset <= size && count <= (size - offset);
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		inline auto View::Data() const noexcept -> const std::uint8_t*
		{
			return data_;
		}

		inline auto View::Size() const noexcept -> std::size_t
		{
			return size_;
		}

		inline auto View::Empty() const noexcept -> bool
		{
			return size_ == 0U;
		}

		inline auto View::TryAt(std::size_t index, std::uint8_t& out) const noexcept -> bool
		{
			const auto can_access = detail::CanAccess(data_, size_, index);
			if (!can_access)
			{
				return false;
			}

			out = data_[index];
			return true;
		}

		inline auto
		View::TrySlice(std::size_t offset, std::size_t count, View& out) const noexcept -> bool
		{
			const auto can_slice = detail::CanSlice(data_, size_, offset, count);
			if (!can_slice)
			{
				return false;
			}

			const std::uint8_t* slice_data = nullptr;
			if (data_ != nullptr)
			{
				slice_data = data_ + offset;
			}

			out = View(slice_data, count);
			return true;
		}

		inline auto MutableView::Data() const noexcept -> std::uint8_t*
		{
			return data_;
		}

		inline auto MutableView::Size() const noexcept -> std::size_t
		{
			return size_;
		}

		inline auto MutableView::Empty() const noexcept -> bool
		{
			return size_ == 0U;
		}

		inline auto MutableView::TryAt(std::size_t index, std::uint8_t& out) const noexcept -> bool
		{
			const auto can_access = detail::CanAccess(data_, size_, index);
			if (!can_access)
			{
				return false;
			}

			out = data_[index];
			return true;
		}

		inline auto MutableView::TrySet(std::size_t index, std::uint8_t value) noexcept -> bool
		{
			const auto can_access = detail::CanAccess(data_, size_, index);
			if (!can_access)
			{
				return false;
			}

			data_[index] = value;
			return true;
		}

		inline auto MutableView::TrySlice(std::size_t offset,
										  std::size_t count,
										  MutableView& out) const noexcept -> bool
		{
			const auto can_slice = detail::CanSlice(data_, size_, offset, count);
			if (!can_slice)
			{
				return false;
			}

			std::uint8_t* slice_data = nullptr;
			if (data_ != nullptr)
			{
				slice_data = data_ + offset;
			}

			out = MutableView(slice_data, count);
			return true;
		}

	} // namespace byte_view

} // namespace ket
