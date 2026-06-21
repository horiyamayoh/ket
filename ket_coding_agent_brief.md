# ket Coding Agent Brief

この文書は、C++ユーティリティ集 **ket** を実装するコーディングエージェント向けの設計ブリーフです。
ここまでの会話で固まってきた思想・スコープ・実装方針・候補APIをまとめています。

重要: この文書は「ロードマップ」や「MVP計画」ではありません。  
今すぐ何から作るかではなく、**ket がどんな宇宙を持つライブラリなのか**を共有するための文書です。
`v1`、初期実装、最小APIという表現は、採用範囲を小さく保つ意味でだけ使います。
採用した関数1つ1つについて、設計、実装、Doxygen、テスト、検証の品質を下げてよいという意味ではありません。

---

## 1. ket の一文定義

> **ket は、C++で毎回調べる・毎回迷う・毎回バグる・毎回儀式が長い小さな処理を、短く安全に再利用できるAPIとして切り出す drop-in utility catalog である。**

もう少し実務寄りに言うと、

> **ket は、業務C++で繰り返し現れる低レベルな定型ロジックを、持ち出し可能な `.h/.cpp` 単位で集める小さな部品箱である。**

たとえば、利用側のコードからこういうノイズを消す。

```cpp
(value >> 4) & 0x0F
```

こう書きたい。

```cpp
auto hi = ket::bits::HighNibble(value);
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
return ket::container::AtOr(map, key, default_value);
```

ket の価値は、C++の機能を隠すことではなく、**C++で何度も書く小さな処理の意図を、短く安全なAPIとして明示すること**にある。

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
ket::bcd::ToInt(byte);
ket::string::Cat("id=", id, ", mode=", mode);
ket::numeric::TryAlignUp(size, 4U, aligned_size);
ket::endian::LoadBe32(ptr);
ket::enums::Name(mode, table);
ket::container::AtOrNull(map, key);
ket::scope::Exit{[&] { cleanup(); }};
ket::hex::Dump(buffer);
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
moduleを1つずつ増やす方針は、同時に扱うスコープを抑えるためのものです。
追加すると決めた関数については、初期版であっても実務で持ち出せる品質を要求します。

```txt
modules/<module_name>/ket_<module_name>.h
modules/<module_name>/ket_<module_name>.cpp  # 実装がある場合だけ
```

例:

```txt
modules/bcd/ket_bcd.h
modules/bcd/ket_bcd.cpp

modules/string/ket_string.h
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
ket_string.h だけで使える
```

やむを得ず依存する場合は、ファイル先頭に明記する。

```cpp
/**
 * @par プロジェクトへの適用方法
 * 必須: `ket_bytes.h`, `ket_bytes.cpp`。
 * 任意: なし。
 *
 * @par 他のライブラリへの依存
 * C++17以降。
 * 標準ライブラリのみ。
 */
```

ただし、最初の設計原則としては **標準ライブラリのみ** を目指す。

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
ket::container::Contains(vec, value);
ket::container::AtOrNull(map, key);
ket::string::Cat(...);
ket::parse::UInt<std::uint32_t>(text);
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
ket::bcd::Parse()
ket::bits::HighNibble()
ket::bits::LowNibble()
ket::numeric::TryAlignUp()
ket::endian::LoadBe32()
ket::string::Cat()
ket::scope::Exit
ket::enums::ToUnderlying()
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
bool TryParseUInt(const std::string& text, std::uint32_t& out) noexcept;

#if KET_HAS_STD_OPTIONAL
std::optional<std::uint32_t> ParseUInt(std::string_view text) noexcept;
#endif
```

C++11〜14対応を重視するmoduleでは、`TryXxx(..., out&) -> bool` を中核APIにする。
C++17以降では `std::optional<T>` を返す便利APIを追加してよい。

### 4.3 C++20/23固有の便利module

C++20/23だからこそ作れるmoduleもスコープに含める。

例:

```cpp
span-like helpers
calendar date helpers
duration formatting helpers
expected helpers
move-only function wrapper
concept-based overload guards
ranges helpers
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
modules/<name>/ket_<name>.cpp  # 実装がある場合だけ
modules/<name>/ket_<name>_test.cpp
```

header-onlyで十分なmoduleでは `.cpp` を置かず、READMEまたはファイル先頭に
`header-only` と明記する。空に近い `.cpp` を置き、`#include "ket_<name>.h"` だけ書く形は採らない。

### 6.2 ヘッダ先頭Doxygenコメント

各ヘッダには、Doxygen `@file`コメントとしてmoduleの概略、drop-in条件、C++バージョン要件、依存、namespaceを明記する。
Doxygenタグ以外のsection名と説明本文は日本語を基本にし、既存コメントと同じく簡潔な常体にする。

```cpp
#pragma once

/**
 * @file ket_bcd.h
 * @brief packed BCDと10進表現の変換API。
 *
 * @details 固定幅packed BCDと任意バイト長packed BCDを、整数または10進文字列へ相互変換する。
 * drop-in時は宣言と実装を同じ単位で持ち出す。標準ライブラリにpacked BCDの直接APIがないため、
 * BCD固有のnibble検証と桁保持をmodule内で扱う。
 *
 * @par プロジェクトへの適用方法
 * `ket_bcd.h` と `ket_bcd.cpp` を対象プロジェクトへコピー。
 *
 * @par C++バージョン要件
 * 最小要件：C++17。
 * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
 * 推奨理由：packed BCDの直接代替が標準ライブラリになく、`std::optional`で失敗値を明確に扱える。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket
 * 内部実装：ket::detail
 */

#include <cstdint>

namespace ket
{
    // ...
}
```

`本ライブラリの適用を推奨しない C++ バージョン` が `なし` 以外の場合は、`非推奨理由` に理由を書く。理由は原則として、対象C++標準では標準ライブラリで容易かつ明確に代替可能であることを示す。
module全体を非推奨にする意図ではないため、C++バージョン単位の非推奨表現にDoxygen `@deprecated` は使わない。

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
ket::bcd::Parse()
ket::bits::HighNibble()
ket::bits::LowNibble()
ket::string::Cat()
ket::numeric::TryAlignUp()
ket::endian::LoadBe32()
ket::scope::Exit
ket::byte_writer::Writer
```

型名も UpperCamelCase。

```cpp
ket::scope::Exit
ket::byte_view::View
ket::enums::Entry
ket::deadline::Deadline
```

定数は `k` prefix を使ってよい。

```cpp
constexpr std::uint32_t kMaxPort = 65535U;
```

`const` は左寄せにする。`const char* text` のように対象の型の左に置き、ポインタ自体を
constにする場合など意味があるときだけ `const char* const text` のように右側へ置く。

非optionalの出力引数と入出力引数は参照型で受ける。`nullptr` が「出力不要」などの意味を持つ
optional出力や、C API境界でraw pointerをそのまま扱う場合だけポインタ型を使う。ポインタ型を使う
公開APIでは、nullable性とポインタにする理由をDoxygenに書く。

`template <...>` 宣言の後は必ず改行し、対象の `struct`、`class`、関数宣言、関数定義を同じ行に
置かない。`.clang-format` の `AlwaysBreakTemplateDeclarations: Yes` で自動整形する。

公開ヘッダは、利用者が公開APIをすぐ把握できるように、`namespace ket` の先頭へ公開APIの
Doxygen付き宣言を並べる。続けて `ket::detail` の内部helperを書き、最後にinline、constexpr、
templateなどヘッダ内に必要な公開API定義を書く。公開API定義を後半に置く場合、Doxygenコメントは
宣言側だけに書き、定義側へ重複させない。各sectionには `Public API declarations`、
`Internal implementation details`、`Public API definitions` のdashed bannerコメントを置く。

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
ヘッダ内の公開inline関数、constexpr関数、template関数で宣言と定義を分ける場合は、
宣言側にDoxygenコメントを書く。宣言と定義を分けない小さな関数では、宣言兼定義の側に
Doxygenコメントを書く。
`.cpp`内だけで使うhelperは実装詳細なのでDoxygenコメントを書かない。

- `@brief`
- `@param[in]`、`@param[out]`、または `@param[in,out]`
- `@retval`
- `@pre`
- `@post`
- 公開API関数では `@code` と `@endcode`

必要な場合だけ `@note`、`@remark` を追加する。
公開API関数の `@code` は、入力例、戻り値、失敗時の形が一目で分かる短い例にする。
`ket::<module>::detail` の内部helperでは、必要な場合だけ `@code` を追加する。

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
bool TryParseBcd(std::uint8_t value, int& out) noexcept;
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
この章は旧案を含む候補メモであり、canonical name は `docs/module_api_catalog.md`
を正とする。

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
KET_EXPECTS(condition)
KET_ENSURES(condition)
KET_ASSERT_INVARIANT(condition)
KET_REQUIRE_NON_NULL(ptr)
ket::contract::IsInBounds(index, size)
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
ket::numeric::InRange<T>(value)
ket::numeric::Clamp(value, min, max)
ket::numeric::AbsDiff(a, b)
ket::numeric::TryDivideRoundUp(value, divisor, out)
ket::numeric::TryAlignUp(value, alignment, out)
ket::numeric::TryAlignDown(value, alignment, out)
ket::numeric::TryAdd(a, b, out)
ket::numeric::TrySub(a, b, out)
ket::numeric::TryMul(a, b, out)
ket::numeric::SaturatingAdd(a, b)
ket::numeric::SaturatingSub(a, b)
ket::numeric::TryCast<T>(value, out)
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
ket::endian::ByteSwap16(value)
ket::endian::ByteSwap32(value)
ket::endian::ByteSwap64(value)
ket::endian::LoadBe16(ptr)
ket::endian::LoadBe32(ptr)
ket::endian::LoadBe64(ptr)
ket::endian::LoadLe16(ptr)
ket::endian::LoadLe32(ptr)
ket::endian::LoadLe64(ptr)
ket::endian::StoreBe16(ptr, value)
ket::endian::StoreBe32(ptr, value)
ket::endian::StoreBe64(ptr, value)
ket::endian::StoreLe16(ptr, value)
ket::endian::StoreLe32(ptr, value)
ket::endian::StoreLe64(ptr, value)
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
ket::memory::IsAligned(ptr, alignment)
ket::memory::TryAlignUp(ptr, alignment, out)
ket::memory::TryAlignDown(ptr, alignment, out)
ket::memory::Zero(ptr, size)
ket::memory::SecureZero(ptr, size)
ket::memory::ObjectBytes(obj)
ket::memory::ObjectByteSize(obj)
```

注意:

- C++20の `std::construct_at` と重なる
- object representation と object lifetime は危険領域なので、API名とコメントで用途を限定する
- pointer alignment の戻り値は address-level の結果で、object bounds や dereference 可能性は保証しない
- `ObjectBytes` は padding/endian/layout を含むため、serialization や安定比較には使わない

---

### 8.11 bytes — payload / buffer / binary IO

目的: byte列構築・読み取り・書き込みを見通しよくする。

候補:

```cpp
ket::byte_view::View
ket::byte_view::MutableView
ket::byte_reader::Reader
ket::byte_writer::Writer
ket::bytes::Builder
reader.ReadU8(out)
reader.ReadBe16(out)
reader.ReadBe32(out)
writer.WriteU8(value)
writer.WriteBe16(value)
writer.WriteBe32(value)
builder.Append(data, size)
```

fluent interface 例:

```cpp
auto payload = ket::bytes::Builder{}
    .AppendU8(command)
    .AppendBe16(sequence)
    .AppendBe32(id)
    .Append(date_bcd, sizeof(date_bcd))
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
ket::DecodeLengthPrefixed(view, out)
ket::EncodeTlv(type, value)
ket::DecodeTlv(view, out)
ket::SerializeBe(value)
ket::DeserializeBe(view, out)
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
ket::string::Cat(...)
ket::string::Append(dst, ...)
ket::ascii::StartsWith(s, prefix)
ket::ascii::EndsWith(s, suffix)
ket::ascii::Contains(s, needle)
ket::ascii::Trim(s)
ket::ascii::TrimLeft(s)
ket::ascii::TrimRight(s)
ket::ascii::Split(s, delimiter)
ket::ascii::Join(parts, delimiter)
ket::ascii::ReplaceAll(s, from, to)
ket::ascii::StripPrefix(s, prefix)
ket::ascii::StripSuffix(s, suffix)
ket::ascii::ToLower(s)
ket::ascii::ToUpper(s)
ket::ascii::EqualsIgnoreCase(a, b)
```

注意:

- `std::format` / fmt の再実装はしない
- ASCII限定処理は `ket::ascii` module に置き、fully qualified name で前提を読めるようにする
- `string_view` を使う場合、C++17以上が必要

---

### 8.14 encoding — UTF-8 / wchar_t / native encoding

目的: 文字コード境界の面倒を隔離する。

候補:

```cpp
ket::utf8::IsAscii(s)
ket::utf8::IsValid(s)
ket::utf8::Validate(s)
ket::utf8::CountCodePoints(s)
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
ket::format::Bool(value)
ket::format::Binary(value)
ket::format::ByteCount(size)
ket::format::Duration(duration)
ket::hex::Dump(bytes)
```

注意:

- `ket::string::Cat` と `format` の境界を整理する
- C++20 `std::format` がある環境では薄いラッパでもよい
- 例外・メモリ確保を伴うので `noexcept` にしない場合が多い

---

### 8.16 parse — 文字列から値へ

目的: `from_chars` / `strtol` / enum table などの儀式を短く安全にする。

候補:

```cpp
ket::parse::TryInt<T>(text, out)
ket::parse::TryUInt<T>(text, out)
ket::parse::TryHex<T>(text, out)
ket::parse::TryBool(text, out)
ket::enums::Parse(text, table)
ket::bcd::ToInt(byte)
ket::parse::Int<T>(text)
ket::parse::UInt<T>(text)
ket::parse::Hex<T>(text)
ket::parse::Bool(text)
ket::bcd::Parse(text)
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
ket::enums::ToUnderlying(e)
ket::enums::Entry<E>
ket::enums::Name(value, table)
ket::enums::Parse(text, table)
ket::enums::IsValid(value, table)
ket::enums::HasFlag(flags, flag)
ket::enums::SetFlag(flags, flag)
ket::enums::ClearFlag(flags, flag)
ket::enums::HasAnyFlag(flags, mask)
ket::enums::HasAllFlags(flags, mask)
```

例:

```cpp
enum class Mode {
    kAuto,
    kManual,
};

constexpr ket::enums::Entry<Mode> kModeNames[] = {
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
ket::container::Contains(container, value)
ket::container::ContainsKey(map, key)
ket::container::EraseIf(container, pred)
ket::container::SortUnique(vec)
ket::container::AtOrNull(map, key)
ket::container::AtOr(map, key, default_value)
ket::container::AtOrCreate(map, key, factory)
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
ket::byte_view::View
ket::byte_view::MutableView
view.TrySlice(offset, count, out)
view.TryAt(index, out)
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
ket::tuple::ForEach(tuple, f)
ket::tuple::Transform(tuple, f)
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
ket::function::Overload{...}
ket::variant::Match(variant, handlers...)
```

注意:

- `function` module と重なる部分がある
- variantを使う側の可読性を上げるために入れる

---

### 8.24 cache — lazy / memoize / once

目的: 「なければ作る」「一度だけ作る」「結果を覚える」を安全にする。

候補:

```cpp
ket::cache::Lazy<T>
lazy.GetOrCreate(factory)
lazy.Reset()
```

注意:

- thread-safeかどうかをAPI名・ドキュメントで明示する
- concurrencyに依存しすぎない

---

### 8.25 pointer — raw pointer / smart pointer / ownership

目的: nullable、所有、借用、smart pointer変換の意図を明確にする。

候補:

```cpp
ket::pointer::NotNull<T>
ket::pointer::LockWeak(weak_ptr)
ket::pointer::AddressOf(obj)
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
- parse系は最初 `bool TryXxx(..., out&)` でもよい

---

### 8.27 scope — RAII / defer / cleanup

目的: 後始末処理を安全にする。

候補:

```cpp
ket::scope::Exit
ket::scope::MakeExit(f)
ket::scope::Restore<T>
ket::scope::MakeRestore(var)
```

例:

```cpp
auto guard = ket::scope::Exit{[&] {
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
ket::DebugString(value)
ket::TypeName<T>()
ket::SourceLocation
ket::hex::Dump(bytes)
ket::TraceValue(name, value)
```

注意:

- `ket::hex::Dump` は bytes/format とも関連する
- C++20以降は `std::source_location` を使える
- ログフレームワークは作らない。ログ文字列・診断補助に留める

---

### 8.29 testing — test helper / golden data / table test

目的: ket自身と業務コードの小さいテスト補助。

候補:

```cpp
ket::testing::BytesEq(expected, actual)
ket::testing::HexEq("00 01 FF", actual)
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
ket::interop::ErrnoGuard
ket::interop::CopyStringToBuffer(dst, dst_size, src, src_size)
ket::interop::CopyBytesToBuffer(dst, dst_size, src, src_size)
ket::interop::UniqueHandle<Handle, Deleter>
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
KET_CXX_AT_LEAST(value)
KET_COMPILER_GCC
KET_COMPILER_CLANG
KET_COMPILER_MSVC
KET_OS_WINDOWS
KET_OS_LINUX
KET_OS_MACOS
KET_HAS_STD_OPTIONAL
KET_HAS_STD_STRING_VIEW
KET_HAS_STD_SPAN
KET_HAS_STD_FORMAT
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
ket::ipv4::Address
ket::mac::Address
ket::port::Port
ket::uuid::Uuid
ket::version::Triplet
ket::color::Rgb
ket::percent::Percent
```

候補API:

```cpp
ket::ipv4::Parse(text)
ket::ipv4::Format(ip)
ket::mac::Parse(text)
ket::mac::Format(mac)
ket::version::Parse(text)
ket::version::Format(value)
ket::version::Compare(a, b)
```

注意:

- URLやUUIDなどは仕様が大きい
- 完全実装ではなく、小さい値型 + parse/format に留める

---

### 8.39 state — state machine / transition table

目的: 状態遷移の妥当性と読みやすさを支える。

候補:

```cpp
ket::state::Transition<State, Event>
ket::state::IsAllowed(current, event, table)
ket::state::Next(current, event, table)
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
bool TryParseBcd(std::uint8_t value, int& out) noexcept;
bool TryParseUInt(const char* text, std::uint32_t& out) noexcept;
```

C++17以降ではoptional-returnを追加してよい。

```cpp
std::optional<int> ParseBcd(std::uint8_t value) noexcept;
std::optional<std::uint32_t> ParseUInt(std::string_view text) noexcept;
```

### 9.2 `Or` suffix

fallbackを受け取るAPIは `Or` suffix を検討する。

```cpp
ket::container::AtOr(map, key, default_value)
ket::optional::ValueOrEval(opt, factory)
ket::parse::UIntOr(text, fallback)
```

### 9.3 ASCII限定は名前に出す

悪い例:

```cpp
ToLower(s)
```

良い例:

```cpp
ket::ascii::ToLower(s)
ket::ascii::EqualsIgnoreCase(a, b)
```

文字コードを誤解させない。

### 9.4 endianは名前に出す

悪い例:

```cpp
ket::endian::ReadU32(ptr)
```

良い例:

```cpp
ket::endian::LoadBe32(ptr)
ket::endian::LoadLe32(ptr)
```

### 9.5 所有・非所有を名前に出す

```cpp
ket::byte_view::View           // non-owning
ket::byte_view::MutableView    // non-owning mutable
ket::bytes::Builder            // owning builder
FunctionRef                    // non-owning callable reference
Owner<T*>                      // owning raw pointer annotation
ket::pointer::NotNull<T>       // non-null invariant
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
ket::numeric::TryAlignUp(value, alignment, out)
```
````

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
6. 公開ヘッダは、公開API宣言、内部helper、ヘッダ内公開API定義の順に書く
7. 公開ヘッダで必要な標準ヘッダをすべてincludeする
8. 失敗条件・境界条件をテストで固定する
9. C++標準バージョン要求をファイル先頭に書く
10. 便利さより、読みやすさ・安全性・持ち出しやすさを優先する
11. 「名前を見れば中身が想像できる」APIだけを採用する
12. `v1` や最小APIを理由に、採用した関数の品質、性能、安全性、ドキュメント、検証を軽く扱わない
13. 機能を増やさない判断と、実装品質を落とす判断を混同しない

---

## 15. ket の美学

ket は、C++を嫌ってC++を隠すためのライブラリではない。

ket は、C++を扱う現場で繰り返し発生する小さな負担を、短く安全なAPIとして切り出すためのライブラリである。

C++には、ビット、型、寿命、所有権、文字列、parse、時刻、並行性、OS境界、C API境界など、無数の細かい面倒がある。  
その一つ一つは小さい。  
しかし、業務コードに散らばると読みづらく、壊れやすく、何度も調べ直すことになる。

ket はそれを回収する。

> **ket は、C++で繰り返し発生する小さな負担を、短く安全に再利用できるAPIとして整理するプロジェクトである。**

この思想を壊さないこと。
