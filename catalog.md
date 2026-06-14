# ket catalog

候補APIの保管場所です。

ここに書くことは実装予定ではありません。C++で何度も調べる、書き間違えると危ない、
標準ライブラリだけでは儀式が長い、という小さな痛みを失わないために記録します。

## 記入テンプレート

````md
## Idea: AlignUp

Category: numeric / memory

Pain:

- `(value + alignment - 1) / alignment * alignment` を毎回書きたくない
- alignment == 0 や overflow が怖い

Candidate API:

```cpp
ket::AlignUp(value, alignment)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：標準ライブラリだけでは意図が読み取りにくい小さな定型処理を名前付きで扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- alignment == 0
- overflow
- signed / unsigned

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- AlignUp(0, 4) == 0
- AlignUp(1, 4) == 4
- AlignUp(4, 4) == 4
- AlignUp(5, 4) == 8
````

## Ideas

## Idea: Bcd

Category: numeric / binary

Pain:

- packed BCDのnibble検査を毎回書きたくない
- `0x0A` や `0xA0` のような不正BCDを見落としたくない
- 任意バイト長BCDでは先頭ゼロと整数overflowの扱いが曖昧になりやすい
- 10進整数から固定幅packed BCDを作るときの桁数確認を毎回書きたくない

Candidate API:

```cpp
ket::ParseBcd(value)
ket::ToBcd8(value)
ket::ToBcd16(value)
ket::ToBcd32(value)
ket::BcdToDecimalString(data, size)
ket::DecimalStringToBcd(text)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：packed BCDの直接代替が標準ライブラリになく、`std::optional`で失敗値を明確に扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし。標準ライブラリにpacked BCD変換の直接APIなし。

Failure / edge cases:

- nibble > 9
- `data == nullptr`
- `size == 0`
- empty decimal string
- non-digit character
- accumulated integer overflow
- negative decimal value
- fixed-width BCD digit overflow

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- ParseBcd(0x00) == 0
- ParseBcd(0x09) == 9
- ParseBcd(0x10) == 10
- ParseBcd(0x99) == 99
- ParseBcd(0x0A) == std::nullopt
- ToBcd8(42) == 0x42
- ToBcd8(100) == std::nullopt
- ToBcd16(1234) == 0x1234
- ToBcd32(20260613) == 0x20260613
- BcdToDecimalString({0x00, 0x42}) == "0042"
- DecimalStringToBcd("0042") == {0x00, 0x42}
- DecimalStringToBcd("123") == {0x01, 0x23}
- DecimalStringToBcd("12A4") == std::nullopt

## Idea: String

Category: string

Pain:

- `std::string::append` を複数行に分けて書くと、業務意図より連結手順が目立つ
- `operator+` 連鎖では一時文字列や型の違いを意識しやすい
- 文字列片の連結と既存文字列への追記を、formatではない小さな意図として名前にしたい

Candidate API:

```cpp
ket::StrCat(...)
ket::StrAppend(dst, ...)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view`を利用でき、文字列片連結を標準ライブラリのみで安全に薄く包める
- 本ライブラリの適用を推奨しない C++ バージョン：C++20以降
- 非推奨理由：書式付き文字列生成が目的の場合は、`std::format`で容易かつ明確に代替可能
- 標準代替：書式付き文字列生成ではC++20 `std::format`。

Failure / edge cases:

- empty argument list
- `char` as one-character part
- embedded NUL in length-aware `std::string_view`
- raw C string must be non-null and null-terminated
- destination reference for `StrAppend` must refer to a valid `std::string`
- self-reference such as `StrAppend(dst, dst, std::string_view(dst))`

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- StrCat() == ""
- StrCat("a", std::string("b"), std::string_view("c"), 'd') == "abcd"
- StrCat(std::string_view("a\\0b", 3)) preserves 3 bytes
- StrAppend(dst, ...) appends to existing text
- StrAppend(dst) keeps dst unchanged
- StrAppend(dst, dst, ":", std::string_view(dst)) uses pre-append content
