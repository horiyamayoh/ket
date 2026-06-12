# Testing

ketのテストは、小さい仕様を境界値で固定するために書きます。

## 基本方針

- functional testはGoogleTestで書きます。
- GoogleTestはFetchContentで `v1.17.0` に固定します。
- GoogleTest v1.17.0はC++17以上を要求するため、functional testはC++17でビルドします。
- C++11/14対応moduleは、別途compile-only checkで最低標準を確認します。

## コマンド

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
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
