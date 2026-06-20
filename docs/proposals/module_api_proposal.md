# ket 発展提案: module / API 仕様案

作成日: 2026-06-13

## 0. この文書の目的

この文書は、C++ utility catalog `ket` を次に大きく育てるための module 候補と API 仕様案である。
この文書は旧案を含む履歴資料であり、canonical name は
`docs/module_api_catalog.md` を正とする。

ここでの目的は「module 数を増やすこと」ではない。目的は、ket の価値である **小さい正解を、持ち出しやすい `.h/.cpp` 単位で増やすこと** である。

そのため、候補は多めに挙げるが、実装時は次の順序を守る。

1. `catalog.md` に候補として記録する。
2. 実装するものだけ `modules/<name>/` を作る。
3. `.h/.cpp/test` と境界値テストを 1 セットで作る。
4. `progress.md` は実moduleとして着手したものだけ更新する。

---

## 1. ket の意図理解

ket は「しょうもない処理」を雑に集める場所ではない。ここで言う「しょうもない処理」とは、次のような処理である。

- C++で何度も書く。
- 毎回、書き方を思い出す。
- 書き間違えると境界値、未定義動作、寿命、所有権、overflow、encoding、endian などで壊れる。
- 標準ライブラリだけで書くと、本来読みたい業務意図より手続きが目立つ。
- しかし、大きなライブラリを入れるほどではない。
- `.h/.cpp` 単位で業務プロジェクトへコピーできると嬉しい。

ket が与えるべき価値は、C++を隠すことではなく、**C++の小さな意図に名前を与えること** である。

例:

```cpp
const auto value = ket::endian::LoadBe32(data);
std::size_t aligned = 0;
const auto aligned_ok = ket::numeric::TryAlignUp(size, 4U, aligned);
const auto mode = ket::enums::Parse<Mode>(text, kModeTable);
const auto name = ket::container::AtOr(names, id, "unknown");
```

この文書では、既存方針に合わせて次を重視する。

- 公開APIは `namespace ket`。
- API名は UpperCamelCase。
- 各moduleは原則として他の ket module に依存しない。
- 小さい重複は許容し、drop-in 性を優先する。
- 標準ライブラリを置き換えない。
- 失敗条件と境界条件をテストで固定する。
- `constexpr` / `noexcept` は無理なく付ける。
- ASCII、endian、ownership、lifetime など誤解されやすい条件は名前かコメントに出す。

---

## 2. 採用基準

候補APIは、次を多く満たすほど ket 向きである。

| 観点         | 判断基準                                                |
| ------------ | ------------------------------------------------------- |
| 反復性       | C++で2回以上書いた、または調べたことがある              |
| 可読性       | 名前にすると業務コードの意図が読める                    |
| 危険性       | 直書きだと overflow、UB、寿命、null、境界値で壊れやすい |
| 小ささ       | `.h/.cpp` で読めるサイズに収まる                        |
| 独立性       | 他の ket module に依存しなくても成立する                |
| 標準尊重     | `std::vector` や `std::string` を置き換えない           |
| テスト可能性 | 境界値・失敗条件を GoogleTest で固定できる              |
| 汎用性       | 業務固有名を含まない                                    |

入れないほうがよいもの:

- 独自 container / string / smart pointer 体系。
- thread pool、logging framework、CLI framework、serialization framework などの大きな世界。
- 仕様が巨大な protocol / URL / JSON / date-time timezone 完全実装。
- 「単なる `std::xxx` の別名」でしかないもの。
- 名前を見ても中身が想像できないもの。

---

## 3. API設計共通規約案

### 3.1 失敗表現

C++11/14 も視野に入る module では、まず `TryXxx(..., out*) -> bool` を中核にする。

```cpp
bool TryParseUInt(std::string_view text, std::uint32_t* out) noexcept;
bool TryLoadBe32(const std::uint8_t* data, std::size_t size, std::uint32_t* out) noexcept;
```

C++17 以降の module では `std::optional<T>` を返す便利APIを追加してよい。

```cpp
std::optional<std::uint32_t> ParseUInt(std::string_view text) noexcept;
std::optional<std::uint32_t> LoadBe32(std::span<const std::uint8_t> data) noexcept;
```

ただし、allocation を伴う API は `noexcept` を付けない。

```cpp
std::optional<std::vector<std::uint8_t>> HexToBytes(std::string_view text);
```

### 3.2 fallback は `Or` / `OrDefault`

```cpp
ket::container::AtOr(map, key, fallback)
ket::parse::UIntOr(text, fallback)
ket::optional::ValueOrEval(opt, factory)
```

### 3.3 ASCII 限定は名前に出す

```cpp
ket::ascii::ToLower(text)
ket::ascii::EqualsIgnoreCase(a, b)
ket::ascii::Trim(text)
```

`ToLower` のような名前は Unicode 対応と誤解されるため避ける。

### 3.4 endian は名前に出す

```cpp
ket::endian::LoadBe32(data)
ket::endian::LoadLe32(data)
writer.WriteBe16(value)
reader.ReadLe16(out)
```

`ReadU32` のような endian 不明APIは避ける。

### 3.5 所有・非所有・寿命は名前か型に出す

```cpp
ket::byte_view::View           // non-owning
ket::byte_view::MutableView    // non-owning mutable
ket::bytes_builder::Builder    // owning builder
ket::pointer::NotNull<T>       // non-null invariant
```

view/ref 系は lifetime を Doxygen に必ず書く。

### 3.6 迷う名前は、より説明的にする

避けたい例:

```cpp
ket::Fix(value)
ket::SmartParse(text)
ket::DoString(s)
ket::endian::ReadU32(data)
```

望ましい例:

```cpp
ket::numeric::TryAlignUp(value, alignment, out)
ket::parse::UInt<std::uint32_t>(text)
ket::ascii::Trim(text)
ket::endian::LoadBe32(data)
```

---

## 4. 優先ロードマップ

### P0: すぐに catalog 化し、順に実装したい核

BCD の次に ket の価値を最も表しやすい module 群。

1. `bits`
2. `numeric`
3. `endian`
4. `hex`
5. `parse_numeric`
6. `enum_table`
7. `container`
8. `string_ascii`
9. `scope`
10. `byte_reader`
11. `byte_writer`
12. `bytes_builder`
13. `date`
14. `deadline`
15. `cli`

### P1: 実務で強いが、仕様を小さく切る必要がある

1. `byte_view`
2. `utf8`
3. `file`
4. `io_stream`
5. `format_value`
6. `algorithm_range`
7. `memory`
8. `pointer`
9. `testing_bytes`
10. `semantic_version`
11. `ipv4`
12. `mac_address`

### P2: 便利だが、大きくなりやすいので抑制して入れる

1. `function`
2. `variant_match`
3. `optional_ext`
4. `contract`
5. `c_interop`
6. `platform_error`
7. `state_table`
8. `cache_once`
9. `serialization_tlv`
10. `tuple`
11. `build_config`
12. `math_small`

### P3: 保留・recipes 向き

1. `meta`
2. `concurrency_small`
3. `uuid`
4. `color_rgb`
5. `percent`
6. `recipes/binary_payload`
7. `recipes/command_parser`
8. `recipes/c_api_wrapper`
9. `recipes/state_transition_table`

---

## 5. module proposal 一覧

| Module              | 優先 | C++ Min  | 狙い                             | 代表API                                             |
| ------------------- | ---- | -------- | -------------------------------- | --------------------------------------------------- |
| `bcd`               | done | C++17    | packed BCD 変換                  | `ParseBcd`, `ToBcd8`, `BcdToDecimalString`          |
| `bits`              | P0   | C++11    | bit/nibble/mask の事故防止       | `HighNibble`, `HasBit`, `Mask`                      |
| `numeric`           | P0   | C++11    | overflow/align/cast の小さい正解 | `AlignUp`, `CheckedAdd`, `InRange`                  |
| `endian`            | P0   | C++11    | unaligned/endian 読み書き        | `LoadBe32`, `StoreLe16`                             |
| `hex`               | P0   | C++17    | bytes と16進文字列/hex dump      | `BytesToHex`, `HexToBytes`, `HexDump`               |
| `parse_numeric`     | P0   | C++17    | `from_chars` 周りの儀式除去      | `ParseUInt`, `TryParseHex`                          |
| `enum_table`        | P0   | C++17    | enum class と文字列変換          | `EnumName`, `ParseEnum`, `ToUnderlying`             |
| `container`         | P0   | C++11/17 | map/vector の小さい儀式          | `ContainsKey`, `AtOrNull`, `GetOrDefault`           |
| `string_ascii`      | P0   | C++17    | ASCII前提の文字列処理            | `TrimAscii`, `SplitView`, `ToLowerAscii`            |
| `scope`             | P0   | C++11    | RAII cleanup                     | `ScopeExit`, `MakeScopeExit`, `RestoreOnExit`       |
| `byte_reader`       | P0   | C++11    | byte列の安全な逐次読み取り       | `ReadU8`, `ReadBe16`, `Remaining`                   |
| `byte_writer`       | P0   | C++11    | fixed buffer への安全な書き込み  | `WriteU8`, `WriteLe32`, `Remaining`                 |
| `bytes_builder`     | P0   | C++17    | owning payload builder           | `AppendU8`, `AppendBe16`, `Build`                   |
| `date`              | P0   | C++11    | 日付・時刻の妥当性               | `IsLeapYear`, `IsValidDate`                         |
| `deadline`          | P0   | C++11    | timeout と elapsed time          | `Stopwatch`, `Deadline`                             |
| `cli`               | P0   | C++17    | 小さい社内CLIの option 取得      | `HasOption`, `GetOption`, `Positional`              |
| `byte_view`         | P1   | C++11    | non-owning byte span             | `ByteView`, `SubView`, `SafeAt`                     |
| `utf8`              | P1   | C++17    | UTF-8検査を小さく隔離            | `ValidateUtf8`, `IsUtf8`, `Utf8Length`              |
| `file`              | P1   | C++17    | ファイル全読み/全書き            | `ReadAllText`, `WriteAllBytes`                      |
| `io_stream`         | P1   | C++11    | stream の確実な読み書き          | `ReadExactly`, `StreamStateSaver`                   |
| `format_value`      | P1   | C++17    | 診断用文字列                     | `ToHexString`, `FormatBytes`, `FormatDuration`      |
| `algorithm_range`   | P1   | C++11    | iterator pair の儀式除去         | `AllOf`, `FindIf`, `IndexOf`                        |
| `memory`            | P1   | C++11    | alignment/object bytes           | `IsAligned`, `ObjectBytes`, `SecureZeroMemory`      |
| `pointer`           | P1   | C++11    | null/ownership の明示            | `NotNull`, `LockWeak`, `AddressOf`                  |
| `testing_bytes`     | P1   | C++17    | bytes系テスト補助                | `BytesEq`, `HexEq`                                  |
| `semantic_version`  | P1   | C++17    | semver-like 値の parse/compare   | `ParseSemanticVersion`, `CompareVersion`            |
| `ipv4`              | P1   | C++17    | IPv4 parse/format                | `ParseIpV4`, `FormatIpV4`                           |
| `mac_address`       | P1   | C++17    | MAC address parse/format         | `ParseMacAddress`, `FormatMacAddress`               |
| `function`          | P2   | C++17    | callable/visitor の儀式除去      | `Overload`, `MakeOverload`                          |
| `variant_match`     | P2   | C++17    | `std::variant` visitor 補助      | `Match`, `Holds`, `GetIf`                           |
| `optional_ext`      | P2   | C++17    | optional の小さい合成            | `MapOptional`, `AndThen`, `ValueOrEval`             |
| `contract`          | done | C++11    | precondition 明示                | `KET_EXPECTS`, `KET_REQUIRE_NON_NULL`, `IsInBounds` |
| `c_interop`         | P2   | C++11    | C API 境界の事故防止             | `ErrnoGuard`, `CopyStringToBuffer`, `UniqueHandle`  |
| `platform_error`    | P2   | C++17    | errno/Windows error の文字列化   | `ErrnoMessage`, `WindowsErrorMessage`               |
| `state_table`       | P2   | C++17    | 小さい状態遷移表                 | `NextState`, `IsValidTransition`                    |
| `cache_once`        | P2   | C++11    | once/lazy value                  | `OnceValue`, `Lazy`, `GetOrCreate`                  |
| `serialization_tlv` | P2   | C++17    | length-prefix/TLV                | `EncodeTlv`, `TryDecodeTlv`                         |
| `tuple`             | P2   | C++17    | tuple/pair の小さい補助          | `ForEach`, `Transform`                              |
| `build_config`      | P2   | C++11    | feature detection                | `KET_HAS_STD_OPTIONAL`                              |
| `math_small`        | P2   | C++11    | 単位・補間など小さい数学         | `Lerp`, `MapRange`, `DegreesToRadians`              |
| `meta`              | P3   | C++11/17 | type traits 補助                 | `RemoveCvref`, `AlwaysFalse`                        |
| `concurrency_small` | P3   | C++11    | join/lock/timeout の局所補助     | `JoiningThread`, `FutureReady`                      |
| `uuid`              | P3   | C++17    | UUID parse/format                | `ParseUuid`, `FormatUuid`                           |
| `color_rgb`         | P3   | C++11    | RGB小値型                        | `ParseColorRgb`, `FormatColorRgbHex`                |
| `percent`           | P3   | C++11    | percent小値型                    | `Percent::FromRatio`, `ClampPercent`                |
| `recipes`           | P3   | mixed    | moduleの使い方実例               | `recipes/binary_payload`, `recipes/c_api_wrapper`   |

---

## 6. P0 module API 仕様案

### 6.1 `modules/bits/ket_bits.h`

目的: bit / nibble / mask の「短いが毎回怖い処理」を固定する。

C++ Min: C++11

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	namespace bits
	{
		constexpr bool IsNibble(std::uint8_t value) noexcept;
		constexpr std::uint8_t HighNibble(std::uint8_t value) noexcept;
		constexpr std::uint8_t LowNibble(std::uint8_t value) noexcept;
		bool TryPackNibbles(std::uint8_t high, std::uint8_t low, std::uint8_t& out) noexcept;

		template <typename T>
		constexpr unsigned TypeBitWidth() noexcept;

		template <typename T>
		constexpr bool HasBit(T value, unsigned bit_index) noexcept;

		template <typename T>
		bool TrySetBit(T value, unsigned bit_index, T& out) noexcept;

		template <typename T>
		bool TryClearBit(T value, unsigned bit_index, T& out) noexcept;

		template <typename T>
		bool TryToggleBit(T value, unsigned bit_index, T& out) noexcept;

		template <typename T>
		bool TryMask(unsigned width, T& out) noexcept;

		template <typename T>
		constexpr unsigned PopCount(T value) noexcept;

		template <typename T>
		constexpr bool IsPowerOfTwo(T value) noexcept;

		template <typename T>
		constexpr T Rotl(T value, unsigned count) noexcept;

		template <typename T>
		constexpr T Rotr(T value, unsigned count) noexcept;

	} // namespace bits

} // namespace ket
```

仕様メモ:

- `HasBit(value, bit_index)` は `bit_index >= TypeBitWidth<T>()` の場合 `false`。
- `TrySetBit` / `TryClearBit` / `TryToggleBit` は index 範囲外なら `false`。
- `TryMask<T>(0, out)` は `out = 0` で成功。
- `TryMask<T>(TypeBitWidth<T>(), out)` は全bit 1 で成功。
- `TryMask<T>(width, out)` は `width > TypeBitWidth<T>()` なら失敗。
- `Rotl` / `Rotr` は count を bit 幅で剰余化し、shift 幅 overflow を起こさない。
- まず bool、char、wchar_t、char16_t、char32_t を除く unsigned integral 対象に絞る。
  signed 対応は入れないか、内部で unsigned 化して明示する。

テスト観点:

- `HighNibble(0xAB) == 0x0A`
- `LowNibble(0xAB) == 0x0B`
- `TryPackNibbles(0x0A, 0x0B) == 0xAB`
- `TryPackNibbles(0x10, 0x00)` は失敗。
- `HasBit(0b1000, 3)` は true。
- `HasBit(0b1000, 8)` は false。
- `TryMask<std::uint8_t>(0) == 0x00`
- `TryMask<std::uint8_t>(8) == 0xFF`
- `TryMask<std::uint8_t>(9)` は失敗。

---

### 6.2 `modules/numeric/ket_numeric.h`

目的: alignment、rounding、overflow、narrowing cast の危険な定型処理を安全にする。

C++ Min: C++11

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	template <typename To, typename From>
	constexpr bool InRange(From value) noexcept;

	template <typename T>
	constexpr T Clamp(T value, T min_value, T max_value) noexcept;

	template <typename T>
	constexpr typename std::make_unsigned<T>::type AbsDiff(T a, T b) noexcept;

	template <typename T>
	constexpr bool TryDivideRoundUp(T value, T divisor, T* out) noexcept;

	template <typename T>
	constexpr bool TryAlignUp(T value, T alignment, T* out) noexcept;

	template <typename T>
	constexpr bool TryAlignDown(T value, T alignment, T* out) noexcept;

	template <typename T>
	constexpr bool TryCheckedAdd(T a, T b, T* out) noexcept;

	template <typename T>
	constexpr bool TryCheckedSub(T a, T b, T* out) noexcept;

	template <typename T>
	constexpr bool TryCheckedMul(T a, T b, T* out) noexcept;

	template <typename T>
	constexpr T SaturatingAdd(T a, T b) noexcept;

	template <typename T>
	constexpr T SaturatingSub(T a, T b) noexcept;

	template <typename To, typename From>
	constexpr bool TryCheckedCast(From value, To* out) noexcept;

#if KET_HAS_STD_OPTIONAL
	template <typename T>
	constexpr std::optional<T> AlignUp(T value, T alignment) noexcept;

	template <typename T>
	constexpr std::optional<T> DivideRoundUp(T value, T divisor) noexcept;

	template <typename To, typename From>
	constexpr std::optional<To> CheckedCast(From value) noexcept;
#endif

} // namespace ket
```

仕様メモ:

- `TryAlignUp` は `alignment == 0` で失敗。
- `TryAlignUp` は overflow で失敗。
- `TryDivideRoundUp` は `divisor == 0` で失敗。
- alignment 系はまず unsigned integral に絞ると安全。
- signed/unsigned を混ぜる overload は避ける。
- `Clamp` は `min_value <= max_value` を precondition とするか、違反時に `min_value` を返すかを先に固定する。推奨は precondition。
- `TryCheckedCast` は値が `To` に収まる場合のみ成功。

テスト観点:

- `TryAlignUp(0, 4) == 0`
- `TryAlignUp(1, 4) == 4`
- `TryAlignUp(4, 4) == 4`
- `TryAlignUp(5, 4) == 8`
- `TryAlignUp(max, 4)` は overflow 失敗。
- `TryDivideRoundUp(0, 4) == 0`
- `TryDivideRoundUp(1, 4) == 1`
- `TryDivideRoundUp(4, 4) == 1`
- `TryDivideRoundUp(5, 4) == 2`
- `TryCheckedCast<std::uint8_t>(255)` 成功。
- `TryCheckedCast<std::uint8_t>(256)` 失敗。

---

### 6.3 `modules/endian/ket_endian.h`

目的: byte order の読み書きを、unaligned access や strict aliasing に頼らず安全にする。

C++ Min: C++11

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	constexpr std::uint16_t ByteSwap16(std::uint16_t value) noexcept;
	constexpr std::uint32_t ByteSwap32(std::uint32_t value) noexcept;
	constexpr std::uint64_t ByteSwap64(std::uint64_t value) noexcept;

	std::uint16_t LoadBe16(const std::uint8_t* data) noexcept;
	std::uint32_t LoadBe32(const std::uint8_t* data) noexcept;
	std::uint64_t LoadBe64(const std::uint8_t* data) noexcept;

	std::uint16_t LoadLe16(const std::uint8_t* data) noexcept;
	std::uint32_t LoadLe32(const std::uint8_t* data) noexcept;
	std::uint64_t LoadLe64(const std::uint8_t* data) noexcept;

	void StoreBe16(std::uint8_t* data, std::uint16_t value) noexcept;
	void StoreBe32(std::uint8_t* data, std::uint32_t value) noexcept;
	void StoreBe64(std::uint8_t* data, std::uint64_t value) noexcept;

	void StoreLe16(std::uint8_t* data, std::uint16_t value) noexcept;
	void StoreLe32(std::uint8_t* data, std::uint32_t value) noexcept;
	void StoreLe64(std::uint8_t* data, std::uint64_t value) noexcept;

	bool TryLoadBe16(const std::uint8_t* data, std::size_t size, std::uint16_t* out) noexcept;
	bool TryLoadBe32(const std::uint8_t* data, std::size_t size, std::uint32_t* out) noexcept;
	bool TryLoadLe16(const std::uint8_t* data, std::size_t size, std::uint16_t* out) noexcept;
	bool TryLoadLe32(const std::uint8_t* data, std::size_t size, std::uint32_t* out) noexcept;

	bool TryStoreBe16(std::uint8_t* data, std::size_t size, std::uint16_t value) noexcept;
	bool TryStoreBe32(std::uint8_t* data, std::size_t size, std::uint32_t value) noexcept;
	bool TryStoreLe16(std::uint8_t* data, std::size_t size, std::uint16_t value) noexcept;
	bool TryStoreLe32(std::uint8_t* data, std::size_t size, std::uint32_t value) noexcept;

} // namespace ket
```

仕様メモ:

- `LoadBe32(data)` は `data != nullptr` かつ 4 bytes 読めることを precondition にする。
- null/size不足を戻り値で扱いたい場合は `TryLoadBe32(data, size, &out)` を使う。
- 実装では `reinterpret_cast<const std::uint32_t*>` を使わない。
- 必ず byte 単位の shift/or で組み立てる。
- `StoreXxx` も unaligned write をしない。

テスト観点:

- `{0x12, 0x34}` -> `LoadBe16 == 0x1234`
- `{0x12, 0x34}` -> `LoadLe16 == 0x3412`
- `StoreBe32(0x12345678)` -> `{0x12,0x34,0x56,0x78}`
- `StoreLe32(0x12345678)` -> `{0x78,0x56,0x34,0x12}`
- `TryLoadBe32(nullptr, 4, &out)` は失敗。
- `TryLoadBe32(data, 3, &out)` は失敗。

---

### 6.4 `modules/hex/ket_hex.h`

目的: byte列、数値、診断用 hexdump を安全に文字列化する。

C++ Min: C++17

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	enum class HexCase
	{
		kLower,
		kUpper,
	};

	struct HexFormatOptions
	{
		HexCase hex_case = HexCase::kUpper;
		char separator = '\0';
	};

	std::string ToHexString(std::uint64_t value, unsigned min_width = 0, HexCase hex_case = HexCase::kUpper);

	std::string BytesToHex(const std::uint8_t* data, std::size_t size, HexFormatOptions options = {});
	std::optional<std::vector<std::uint8_t>> HexToBytes(std::string_view text);

	std::string HexDump(const std::uint8_t* data, std::size_t size);
	std::string HexDump(const void* data, std::size_t size);

} // namespace ket
```

仕様メモ:

- `BytesToHex(nullptr, 0)` は空文字列。
- `BytesToHex(nullptr, size > 0)` は空文字列ではなく、precondition違反にするか `std::optional<std::string>` にするかを決める。推奨は `TryBytesToHex` 追加より、`data != nullptr || size == 0` を precondition とする。
- `HexToBytes` は whitespace を無視するかどうかを固定する。推奨は ASCII whitespace と区切り記号 `' '` を無視し、それ以外は失敗。
- `HexToBytes("0")` は奇数桁なので失敗。
- `HexDump` は offset + hex bytes + ASCII preview の固定形式にする。

テスト観点:

- `{0x00,0x01,0xAB,0xFF}` -> `"0001ABFF"`
- separator `' '` -> `"00 01 AB FF"`
- `HexToBytes("00 01 AB FF")` -> same bytes。
- `HexToBytes("0")` は失敗。
- `HexToBytes("GG")` は失敗。
- `HexDump` は空入力で空文字列。

---

### 6.5 `modules/parse_numeric/ket_parse_numeric.h`

目的: `std::from_chars` / `strtol` の境界条件を固定し、数値parseを短く安全にする。

C++ Min: C++17

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	template <typename T>
	bool TryParseInt(std::string_view text, T* out) noexcept;

	template <typename T>
	bool TryParseUInt(std::string_view text, T* out) noexcept;

	template <typename T>
	bool TryParseHex(std::string_view text, T* out) noexcept;

	bool TryParseBool(std::string_view text, bool* out) noexcept;

	template <typename T>
	std::optional<T> ParseInt(std::string_view text) noexcept;

	template <typename T>
	std::optional<T> ParseUInt(std::string_view text) noexcept;

	template <typename T>
	std::optional<T> ParseHex(std::string_view text) noexcept;

	std::optional<bool> ParseBool(std::string_view text) noexcept;

	template <typename T>
	T ParseIntOr(std::string_view text, T fallback) noexcept;

	template <typename T>
	T ParseUIntOr(std::string_view text, T fallback) noexcept;

} // namespace ket
```

仕様メモ:

- 成功条件は **完全消費**。
- `"123abc"` は失敗。
- 先頭末尾 whitespace は許さない。許すAPIを作るなら `ParseTrimmedUInt` のように名前へ出す。
- `TryParseUInt` は `"-1"` を失敗にする。
- `TryParseInt` は符号付き integral のみ。
- `TryParseUInt` は符号なし integral のみ。
- `TryParseHex` は `0x` / `0X` prefix を許可するならテストで固定する。推奨は「prefixあり/なし両方許可」。
- `TryParseBool` は `true`, `false`, `1`, `0` のみを基本にする。`yes/no/on/off` は広げすぎなので最初は入れない。

テスト観点:

- `ParseUInt<std::uint32_t>("0") == 0`
- `ParseUInt<std::uint32_t>("4294967295") == max`
- overflow は失敗。
- 空文字列は失敗。
- `" 1"`, `"1 "`, `"1x"` は失敗。
- `ParseBool("true") == true`
- `ParseBool("False")` は、case insensitive にするなら成功、しないなら失敗。推奨は失敗。

---

### 6.6 `modules/enum_table/ket_enum_table.h`

目的: `enum class` と文字列・整数・flags の変換を table-based で明示する。

C++ Min: C++17

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	template <typename E>
	constexpr std::underlying_type_t<E> ToUnderlying(E value) noexcept;

	template <typename E>
	struct EnumEntry
	{
		E value;
		std::string_view name;
	};

	template <typename E, std::size_t N>
	bool TryEnumName(E value, const EnumEntry<E> (&table)[N], std::string_view* out) noexcept;

	template <typename E, std::size_t N>
	std::optional<std::string_view> EnumName(E value, const EnumEntry<E> (&table)[N]) noexcept;

	template <typename E, std::size_t N>
	bool TryParseEnum(std::string_view text, const EnumEntry<E> (&table)[N], E* out) noexcept;

	template <typename E, std::size_t N>
	std::optional<E> ParseEnum(std::string_view text, const EnumEntry<E> (&table)[N]) noexcept;

	template <typename E, std::size_t N>
	bool IsValidEnumValue(E value, const EnumEntry<E> (&table)[N]) noexcept;

	template <typename E>
	constexpr bool HasFlag(E flags, E flag) noexcept;

	template <typename E>
	constexpr E SetFlag(E flags, E flag) noexcept;

	template <typename E>
	constexpr E ClearFlag(E flags, E flag) noexcept;

	template <typename E>
	constexpr bool AnyFlag(E flags, E mask) noexcept;

	template <typename E>
	constexpr bool AllFlags(E flags, E mask) noexcept;

} // namespace ket
```

使用例:

```cpp
enum class Mode
{
	kAuto,
	kManual,
};

constexpr ket::enums::Entry<Mode> kModeTable[] = {
	{Mode::kAuto, "auto"},
	{Mode::kManual, "manual"},
};

const auto mode = ket::enums::Parse("auto", kModeTable);
```

仕様メモ:

- reflection はしない。
- table はユーザーが明示する。
- `EnumName` は重複 entry があれば先に出たものを返す。
- `ParseEnum` は完全一致。case insensitive は別API `ParseEnumIgnoreCaseAscii` が必要になるまで作らない。
- flags は enum の underlying type に変換して bit operation する。

テスト観点:

- known value -> name。
- unknown value -> `std::nullopt`。
- known text -> enum。
- unknown text -> `std::nullopt`。
- duplicate table の挙動。
- flags の set/clear/has/all/any。

---

### 6.7 `modules/container/ket_container.h`

目的: `find`, `end`, `erase-remove`, default 値取得など、標準コンテナ利用時の儀式を短くする。

C++ Min: C++11 core + C++17 optional convenience

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	template <typename Container, typename Value>
	bool Contains(const Container& container, const Value& value);

	template <typename Map, typename Key>
	bool ContainsKey(const Map& map, const Key& key);

	template <typename Map, typename Key>
	typename Map::mapped_type* AtOrNull(Map& map, const Key& key) noexcept;

	template <typename Map, typename Key>
	const typename Map::mapped_type* AtOrNull(const Map& map, const Key& key) noexcept;

	template <typename Map, typename Key>
	typename Map::mapped_type GetOrDefault(
		const Map& map,
		const Key& key,
		typename Map::mapped_type default_value);

	template <typename Map, typename Key, typename Factory>
	typename Map::mapped_type& GetOrCreate(Map& map, const Key& key, Factory factory);

	template <typename Sequence, typename Value>
	std::optional<std::size_t> IndexOf(const Sequence& sequence, const Value& value);

	template <typename Sequence, typename Predicate>
	std::size_t EraseIf(Sequence& sequence, Predicate predicate);

	template <typename Vector>
	void SortUnique(Vector& values);

	template <typename Dst, typename Src>
	void Append(Dst& dst, const Src& src);

} // namespace ket
```

仕様メモ:

- `AtOrNull` は copy を避けたい時の基本API。
- `GetOrDefault` は値を copy/move して返す。
- `GetOrCreate` は key が無い場合のみ factory を呼ぶ。
- `EraseIf` は C++20 `std::erase_if` と重なるが C++11〜17 で価値がある。
- `Append` は `dst.insert(dst.end(), src.begin(), src.end())` の意図を名前にする。

テスト観点:

- keyあり/なし。
- default が必要な時だけ使われるか。
- `GetOrCreate` factory が key あり時に呼ばれないこと。
- `EraseIf` の戻り値が削除件数になること。
- `SortUnique` が sort 済み・重複なしにすること。

---

### 6.8 `modules/string_ascii/ket_string_ascii.h`

目的: ASCII 前提の小さい文字列処理を、Unicode と誤解されない名前で提供する。

C++ Min: C++17

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	bool StartsWith(std::string_view text, std::string_view prefix) noexcept;
	bool EndsWith(std::string_view text, std::string_view suffix) noexcept;
	bool Contains(std::string_view text, std::string_view needle) noexcept;

	std::string_view TrimAscii(std::string_view text) noexcept;
	std::string_view TrimLeftAscii(std::string_view text) noexcept;
	std::string_view TrimRightAscii(std::string_view text) noexcept;

	std::string_view RemovePrefix(std::string_view text, std::string_view prefix) noexcept;
	std::string_view RemoveSuffix(std::string_view text, std::string_view suffix) noexcept;

	std::vector<std::string_view> SplitView(std::string_view text, char delimiter);
	std::vector<std::string> Split(std::string_view text, char delimiter);
	std::string Join(const std::vector<std::string_view>& parts, std::string_view delimiter);

	std::string ReplaceAll(std::string_view text, std::string_view from, std::string_view to);

	std::string ToLowerAscii(std::string_view text);
	std::string ToUpperAscii(std::string_view text);
	bool EqualsIgnoreCaseAscii(std::string_view a, std::string_view b) noexcept;

} // namespace ket
```

仕様メモ:

- `TrimAscii` が削るのは ASCII whitespace のみ。
- `SplitView` の戻り値は元文字列を参照する。lifetime を Doxygen に明記する。
- `SplitView("a,,b", ',')` は空要素を保持するかを固定する。推奨は保持。
- `RemovePrefix` は prefix が無ければ元の `text` を返す。
- `ReplaceAll(text, "", to)` は無限ループ回避のため失敗にするか、元文字列を返すか固定する。推奨は元文字列を返すより、`from.empty()` を precondition 違反としてコメントに書く。

テスト観点:

- empty string。
- delimiterなし。
- leading/trailing delimiter。
- ASCII uppercase/lowercase。
- 日本語や UTF-8 bytes は壊さず byte列として扱うこと。ただし case 変換対象ではない。

---

### 6.9 `modules/scope/ket_scope.h`

目的: cleanup 漏れ、早期 return 時の復元漏れを防ぐ。

C++ Min: C++11

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	template <typename F>
	class ScopeExit
	{
	public:
		explicit ScopeExit(F f) noexcept(std::is_nothrow_move_constructible<F>::value);
		ScopeExit(ScopeExit&& other) noexcept(std::is_nothrow_move_constructible<F>::value);
		ScopeExit(const ScopeExit&) = delete;
		ScopeExit& operator=(const ScopeExit&) = delete;
		ScopeExit& operator=(ScopeExit&&) = delete;
		~ScopeExit() noexcept;

		void Dismiss() noexcept;
		bool Active() const noexcept;
	};

	template <typename F>
	ScopeExit<F> MakeScopeExit(F f) noexcept(std::is_nothrow_move_constructible<F>::value);

	template <typename T>
	class RestoreOnExit
	{
	public:
		explicit RestoreOnExit(T& target);
		~RestoreOnExit() noexcept(noexcept(std::declval<T&>() = std::declval<T>()));
		void Dismiss() noexcept;
	};

	template <typename T>
	RestoreOnExit<T> MakeRestoreOnExit(T& target);

} // namespace ket
```

仕様メモ:

- destructor から例外を外へ出さない。callback が例外を投げる場合の扱いを固定する。推奨は `std::terminate`。
- `ScopeExit` は move-only。
- move 後の source は inactive。
- `Dismiss()` 後は destructor で callback を呼ばない。
- `Finally` は `MakeScopeExit` と重複するため最初は作らない。

テスト観点:

- scope exit で呼ばれる。
- early return 相当で呼ばれる。
- `Dismiss` 後は呼ばれない。
- move 後に二重実行されない。
- callback の参照キャプチャ寿命は利用者責任として doc に明記。

---

### 6.10 `modules/byte_reader/ket_byte_reader.h`

目的: 固定 buffer からの逐次読み取りを、offset/remaining を明示しながら安全に行う。

C++ Min: C++11

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	namespace byte_reader
	{
		class Reader
		{
		public:
			Reader(const std::uint8_t* data, std::size_t size) noexcept;

			std::size_t Size() const noexcept;
			std::size_t Offset() const noexcept;
			std::size_t Remaining() const noexcept;
			bool Empty() const noexcept;

			bool Skip(std::size_t size) noexcept;
			bool ReadU8(std::uint8_t& out) noexcept;
			bool ReadBe16(std::uint16_t& out) noexcept;
			bool ReadBe32(std::uint32_t& out) noexcept;
			bool ReadLe16(std::uint16_t& out) noexcept;
			bool ReadLe32(std::uint32_t& out) noexcept;
			bool ReadBytes(std::size_t size, const std::uint8_t*& out_data) noexcept;
		};

	} // namespace byte_reader

} // namespace ket
```

仕様メモ:

- `data == nullptr && size == 0` は有効な空 reader。
- `data == nullptr && size > 0` は invalid reader とし、全 read を失敗させる。
- `Empty()` は valid reader が末尾に到達した場合だけ true を返す。
- `ReadXxx` は成功時のみ offset を進める。
- 失敗時は offset を変えない。
- `ReadBytes` は non-owning pointer を返す。reader の元 buffer lifetime が必要。
- endian module に依存せず、内部で必要な Load 処理を小さく持つ。

テスト観点:

- 空 buffer。
- ぴったり読み切る。
- 1 byte 足りない。
- 失敗時に offset が進まない。
- BE/LE の値。

---

### 6.11 `modules/byte_writer/ket_byte_writer.h`

目的: 固定 buffer への逐次書き込みを、overflow なしで行う。

C++ Min: C++11

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	class ByteWriter
	{
	public:
		ByteWriter(std::uint8_t* data, std::size_t size) noexcept;

		std::size_t Size() const noexcept;
		std::size_t Offset() const noexcept;
		std::size_t Remaining() const noexcept;
		bool Full() const noexcept;

		bool Skip(std::size_t size) noexcept;
		bool WriteU8(std::uint8_t value) noexcept;
		bool WriteBe16(std::uint16_t value) noexcept;
		bool WriteBe32(std::uint32_t value) noexcept;
		bool WriteLe16(std::uint16_t value) noexcept;
		bool WriteLe32(std::uint32_t value) noexcept;
		bool WriteBytes(const std::uint8_t* data, std::size_t size) noexcept;
	};

} // namespace ket
```

仕様メモ:

- `data == nullptr && size == 0` は有効な空 writer。
- `data == nullptr && size > 0` は invalid writer。
- write 成功時のみ offset を進める。
- 失敗時は offset と buffer を変更しない。これを守るため、サイズ確認後に書く。
- endian module に依存せず、内部で必要な Store 処理を小さく持つ。

テスト観点:

- 空 buffer 書き込み失敗。
- ぴったり書き切る。
- 1 byte 足りない。
- 失敗時に offset が進まない。
- 失敗時に既存 buffer が変更されない。

---

### 6.12 `modules/bytes_builder/ket_bytes_builder.h`

目的: 可変長 payload を `std::vector<std::uint8_t>` へ読みやすく組み立てる。

C++ Min: C++17

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	class BytesBuilder
	{
	public:
		BytesBuilder() = default;
		explicit BytesBuilder(std::size_t reserve_size);

		BytesBuilder& U8(std::uint8_t value);
		BytesBuilder& Be16(std::uint16_t value);
		BytesBuilder& Be32(std::uint32_t value);
		BytesBuilder& Le16(std::uint16_t value);
		BytesBuilder& Le32(std::uint32_t value);
		BytesBuilder& Bytes(const std::uint8_t* data, std::size_t size);
		BytesBuilder& StringAscii(std::string_view text);

		const std::vector<std::uint8_t>& Bytes() const noexcept;
		std::vector<std::uint8_t> Build() &&;
		void Clear() noexcept;
	};

	void AppendU8(std::vector<std::uint8_t>& dst, std::uint8_t value);
	void AppendBe16(std::vector<std::uint8_t>& dst, std::uint16_t value);
	void AppendBe32(std::vector<std::uint8_t>& dst, std::uint32_t value);
	void AppendLe16(std::vector<std::uint8_t>& dst, std::uint16_t value);
	void AppendLe32(std::vector<std::uint8_t>& dst, std::uint32_t value);
	void AppendBytes(std::vector<std::uint8_t>& dst, const std::uint8_t* data, std::size_t size);

} // namespace ket
```

仕様メモ:

- allocation を伴うため `noexcept` は付けない。
- `Bytes(nullptr, 0)` は no-op。
- `Bytes(nullptr, size > 0)` は precondition 違反。
- fluent API は読みやすいが、巨大 builder framework にしない。
- `AppendXxx` free functions も提供すると、既存 vector への追加が楽になる。

使用例:

```cpp
auto payload = ket::bytes_builder::Builder{}
	.AppendU8(command)
	.AppendBe16(sequence)
	.AppendBe32(id)
	.Build();
```

---

### 6.13 `modules/date/ket_date.h`

目的: 日付・時刻の小さい妥当性判定を、calendar framework なしで提供する。

C++ Min: C++11

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	constexpr bool IsLeapYear(int year) noexcept;
	constexpr bool IsValidMonth(unsigned month) noexcept;
	constexpr bool TryDaysInMonth(int year, unsigned month, unsigned* out) noexcept;
	constexpr bool IsValidDate(int year, unsigned month, unsigned day) noexcept;
	constexpr bool IsValidTime(unsigned hour, unsigned minute, unsigned second) noexcept;
	constexpr bool IsValidTimeMillis(unsigned hour, unsigned minute, unsigned second, unsigned millisecond) noexcept;

} // namespace ket
```

仕様メモ:

- Gregorian calendar 前提。
- timezone は扱わない。
- `year == 0` を有効にするかは先に固定する。推奨は proleptic Gregorian として許すより、業務で使いやすい `1 <= year` を前提にする。
- leap second は扱わない。`second <= 59`。

テスト観点:

- 2000 は leap year。
- 1900 は leap year ではない。
- 2024-02-29 は valid。
- 2023-02-29 は invalid。
- month 0/13 invalid。
- hour 24 invalid。

---

### 6.14 `modules/deadline/ket_deadline.h`

目的: `steady_clock` ベースの timeout / elapsed time 計算を読みやすくする。

C++ Min: C++11

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	class Stopwatch
	{
	public:
		static Stopwatch StartNew() noexcept;

		void Restart() noexcept;
		std::chrono::steady_clock::duration Elapsed() const noexcept;
		std::chrono::milliseconds ElapsedMillis() const noexcept;
	};

	class Deadline
	{
	public:
		static Deadline After(std::chrono::steady_clock::duration timeout) noexcept;
		static Deadline At(std::chrono::steady_clock::time_point time_point) noexcept;

		bool Expired() const noexcept;
		std::chrono::steady_clock::duration Remaining() const noexcept;
		std::chrono::steady_clock::time_point TimePoint() const noexcept;
	};

} // namespace ket
```

仕様メモ:

- `steady_clock` のみ。`system_clock` と混ぜない。
- `Remaining()` は期限切れなら zero を返す。
- 負の timeout を受けた場合は即 expired 扱い。

テスト観点:

- zero timeout は expired。
- future deadline は expired ではない。
- `Remaining()` が負にならない。
- `Stopwatch::Restart()` で elapsed がリセットされる。

---

### 6.15 `modules/cli/ket_cli.h`

目的: 小さい社内CLIで `argc/argv` の option 取得を短くする。

C++ Min: C++17

Dependencies: Standard library only, no ket dependencies

候補API:

```cpp
namespace ket
{
	class ArgvView
	{
	public:
		ArgvView(int argc, const char* const* argv) noexcept;

		std::size_t Size() const noexcept;
		std::string_view AtOrEmpty(std::size_t index) const noexcept;
		std::string_view ProgramNameOrEmpty() const noexcept;
	};

	bool HasOption(ArgvView args, std::string_view name) noexcept;
	std::optional<std::string_view> GetOption(ArgvView args, std::string_view name) noexcept;
	std::string_view GetOptionOr(ArgvView args, std::string_view name, std::string_view fallback) noexcept;

	template <typename T>
	std::optional<T> OptionUInt(ArgvView args, std::string_view name) noexcept;

	std::vector<std::string_view> Positional(ArgvView args);

} // namespace ket
```

仕様メモ:

- `--key value` と `--key=value` の両方を許すか固定する。推奨は両方。
- `--flag` は `HasOption` で扱う。
- `GetOption(args, "--id")` で次要素が別 option なら失敗にする。
- `std::string_view` は `argv` の lifetime に依存する。
- shell quote 展開は扱わない。
- subcommand framework は作らない。

テスト観点:

- `--help` flag。
- `--id 123`。
- `--id=123`。
- missing value。
- duplicated option の扱い。推奨は最初を返す。
- positional arguments。

---

## 7. P1 module API 仕様案

### 7.1 `modules/byte_view/ket_byte_view.h`

```cpp
namespace ket
{
	class ByteView
	{
	public:
		ByteView() noexcept;
		ByteView(const std::uint8_t* data, std::size_t size) noexcept;

		const std::uint8_t* Data() const noexcept;
		std::size_t Size() const noexcept;
		bool Empty() const noexcept;
		bool TryAt(std::size_t index, std::uint8_t* out) const noexcept;
		bool TrySubView(std::size_t offset, std::size_t count, ByteView* out) const noexcept;
	};

	class MutableByteView
	{
	public:
		MutableByteView() noexcept;
		MutableByteView(std::uint8_t* data, std::size_t size) noexcept;

		std::uint8_t* Data() const noexcept;
		std::size_t Size() const noexcept;
		bool Empty() const noexcept;
		bool TryAt(std::size_t index, std::uint8_t* out) const noexcept;
		bool TrySet(std::size_t index, std::uint8_t value) noexcept;
		bool TrySubView(std::size_t offset, std::size_t count, MutableByteView* out) const noexcept;
	};

	ByteView MakeByteView(const std::uint8_t* data, std::size_t size) noexcept;
	MutableByteView MakeMutableByteView(std::uint8_t* data, std::size_t size) noexcept;

} // namespace ket
```

注意: `std::span` がある C++20 では標準機能を尊重する。C++11〜17 向けの薄い non-owning view として入れる価値がある。

---

### 7.2 `modules/utf8/ket_utf8.h`

```cpp
namespace ket
{
	struct Utf8ValidationResult
	{
		bool valid = false;
		std::size_t error_offset = 0;
	};

	bool IsAscii(std::string_view text) noexcept;
	Utf8ValidationResult ValidateUtf8(std::string_view text) noexcept;
	bool IsUtf8(std::string_view text) noexcept;
	std::optional<std::size_t> Utf8Length(std::string_view text) noexcept;

} // namespace ket
```

仕様メモ:

- normalization、case folding、locale、Shift_JIS 変換は扱わない。
- `Utf8Length` は code point 数。grapheme cluster 数ではない。
- 不正UTF-8なら `std::nullopt`。

---

### 7.3 `modules/file/ket_file.h`

```cpp
namespace ket
{
	bool TryReadAllText(const std::filesystem::path& path, std::string* out, std::error_code* error = nullptr);
	bool TryReadAllBytes(const std::filesystem::path& path, std::vector<std::uint8_t>* out, std::error_code* error = nullptr);

	bool WriteAllText(const std::filesystem::path& path, std::string_view text, std::error_code* error = nullptr);
	bool WriteAllBytes(const std::filesystem::path& path, const std::uint8_t* data, std::size_t size, std::error_code* error = nullptr);

	bool FileExists(const std::filesystem::path& path) noexcept;
	bool DirectoryExists(const std::filesystem::path& path) noexcept;
	std::optional<std::uintmax_t> FileSize(const std::filesystem::path& path) noexcept;

} // namespace ket
```

仕様メモ:

- C++17 `std::filesystem` 前提。
- text encoding は変換しない。byte列をそのまま `std::string` に読む。
- error detail が必要な場合は `std::error_code*` を渡す。

---

### 7.4 `modules/io_stream/ket_io_stream.h`

```cpp
namespace ket
{
	bool ReadExactly(std::istream& stream, std::uint8_t* data, std::size_t size);
	bool WriteAll(std::ostream& stream, const std::uint8_t* data, std::size_t size);
	std::optional<std::string> ReadLineTrimmedAscii(std::istream& stream);

	class StreamStateSaver
	{
	public:
		explicit StreamStateSaver(std::ios& stream);
		~StreamStateSaver() noexcept;
		StreamStateSaver(const StreamStateSaver&) = delete;
		StreamStateSaver& operator=(const StreamStateSaver&) = delete;
	};

} // namespace ket
```

仕様メモ:

- `ReadExactly` は requested size を読み切った場合のみ true。
- `StreamStateSaver` は flags, precision, fill を保存・復元する。

---

### 7.5 `modules/format_value/ket_format_value.h`

```cpp
namespace ket
{
	std::string ToString(bool value);
	std::string ToString(std::int64_t value);
	std::string ToString(std::uint64_t value);
	std::string ToHexString(std::uint64_t value, unsigned min_width = 0);
	std::string ToBinaryString(std::uint64_t value, unsigned min_width = 0);
	std::string FormatBytes(std::uint64_t bytes);
	std::string FormatDuration(std::chrono::nanoseconds duration);

} // namespace ket
```

注意: `std::format` の再実装はしない。ログ framework も作らない。診断用の短い文字列化に留める。

---

### 7.6 `modules/algorithm_range/ket_algorithm_range.h`

```cpp
namespace ket
{
	template <typename Range, typename Predicate>
	bool AllOf(const Range& range, Predicate predicate);

	template <typename Range, typename Predicate>
	bool AnyOf(const Range& range, Predicate predicate);

	template <typename Range, typename Predicate>
	bool NoneOf(const Range& range, Predicate predicate);

	template <typename Range, typename Predicate>
	std::size_t CountIf(const Range& range, Predicate predicate);

	template <typename Range, typename Predicate>
	auto FindIf(Range& range, Predicate predicate) -> decltype(std::begin(range));

	template <typename Range, typename F>
	void ForEachIndex(Range& range, F f);

} // namespace ket
```

注意: ただの `std::algorithm` 別名になりすぎる場合は入れない。iterator pair を毎回書く煩わしさが消える場合のみ採用する。

---

### 7.7 `modules/memory/ket_memory.h`

```cpp
namespace ket
{
	bool IsAligned(const void* ptr, std::size_t alignment) noexcept;
	bool TryAlignUpPtr(const void* ptr, std::size_t alignment, const void** out) noexcept;
	bool TryAlignDownPtr(const void* ptr, std::size_t alignment, const void** out) noexcept;

	void ZeroMemory(void* ptr, std::size_t size) noexcept;
	void SecureZeroMemory(void* ptr, std::size_t size) noexcept;

	template <typename T>
	const std::uint8_t* ObjectBytes(const T& object) noexcept;

	template <typename T>
	std::size_t ObjectByteSize(const T& object) noexcept;

} // namespace ket
```

注意: object lifetime に踏み込む API は危険。最初は alignment と object representation の読み取りに絞る。

---

### 7.8 `modules/pointer/ket_pointer.h`

```cpp
namespace ket
{
	template <typename Ptr>
	class NotNull
	{
	public:
		explicit NotNull(Ptr ptr);
		Ptr Get() const noexcept;
		auto operator*() const -> decltype(*std::declval<Ptr>());
		Ptr operator->() const noexcept;
	};

	template <typename T>
	std::shared_ptr<T> LockWeak(const std::weak_ptr<T>& weak) noexcept;

	template <typename T>
	T* GetOrNull(std::optional<std::reference_wrapper<T>>& value) noexcept;

	template <typename T>
	T* AddressOf(T& value) noexcept;

} // namespace ket
```

注意: `NotNull` は所有権を持たない。`Owner<T*>` は便利だが誤解も大きいため、まずは入れないか experimental にする。

---

### 7.9 `modules/testing_bytes/ket_testing_bytes.h`

```cpp
namespace ket
{
	::testing::AssertionResult BytesEq(
		const std::uint8_t* expected,
		std::size_t expected_size,
		const std::uint8_t* actual,
		std::size_t actual_size);

	::testing::AssertionResult HexEq(
		std::string_view expected_hex,
		const std::uint8_t* actual,
		std::size_t actual_size);

} // namespace ket
```

仕様メモ:

- Dependencies に GoogleTest を明記する。
- library本体というより test helper module として扱う。
- bytes mismatch 時に offset と hex を表示する。

---

### 7.10 `modules/semantic_version/ket_semantic_version.h`

```cpp
namespace ket
{
	struct SemanticVersion
	{
		std::uint32_t major = 0;
		std::uint32_t minor = 0;
		std::uint32_t patch = 0;
	};

	std::optional<SemanticVersion> ParseSemanticVersion(std::string_view text) noexcept;
	std::string FormatSemanticVersion(SemanticVersion version);
	int CompareSemanticVersion(SemanticVersion a, SemanticVersion b) noexcept;

} // namespace ket
```

仕様メモ:

- 最初は `major.minor.patch` のみ。
- prerelease/build metadata まで入れると大きくなるため後回し。
- `01.2.3` のような leading zero を許すか固定する。推奨は失敗。

---

### 7.11 `modules/ipv4/ket_ipv4.h`

```cpp
namespace ket
{
	struct IpV4
	{
		std::uint8_t octets[4] = {0, 0, 0, 0};
	};

	std::optional<IpV4> ParseIpV4(std::string_view text) noexcept;
	std::string FormatIpV4(IpV4 value);
	std::uint32_t IpV4ToU32Be(IpV4 value) noexcept;
	IpV4 U32BeToIpV4(std::uint32_t value) noexcept;

} // namespace ket
```

仕様メモ:

- dotted decimal のみ。
- CIDR、DNS、IPv6 は扱わない。
- `001.002.003.004` の leading zero を許すか固定する。推奨は失敗。

---

### 7.12 `modules/mac_address/ket_mac_address.h`

```cpp
namespace ket
{
	struct MacAddress
	{
		std::uint8_t bytes[6] = {0, 0, 0, 0, 0, 0};
	};

	std::optional<MacAddress> ParseMacAddress(std::string_view text) noexcept;
	std::string FormatMacAddress(MacAddress value);
	std::string FormatMacAddressUpper(MacAddress value);

} // namespace ket
```

仕様メモ:

- `AA:BB:CC:DD:EE:FF` と `aa-bb-cc-dd-ee-ff` のどちらを許すか固定する。推奨は `:` と `-` を許す。
- Cisco形式 `aabb.ccdd.eeff` は最初は扱わない。

---

## 8. P2 module API 仕様案

### 8.1 `modules/function/ket_function.h`

```cpp
namespace ket
{
	template <typename... Fs>
	struct Overload : Fs...
	{
		using Fs::operator()...;
	};

	template <typename... Fs>
	Overload<Fs...> MakeOverload(Fs... fs);

	struct Noop
	{
		template <typename... Args>
		void operator()(Args&&...) const noexcept;
	};

} // namespace ket
```

注意: `FunctionRef` は寿命事故が多いため、最初は入れない。`Overload` は `std::visit` で価値が明確。

---

### 8.2 `modules/variant_match/ket_variant_match.h`

```cpp
namespace ket
{
	template <typename Variant, typename... Handlers>
	decltype(auto) Match(Variant&& variant, Handlers&&... handlers);

	template <typename T, typename Variant>
	bool Holds(const Variant& variant) noexcept;

	template <typename T, typename Variant>
	T* GetIf(Variant* variant) noexcept;

} // namespace ket
```

注意: `function` module と重なる。drop-in 独立性のため、`Overload` 相当を内部に持つか、`variant_match` 側にだけ `Match` を置く。

---

### 8.3 `modules/optional_ext/ket_optional_ext.h`

```cpp
namespace ket
{
	template <typename T, typename F>
	auto MapOptional(const std::optional<T>& value, F f) -> std::optional<decltype(f(*value))>;

	template <typename T, typename F>
	auto AndThen(const std::optional<T>& value, F f) -> decltype(f(*value));

	template <typename T, typename F>
	T ValueOrEval(const std::optional<T>& value, F fallback_factory);

	template <typename... Optionals>
	bool HasValueAll(const Optionals&... values) noexcept;

} // namespace ket
```

注意: `Result` / `StatusOr` は重くなりやすい。まず optional helper に留める。

---

### 8.4 `modules/contract/ket_contract.h`

```cpp
namespace ket
{
	namespace contract
	{
		enum class Kind
		{
			kExpects,
			kEnsures,
			kInvariant
		};

		[[noreturn]] void Fail(Kind kind, const char* expression, const char* file, int line) noexcept;
		void Expects(bool condition, const char* expression, const char* file, int line) noexcept;
		void Ensures(bool condition, const char* expression, const char* file, int line) noexcept;
		void AssertInvariant(bool condition, const char* expression, const char* file, int line) noexcept;

		template <typename T>
		T* RequireNonNull(T* ptr, const char* expression, const char* file, int line) noexcept;

		constexpr bool IsInBounds(std::size_t index, std::size_t size) noexcept;

	} // namespace contract

} // namespace ket

#define KET_EXPECTS(condition)
#define KET_ENSURES(condition)
#define KET_ASSERT_INVARIANT(condition)
#define KET_REQUIRE_NON_NULL(ptr)
```

仕様メモ:

- assert/abort/terminate のポリシーを明確にする。
- global macro は `KET_` prefixの4個に限定する。
- `debug` とは分ける。contract は意味、debug は観測。

---

### 8.5 `modules/c_interop/ket_c_interop.h`

```cpp
namespace ket
{
	class ErrnoGuard
	{
	public:
		ErrnoGuard() noexcept;
		~ErrnoGuard() noexcept;
		int Saved() const noexcept;
	};

	bool CopyToCBuffer(char* dst, std::size_t dst_size, std::string_view src) noexcept;
	bool CopyBytesToCBuffer(void* dst, std::size_t dst_size, const void* src, std::size_t src_size) noexcept;

	template <typename Handle, typename Deleter>
	class UniqueHandle
	{
	public:
		UniqueHandle() noexcept;
		UniqueHandle(Handle handle, Deleter deleter) noexcept;
		~UniqueHandle() noexcept;
		Handle Get() const noexcept;
		Handle Release() noexcept;
		void Reset(Handle handle) noexcept;
	};

} // namespace ket
```

注意: C API wrapper recipes と相性が良い。OS handle 固有の invalid value は template parameter または traits が必要になるため、初回は小さく。

---

### 8.6 `modules/platform_error/ket_platform_error.h`

```cpp
namespace ket
{
	std::string ErrnoMessage(int errno_value);

#ifdef _WIN32
	std::string WindowsErrorMessage(unsigned long error_code);
	unsigned long GetLastErrorCode() noexcept;
#endif

	std::optional<std::string> EnvironmentVariable(std::string_view name);

} // namespace ket
```

注意: POSIX/Windows 差を隠しすぎない。OS専用APIは `#ifdef` と drop-in 条件を明記する。

---

### 8.7 `modules/state_table/ket_state_table.h`

```cpp
namespace ket
{
	template <typename State, typename Event>
	struct Transition
	{
		State from;
		Event event;
		State to;
	};

	template <typename State, typename Event, std::size_t N>
	bool IsValidTransition(
		State current,
		Event event,
		const Transition<State, Event> (&table)[N]) noexcept;

	template <typename State, typename Event, std::size_t N>
	std::optional<State> NextState(
		State current,
		Event event,
		const Transition<State, Event> (&table)[N]) noexcept;

} // namespace ket
```

注意: FSM framework を作らない。table lookup だけにする。

---

### 8.8 `modules/cache_once/ket_cache_once.h`

```cpp
namespace ket
{
	template <typename T>
	class Lazy
	{
	public:
		bool HasValue() const noexcept;
		void Reset();

		template <typename Factory>
		T& GetOrCreate(Factory factory);
	};

	template <typename T>
	class OnceValue
	{
	public:
		bool HasValue() const noexcept;

		template <typename Factory>
		const T& Get(Factory factory);
	};

} // namespace ket
```

注意: thread-safe かどうかを名前かコメントに出す。初回は non-thread-safe でよい。

---

### 8.9 `modules/serialization_tlv/ket_serialization_tlv.h`

```cpp
namespace ket
{
	struct TlvView
	{
		std::uint16_t type = 0;
		const std::uint8_t* value = nullptr;
		std::size_t value_size = 0;
	};

	void EncodeLengthPrefixed(std::vector<std::uint8_t>& dst, const std::uint8_t* data, std::size_t size);
	bool TryDecodeLengthPrefixed(const std::uint8_t* data, std::size_t size, TlvView* out) noexcept;

	void EncodeTlv(std::vector<std::uint8_t>& dst, std::uint16_t type, const std::uint8_t* value, std::size_t value_size);
	bool TryDecodeTlv(const std::uint8_t* data, std::size_t size, TlvView* out) noexcept;

} // namespace ket
```

注意: struct をそのまま bytes 化する API は入れない。field 単位 serialize を優先する。

---

### 8.10 `modules/tuple/ket_tuple.h`

```cpp
namespace ket
{
	namespace tuple
	{
		template <typename Tuple, typename F>
		void ForEach(Tuple&& tuple, F&& f);

		template <typename Tuple, typename F>
		auto Transform(Tuple&& tuple, F&& f);

	} // namespace tuple

} // namespace ket
```

注意: tuple は読みづらくなりやすい。`First(pair)` / `Second(pair)` は structured binding と競合するため優先度低。

---

### 8.11 `modules/build_config/ket_build_config.h`

```cpp
#define KET_CXX_VERSION 201703L
#define KET_HAS_STD_OPTIONAL 1
#define KET_HAS_STD_STRING_VIEW 1
#define KET_HAS_STD_SPAN 0
#define KET_HAS_STD_FORMAT 0
#define KET_COMPILER_GCC 1
#define KET_COMPILER_CLANG 0
#define KET_COMPILER_MSVC 0
#define KET_OS_WINDOWS 0
#define KET_OS_LINUX 1
#define KET_OS_MACOS 0
```

注意:

- global macro は汚染なので最小限。
- module独立性を優先するなら、各module内で小さく feature detection してもよい。
- C++11〜23 を本気で跨ぐ module が増えてから導入でもよい。

---

### 8.12 `modules/math_small/ket_math_small.h`

```cpp
namespace ket
{
	template <typename T, typename U>
	constexpr auto Lerp(T a, T b, U t) noexcept -> decltype(a + (b - a) * t);

	template <typename T>
	constexpr T DegreesToRadians(T degrees) noexcept;

	template <typename T>
	constexpr T RadiansToDegrees(T radians) noexcept;

	template <typename T>
	constexpr bool NearlyEqual(T a, T b, T epsilon) noexcept;

	constexpr std::uint64_t KiBToBytes(std::uint64_t kib) noexcept;
	constexpr std::uint64_t MiBToBytes(std::uint64_t mib) noexcept;
	constexpr double BytesToKiB(std::uint64_t bytes) noexcept;
	constexpr double BytesToMiB(std::uint64_t bytes) noexcept;

} // namespace ket
```

注意: statistics や units framework へ広げすぎない。最初は補間・角度・byte単位変換程度。

---

## 9. P3 / recipes 仕様案

### 9.1 `modules/meta/ket_meta.h`

```cpp
namespace ket
{
	template <typename T>
	using RemoveCvref = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

	template <typename T>
	struct TypeIdentity
	{
		using type = T;
	};

	template <typename...>
	struct AlwaysFalse : std::false_type
	{
	};

	template <typename...>
	using VoidT = void;

} // namespace ket
```

注意: meta は便利だが魔術化しやすい。テンプレートエラーを悪化させるAPIは避ける。

---

### 9.2 `modules/concurrency_small/ket_concurrency_small.h`

```cpp
namespace ket
{
	class JoiningThread
	{
	public:
		JoiningThread() noexcept;
		explicit JoiningThread(std::thread thread) noexcept;
		~JoiningThread() noexcept;
		JoiningThread(JoiningThread&& other) noexcept;
		JoiningThread& operator=(JoiningThread&& other) noexcept;
		std::thread& Get() noexcept;
		bool Joinable() const noexcept;
	};

	template <typename Future>
	bool FutureReady(Future& future) noexcept;

} // namespace ket
```

注意: thread pool は作らない。join 忘れと timeout 確認程度に留める。

---

### 9.3 `modules/uuid/ket_uuid.h`

```cpp
namespace ket
{
	struct Uuid
	{
		std::uint8_t bytes[16] = {};
	};

	std::optional<Uuid> ParseUuid(std::string_view text) noexcept;
	std::string FormatUuid(Uuid value);

} // namespace ket
```

注意: UUID generation は乱数・OS API が絡むため最初は parse/format だけ。

---

### 9.4 `recipes/binary_payload/`

目的: BCD、endian、byte_writer、hex を組み合わせた電文構築例。

候補ファイル:

```txt
recipes/binary_payload/README.md
recipes/binary_payload/binary_payload_example.cpp
```

例で示すこと:

- command byte。
- BE sequence。
- BCD date。
- checksum 風の単純計算。
- HexDump で診断。

### 9.5 `recipes/command_parser/`

目的: CLI、parse_numeric、enum_table を組み合わせた小さい command parser 例。

例で示すこと:

- `--mode auto`
- `--port 1234`
- `--verbose`
- enum table。
- parse 失敗時のメッセージ。

### 9.6 `recipes/c_api_wrapper/`

目的: `c_interop` と `scope` を使った C API 境界の RAII 化例。

例で示すこと:

- handle close。
- errno 保存。
- fixed C buffer への安全コピー。

---

## 10. 既存 BCD module への追加提案

既存 `bcd` はすでに ket の最初の実moduleとして十分よい。追加する場合は、増やす目的を明確にする。

### 10.1 追加してもよい候補

```cpp
namespace ket
{
	constexpr bool IsBcdByte(std::uint8_t value) noexcept;
	constexpr bool IsBcd16(std::uint16_t value) noexcept;
	constexpr bool IsBcd32(std::uint32_t value) noexcept;
}
```

理由:

- parse までは不要で、妥当性だけ見たい場面がある。
- `ParseBcd(value).has_value()` より意図が読みやすい。

ただし、`detail::IsBcdNibble` をそのまま公開する必要は薄い。nibble 単体の判定は `bits` module の `IsNibble` と紛らわしいため、公開するなら BCD byte/word 判定に寄せる。

### 10.2 追加しないほうがよい候補

```cpp
BcdDate
ParseBcdDate
BcdTime
```

理由:

- BCD と date/time の組み合わせであり、`recipes/bcd_datetime` 向き。
- domain が濃くなりやすい。

---

## 11. 名前・仕様の整理提案

### 11.1 `Trim` より `TrimAscii`

`Trim` は Unicode whitespace 対応と誤解される。最初は `TrimAscii` を推奨。

```cpp
ket::ascii::Trim(text)
ket::ascii::TrimLeft(text)
ket::ascii::TrimRight(text)
```

### 11.2 `Bytes` accessor と `Builder` 型を分ける

型は `Builder`、保持 byte 列への accessor は `Bytes()` として役割を分ける。

```cpp
ket::bytes_builder::Builder{}.AppendU8(x).AppendBe16(y).Build();
```

### 11.3 `Finally` は最初は不要

`scope::Exit` と `MakeExit` があれば十分。

```cpp
auto guard = ket::scope::MakeExit([&] { cleanup(); });
```

`Finally` は同義語として増えるだけになりやすい。

### 11.4 `Result` / `StatusOr` は急がない

独自 error 体系は巨大化しやすい。現時点では次で足りる。

- `TryXxx(..., out*) -> bool`
- `std::optional<T>`
- 必要な場合だけ `std::error_code*`

### 11.5 `ReadU32` は作らない

endian が不明な読み取りAPIは避ける。

```cpp
reader.ReadBe32(out)
reader.ReadLe32(out)
ket::endian::LoadBe32(data)
ket::endian::LoadLe32(data)
```

---

## 12. 実装順の具体案

### Sprint 1: bit / numeric / endian

1. `catalog.md` に `bits`, `numeric`, `endian` を追加。
2. `modules/bits` 実装。
3. `modules/numeric` 実装。
4. `modules/endian` 実装。
5. それぞれ境界値テストと compile-only check を追加。

理由:

- BCD の隣接領域であり、ket の小さい正解感が強い。
- overflow、shift幅、unaligned access など「C++で毎回危ない」要素が多い。

### Sprint 2: hex / byte_reader / byte_writer

1. `hex` を実装。
2. `byte_reader` を実装。
3. `byte_writer` を実装。
4. recipes ではなく module として単独テストする。

理由:

- binary payload と diagnostics に直結する。
- 以後の recipes が作りやすくなる。

### Sprint 3: parse_numeric / enum_table / container

1. `parse_numeric` を実装。
2. `enum_table` を実装。
3. `container` を実装。

理由:

- 業務C++で頻度が高い。
- 標準ライブラリの儀式を短くする効果が分かりやすい。

### Sprint 4: string_ascii / scope / date / deadline

1. `string_ascii` を実装。
2. `scope` を実装。
3. `date` を実装。
4. `deadline` を実装。

理由:

- utility catalog として使い勝手が一気に増える。
- ただし仕様が広がりやすいので、ASCII/date/steady_clock など境界を固定する。

### Sprint 5: cli / recipes

1. `cli` を小さく実装。
2. `recipes/command_parser` を追加。
3. `recipes/binary_payload` を追加。

理由:

- module 単体の使い方を示せる。
- ket を使う側に価値が伝わる。

---

## 13. catalog.md に追加する時の粒度

`catalog.md` には大量の候補を一度に入れてよいが、module folder は作らない。

1 entry は次の粒度にする。

良い粒度:

- `Idea: AlignUp`
- `Idea: LoadBe32`
- `Idea: ByteReader`
- `Idea: EnumName`
- `Idea: ScopeExit`

大きすぎる粒度:

- `Idea: Binary Framework`
- `Idea: String Utilities Everything`
- `Idea: Error Handling Framework`

小さすぎる粒度:

- `Idea: Add one to index`
- `Idea: IsZero`
- `Idea: Return true`

---

## 14. module 実装時チェックリスト

各moduleを実装する時は、最低限これを確認する。

```txt
[ ] modules/<name>/ket_<name>.h を作った
[ ] modules/<name>/ket_<name>.cpp を作った、または header-only と明記した
[ ] modules/<name>/ket_<name>_test.cpp を作った
[ ] ヘッダ先頭に drop-in 条件を書いた
[ ] C++標準要求を書いた
[ ] Dependencies を書いた
[ ] 公開APIは namespace ket
[ ] 他の ket module に依存していない
[ ] 公開ヘッダが必要な標準ヘッダを自分で include している
[ ] Doxygen に @brief / @param / @retval / @pre / @post がある（constructor/destructorは @retval なし）
[ ] 失敗条件を戻り値・precondition・例外のどれで扱うか固定した
[ ] null / empty / overflow / size不足 / invalid input のテストがある
[ ] format / static analysis / conventions / CTest が通る
[ ] progress.md を実moduleとして更新した
```

---

## 15. まとめ

ket の次の進化では、以下の順に進めるのがよい。

1. `bits`, `numeric`, `endian` で「小さいが危険なC++」を押さえる。
2. `hex`, `byte_reader`, `byte_writer` で binary payload 系の価値を出す。
3. `parse_numeric`, `enum_table`, `container`, `string_ascii` で業務コードのノイズを減らす。
4. `scope`, `date`, `deadline`, `cli` で日常の小さい事故を減らす。
5. `recipes` で module の組み合わせ方を示す。

大事なのは、module を一気に増やすことではない。

**catalog には広く、modules には狭く、実装には深く。**

この方針なら、ket は「しょうもない処理」専用という軽さを保ったまま、C++実務で何度も使える小さな正解の集まりとして育つ。
