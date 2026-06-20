# ket-wire package design

この文書は、最初の ket package である `ket-wire` の v0 設計である。
v0 design-only の成果物として作成した時点では、生産コード、`modules/wire`、空の package 実装フォルダを
作成対象にしなかった。

## 実装依頼時の読み方

この文書にある「v0 では作らない」「実装しない」「test 追加なし」は、v0 design-only 作業範囲の説明であり、
将来の明示的な実装依頼を拒否する根拠ではない。

ユーザーが `ket-wire` の runtime API、compile 対象 recipe、または関連 module 抽出を明示的に依頼した場合は、
その依頼が現在の作業範囲を更新する。実装対象を確認し、package runtime は `packages/wire/`、compile 対象
recipe は `examples/wire/`、独立 utility は通常の module として扱う。

`modules/wire` は原則として作らない。`wire` は ordinary module ではなく package として扱い、runtime API を
追加する場合の配置は `packages/wire/` を第一候補にする。

## 0. documentation location

v0 の成果物は `docs/packages/ket_wire_design.md` とする。

理由:

- 現在の repository に package 文書領域はないが、これは proposal ではなく package 設計の正本である。
- `packages/wire/README.md` は package code を作る段階まで作らない。
- `docs/proposals/ket_wire_package_design.md` は一時提案の置き場としては自然だが、v0 方針を継続的に参照する
  文書には弱い。
- `examples/wire/README.md` は compile 対象 example を作る段階まで作らない。

将来 code を追加する場合の推奨 layout:

```txt
docs/packages/ket_wire_design.md
examples/wire/...        # compile 対象 example を追加する時だけ作成
packages/wire/...        # package runtime API を追加する時だけ作成
```

## 1. ket package の定義

ket package は、複数の ket module を組み合わせ、実務上まとまった設計層、利用手順、
命名規約、失敗規約、必要なら package 専用 API を提供する単位である。

module は単独で持ち出せる小さい drop-in utility であり、package は複数 module の使い方を
束ねる上位概念である。package は module の独立性ルールを壊さず、依存関係の例外を
package 側だけに閉じ込める。

## 2. module と package の違い

| 観点      | module                               | package                                  |
| --------- | ------------------------------------ | ---------------------------------------- |
| 主単位    | `modules/<name>/ket_<name>.h/.cpp`   | docs、examples、必要時だけ package code  |
| 依存      | 原則として他の ket module に依存なし | 複数 ket module の明示的な合成を許可     |
| 目的      | 小さい定型処理を短い API にする      | 複数処理を実務パターンとして組み立てる   |
| namespace | `ket::<module>`                      | package namespace は別途設計してから採用 |
| 完了条件  | 実装、Doxygen、test、検証            | 設計、recipe、例、必要時だけ API と検証  |

`ket-wire` は `byte_reader`、`byte_writer`、`endian`、`bcd`、`bits` などを
組み合わせる利用層であり、初期状態では単独責務の module ではない。

## 3. `ket-wire` が最初は module ではない理由

`ket-wire` の責務は「固定 binary wire format を C++ struct と相互変換する設計方法」を
与えることにある。これは 1 つの小関数ではなく、次の複数要素の合成である。

- byte cursor による順次読み書き
- endian を明示した整数 field
- BCD の妥当性確認と変換
- 固定長 byte field
- bit field の取り扱い方針
- exact decode、prefix decode、trailing bytes の規約
- field 名を使った診断
- decode/encode 失敗時の object/buffer 不変規約

これらを初手で `modules/wire` に入れると、module の「小さく、独立し、drop-in 可能」
という性質を弱める。したがって v0 は package design と recipe から始める。

## 4. module へ抽出または demote する条件

`ket-wire` 内で見つかった処理は、次を満たす場合だけ普通の module として抽出する。

- `wire` 以外でも独立して使う価値がある。
- 標準ライブラリだけ、または既存 module なしで成立する。
- 公開 API が小さく、責務を一文で説明できる。
- C++ version、失敗規約、境界条件、test 観点を module 単体で固定できる。
- `modules/<name>/` として持ち出しても package 固有語を含まない。

候補例:

- C++11 互換の `TryToInt` / `TryFromInt` 型 BCD helper は `bcd` module の追加候補。
- bit 範囲抽出と挿入が安定すれば `bits` module の追加候補。
- 固定長 byte copy/view の小 helper が一般化できれば `byte_view` または別 module の候補。

package に残すべきもの:

- message schema 全体の合成
- field 名と offset を持つ wire 診断
- 複数 module を前提にした decode/encode orchestration
- exact/prefix decode など package-wide な利用規約

## 5. 目的と非目標

目的:

- 固定長またはほぼ固定長の binary message layout を一度だけ定義する。
- byte 列から C++ struct へ decode し、C++ struct から byte 列へ encode する。
- field 名、offset、失敗理由を diagnostics に使える形で残す。
- core decode/encode では例外、dynamic allocation、所有権の曖昧さを避ける。
- C++11 互換の public surface を優先する。

非目標:

- socket、network connection、transport retry の提供。
- Protocol Buffers、ASN.1、FlatBuffers などの代替。
- 汎用 serialization framework。
- 可変長 object graph、継承階層、多態 codec。
- 業務固有 protocol の完全実装。
- 標準ライブラリや既存 ket module の置き換え。

## 6. target use cases

- embedded device command/response record
- 通信制御 software の固定 binary frame
- big-endian / little-endian の混在 field
- BCD で表現される年月日、時刻、識別番号
- 固定長 byte payload、reserved bytes、checksum 対象範囲
- field 名付きの decode failure log

## 7. v0 scope

v0 design-only の作業範囲では recipe-only package とし、生産 runtime API は追加対象外だった。これは将来の
明示的な実装依頼を拒否する根拠ではない。

含めるもの:

- この設計文書
- schema style recommendation
- failure model convention
- output invariance convention
- exact decode と encode size convention
- 2 つ以上の struct 例
- 将来 API の評価と review checklist

v0 design-only で含めなかったもの:

- `modules/wire`
- `packages/wire` の production header/source
- macro schema
- generic runtime descriptor engine
- package CMake target
- compile 対象の examples

理由:

- 現在の `byte_reader` / `byte_writer` だけで C++11 recipe は十分書ける。
- 現在の `bcd` module は C++17 API であり、C++11-facing runtime API の必須依存に
  するにはまだ早い。
- bit field は複数 logical field が同じ source byte を共有するため、field 単位の単純な
  descriptor へまだ落としにくい。
- encode 失敗時の output buffer 不変を generic API で保証するには、preflight、two-pass
  validation、temporary buffer のどれを採るか設計確認が必要。

## 8. v1+ に明示的に defer する範囲

- package runtime API
- `Status` と field 名付き diagnostics の実装
- pointer-to-member field descriptor engine
- cursor callback wrapper API
- bit field descriptor
- fixed-size array member descriptor
- optional prefix decode API
- generated documentation from schema
- compile 対象 examples
- package-level test helper
- checksum、length field、conditional field

## 9. dependency policy

package は複数 ket module を合成してよい。ただし依存は package 文書と将来 header の
`@par 他のライブラリへの依存` に明記する。ordinary module が `ket-wire` に依存することは
禁止する。

v0 recipe の module 利用方針:

| module         | v0 での扱い                    | 備考                                      |
| -------------- | ------------------------------ | ----------------------------------------- |
| `byte_reader`  | 推奨                           | C++11。cursor decode の主部品。           |
| `byte_writer`  | 推奨                           | C++11。fixed buffer encode の主部品。     |
| `endian`       | 任意                           | raw pointer load/store recipe で利用。    |
| `byte_view`    | 任意                           | non-owning buffer 引数を表したい場合。    |
| `bcd`          | 条件付き                       | 現状 C++17。C++11 core 必須依存にしない。 |
| `bits`         | 任意                           | bit field recipe で利用。                 |
| `numeric`      | 任意                           | size 計算や range preflight で利用。      |
| `bytes`        | examples 限定                  | C++17、allocation あり。core encode 外。  |
| `meta`         | 将来 descriptor 実装時だけ検討 | v0 recipe では不要。                      |
| `tuple`        | 将来 schema 実験のみ           | C++17。v0 では不要。                      |
| `optional`     | C++17 recipe のみ              | C++11 public surface には使わない。       |
| `build_config` | 依存しない                     | 現在 `modules/build_config` は未作成。    |
| `tlv`          | 依存しない                     | 現在 `modules/tlv` は未作成。別 package。 |

## 10. namespace と naming policy

v0 は runtime API を持たないため、package namespace は導入しない。

将来 runtime API を追加する場合は、C++ namespace は `ket::wire` を第一候補とする。
repository 上の配置を `packages/wire/` など `modules/` 外にすることで、`wire` が ordinary
module ではなく package namespace であることを明示する。

`ket_wire` top-level namespace は推奨しない。ket の top-level namespace を 1 つに保つほうが、
既存 module と drop-in rename 方針に合うためである。

API 名の方針:

- endian は `U16Be`、`U16Le`、`U32Be`、`U32Le` のように名前へ必ず出す。
- fallible free function は `TryDecode`、`TryEncode` の形。
- cursor member を作るなら `ReadU16Be` より既存 `byte_reader` に近い `ReadBe16` を優先するか、
  schema field 名との読みやすさを重視して `U16Be` とするかを runtime API 設計時に再評価する。
- field 名は `const char*` で、v0 convention では string literal または static storage のみ。

## 11. public API proposal

v0 の public API はなし。

将来 runtime API を作る場合、最有力候補は pointer-to-member field descriptor API である。
ただし v0 では未採用とする。

将来案:

```cpp
namespace ket
{
    namespace wire
    {
        enum class Error
        {
            kNone,
            kNullData,
            kShortInput,
            kShortOutput,
            kInvalidBcd,
            kValueOutOfRange,
            kTrailingBytes,
            kInternalError
        };

        struct Status
        {
            Error error;
            std::size_t offset;
            const char* field;

            bool Ok() const noexcept;
        };

        template <typename T>
        struct Field;

        template <typename T, std::uint16_t T::*Member>
        Field<T> U16Be(const char* name) noexcept;

        template <typename T, std::uint8_t T::*Member>
        Field<T> BcdU8(const char* name) noexcept;
    } // namespace wire

} // namespace ket
```

この形は C++11 で表現可能だが、bit field、fixed array、custom validation、encode の output
不変を解決してから採用する。

## 12. candidate approach evaluation

### 12.1 Cursor callback API

例:

```cpp
bool ok = ket::wire::TryDecode(data, size, out, DecodeFooFields);
```

評価:

- C++11 でも function template と lambda/function object で実現可能。
- `std::function` を使わなければ dynamic allocation は不要。
- capturing lambda は function pointer へ変換できないが、template callback なら利用可能。
- header-only template API になりやすく、ABI 安定性より source drop-in を優先する形になる。
- schema は data ではなく code なので、field list の再利用、size 計算、自動 diagnostics は弱い。
- `TryDecode` 側で `T temp` を作り、callback が成功した時だけ `out = temp` とすれば decode
  output invariance は実現しやすい。
- encode では writer が逐次 buffer を更新するため、失敗時の output buffer 不変には
  prevalidation または temporary buffer が必要。

結論: v0 で実装しない。手書き recipe としては現実的。将来の軽量 wrapper 候補。

### 12.2 Field descriptor API using pointer-to-member templates

評価:

- C++11 で実現可能。
- `template <typename T, std::uint16_t T::*Member>` のように member 型を signature に含めれば、
  `U16Be` を `std::uint8_t` member に誤って適用する失敗は compile error にできる。
- `Field<T>` に function pointer、encoded size、field name を持たせれば dynamic allocation は不要。
- schema が data になるため、`EncodedSize`、diagnostics、roundtrip test が書きやすい。
- pointer-to-array member、nested struct、conditional field は急に複雑になる。
- BCD は struct member に decoded integer を置く方針なら自然だが、raw BCD byte を保持したい場合は
  別 field 種別が必要。
- bit field は複数 logical field が 1 byte を共有するため、単純な `Field<T>` 配列では
  offset 進行と member 更新が扱いにくい。
- encode output invariance は two-pass validation か temporary buffer が必要。

結論: 将来 runtime API の第一候補。ただし v0 で急いで入れない。

### 12.3 Macro-assisted schema API

評価:

- schema 記述は短くなる。
- 生成される型、関数、namespace、field 名の見通しが悪くなりやすい。
- compiler diagnostics が macro 展開後になり、C++11 利用者にとって読みにくい。
- ket の「小さく明示的な utility」方針に合いにくい。
- Doxygen、test、review checklist と相性が悪い。

結論: v0 では不採用。descriptor API で記述量が受け入れられないと実証された場合だけ再検討。

### 12.4 Recipe-only API using existing ket modules

評価:

- 今すぐ C++11 recipe を書ける。
- `byte_reader` / `byte_writer` の offset 不変規約をそのまま利用できる。
- module API を変更しない。
- runtime framework を追加せず、失敗規約と schema style を先に固められる。
- field 名付き diagnostics や schema 再利用は手書きになる。
- BCD は現状 `ket::bcd` が C++17 のため、C++11 recipe では package-local helper または
  将来の `bcd` module 拡張が必要。

結論: v0 の推奨形。

## 13. failure model

core decode/encode は例外を投げない。失敗は `bool` で返す。

v0 recipe の最小規約:

- `true`: 完全に成功。
- `false`: 入力不足、出力不足、invalid BCD、range 外、trailing bytes、invalid pointer など。
- 失敗時の詳細が必要な recipe は local `Status` を追加してよい。
- failure detail は `error`、`offset`、`field` の 3 要素を推奨する。

将来 `ket::wire::Status` を採用する場合の error 候補:

- `kNone`
- `kNullData`
- `kShortInput`
- `kShortOutput`
- `kInvalidBcd`
- `kValueOutOfRange`
- `kTrailingBytes`
- `kInternalError`

## 14. object mutation と output invariance

decode:

- `TryDecodeXxx(..., T& out)` は失敗時に `out` を変更しない。
- recipe は `T temp = {};` または field ごとの temporary を使い、最後に `out = temp` とする。
- この形は `T` が default constructible かつ assignable であることを要求する。
- その要求を満たせない型では、別 factory recipe を設計し、失敗時不変を個別に示す。

encode:

- `TryEncodeXxx(value, out, out_size)` は失敗時に `out` buffer を変更しないことを推奨する。
- `byte_writer::Writer` は失敗時にそれまでの成功書き込みを戻さないため、recipe 側で対策する。
- 推奨対策は、size と fallible value を先に検査し、固定長 temporary buffer へ書き、成功後に
  caller buffer へ copy すること。
- output capacity は `out_size >= EncodedSize` を成功条件とし、書き込むのは encoded size 分だけ。
  余剰領域は変更しない。

## 15. buffer ownership と lifetime

- `data` と `out` は non-owning pointer。
- `data == nullptr && size == 0` は空 message でだけ有効。
- `data == nullptr && size > 0` は失敗。
- `out == nullptr && EncodedSize == 0` は成功可能。
- `out == nullptr && EncodedSize > 0` は失敗。
- `byte_view::View` / `MutableView` を使う場合も元 buffer lifetime は呼び出し側が保持する。
- field name は `const char*` とし、string literal または static storage duration を要求する。

## 16. endianness policy

- multi-byte integer field は endian を API 名または recipe 名に必ず含める。
- host endian に依存しない。
- plain `U16`、`U32` は使わない。
- recipe では cursor を使うなら `ReadBe16` / `ReadLe16`、raw pointer なら
  `ket::endian::TryLoadBe16` / `TryLoadLe16` を使う。

## 17. BCD policy

- decode では encoded BCD byte を struct にそのまま入れず、decoded decimal value を保存する。
- `BcdU8` は 1 byte packed BCD、値域 0..99 を表す。
- invalid nibble は decode 失敗。
- encode では値が 99 を超える場合に失敗。
- 先頭ゼロを意味として保持したい field は decoded integer ではなく fixed byte field として扱う。
- 現在の `ket::bcd` は C++17 API であるため、C++11-facing runtime API の必須依存にはしない。
  C++11 で BCD を package runtime に入れる前に、`bcd` module への bool-out API 追加を検討する。

## 18. bit-field policy

v0 では bit field descriptor を実装しない。文書と recipe のみとする。

理由:

- bit 7..5 と bit 4..0 のように、複数 logical field が 1 source byte を共有する。
- field descriptor を単純に順番実行すると、offset を field ごとに進める model と合わない。
- decode では 1 byte を読んでから複数 member に展開する必要がある。
- encode では複数 member を 1 byte へ pack する前に range validation が必要。

v0 recipe では raw byte を読み、`ket::bits` の helper または明示的な mask/shift を使う。
安定した形が見えたら `bits` module への範囲抽出 helper か、`ket::wire` package descriptor として再検討する。

## 19. fixed-size byte field policy

v0 recipe で扱ってよい。

- C++11 では `std::uint8_t member[N]` または `std::array<std::uint8_t, N>` を使う。
- decode は `ReadBytes(N, pointer)` 後に member へ copy し、source buffer への pointer を struct に
  保存しない。
- encode は member から fixed size 分だけ copy する。
- pointer-to-member descriptor API では C array と `std::array` の両方をどう扱うか未確定のため、
  runtime descriptor は v1+ に defer する。

## 20. exact decode, prefix decode, trailing bytes

default は exact decode。

- `TryDecodeXxx` は schema 全体を読み終えた後、reader が empty であることを要求する。
- trailing bytes があれば失敗。
- envelope 内の payload や stream prefix を読む用途では、別名の `TryDecodeXxxPrefix` を使う。
- 将来 package API を作る場合、`TryDecode` は exact、`TryDecodePrefix` は消費 byte 数を返す設計を
  別途検討する。

## 21. encoded size calculation

recipe-only v0 では message ごとに `kXxxEncodedSize` を定義する。

```cpp
constexpr std::size_t kCommandFrameEncodedSize = 2U + 1U + 4U;
```

将来 descriptor API では `EncodedSize(fields)` を提供する。ただし dynamic allocation を避け、
固定 schema の合計 byte 数として扱う。variable-length field を入れる場合は別 package phase とする。

## 22. C++ standard requirement

v0 package convention は C++11 を基準にする。

- public signature に `std::optional`、`std::string_view`、`std::span`、CTAD、C++17 namespace 短縮形を使わない。
- C++17-only module を使う recipe は、その recipe を C++17 と明記する。
- 現状の `ket::bcd` を使う BCD recipe は C++17 recipe である。
- 将来 runtime API は C++11 compile-only check を必須にする。

## 23. header-only vs `.cpp`

v0 design-only の文書作成では runtime header/source を作成対象外にした。runtime API の実装依頼では、
次の判断基準に従う。

将来 descriptor API を作る場合:

- member pointer template factory は header 定義が必要。
- `Field<T>` の decode/encode function pointer も template 生成になりやすい。
- 非 template の `Status::Ok` 程度で `.cpp` を作る価値がない場合は header-only にする。
- `.cpp` を置くなら実装がある場合だけにし、空または include だけの source は作らない。

## 24. test plan

v0 design-only の文書作成だけなら test 追加なし。

runtime API または compile 対象 recipe を追加するときの test plan:

- C++11 compile-only check。
- exact decode success。
- 各 field 位置での short input。
- trailing bytes failure。
- null pointer と size の組み合わせ。
- invalid BCD nibble。
- BCD encode value > 99。
- short output。
- encode 失敗時の output buffer 不変。
- decode 失敗時の output object 不変。
- fixed-size byte field が source pointer を保持しないこと。
- endian roundtrip。
- field name と offset を含む status。
- prefix decode を入れる場合は consumed size。

## 25. example usage

### 25.1 C++11 exact fixed frame recipe

```cpp
struct CommandFrame
{
    std::uint16_t command;
    std::uint8_t mode;
    std::uint8_t payload[4];
};

constexpr std::size_t kCommandFrameEncodedSize = 2U + 1U + 4U;

bool TryDecodeCommandFrame(const std::uint8_t* data,
                           std::size_t size,
                           CommandFrame& out) noexcept
{
    ket::byte_reader::Reader reader(data, size);
    CommandFrame temp = {};
    const std::uint8_t* payload = nullptr;

    bool ok = reader.ReadBe16(temp.command);
    if (!ok)
    {
        return false;
    }

    ok = reader.ReadU8(temp.mode);
    if (!ok)
    {
        return false;
    }

    ok = reader.ReadBytes(4U, payload);
    if (!ok)
    {
        return false;
    }

    std::copy(payload, payload + 4U, temp.payload);

    const bool exact = reader.Empty();
    if (!exact)
    {
        return false;
    }

    out = temp;
    return true;
}

bool TryEncodeCommandFrame(const CommandFrame& value,
                           std::uint8_t* out,
                           std::size_t out_size) noexcept
{
    const bool output_is_valid = out != nullptr;
    if (!output_is_valid)
    {
        return false;
    }

    const bool output_is_large_enough = out_size >= kCommandFrameEncodedSize;
    if (!output_is_large_enough)
    {
        return false;
    }

    std::uint8_t temp[kCommandFrameEncodedSize] = {};
    ket::byte_writer::Writer writer(temp, kCommandFrameEncodedSize);

    bool ok = writer.WriteBe16(value.command);
    if (!ok)
    {
        return false;
    }

    ok = writer.WriteU8(value.mode);
    if (!ok)
    {
        return false;
    }

    ok = writer.WriteBytes(value.payload, 4U);
    if (!ok)
    {
        return false;
    }

    const bool exact = writer.Full();
    if (!exact)
    {
        return false;
    }

    std::copy(temp, temp + kCommandFrameEncodedSize, out);
    return true;
}
```

### 25.2 C++17 BCD recipe using current `ket::bcd`

現在の `ket::bcd` は `std::optional` を返すため、この recipe は C++17 例である。
C++11 package runtime へ入れる前に bool-out BCD API が必要である。

```cpp
struct ClockFrame
{
    std::uint8_t year;
    std::uint8_t month;
    std::uint8_t day;
    std::uint16_t sequence;
};

constexpr std::size_t kClockFrameEncodedSize = 1U + 1U + 1U + 2U;

bool TryDecodeClockFrame(const std::uint8_t* data,
                         std::size_t size,
                         ClockFrame& out) noexcept
{
    ket::byte_reader::Reader reader(data, size);
    ClockFrame temp = {};
    std::uint8_t year_bcd = 0U;
    std::uint8_t month_bcd = 0U;
    std::uint8_t day_bcd = 0U;

    bool ok = reader.ReadU8(year_bcd);
    if (!ok)
    {
        return false;
    }

    ok = reader.ReadU8(month_bcd);
    if (!ok)
    {
        return false;
    }

    ok = reader.ReadU8(day_bcd);
    if (!ok)
    {
        return false;
    }

    ok = reader.ReadBe16(temp.sequence);
    if (!ok)
    {
        return false;
    }

    const auto year = ket::bcd::ToInt(year_bcd);
    const bool year_ok = year.has_value();
    if (!year_ok)
    {
        return false;
    }

    const auto month = ket::bcd::ToInt(month_bcd);
    const bool month_ok = month.has_value();
    if (!month_ok)
    {
        return false;
    }

    const auto day = ket::bcd::ToInt(day_bcd);
    const bool day_ok = day.has_value();
    if (!day_ok)
    {
        return false;
    }

    temp.year = static_cast<std::uint8_t>(*year);
    temp.month = static_cast<std::uint8_t>(*month);
    temp.day = static_cast<std::uint8_t>(*day);

    const bool exact = reader.Empty();
    if (!exact)
    {
        return false;
    }

    out = temp;
    return true;
}

bool TryEncodeClockFrame(const ClockFrame& value,
                         std::uint8_t* out,
                         std::size_t out_size) noexcept
{
    const bool output_is_valid = out != nullptr;
    if (!output_is_valid)
    {
        return false;
    }

    const bool output_is_large_enough = out_size >= kClockFrameEncodedSize;
    if (!output_is_large_enough)
    {
        return false;
    }

    const auto year_bcd = ket::bcd::FromInt<std::uint8_t>(value.year);
    const bool year_ok = year_bcd.has_value();
    if (!year_ok)
    {
        return false;
    }

    const auto month_bcd = ket::bcd::FromInt<std::uint8_t>(value.month);
    const bool month_ok = month_bcd.has_value();
    if (!month_ok)
    {
        return false;
    }

    const auto day_bcd = ket::bcd::FromInt<std::uint8_t>(value.day);
    const bool day_ok = day_bcd.has_value();
    if (!day_ok)
    {
        return false;
    }

    std::uint8_t temp[kClockFrameEncodedSize] = {};
    ket::byte_writer::Writer writer(temp, kClockFrameEncodedSize);

    bool ok = writer.WriteU8(*year_bcd);
    if (!ok)
    {
        return false;
    }

    ok = writer.WriteU8(*month_bcd);
    if (!ok)
    {
        return false;
    }

    ok = writer.WriteU8(*day_bcd);
    if (!ok)
    {
        return false;
    }

    ok = writer.WriteBe16(value.sequence);
    if (!ok)
    {
        return false;
    }

    const bool exact = writer.Full();
    if (!exact)
    {
        return false;
    }

    std::copy(temp, temp + kClockFrameEncodedSize, out);
    return true;
}
```

## 26. review checklist

- documentation-only v0 では `ket-wire` を package として扱い、`modules/wire` を作っていない。
- runtime package code を追加する場合は、`packages/wire/` を第一候補にして配置判断を明記した。
- package code を追加する前に recipe で重複と不足を確認した。
- C++11-facing API に C++17-only 型を入れていない。
- dependency が package 側にだけ閉じている。
- decode 失敗時に output object が不変。
- encode 失敗時に output buffer が不変。
- exact decode と prefix decode が名前で分かれている。
- endian が名前に出ている。
- BCD は decoded integer と raw byte のどちらを保存するか明記している。
- bit field の source byte 共有を設計で扱っている。
- fixed byte field が source pointer lifetime に依存しない。
- dynamic allocation が core decode/encode に入っていない。
- field name の lifetime が static storage として説明されている。
- examples を compile 対象にするなら CMake と C++ standard check を追加する。

## 27. open questions

- `bcd` module に C++11-compatible `TryToInt` / `TryFromInt` を追加するか。
- 初回 runtime API は cursor callback と descriptor API のどちらにするか。
- runtime API でも field name を v0 convention と同じく static storage duration の `const char*`
  まで許すか。
- package code の正式配置を `packages/wire/` にするか。
- compile 対象 recipe を `examples/wire/` に置くか、test helper として置くか。
- runtime API でも encode output invariance は v0 convention と同じ temporary buffer 標準にするか。
- runtime API でも `TryEncode` は v0 convention と同じ `out_size >= EncodedSize` を許すか。
- bit field は `bits` module の追加 API として解くか、`ket::wire` descriptor として解くか。
- fixed-size byte field descriptor は C array と `std::array` のどちらを first-class にするか。
- diagnostics `Status` を v1 に入れるか、recipe-local convention に留めるか。
