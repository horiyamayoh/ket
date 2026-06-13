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

C++ versions:

- C++11 or later

Failure / edge cases:

- alignment == 0
- overflow
- signed / unsigned

Dependencies:

- Standard library only
- No ket dependencies

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

C++ versions:

- C++17 or later

Failure / edge cases:

- nibble > 9
- `data == nullptr`
- `size == 0`
- empty decimal string
- non-digit character
- accumulated integer overflow
- negative decimal value
- fixed-width BCD digit overflow

Dependencies:

- Standard library only
- No ket dependencies

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

C++ versions:

- C++17 or later

Failure / edge cases:

- empty argument list
- `char` as one-character part
- embedded NUL in length-aware `std::string_view`
- raw C string must be non-null and null-terminated
- destination reference for `StrAppend` must refer to a valid `std::string`
- self-reference such as `StrAppend(dst, dst, std::string_view(dst))`

Dependencies:

- Standard library only
- No ket dependencies

Tests:

- StrCat() == ""
- StrCat("a", std::string("b"), std::string_view("c"), 'd') == "abcd"
- StrCat(std::string_view("a\\0b", 3)) preserves 3 bytes
- StrAppend(dst, ...) appends to existing text
- StrAppend(dst) keeps dst unchanged
- StrAppend(dst, dst, ":", std::string_view(dst)) uses pre-append content
