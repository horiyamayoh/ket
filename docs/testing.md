# Testing

ketのテストは、小さい仕様を境界値で固定するために書きます。

## 基本方針

- functional testはGoogleTestで書きます。
- GoogleTestはFetchContentで `v1.17.0` に固定します。
- GoogleTest v1.17.0はC++17以上を要求するため、functional testはC++17でビルドします。
- C++11/14対応moduleは、別途compile-only checkで最低標準を確認します。
- 各 `TEST` にはDoxygen形式の試験仕様コメントを書きます。
- 試験仕様コメントでは `@test`、`@brief`、`@details`、`@pre`、`@post` を使います。
- `@brief` は試験目的、`@details` は入力条件と期待結果を1から2文で書きます。
- `@pre`、`@post` は、テストの前提とテスト後に変わらない性質が分かる文章にします。
- GoogleTestの `TEST` macroには通常の関数引数がないため、`@param` と `@retval` は要求しません。

例:

```cpp
/**
 * @test
 * @brief 2桁固定幅パックBCDの正常系確認。
 * @details 代表値と境界値を入力し、10進整数へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, ParsesUint8Bcd)
{
	// ...
}
```

## コマンド

```sh
python3 tools/check_repository.py
```

個別の確認を分けて実行する場合:

```sh
python3 tools/check_python.py
python3 tools/check_layout.py
python3 tools/check_format.py
python3 tools/check_site.py
cmake --preset dev
cmake --build --preset dev
cmake --build --preset dev --target check-static
cmake --build --preset dev --target check-conventions
cmake --build --preset dev --target check-site
ctest --preset dev
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize
git diff --check
```

## CMake helper

module testは次の形で追加します。

```cmake
ket_add_module_test(
	ket_bcd_test
	SOURCES
		modules/bcd/ket_bcd.cpp
		modules/bcd/ket_bcd_test.cpp
)
```

C++11/14などの最低標準確認はcompile-only checkで追加します。

```cmake
ket_add_compile_check(
	ket_bcd_cxx11_check
	CXX_STANDARD 11
	SOURCES
		modules/bcd/ket_bcd.cpp
		modules/bcd/ket_bcd.h
)
```

## テスト観点

- 正常系の代表値
- 最小値、最大値、空入力
- overflow、underflow、alignment == 0などの危険条件
- invalid input
- signed/unsigned、endian、shift幅、寿命など未定義動作につながる条件
