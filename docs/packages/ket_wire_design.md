# ket-wire package design

この文書は、`ket-wire` の目的、価値、公開 API、失敗規約、検証条件を固定する設計契約である。
`ket-wire` は ordinary module ではなく、複数の ket module を合成して固定またはほぼ固定の binary wire
layout を C++ struct と相互変換する C++17 package runtime である。

`ket-wire` の価値は serialization framework を提供することではない。byte order、field offset、
encoded size、BCD 妥当性、bit packing、reserved bytes、exact/prefix decode、named diagnostics、
decode/encode 失敗時不変性を、schema から一貫して扱えることにある。

## 1. design values

- wire layout を一度だけ宣言し、decode、encode、size 計算、diagnostics、test 観点を同じ情報から導く。
- C++17 を最小要件とし、`std::optional`、`std::array`、`std::string_view`、`std::vector<std::uint8_t>`を public API で利用する。
- `std::span` は C++20 機能であるため public API に出さず、byte列の non-owning view は `ket::byte_view::View` と `ket::byte_view::MutableView` を使う。
- host endian、packed struct、strict aliasing、object representation 依存を避ける。
- 失敗時に caller の object と output buffer を壊さない。
- field 名、group 名、offset、期待値、実値、必要 byte 数、利用可能 byte 数を diagnostics に残す。
- ordinary module の drop-in 性を壊さず、複数 module を合成する責務は package 側へ閉じ込める。
- 採用する API は少数でも、その範囲は Doxygen、test、static analysis、sanitizer まで完了条件に含める。

## 2. C++ version and dependencies

`ket-wire` の C++ version contract:

- 最小要件：C++17。
- 本ライブラリの適用を推奨する C++ バージョン：C++17以降。
- 推奨理由：既存の C++17 ket module である `bcd`、`bytes`、`optional`、`tuple` と同じ水準で、失敗値と所有 byte列を `std::optional` と `std::vector<std::uint8_t>` で明示できる。
- 本ライブラリの適用を推奨しない C++ バージョン：なし。
- 非推奨理由：なし。

`ket-wire` は package runtime であり、ordinary module の「他の ket module へ依存しない」制約とは別の層にある。
実装では次の既存 module を利用してよい。

- `byte_view`: public API の input/output view。
- `endian`: fixed-width integer の byte order 変換。
- `bits`: bit packing、mask、reserved bit の小処理。
- `numeric`: overflow、range、size 計算の境界確認。
- `bcd`: packed BCD の nibble 検証と integer 変換。
- `bytes`: owning encode convenience の byte列構築。

package 内だけで必要な処理でも、`wire` 以外で独立して使う価値がある場合は ordinary module として抽出する。

## 3. placement and namespace

runtime の実装単位は `packages/wire/` とする。`modules/wire` は runtime 配置ではない。
`wire` は ordinary module ではなく package runtime であり、既存の module 独立性ルールとは別の層で
複数 module を合成する。

公開 namespace は `ket::wire` とする。repository style に従い、実装時の C++ source では
`namespace ket { namespace wire { ... } }` の block 形式を使う。

配置の設計契約:

```txt
docs/packages/ket_wire_design.md
packages/wire/ket_wire.h
packages/wire/ket_wire.cpp        # 非 template 実装がある場合だけ
packages/wire/ket_wire_test.cpp
```

header-only で十分な template runtime だけなら `.cpp` は置かない。空 source、include だけの source、
空 package folder は置かない。旧標準向けの compile-only check は作らない。

## 4. relationship to ordinary modules

`ket-wire` は既存 module を置き換えない。必要な処理が ordinary module として独立して価値を持つ場合は、
package runtime へ埋め込まず module として抽出する。

ordinary module として扱う条件:

- `wire` 以外でも独立して使う価値がある。
- 標準ライブラリだけ、または既存 module なしで成立する。
- 公開 API が小さく、責務を一文で説明できる。
- C++ version、失敗規約、境界条件、test 観点を module 単体で固定できる。
- `modules/<name>/` として持ち出しても package 固有語を含まない。

package 側へ残す責務:

- message schema 全体の合成。
- field 名と offset を持つ wire diagnostics。
- 複数 module を前提にした decode/encode orchestration。
- exact/prefix decode、trailing bytes、encoded size、computed validation の package-wide 規約。

## 5. relationship to tlv

`tlv` は `docs/module_api_catalog.md` に存在する別 ordinary module の仕様カードである。`ket-wire` は `tlv` を
置き換えず、`tlv` を `wire` へ改名しない。

併用規約:

- TLV record の type/length/value 境界は `ket::tlv` が扱う。
- TLV value bytes の内部 layout を struct へ decode/encode する処理は `ket::wire` schema が扱う。
- `ket::wire` は TLV の type registry、nested TLV tree、可変 object graph を扱わない。
- `ket::wire` schema は TLV 以外の fixed frame、command/response record、device register payload にも適用する。

## 6. target use cases

- embedded device command/response record。
- 通信制御 software の固定 binary frame。
- big-endian / little-endian が混在する field。
- BCD で表現される年月日、時刻、識別番号。
- fixed byte payload、reserved bytes、checksum 対象範囲。
- field 名付き decode failure log。
- stream 先頭から 1 message だけ読み、consumed byte 数を caller へ返す prefix decode。
- protocol 固有 struct を、wire layout に忠実な byte 列へ戻す encode。

## 7. non-goals

`ket-wire` は次を提供しない。

- socket、network connection、transport retry。
- async runtime、thread pool、event loop。
- Protocol Buffers、ASN.1、FlatBuffers、IDL compiler、code generator。
- macro-first schema API。
- 任意 C++ object serialization。
- pointer graph、shared ownership graph、継承階層、多態 codec。
- protocol server、router、command dispatcher。
- logging framework、metrics framework。
- checksum library、crypto、compression。
- BCD calendar semantics、timezone、date validation。
- TLV tree framework。
- generated documentation engine。
- 標準ライブラリまたは既存 ket module の置き換え。

## 8. public API overview

`ket-wire` の public surface は schema declaration、field descriptor、decode/encode operation、
diagnostics の 4 系列で構成する。C++17 public API として、失敗値は operation 固有 result 型に埋め込む。
generic `Result` / `StatusOr` は作らない。

```cpp
namespace ket
{
    namespace wire
    {
        enum class Error;
        struct Status;

        template <typename T>
        struct Field;

        template <typename T, std::size_t FieldCount>
        struct Schema;

        template <typename T>
        struct DecodeResult;

        struct EncodeResult;
        struct EncodeToResult;
        struct SizeResult;

        template <typename T, std::size_t FieldCount>
        Schema<T, FieldCount> MakeSchema(
            std::string_view name,
            std::array<Field<T>, FieldCount> fields) noexcept;

        template <typename T, std::size_t FieldCount>
        DecodeResult<T> DecodeExact(ket::byte_view::View data,
                                    const Schema<T, FieldCount>& schema);

        template <typename T, std::size_t FieldCount>
        DecodeResult<T> DecodePrefix(ket::byte_view::View data,
                                     const Schema<T, FieldCount>& schema);

        template <typename T, std::size_t FieldCount>
        EncodeResult Encode(const T& value, const Schema<T, FieldCount>& schema);

        template <typename T, std::size_t FieldCount>
        EncodeToResult EncodeTo(const T& value,
                                const Schema<T, FieldCount>& schema,
                                ket::byte_view::MutableView out);

        template <typename T, std::size_t FieldCount>
        std::optional<std::size_t> EncodedSize(const Schema<T, FieldCount>& schema) noexcept;

        template <typename T, std::size_t FieldCount>
        SizeResult MeasureEncodedSize(const T& value, const Schema<T, FieldCount>& schema);
    } // namespace wire

} // namespace ket
```

`DecodeExact` は schema 全体を読み終えた後、input を全消費していることを要求する。`DecodePrefix` だけが
trailing bytes を許し、成功時に `DecodeResult<T>::consumed` へ消費 byte 数を入れる。

`Encode` は owning convenience API であり、`std::vector<std::uint8_t>` を確保して返す。allocation があるため
`noexcept` にしない。`EncodeTo` は caller が渡した `ket::byte_view::MutableView` へ書く fixed-buffer API であり、
成功時だけ先頭 `encoded_size` byte を更新する。

field descriptor factory は typed pointer-to-member template とし、member 型の不一致を compile error にする。

```cpp
const auto schema = ket::wire::MakeSchema<Frame>(
    "Frame",
    std::array<ket::wire::Field<Frame>, 2U>{
        ket::wire::U16Be<Frame, &Frame::command>("command"),
        ket::wire::U8<Frame, &Frame::mode>("mode")
    });
```

## 9. result and diagnostics contract

`Status` は allocation なしで失敗理由を保持する。operation result は常に `Status` を内包し、失敗時にも
diagnostics を取得できる。

```cpp
namespace ket
{
    namespace wire
    {
        enum class Error
        {
            kNone,
            kInvalidInputView,
            kInvalidOutputView,
            kShortInput,
            kShortOutput,
            kTrailingBytes,
            kInvalidBcd,
            kValueOutOfRange,
            kReservedMismatch,
            kLengthMismatch,
            kChecksumMismatch,
            kCallbackFailed,
            kSizeOverflow,
            kSchemaError
        };

        struct Status
        {
            Error error;
            std::size_t offset;
            std::string_view field;
            std::string_view group;
            std::size_t required_size;
            std::size_t available_size;
            std::uint64_t expected;
            std::uint64_t actual;
            bool has_expected;
            bool has_actual;

            bool Ok() const noexcept;
        };

        template <typename T>
        struct DecodeResult
        {
            std::optional<T> value;
            Status status;
            std::size_t consumed;
        };

        struct EncodeResult
        {
            std::optional<std::vector<std::uint8_t>> bytes;
            Status status;
        };

        struct EncodeToResult
        {
            Status status;
            std::size_t encoded_size;
        };

        struct SizeResult
        {
            std::optional<std::size_t> value;
            Status status;
        };
    } // namespace wire

} // namespace ket
```

status 規約:

- 成功時は `Error::kNone`、`offset == 0`、`field.empty()`、`group.empty()` を基本値とする。
- `Status::Ok()` は `error == Error::kNone` と同義。
- field error の `offset` は field 先頭の byte offset。
- short input/output の `offset` は不足が判明した byte offset。
- trailing bytes の `offset` は schema encoded size。
- `field` と `group` は non-owning `std::string_view`。schema、field、group 名の参照先は schema 利用中 caller が保持する。
- `required_size` と `available_size` は short input/output、length mismatch、size overflow の説明に使う。
- `expected` と `actual` は reserved value、constant value、checksum、range validation の説明に使う。
- size 計算 overflow は `Error::kSizeOverflow` とする。
- diagnostics formatting は core API に含めない。必要なら allocation を許す別 helper として扱う。

result 規約:

- `DecodeResult<T>::value` は成功時だけ値を持つ。失敗時は `std::nullopt`。
- `DecodeResult<T>::consumed` は prefix decode 成功時だけ実消費 byte 数。exact decode 成功時は input size と同じ値。失敗時は 0。
- `EncodeResult::bytes` は成功時だけ値を持つ。失敗時は `std::nullopt`。
- `EncodeToResult::encoded_size` は成功時だけ書き込み byte 数。失敗時は 0。
- `SizeResult::value` は成功時だけ値を持つ。失敗時は `std::nullopt`。

## 10. schema and field model

`Schema<T, N>` は message 名、`std::array<Field<T>, N>`、field 数、fixed/dynamic size metadata を保持する。
schema は field 配列を所有し、caller-held field array の lifetime に依存しない。

`Field<T>` は次を保持する。

- field 名。
- field kind。
- encoded size policy。
- decode function。
- encode preflight function。
- encode function。
- validation function。
- member pointer または member accessor。

`Field<T>` と `Schema<T, N>` は dynamic allocation を行わない。schema と field は immutable であり、共有 schema の
read-only 利用は thread-safe である。`N == 0` の empty schema は encoded size 0 の valid schema とする。

schema name、field name、group name は `std::string_view` として保持する。string literal または schema 利用中に
破棄されない文字列を渡す。temporary `std::string` から作った `std::string_view` を渡してはならない。

## 11. field descriptor families

scalar field:

- `U8`
- `U16Be`, `U16Le`
- `U32Be`, `U32Le`
- `U64Be`, `U64Le`
- `I8`
- `I16Be`, `I16Le`
- `I32Be`, `I32Le`
- `I64Be`, `I64Le`

signed integer は two's complement の wire 表現として明示的に扱う。object representation をそのまま
copy しない。

BCD field:

- `BcdU8`
- `BcdU16Be`, `BcdU16Le`
- `BcdU32Be`, `BcdU32Le`
- raw fixed BCD bytes descriptor

decoded integer と raw BCD byte のどちらを struct member に持つかは descriptor 名で明示する。
先頭ゼロが意味を持つ field は raw fixed BCD bytes として扱う。

byte field:

- fixed C array copy field。
- `std::array<std::uint8_t, N>` copy field。
- explicit view field。
- fixed padding field。

copy field は decode 時に source buffer pointer を struct へ保存しない。view field は API 名と Doxygen で
source lifetime 依存を明示する。

bit field:

- `BitsU8`
- `BitsU16Be`, `BitsU16Le`
- reserved bit group。

bit descriptor は storage unit を 1 回だけ consume し、複数 logical field へ展開する。logical field ごとに
cursor を進める descriptor は採用しない。

constant and reserved field:

- `ConstU8`
- `ConstBytes`
- `ReservedBytes`
- `PadBytes`
- `ReservedBits`

reserved field は decode 時に期待値と一致しなければ失敗する。encode 時は期待値を書き込むか、対象 member の
値が期待値と一致することを preflight で検査する。

computed validation:

- length validation hook。
- checksum validation hook。
- range validation hook。
- cross-field validation hook。

computed hook は `bool` と `Status&` で失敗を返す。checksum 計算の algorithm catalog は `ket-wire` の責務に
含めない。

## 12. encoded size contract

fixed schema は compile-time または schema construction 時に encoded size を決定できる。
mostly fixed schema は bounded length、remaining payload、optional contiguous group により runtime size を持つ。

size API:

- `EncodedSize(schema)` は fixed schema の byte 数を返す。value 依存 schema では `std::nullopt`。
- `MaxEncodedSize(schema)` は bounded schema の最大 byte 数を返す。
- `IsFixedSize(schema)` は encoded size が value に依存しない場合に true。
- `MeasureEncodedSize(value, schema)` は value 依存 size を計算し、overflow と range violation を失敗にする。

size 計算は `std::size_t` overflow を検出する。overflow 時は `Status::error == Error::kSizeOverflow` とし、
`SizeResult::value` は `std::nullopt` とする。

## 13. decode data flow

decode は次の順序で行う。

1. input `ket::byte_view::View` の valid/invalid を検査する。
2. schema の field 配列、field count、message 名を検査する。
3. temporary object を用意する。
4. field を順番に decode し、temporary object だけを更新する。
5. exact decode では reader が末尾に到達したことを検査する。
6. prefix decode では consumed byte 数を計算し、成功 result へ入れる。
7. すべて成功した場合だけ `DecodeResult<T>::value` へ temporary object を move/copy する。

decode failure invariants:

- caller-owned object は変更しない。
- input buffer は常に変更しない。
- 失敗 result は `value == std::nullopt`。
- 失敗 result の `consumed` は 0。
- status は失敗理由を保持する。

`T` の default construction、copy/move、assignment が例外を投げる場合、operation 自体も例外を投げ得る。
allocation なし・例外なしを要求する schema では、対象型と hook も nothrow contract を満たすことを Doxygen に明記する。

## 14. encode data flow

`EncodeTo` は two-pass を標準とする。

1. output `ket::byte_view::MutableView` の valid/invalid を検査する。
2. schema の field 配列、field count、message 名を検査する。
3. `MeasureEncodedSize` と field preflight で size、range、BCD、bit、reserved、length、checksum を検査する。
4. caller buffer へ書く前に完全検証済みであることを示せる backend を用意する。
5. field を順番に encode する。
6. すべて成功した場合だけ caller buffer の先頭 `encoded_size` byte を更新する。

`Encode` は `EncodeTo` と同じ preflight を行った後、owning `std::vector<std::uint8_t>` を確保し、成功時だけ
`EncodeResult::bytes` に格納する。

encode failure invariants:

- `EncodeTo` 失敗時は output buffer を変更しない。
- `EncodeTo` 成功時も extra capacity は変更しない。
- `Encode` 失敗時は `bytes == std::nullopt`。
- value object は変更しない。
- status は失敗理由を保持する。

fixed-buffer encode、size 計算、built-in descriptor は allocation なしで実装する。owning `Encode` は
`std::vector<std::uint8_t>` の確保があるため `noexcept` にしない。

## 15. buffer and lifetime policy

- public API の input は `ket::byte_view::View`。
- public API の fixed output は `ket::byte_view::MutableView`。
- `View(nullptr, 0)` と `MutableView(nullptr, 0)` は有効な空 view。
- `View(nullptr, size > 0)` は invalid input view とし、`Error::kInvalidInputView`。
- `MutableView(nullptr, size > 0)` は invalid output view とし、`Error::kInvalidOutputView`。
- view field は source buffer lifetime を caller が保持する。
- copy field は source buffer pointer を struct に保存しない。
- field name、group name、schema name は schema 利用中に参照可能な storage を要求する。
- encode output が value object の storage と重なる使い方は API contract 外。

## 16. endianness policy

- multi-byte field は API 名に endian を必ず含める。
- plain `U16`、`U32`、`U64` は採用しない。
- host endian に依存しない。
- unaligned access に依存しない。
- `reinterpret_cast` による整数 load/store を使わない。
- signed field も byte order と two's complement を明示した descriptor だけ採用する。

## 17. BCD policy

- BCD descriptor は nibble が 0..9 の範囲にあることを検査する。
- invalid nibble は `Error::kInvalidBcd`。
- decoded integer descriptor は leading zero を保持しない。
- leading zero が意味を持つ field は raw fixed BCD bytes descriptor を使う。
- encode は BCD 桁数へ収まらない値を `Error::kValueOutOfRange` にする。
- C++17 API では BCD decode/size 失敗値を `std::optional` と `Status` で表す。

## 18. bit field policy

- bit field は storage unit descriptor と logical bit member descriptor の group で扱う。
- decode は storage unit を 1 回読み、logical fields へ mask/shift で展開する。
- encode は logical fields の range をすべて preflight し、1 storage unit へ pack してから書く。
- reserved bits は decode で期待値一致を検査し、encode で期待値を書く。
- bit numbering は descriptor 名または Doxygen で MSB/LSB 基準を明示する。

## 19. exact and prefix decode

exact decode:

- `DecodeExact` は schema 全体を読み終えた後、input を全消費していることを要求する。
- trailing bytes は `Error::kTrailingBytes`。
- 成功時の `DecodeResult<T>::consumed` は input size と同じ値。

prefix decode:

- `DecodePrefix` は schema で定義した message だけを読み、success result の `consumed` に消費 byte 数を入れる。
- prefix decode は stream、envelope payload、TLV value の先頭 message に使う。

## 20. validation hooks

validation hook は fixed-buffer operation と同じ core contract に従う。

- `noexcept`。
- allocation なし。
- 例外なし。
- `bool` で成功/失敗を返す。
- 失敗時は `Status&` に field、offset、expected、actual を設定できる。

length hook は declared length と actual consumed size を比較する。checksum hook は caller が提供した algorithm を
呼び出すだけに留める。cross-field hook は業務固有 protocol の完全実装に踏み込まず、schema-local invariant を
確認する用途に限定する。

## 21. examples as executable contracts

examples を追加する場合は compile 対象にする。文書だけの example は正本にしない。

executable example は次を示す。

- exact fixed frame。
- prefix decode。
- owning `Encode`。
- fixed-buffer `EncodeTo`。
- BCD field。
- grouped bit field。
- fixed byte copy field。
- reserved bytes。
- length validation。
- checksum validation hook。
- failure diagnostics。

## 22. completion criteria

`ket-wire` runtime を完了扱いにする条件:

- `packages/wire/ket_wire.h` に公開 API、Doxygen、C++ version、dependency、namespace、drop-in 条件がある。
- `.cpp` が必要な場合は実装を持ち、空 source や include-only source ではない。
- public API は C++17 で build される。
- functional test は GoogleTest で境界条件を網羅する。
- package tests は exact decode、prefix decode、owning encode、fixed-buffer encode、diagnostics、size 計算を含む。
- static analysis、convention check、sanitizer test、format check、layout check が通る。
- 実装開始または状態変更時に `progress.md` へ package runtime としての検証状態を記録する。
- docs は public API、失敗規約、非目標、TLV との関係を反映する。

## 23. test matrix

必須 test:

- valid empty `ket::byte_view::View`。
- invalid input `ket::byte_view::View`。
- valid empty `ket::byte_view::MutableView`。
- invalid output `ket::byte_view::MutableView`。
- empty schema。
- schema-owned `std::array<Field<T>, N>` が caller-held field array lifetime に依存しない。
- engaged `std::optional` decode result。
- empty `std::optional` decode result with embedded `Status`。
- engaged `std::optional` size result。
- empty `std::optional` size result with embedded `Status`。
- short input at every field。
- short output。
- trailing bytes。
- exact decode consumed size。
- prefix decode consumed size。
- exact-size output。
- output extra capacity preservation。
- decode failure keeps caller-owned object unchanged。
- `EncodeTo` failure keeps output buffer unchanged。
- owning `Encode` success returns vector bytes。
- owning `Encode` failure returns `std::nullopt` bytes。
- invalid BCD nibble。
- BCD value out of range。
- bit field range overflow。
- grouped bit field roundtrip。
- reserved byte mismatch。
- reserved bit mismatch。
- length mismatch。
- checksum mismatch。
- computed callback failure。
- `Error::kSizeOverflow`。
- endian golden bytes。
- fixed byte copy does not keep source pointer。
- view field documents source lifetime and preserves pointer intentionally。

verification command:

```sh
python3 tools/check_repository.py
```

failure isolation commands:

```sh
python3 tools/check_python.py
python3 tools/check_layout.py
python3 tools/check_format.py
cmake --preset dev
cmake --build --preset dev
cmake --build --preset dev --target check-static
cmake --build --preset dev --target check-conventions
ctest --preset dev
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize
git diff --check
```

## 24. implementation work split

長時間の goal-driven 実装では、write scope を分けて並列化する。

- public API owner: `packages/wire/ket_wire.h`。
- implementation owner: `packages/wire/ket_wire.cpp`。
- test owner: `packages/wire/ket_wire_test.cpp`。
- build/metadata owner: `CMakeLists.txt`、`progress.md`、package docs。
- verification owner: read-only checks と failure triage。

各 owner は他 owner の変更を revert しない。既存 binary modules は read-only reference として扱う。

## 25. review checklist

- `ket-wire` は package runtime であり、runtime 配置は `packages/wire/` で固定されている。
- public namespace は `ket::wire`。
- C++17 minimum と C++17 public API が Doxygen に明記されている。
- 旧標準互換のための public-shape compromise が残っていない。
- schema descriptor が decode、encode、size、diagnostics の正本になっている。
- schema が `std::array<Field<T>, N>` を所有し、caller-held field array lifetime に依存していない。
- macro-first API や code generator を要求していない。
- public API は `std::span` ではなく `ket::byte_view::View` と `ket::byte_view::MutableView` を使っている。
- operation 固有 result 型を使い、generic `Result` / `StatusOr` を追加していない。
- `Status` は `std::string_view` 名と `Error::kSizeOverflow` を持つ。
- owning `Encode` と fixed-buffer `EncodeTo` の allocation/noexcept 方針が分かれている。
- validation hook は `Status&` と `noexcept` contract を持つ。
- multi-byte field 名に endian が出ている。
- exact decode と prefix decode が名前で分かれている。
- prefix decode は consumed byte 数を success result に入れる。
- decode 失敗時に caller-owned object が不変。
- `EncodeTo` 失敗時に output buffer が不変。
- BCD の decoded integer と raw bytes が descriptor 名で区別されている。
- bit field の shared storage unit を group descriptor で扱っている。
- fixed byte copy field が source pointer lifetime に依存しない。
- view field は lifetime 依存を API 名と Doxygen で説明している。
- reserved bytes/bits、length、checksum、custom validation の diagnostics がある。
- `tlv` を置き換えず、併用関係を説明している。
- socket、transport、IDL、任意 object serialization、protocol server へ広げていない。
