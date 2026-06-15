# ket catalog

候補APIの保管場所です。

ここに書くことは実装予定ではありません。C++で何度も調べる、書き間違えると危ない、
標準ライブラリだけでは儀式が長い、という小さな痛みを失わないために記録します。
`docs/module_api_catalog.md` に仕様カードがある module では、canonical name は
`docs/module_api_catalog.md` を正とします。

## 記入テンプレート

````md
## Idea: AlignUp

Category: numeric / memory

Pain:

- `(value + alignment - 1) / alignment * alignment` を毎回書きたくない
- alignment == 0 や overflow が怖い

Candidate API:

```cpp
ket::numeric::AlignUp(value, alignment)
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
ket::bcd::ToInt(value)
ket::bcd::FromInt<std::uint8_t>(value)
ket::bcd::FromInt<std::uint16_t>(value)
ket::bcd::FromInt<std::uint32_t>(value)
ket::bcd::Format(data, size)
ket::bcd::Parse(text)
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

- ToInt(0x00) == 0
- ToInt(0x09) == 9
- ToInt(0x10) == 10
- ToInt(0x99) == 99
- ToInt(0x0A) == std::nullopt
- FromInt<std::uint8_t>(42) == 0x42
- FromInt<std::uint8_t>(100) == std::nullopt
- FromInt<std::uint16_t>(1234) == 0x1234
- FromInt<std::uint32_t>(20260613) == 0x20260613
- Format({0x00, 0x42}) == "0042"
- Parse("0042") == {0x00, 0x42}
- Parse("123") == {0x01, 0x23}
- Parse("12A4") == std::nullopt

## Idea: String

Category: string

Pain:

- `std::string::append` を複数行に分けて書くと、業務意図より連結手順が目立つ
- `operator+` 連鎖では一時文字列や型の違いを意識しやすい
- 文字列片の連結と既存文字列への追記を、formatではない小さな意図として名前にしたい

Candidate API:

```cpp
ket::string::Cat(...)
ket::string::Append(dst, ...)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view`を利用でき、文字列片連結を標準ライブラリのみで安全に薄く包める
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：書式付き文字列生成ではC++20 `std::format`。文字列片の単純連結と追記の直接代替ではない。

Failure / edge cases:

- empty argument list
- `char` as one-character part
- embedded NUL in length-aware `std::string_view`
- raw C string must be non-null and null-terminated
- destination reference for `Append` must refer to a valid `std::string`
- self-reference such as `Append(dst, dst, std::string_view(dst))`

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- Cat() == ""
- Cat("a", std::string("b"), std::string_view("c"), 'd') == "abcd"
- Cat(std::string_view("a\\0b", 3)) preserves 3 bytes
- Append(dst, ...) appends to existing text
- Append(dst) keeps dst unchanged
- Append(dst, dst, ":", std::string_view(dst)) uses pre-append content

## Idea: Port

Category: network / value

Pain:

- TCP/UDP port番号の範囲確認を毎回書きたくない
- `int` や `uint16_t` の裸値だけだと、0や65535超過の扱いが呼び出し側ごとにぶれやすい
- CLIや設定値から読むときの空白、符号、leading zero、overflowの扱いを固定したい
- socket addressやservice name解決までは不要だが、値型として意図を読みたい

Candidate API:

```cpp
ket::port::Port
ket::port::TryFromUInt(value, out)
ket::port::Parse(text)
ket::port::Format(port)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view`と`std::optional`で範囲外や不正文字を明確に扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし。標準ライブラリにport番号の値型やparse/formatの直接APIなし。

Failure / edge cases:

- value > 65535
- empty string
- leading / trailing whitespace
- `+` / `-`
- non-digit character
- integer overflow
- multi-digit leading zero

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- TryFromUInt(0) succeeds
- TryFromUInt(65535) succeeds
- TryFromUInt(65536) fails
- Parse("0") succeeds
- Parse("65535") succeeds
- Parse("65536") fails
- Parse(" 80") fails
- Parse("+80") fails
- Parse("080") fails
- Format(Port{80}) == "80"

## Idea: Bits

Category: numeric / binary

Pain:

- bit、nibble、mask の手書き処理は shift 幅や符号で壊れやすい
- bit index 範囲外、mask 幅 0/full/超過の扱いが呼び出し側ごとにぶれやすい
- rotate や popcount を小さい名前付き処理として持ち出したい

Candidate API:

```cpp
ket::bits::HighNibble(value)
ket::bits::LowNibble(value)
ket::bits::TryPackNibbles(high, low, out)
ket::bits::HasBit(value, bit_index)
ket::bits::TryMask(width, out)
ket::bits::Rotl(value, count)
ket::bits::PopCount(value)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：unsigned integral の小さい bit 処理を標準ライブラリだけで安全に名前付けできる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：C++20 `<bit>` と一部重なるが、nibble と失敗値付き mask の直接代替ではない

Failure / edge cases:

- bit index 範囲外
- width 0 / bit幅 full / bit幅超過
- 不正 nibble
- signed integral は対象外

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- HighNibble(0xAB) == 0x0A
- LowNibble(0xAB) == 0x0B
- HasBit(value, 0) / 最上位 / 範囲外
- TryMask(0) succeeds with 0
- TryMask(BitWidth<T>()) succeeds with all bits set
- rotate count 0 / bit幅以上

## Idea: Numeric

Category: numeric

Pain:

- align、round up、checked arithmetic を毎回手書きすると overflow を見落としやすい
- signed overflow を避ける実装を毎回思い出したくない
- narrowing cast の範囲確認を呼び出し側ごとにばらけさせたくない

Candidate API:

```cpp
ket::numeric::InRange<To>(value)
ket::numeric::Clamp(value, min_value, max_value)
ket::numeric::AbsDiff(a, b)
ket::numeric::TryDivideRoundUp(value, divisor, out)
ket::numeric::TryAlignUp(value, alignment, out)
ket::numeric::TryAdd(a, b, out)
ket::numeric::TrySub(a, b, out)
ket::numeric::TryMul(a, b, out)
ket::numeric::TryCast<To>(value, out)
ket::numeric::SaturatingAdd(a, b)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：overflow と範囲外を戻り値で固定し、手書き算術の未定義動作を避けられる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- alignment == 0
- divisor == 0
- signed / unsigned overflow
- signed 最小値を含む AbsDiff
- bool / character 型の不採用
- cast 範囲外

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- TryAlignUp(0, 4) == 0
- TryAlignUp(max, 2) fails when overflow
- TryAdd(max, 1) fails
- TrySub(min, 1) fails
- TryMul boundary cases
- TryCast<std::uint8_t>(255) succeeds
- TryCast<std::uint8_t>(256) fails
- bool / char の compile-only 不採用確認

## Idea: Endian

Category: binary

Pain:

- endian 読み書きで `reinterpret_cast` や unaligned access に頼りたくない
- BE/LE が名前に出ない `ReadU32` は誤用しやすい
- size不足や null を失敗値で扱う Try API もほしい

Candidate API:

```cpp
ket::endian::ByteSwap32(value)
ket::endian::LoadBe32(data)
ket::endian::LoadLe32(data)
ket::endian::StoreBe16(data, value)
ket::endian::TryLoadBe32(data, size, out)
ket::endian::TryStoreLe64(data, size, value)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：endian と unaligned access の意図を名前に出し、strict aliasing 依存を避けられる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：C++20 `std::endian` は判定であり、byte列読み書きの直接代替ではない

Failure / edge cases:

- Load/Store の pointer と必要長は precondition
- TryLoad/TryStore の null
- size不足
- Try 失敗時の out / buffer 不変
- unaligned address 相当

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- BE/LE 16/32/64 golden bytes
- ByteSwap16/32/64
- TryLoad null / size不足
- TryStore null / size不足
- 失敗時 out / buffer 不変

## Idea: Hex

Category: diagnostic / binary

Pain:

- bytes と16進文字列の相互変換を毎回書くと大文字小文字、separator、不正文字の扱いが揺れる
- hex dump の形式が呼び出し側ごとにばらけやすい
- 整数の固定幅 hex 表記を診断用に短く書きたい

Candidate API:

```cpp
ket::hex::Encode(data, size, options)
ket::hex::Decode(text)
ket::hex::Dump(data, size, options)
ket::hex::Encode(value, width)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view` と `std::optional` で不正入力を明確に扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- null + 非0 size
- 空入力
- 奇数桁 hex
- 不正 hex 文字
- separator の扱い
- dump 形式の固定

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- empty bytes
- upper/lower hex output
- separator あり/なし
- Decode odd length fails
- invalid character fails
- Dump golden output

## Idea: ParseNumeric

Category: parsing / numeric

Pain:

- `std::from_chars` の完全消費、overflow、空白拒否を毎回書きたくない
- bool や hex parse の受け付け文字列を固定したい
- `std::optional` 版と bool+out 版を用途で選びたい

Candidate API:

```cpp
ket::parse::TryUInt<T>(text, out)
ket::parse::TryInt<T>(text, out)
ket::parse::TryHex<T>(text, out)
ket::parse::TryBool(text, out)
ket::parse::UInt<T>(text)
ket::parse::Int<T>(text)
ket::parse::Hex<T>(text)
ket::parse::Bool(text)
ket::parse::UIntOr<T>(text, fallback)
ket::parse::IntOr<T>(text, fallback)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::from_chars` 周辺の儀式を標準ライブラリだけで薄く包める
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：`std::from_chars` は部品であり、完全消費や bool 方針の直接代替ではない

Failure / edge cases:

- empty
- leading / trailing whitespace
- non-digit
- overflow / underflow
- hex prefix
- bool は case-sensitive

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- "0" / max value
- overflow
- " 1" and "1 " fail
- "1x" fails
- hex prefix acceptance policy
- Bool("true") / "false" / uppercase rejection

## Idea: EnumTable

Category: enum

Pain:

- enum class と文字列の対応表を毎回書くと unknown、重複、flags の扱いが揺れる
- reflection までは不要だが、table ベースの変換を短くしたい
- enum の underlying value を安全に取りたい

Candidate API:

```cpp
ket::enums::Entry<Mode>
ket::enums::Name(value, table)
ket::enums::Parse<Mode>(text, table)
ket::enums::ToUnderlying(value)
ket::enums::HasFlag(value, flag)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view` と `std::optional` で unknown を小さく表現できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし。C++標準に enum reflection はない

Failure / edge cases:

- unknown enum
- unknown text
- duplicate table entry は先勝ち
- flags の underlying 演算
- case-sensitive parse

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- known value name
- unknown value
- Parse known / unknown
- duplicate table first wins
- HasFlag true / false

## Idea: Container

Category: container

Pain:

- map/vector の find 儀式が業務意図より目立つ
- not found の null/default/factory 方針を呼び出し側ごとにばらけさせたくない
- C++11 でも drop-in できる小さい helper がほしい

Candidate API:

```cpp
ket::container::Contains(range, value)
ket::container::ContainsKey(map, key)
ket::container::AtOrNull(map, key)
ket::container::AtOr(map, key, fallback)
ket::container::AtOrCreate(map, key, factory)
ket::container::EraseIf(container, predicate)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：container の小さい定型処理を標準ライブラリだけで名前付きにできる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：C++20 `contains` / `erase_if` と一部重なるが C++11〜17 では直接代替なし

Failure / edge cases:

- key not found
- factory は必要時だけ呼ぶ
- const / non-const map
- erase 件数
- predicate 例外伝播

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- Contains true / false
- ContainsKey true / false
- AtOrNull found / missing
- AtOr fallback
- AtOrCreate factory count
- EraseIf removed count

## Idea: StringAscii

Category: string

Pain:

- ASCII 限定の trim/case/split を Unicode 対応と誤解されない名前で使いたい
- `std::string_view` の lifetime と空要素の扱いを固定したい
- replace や starts/ends の小さい文字列処理を業務コードから分離したい

Candidate API:

```cpp
ket::ascii::Trim(text)
ket::ascii::SplitViews(text, delimiter)
ket::ascii::ToLower(text)
ket::ascii::ReplaceAll(text, from, to)
ket::ascii::StartsWith(text, prefix)
ket::ascii::EndsWith(text, suffix)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view` で non-owning split 結果を小さく扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：C++20 `starts_with` / `ends_with` と一部重なるが ASCII trim/split の直接代替ではない

Failure / edge cases:

- ASCII whitespace のみ
- UTF-8 byte は保持
- leading / trailing delimiter
- empty fields
- view lifetime
- allocation 例外

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- Trim empty / whitespace / normal
- SplitViews keeps empty fields
- ToLower leaves non-ASCII bytes unchanged
- ReplaceAll no match / repeated match
- StartsWith / EndsWith boundaries

## Idea: Scope

Category: RAII

Pain:

- early return や例外経路で cleanup / restore を漏らしたくない
- small RAII を毎回ローカル class で書きたくない
- move 後 inactive や dismiss の動作を固定したい

Candidate API:

```cpp
ket::scope::Exit
ket::scope::MakeExit(cleanup)
ket::scope::Restore<T>
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：cleanup と復元漏れを標準ライブラリだけで小さく防げる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- destructor 中の callable 例外は terminate
- dismiss 後は実行しない
- move 元は inactive
- 二重実行なし
- restore 先 lifetime

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- scope exit executes once
- dismiss skips cleanup
- move transfers cleanup
- callable throwing in destructor terminates
- Restore restores original value

## Idea: ByteReader

Category: binary

Pain:

- byte列の offset 更新と size不足チェックを毎回書きたくない
- 読み取り失敗時に offset が進むバグを避けたい
- endian module に依存せず drop-in できる reader がほしい

Candidate API:

```cpp
ket::byte_reader::Reader
reader.ReadU8(out)
reader.ReadBe16(out)
reader.ReadLe32(out)
reader.ReadBytes(count, out)
reader.Remaining()
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：byte列の逐次読み取りと offset 保持を小さい状態付きAPIにできる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- null + 非0 size は invalid reader
- empty
- size不足
- 成功時だけ offset 更新
- 失敗時 out 不変
- non-owning view lifetime

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- empty reader
- exact-size read
- short read fails
- offset stays on failure
- BE/LE golden values
- ReadBytes lifetime note

## Idea: ByteWriter

Category: binary

Pain:

- fixed buffer への書き込みで size不足や offset 更新順を間違えやすい
- write 失敗時に部分書き込みするバグを避けたい
- endian module に依存せず drop-in できる writer がほしい

Candidate API:

```cpp
ket::byte_writer::Writer
writer.WriteU8(value)
writer.WriteBe16(value)
writer.WriteLe32(value)
writer.WriteBytes(data, size)
writer.Remaining()
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：fixed buffer への逐次書き込みと offset 保持を小さい状態付きAPIにできる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- null + 非0 size は invalid writer
- size不足
- null source + 非0 size
- 成功時だけ offset 更新
- 失敗時 buffer 不変

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- empty buffer
- exact-size write
- short write fails
- offset and buffer unchanged on failure
- BE/LE golden bytes
- WriteBytes null + 0

## Idea: BytesBuilder

Category: binary

Pain:

- owning payload を作る時に push_back と endian 書き込みが散らばる
- reserve、Build 後の move、null data の扱いを固定したい
- protocol framework ではなく小さい builder がほしい

Candidate API:

```cpp
ket::bytes_builder::Builder
builder.AppendU8(value)
builder.AppendBe16(value)
builder.AppendBytes(data, size)
builder.Bytes()
builder.Build()
ket::bytes_builder::AppendU8(dst, value)
```

Canonical name は `docs/module_api_catalog.md` の `bytes_builder Module` を正とします。

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::vector<std::uint8_t>` を所有し、payload 構築を小さく名前付けできる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- allocation 例外
- null + 非0 size は precondition
- BE/LE fixed width
- reserve size
- Build 後の moved-from state

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- AppendU8 / BE / LE golden bytes
- AppendBytes empty
- reserve
- Build returns expected vector
- Build after move leaves valid object

## Idea: Date

Category: date

Pain:

- leap year、month/day、時刻範囲を毎回書くと境界値を間違えやすい
- timezone や calendar 完全実装までは不要
- C++11〜17 で Gregorian の小さい妥当性確認を使いたい

Candidate API:

```cpp
ket::date::IsLeapYear(year)
ket::date::TryDaysInMonth(year, month, out)
ket::date::IsValidDate(year, month, day)
ket::date::IsValidTime(hour, minute, second)
```

Canonical API in `docs/module_api_catalog.md`: `ket::date::TryDaysInMonth` is superseded by
`ket::date::TryGetDaysInMonth`.

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11〜17
- 推奨理由：C++17以前で Gregorian の小さい妥当性確認を標準ライブラリだけで扱える
- 本ライブラリの適用を推奨しない C++ バージョン：C++20以降
- 非推奨理由：C++20以降は `std::chrono` calendar を優先できる
- 標準代替：C++20 `std::chrono`

Failure / edge cases:

- year < 1
- month 0 / 13
- day 0 / 32
- leap day
- hour 24
- leap second は対象外

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- 2000 leap
- 1900 not leap
- 2024-02-29 valid
- 2023-02-29 invalid
- month 0 / 13
- hour 24 invalid
- C++11 compile-only

## Idea: Deadline

Category: time

Pain:

- timeout と elapsed time の `steady_clock` 儀式を毎回書きたくない
- system_clock と混ぜるバグを避けたい
- negative timeout や remaining time の方針を固定したい

Candidate API:

```cpp
ket::deadline::Stopwatch
ket::deadline::Deadline
ket::deadline::Deadline::After(timeout)
ket::deadline::Deadline::At(time_point)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：`std::chrono::steady_clock` に限定した timeout 処理を小さく扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- negative timeout は期限切れ
- zero timeout
- future deadline
- remaining の下限 0
- restart

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- Stopwatch elapsed is non-negative
- Restart resets start time
- Deadline zero expires
- Deadline future not expired
- Remaining clamps to zero

## Idea: Cli

Category: CLI

Pain:

- `argc/argv` の option 取得を小さい社内ツールで毎回書きたくない
- `--key value` と `--key=value`、重複、missing value の方針を固定したい
- shell parser や CLI framework までは不要

Candidate API:

```cpp
ket::cli::ArgvView
ket::cli::HasOption(args, "--help")
ket::cli::OptionValue(args, "--id")
ket::cli::OptionValueOr(args, "--mode", "default")
ket::cli::PositionalArguments(args)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view` で argv lifetime に依存する値を明示しながら扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- argc < 0
- argv == nullptr
- argv[i] == nullptr
- option name が "--" で始まらない
- missing value
- duplicate option は先勝ち

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- --help flag
- --id 123
- --id=123
- missing value
- duplicate option
- positional args

## Idea: ByteView

Category: view / binary

Pain:

- C++11〜17 で `std::span` 相当の non-owning byte view がほしい
- `nullptr + 0` と `nullptr + 非0` の扱いを明確にしたい
- slice や bounds check を失敗値で扱いたい

Candidate API:

```cpp
ket::byte_view::View
ket::byte_view::MutableView
view.TryAt(index, out)
view.TrySlice(offset, count, out)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11〜17
- 推奨理由：C++17以前で non-owning byte span の寿命と境界確認を小さく表現できる
- 本ライブラリの適用を推奨しない C++ バージョン：C++20以降
- 非推奨理由：C++20以降は `std::span` を優先できる
- 標準代替：C++20 `std::span`

Failure / edge cases:

- lifetime は呼び出し側責任
- nullptr + 0 は空 view
- nullptr + 非0 は invalid view
- bounds overrun
- slice 失敗時 out 不変

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- default view
- empty view
- invalid view
- TryAt bounds
- TrySlice success / failure
- mutable set

## Idea: Utf8

Category: text

Pain:

- UTF-8 validation を業務処理から隔離したい
- 最初の不正 byte offset を返す方針を固定したい
- grapheme や normalization までは扱わない小さい検査がほしい

Candidate API:

```cpp
ket::utf8::Validate(text)
ket::utf8::IsValid(text)
ket::utf8::CountCodePoints(text)
ket::utf8::IsAscii(text)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view` と `std::optional` で UTF-8 検査結果と失敗位置を小さく扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：標準ライブラリに UTF-8 byte列検証の直接APIなし

Failure / edge cases:

- overlong sequence
- surrogate
- truncated sequence
- bad continuation byte
- code point 範囲外
- empty は valid

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- ASCII
- 2/3/4 byte sequence
- empty
- overlong
- truncated
- surrogate
- bad continuation

## Idea: File

Category: filesystem

Pain:

- ファイル全読み/全書きの失敗方針を毎回書くと長い
- bytes/text と error_code optional detail の扱いを固定したい
- filesystem wrapper 全体ではなく小さい read/write がほしい

Candidate API:

```cpp
ket::file::TryReadAllText(path, out, error)
ket::file::TryReadAllBytes(path, out, error)
ket::file::TryWriteAllText(path, text, error)
ket::file::TryWriteAllBytes(path, data, size, error)
ket::file::Size(path)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::filesystem` と標準文字列/vector で path と bytes/text の扱いを標準型へ寄せられる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：標準ライブラリだけでは全読み/全書きの失敗方針が長くなりやすい

Failure / edge cases:

- not found
- permission
- directory path
- huge file
- short write
- error_code\* は optional detail

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- empty file
- text
- binary
- missing file
- directory path
- error_code set / ignored

## Idea: IoStream

Category: stream

Pain:

- stream の exact read/write と state 復元を毎回書くと失敗条件が曖昧になりやすい
- short read を成功扱いしたくない
- ASCII line trim 程度の小さい stream 補助がほしい

Candidate API:

```cpp
ket::io_stream::TryReadExactly(stream, data, size)
ket::io_stream::TryWriteAll(stream, data, size)
ket::io_stream::StateSaver
ket::io_stream::TryReadLineTrimmed(stream, out)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：stream の確実な読み書きと状態復元を小さいAPIで固定できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- short read
- write failure
- null buffer + 非0 size
- stream exception
- state restore
- ASCII trim only

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- exact read success
- short read fails
- write all success
- stream state restored
- trim line ASCII whitespace
- exception propagation

## Idea: FormatValue

Category: diagnostic

Pain:

- bool、binary、byte count、duration の診断表記を毎回決めたくない
- locale や logging framework ではなく固定 ASCII 表記がほしい
- golden output で表記を固定したい

Candidate API:

```cpp
ket::format::Bool(value)
ket::format::Binary(value, min_width)
ket::format::ByteCount(bytes)
ket::format::Duration(duration)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：診断用の固定表記を標準ライブラリのみで小さく提供できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：`std::format` は書式化部品であり、固定診断表記の直接代替ではない

Failure / edge cases:

- allocation 例外
- binary min_width
- IEC 1024 byte units
- negative duration
- decimal rounding
- extreme duration

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- Bool true / false
- Binary zero / width / overflow width
- ByteCount 0 / 1024 / 1536
- Duration ns / us / negative / hour
- max values

## Idea: AlgorithmRange

Category: algorithm

Pain:

- index 付き range 走査を毎回手書きしたくない
- not found の out 不変を固定したい
- `std::algorithm` の単なる別名ではなく index を意図に出したい

Candidate API:

```cpp
ket::ranges::ForEachWithIndex(range, f)
ket::ranges::FindIndexIf(range, predicate, out)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11〜17
- 推奨理由：C++17以前で index 付き range 走査を小さく書ける
- 本ライブラリの適用を推奨しない C++ バージョン：C++20以降
- 非推奨理由：C++20以降は `std::ranges` を優先できる
- 標準代替：C++20 ranges

Failure / edge cases:

- empty range
- not found
- predicate exception
- out 不変
- const / non-const element reference

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- empty
- index order
- const / non-const elements
- first match
- not found out unchanged
- predicate exception propagation

## Idea: Memory

Category: memory

Pain:

- pointer alignment と object representation 読み取りを意図が見える名前にしたい
- secure zero の best-effort 方針を毎回書きたくない
- object lifetime 操作や type punning には踏み込みたくない

Candidate API:

```cpp
ket::memory::IsAligned(ptr, alignment)
ket::memory::TryAlignUp(ptr, alignment, out)
ket::memory::Zero(ptr, size)
ket::memory::SecureZero(ptr, size)
ket::memory::ObjectBytes(object)
ket::memory::ObjectByteSize(object)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：pointer alignment と object representation の意図を小さいAPIへ分離できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：secure zero と pointer alignment の直接代替は標準だけでは不足する

Failure / edge cases:

- alignment 0
- non-power-of-two alignment
- null pointer
- nullptr + 0 zeroing
- trivially copyable 制約
- secure zero best-effort

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- aligned / unaligned
- alignment 0
- null + 0
- zeroing
- secure zero writes bytes
- object byte size

## Idea: Pointer

Category: pointer

Pain:

- raw pointer の null 不許可を型で表したい
- weak_ptr lock の expired 分岐を名前で固定したい
- overloaded `operator&` を避ける意図を短く書きたい

Candidate API:

```cpp
ket::pointer::NotNull<T>
ket::pointer::LockWeak(weak)
ket::pointer::AddressOf(value)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：null許容性と所有権の有無を型名や関数名で明示できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- NotNull(nullptr) throws
- non-owning lifetime
- weak expired
- overloaded operator&

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- nullptr rejected
- dereference / operator->
- weak alive / expired
- AddressOf ignores overloaded operator&

## Idea: TestingBytes

Category: testing / binary

Pain:

- byte列比較の失敗時に offset と期待/実際値が読みづらい
- hex 文字列との比較を test helper に閉じ込めたい
- production library には依存させたくない

Candidate API:

```cpp
ket::testing::BytesEq(expected, actual)
ket::testing::HexEq(hex, actual)
```

Canonical API in `docs/module_api_catalog.md`: `ket::testing::BytesEq` and
`ket::testing::HexEq` are superseded by `ket::testing::BytesEqual` and
`ket::testing::HexEqual`.

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：GoogleTest v1.17.0 のC++17要件に合わせ、byte列差分を読みやすく表示できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- length mismatch
- first differing offset
- invalid hex
- GoogleTest dependency
- test-helper only

他のライブラリへの依存:

- GoogleTest
- 標準ライブラリ
- ket本体依存なし

Tests:

- equal bytes
- mismatch offset
- length mismatch
- HexEq valid
- HexEq invalid hex failure message

## Idea: SemanticVersion

Category: parsing / value

Pain:

- simple numeric version triplet の parse/compare を毎回書きたくない
- full SemVer ではない範囲を名前と仕様で固定したい
- leading zero や overflow を統一したい

Candidate API:

```cpp
ket::semver::Version
ket::semver::Parse(text)
ket::semver::Compare(a, b)
ket::semver::Format(value)
```

Canonical API in `docs/module_api_catalog.md`: the `ket::semver` candidates are superseded by
`ket::version::Triplet`, `ket::version::Parse`, `ket::version::Compare`, and
`ket::version::Format`.

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view` と `std::optional` で numeric triplet の失敗を明確に扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- numeric triplet only
- leading zero
- overflow
- missing / extra component
- prerelease / build metadata は対象外

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- 0.0.0
- normal parse
- compare major/minor/patch
- leading zero fails
- overflow fails
- format golden output

## Idea: Ipv4

Category: network / value

Pain:

- IPv4 dotted decimal の parse/format を毎回書きたくない
- octet 境界、個数不足/過多、leading zero の扱いを固定したい
- IPv6/CIDR/DNS までは不要

Candidate API:

```cpp
ket::ipv4::Address
ket::ipv4::Parse(text)
ket::ipv4::Format(address)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view` と `std::optional` で dotted decimal の失敗を明確に扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- octet > 255
- empty component
- too few / too many components
- leading zero
- whitespace
- sign

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- 0.0.0.0
- 255.255.255.255
- octet overflow
- missing / extra component
- leading zero fails
- format golden output

## Idea: MacAddress

Category: network / value

Pain:

- MAC address の区切りと hex parse を毎回書きたくない
- `:` と `-` は許しつつ混在や Cisco 形式は拒否したい
- format の大文字小文字を固定したい

Candidate API:

```cpp
ket::mac::Address
ket::mac::Parse(text)
ket::mac::Format(address)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view` と `std::optional` で fixed length address の失敗を明確に扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- invalid hex
- separator mix
- too few / many octets
- Cisco dotted form
- upper/lower input

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- colon format
- hyphen format
- upper/lower input
- mixed separator fails
- invalid hex fails
- format golden output

## Idea: Function

Category: callable

Pain:

- `std::visit` 用 overload helper を毎回書きたくない
- Noop callable を引数破棄用途で小さく使いたい
- `std::function` や signal framework までは不要

Candidate API:

```cpp
ket::function::Overload
ket::function::MakeOverload(...)
ket::function::Noop
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：variadic using declaration で visitor overload set を小さく表現できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- callable copy/move constraints
- handler exception propagation
- overload resolution
- Noop noexcept

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- std::visit with variants
- overload resolution
- return value
- Noop accepts arbitrary args
- copy / move constraints

## Idea: VariantMatch

Category: variant

Pain:

- `std::variant` visitor の overload helper を利用箇所に散らしたくない
- reference category と const を保った thin wrapper がほしい
- pattern matching DSL にはしたくない

Candidate API:

```cpp
ket::variant::Match(variant, handlers...)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::variant` と visitor補助を標準ライブラリのみで薄く包める
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- unhandled alternative is compile error
- handler exception propagation
- const / non-const
- lvalue / rvalue
- void / non-void return

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- value alternatives
- reference preservation
- const variant
- rvalue variant
- exception propagation
- missing handler compile error

## Idea: OptionalExt

Category: optional

Pain:

- optional の map / and_then / lazy fallback を C++17 で小さく使いたい
- factory が必要時だけ呼ばれることを固定したい
- C++23 optional API と衝突しない名前で補いたい

Candidate API:

```cpp
ket::optional::Map(opt, mapper)
ket::optional::AndThen(opt, mapper)
ket::optional::ValueOrEval(opt, fallback_factory)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：C++17の `std::optional` に小さい合成処理を足せる
- 本ライブラリの適用を推奨しない C++ バージョン：API別
- 非推奨理由：C++23で `transform` / `and_then` が標準化されたため API ごとに判断する
- 標準代替：C++23 `std::optional::transform` / `std::optional::and_then`

Failure / edge cases:

- mapper exception propagation
- mapper called only when value exists
- fallback factory called only when empty
- rvalue move
- reference_wrapper retention
- non-optional mapper for AndThen is compile error

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- value / empty
- mapper call count
- factory laziness
- return type
- rvalue move
- mapper exception propagation

## Idea: Contract

Category: contract

Pain:

- precondition / postcondition / invariant 違反の方針を局所的に固定したい
- debug/release で消えない契約 check がほしい
- macro 式の1回評価と null check の戻り値形を固定したい

Candidate API:

```cpp
KET_EXPECTS(condition)
KET_ENSURES(condition)
KET_ASSERT_INVARIANT(condition)
KET_REQUIRE_NON_NULL(ptr)
ket::contract::CheckBounds(index, size)
```

Canonical API in `docs/module_api_catalog.md`: `ket::contract::CheckBounds` is superseded by
`ket::contract::IsInBounds`.

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：契約違反時のプロジェクト方針を小さいAPIへ閉じ込められる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：C++ contracts は標準化状況と利用可能性が安定していない

Failure / edge cases:

- contract violation terminates
- expression evaluated once
- NDEBUG でも評価
- null pointer
- file/expression null
- death test は message 完全一致に依存しない

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- valid path
- KET_EXPECTS death
- KET_ENSURES death
- KET_REQUIRE_NON_NULL success / failure
- expression one-time evaluation
- NDEBUG compile path

## Idea: CInterop

Category: interop

Pain:

- C API 境界で errno 復元、C buffer copy、handle cleanup を毎回書きたくない
- sentinel handle ではなく engaged flag で所有を明示したい
- deleter 例外や buffer 不足時の状態を固定したい

Candidate API:

```cpp
ket::interop::ErrnoGuard
ket::interop::CopyStringToBuffer(dst, dst_size, src, src_size)
ket::interop::CopyBytesToBuffer(dst, dst_size, src, src_size)
ket::interop::UniqueHandle<Handle, Deleter>
```

Canonical name は `docs/module_api_catalog.md` の `interop Module` を正とします。

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：errno保存、C buffer copy、handle cleanup の事故をC API境界に閉じ込められる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- dst_size == 0
- null pointer
- src truncation
- failure leaves dst unchanged
- deleter throws terminate
- default constructor requires default constructible deleter
- release after non-engaged is precondition

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- errno restore
- string copy success / insufficient
- bytes copy
- failure leaves buffer unchanged
- UniqueHandle reset / release / move / self-move
- throwing deleter terminates

## Idea: PlatformError

Category: platform

Pain:

- errno message と Windows error message の platform 差を利用箇所に出したくない
- environment variable の missing / invalid name 方針を固定したい
- localization や error category framework までは不要

Candidate API:

```cpp
ket::platform::FormatErrno(error_number)
ket::platform::GetEnvironmentVariable(name)
#ifdef _WIN32
ket::platform::GetLastErrorCode()
ket::platform::FormatWindowsError(code)
#endif
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：platform API の差分を隠しすぎず、標準文字列で結果を扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：標準ライブラリだけでは errno/Windows error message の扱いが不十分

Failure / edge cases:

- unknown errno fallback
- missing env
- empty / NUL env name
- POSIX/GNU strerror_r 差
- Windows wide to UTF-8 conversion failure
- non-Windows では Windows API を宣言しない

他のライブラリへの依存:

- 標準ライブラリ
- platform API
- ket依存なし

Tests:

- known errno non-empty
- unknown errno fallback
- missing env
- present env with restore
- empty / NUL env name
- Windows guard conditional compile

## Idea: StateTable

Category: state

Pain:

- 小さい状態遷移表の known/unknown と duplicate 方針を毎回書きたくない
- FSM framework ではなく table lookup だけがほしい
- enum class と組み合わせて未定義遷移を失敗値にしたい

Candidate API:

```cpp
ket::state::Transition<State, Event>
ket::state::IsAllowed(state, event, table)
ket::state::Next(state, event, table)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：小さい状態遷移表を標準ライブラリのみで固定できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- undefined transition
- duplicate transition first wins
- table order
- unknown enum value
- no actions / guards

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- known transition
- unknown transition
- duplicate first wins
- enum class use
- table empty

## Idea: CacheOnce

Category: cache

Pain:

- lazy value の factory 呼び出し回数と例外後状態を固定したい
- C++11 で `std::optional` なしに object 内 storage を使いたい
- thread-safe cache や global registry にはしたくない

Candidate API:

```cpp
ket::cache::Lazy<T>
lazy.HasValue()
lazy.GetOrCreate(factory)
lazy.Reset()
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：lazy value の thread-safety と例外後状態を局所的に固定できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- factory exception leaves empty
- Reset destroys value
- move-only value
- copy/move of Lazy disabled
- destructor exception terminates
- reentrancy precondition

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- factory called once
- reset regenerates
- exception leaves empty
- move-only value
- address stability
- copy/move disabled compile-only

## Idea: SerializationTlv

Category: binary / serialization

Pain:

- small TLV record の BE header と length 境界を毎回書きたくない
- decode 失敗時に out を壊したくない
- schema language や protocol framework までは不要

Candidate API:

```cpp
ket::tlv::Encode(type, value, value_size)
ket::tlv::Append(dst, type, value, value_size)
ket::tlv::TryDecode(data, size, out)
ket::tlv::View
ket::tlv::DecodeResult
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::vector<std::uint8_t>` と bool+out で wire format 境界を固定できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- header shorter than 6 bytes
- declared length exceeds remaining size
- null + 0 value
- null + non-zero precondition
- size_t overflow
- decode failure leaves out unchanged
- view lifetime

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- empty value
- roundtrip
- multiple records decode first
- short header / value
- big-endian golden bytes
- max uint32 length header
- out unchanged on failure

## Idea: Tuple

Category: tuple

Pain:

- tuple 要素ごとの副作用呼び出しや変換を毎回 index_sequence で書きたくない
- heterogeneous tuple の const/reference を保ちたい
- reflection や DSL にはしたくない

Candidate API:

```cpp
ket::tuple::ForEach(tuple, f)
ket::tuple::Transform(tuple, f)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::apply` と fold expression を使い、tuple走査を小さく実装できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：標準ライブラリだけでは tuple要素ごとの副作用呼び出しが冗長

Failure / edge cases:

- empty tuple
- heterogeneous types
- const tuple
- reference elements
- callable exception propagation
- evaluation order

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- empty
- heterogeneous
- const
- reference preservation
- transform return tuple type
- call order

## Idea: BuildConfig

Category: config / build

Pain:

- compiler、OS、standard library feature macro の差を利用側に散らしたくない
- macro 値を 0/1 に揃えたい
- project policy switch ではなく feature detection だけがほしい

Candidate API:

```cpp
KET_CXX_VERSION
KET_CXX_AT_LEAST(value)
KET_HAS_STD_OPTIONAL
KET_HAS_STD_STRING_VIEW
KET_HAS_STD_SPAN
KET_HAS_STD_FORMAT
KET_COMPILER_CLANG
KET_OS_LINUX
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：compiler/OS/feature macro の差分を小さい範囲へ閉じ込められる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- MSVC `_MSVC_LANG`
- clang-cl
- unknown OS
- `__has_include` absence
- feature-test macro differences
- include order independence

他のライブラリへの依存:

- 標準ライブラリ
- compiler predefined macros
- ket依存なし

Tests:

- C++11 compile-only
- all macros defined
- values are 0/1
- KET_CXX_AT_LEAST boundaries
- compiler/OS mutual exclusion
- include order

## Idea: MathSmall

Category: math / numeric

Pain:

- lerp、角度変換、近似比較、byte単位変換を毎回小さく書きたくない
- floating-point 限定や NaN 方針を固定したい
- units framework ではなく数個の名前付き helper がほしい

Candidate API:

```cpp
ket::math::Lerp(a, b, t)
ket::math::ToRadians(degrees)
ket::math::ToDegrees(radians)
ket::math::NearlyEqual(a, b, epsilon)
ket::math::TryBytesFromKiB(kib, out)
ket::math::ToKiB(bytes)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：小さい数学処理の丸め、overflow、単位名をAPIで固定できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- floating-point 型限定
- epsilon <= 0
- NaN
- Inf
- byte conversion overflow
- out 不変

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- endpoints
- midpoint
- angle roundtrip
- epsilon
- NaN
- byte conversion overflow
- large byte values

## Idea: Language

Category: language

Pain:

- C++11/14 の小さい言語儀式を業務コードから薄く隠したい
- unused、array size、as const の意図を名前にしたい
- macro 大量追加や attribute framework にはしたくない

Candidate API:

```cpp
ket::lang::IgnoreUnused(args...)
ket::lang::ArraySize(array)
ket::lang::AsConst(value)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：C++11/14の欠落や冗長な言語儀式を小さいAPIで名前付けできる
- 本ライブラリの適用を推奨しない C++ バージョン：API別
- 非推奨理由：APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない
- 標準代替：C++17 `std::size` / `std::as_const` / `[[maybe_unused]]`

Failure / edge cases:

- array reference only
- AsConst lifetime
- unused expression side effects
- C++11 compile

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- unused compiles
- array length
- const conversion
- C++11 compile-only

## Idea: Object

Category: object

Pain:

- copy/move 禁止や move 後 reset の意図を型定義の近くで明示したい
- mixin が比較演算や object size に悪影響を出さないようにしたい
- regular type framework にはしたくない

Candidate API:

```cpp
ket::object::NonCopyable
ket::object::NonMovable
ket::object::MoveOnly
ket::object::ResetOnMove<T>
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：copy/move意図を型定義の近くへ集約できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- copy disabled compile
- move behavior
- reset source state
- empty base optimization
- defaulted comparison interaction
- noexcept depends on T

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- copy forbidden compile-only
- move works
- reset on move
- empty base size
- defaulted comparison coexistence

## Idea: Meta

Category: meta

Pain:

- C++11/14 で欠けている小さい type traits を局所的に補いたい
- template diagnostics を読みやすくする helper がほしい
- concepts 代替体系や難読化する meta framework にはしたくない

Candidate API:

```cpp
ket::meta::RemoveCvref<T>
ket::meta::TypeIdentity<T>
ket::meta::AlwaysFalse<Ts...>
ket::meta::VoidT<Ts...>
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：C++11/14で不足する小さいtraitsを局所的に補える
- 本ライブラリの適用を推奨しない C++ バージョン：API別
- 非推奨理由：APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない
- 標準代替：C++17 `std::void_t`、C++20 `std::remove_cvref` / `std::type_identity`

Failure / edge cases:

- alias type correctness
- SFINAE
- no runtime behavior
- standard alternatives in later C++

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- alias type equality
- AlwaysFalse false_type
- SFINAE usage
- C++11 compile-only

## Idea: ConcurrencySmall

Category: concurrency

Pain:

- `std::thread` の join 漏れを小さい RAII で防ぎたい
- future ready 判定で deferred を ready 扱いするかを固定したい
- thread pool / executor までは不要

Candidate API:

```cpp
ket::concurrency::JoiningThread
ket::concurrency::IsReady(future)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：thread join と future ready 判定の小さい儀式を局所化できる
- 本ライブラリの適用を推奨しない C++ バージョン：API別
- 非推奨理由：`JoiningThread` は C++20 `std::jthread` と一部重なるため API ごとに判断する
- 標準代替：C++20 `std::jthread`

Failure / edge cases:

- destructor blocks until join
- self-join precondition
- move assignment joins old thread first
- join exception terminates
- invalid future precondition
- deferred is not ready

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- default
- joinable
- move / self-move
- old thread joined on move assignment
- ready / not ready / deferred
- C++11 compile-only

## Idea: Uuid

Category: parsing / value

Pain:

- canonical UUID の parse/format を毎回書きたくない
- generation や OS randomness なしで固定形式だけ扱いたい
- version/variant validation をしない方針を明示したい

Candidate API:

```cpp
ket::uuid::Uuid
ket::uuid::Parse(text)
ket::uuid::Format(uuid)
```

C++バージョン要件:

- 最小要件：C++17
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降
- 推奨理由：`std::string_view` と `std::optional` で canonical 形式の失敗を明確に扱える
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- bad length
- bad hyphen position
- invalid hex
- uppercase input accepted
- brace / URN form rejected
- generation not included

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- zero uuid
- normal uuid
- upper input
- bad length
- bad hyphen
- bad hex
- format lower-case

## Idea: ColorRgb

Category: value / color

Pain:

- RGB hex の parse/format と alpha 非対応を小さい値型で固定したい
- `#` 任意、case 混在、不正長の扱いを毎回書きたくない
- CSS color name や color space 変換までは不要

Candidate API:

```cpp
ket::color::Rgb
ket::color::TryParse(text, out)
ket::color::Format(color, options)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：RGB小値型の許容表記と不正hexを小さいAPIで固定できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- invalid length
- invalid hex
- optional leading #
- 3-digit shorthand rejected
- alpha rejected
- format lowercase

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- black / white
- leading # present / absent
- upper / lower
- invalid length
- invalid character
- format with / without #

## Idea: Percent

Category: value / numeric

Pain:

- 0〜100% の値範囲、basis points、ratio 丸めを小さい値型で固定したい
- NaN や範囲外の扱いを毎回決めたくない
- progress UI や localization にはしない

Candidate API:

```cpp
ket::percent::Percent
ket::percent::Percent::TryFromBasisPoints(value, out)
ket::percent::Percent::TryFromPercent(value, out)
ket::percent::Percent::TryFromRatio(ratio, out)
ket::percent::Clamp(value)
```

C++バージョン要件:

- 最小要件：C++11
- 本ライブラリの適用を推奨する C++ バージョン：C++11以降
- 推奨理由：percent小値型の範囲、丸め、NaN方針を局所的に固定できる
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- basis points < 0 / > 10000
- ratio denominator 0
- negative ratio
- ratio > 1
- NaN
- rounding
- clamp boundaries

他のライブラリへの依存:

- 標準ライブラリのみ
- ket依存なし

Tests:

- 0%
- 100%
- negative fails
- > 100 fails
- ratio normal
- ratio denominator 0 fails
- rounding
- clamp

## Idea: BinaryPayloadRecipe

Category: recipe / binary

Pain:

- BCD、endian、byte_writer、hex を組み合わせた実用的な電文構築例がほしい
- module API を増やさず、組み合わせ方だけを示したい
- checksum や診断 hex の流れを小さい example で確認したい

Candidate API:

```cpp
recipes/binary_payload/binary_payload_example.cpp
```

C++バージョン要件:

- 最小要件：mixed
- 本ライブラリの適用を推奨する C++ バージョン：利用する実moduleの要件に従う
- 推奨理由：既存 module の組み合わせ例として利用するため
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- module API の失敗値処理
- payload size
- checksum
- expected hex
- recipe は新規 public API を追加しない

他のライブラリへの依存:

- 実装済み ket module
- 標準ライブラリ

Tests:

- example builds
- representative payload matches expected hex
- failure path handles module failure

## Idea: CommandParserRecipe

Category: recipe / CLI

Pain:

- CLI、parse、enums を組み合わせる小さい例がほしい
- CLI framework を作らず、option 取得と parse 失敗の扱いだけを示したい
- duplicate や missing option の扱いを example で確認したい

Candidate API:

```cpp
recipes/command_parser/command_parser_example.cpp
```

C++バージョン要件:

- 最小要件：mixed
- 本ライブラリの適用を推奨する C++ バージョン：利用する実moduleの要件に従う
- 推奨理由：既存 module の組み合わせ例として利用するため
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- missing option
- invalid enum
- invalid port
- duplicate option
- recipe は新規 public API を追加しない

他のライブラリへの依存:

- 実装済み ket module
- 標準ライブラリ

Tests:

- example builds
- representative argv succeeds
- invalid enum fails
- missing option path is covered

## Idea: CApiWrapperRecipe

Category: recipe / interop

Pain:

- `interop` と `scope` を使った C API 境界 RAII 化例がほしい
- fake C API で open/copy/cleanup/errno restore の失敗経路を見たい
- OS固有大規模 wrapper や新規 module API にはしたくない

Candidate API:

```cpp
recipes/c_api_wrapper/c_api_wrapper_example.cpp
```

C++バージョン要件:

- 最小要件：mixed
- 本ライブラリの適用を推奨する C++ バージョン：利用する実moduleの要件に従う
- 推奨理由：既存 module の組み合わせ例として利用するため
- 本ライブラリの適用を推奨しない C++ バージョン：なし
- 非推奨理由：なし
- 標準代替：なし

Failure / edge cases:

- open failure
- copy buffer不足
- cleanup execution
- errno restore
- recipe は新規 public API を追加しない

他のライブラリへの依存:

- 実装済み ket module
- 標準ライブラリ

Tests:

- example builds
- successful wrapper path
- open failure path
- cleanup and errno restore path
