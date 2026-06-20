#pragma once

/**
 * @file ket_testing.h
 * @brief byte列比較用GoogleTest assertion helper。
 *
 * @details byte列とhex文字列の比較失敗時に、差分offsetと期待値/実際値のhex表現を
 * GoogleTest failure messageへ出力する。production library本体ではなく、test helperとして
 * GoogleTestを明示的に依存に持つ。drop-in時は宣言と実装を同じ単位で持ち出す。
 *
 * @par プロジェクトへの適用方法
 * `ket_testing.h` と `ket_testing.cpp` をテストプロジェクトへコピー。GoogleTestを利用する
 * テストターゲットだけから参照。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：GoogleTest v1.17.0 のC++17要件に合わせ、byte列差分を読みやすく表示できる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * GoogleTestと標準ライブラリ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::testing
 * 内部実装：ket::testing::detail
 */

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <gtest/gtest.h>

namespace ket
{
	namespace testing
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief 2つのbyte列をGoogleTest assertionとして比較。
		 * @param[in] expected 期待するbyte列。`expected_size == 0`
		 * の場合はnullptrを空列として許容。
		 * @param[in] expected_size `expected`のバイト数。
		 * @param[in] actual 実際のbyte列。`actual_size == 0` の場合はnullptrを空列として許容。
		 * @param[in] actual_size `actual`のバイト数。
		 * @retval success byte列の長さと内容が一致。
		 * @retval failure nullptr+非0サイズ、長さ差、または内容差。failure messageに差分offsetと
		 * expected/actual hexを含む。
		 * @pre `expected`と`actual`は、それぞれのsizeが0でない場合にsizeバイト以上読み取り可能な
		 * 配列を指す。nullptr+非0サイズはfailureとして扱う。
		 * @post 引数と外部状態の変更なし。成功時は診断用stringを生成しない。
		 * @code
		 * const std::uint8_t expected[] = {0x12U, 0x34U};
		 * const std::uint8_t actual[] = {0x12U, 0x34U};
		 * EXPECT_TRUE(ket::testing::BytesEqual(expected, 2U, actual, 2U));
		 * @endcode
		 */
		::testing::AssertionResult BytesEqual(const std::uint8_t* expected,
											  std::size_t expected_size,
											  const std::uint8_t* actual,
											  std::size_t actual_size);

		/**
		 * @brief hex文字列で表した期待byte列と実際のbyte列をGoogleTest assertionとして比較。
		 * @param[in] expected_hex 期待するbyte列の連続hex文字列。大文字小文字を許容し、区切り文字は
		 * 許容なし。
		 * @param[in] actual 実際のbyte列。`actual_size == 0` の場合はnullptrを空列として許容。
		 * @param[in] actual_size `actual`のバイト数。
		 * @retval success `expected_hex`が妥当なhexで、復号後の長さと内容が`actual`と一致。
		 * @retval failure 不正hex、nullptr+非0サイズ、長さ差、または内容差。failure messageに
		 * 差分offsetとexpected/actual hexを含む。
		 * @pre `expected_hex`は呼び出し中有効な文字列範囲。`actual`は`actual_size`が0でない場合に
		 * `actual_size`バイト以上読み取り可能な配列を指す。nullptr+非0サイズはfailureとして扱う。
		 * @post 引数と外部状態の変更なし。成功時は診断用stringを生成しない。
		 * @code
		 * const std::uint8_t actual[] = {0x12U, 0x34U};
		 * EXPECT_TRUE(ket::testing::HexEqual("1234", actual, 2U));
		 * @endcode
		 */
		::testing::AssertionResult HexEqual(std::string_view expected_hex,
											const std::uint8_t* actual,
											std::size_t actual_size);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

	} // namespace testing

} // namespace ket
