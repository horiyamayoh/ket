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

Candidate API:
```cpp
ket::ParseBcd(value)
ket::BcdToDecimalString(data, size)
```

C++ versions:
- C++17 or later

Failure / edge cases:
- nibble > 9
- `data == nullptr`
- `size == 0`
- accumulated integer overflow

Dependencies:
- Standard library only
- No ket dependencies

Tests:
- ParseBcd(0x00) == 0
- ParseBcd(0x09) == 9
- ParseBcd(0x10) == 10
- ParseBcd(0x99) == 99
- ParseBcd(0x0A) == std::nullopt
- BcdToDecimalString({0x00, 0x42}) == "0042"
