# ket-wire package design

この文書は、`ket-wire` の目的、価値、公開 API、失敗規約、検証条件を固定する設計契約である。
`ket-wire` は ordinary module ではなく、複数の ket module を合成して固定またはほぼ固定の binary wire
layout を C++ struct と相互変換する package runtime である。

`ket-wire` の価値は serialization framework を提供することではない。byte order、field offset、
encoded size、BCD 妥当性、bit packing、reserved bytes、exact/prefix decode、named diagnostics、
decode/encode 失敗時不変性を、schema から一貫して扱えることにある。

## 1. design values

- wire layout を一度だけ宣言し、decode、encode、size 計算、diagnostics、test 観点を同じ情報から導く。
- host endian、packed struct、strict aliasing、object representation 依存を避ける。
- core decode/encode は C++11 互換、`noexcept`、allocation なし、例外なしで成立させる。
- 失敗時に caller の object と output buffer を壊さない。
- field 名、offset、期待値、実値、必要 byte 数、利用可能 byte 数を diagnostics に残す。
- ordinary module の drop-in 性を壊さず、複数 module を合成する責務は package 側へ閉じ込める。
- 採用する API は少数でも、その範囲は Doxygen、test、static analysis、sanitizer まで完了条件に含める。

## 2. placement and namespace

runtime の実装単位は `packages/wire/` とする。`modules/wire` は runtime 配置ではない。
`wire` は ordinary module ではなく package runtime であり、既存の module 独立性ルールとは別の層で
複数 module を合成する。

公開 namespace は `ket::wire` とする。C++11 互換を保つため、実装時の C++ source では
`namespace ket { namespace wire { ... } }` の block 形式を使う。

配置の設計契約:

```txt
docs/packages/ket_wire_design.md
packages/wire/ket_wire.h
packages/wire/ket_wire.cpp        # 非 template 実装がある場合だけ
packages/wire/ket_wire_test.cpp
packages/wire/ket_wire_cxx11_check.cpp
```

header-only で十分な template runtime だけなら `.cpp` は置かない。空 source、include だけの source、
空 package folder は置かない。

## 3. relationship to ordinary modules

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

## 4. relationship to tlv

`tlv` は `docs/module_api_catalog.md` に存在する別 ordinary module の仕様カードである。`ket-wire` は `tlv` を
置き換えず、`tlv` を `wire` へ改名しない。

併用規約:

- TLV record の type/length/value 境界は `ket::tlv` が扱う。
- TLV value bytes の内部 layout を struct へ decode/encode する処理は `ket::wire` schema が扱う。
- `ket::wire` は TLV の type registry、nested TLV tree、可変 object graph を扱わない。
- `ket::wire` schema は TLV 以外の fixed frame、command/response record、device register payload にも適用する。

## 5. target use cases

- embedded device command/response record。
- 通信制御 software の固定 binary frame。
- big-endian / little-endian が混在する field。
- BCD で表現される年月日、時刻、識別番号。
- fixed byte payload、reserved bytes、checksum 対象範囲。
- field 名付き decode failure log。
- stream 先頭から 1 message だけ読み、consumed byte 数を caller へ返す prefix decode。
- protocol 固有 struct を、wire layout に忠実な byte 列へ戻す encode。

## 6. non-goals

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

## 7. public API overview

`ket-wire` の public surface は schema declaration、field descriptor、decode/encode operation、
diagnostics の 4 系列で構成する。
C++11-facing public signature には `std::optional`、`std::span`、`std::string_view`、CTAD、
C++17 namespace shorthand を使わない。

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

        template <typename T, std::size_t FieldCount>
        Schema<T, FieldCount> MakeSchema(const char* name,
                                          const Field<T> (&fields)[FieldCount]) noexcept;

        template <typename T, std::size_t FieldCount>
        bool TryDecode(const std::uint8_t* data,
                       std::size_t size,
                       const Schema<T, FieldCount>& schema,
                       T& out,
                       Status* status) noexcept;

        template <typename T, std::size_t FieldCount>
        bool TryDecodeExact(const std::uint8_t* data,
                            std::size_t size,
                            const Schema<T, FieldCount>& schema,
                            T& out,
                            Status* status) noexcept;

        template <typename T, std::size_t FieldCount>
        bool TryDecodePrefix(const std::uint8_t* data,
                             std::size_t size,
                             const Schema<T, FieldCount>& schema,
                             T& out,
                             std::size_t& consumed,
                             Status* status) noexcept;

        template <typename T, std::size_t FieldCount>
        bool TryEncode(const T& value,
                       const Schema<T, FieldCount>& schema,
                       std::uint8_t* out,
                       std::size_t out_size,
                       Status* status) noexcept;

        template <typename T, std::size_t FieldCount>
        bool TryEncodedSize(const T& value,
                            const Schema<T, FieldCount>& schema,
                            std::size_t& out,
                            Status* status) noexcept;
    } // namespace wire

} // namespace ket
```

`TryDecode` は exact decode の別名とし、trailing bytes を失敗にする。prefix decode は
`TryDecodePrefix` だけが行い、成功時に consumed byte 数を返す。

`Status* status` は optional diagnostics 出力である。`nullptr` の場合も失敗判定は bool で返す。
非 optional 出力である `T& out`、`std::size_t& consumed`、`std::size_t& out` は参照で受ける。
field descriptor factory は typed pointer-to-member template とし、member 型の不一致を compile error にする。

```cpp
const ket::wire::Field<Frame> fields[] = {
    ket::wire::U16Be<Frame, &Frame::command>("command"),
    ket::wire::U8<Frame, &Frame::mode>("mode")
};
```

## 8. diagnostics contract

`Status` は allocation なしで失敗理由を保持する。

```cpp
namespace ket
{
    namespace wire
    {
        enum class Error
        {
            kNone,
            kNullInput,
            kNullOutput,
            kShortInput,
            kShortOutput,
            kTrailingBytes,
            kInvalidBcd,
            kValueOutOfRange,
            kReservedMismatch,
            kLengthMismatch,
            kChecksumMismatch,
            kCallbackFailed,
            kSchemaError
        };

        struct Status
        {
            Error error;
            std::size_t offset;
            const char* field;
            const char* group;
            std::size_t required_size;
            std::size_t available_size;
            std::uint64_t expected;
            std::uint64_t actual;
            bool has_expected;
            bool has_actual;

            bool Ok() const noexcept;
        };
    } // namespace wire

} // namespace ket
```

status 規約:

- 成功時は `Error::kNone`、`offset == 0`、`field == nullptr`、`group == nullptr` を基本値とする。
- field error の `offset` は field 先頭の byte offset。
- short input/output の `offset` は不足が判明した byte offset。
- trailing bytes の `offset` は schema encoded size。
- `field` と `group` は string literal または static storage duration の `const char*`。
- `required_size` と `available_size` は short input/output、length mismatch、size overflow の説明に使う。
- `expected` と `actual` は reserved value、constant value、checksum、range validation の説明に使う。
- diagnostics formatting は core API に含めない。必要なら allocation を許す別 helper として扱う。

## 9. schema and field model

`Schema<T, N>` は message 名、field 配列、field 数、fixed/dynamic size metadata を保持する。
schema は caller が static storage duration で持つことを標準形にする。

`Field<T>` は次を保持する。

- field 名。
- field kind。
- encoded size policy。
- decode function。
- encode preflight function。
- encode function。
- validation function。
- member pointer または member accessor。

`Field<T>` と `Schema<T, N>` は dynamic allocation を行わない。field 配列の lifetime は schema 利用中
caller が保持する。
schema と field は immutable であり、共有 schema の read-only 利用は thread-safe である。
`N == 0` の empty schema は encoded size 0 の valid schema とする。

## 10. field descriptor families

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

computed hook は bool と `Status` で失敗を返す。checksum 計算の algorithm catalog は `ket-wire` の責務に
含めない。

## 11. encoded size contract

fixed schema は compile-time または schema construction 時に encoded size を決定できる。
mostly fixed schema は bounded length、remaining payload、optional contiguous group により runtime size を持つ。

size API:

- `EncodedSize(schema)` は fixed schema の byte 数を返す。
- `MaxEncodedSize(schema)` は bounded schema の最大 byte 数を返す。
- `IsFixedSize(schema)` は encoded size が value に依存しない場合に true。
- `TryEncodedSize(value, schema, out, status)` は value 依存 size を計算し、overflow と range violation を失敗にする。

size 計算は `std::size_t` overflow を検出する。overflow 時は `Status::error == Error::kSchemaError` または
`Error::kValueOutOfRange` とし、出力 size は変更しない。

## 12. decode data flow

decode は次の順序で行う。

1. input pointer と size の組み合わせを検査する。
2. schema の field 配列、field count、message 名を検査する。
3. temporary object を用意する。
4. field を順番に decode し、temporary object だけを更新する。
5. exact decode では reader が末尾に到達したことを検査する。
6. prefix decode では consumed byte 数を計算し、成功時だけ caller へ書く。
7. すべて成功した場合だけ `out = temp` とする。

decode failure invariants:

- `out` は変更しない。
- `consumed` は変更しない。
- `status` が非 null の場合だけ diagnostics を書く。
- input buffer は常に変更しない。

## 13. encode data flow

encode は two-pass を標準とする。

1. output pointer と capacity を検査する。
2. schema の field 配列、field count、message 名を検査する。
3. `TryEncodedSize` と field preflight で size、range、BCD、bit、reserved、length、checksum を検査する。
4. caller buffer とは別の temporary buffer、または caller buffer へ書く前に完全検証済みであることを示せる
   backend を用意する。
5. field を順番に encode する。
6. すべて成功した場合だけ caller buffer の先頭 `encoded_size` byte を更新する。

encode failure invariants:

- output buffer は変更しない。
- extra capacity は成功時も変更しない。
- `status` が非 null の場合だけ diagnostics を書く。
- value object は変更しない。

temporary buffer が stack 固定長で持てない bounded schema では、caller supplied scratch buffer を要求する API を
別名で用意する。core encode は dynamic allocation に頼らない。

## 14. buffer and lifetime policy

- `data == nullptr && size == 0` は空 input として有効。
- `data == nullptr && size > 0` は `Error::kNullInput`。
- `out == nullptr && encoded_size == 0` は成功可能。
- `out == nullptr && encoded_size > 0` は `Error::kNullOutput`。
- input と output は non-owning pointer。
- view field は source buffer lifetime を caller が保持する。
- copy field は source buffer pointer を struct に保存しない。
- field name と schema name は static storage duration を要求する。
- encode output が value object の storage と重なる使い方は API contract 外。

## 15. endianness policy

- multi-byte field は API 名に endian を必ず含める。
- plain `U16`、`U32`、`U64` は採用しない。
- host endian に依存しない。
- unaligned access に依存しない。
- `reinterpret_cast` による整数 load/store を使わない。
- signed field も byte order と two's complement を明示した descriptor だけ採用する。

## 16. BCD policy

- BCD descriptor は nibble が 0..9 の範囲にあることを検査する。
- invalid nibble は `Error::kInvalidBcd`。
- decoded integer descriptor は leading zero を保持しない。
- leading zero が意味を持つ field は raw BCD bytes descriptor を使う。
- encode は BCD 桁数へ収まらない値を `Error::kValueOutOfRange` にする。
- C++11 core API は `std::optional` を public signature に出さない。

## 17. bit field policy

- bit field は storage unit descriptor と logical bit member descriptor の group で扱う。
- decode は storage unit を 1 回読み、logical fields へ mask/shift で展開する。
- encode は logical fields の range をすべて preflight し、1 storage unit へ pack してから書く。
- reserved bits は decode で期待値一致を検査し、encode で期待値を書く。
- bit numbering は descriptor 名または Doxygen で MSB/LSB 基準を明示する。

## 18. exact and prefix decode

exact decode:

- `TryDecode` と `TryDecodeExact` は schema 全体を読み終えた後、input を全消費していることを要求する。
- trailing bytes は `Error::kTrailingBytes`。

prefix decode:

- `TryDecodePrefix` は schema で定義した message だけを読み、成功時に consumed byte 数を返す。
- consumed は成功時だけ更新する。
- prefix decode は stream、envelope payload、TLV value の先頭 message に使う。

## 19. validation hooks

validation hook は schema と同じ C++11 core contract に従う。

- `noexcept`。
- allocation なし。
- 例外なし。
- bool で成功/失敗を返す。
- 失敗時は `Status` に field、offset、expected、actual を設定できる。

length hook は declared length と actual consumed size を比較する。checksum hook は caller が提供した algorithm を
呼び出すだけに留める。cross-field hook は業務固有 protocol の完全実装に踏み込まず、schema-local invariant を
確認する用途に限定する。

## 20. examples as executable contracts

examples を追加する場合は compile 対象にする。文書だけの example は正本にしない。

executable example は次を示す。

- exact fixed frame。
- prefix decode。
- BCD field。
- grouped bit field。
- fixed byte copy field。
- reserved bytes。
- length validation。
- checksum validation hook。
- failure diagnostics。

## 21. completion criteria

`ket-wire` runtime を完了扱いにする条件:

- `packages/wire/ket_wire.h` に公開 API、Doxygen、C++ version、dependency、namespace、drop-in 条件がある。
- `.cpp` が必要な場合は実装を持ち、空 source や include-only source ではない。
- public API は C++11 compile-only check を通る。
- functional test は GoogleTest で境界条件を網羅する。
- package tests は exact decode、prefix decode、encode、diagnostics、size 計算を含む。
- static analysis、convention check、sanitizer test、format check、layout check が通る。
- `progress.md` に package runtime としての検証状態を記録する。
- docs は public API、失敗規約、非目標、TLV との関係を反映する。

## 22. test matrix

必須 test:

- null input + zero size。
- null input + nonzero size。
- null output + zero encoded size。
- null output + nonzero encoded size。
- empty schema。
- short input at every field。
- short output。
- trailing bytes。
- prefix consumed size。
- exact-size output。
- output extra capacity preservation。
- decode failure keeps output object unchanged。
- encode failure keeps output buffer unchanged。
- invalid BCD nibble。
- BCD value out of range。
- bit field range overflow。
- grouped bit field roundtrip。
- reserved byte mismatch。
- reserved bit mismatch。
- length mismatch。
- checksum mismatch。
- computed callback failure。
- size calculation overflow。
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

## 23. implementation work split

長時間の goal-driven 実装では、write scope を分けて並列化する。

- public API owner: `packages/wire/ket_wire.h`。
- implementation owner: `packages/wire/ket_wire.cpp`。
- test owner: `packages/wire/ket_wire_test.cpp` と compile-only check。
- build/metadata owner: `CMakeLists.txt`、`progress.md`、package docs。
- verification owner: read-only checks と failure triage。

各 owner は他 owner の変更を revert しない。既存 binary modules は read-only reference として扱う。

## 24. review checklist

- `ket-wire` は package runtime であり、runtime 配置は `packages/wire/` で固定されている。
- public namespace は `ket::wire`。
- schema descriptor が decode、encode、size、diagnostics の正本になっている。
- macro-first API や code generator を要求していない。
- core API が C++11 互換、`noexcept`、allocation なし、例外なし。
- multi-byte field 名に endian が出ている。
- exact decode と prefix decode が名前で分かれている。
- prefix decode は consumed byte 数を成功時だけ更新する。
- decode 失敗時に output object が不変。
- encode 失敗時に output buffer が不変。
- BCD の decoded integer と raw bytes が descriptor 名で区別されている。
- bit field の shared storage unit を group descriptor で扱っている。
- fixed byte copy field が source pointer lifetime に依存しない。
- view field は lifetime 依存を API 名と Doxygen で説明している。
- reserved bytes/bits、length、checksum、custom validation の diagnostics がある。
- `tlv` を置き換えず、併用関係を説明している。
- socket、transport、IDL、任意 object serialization、protocol server へ広げていない。
