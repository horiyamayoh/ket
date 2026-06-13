# ket Coding Agent Brief

この文書は、C++ユーティリティ集 **ket** を実装するコーディングエージェント向けの設計ブリーフです。
ここまでの会話で固まってきた思想・スコープ・実装方針・候補APIをまとめています。

重要: この文書は「ロードマップ」や「MVP計画」ではありません。  
今すぐ何から作るかではなく、**ket がどんな宇宙を持つライブラリなのか**を共有するための文書です。

---

## 1. ket の一文定義

> **ket は、C++で毎回調べる・毎回迷う・毎回バグる・毎回儀式が長い小さな処理に、短く安全な名前を与える drop-in utility catalog である。**

もう少し実務寄りに言うと、

> **ket は、業務C++で繰り返し現れる低レベルな定型ロジックを、持ち出し可能な `.h/.cpp` 単位で集める小さな部品箱である。**

たとえば、利用側のコードからこういうノイズを消す。

```cpp
(value >> 4) & 0x0F
```

こう書きたい。

```cpp
auto hi = ket::HighNibble(value);
```

また、こういう儀式を消す。

```cpp
auto it = map.find(key);
if (it == map.end()) {
    return default_value;
}
return it->second;
```

こう書きたい。

```cpp
return ket::GetOrDefault(map, key, default_value);
```

ket の価値は、C++の機能を隠すことではなく、**C++で何度も書く小さな意図に名前を与えること**にある。

---

## 2. ket が解決したい痛み

ket に入る候補は、次のような処理である。

- C++で二度以上調べたことがある
- 書き方は短いが、毎回ビット・境界・寿命・型変換などを思い出す必要がある
- 書き間違えるとバグる
- 標準ライブラリだけで書くと儀式が長い
- 業務コードに直書きすると、本当に読みたい業務意図が埋もれる
- 既存の大規模ライブラリを導入するほどではない
- しかし、小さい部品としては何度も使う

代表例:

```cpp
ket::ParseBcd(byte);
ket::StrCat("id=", id, ", mode=", mode);
ket::AlignUp(size, 4U);
ket::LoadBe32(ptr);
ket::EnumName(mode, table);
ket::AtOrOptional(map, key);
ket::ScopeExit{[&] { cleanup(); }};
ket::HexDump(buffer);
```

---

## 3. ket の基本思想

### 3.1 ライブラリというより drop-in module 集

ket は、`include/ket/...` にきれいに収めてパッケージとして導入することを第一目的にしない。

主目的は、次のような使い方である。

```txt
この機能が欲しい
↓
modules/bcd/ket_bcd.h と ket_bcd.cpp だけコピー
↓
必要なら namespace を変更
↓
どの業務プロジェクトでもすぐ使える
```

つまり、ket の基本単位は **module** である。

```txt
modules/<module_name>/ket_<module_name>.h
modules/<module_name>/ket_<module_name>.cpp
```

例:

```txt
modules/bcd/ket_bcd.h
modules/bcd/ket_bcd.cpp

modules/string/ket_string.h
modules/string/ket_string.cpp

modules/parse/ket_parse.h
modules/parse/ket_parse.cpp
```

### 3.2 内部依存を極力持たない

ket の最重要制約:

> **各 module は、原則として他の ket module に依存しない。**

望ましい状態:

```txt
ket_bcd.h + ket_bcd.cpp だけで使える
ket_bits.h + ket_bits.cpp だけで使える
ket_string.h + ket_string.cpp だけで使える
```

やむを得ず依存する場合は、ファイル先頭に明記する。

```cpp
// Drop-in files:
//   required: ket_bytes.h, ket_bytes.cpp
//   optional: none
//
// Dependencies:
//   C++17 or later
//   Standard library only
```

ただし、最初の設計原則としては **Standard library only** を目指す。

小さい内部処理は、重複してもよい。  
たとえば `ket_bcd.cpp` の中で `HighNibble` 相当の処理が必要でも、`ket_bits` に依存させず、内部無名名前空間に小さく書いてよい。

```cpp
namespace
{
    constexpr std::uint8_t HighNibbleLocal(std::uint8_t value) noexcept
    {
        return static_cast<std::uint8_t>((value >> 4U) & 0x0FU);
    }
}
```

ket では、DRY より drop-in 性を優先する場面がある。

### 3.3 標準ライブラリを置き換えない

ket は `std::vector`、`std::string`、`std::optional`、`std::span`、`std::format` などを再発明することを目的にしない。

ket がやるべきこと:

```cpp
ket::Contains(vec, value);
ket::AtOrOptional(map, key);
ket::StrCat(...);
ket::ParseUInt<std::uint32_t>(text);
```

ket が安易にやるべきでないこと:

```cpp
ket::vector<T>
ket::string
ket::map
ket::thread_pool
ket::mega_framework
```

標準ライブラリを包む。  
標準ライブラリの儀式を短くする。  
標準ライブラリの意図を名前で補強する。  
しかし、標準ライブラリそのものを置き換えない。

### 3.4 業務固有名を入れない

入れてよい抽象度:

```txt
Bcd
Nibble
Byte
Endian
Enum
Payload
Date
Time
IpV4
MacAddress
Uuid
Port
Version
```

入れない抽象度:

```txt
TrainCommand
DeviceMode
ProjectSpecificHeader
OurCompanyProtocol
FooProductRequest
```

ket は業務コードの下に置く汎用部品であり、業務ドメインそのものではない。

### 3.5 名前で中身が想像できること

良い名前:

```cpp
ket::ParseBcd()
ket::HighNibble()
ket::LowNibble()
ket::AlignUp()
ket::LoadBe32()
ket::StrCat()
ket::ScopeExit
ket::ToUnderlying()
```

悪い名前:

```cpp
ket::MagicConvert()
ket::BuildNiceValue()
ket::DoSmartThing()
ket::Helper()
```

ket のAPIは、使う側のコードに出たときに意図が読めなければならない。

---

## 4. C++ バージョン方針

C++11〜C++23まで幅広く考える。

ただし、全moduleが全標準バージョンを完全に同一APIでサポートする必要はない。

### 4.1 基本方針

- C++11でも成立する純粋ロジックは、できるだけC++11対応にする
- C++17以降で便利になるものは、`std::optional`、`std::string_view`、`std::filesystem` などを利用してよい
- C++20以降で便利になるものは、`std::span`、`std::format`、calendar/date、concepts などを利用してよい
- C++23以降で便利になるものは、`std::expected`、`std::move_only_function` などを利用してよい
- ただし、「標準にあるからketでは扱わない」ではなく、「標準にあるが、C++11〜17でも欲しい」「標準を薄く包むと業務コードが読みやすくなる」ならスコープに含めてよい

### 4.2 C++11対応と optional 問題

C++11には `std::optional` がない。  
そのため、parse系など失敗しうるAPIでは、次の二層構成を検討する。

```cpp
bool TryParseUInt(const std::string& text, std::uint32_t* out) noexcept;

#if KET_HAS_STD_OPTIONAL
std::optional<std::uint32_t> ParseUInt(std::string_view text) noexcept;
#endif
```

C++11〜14対応を重視するmoduleでは、`TryXxx(..., out*) -> bool` を中核APIにする。  
C++17以降では `std::optional<T>` を返す便利APIを追加してよい。

### 4.3 C++20/23固有の便利module

C++20/23だからこそ作れるmoduleもスコープに含める。

例:

```cpp
ket::SpanLike helpers
ket::CalendarDate helpers
ket::FormatDuration
ket::Expected helpers
ket::MoveOnlyFunction wrapper
ket::Concept-based overload guards
ket::Ranges helpers
```

ただし、C++20/23専用のAPIは、ファイル先頭に明記する。

```cpp
// Requires:
//   C++20 or later
//   <span>
```

---

## 5. 推奨リポジトリ構成

ket は drop-in module 集として設計する。

```txt
ket/
├─ README.md
├─ catalog.md
├─ ket_coding_agent_brief.md
│
├─ docs/
│  ├─ design.md
│  ├─ module_policy.md
│  ├─ naming.md
│  └─ compatibility.md
│
├─ modules/
│  ├─ numeric/
│  │  ├─ ket_numeric.h
│  │  ├─ ket_numeric.cpp
│  │  └─ ket_numeric_test.cpp
│  │
│  ├─ bits/
│  │  ├─ ket_bits.h
│  │  ├─ ket_bits.cpp
│  │  └─ ket_bits_test.cpp
│  │
│  ├─ bcd/
│  │  ├─ ket_bcd.h
│  │  ├─ ket_bcd.cpp
│  │  └─ ket_bcd_test.cpp
│  │
│  ├─ string/
│  │  ├─ ket_string.h
│  │  ├─ ket_string.cpp
│  │  └─ ket_string_test.cpp
│  │
│  └─ ...
│
├─ recipes/
│  ├─ bcd_datetime/
│  ├─ binary_payload/
│  ├─ command_parser/
│  ├─ enum_table/
│  └─ tcp_hexdump/
│
├─ examples/
│  ├─ bcd_example.cpp
│  ├─ strcat_example.cpp
│  └─ payload_builder_example.cpp
│
└─ tools/
   └─ optional helper scripts
```

`include/` / `src/` 形式は本格ライブラリとしては自然だが、ket の第一思想ではない。  
ket の主役は `modules/` である。

---

## 6. module の標準形

### 6.1 ファイル構成

各moduleは原則として次の3ファイルを持つ。

```txt
modules/<name>/ket_<name>.h
modules/<name>/ket_<name>.cpp
modules/<name>/ket_<name>_test.cpp
```

header-onlyで十分なmoduleでも、次のどちらかにする。

1. `.cpp` を置かず、READMEまたはファイル先頭に `header-only` と明記する
2. 空に近い `.cpp` を置き、`#include "ket_<name>.h"` だけ書いてコンパイル確認用にする

### 6.2 ヘッダ先頭コメント

各ヘッダには、drop-in条件を明記する。

```cpp
#pragma once

// ket_bcd.h
//
// Drop-in module:
//   Copy ket_bcd.h and ket_bcd.cpp into your project.
//
// Dependencies:
//   C++11 or later
//   Standard library only
//   No other ket modules required
//
// Namespace:
//   ket

#include <cstdint>

namespace ket
{
    // ...
}
```

### 6.3 名前空間

基本は固定でよい。

```cpp
namespace ket
{
}
```

持ち出し先で名前空間を変えたい場合は、一括置換を想定する。

```txt
namespace ket -> namespace project_util
ket::         -> project_util::
```

`KET_NAMESPACE` マクロによる可変名前空間も可能だが、`.h` と `.cpp` の一致管理が難しくなるため、最初から採用する必要はない。

namespaceなどC++ネイティブな語はコメント内でも和訳しない。namespace終端コメントは
`// namespace ket` の形にし、その直前に空行を1行入れる。

### 6.4 関数命名

会話中の例に合わせ、公開APIは原則として UpperCamelCase を使う。

```cpp
ket::ParseBcd()
ket::HighNibble()
ket::LowNibble()
ket::StrCat()
ket::AlignUp()
ket::LoadBe32()
ket::ScopeExit
ket::ByteWriter
```

型名も UpperCamelCase。

```cpp
ket::ScopeExit
ket::ByteView
ket::EnumEntry
ket::Deadline
```

定数は `k` prefix を使ってよい。

```cpp
constexpr std::uint32_t kMaxPort = 65535U;
```

`const` は左寄せにする。`const char* text` のように対象の型の左に置き、ポインタ自体を
constにする場合など意味があるときだけ `const char* const text` のように右側へ置く。

### 6.5 noexcept / constexpr

- 純粋な変換・判定・ビット操作はできるだけ `constexpr` にする
- 例外で失敗を表現しない小関数はできるだけ `noexcept` にする
- ただし、文字列生成やメモリ確保を伴うものは無理に `noexcept` にしない

例:

```cpp
constexpr std::uint8_t HighNibble(std::uint8_t value) noexcept;
constexpr bool IsPowerOfTwo(std::uint32_t value) noexcept;
```

### 6.5.1 コメント文体とDoxygen

C++コメントは「です」「ます」「ください」を使わず、体言止めまたは簡潔な常体にする。

関数の契約は宣言側にDoxygen形式で書き、次を必須にする。
実装定義側に同じDoxygenコメントを重複させない。
ヘッダ内のinline関数やconstexpr関数は宣言兼定義として扱い、ヘッダ側にDoxygenコメントを書く。
`.cpp`内だけで使うhelperは実装詳細なのでDoxygenコメントを書かない。

- `@brief`
- `@param[in]` または `@param[out]`
- `@retval`
- `@pre`
- `@post`

必要な場合だけ `@note`、`@remark` を追加する。
`@code` は必須ではないが、自明なgetterや単純な判定以外では強く推奨する。
入力例、戻り値、失敗時の形が一目で分かる短い例にする。

`@note`、`@pre`、`@post` は、読み手が制約や副作用を判断できる文章にする。
`なし` と書く場合は、本当に事前条件や副作用がない場合だけにする。
保持する性質を書く場合は、何をどう保持するのかまで具体化する。

### 6.5.2 制御構文の条件式

`if`、`while`、テストmacroの条件式では、API呼び出しやメソッド呼び出しを直接行わない。
直前の一時変数に退避し、gdbで値を確認しやすくする。

```cpp
const auto header_is_valid = IsValidHeader(header);
if (!header_is_valid)
{
	return false;
}
```

### 6.6 失敗表現

C++11〜14を意識する場合:

```cpp
bool TryParseBcd(std::uint8_t value, int* out) noexcept;
```

C++17以降:

```cpp
std::optional<int> ParseBcd(std::uint8_t value) noexcept;
```

C++23以降:

```cpp
std::expected<T, Error> ParseXxx(...);
```

ただし、`std::expected` 前提にするとdrop-in性が落ちるため、最初から全体標準にはしない。

---

## 7. ket のスコープ地図

前回の17分類は良い第一層だった。

```txt
1. language      C++言語の面倒
2. numeric       数値・範囲・桁・丸め
3. bits          bit / endian / byte
4. string        文字列処理
5. parse         文字列→値
6. enum          enum変換・flags
7. container     map/vector/setの小さい儀式
8. view          span/string_view/byte_view
9. error         optional/result/status
10. scope        RAII / defer / cleanup
11. time         日時・duration・timeout
12. bytes        payload / buffer / binary IO
13. cli          argc/argv / option
14. file         file/path
15. debug        assert/log/hexdump
16. platform     OS境界
17. recipes      組み合わせスニペット
```

ただし、C++全体を広く見るなら、これは決定版ではなく「第一層」である。  
より広い視野では、ket は次の40カテゴリを持つ可能性がある。

```txt
A. C++言語・型・式
  1. language
  2. meta
  3. object
  4. function
  5. contract

B. 値・数値・ビット・メモリ
  6. numeric
  7. math
  8. bits
  9. endian
  10. memory
  11. bytes
  12. serialization

C. 文字・テキスト・変換
  13. string
  14. encoding
  15. format
  16. parse
  17. enum

D. データ構造・範囲・アルゴリズム
  18. container
  19. algorithm
  20. range
  21. view
  22. tuple
  23. variant
  24. cache

E. 安全性・資源・失敗
  25. pointer
  26. error
  27. scope
  28. debug
  29. testing

F. 時間・並行・外界
  30. time
  31. concurrency
  32. io
  33. file
  34. cli
  35. platform
  36. c_interop
  37. build_config

G. 汎用ドメイン・組み合わせ
  38. domain
  39. state
  40. recipes
```

この40分類は、実装順ではない。  
ket の「広大な視野」を保つための地図である。

---

## 8. 詳細カテゴリと候補API

ここに挙げるAPIは決定ではない。  
コーディングエージェントは、実装前に名前・戻り値・C++バージョン・例外方針を確認すること。

### 8.1 language — C++言語の小さい面倒

目的: C++の構文・属性・基本的な儀式を名前付きにする。

候補:

```cpp
ket::IgnoreUnused(...)
ket::Unreachable()
ket::Exchange(obj, new_value)
ket::ArraySize(array)
ket::AsConst(x)
ket::Identity(x)
```

関連する型:

```cpp
ket::NonCopyable
ket::NonMovable
```

注意:

- `ToUnderlying` は `language` でも `enum` でもよいが、enum module に寄せてもよい
- マクロは最小限にする

---

### 8.2 meta — type traits / concepts / detection idiom

目的: テンプレート周りの「毎回書く型ロジック」をまとめる。

候補:

```cpp
ket::RemoveCvref<T>
ket::TypeIdentity<T>
ket::AlwaysFalse<T>
ket::DependentFalse<T>
ket::VoidT<...>
ket::IsComplete<T>
ket::IsSpecializationOf<T, Template>
ket::IsDetected<Expr, T>
```

C++20以降候補:

```cpp
ket::concepts::Integral
ket::concepts::Enum
ket::concepts::StringLike
ket::concepts::Range
```

注意:

- meta module は魔術的になりやすい
- 実装よりも使う側の可読性を優先する
- テンプレートエラーメッセージが悪化するAPIは避ける

---

### 8.3 object — construction / copy / move / regular type

目的: オブジェクトのコピー・ムーブ・初期化・リセットの儀式を減らす。

候補:

```cpp
ket::NonCopyable
ket::NonMovable
ket::MovableOnly
ket::CopyableOnly
ket::ResetOnMove<T>
ket::DefaultInit<T>
ket::NoInit<T>
ket::SwapAndReset(a, b)
ket::ResetToDefault(x)
```

注意:

- C++11〜17では比較演算子やムーブ制御の儀式が多い
- C++20以降の defaulted comparison と競合しないようにする

---

### 8.4 function — callable / lambda / overload

目的: callable周りの毎回書くパターンを短くする。

候補:

```cpp
ket::Overload{...}
ket::MakeOverload(...)
ket::Noop()
ket::Invoke(f, args...)
ket::BindFront(f, args...)
ket::FunctionRef<R(Args...)>
ket::MoveOnlyFunction<R(Args...)>
```

特に欲しいもの:

```cpp
std::visit(ket::Overload{
    [](const A&) { ... },
    [](const B&) { ... },
}, value);
```

注意:

- `FunctionRef` は非所有参照であり寿命が危険なので、明確なドキュメントが必要
- `MoveOnlyFunction` はC++23の標準機能と重なるため、互換ラッパとして考える

---

### 8.5 contract — precondition / postcondition / invariant

目的: 事前条件・事後条件・不変条件を明示する。

候補:

```cpp
ket::Expects(condition)
ket::Ensures(condition)
ket::AssertInvariant(condition)
ket::RequireNonNull(ptr)
ket::RequireInRange(value, min, max)
ket::CheckBounds(index, size)
```

注意:

- `debug` とは分ける。debugは観測、contractは意味
- abort/assert/exception/return error のポリシーをmodule内で明確にする
- グローバルマクロ汚染に注意

---

### 8.6 numeric — 数値・範囲・桁・丸め

目的: 数値計算の境界・丸め・オーバーフローを名前で安全にする。

候補:

```cpp
ket::InRange<T>(value)
ket::Clamp(value, min, max)
ket::AbsDiff(a, b)
ket::DivideRoundUp(a, b)
ket::AlignUp(value, alignment)
ket::AlignDown(value, alignment)
ket::CheckedAdd(a, b, &out)
ket::CheckedSub(a, b, &out)
ket::CheckedMul(a, b, &out)
ket::SaturatingAdd(a, b)
ket::SaturatingSub(a, b)
ket::NarrowCast<T>(value)
ket::CheckedCast<T>(value, &out)
```

注意:

- signed/unsigned混在は慎重に扱う
- `alignment` が0の場合の仕様を必ず固定する
- オーバーフロー検出はテストを厚くする

---

### 8.7 math — 数学・単位・統計・幾何

目的: 数値より少し高レベルな計算補助。

候補:

```cpp
ket::Lerp(a, b, t)
ket::InverseLerp(a, b, x)
ket::MapRange(x, in_min, in_max, out_min, out_max)
ket::Mean(range)
ket::Median(range)
ket::Variance(range)
ket::DegreesToRadians(deg)
ket::RadiansToDegrees(rad)
ket::Distance2D(a, b)
ket::RectContains(rect, point)
ket::BytesToKiB(bytes)
ket::KiBToBytes(kib)
```

注意:

- 単位変換は便利だが、巨大なunits frameworkにはしない
- 浮動小数点誤差の扱いをドキュメント化する

---

### 8.8 bits — bit / nibble / mask

目的: ビット演算の思い出しコストを消す。

候補:

```cpp
ket::HighNibble(byte)
ket::LowNibble(byte)
ket::HasBit(value, bit_index)
ket::SetBit(value, bit_index)
ket::ClearBit(value, bit_index)
ket::ToggleBit(value, bit_index)
ket::Mask(width)
ket::ByteAt(value, index)
ket::MakeU16(high, low)
ket::MakeU32(b3, b2, b1, b0)
ket::PopCount(value)
ket::CountLeadingZeros(value)
ket::CountTrailingZeros(value)
ket::Rotl(value, n)
ket::Rotr(value, n)
ket::IsPowerOfTwo(value)
```

注意:

- シフト幅が型のbit数以上になるケースは未定義動作に注意
- C++20の `<bit>` と重なるものは多いが、C++11〜17向けにも価値がある

---

### 8.9 endian — byte order

目的: バイナリ電文・ファイル・ネットワークでのエンディアン処理を安全にする。

候補:

```cpp
ket::ByteSwap16(value)
ket::ByteSwap32(value)
ket::ByteSwap64(value)
ket::LoadBe16(ptr)
ket::LoadBe32(ptr)
ket::LoadBe64(ptr)
ket::LoadLe16(ptr)
ket::LoadLe32(ptr)
ket::LoadLe64(ptr)
ket::StoreBe16(ptr, value)
ket::StoreBe32(ptr, value)
ket::StoreBe64(ptr, value)
ket::StoreLe16(ptr, value)
ket::StoreLe32(ptr, value)
ket::StoreLe64(ptr, value)
```

注意:

- `reinterpret_cast` で未アライン読み込みしない
- 基本はbyte単位で組み立てる
- `bits` と分ける価値がある

---

### 8.10 memory — allocation / alignment / object lifetime

目的: メモリ境界、アラインメント、オブジェクト寿命の小さい処理をまとめる。

候補:

```cpp
ket::IsAligned(ptr, alignment)
ket::AlignUpPtr(ptr, alignment)
ket::AlignDownPtr(ptr, alignment)
ket::ZeroMemory(ptr, size)
ket::SecureZeroMemory(ptr, size)
ket::ObjectBytes(obj)
ket::CopyBytes(dst, src, size)
ket::ConstructAt(ptr, args...)
ket::DestroyAt(ptr)
```

注意:

- C++20の `std::construct_at` と重なる
- object representation と object lifetime は危険領域なので、API名とコメントで用途を限定する

---

### 8.11 bytes — payload / buffer / binary IO

目的: byte列構築・読み取り・書き込みを見通しよくする。

候補:

```cpp
ket::ByteView
ket::MutableByteView
ket::ByteReader
ket::ByteWriter
ket::Bytes
ket::ReadU8()
ket::ReadU16Be()
ket::ReadU32Be()
ket::WriteU8()
ket::WriteU16Be()
ket::WriteU32Be()
ket::AppendBytes()
ket::AppendBcd()
```

fluent interface 例:

```cpp
auto payload = ket::Bytes{}
    .U8(command)
    .U16Be(sequence)
    .U32Be(id)
    .Bcd(year)
    .Bcd(month)
    .Bcd(day)
    .Build();
```

注意:

- buffer overrun を避ける
- reader/writer は現在位置と残りサイズを明示する
- endian module との依存を避けるなら、必要な処理を内部に持ってもよい

---

### 8.12 serialization — struct <-> bytes/text

目的: 構造化データとbytes/textの往復を小さく安全にする。

候補:

```cpp
ket::EncodeLengthPrefixed(bytes)
ket::DecodeLengthPrefixed(view, &out)
ket::EncodeTlv(type, value)
ket::DecodeTlv(view, &out)
ket::SerializeBe(value)
ket::DeserializeBe(view, &out)
```

注意:

- C++の構造体をそのままbytes化するのはpadding/endian/alignmentで危険
- `StructToBytes` のようなAPIは、用途を厳しく制限する
- 安全なフィールド単位serializeを優先する

---

### 8.13 string — 文字列処理

目的: C++標準だけだと地味に長い文字列処理をまとめる。

候補:

```cpp
ket::StrCat(...)
ket::StrAppend(&dst, ...)
ket::StartsWith(s, prefix)
ket::EndsWith(s, suffix)
ket::Contains(s, needle)
ket::Trim(s)
ket::TrimLeft(s)
ket::TrimRight(s)
ket::Split(s, delimiter)
ket::Join(range, delimiter)
ket::ReplaceAll(s, from, to)
ket::RemovePrefix(s, prefix)
ket::RemoveSuffix(s, suffix)
ket::ToLowerAscii(s)
ket::ToUpperAscii(s)
ket::EqualsIgnoreCaseAscii(a, b)
```

注意:

- `std::format` / fmt の再実装はしない
- ASCII限定関数は名前に `Ascii` を入れる
- `string_view` を使う場合、C++17以上が必要

---

### 8.14 encoding — UTF-8 / wchar_t / native encoding

目的: 文字コード境界の面倒を隔離する。

候補:

```cpp
ket::IsAscii(s)
ket::IsUtf8(s)
ket::ValidateUtf8(s)
ket::Utf8Length(s)
ket::Utf8ToWide(s)
ket::WideToUtf8(s)
ket::NativePathString(path)
ket::AsciiEqualsIgnoreCase(a, b)
```

注意:

- Windows API、`wchar_t`、`TCHAR`、Shift_JIS、UTF-8境界は混乱しやすい
- platform依存処理は `platform` と分ける
- 仕様が大きくなりすぎるため、最初はASCII/UTF-8の小さい検査・変換に留めるのが安全

---

### 8.15 format — 表示・診断文字列

目的: ログ、エラーメッセージ、診断用文字列を短く作る。

候補:

```cpp
ket::ToString(value)
ket::ToHexString(value)
ket::ToBinaryString(value)
ket::FormatBytes(size)
ket::FormatDuration(duration)
ket::DebugString(value)
ket::HexDump(bytes)
ket::Print(...)
```

注意:

- `StrCat` と `format` の境界を整理する
- C++20 `std::format` がある環境では薄いラッパでもよい
- 例外・メモリ確保を伴うので `noexcept` にしない場合が多い

---

### 8.16 parse — 文字列から値へ

目的: `from_chars` / `strtol` / enum table などの儀式を短く安全にする。

候補:

```cpp
ket::TryParseInt<T>(text, &out)
ket::TryParseUInt<T>(text, &out)
ket::TryParseHex<T>(text, &out)
ket::TryParseBool(text, &out)
ket::TryParseEnum(text, table, &out)
ket::TryParseBcd(byte, &out)
ket::ParseInt<T>(text)
ket::ParseUInt<T>(text)
ket::ParseHex<T>(text)
ket::ParseBool(text)
ket::ParseEnum(text, table)
ket::ParseBcd(byte)
ket::ParseKeyValue(text, delimiter)
ket::ParseCsvLine(text)
```

注意:

- parse成功条件は「完全消費」を基本にする
- 空白を許すかどうかをAPI名または仕様で固定する
- 10進/16進/符号あり/符号なしの境界を明確にする

---

### 8.17 enum — enum変換・flags

目的: enum class と文字列・整数・flagsの変換を短くする。

候補:

```cpp
ket::ToUnderlying(e)
ket::EnumEntry<E>
ket::EnumName(value, table)
ket::ParseEnum(text, table)
ket::IsValidEnumValue(value, table)
ket::HasFlag(flags, flag)
ket::SetFlag(flags, flag)
ket::ClearFlag(flags, flag)
ket::AnyFlag(flags, mask)
ket::AllFlags(flags, mask)
```

例:

```cpp
enum class Mode {
    kAuto,
    kManual,
};

constexpr ket::EnumEntry<Mode> kModeNames[] = {
    {Mode::kAuto, "auto"},
    {Mode::kManual, "manual"},
};
```

注意:

- magic_enum のような巨大reflectionにはしない
- table-basedで明示的にする方がdrop-in性が高い

---

### 8.18 container — map/vector/setの小さい儀式

目的: 標準コンテナ利用時の反復パターンを短くする。

候補:

```cpp
ket::Contains(container, value)
ket::ContainsKey(map, key)
ket::FindPtr(container, pred)
ket::FindOptional(container, pred)
ket::IndexOf(container, value)
ket::EraseIf(container, pred)
ket::Append(dst, src)
ket::SortUnique(vec)
ket::AtOrNull(map, key)
ket::AtOrOptional(map, key)
ket::GetOrDefault(map, key, default_value)
ket::GetOrCreate(map, key, factory)
```

注意:

- `std::vector` や `std::map` を置き換えない
- C++20以降の `contains` や ranges と重なるが、C++11〜17でも価値がある

---

### 8.19 algorithm — std::algorithm の儀式除去

目的: iterator pair を毎回書かず、range単位で標準アルゴリズム的処理を行う。

候補:

```cpp
ket::AllOf(range, pred)
ket::AnyOf(range, pred)
ket::NoneOf(range, pred)
ket::CountIf(range, pred)
ket::FindIf(range, pred)
ket::ContainsIf(range, pred)
ket::MinMax(range)
ket::TransformToVector(range, f)
ket::ForEachIndex(range, f)
ket::AdjacentPairs(range, f)
```

注意:

- 標準アルゴリズムの単なる別名になりすぎるなら入れない
- 「業務コードが短く読める」場合に限る

---

### 8.20 range — iterator pair / ranges / views

目的: index付き走査、zip、chunkなど、range処理の小さい便利部品。

候補:

```cpp
ket::IndexRange(n)
ket::Enumerate(range)
ket::Zip(a, b)
ket::Reverse(range)
ket::Take(range, n)
ket::Drop(range, n)
ket::Chunk(range, n)
ket::Slide(range, n)
```

注意:

- range-v3のような巨大世界を作らない
- C++20 ranges がある環境では標準機能を尊重する
- C++11〜17で本当に欲しい小さいものだけに絞る

---

### 8.21 view — span/string_view/byte_view

目的: 非所有ビューを安全に扱う。

候補:

```cpp
ket::ByteView
ket::MutableByteView
ket::Span<T>
ket::MakeByteView(ptr, size)
ket::MakeSpan(ptr, size)
ket::Subspan(view, offset, count)
ket::SafeAt(view, index)
ket::AsBytes(span)
```

注意:

- viewは所有しないため寿命が危険
- APIコメントで lifetime を強く明記する
- C++20では `std::span` を活用してよい

---

### 8.22 tuple — pair / tuple / struct-like utilities

目的: tuple/pair操作の小さい面倒をまとめる。

候補:

```cpp
ket::TupleForEach(tuple, f)
ket::TupleTransform(tuple, f)
ket::Apply(f, tuple)
ket::First(pair)
ket::Second(pair)
```

注意:

- tuple系は読みにくくなりやすい
- 少数精鋭にする
- C++17以降の structured binding と競合しないようにする

---

### 8.23 variant — variant / any / type erasure

目的: `std::variant` / `std::any` 周りの儀式を減らす。

候補:

```cpp
ket::Overload{...}
ket::Visit(overload, variant)
ket::Match(variant, handlers...)
ket::Holds<T>(variant)
ket::GetIf<T>(variant)
ket::AnyCastPtr<T>(any)
```

注意:

- `function` module と重なる部分がある
- variantを使う側の可読性を上げるために入れる

---

### 8.24 cache — lazy / memoize / once

目的: 「なければ作る」「一度だけ作る」「結果を覚える」を安全にする。

候補:

```cpp
ket::Lazy<T>
ket::OnceValue<T>
ket::Memoize(f)
ket::GetOrCreate(map, key, factory)
ket::CachedCall(key, factory)
ket::Invalidate(cache, key)
```

注意:

- thread-safeかどうかをAPI名・ドキュメントで明示する
- concurrencyに依存しすぎない

---

### 8.25 pointer — raw pointer / smart pointer / ownership

目的: nullable、所有、借用、smart pointer変換の意図を明確にする。

候補:

```cpp
ket::NotNull<T*>
ket::Owner<T*>
ket::LockWeak(weak_ptr)
ket::DynamicPtrCast<T>(ptr)
ket::StaticPtrCast<T>(ptr)
ket::GetOrNull(optional_like)
ket::AddressOf(obj)
ket::DeleteAndNull(ptr)
```

注意:

- `NotNull` と `Owner` は意味を明示する型であり、過信しない
- smart pointerの所有権ルールを曖昧にしない

---

### 8.26 error — optional / result / status

目的: 失敗する処理の戻り値を読みやすくする。

候補:

```cpp
ket::Result<T, E>
ket::Status
ket::StatusOr<T>
ket::Ok()
ket::Err(error)
ket::MapOptional(opt, f)
ket::AndThen(opt, f)
ket::OrElse(opt, f)
ket::ValueOrEval(opt, f)
ket::HasValueAll(...)
ket::FirstError(...)
```

注意:

- C++17以前では `std::optional` がない
- C++23では `std::expected` と重なる
- 独自エラー体系を重くしすぎない
- parse系は最初 `bool TryXxx(..., out*)` でもよい

---

### 8.27 scope — RAII / defer / cleanup

目的: 後始末処理を安全にする。

候補:

```cpp
ket::ScopeExit
ket::Finally(f)
ket::RestoreOnExit(var)
ket::SetOnExit(var, value)
ket::DismissableScopeExit
```

例:

```cpp
auto guard = ket::ScopeExit{[&] {
    cleanup();
}};
```

注意:

- destructor から例外を投げない
- callbackの寿命と参照キャプチャに注意

---

### 8.28 debug — assert / log / hexdump / diagnostics

目的: デバッグ・診断の小さい処理をまとめる。

候補:

```cpp
ket::Assert(condition)
ket::Unreachable()
ket::HexDump(bytes)
ket::DebugString(value)
ket::TypeName<T>()
ket::SourceLocation
ket::DumpBytes(bytes)
ket::TraceValue(name, value)
```

注意:

- `HexDump` は bytes/format とも関連する
- C++20以降は `std::source_location` を使える
- ログフレームワークは作らない。ログ文字列・診断補助に留める

---

### 8.29 testing — test helper / golden data / table test

目的: ket自身と業務コードの小さいテスト補助。

候補:

```cpp
ket::ExpectBytesEq(actual, expected)
ket::ExpectHexEq(actual, "00 01 FF")
ket::TableTest(cases, f)
ket::MakeTempFile()
ket::GoldenText(path)
ket::GoldenBytes(path)
ket::GenerateCases(...)
```

注意:

- テストフレームワークそのものは作らない
- バイナリ・BCD・parse系のテスト補助は価値が高い

---

### 8.30 time — 日時・duration・timeout

目的: 日付妥当性、duration変換、timeout処理を短くする。

候補:

```cpp
ket::IsLeapYear(year)
ket::DaysInMonth(year, month)
ket::IsValidDate(year, month, day)
ket::IsValidTime(hour, minute, second)
ket::IsValidDateTime(...)
ket::Stopwatch
ket::Deadline
ket::HasExpired(deadline)
ket::Remaining(deadline)
ket::FormatDuration(duration)
ket::ToMilliseconds(duration)
ket::ToSeconds(duration)
```

注意:

- C++20以降の calendar/date と重なるが、C++11〜17でも需要がある
- wall-clock と monotonic clock を混ぜない
- timeoutでは `steady_clock` を優先する

---

### 8.31 concurrency — thread / mutex / atomic / future

目的: 並行処理の小さい事故を減らす。

候補:

```cpp
ket::ThreadJoiner
ket::JoiningThread
ket::LockGuardIf(mutex, condition)
ket::AtomicFlagGuard
ket::Synchronized<T>
ket::CallOnce(flag, f)
ket::FutureReady(future)
ket::WaitUntilDeadline(...)
```

注意:

- thread poolやasync frameworkを作らない
- join忘れ、lock忘れ、timeout計算など局所的な事故防止に絞る

---

### 8.32 io — stream / binary/text IO

目的: stream状態管理や確実な読み書きを簡単にする。

候補:

```cpp
ket::ReadExactly(stream, buffer)
ket::ReadLineTrimmed(stream)
ket::WriteAll(stream, bytes)
ket::StreamStateSaver
ket::FlushGuard
ket::ToStringStream(value)
ket::FromStringStream<T>(text)
```

注意:

- `file` より広い概念
- iostreamのformat flag復元などは `StreamStateSaver` で扱える

---

### 8.33 file — file/path

目的: ファイル・パス周りの小さい処理。

候補:

```cpp
ket::ReadAllText(path)
ket::ReadAllBytes(path)
ket::WriteAllText(path, text)
ket::WriteAllBytes(path, bytes)
ket::FileExists(path)
ket::DirectoryExists(path)
ket::PathJoin(a, b)
ket::Extension(path)
ket::Stem(path)
```

注意:

- C++17以降は `std::filesystem` がある
- C++11〜14ではOS差が大きい
- platform依存が出やすいため、drop-in条件を明記する

---

### 8.34 cli — argc/argv / option

目的: 小さいコマンドラインツールを書く時の儀式を減らす。

候補:

```cpp
ket::ArgvView
ket::HasOption(args, "--help")
ket::GetOption(args, "--port")
ket::GetRequiredOption(args, "--id")
ket::OptionUInt<T>(args, "--port")
ket::OptionEnum(args, "--mode", table)
ket::Positional(args)
```

注意:

- Boost.Program_optionsのような大規模機能を目指さない
- 小さい社内ツールで十分便利な程度にする

---

### 8.35 platform — OS境界

目的: OS依存APIの境界を薄く包む。

候補:

```cpp
ket::ErrnoMessage(errno_value)
ket::WindowsErrorMessage(error_code)
ket::GetLastErrorCode()
ket::CurrentThreadIdString()
ket::SleepFor(duration)
ket::EnvironmentVariable(name)
```

注意:

- OS依存を隠すが、巨大platform layerにはしない
- Windows専用、POSIX専用のmoduleは分けてもよい
- drop-in条件を必ず明記する

---

### 8.36 c_interop — C API / handle / errno / char\* boundary

目的: C APIとの境界で発生するout parameter、buffer size、errno、handle管理を安全にする。

候補:

```cpp
ket::CStringView
ket::NullTerminated
ket::OutParam<T>
ket::InOutParam<T>
ket::ErrnoGuard
ket::HandleGuard
ket::ArraySize(array)
ket::SpanFromCArray(array)
ket::CopyToCBuffer(dst, dst_size, src)
ket::UniqueHandle<Handle, Deleter>
```

注意:

- `platform` より一般的なC境界
- `char*`、`void*`、size、errno、out param は毎回事故りやすい

---

### 8.37 build_config — compiler / OS / feature detection

目的: C++11〜23、OS、コンパイラ差分を扱う。

候補:

```cpp
KET_CXX_VERSION
KET_COMPILER_GCC
KET_COMPILER_CLANG
KET_COMPILER_MSVC
KET_OS_WINDOWS
KET_OS_LINUX
KET_OS_MAC
KET_HAS_CPP17
KET_HAS_CPP20
KET_HAS_STD_OPTIONAL
KET_HAS_STD_STRING_VIEW
KET_HAS_STD_SPAN
KET_HAS_STD_FORMAT
KET_HAS_STD_EXPECTED
```

注意:

- マクロはグローバル汚染なので最小限にする
- 各moduleが完全独立を目指すなら、build_configに依存しない選択肢もある
- ただしC++11〜23を本気で跨ぐなら必要になりやすい

---

### 8.38 domain — 汎用ドメイン型

目的: 業務固有ではないが、よく出る小さい値型を扱う。

候補:

```cpp
ket::Bcd
ket::IpV4
ket::MacAddress
ket::Port
ket::Uuid
ket::SemanticVersion
ket::ColorRgb
ket::Percent
ket::ByteSize
```

候補API:

```cpp
ket::ParseIpV4(text)
ket::FormatIpV4(ip)
ket::ParseMacAddress(text)
ket::FormatMacAddress(mac)
ket::ParseSemanticVersion(text)
ket::CompareVersion(a, b)
```

注意:

- URLやUUIDなどは仕様が大きい
- 完全実装ではなく、小さい値型 + parse/format に留める

---

### 8.39 state — state machine / transition table

目的: 状態遷移の妥当性と読みやすさを支える。

候補:

```cpp
ket::Transition<State, Event>
ket::TransitionTable<State, Event>
ket::IsValidTransition(current, event, table)
ket::NextState(current, event, table)
ket::StateName(state, table)
ket::EventName(event, table)
```

注意:

- Boost.Statechartのような巨大FSM frameworkを目指さない
- table-basedで小さく、テストしやすくする

---

### 8.40 recipes — 組み合わせスニペット

目的: moduleを組み合わせた実用パターンを保存する。

recipesはライブラリ本体ではなく、「こう使うと良い」という実例。

候補:

```txt
recipes/bcd_datetime/
recipes/binary_payload/
recipes/command_parser/
recipes/enum_table/
recipes/tcp_hexdump/
recipes/safe_file_copy/
recipes/c_api_wrapper/
recipes/state_transition_table/
```

例:

```txt
BCD日時を読む
バイナリpayloadを作る
argc/argvからenumとuint32_tをparseする
受信bufferをHexDump付きでログに出す
C APIのhandleをRAIIで包む
状態遷移表を使ってイベントを処理する
```

---

## 9. API設計規約

### 9.1 `TryXxx` と `Xxx` の使い分け

失敗しうる処理は、C++11互換を考えるなら `TryXxx` を中核にする。

```cpp
bool TryParseBcd(std::uint8_t value, int* out) noexcept;
bool TryParseUInt(const char* text, std::uint32_t* out) noexcept;
```

C++17以降ではoptional-returnを追加してよい。

```cpp
std::optional<int> ParseBcd(std::uint8_t value) noexcept;
std::optional<std::uint32_t> ParseUInt(std::string_view text) noexcept;
```

### 9.2 `Or` suffix

fallbackを受け取るAPIは `Or` suffix を検討する。

```cpp
ket::GetOrDefault(map, key, default_value)
ket::ValueOrEval(opt, factory)
ket::ParseUIntOr(text, fallback)
```

### 9.3 ASCII限定は名前に出す

悪い例:

```cpp
ket::ToLower(s)
```

良い例:

```cpp
ket::ToLowerAscii(s)
ket::EqualsIgnoreCaseAscii(a, b)
```

文字コードを誤解させない。

### 9.4 endianは名前に出す

悪い例:

```cpp
ket::ReadU32(ptr)
```

良い例:

```cpp
ket::ReadU32Be(ptr)
ket::ReadU32Le(ptr)
ket::LoadBe32(ptr)
ket::LoadLe32(ptr)
```

### 9.5 所有・非所有を名前に出す

```cpp
ket::ByteView          // non-owning
ket::MutableByteView   // non-owning mutable
ket::Bytes             // owning builder/container-like
ket::FunctionRef       // non-owning callable reference
ket::Owner<T*>         // owning raw pointer annotation
ket::NotNull<T*>       // non-null invariant
```

view/ref系は寿命の注意を必ず書く。

---

## 10. 実装規約

### 10.1 include what you use

各moduleは、自分が使う標準ヘッダを自分でincludeする。

```cpp
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
```

他moduleのincludeに依存しない。

### 10.2 using namespace禁止

公開ヘッダで `using namespace std;` は禁止。

### 10.3 グローバルマクロは最小限

許容される可能性のあるもの:

```cpp
KET_HAS_STD_OPTIONAL
KET_HAS_STD_STRING_VIEW
KET_HAS_STD_SPAN
KET_CXX_VERSION
```

ただし、module独立性を優先し、必要な場合にだけ導入する。

### 10.4 例外方針

- 低レベル変換・判定は例外を投げない
- parse失敗は原則として戻り値で表現する
- 文字列生成・vector拡張など、標準ライブラリのallocationが絡むものは例外を投げうる
- `noexcept` は無理につけない

### 10.5 未定義動作を避ける

特に注意:

- signed overflow
- shift count overflow
- unaligned access
- strict aliasing
- object lifetime
- dangling view
- null pointer
- out-of-bounds

ket の目的は「小さい便利」ではなく「小さい正解」である。

---

## 11. テスト方針

ket の価値は、テスト済みの小さい正解であることにある。

各moduleは最低限、境界値テストを持つ。

例: BCD

```txt
0x00 -> 0
0x09 -> 9
0x10 -> 10
0x99 -> 99
0x0A -> invalid
0xA0 -> invalid
0xFF -> invalid
```

例: AlignUp

```txt
AlignUp(0, 4)  -> 0
AlignUp(1, 4)  -> 4
AlignUp(4, 4)  -> 4
AlignUp(5, 4)  -> 8
alignment == 0 -> invalid or documented behavior
```

例: endian

```txt
{0x12, 0x34} -> LoadBe16 == 0x1234
{0x12, 0x34} -> LoadLe16 == 0x3412
StoreBe16(0x1234) -> {0x12, 0x34}
StoreLe16(0x1234) -> {0x34, 0x12}
```

### 11.1 テストで仕様を固定する

C++の小さい処理は、実装を見ると簡単に見える。  
しかし、境界条件が曖昧なままだと再利用できない。

GoogleTestの各 `TEST` にはDoxygen形式の試験仕様コメントを書く。
通常の関数引数を持たないmacroなので、関数用の `@param` と `@retval` は要求しない。

必須タグ:

- `@test`
- `@brief`
- `@details`
- `@pre`
- `@post`

`@brief` は試験目的、`@details` は入力条件と期待結果を1から2文で書く。
`@pre`、`@post` は、テストの前提とテスト後に変わらない性質が分かる文章にする。
テスト本文の全入力値をコメントへ重複列挙しすぎない。

テストは「このmoduleの仕様書」でもある。

### 11.2 複数標準バージョンでのコンパイル確認

可能なら、各moduleは複数のC++標準でコンパイル確認する。

```txt
C++11
C++14
C++17
C++20
C++23
```

ただし、moduleごとに要求標準は異なってよい。

---

## 12. catalog.md の役割

ket は最初から全てを作るのではなく、思い出せないロジックを継続的に蓄積する。

`catalog.md` は候補APIの保管場所にする。

記入テンプレート:

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
````

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

---

## 13. ket に入れる基準

候補処理は、次の条件を満たすほど ket 向きである。

```txt
1. C++で二度以上調べる
2. 直書きすると意図より手続きが目立つ
3. 書き間違えると危ない
4. 標準ライブラリだけだと儀式が長い
5. 名前にすると読みやすくなる
6. .h/.cpp単位で持ち出せる
7. 内部依存を持たなくても成立する
8. 業務固有ではない
9. 実装が読めるサイズに収まる
10. テストケースで仕様を固定できる
````

逆に、次のものは ket には向かない。

```txt
巨大フレームワーク
標準コンテナの置き換え
独自string型
独自vector型
独自スマートポインタ体系
業務固有ロジック
過度なtemplate magic
内部依存が深いmodule
ビルドシステム前提の部品
```

---

## 14. コーディングエージェントへの実装指示

実装時は、次を守ること。

1. いきなり巨大な共通基盤を作らない
2. moduleは原則として単独でコピー可能にする
3. 他のket moduleへの依存を増やさない
4. 小さい重複は許容する
5. 公開APIは `namespace ket` に置く
6. 公開ヘッダで必要な標準ヘッダをすべてincludeする
7. 失敗条件・境界条件をテストで固定する
8. C++標準バージョン要求をファイル先頭に書く
9. 便利さより、読みやすさ・安全性・持ち出しやすさを優先する
10. 「名前を見れば中身が想像できる」APIだけを採用する

---

## 15. ket の美学

ket は、C++を嫌ってC++を隠すためのライブラリではない。

ket は、C++を愛しているからこそ、C++の中で何度も出会う小さな苦痛に名前を与えるためのライブラリである。

C++には、ビット、型、寿命、所有権、文字列、parse、時刻、並行性、OS境界、C API境界など、無数の細かい面倒がある。  
その一つ一つは小さい。  
しかし、業務コードに散らばると読みづらく、壊れやすく、何度も調べ直すことになる。

ket はそれを回収する。

> **ket は、C++で生きてきた中で何度も出会った小さい苦痛に、すべて名前を与えて供養するプロジェクトである。**

この思想を壊さないこと。
