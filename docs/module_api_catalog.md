# ket module/API catalog

作成日: 2026-06-14

## 1. 正本の目的と使い方

この文書は、ket の module/API 製造依頼に使う正本である。
[`module_api_proposal.md`](proposals/module_api_proposal.md) を入力資料として整理し、
実装依頼時に必要な API、境界条件、テスト観点、禁止事項を module 単位で固定する。

この文書は `catalog.md` の置き換えではない。`catalog.md` は候補APIの保管場所、
`progress.md` は実装状況、各 module header の Doxygen は実装済みAPIの詳細仕様を管理する。
未実装 module の最終的な公開API、C++要件、失敗方針は、この文書の module別仕様カードを正とする。
未実装候補をこの文書へ追加した場合も、候補APIとしての履歴と痛みは `catalog.md` に残す。
製造前に `catalog.md` へ候補が反映されていない場合は、製造着手前に候補項目を追加する。
この文書では `Manufacturing Status` は一覧表の列を正とし、module別仕様カードには同じ status を
重複保持しない。status を変更する場合は、一覧表の `Manufacturing Status` と仕様カード本文の成熟度表現を
同時に見直す。

`Manufacturing Status` が `Ready` または `Existing` である module の仕様カードは、完全な公開signature、
計算量・性能、失敗方針、境界条件、テスト観点を固定済みであり、追加の仕様分割なしで製造依頼できる。
signature の `template`、`const`、`noexcept`、`constexpr`、参照/ポインタ、戻り型は実装契約として扱う。
製造時に逸脱する場合は、逸脱内容と理由を Doxygen と `progress.md` に明記する。

製造依頼では、対象 module の仕様カードだけでなく、この文書の共通ルールと
`AGENTS.md`、`README.md`、`docs/module_lifecycle.md`、`docs/style.md`、
`docs/testing.md` を同時に守る。

## 2. module製造の共通ルール

- 実装する module だけ `modules/<name>/` を作る。空フォルダや空 `.cpp` は作らない。
- 標準形は `modules/<name>/ket_<name>.h`、
  `modules/<name>/ket_<name>.cpp`、`modules/<name>/ket_<name>_test.cpp`。
  header-only で十分な場合は `.cpp` を置かない。
- 公開APIは module ごとの入れ子 `namespace ket::<module>` に置く。C++11/14 の drop-in を維持するため `namespace ket { namespace <module> { ... } }` の入れ子 block 形式で書き、C++17 短縮形は使わない。top-level の `namespace ket` は1つだけ。内部 helper は header 内なら `ket::<module>::detail`、
  `.cpp` 内なら無名 namespace。内部 helper がない場合は file Doxygen に `内部実装：なし` と書く。
- 各 module は原則として他の ket module に依存しない。小さい内部処理の重複は許容する。
- 公開ヘッダは include what you use を守り、自分が必要な標準ヘッダを自分で include する。
- 公開ヘッダは Doxygen `@file` コメント、公開API宣言、内部実装、公開API定義の順に書く。
- 非optionalの出力引数と入出力引数は参照型で受ける。`nullptr` が意味を持つ optional
  出力や C API 境界だけポインタ型を使い、その理由を Doxygen に書く。
- proposal に `T* out` と書かれていた非optional出力は、製造時に `T& out` へ正規化する。
- C++11/14 対応 module では、GoogleTest とは別に最低標準の compile-only check を追加する。
- `std::optional`、`std::string_view`、CTAD は C++17 以降の機能であり、最小要件が C++11/14 の
  module の公開signatureには使わない。失敗は `TryXxx(..., T& out) -> bool`、欠落時の既定値は
  `XxxOr`/`XxxOrDefault`、不在は `XxxOrNull`(pointer)、空文字列または空viewは `XxxOrEmpty` で表現する。`std::optional` 便利版は
  最小要件が C++17 以降の module でのみ採用する。
- C++11 を最小要件にする module の `constexpr` は C++11 制約(単一 return、loop 不可、recursion 可、
  定数評価中の mutation 不可)に収める。純粋な query/変換関数は C++11 でも `constexpr noexcept`、
  出力引数を変更する `TryXxx` は C++11 では `constexpr` を付けず `noexcept` のみとし、C++14 以降で
  `constexpr` 化できる場合はその旨を仕様カードに明記する。
- module 名は folder 名と一致させ、冗長な folder 名は短い別名にする(`parse_numeric`→`parse`、`string_ascii`→`ascii`、`semantic_version`→`version` など)。
- namespace で対象 module が明らかになるため、API 名から module token と型 token を落とす(`ParseIpv4Address`→`ket::ipv4::Parse`、`Ipv4Address`→`ket::ipv4::Address`)。ただし `ket::port::Port`、`ket::uuid::Uuid`、`ket::percent::Percent` のように短い domain 名がそのまま型名として自然な場合は重複を許容する。
- `ket::bcd::ToInt`、`ket::bcd::FromInt` のように、変換先や変換元を名前に出すことで意図が明確になる場合は `To<X>` / `From<X>` の対象 token を残す。
- `parse`、`format` のように操作そのものを module 名にした namespace では、関数名は操作名ではなく対象名を主にする(`ket::parse::UInt<T>()`、`ket::parse::UIntOr<T>()`、`ket::format::Bool()` など)。`ipv4`、`mac`、`version` のように対象 domain を module 名にした namespace では、従来通り `Parse` / `Format` などの正準動詞を使う。
- 正準動詞は domain module の text→値=`Parse`/`TryParse`、`parse` module の text→値=`UInt`/`TryUInt` のような対象名、値→text=`Format`、値↔値=`To<X>`/`From<X>`、単一 encoded 形の codec=`Encode`/`Decode`、buffer 固定位置=`Load<Order><Width>`/`Store<Order><Width>`、stateful cursor の member=`Read*`/`Write*` に揃える。失敗は `TryXxx(..., T& out) -> bool`、欠落時の既定値は `XxxOr`/`XxxOrDefault`、不在は `XxxOrNull`(pointer)、空文字列または空viewは `XxxOrEmpty`、C++17 便利版は `std::optional` を返す。
- `Try` の後ろは動詞、または操作対象として自然に読める名詞にする。値を取得する処理では、単独の名詞より `TryGet<X>` を優先する。
- format 変種は `enum class LetterCase { kLower, kUpper }` か `<T>FormatOptions` の引数で表し、`...Upper` のような名前接尾辞や無名 bool は使わない。`FormatOptions` などの名前付き options 型では、`with_hash` のように意味が名前で固定される bool field を許容する。endian を含む読み書きは `LoadBe32`/`LoadLe32` のように byte order を名前へ必ず出す。
- bool predicate は原則として free 関数で `Is` / `Has` / `Contains` を使う。unit 名は、API 名では広く認知された単位記号（`KiB`、`MiB` など）以外を省略しない。
- `Existing` module は header の Doxygen と実装が正本であり、この文書の記述が header と矛盾する場合は
  header を優先する。仕様を変えるべき場合は破壊的変更提案として明示し、勝手に header を書き換えない。
- `TryXxx` は失敗時に出力引数と対象 object の状態を変更しない。
- 文字列生成、vector 生成、I/O など allocation や stream 例外の可能性がある API には無理に
  `noexcept` を付けない。
- ASCII 限定、endian、non-owning view/ref、lifetime、overflow、invalid input は API 名または
  Doxygen に明記する。
- 標準ライブラリの再実装、framework 化、業務固有ロジック、巨大な protocol 完全実装はしない。

### Manufacturing Status

| Status             | 意味                                                                              |
| ------------------ | --------------------------------------------------------------------------------- |
| `Existing`         | 実装済み。正本には現状仕様と追加禁止範囲を記録する。                              |
| `Ready`            | この正本だけで製造依頼できる。署名、失敗方針、境界条件、テスト観点を固定済み。    |
| `Needs Spec Split` | 候補として有効だが、初回 API の切り方や失敗方針を別途小さく分割してから製造する。 |
| `Recipe`           | module ではなく利用例。module 製造依頼ではなく recipe 作成依頼として扱う。        |

#### Ready 判定基準

`Ready` は優先度ではなく、追加の仕様判断なしで製造できる状態を表す。次の条件を満たせない module は、
一覧表の `Manufacturing Status` を `Needs Spec Split` に戻し、仕様カードへ未確定の設計判断を列挙する。

- 公開signatureの型、`const`、参照/ポインタ、`noexcept`、`constexpr`、macro 名と値域が固定済み。
- 失敗を戻り値、precondition、例外、process termination のどれで扱うか固定済み。
- 最小 C++ 要件、推奨版、非推奨版、標準代替または標準代替なしの理由が固定済み。
- C++11/14 module では、最低標準の compile-only check が必要かどうか固定済み。
- 他 ket module へ依存しない方針と、必要な標準ヘッダまたは platform API が固定済み。
- null、empty、overflow、size不足、lifetime、encoding、endian、platform差など主要境界のテスト観点が固定済み。
- `Naming Audit` を通過し、公開API名が命名規約と module の責務に合っている。
- `Do not implement` で初回製造時に広げない範囲が固定済み。

#### Naming Audit

`Ready` にする前に、公開API名は次を確認し、仕様カード内の canonical name として固定する。

- module token と型 token を公開API名で重複させない。ただし `ket::port::Port`、`ket::uuid::Uuid`、
  `ket::percent::Percent` のように短い domain 名がそのまま自然な型名になる場合は許容する。
- `ket::<module>::<API>` として読んだときに冗長でないことを確認する。
- `Kind`、`Address`、`View` のような短い型名は、namespace が十分な文脈を持つ場合に優先する。
- ASCII、endian、non-owning、unit、lifetime など誤解しやすい制約は、名前か Doxygen のどちらかに必ず出す。
  module名から推測しにくい場合は名前に出す。
- 標準 API と同名で意味が異なる場合は、対象や単位を名前に足して誤読を避ける。
- generic verb が変換、丸め、単位解釈を隠す場合は、`To<X>`、`From<X>`、または unit 名で入力・出力の意味を明示する。
- 失敗表現は `TryXxx(..., out&) -> bool`、`Xxx() -> std::optional<T>`、`XxxOr`/`XxxOrDefault`/
  `XxxOrNull`/`XxxOrEmpty` のいずれかに揃える。
- format 変種は API 名接尾辞ではなく、`LetterCase` や `FormatOptions` などの引数で表す。
- `Parse`、`Format`、`Encode`、`Decode`、`To<X>`、`From<X>`、`Load<Order><Width>`、
  `Store<Order><Width>` の正準動詞から外れる名前は、namespace と仕様だけで意味が閉じる場合に限る。
- `ket::percent::Percent::TryFromPercent` は `TryFromBasisPoints` / `TryFromRatio` と入力単位を揃えるため維持する。
- `ket::ascii::SplitViews` は owning/non-owning の違いを名前で区別するため維持する。
- 現時点で維持する境界名は `hex::Dump`、`file::Size`、`state::Next`、`function::Noop`、
  `byte_view::TryAt`、`memory::Zero`。いずれも対象 module の責務が狭く、名前だけで操作対象が特定できるため採用する。

`P0`、`P1`、`P2`、`P3` は実装優先度であり、仕様成熟度ではない。高優先度でも上記を満たさないものは
`Needs Spec Split` とし、低優先度でも判断が固定済みなら `Ready` とする。
proposal 段階で P1/P2/P3 に「仕様を小さく切る」「大きくなりやすい」注意があった module も、
この文書の仕様カードで初回API、失敗方針、境界条件、テスト観点、広げない範囲を固定済みなら
`Ready` として扱う。逆に、実装検討中に未固定判断が見つかった場合は priority に関係なく
`Needs Spec Split` へ戻す。

`Ready` と `Existing` の仕様カードは次の項目をこの順で固定する: `Purpose`、`C++ version`、
`Drop-in files`、`Dependencies`、`Public API Signatures`、`Behavior`、`Failure/edge cases`、
`Complexity/performance`、`Tests`、`Do not implement`。APIごとに標準代替の登場版や採用理由が
異なる場合は、`Do not implement` の直前に任意項目 `API別標準代替` を置いてよい。`Needs Spec Split` の仕様カードは
`Public API Signatures` の代わりに `Public API (候補)` と `未確定の設計判断` を持ち、製造前に確定すべき
判断を箇条書きで列挙する。

### catalog.md 対応

`catalog.md` は候補APIの痛み、候補名、失敗条件、テスト観点を残す履歴であり、この文書は製造依頼時の
署名・境界条件・禁止範囲を固定する正本である。`Ready` または `Recipe` を製造依頼に使う前に、次の
Idea が `catalog.md` に存在することを確認する。

| Module / Recipe  | catalog.md Idea             |
| ---------------- | --------------------------- |
| `bcd`            | `Idea: Bcd`                 |
| `string`         | `Idea: String`              |
| `bits`           | `Idea: Bits`                |
| `numeric`        | `Idea: Numeric`             |
| `endian`         | `Idea: Endian`              |
| `hex`            | `Idea: Hex`                 |
| `parse`          | `Idea: ParseNumeric`        |
| `enums`          | `Idea: EnumTable`           |
| `container`      | `Idea: Container`           |
| `ascii`          | `Idea: StringAscii`         |
| `scope`          | `Idea: Scope`               |
| `byte_reader`    | `Idea: ByteReader`          |
| `byte_writer`    | `Idea: ByteWriter`          |
| `bytes`          | `Idea: BytesBuilder`        |
| `date`           | `Idea: Date`                |
| `deadline`       | `Idea: Deadline`            |
| `cli`            | `Idea: Cli`                 |
| `byte_view`      | `Idea: ByteView`            |
| `utf8`           | `Idea: Utf8`                |
| `file`           | `Idea: File`                |
| `io_stream`      | `Idea: IoStream`            |
| `format`         | `Idea: FormatValue`         |
| `ranges`         | `Idea: AlgorithmRange`      |
| `memory`         | `Idea: Memory`              |
| `pointer`        | `Idea: Pointer`             |
| `testing`        | `Idea: TestingBytes`        |
| `version`        | `Idea: SemanticVersion`     |
| `ipv4`           | `Idea: Ipv4`                |
| `port`           | `Idea: Port`                |
| `mac`            | `Idea: MacAddress`          |
| `function`       | `Idea: Function`            |
| `variant`        | `Idea: VariantMatch`        |
| `optional`       | `Idea: OptionalExt`         |
| `contract`       | `Idea: Contract`            |
| `interop`        | `Idea: CInterop`            |
| `platform`       | `Idea: PlatformError`       |
| `state`          | `Idea: StateTable`          |
| `cache`          | `Idea: CacheOnce`           |
| `tlv`            | `Idea: Tlv`                 |
| `tuple`          | `Idea: Tuple`               |
| `build_config`   | `Idea: BuildConfig`         |
| `math`           | `Idea: MathSmall`           |
| `lang`           | `Idea: Language`            |
| `object`         | `Idea: Object`              |
| `meta`           | `Idea: Meta`                |
| `concurrency`    | `Idea: ConcurrencySmall`    |
| `uuid`           | `Idea: Uuid`                |
| `color`          | `Idea: ColorRgb`            |
| `percent`        | `Idea: Percent`             |
| `binary_payload` | `Idea: BinaryPayloadRecipe` |
| `command_parser` | `Idea: CommandParserRecipe` |
| `c_api_wrapper`  | `Idea: CApiWrapperRecipe`   |

## 3. module/API 一覧表

`C++ Min` は最小要件、`C++ 推奨` は適用を推奨する版、`C++ 非推奨` は標準ライブラリで
容易かつ明確に代替できるため適用を推奨しない版を表す。3列は各 module header の Doxygen
`@par C++バージョン要件` と一致させる。`C++ 非推奨` の `なし` は、対象範囲の標準ではこの
module の中核 API に同等の標準代替がないことを意味する。`API別` は module 全体を非推奨にせず、
API別標準代替で個別に標準代替・採用理由を固定することを意味する。`Representative API` は一覧用の代表名であり、
完全な公開APIは module別仕様カードを正とする。

| Module           | Priority | Manufacturing Status | C++ Min | C++ 推奨  | C++ 非推奨 | Files                            | Kind        | Purpose                                      | Representative API                                                                                                                               | Failure/Boundary Policy                                  | Tests                                   | Notes                            |
| ---------------- | -------- | -------------------- | ------- | --------- | ---------- | -------------------------------- | ----------- | -------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------------------------------------------- | --------------------------------------- | -------------------------------- |
| `bcd`            | done     | `Existing`           | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | binary      | packed BCD 変換                              | `ket::bcd::ToInt`, `ket::bcd::FromInt<std::uint8_t>`, `ket::bcd::Format`                                                                         | 不正nibble、空入力、overflow は失敗値                    | 実装済み境界値テスト                    | 追加は BCD 妥当性判定まで        |
| `string`         | done     | `Existing`           | C++17   | C++17以降 | なし       | header-only + test               | string      | 文字列片の連結と追記                         | `ket::string::Cat`, `ket::string::Append`                                                                                                        | raw C string は非null、allocation例外あり                | 実装済み境界値テスト                    | format API ではない              |
| `bits`           | P0       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test               | numeric     | bit/nibble/mask の事故防止                   | `ket::bits::HighNibble`, `ket::bits::LowNibble`, `ket::bits::TryPackNibbles`, `ket::bits::TryMask`                                               | unsigned integral 限定、char系と範囲外 bit は失敗/false  | nibble、bit幅、mask境界                 | C++20 `<bit>` は一部のみ重複     |
| `numeric`        | P0       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test               | numeric     | overflow、align、cast の小さい正解           | `ket::numeric::TryAlignUp`, `ket::numeric::TryAdd`, `ket::numeric::TryCast`                                                                      | 0除算、alignment 0、overflow は失敗                      | min/max、overflow、signed/unsigned      | optional convenience は初回なし  |
| `endian`         | P0       | `Ready`              | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test             | binary      | unaligned/endian 読み書き                    | `ket::endian::LoadBe32`, `ket::endian::LoadLe32`, `ket::endian::StoreBe16`, `ket::endian::TryLoadBe32`                                           | plain load/store は precondition、Try は失敗値           | BE/LE、null、size不足                   | reinterpret cast 禁止            |
| `hex`            | P0       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | diagnostic  | bytes と16進文字列/hex dump                  | `ket::hex::Encode`, `ket::hex::Decode`, `ket::hex::Dump`                                                                                         | null+非0 size は precondition、奇数桁は失敗              | 空入力、separator、不正文字             | dump形式は固定                   |
| `parse`          | done     | `Existing`           | C++17   | C++17以降 | なし       | header-only + test               | parsing     | `from_chars` 周りの儀式除去                  | `ket::parse::TryUInt`, `ket::parse::UInt`, `ket::parse::Bool`, `ket::parse::Hex`                                                                 | 完全消費、whitespaceなし、overflow失敗、hex符号拒否      | 実装済み境界値テスト                    | bool は case-sensitive           |
| `enums`          | P0       | `Ready`              | C++17   | C++17以降 | なし       | header-only + test               | enum        | enum class と文字列変換                      | `ket::enums::Entry`, `ket::enums::Name`, `ket::enums::Parse`, `ket::enums::HasFlag`                                                              | table完全一致、重複は先勝ち                              | known/unknown、duplicate、flags         | reflection はしない              |
| `container`      | P0       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test               | container   | map/vector の小さい儀式                      | `ket::container::Contains`, `ket::container::ContainsKey`, `ket::container::AtOrNull`, `ket::container::AtOr`                                    | 見つからない場合は null/default/0                        | key有無、factory呼び出し、削除件数      | `IndexOf`/`Append` は初回外      |
| `ascii`          | done     | `Existing`           | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | string      | ASCII 前提の文字列処理                       | `ket::ascii::Trim`, `ket::ascii::SplitViews`, `ket::ascii::ToLower`, `ket::ascii::ReplaceAll`                                                    | ASCII whitespaceのみ、view lifetime明記                  | 実装済み境界値テスト                    | Unicode処理ではない              |
| `scope`          | done     | `Existing`           | C++11   | C++11以降 | なし       | header-only + test               | RAII        | cleanup と復元漏れ防止                       | `ket::scope::Exit`, `ket::scope::MakeExit`, `ket::scope::Restore`, `ket::scope::MakeRestore`                                                     | destructor 例外は terminate、move後 inactive             | dismiss、move、二重実行なし             | `Finally` は作らない             |
| `byte_reader`    | P0       | `Ready`              | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test             | binary      | byte列の安全な逐次読み取り                   | `ket::byte_reader::Reader`, `ket::byte_reader::Reader::ReadU8`, `ket::byte_reader::Reader::ReadBytes`                                            | 成功時だけ offset 更新、invalid reader は失敗            | 空、ぴったり、size不足、offset保持      | endian module 非依存             |
| `byte_writer`    | done     | `Existing`           | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test             | binary      | fixed buffer への安全な逐次書き込み          | `ket::byte_writer::Writer`, `ket::byte_writer::Writer::WriteU8`, `ket::byte_writer::Writer::WriteBytes`                                          | 成功時だけ offset と buffer 更新                         | 空、ぴったり、size不足、buffer不変      | endian module 非依存             |
| `bytes`          | P0       | `Ready`              | C++17   | C++17以降 | なし       | header-only + test               | binary      | owning payload builder                       | `ket::bytes::Builder`, `ket::bytes::Builder::AppendU8`, `ket::bytes::AppendBe16`                                                                 | allocation例外あり、null+非0 size は precondition        | BE/LE、reserve、Build後move             | fluent と free を併用            |
| `date`           | P0       | `Ready`              | C++11   | C++11〜17 | C++20以降  | header-only + test               | date        | 日付・時刻の妥当性                           | `ket::date::IsLeapYear`, `ket::date::IsValidDateTime`, `ket::date::TryGetDaysInMonth`                                                            | Gregorian、year >= 1、leap secondなし                    | 2000/1900、2/29、month 0/13             | C++20以降は `std::chrono` を優先 |
| `deadline`       | P0       | `Ready`              | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test             | time        | timeout と elapsed time                      | `ket::deadline::Stopwatch`, `ket::deadline::Deadline`                                                                                            | `steady_clock`のみ、負timeoutは期限切れ                  | zero/future/remaining/restart           | system_clock と混ぜない          |
| `cli`            | done     | `Existing`           | C++17   | C++17以降 | なし       | header-only + test               | CLI         | 小さい CLI option 取得                       | `ket::cli::ArgvView`, `ket::cli::HasOption`, `ket::cli::OptionValue`, `ket::cli::OptionValueOr`, `ket::cli::PositionalArguments`                 | `--key value`/`--key=value`、bare `--`終端、重複は先勝ち | 実装済み境界値テスト                    | 値 parse は parse                |
| `byte_view`      | P1       | `Ready`              | C++11   | C++11〜17 | C++20以降  | header-only + test               | view        | non-owning byte span                         | `ket::byte_view::View`, `ket::byte_view::MutableView`, `ket::byte_view::View::TrySlice`                                                          | `nullptr+0` は空、`nullptr+非0` は invalid               | lifetime、slice、bounds                 | constructor を使う               |
| `utf8`           | done     | `Existing`           | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | text        | UTF-8 検査の隔離                             | `ket::utf8::Validate`, `ket::utf8::IsValid`, `ket::utf8::CountCodePoints`, `ket::utf8::IsAscii`                                                  | normalizationなし、不正offsetを返す                      | 実装済み境界値テスト                    | grapheme数ではない               |
| `file`           | done     | `Existing`           | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | filesystem  | ファイル全読み/全書きと基本query             | `ket::file::TryReadAllText`, `ket::file::TryWriteAllBytes`, `ket::file::Exists`, `ket::file::Size`                                               | optional error、巨大file、非atomic write                 | 実装済み境界値テスト                    | header Doxygen が正本            |
| `io_stream`      | done     | `Existing`           | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test             | stream      | stream の確実な読み書き                      | `ket::io_stream::TryReadExactly`, `ket::io_stream::TryWriteAll`, `ket::io_stream::TryReadLineTrimRightAscii`, `ket::io_stream::FormatStateSaver` | read/writeは完了時だけ成功、行読みは成功時だけout更新    | 実装済み境界値テスト                    | ASCII右端trimのみ                |
| `format`         | P1       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | diagnostic  | 診断用文字列化                               | `ket::format::Bool`, `ket::format::ByteCount`, `ket::format::Duration`                                                                           | allocation例外あり、表記は固定                           | 単位境界、負duration、幅指定            | byte単位は IEC 1024 固定         |
| `ranges`         | done     | `Existing`           | C++11   | C++11以降 | なし       | header-only + test               | algorithm   | index付き range 走査                         | `ket::ranges::ForEachWithIndex`, `ket::ranges::FindIndexIf`                                                                                      | not found は false/out不変、callable/predicate例外伝播   | 実装済み境界値テスト                    | std algorithm 別名なし           |
| `memory`         | done     | `Existing`           | C++11   | C++11以降 | なし       | header-only + test               | memory      | alignment/object bytes                       | `ket::memory::IsAligned`, `ket::memory::TryAlignUp`, `ket::memory::SecureZero`                                                                   | null非0 zeroing は terminate、secure zero は best-effort | alignment 0、overflow、partial zeroing  | object lifetime へ踏み込まない   |
| `pointer`        | done     | `Existing`           | C++11   | C++11以降 | なし       | header-only + test               | pointer     | null/ownership の明示                        | `ket::pointer::NotNull`, `ket::pointer::LockWeak`, `ket::pointer::AddressOf`                                                                     | invalid_argument、lifetime非追跡、void対象外             | 実装済み境界値テスト                    | header Doxygen が正本            |
| `testing`        | done     | `Existing`           | C++17   | C++17以降 | なし       | test-helper `.h` + `.cpp` + test | testing     | bytes系テスト補助                            | `ket::testing::BytesEqual`, `ket::testing::HexEqual`                                                                                             | GoogleTest 依存を明記、mismatch情報を返す                | offset差分、hex差分、不正hex            | library本体ではない              |
| `version`        | done     | `Existing`           | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | parsing     | numeric version triplet parse/format/compare | `ket::version::Triplet`, `ket::version::Parse`, `ket::version::Format`, `ket::version::Compare`                                                  | numeric tripletのみ、leading zero失敗                    | 0.0.0、format、compare、overflow        | full SemVer ではない             |
| `ipv4`           | done     | `Existing`           | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | network     | IPv4 parse/format                            | `ket::ipv4::Address`, `ket::ipv4::Parse`, `ket::ipv4::Format`, `ket::ipv4::ToBe32`, `ket::ipv4::FromBe32`                                        | dotted decimalのみ、leading zero失敗                     | octet境界、個数不足/過多、BE32、format  | IPv6/CIDRなし                    |
| `port`           | done     | `Existing`           | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | network     | TCP/UDP port parse/format                    | `ket::port::Port`, `ket::port::TryFromUInt`, `ket::port::Parse`, `ket::port::Format`                                                             | 0〜65535、空白・符号・leading zero失敗                   | 0/65535、overflow、不正文字             | socket addressなし               |
| `mac`            | P1       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | network     | MAC address parse/format                     | `ket::mac::Address`, `ket::mac::Parse`, `ket::mac::Format`                                                                                       | `:` と `-` を許可、混在とCisco形式は失敗                 | upper/lower、不正hex、区切り            | Cisco形式なし                    |
| `function`       | P2       | `Ready`              | C++17   | C++17以降 | なし       | header-only + test               | callable    | callable/visitor の儀式除去                  | `ket::function::Overload`, `ket::function::MakeOverload`, `ket::function::Noop`                                                                  | `Noop` は例外なし、`Overload` は所有                     | visit、overload解決、copy/move          | `FunctionRef` は初回なし         |
| `variant`        | done     | `Existing`           | C++17   | C++17以降 | なし       | header-only + test               | variant     | `std::variant` visitor 補助                  | `ket::variant::Match`                                                                                                                            | visitor 例外は伝播                                       | value/ref、const、exception伝播         | overload は内部に持つ            |
| `optional`       | done     | `Existing`           | C++17   | C++17以降 | API別      | header-only + test               | optional    | optional の小さい合成                        | `ket::optional::Map`, `ket::optional::AndThen`, `ket::optional::ValueOrEval`                                                                     | factory は必要時だけ呼ぶ                                 | empty/value、参照、factory回数          | transform/and_then は C++23      |
| `contract`       | done     | `Existing`           | C++11   | C++11以降 | なし       | header-only + test               | contract    | precondition 明示                            | `KET_EXPECTS`, `KET_ENSURES`, `KET_REQUIRE_NON_NULL`, `ket::contract::IsInBounds`                                                                | 違反は常時評価して `std::terminate`                      | death、式1回評価、nullptr、bounds       | NDEBUG 非連動                    |
| `interop`        | P2       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test               | interop     | C API 境界の事故防止                         | `ket::interop::ErrnoGuard`, `ket::interop::CopyStringToBuffer`, `ket::interop::UniqueHandle`                                                     | engaged flag で deleter 制御                             | errno復元、buffer不足、release          | OS handle専用化しない            |
| `platform`       | P2       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | platform    | errno/Windows error の文字列化               | `ket::platform::FormatErrno`, `ket::platform::FormatWindowsError`, `ket::platform::GetEnvironmentVariable`                                       | Windows API は `_WIN32` 限定、env missing は空           | known errno、missing env、Windows guard | Windows は UTF-8 変換            |
| `state`          | done     | `Existing`           | C++17   | C++17以降 | なし       | header-only + test               | state       | 小さい状態遷移表                             | `ket::state::Transition`, `ket::state::IsAllowed`, `ket::state::Next`                                                                            | table先頭一致、未定義遷移は失敗                          | known/unknown、duplicate、enum、empty   | FSM frameworkなし                |
| `cache`          | P2       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test               | cache       | once/lazy value                              | `ket::cache::Lazy<T>`, `ket::cache::Lazy<T>::HasValue`, `ket::cache::Lazy<T>::GetOrCreate`                                                       | non-thread-safe、例外後は空                              | factory回数、reset、例外後状態          | thread-safe にしない             |
| `tlv`            | done     | `Existing`           | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test             | binary      | length-prefix/TLV                            | `ket::tlv::Encode`, `ket::tlv::Append`, `ket::tlv::TryDecode`                                                                                    | type u16/length u32 BE、decode失敗は out不変             | 短い入力、length超過、roundtrip、C++11  | header は6 bytes                 |
| `tuple`          | P2       | `Ready`              | C++17   | C++17以降 | なし       | header-only + test               | tuple       | tuple/pair の小さい補助                      | `ket::tuple::ForEach`, `ket::tuple::Transform`                                                                                                   | evaluation order と戻り型を固定                          | empty、heterogeneous、const             | structured binding 競合は避ける  |
| `build_config`   | P2       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test               | config      | feature detection                            | `KET_CXX_VERSION`, `KET_HAS_STD_OPTIONAL`, `KET_OS_LINUX`                                                                                        | macro は `KET_` prefix、値は 0/1                         | compiler/OS 条件、include順             | 他moduleの必須依存にしない       |
| `math`           | done     | `Existing`           | C++11   | C++11以降 | なし       | header-only + test               | math        | 補間・角度・単位変換                         | `ket::math::Lerp`, `ket::math::NearlyEqual`, `ket::math::TryBytesFromKiB`                                                                        | 浮動小数点演算は FP型限定、byte変換はoverflow失敗        | endpoints、epsilon、large values        | units frameworkなし              |
| `lang`           | done     | `Existing`           | C++11   | C++11以降 | API別      | header-only + test               | language    | C++言語の小さい儀式                          | `ket::lang::IgnoreUnused`, `ket::lang::ArraySize`, `ket::lang::AsConst`                                                                          | 標準代替の登場版をAPI別に明記                            | unused/move-only、配列長、拒否、const化 | `Unreachable` は初回外           |
| `object`         | done     | `Existing`           | C++11   | C++11以降 | なし       | header-only + test               | object      | copy/move/regular型の儀式                    | `ket::object::NonCopyable`, `ket::object::NonMovable`, `ket::object::MoveOnly`, `ket::object::ResetOnMove`                                       | mixin は比較演算を宣言しない                             | copy禁止、move、reset、空base           | =deleteで足りる範囲は入れない    |
| `meta`           | P3       | `Ready`              | C++11   | C++11以降 | API別      | header-only + test               | meta        | type traits 補助                             | `ket::meta::RemoveCvref`, `ket::meta::TypeIdentity`, `ket::meta::VoidT`                                                                          | alias のみ、評価時動作なし                               | alias、SFINAE、C++11 compile            | module単位では非推奨にしない     |
| `concurrency`    | P3       | `Ready`              | C++11   | C++11以降 | API別      | header-only + test               | concurrency | join/lock/timeout の局所補助                 | `ket::concurrency::JoiningThread`, `ket::concurrency::JoiningThread::Joinable`, `ket::concurrency::IsReady`                                      | dtorでjoin、move代入で旧threadをjoin                     | move、joinable、ready timeout           | module単位では非推奨にしない     |
| `uuid`           | P3       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test             | parsing     | UUID parse/format                            | `ket::uuid::Uuid`, `ket::uuid::Parse`, `ket::uuid::Format`                                                                                       | canonical hyphen形式のみ、generationなし                 | upper/lower、不正長、不正hex            | 乱数/OS APIなし                  |
| `color`          | done     | `Existing`           | C++11   | C++11以降 | なし       | header-only + test               | value       | RGB小値型                                    | `ket::color::Rgb`, `ket::color::TryParse`, `ket::color::Format`                                                                                  | C++11なので Try-parse、alpha なし                        | hex長、不正文字、format、C++11 compile  | 6桁hex、先頭 # 任意              |
| `percent`        | P3       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test               | value       | percent小値型                                | `ket::percent::Percent`, `ket::percent::Percent::TryFromRatio`, `ket::percent::Percent::FromPercentClamped`                                      | basis points、範囲外/NaN は失敗、clamp は0%              | 0/100、負値、>100、ratio、rounding      | 内部表現は0..10000               |
| `binary_payload` | P3       | `Recipe`             | mixed   | —         | —          | `recipes/...`                    | recipe      | binary payload 構築例                        | recipe code                                                                                                                                      | module API は追加しない                                  | example build/test                      | module製造依頼ではない           |
| `command_parser` | P3       | `Recipe`             | mixed   | —         | —          | `recipes/...`                    | recipe      | CLI/parser/enum の組み合わせ例               | recipe code                                                                                                                                      | module API は追加しない                                  | example build/test                      | module製造依頼ではない           |
| `c_api_wrapper`  | P3       | `Recipe`             | mixed   | —         | —          | `recipes/...`                    | recipe      | C API 境界 RAII 化例                         | recipe code                                                                                                                                      | module API は追加しない                                  | example build/test                      | module製造依頼ではない           |

## 4. 製造依頼プロンプト雛形

```txt
docs/module_api_catalog.md の「<XXX> Module」仕様に従って、<XXX> Module を製造してください。
AGENTS.md、README.md、docs/module_lifecycle.md、docs/style.md、docs/testing.md の規約を守ってください。
他の ket module には依存させず、必要な標準ヘッダを自分で include してください。
実装、Doxygen、GoogleTest、最低C++標準のcompile-only check、progress.md更新、検証まで1セットで行ってください。
```

`Manufacturing Status` が `Needs Spec Split` の module は、この雛形を使う前に仕様分割を行う。
`Recipe` は「XXX Recipe を作成してください」と依頼し、module folder を作らない。

## 5. module別仕様カード

### bcd Module

- Purpose: packed BCD と10進整数・10進文字列の相互変換。Existing module のため、公開詳細は
  header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  packed BCDの直接代替が標準ライブラリになく、`std::optional`で失敗値を明確に扱える。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/bcd/ket_bcd.h`、`modules/bcd/ket_bcd.cpp`、
  `modules/bcd/ket_bcd_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::bcd`）:
  - `constexpr std::optional<int> ToInt(std::uint8_t value) noexcept`
  - `constexpr std::optional<int> ToInt(std::uint16_t value) noexcept`
  - `constexpr std::optional<int> ToInt(std::uint32_t value) noexcept`
  - `template <typename Packed> constexpr std::optional<Packed> FromInt(int value) noexcept`
  - `std::optional<std::string> Format(const std::uint8_t* data, std::size_t size)`
  - `std::optional<std::vector<std::uint8_t>> Parse(std::string_view text)`
- Behavior: 固定幅 packed BCD は整数へ変換し、任意バイト長 packed BCD は桁数と先頭ゼロを
  10進文字列として保持する。`FromInt<Packed>` の `Packed` は `std::uint8_t`、`std::uint16_t`、
  `std::uint32_t` のみ対応し、それぞれ2桁、4桁、8桁の fixed-width packed BCD を返す。
- Failure/edge cases: nibble > 9、`nullptr`、空入力、非10進数字、整数 overflow、負数、
  固定幅桁数超過は失敗値。`Format` は既存実装どおり `data == nullptr` または `size == 0`
  を `std::nullopt` として扱い、空文字列は返さない。
- Complexity/performance: 固定幅 `ToInt`/`FromInt` は定数時間、`constexpr` でコンパイル時評価可。
  `Format`/`Parse` は入力長 O(n)で、戻り値の生成に allocation 1回。
- Tests: 0x00、0x09、0x10、0x99、不正nibble、空入力失敗、先頭ゼロ保持、奇数桁文字列、非数字。
- Do not implement: `BcdDate`、`BcdTime`、業務固有BCD解釈。妥当性判定を追加する場合も
  `IsBcdByte`/`IsBcd16`/`IsBcd32` の byte/word 単位に留め、nibble単体判定の公開APIは追加しない。

### string Module

- Purpose: format ではない文字列片の連結と既存文字列への追記。Existing module のため、公開詳細は
  header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view`を利用でき、文字列片連結を標準ライブラリのみで安全に薄く包める。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/string/ket_string.h`、`modules/string/ket_string_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::string`）:
  - `template <typename... Parts> std::string Cat(const Parts&... parts)`
  - `template <typename... Parts> void Append(std::string& destination, const Parts&... parts)`
- Behavior: `std::string_view` に変換可能な文字列片と `char` を入力順に連結する。embedded
  NUL は length-aware に保持する。`Append` は destination 自身に由来する入力を含む場合でも、
  追記部分を先に確定してから destination を更新する。
- Failure/edge cases: raw C string は非nullかつ null 終端が precondition。合計長が
  `std::string::max_size()` を超える場合は `std::length_error`。長さ計算や一時領域確保で例外が出た場合、
  `Append` は destination を変更しない。
- Complexity/performance: 全要素長の合計 O(n)。`Cat` は合計長を先に求めて 1回 reserve し、
  中間 allocation を避ける。`Append` は destination の既存容量を再利用する。
- Tests: empty argument、`char`、`std::string`、`std::string_view`、embedded NUL、
  self-reference append。
- Do not implement: 数値・enum・stream 変換、format API、`std::format` の再実装。

### bits Module

- Purpose: bit、nibble、mask、rotate の危険な小処理を安全に名前付き API 化。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  unsigned integral の小さいbit処理を標準ライブラリだけで安全に名前付けできる。
  非推奨版 なし。非推奨理由: C++20 `<bit>` と一部重なるが、nibble、mask失敗値、
  bit index境界処理の直接代替ではない。
- Drop-in files: `modules/bits/ket_bits.h`、`modules/bits/ket_bits_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::bits`）:
  - `constexpr bool IsNibble(std::uint8_t value) noexcept`
  - `constexpr std::uint8_t HighNibble(std::uint8_t value) noexcept`
  - `constexpr std::uint8_t LowNibble(std::uint8_t value) noexcept`
  - `bool TryPackNibbles(std::uint8_t high, std::uint8_t low, std::uint8_t& out) noexcept`
  - `template <typename T> constexpr unsigned TypeBitWidth() noexcept`
  - `template <typename T> constexpr bool HasBit(T value, unsigned bit_index) noexcept`
  - `template <typename T> bool TrySetBit(T value, unsigned bit_index, T& out) noexcept`
  - `template <typename T> bool TryClearBit(T value, unsigned bit_index, T& out) noexcept`
  - `template <typename T> bool TryToggleBit(T value, unsigned bit_index, T& out) noexcept`
  - `template <typename T> bool TryMask(unsigned width, T& out) noexcept`
  - `template <typename T> constexpr unsigned PopCount(T value) noexcept`
  - `template <typename T> constexpr bool IsPowerOfTwo(T value) noexcept`
  - `template <typename T> constexpr T Rotl(T value, unsigned count) noexcept`
  - `template <typename T> constexpr T Rotr(T value, unsigned count) noexcept`
- Behavior: template API は bool、char、wchar_t、char16_t、char32_t を除く unsigned integral
  対象。`TypeBitWidth<T>()` は型 `T` の bit 数を返し、値の有効 bit 幅ではない。rotate は count
  を bit幅で剰余化する。純粋な query/変換は C++11 でも `constexpr`。出力引数を変更する
  `TryXxx` は C++11 では `noexcept` のみ、C++14 以降で `constexpr` 化してよい。
- Failure/edge cases: bit index 範囲外の `HasBit` は `false`。`TryXxx` は範囲外や不正nibbleで
  `false`、出力不変。`TryMask(0)` は 0、`TryMask(TypeBitWidth<T>())` は全bit 1。
- Complexity/performance: 各APIは語幅に対し定数〜O(bit幅)。`PopCount` は C++11 では recursion、
  C++14 以降は loop で O(bit幅)。`constexpr` query はコンパイル時に評価できる。allocation・例外なし。
- Tests: nibble境界、byte生成、TypeBitWidth、bit 0/最上位/範囲外、mask 0/full/超過、rotate 0/幅以上。
- Do not implement: signed integral 対応、bitset wrapper、BCD固有API。

### numeric Module

- Purpose: alignment、rounding、overflow、narrowing cast の小さい正解を提供。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  overflow と範囲外を戻り値で固定し、手書き算術の未定義動作を避けられる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/numeric/ket_numeric.h`、`modules/numeric/ket_numeric_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::numeric`）:
  - `template <typename To, typename From> constexpr bool InRange(From value) noexcept`
  - `template <typename T> constexpr T Clamp(T value, T min_value, T max_value) noexcept`
  - `template <typename T> constexpr typename std::make_unsigned<T>::type AbsDiff(T a, T b) noexcept`
  - `template <typename T> bool TryDivideRoundUp(T value, T divisor, T& out) noexcept`
  - `template <typename T> bool TryAlignUp(T value, T alignment, T& out) noexcept`
  - `template <typename T> bool TryAlignDown(T value, T alignment, T& out) noexcept`
  - `template <typename T> bool TryAdd(T a, T b, T& out) noexcept`
  - `template <typename T> bool TrySub(T a, T b, T& out) noexcept`
  - `template <typename T> bool TryMul(T a, T b, T& out) noexcept`
  - `template <typename T> constexpr T SaturatingAdd(T a, T b) noexcept`
  - `template <typename T> constexpr T SaturatingSub(T a, T b) noexcept`
  - `template <typename To, typename From> bool TryCast(From value, To& out) noexcept`
- Behavior: arithmetic API は integral 型を対象にし、`bool` と text character 型（`char`、`wchar_t`、
  `char16_t`、`char32_t`）は対象外。`signed char` と `unsigned char` は
  `std::int8_t` / `std::uint8_t` の実装aliasを壊さないため対象。対象外型は `static_assert` または SFINAE で
  compile error にする。alignment と divide-round-up は unsigned integral のみ。checked arithmetic は
  `std::numeric_limits<T>` による事前比較で成否を判定し、signed overflow を起こす式を評価しない。
  `AbsDiff` は signed 最小値を単純に符号反転せず、unsigned 変換と範囲比較で min/max 差を表す。
  `SaturatingAdd`/`SaturatingSub` は signed/unsigned それぞれの min/max 境界で飽和し、wraparound を外部仕様にしない。
  出力引数を変更する `TryXxx` は C++11 では `noexcept` のみ、C++14 以降で `constexpr` 化してよい。
  `InRange`/`Clamp`/`AbsDiff`/`SaturatingXxx` は C++11 でも `constexpr`。
- Failure/edge cases: `alignment == 0`、`divisor == 0`、overflow、cast範囲外は `false`。
  `Clamp` は `min_value <= max_value` を precondition。
- Complexity/performance: 全API定数時間。`constexpr` query はコンパイル時評価可。allocation・例外なし。
- Tests: align 0/1/exact/overflow、divide 0/1/exact、checked add/sub/mul の min/max、
  `std::numeric_limits<T>::min()` を含む `AbsDiff`、saturating の上下限、cast 255/256、signed境界、
  `bool`/text character型の不採用 compile-only、`std::int8_t`/`std::uint8_t` の採用 compile-only。
- Do not implement: arbitrary precision、numeric framework、C++17 optional convenience の初回追加。

### endian Module

- Purpose: byte order の読み書きを unaligned access や strict aliasing に頼らず安全に行う。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  endian と unaligned access の意図を名前に出し、strict aliasing 依存を避けられる。
  非推奨版 なし。非推奨理由: なし。
  標準代替: C++20 `std::endian` は byte order の判定であり、byte列の固定幅整数読み書きや
  失敗値付き Try API の直接代替ではない。
- Drop-in files: `modules/endian/ket_endian.h`、`modules/endian/ket_endian.cpp`、
  `modules/endian/ket_endian_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::endian`）:
  - `constexpr std::uint16_t ByteSwap16(std::uint16_t value) noexcept`
  - `constexpr std::uint32_t ByteSwap32(std::uint32_t value) noexcept`
  - `constexpr std::uint64_t ByteSwap64(std::uint64_t value) noexcept`
  - `std::uint16_t LoadBe16(const std::uint8_t* data) noexcept`
  - `std::uint32_t LoadBe32(const std::uint8_t* data) noexcept`
  - `std::uint64_t LoadBe64(const std::uint8_t* data) noexcept`
  - `std::uint16_t LoadLe16(const std::uint8_t* data) noexcept`
  - `std::uint32_t LoadLe32(const std::uint8_t* data) noexcept`
  - `std::uint64_t LoadLe64(const std::uint8_t* data) noexcept`
  - `void StoreBe16(std::uint8_t* data, std::uint16_t value) noexcept`
  - `void StoreBe32(std::uint8_t* data, std::uint32_t value) noexcept`
  - `void StoreBe64(std::uint8_t* data, std::uint64_t value) noexcept`
  - `void StoreLe16(std::uint8_t* data, std::uint16_t value) noexcept`
  - `void StoreLe32(std::uint8_t* data, std::uint32_t value) noexcept`
  - `void StoreLe64(std::uint8_t* data, std::uint64_t value) noexcept`
  - `bool TryLoadBe16(const std::uint8_t* data, std::size_t size, std::uint16_t& out) noexcept`
  - `bool TryLoadBe32(const std::uint8_t* data, std::size_t size, std::uint32_t& out) noexcept`
  - `bool TryLoadBe64(const std::uint8_t* data, std::size_t size, std::uint64_t& out) noexcept`
  - `bool TryLoadLe16(const std::uint8_t* data, std::size_t size, std::uint16_t& out) noexcept`
  - `bool TryLoadLe32(const std::uint8_t* data, std::size_t size, std::uint32_t& out) noexcept`
  - `bool TryLoadLe64(const std::uint8_t* data, std::size_t size, std::uint64_t& out) noexcept`
  - `bool TryStoreBe16(std::uint8_t* data, std::size_t size, std::uint16_t value) noexcept`
  - `bool TryStoreBe32(std::uint8_t* data, std::size_t size, std::uint32_t value) noexcept`
  - `bool TryStoreBe64(std::uint8_t* data, std::size_t size, std::uint64_t value) noexcept`
  - `bool TryStoreLe16(std::uint8_t* data, std::size_t size, std::uint16_t value) noexcept`
  - `bool TryStoreLe32(std::uint8_t* data, std::size_t size, std::uint32_t value) noexcept`
  - `bool TryStoreLe64(std::uint8_t* data, std::size_t size, std::uint64_t value) noexcept`
- Behavior: `LoadXxx`/`StoreXxx` は pointer が十分な長さの buffer を指すことを precondition。
  呼び出し境界で長さ確認が残る入力は `TryXxx` を優先し、plain Load/Store は固定長 protocol や
  直前検証で必要 byte 数を保証済みの内部経路向け。`TryXxx` は null、size不足を `false` で扱う。
  plain と `Try` は 16/32/64 すべてで対称に揃える。
  実装は byte 単位の shift/or で組み立て、`reinterpret_cast` と unaligned access をしない。
- Failure/edge cases: `TryLoadXxx` は失敗時に出力不変。`TryStoreXxx` は失敗時に buffer 不変。
- Complexity/performance: 各 Load/Store は語幅分の定数 byte のみ触る O(1)。`ByteSwap` は `constexpr` で
  コンパイル時評価可。allocation・例外なし。
- Tests: BE/LE 16/32/64、byteswap、null、size不足、unaligned address 相当。
- Do not implement: endian不明の `ReadU32`/`WriteU32`、`reinterpret_cast` による整数読み書き。

### hex Module

- Purpose: byte列、数値、診断用 hexdump を安全に文字列化する。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で失敗と入力範囲を明確に扱える。
  非推奨版 なし。非推奨理由: 標準ライブラリに byte列 hex parse/dump の直接APIなし。
- Drop-in files: `modules/hex/ket_hex.h`、`modules/hex/ket_hex.cpp`、
  `modules/hex/ket_hex_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::hex`）:
  - `enum class LetterCase { kLower, kUpper };`
  - `struct FormatOptions { LetterCase letter_case = LetterCase::kUpper; char separator = '\0'; };`
  - `std::string Format(std::uint64_t value, unsigned min_width = 0, LetterCase letter_case = LetterCase::kUpper)`
  - `std::string Encode(const std::uint8_t* data, std::size_t size, FormatOptions options = {})`
  - `std::optional<std::vector<std::uint8_t>> Decode(std::string_view text)`
  - `std::string Dump(const std::uint8_t* data, std::size_t size)`
  - `std::string DumpMemory(const void* data, std::size_t size)`
- Behavior: `Format` は整数を最小幅zero paddingつきhex文字列へ変換する。`Encode` は upper/lower と任意 separator を指定可能。`Decode` は hex digit と
  ASCII whitespace を受け付け、完全な偶数桁だけ変換する。`Dump` は offset、hex bytes、
  ASCII preview の固定形式。`DumpMemory` はC API境界の`void*` bufferをobject representationとしてdumpする明示API。
  `Dump` は 16 bytes/row、最小8桁 zero-pad lower hex offset、lower hex byte、
  8 byte ごとの追加 space、ASCII preview を `|...|` で囲む形式に固定する。ASCII preview は `0x20`〜`0x7e`
  をそのまま出し、それ以外は `.`。最終行の hex 欠落分は space padding で ASCII column を揃える。空入力は
  空文字列、非空入力は行を `\n` で区切り、末尾 newline は付けない。
- Golden output:

  ```txt
  Dump(empty) == ""
  Dump({0x00..0x0f}) ==
  "00000000  00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f  |................|"
  ```

- Failure/edge cases: pointer API の `nullptr` かつ `size == 0` は空文字列。`nullptr` かつ非0 size は
  precondition違反。`Decode` の奇数桁、不正文字は `std::nullopt`。
- Complexity/performance: byte列の `Encode`/`Dump` は入力長 O(n)で出力 string を 1回確保。
  `Decode` は O(n)で vector を 1回確保。整数の `Format` は桁数分の O(1)。
- Tests: 空入力、lower/upper、separator、ASCII whitespace、不正文字、奇数桁、dump空入力、dump 16 byte行、
  dump複数行、dump ASCII printable/non-printable、dump末尾 newline なし。
- Do not implement: Base64、binary viewer framework、任意 separator の parse 自動対応。

### parse Module

- Purpose: `std::from_chars` 周辺の境界条件を固定し、数値 parse を短く安全にする。Existing module
  のため、公開詳細は header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::from_chars` の完全消費、空白、overflow処理を小さいAPIで固定できる。
  非推奨版 なし。非推奨理由: `std::from_chars` は部品であり、ketの境界方針の直接代替ではない。
- Drop-in files: `modules/parse/ket_parse.h`、
  `modules/parse/ket_parse_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::parse`）:
  - `template <typename T> bool TryInt(std::string_view text, T& out) noexcept`
  - `template <typename T> bool TryUInt(std::string_view text, T& out) noexcept`
  - `template <typename T> bool TryHex(std::string_view text, T& out) noexcept`
  - `constexpr bool TryBool(std::string_view text, bool& out) noexcept`
  - `template <typename T> std::optional<T> Int(std::string_view text) noexcept`
  - `template <typename T> std::optional<T> UInt(std::string_view text) noexcept`
  - `template <typename T> std::optional<T> Hex(std::string_view text) noexcept`
  - `constexpr std::optional<bool> Bool(std::string_view text) noexcept`
  - `template <typename T> T IntOr(std::string_view text, T fallback) noexcept`
  - `template <typename T> T UIntOr(std::string_view text, T fallback) noexcept`
- Behavior: 成功条件は完全消費。整数 template の `T` は cv修飾なしで、bool と plain
  character型を除く integral 型。`TryInt` は signed integral、`TryUInt` は unsigned
  integral。`TryHex` は `0x`/`0X` prefix あり/なし両方を許可し、signed `T` でも符号文字は許可しない。
- Failure/edge cases: 空文字列、leading/trailing whitespace、部分消費、overflow、`"-1"` の
  unsigned parse、10進parseの先頭`+`、hex parseの符号文字は失敗。bool は `true`、`false`、`1`、`0`
  のみで case-sensitive。
- Complexity/performance: 入力長 O(n)。`std::from_chars` ベースで allocation・例外なし、`noexcept`。
- Tests: 0、最大値、型幅境界、overflow、空、`" 1"`、`"1 "`、`"1x"`、先頭不正文字、hex
  prefix、hex符号拒否、bool全候補。
- Do not implement: locale対応、trim込みparse、`yes/no/on/off`、浮動小数点 parse。

### enums Module

- Purpose: enum型と文字列・整数・flags の変換を table-based で明示する。flags操作は
  unsigned underlying typeのbit mask enumを対象にする。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と class template argument deduction を使い、table-based変換を短く書ける。
  非推奨版 なし。非推奨理由: 標準ライブラリに enum reflection や文字列変換の直接APIなし。
- Drop-in files: `modules/enums/ket_enums.h`、
  `modules/enums/ket_enums_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::enums`）:
  - `template <typename E> constexpr std::underlying_type_t<E> ToUnderlying(E value) noexcept`
  - `template <typename E> struct Entry { E value; std::string_view name; };`
  - `template <typename E> Entry(E value, std::string_view name) -> Entry<E>;`
  - `template <typename E, std::size_t N> std::optional<std::string_view> Name(E value, const Entry<E> (&table)[N]) noexcept`
  - `template <typename E, std::size_t N> std::string_view NameOr(E value, const Entry<E> (&table)[N], std::string_view fallback) noexcept`
  - `template <typename E, std::size_t N> std::optional<E> Parse(std::string_view text, const Entry<E> (&table)[N]) noexcept`
  - `template <typename E, std::size_t N> bool IsValid(E value, const Entry<E> (&table)[N]) noexcept`
  - `template <typename E> constexpr bool HasFlag(E flags, E flag) noexcept`
  - `template <typename E> constexpr E SetFlag(E flags, E flag) noexcept`
  - `template <typename E> constexpr E ClearFlag(E flags, E flag) noexcept`
  - `template <typename E> constexpr bool HasAnyFlag(E flags, E mask) noexcept`
  - `template <typename E> constexpr bool HasAllFlags(E flags, E mask) noexcept`
- Behavior: table は利用者が明示する。name と parse は完全一致。重複 entry は先に出たものを返す。
  C++17 では optional を返す `Name`/`Parse` と fallback 版 `NameOr` で足り、`TryXxx` の
  bool+out 版は持たない。CTAD 用の deduction guide を公開ヘッダに置き、`Entry{value, name}` の
  短い table 初期化を許可する。flag helper は unsigned underlying type だけを受け付け、
  `HasFlag`/`HasAllFlags` の 0 mask は true、`HasAnyFlag` の 0 mask は false とする。
- Failure/edge cases: unknown enum value、unknown text は失敗。flags は underlying type へ変換して
  bit operation する。signed underlying type のflag enumはcompile error。
- Complexity/performance: `Name`/`Parse`/`IsValid` は table 線形走査 O(N)。flags 操作と
  `ToUnderlying` は `constexpr` の O(1)。allocation なし。
- Tests: known/unknown value、known/unknown text、duplicate table、flags set/clear/has/all/any、
  `NameOr` の fallback。
- Do not implement: reflection、case-insensitive parse、enum registration framework、bool+out の
  `TryName`/`TryParse`（optional と `NameOr` で代替）。

### container Module

- Purpose: `find`、`end`、erase-remove、default値取得など標準コンテナ利用時の儀式を短くする。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  map/vector の反復的な lookup と erase-remove 手順を標準ライブラリのみで薄く包める。
  非推奨版 なし。非推奨理由: C++20以降の `contains` や `std::erase_if` と一部重なるが、
  module候補全体の直接代替ではない。
- Drop-in files: `modules/container/ket_container.h`、`modules/container/ket_container_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::container`）:
  - `template <typename Container, typename Value> bool Contains(const Container& container, const Value& value)`
  - `template <typename Map, typename Key> bool ContainsKey(const Map& map, const Key& key)`
  - `template <typename Map, typename Key> typename Map::mapped_type* AtOrNull(Map& map, const Key& key)`
  - `template <typename Map, typename Key> const typename Map::mapped_type* AtOrNull(const Map& map, const Key& key)`
  - `template <typename Map, typename Key> typename Map::mapped_type AtOr(const Map& map, const Key& key, typename Map::mapped_type default_value)`
  - `template <typename Map, typename Key, typename Factory> typename Map::mapped_type& AtOrCreate(Map& map, const Key& key, Factory factory)`
  - `template <typename Sequence, typename Predicate> std::size_t EraseIf(Sequence& sequence, Predicate predicate)`
  - `template <typename Vector> void SortUnique(Vector& values)`
- Behavior: `AtOrNull` は copy を避ける pointer API で const/非const の 2 overload。返却 pointer は
  map 内要素を指し、container の erase、rehash、破棄、値移動など標準コンテナ規則に従って invalidation される。
  `AtOr` は copy constructible な `mapped_type` 向けに値を返す。move-only 値は `AtOrNull` を使う。
  `AtOrCreate` は key が無い場合だけ key を materialize してから factory を呼ぶ。`EraseIf` は sequence 専用の
  erase-remove wrapper として削除件数を返す。全 API は `std::optional` を使わず C++11 で成立させる。
- Failure/edge cases: keyなし、空container、factory例外、重複値。`AtOrNull` は無い key で nullptr。
  `find`、hash、equality、less-than、factory、copy/move が投げる例外は呼び出し元へ伝播する。
  `EraseIf` の例外時状態は標準 erase-remove の規則に従う。`SortUnique` は `<` と `==` が同じ重複集合を表す型向け。
- Complexity/performance: `Contains` は sequence で O(n)、`ContainsKey`/`AtOrNull` は map 種別の lookup
  コスト。`AtOrCreate` の missing key は lookup 後に key materialize、factory、挿入を行う。`EraseIf` は O(n)、
  `SortUnique` は O(n log n)。
- Tests: keyあり/なし、factory呼び出し回数、EraseIf削除件数、SortUnique、AtOrNull null、例外伝播。
- Do not implement: 独自container、range framework、`std::erase_if`/`contains` 完全互換の追求、
  `IndexOf`（optional で C++11 baseline を崩す）、`Append`（self append が footgun で `insert` と等価）。

### ascii Module

- Purpose: ASCII 前提の小さい文字列処理を、Unicode と誤解されない名前で提供する。Existing module のため、
  公開詳細は header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` で non-owning な文字列処理を短く扱える。
  非推奨版 なし。非推奨理由: なし。標準代替:
  C++20 `starts_with` / `ends_with` と一部重なるが、ASCII 限定の trim/case/split 方針を固定した直接APIなし。
- Drop-in files: `modules/ascii/ket_ascii.h`、
  `modules/ascii/ket_ascii.cpp`、`modules/ascii/ket_ascii_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::ascii`）:
  - `bool StartsWith(std::string_view text, std::string_view prefix) noexcept`
  - `bool EndsWith(std::string_view text, std::string_view suffix) noexcept`
  - `bool Contains(std::string_view text, std::string_view needle) noexcept`
  - `std::string_view Trim(std::string_view text) noexcept`
  - `std::string_view TrimLeft(std::string_view text) noexcept`
  - `std::string_view TrimRight(std::string_view text) noexcept`
  - `std::string_view StripPrefix(std::string_view text, std::string_view prefix) noexcept`
  - `std::string_view StripSuffix(std::string_view text, std::string_view suffix) noexcept`
  - `std::vector<std::string_view> SplitViews(std::string_view text, char delimiter)`
  - `std::vector<std::string> Split(std::string_view text, char delimiter)`
  - `std::string Join(const std::vector<std::string_view>& parts, std::string_view delimiter)`
  - `std::string ReplaceAll(std::string_view text, std::string_view from, std::string_view to)`
  - `std::string ToLower(std::string_view text)`
  - `std::string ToUpper(std::string_view text)`
  - `bool EqualsIgnoreCase(std::string_view a, std::string_view b) noexcept`
- Behavior: trim は ASCII whitespace のみ。`Trim*`、`Strip*`、`SplitViews` は元文字列を参照する
  non-owning view を返す。`SplitViews` は空要素を保持する。`StripPrefix`/`StripSuffix` は一致しなければ
  元 view を返す。`StartsWith`/`EndsWith`/`Contains` は標準関数に近いが、C++17 での不足補完と ASCII
  module 内の命名統一、lifetime 方針の集約を目的に採用する。Unicode、locale、正規化を扱う API ではない。
- Failure/edge cases: viewを返すAPIは一時 `std::string` からの呼び出しをcompile時に拒否する。
  `ReplaceAll(text, "", to)` は `std::invalid_argument`。UTF-8 は byte列として扱い、case 変換では
  ASCII case 変換対象外 byte を変更しない。`ReplaceAll` は byte列一致を置換する。
- Complexity/performance: 判定・trim・case 変換は入力長 O(n)。`SplitViews` は view を返し要素 string の複製を
  避けるが結果 vector は確保する。`Split`/`Join`/`ReplaceAll`/`ToLower`/`ToUpper` は string を
  確保する O(n)。
- Tests: empty、delimiterなし、leading/trailing delimiter、空要素、view位置、rvalue string拒否、ASCII
  case、UTF-8 byte保持、embedded NUL、ReplaceAll空from拒否、Join edge。
- Do not implement: Unicode normalization、locale、regex、巨大 string utility 集。

### scope Module

- Purpose: cleanup 漏れ、早期 return 時の復元漏れを防ぐ Existing module。
  公開API詳細は `modules/scope/ket_scope.h` のDoxygenを正本とする。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  RAII cleanup を小さい header-only API として持ち出せる。C++23 `std::scope_exit` は `Exit`
  の代替候補だが、`Restore` とC++11〜20向けdrop-in性は本moduleで補う。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/scope/ket_scope.h`、`modules/scope/ket_scope_test.cpp`、
  `modules/scope/ket_scope_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::scope`）:
  - `template <typename F> class Exit`:
    - `explicit Exit(F f) noexcept`
    - `Exit(Exit&& other) noexcept`
    - `Exit(const Exit&) = delete`
    - `Exit& operator=(const Exit&) = delete`
    - `Exit& operator=(Exit&&) = delete`
    - `~Exit() noexcept`
    - `void Dismiss() noexcept`
    - `bool Active() const noexcept`
  - `template <typename F> Exit<F> MakeExit(F f) noexcept`
  - `template <typename T> class Restore`:
    - `explicit Restore(T& target) noexcept(noexcept(T(std::declval<const T&>())))`
    - `Restore(Restore&& other) noexcept(noexcept(T(std::declval<const T&>())))`
    - `Restore(const Restore&) = delete`
    - `Restore& operator=(const Restore&) = delete`
    - `Restore& operator=(Restore&&) = delete`
    - `~Restore() noexcept`
    - `void Dismiss() noexcept`
    - `bool Active() const noexcept`
  - `template <typename T> Restore<T> MakeRestore(T& target) noexcept(noexcept(T(std::declval<const T&>())))`
- Behavior: `Exit` は move-only。move 後 source は inactive。`Dismiss()` 後は callback を呼ばない。
  `Restore` は構築時の値へ destructor で復元する。`Restore` の move は復元責務を移送し、source を
  inactive にする。
- Failure/edge cases: destructor から例外を外へ出さない。callback や復元代入が例外を投げた場合は
  `std::terminate`。`Exit` の callback 型 `F` は nothrow move constructible。callback の参照先と
  `Restore` の復元先 lifetime は呼び出し側責任。`MakeExit` / `MakeRestore` はC++17以降で
  `[[nodiscard]]` 相当。
- Complexity/performance: 生成・実行は callback 1回呼び出しの O(1)。callback `F` を value 保持し、追加
  allocation をしない。
- Tests: scope exit、dismiss、move、二重実行なし、restore、early return 相当、destructor 例外の
  terminate、C++11 compile-only。
- Do not implement: `Finally` alias、scope success/failure 分岐、defer macro。

### byte_reader Module

- Purpose: 固定 buffer からの逐次読み取りを、offset/remaining を明示しながら安全に行う。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  buffer lifetime と offset 更新条件を明示し、size不足時の状態不変を固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/byte_reader/ket_byte_reader.h`、
  `modules/byte_reader/ket_byte_reader.cpp`、`modules/byte_reader/ket_byte_reader_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::byte_reader`）:

  ```cpp
  class Reader {
  public:
      Reader(const std::uint8_t* data, std::size_t size) noexcept;
      Reader(const Reader&) noexcept = default;
      Reader& operator=(const Reader&) noexcept = default;
      Reader(Reader&&) noexcept = default;
      Reader& operator=(Reader&&) noexcept = default;

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
  ```

- Behavior: `data == nullptr && size == 0` は有効な空 reader。`data == nullptr && size > 0` は
  invalid reader。`Empty()` は valid reader が末尾に到達したときだけ true を返す。copy/move は non-owning
  pointer、size、offset をそのまま複製/移動する。成功時だけ offset を進める。`ReadBytes` は non-owning pointer を返す。
- Failure/edge cases: 失敗時は offset と出力不変。元 buffer lifetime は利用者責任。
- Complexity/performance: 各 Read/Skip は読み取る byte 数に比例する O(k)。reader は buffer を所有せず
  allocation なし。
- Tests: 空buffer、ぴったり読み切り、1 byte不足、失敗時offset不変、BE/LE値、invalid reader。
- Do not implement: endian module 依存、owning buffer、protocol parser。

### byte_writer Module

- Purpose: fixed buffer への逐次書き込みを overflow なしで行う。Existing module のため、公開詳細は
  header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  fixed buffer 書き込みのsize確認とoffset更新条件を小さいAPIで固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/byte_writer/ket_byte_writer.h`、
  `modules/byte_writer/ket_byte_writer.cpp`、`modules/byte_writer/ket_byte_writer_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::byte_writer`）:

  ```cpp
  class Writer {
  public:
      Writer(std::uint8_t* data, std::size_t size) noexcept;
      Writer(const Writer&) noexcept = default;
      Writer& operator=(const Writer&) noexcept = default;
      Writer(Writer&&) noexcept = default;
      Writer& operator=(Writer&&) noexcept = default;

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
  ```

- Behavior: `data == nullptr && size == 0` は有効な空 writer。`data == nullptr && size > 0` は
  invalid writer。copy/move は non-owning pointer、size、offset をそのまま複製/移動する。
  `Full()` は valid writer かつ offset が末尾の場合だけ true。成功時だけ offset と buffer を更新する。
- Failure/edge cases: 失敗時は offset と既存 buffer を変更しない。サイズ確認後に書き込む。
- Complexity/performance: 各 Write/Skip は書き込む byte 数に比例する O(k)。writer は buffer を所有せず
  allocation なし。
- Tests: 空buffer書き込み失敗、ぴったり書き切り、1 byte不足、失敗時offset/buffer不変、BE/LE値。
- Do not implement: endian module 依存、可変長 vector builder、stream writer。

### bytes Module

- Purpose: 可変長 payload を `std::vector<std::uint8_t>` へ読みやすく組み立てる。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::vector<std::uint8_t>` を所有するpayload構築を、標準ライブラリのみで薄く包める。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/bytes/ket_bytes.h`、
  `modules/bytes/ket_bytes_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::bytes`）:
  - `class Builder`:
    - `Builder() = default;`
    - `explicit Builder(std::size_t reserve_size);`
    - `Builder& AppendU8(std::uint8_t value);`
    - `Builder& AppendBe16(std::uint16_t value);`
    - `Builder& AppendBe32(std::uint32_t value);`
    - `Builder& AppendLe16(std::uint16_t value);`
    - `Builder& AppendLe32(std::uint32_t value);`
    - `Builder& Append(const std::uint8_t* data, std::size_t size);`
    - `Builder& AppendAscii(std::string_view text);`
    - `const std::vector<std::uint8_t>& Buffer() const noexcept;`
    - `std::vector<std::uint8_t> Build() &&;`
    - `void Clear() noexcept;`
  - `void AppendU8(std::vector<std::uint8_t>& dst, std::uint8_t value);`
  - `void AppendBe16(std::vector<std::uint8_t>& dst, std::uint16_t value);`
  - `void AppendBe32(std::vector<std::uint8_t>& dst, std::uint32_t value);`
  - `void AppendLe16(std::vector<std::uint8_t>& dst, std::uint16_t value);`
  - `void AppendLe32(std::vector<std::uint8_t>& dst, std::uint32_t value);`
  - `void Append(std::vector<std::uint8_t>& dst, const std::uint8_t* data, std::size_t size);`
- Behavior: fluent API は `*this` を返す。free function は既存 vector へ追加する。`AppendAscii` は
  ASCII byte列として扱う文字列片を追加し、encoding 変換はしない。`Buffer()` は構築途中の
  内部 vector への const 参照を返す。`Build() &&` は内部 vector を move して返す。
- Failure/edge cases: allocation があるため `noexcept` なし。固定幅 append（`AppendBe16/Be32/Le16/Le32`）は
  一時配列を単一 insert で追記し、allocation 失敗時は `dst` を変更しない strong exception guarantee。
  `Append(nullptr, 0)` は no-op。`Append(nullptr, size > 0)` は precondition 違反。raw `Append` と
  `AppendAscii` の入力は `dst`（builder では内部 buffer）と重ならない（self-append 未対応、重なる場合は未定義）。
  `AppendAscii` の入力は ASCII byte列であることを precondition とし、検査せず byte copy のみで
  UTF-8 validation や変換はしない。
- Complexity/performance: 各 append は追加 byte 数に比例し、vector 再確保は amortized O(1)。
  `reserve_size` で再確保を抑制。`Build() &&` は move で O(1)。
- Tests: U8/BE/LE append、reserve constructor、Clear、Buffer、Build move、null+0、ASCII append。
- Do not implement: serializer framework、field schema、checksum、protocol固有処理。

### date Module

- Purpose: 日付・時刻の小さい妥当性判定を calendar framework なしで提供する。
- C++ version: 最小要件 C++11。推奨版 C++11〜17。推奨理由:
  C++17以前では標準calendar APIがなく、Gregorian妥当性判定を小さく持ち出せる。
  非推奨版 C++20以降。非推奨理由: C++20 `std::chrono` calendar を使える環境では、
  日付表現と妥当性判定を標準型へ寄せられる。
- Drop-in files: `modules/date/ket_date.h`、`modules/date/ket_date_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::date`）:
  - `constexpr bool IsLeapYear(int year) noexcept`
  - `constexpr bool IsValidMonth(unsigned month) noexcept`
  - `bool TryGetDaysInMonth(int year, unsigned month, unsigned& out) noexcept`
  - `constexpr bool IsValidDate(int year, unsigned month, unsigned day) noexcept`
  - `constexpr bool IsValidTime(unsigned hour, unsigned minute, unsigned second) noexcept`
  - `constexpr bool IsValidDateTime(int year, unsigned month, unsigned day, unsigned hour, unsigned minute, unsigned second) noexcept`
  - `constexpr bool IsValidTimeWithMilliseconds(unsigned hour, unsigned minute, unsigned second, unsigned millisecond) noexcept`
- Behavior: Gregorian calendar、`year >= 1`。timezone と leap second は扱わない。`IsValidDate` は
  内部 constexpr helper で当月日数を求め、`TryGetDaysInMonth` に依存せず constexpr を保つ。`IsValidDateTime` は
  `IsValidDate` と `IsValidTime` の合成判定。`TryGetDaysInMonth` は out へ当月日数を書き、C++11 では
  mutating Try を constexpr 化できないため `noexcept` のみ。C++14 以降で `constexpr` 化してよい。
- Failure/edge cases: month 0/13、day 0、月末超過、`hour >= 24`、`minute >= 60`、`second >= 60`、`millisecond >= 1000`。
- Complexity/performance: 全関数 O(1) の算術判定。allocation・例外なし。
- Tests: 2000 leap、1900 not leap、2024-02-29、2023-02-29、date-time合成、month境界、hour 24、C++11/14 compile-only check。
- Do not implement: timezone、calendar conversion、date arithmetic、BCD date。

### deadline Module

- Purpose: `steady_clock` ベースの timeout と elapsed time 計算を読みやすくする。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  `steady_clock` と timeout の扱いを明示し、`system_clock` 混在を避けられる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/deadline/ket_deadline.h`、`modules/deadline/ket_deadline.cpp`、
  `modules/deadline/ket_deadline_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::deadline`）:
  - `class Stopwatch`:
    - `static Stopwatch StartNew() noexcept;`
    - `void Restart() noexcept;`
    - `std::chrono::steady_clock::duration Elapsed() const noexcept;`
    - `std::chrono::milliseconds ElapsedMilliseconds() const noexcept;`
  - `class Deadline`:
    - `static Deadline After(std::chrono::steady_clock::duration timeout) noexcept;`
    - `static Deadline At(std::chrono::steady_clock::time_point time_point) noexcept;`
    - `bool Expired() const noexcept;`
    - `std::chrono::steady_clock::duration Remaining() const noexcept;`
    - `std::chrono::steady_clock::time_point TimePoint() const noexcept;`
- Behavior: `steady_clock` のみ。`Remaining()` は期限切れなら zero を返す。負 timeout は即 expired。
  表現上限を超える正 timeout は `steady_clock::time_point::max()` に飽和。
- Failure/edge cases: clock差し替えは初回なし。sleep依存テストは許容誤差を持たせる。
- Complexity/performance: 全 query は clock 1回参照の O(1)。allocation・例外なし。
- Tests: zero timeout、future deadline、Remaining非負、Restart、At。
- Do not implement: `system_clock` deadline、scheduler、timer thread。

### cli Module

- Purpose: 小さい社内 CLI で `argc/argv` の option 取得を短くする。Existing module のため、公開詳細は
  header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` で `argv` lifetime に依存する値を明示し、
  `std::optional` で option値の不在を小さく扱える。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/cli/ket_cli.h`、`modules/cli/ket_cli_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::cli`）:

  ```cpp
  class ArgvView {
  public:
      ArgvView(int argc, const char* const* argv) noexcept;
      ArgvView(const ArgvView&) noexcept = default;
      ArgvView& operator=(const ArgvView&) noexcept = default;
      ArgvView(ArgvView&&) noexcept = default;
      ArgvView& operator=(ArgvView&&) noexcept = default;

      std::size_t Size() const noexcept;
      std::string_view AtOrEmpty(std::size_t index) const noexcept;
      std::string_view ProgramNameOrEmpty() const noexcept;
  };
  ```

  - `bool HasOption(ArgvView args, std::string_view name) noexcept;`
  - `std::optional<std::string_view> OptionValue(ArgvView args, std::string_view name) noexcept;`
  - `std::string_view OptionValueOr(ArgvView args, std::string_view name, std::string_view fallback) noexcept;`
  - `std::vector<std::string_view> PositionalArguments(ArgvView args);`

- Behavior: `--key value` と `--key=value` を両方許す。`--flag` は `HasOption`。
  bare `--` はoption終端で、option検索はそこで止まり、以降のargumentは`--`で始まってもpositional候補。
  duplicated option は最初の出現を採用し、最初の出現に値がない場合は後続の値付き出現を見ない。戻り値の
  `std::string_view` は `argv` または fallback lifetime に依存する。`argc < 0` は 0 として扱う。
  `argc > 0 && argv == nullptr` は空 view として扱う。`argv[i] == nullptr` は空文字列の argument として扱う。
  `AtOrEmpty()` 範囲外と `ProgramNameOrEmpty()` 不在は空 `std::string_view` を返す。
- Failure/edge cases: option名は `"--"` で始まり、名前部分が非空で、`=`を含まない文字列のみ有効。
  `OptionValue(args, "--id")` で次要素が別 optionまたはbare `--`なら失敗。inline/separate値の空文字列は
  有効値。single dashで始まる次要素は値として扱う。`PositionalArguments` はschemaを受け取らず、
  separate option値をpositional候補として保持する。shell quote 展開なし。
- Complexity/performance: `HasOption`/`OptionValue`/`PositionalArguments` は argv を O(argc) 走査。`PositionalArguments` は
  結果 vector の上限容量を確保。値の `std::string_view` は argv を参照し追加 allocation なし。
- Tests: `--help`、`--id 123`、`--id=123`、missing value、empty value、duplicate、program name除外、
  option terminator、option名境界、schemaなしpositional。
- Do not implement: subcommand framework、help generator、shell parser、config file、
  `OptionUInt`（数値 parse は parse module の責務）。

### byte_view Module

- Purpose: C++11〜17 向けの薄い non-owning byte span。
- C++ version: 最小要件 C++11。推奨版 C++11〜17。推奨理由:
  C++17以前で non-owning byte span の寿命と境界確認を小さく表現できる。
  非推奨版 C++20以降。非推奨理由: C++20以降は `std::span` を優先できる。
- Drop-in files: `modules/byte_view/ket_byte_view.h`、`modules/byte_view/ket_byte_view_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::byte_view`）:
  - `class View`:
    - `constexpr View() noexcept;`
    - `constexpr View(const std::uint8_t* data, std::size_t size) noexcept;`
    - `View(const View& other) noexcept = default;`
    - `View& operator=(const View& other) noexcept = default;`
    - `View(View&& other) noexcept = default;`
    - `View& operator=(View&& other) noexcept = default;`
    - `constexpr const std::uint8_t* Data() const noexcept;`
    - `constexpr std::size_t Size() const noexcept;`
    - `constexpr bool Empty() const noexcept;`
    - `bool TryAt(std::size_t index, std::uint8_t& out) const noexcept;`
    - `bool TrySlice(std::size_t offset, std::size_t count, View& out) const noexcept;`
  - `class MutableView`:
    - `constexpr MutableView() noexcept;`
    - `constexpr MutableView(std::uint8_t* data, std::size_t size) noexcept;`
    - `MutableView(const MutableView& other) noexcept = default;`
    - `MutableView& operator=(const MutableView& other) noexcept = default;`
    - `MutableView(MutableView&& other) noexcept = default;`
    - `MutableView& operator=(MutableView&& other) noexcept = default;`
    - `constexpr std::uint8_t* Data() const noexcept;`
    - `constexpr std::size_t Size() const noexcept;`
    - `constexpr bool Empty() const noexcept;`
    - `bool TryAt(std::size_t index, std::uint8_t& out) const noexcept;`
    - `bool TrySet(std::size_t index, std::uint8_t value) noexcept;`
    - `bool TrySlice(std::size_t offset, std::size_t count, MutableView& out) const noexcept;`
- Behavior: view は所有権を持たず、元 buffer lifetime に依存する。`TryAt`/`TrySlice` は範囲内のみ
  成功し out へ書き込む。copy/move は non-owning pointer と size を複製し、元 buffer を変更しない。
  `MutableView::TrySet` は範囲内のみ書き込む。
- Failure/edge cases: `nullptr+0` は空 view。`nullptr+非0` は invalid view とし、access は失敗。
  slice 範囲超過は失敗で out 不変。
- Complexity/performance: 全 method は O(1)。view は pointer と size のみ保持し allocation なし。
- Tests: default、empty、invalid、bounds、slice、mutable set。
- Do not implement: `std::span` 置き換え、iterator 完備、owning bytes。

### utf8 Module

- Purpose: UTF-8 検査を小さく隔離する。Existing module のため、公開詳細は header
  Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view`でbyte列を非所有参照し、`std::optional`でcode point数取得の失敗を
  小さく表せる。
  非推奨版 なし。非推奨理由: 標準ライブラリに UTF-8 byte列検証の直接APIなし。
- Drop-in files: `modules/utf8/ket_utf8.h`、`modules/utf8/ket_utf8.cpp`、
  `modules/utf8/ket_utf8_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::utf8`）:
  - `struct ValidationResult { bool valid = false; std::size_t error_offset = 0; };`
  - `bool IsAscii(std::string_view text) noexcept;`
  - `ValidationResult Validate(std::string_view text) noexcept;`
  - `bool IsValid(std::string_view text) noexcept;`
  - `std::optional<std::size_t> CountCodePoints(std::string_view text) noexcept;`
- Behavior: `Validate` は valid 時に `error_offset == 0` を返す。invalid 時は、存在する
  byte がUTF-8構文または範囲制約に最初に反した位置を `error_offset` に入れる。妥当な
  prefix のまま EOF に達した truncated sequence は sequence 先頭を返す。`CountCodePoints` は code
  point 数を返し、invalid UTF-8 は `std::nullopt`。`IsValid` は `Validate(text).valid` と同義。
- Failure/edge cases: overlong、surrogate、truncated sequence、continuation byte 不正、範囲外 code
  point は invalid。空文字列は valid で code point 数 0。
- Complexity/performance: 全関数 1パス O(n)。allocation なし。
- Tests: ASCII境界、2/3/4 byte、空、overlong、truncated、short malformed、surrogate、bad
  continuation、範囲外 code point。
- Do not implement: normalization、case folding、locale、encoding conversion、grapheme cluster。

### file Module

- Purpose: ファイル全読み/全書きと基本的なpath queryの小さい定型処理を安全にまとめる。
  Existing module のため、公開詳細は header Doxygen と実装を正本とし、このカードは採用境界を記録する。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::filesystem` と `std::string_view` を使い、path と bytes/text の扱いを標準型へ寄せられる。
  非推奨版 なし。非推奨理由: なし。標準代替:
  標準ライブラリだけでは全読み/全書きの失敗方針が長くなりやすい。
- Drop-in files: `modules/file/ket_file.h`、`modules/file/ket_file.cpp`、
  `modules/file/ket_file_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::file`）:
  - `bool TryReadAllText(const std::filesystem::path& path, std::string& out, std::error_code* error = nullptr);`
  - `bool TryReadAllBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& out, std::error_code* error = nullptr);`
  - `bool TryWriteAllText(const std::filesystem::path& path, std::string_view text, std::error_code* error = nullptr);`
  - `bool TryWriteAllBytes(const std::filesystem::path& path, const std::uint8_t* data, std::size_t size, std::error_code* error = nullptr);`
  - `bool Exists(const std::filesystem::path& path) noexcept;`
  - `bool IsDirectory(const std::filesystem::path& path) noexcept;`
  - `std::optional<std::uintmax_t> Size(const std::filesystem::path& path) noexcept;`
- Behavior: 非optional 出力は `out` 参照で受け、`std::error_code*` だけは optional detail のため
  pointer（`nullptr` で error 詳細を無視）。text encoding は変換せず bytes をそのまま `std::string`
  に読む。読み書きは通常ファイルを対象にし、書き込みは既存内容をtruncateして置換する。未存在pathは作成する。
  `Exists` と `IsDirectory` は通常ファイル限定ではなく path query。`Size` は失敗詳細を捨てて
  `std::nullopt` を返す。
- Failure/edge cases: not found、permission、directory path、既存pathが通常ファイル以外、invalid path、
  短い read/write、読み込み中の内容増加、巨大 file、`TryWriteAllBytes(nullptr, nonzero)`、`error == nullptr`。
  stream open/write/close
  由来の失敗は標準ライブラリから詳細を取得できない場合にgenericな `io_error`。writeはatomicではなく、
  fileを開いた後の失敗では既存内容がtruncate済みまたは途中まで置換済みの場合がある。symlink処理は
  `std::filesystem` と stream の通常挙動に従う。
- Complexity/performance: 読み込みは file size O(n) で 1回 allocation。書き込みは入力 size O(n) で
  入力bufferから直接 stream へ書く。`Exists`/`IsDirectory`/`Size` は filesystem query のみ。
- Tests: 空file、text、binary、missing file、directory、環境が作成できる場合の通常ファイル以外、
  invalid path、permission denied（環境が強制できる場合）、既存fileのtruncate、NUL入りtext、
  空text/bytes、nullptr+nonzero、error_code clear/set。短い read/write と読み込み中の内容増加は
  deterministicなfunctional test対象外とし、実装のstream状態検査とDoxygenで固定。
- Do not implement: recursive copy、watcher、encoding conversion、path normalization framework。

### io_stream Module

- Purpose: stream の確実な読み書きと stream 書式状態(flags、precision、fill)保存を小さく提供する。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  iostream の short read/write と書式状態復元を小さいAPIで固定できる。行読みは C++11 で成立する
  `TryReadLineTrimRightAscii(out&)` に固定する。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/io_stream/ket_io_stream.h`、`modules/io_stream/ket_io_stream.cpp`、
  `modules/io_stream/ket_io_stream_test.cpp`、`modules/io_stream/ket_io_stream_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::io_stream`）:
  - `bool TryReadExactly(std::istream& stream, std::uint8_t* data, std::size_t size);`
  - `bool TryWriteAll(std::ostream& stream, const std::uint8_t* data, std::size_t size);`
  - `bool TryReadLineTrimRightAscii(std::istream& stream, std::string& out);`

  ```cpp
  class FormatStateSaver {
  public:
      explicit FormatStateSaver(std::ios& stream);
      ~FormatStateSaver() noexcept;
      FormatStateSaver(const FormatStateSaver& other) = delete;
      FormatStateSaver& operator=(const FormatStateSaver& other) = delete;
      FormatStateSaver(FormatStateSaver&& other) = delete;
      FormatStateSaver& operator=(FormatStateSaver&& other) = delete;
  };
  ```

- Behavior: `TryReadExactly` は requested size を読み切った場合のみ `true`。`TryWriteAll` は requested
  size の書き込みが標準ostreamで成功した場合のみ `true`。stream I/Oは一般的な `TryXxx` の
  状態不変規則の例外で、short read/writeやstream errorではstream位置、出力buffer、書き込み先が
  標準iostreamの結果に従って途中まで変化する場合がある。行読みは末尾の ASCII whitespace だけを
  除去し、成功時だけ `out` を更新する。C++17 convenience は初回 API に含めない。`FormatStateSaver` は
  flags、precision、fill を保存・復元する。error state、exception mask、locale、tie、widthは
  保存対象外。copy/move はともに禁止し、二重復元や復元対象 stream の差し替えを許可しない。
- Failure/edge cases: short read/write、stream error は `false` を返し、stream位置、出力buffer、
  書き込み先は標準iostreamの結果に従って途中まで変化する場合がある。stream exception は呼び出し側へ伝播。
  `data == nullptr && size > 0` はstreamへアクセスせず`false`。EOF で1文字も読めない場合は
  行読み失敗で `out` 不変。空行を読めた場合と、改行なしでEOFへ到達した最終行は成功。
- Complexity/performance: read/write は size O(n)。行読みは行長 O(n)。`FormatStateSaver` は O(1)。
- Tests: exact read、short read/write、null buffer、stream error、exception propagation、format state復元、
  保存対象外state、right-trimmed line、EOF。
- Do not implement: async I/O、filesystem wrapper、binary serialization。

### format Module

- Purpose: 診断用の短い固定表記API。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  診断用の固定表記を標準ライブラリのみで小さく提供できる。
  非推奨版 なし。非推奨理由: `std::format` は書式化部品であり、ketの固定診断表記の直接代替ではない。
- Drop-in files: `modules/format/ket_format.h`、`modules/format/ket_format.cpp`、
  `modules/format/ket_format_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::format`）:
  - `std::string Bool(bool value);`
  - `std::string Binary(std::uint64_t value, unsigned min_width = 0);`
  - `std::string ByteCount(std::uint64_t bytes);`
  - `std::string Duration(std::chrono::nanoseconds duration);`
- Behavior: `Bool` は `"true"`/`"false"`。`Binary` は2進表記で `min_width` まで `0` 詰め。
  `ByteCount` は IEC 1024 基準（B、KiB、MiB、GiB、TiB）で小数1桁に丸める。`Duration` は
  ASCII 表記の ns/us/ms/s/min/h から自動で単位を選ぶ。
- Golden output:

  | API         | Input                      | Output       | Rule                                    |
  | ----------- | -------------------------- | ------------ | --------------------------------------- |
  | `Binary`    | `0, 0`                     | `"0"`        | 0 は1桁。                               |
  | `Binary`    | `5, 8`                     | `"00000101"` | `min_width` まで左0詰め。               |
  | `Binary`    | `0xff, 4`                  | `"11111111"` | 桁数が `min_width` を超えても切らない。 |
  | `ByteCount` | `0`                        | `"0 B"`      | B は小数なし。                          |
  | `ByteCount` | `1024`                     | `"1.0 KiB"`  | IEC 1024、KiB 以上は小数1桁。           |
  | `ByteCount` | `1536`                     | `"1.5 KiB"`  | 小数1桁へ四捨五入。                     |
  | `Duration`  | `999ns`                    | `"999 ns"`   | ns は整数。                             |
  | `Duration`  | `1500ns`                   | `"1.5 us"`   | microseconds は ASCII の `us`。         |
  | `Duration`  | `-1500ns`                  | `"-1.5 us"`  | 負値は `-` を付け絶対値で単位選択。     |
  | `Duration`  | `std::chrono::minutes(90)` | `"1.5 h"`    | 60分以上は h。                          |

- Failure/edge cases: allocation 例外あり。負 duration は符号付きで表記。小数1桁の丸めは
  `std::round(value * 10.0) / 10.0` 相当。極大値でも選択単位の有限値として表記し、overflow する中間整数計算をしない。
- Complexity/performance: 各関数は出力長 O(k) の string を1回確保する。loop なしの定数規模処理。
- Tests: golden output table の全例、単位境界、min_width、負duration、0、最大値。
- Do not implement: `std::format` 再実装、logging framework、locale対応、数値hex表記（hex module）、
  `ToString`/`FormatSignedDecimal`/`FormatUnsignedDecimal`（`std::to_string` で十分）。

### ranges Module

- Purpose: range 要素を index 付きで扱う小さい補助。Existing module のため、公開詳細は header
  Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  index 付き range 走査を小さく書ける。非推奨版 なし。非推奨理由: なし。
  標準代替: C++20 ranges algorithmで一部用途を置き換え可能。ただしindex付き走査の直接代替ではない。
- Drop-in files: `modules/ranges/ket_ranges.h`、
  `modules/ranges/ket_ranges_cxx11_check.cpp`、
  `modules/ranges/ket_ranges_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::ranges`）:
  - `template <typename Range, typename F> void ForEachWithIndex(Range&& range, F&& f);`
  - `template <typename Range, typename Predicate> bool FindIndexIf(Range&& range, Predicate&& predicate, std::size_t& out);`
- Behavior: range は `begin`/`end` が使える object。ADL による free function も対象。
  iterator pair を受ける overload は初回なし。
  `ForEachWithIndex` は 0 から始まる index 昇順で `f(index, element)` を呼ぶ。`element` は `*it` の参照性と const 性を
  保持する。`FindIndexIf` は最初に `predicate(element)` が true になる index を `out` に書き、`true` を返す。
- Failure/edge cases: 空rangeの `ForEachWithIndex` は no-op。`FindIndexIf` は not found で `false`、`out` 不変。
  callable 例外は伝播する。`predicate` の副作用を除き、module側は成功時まで `out` に書き込まない。
  `std::algorithm` と同じく、走査中に range を無効化する変更は利用者責任。走査する要素数は
  `std::size_t` で表現可能であること。
- Complexity/performance: 両 API とも range 長 O(n)。module内部の追加 allocation なし。callable/predicate
  の実行コストは除く。`FindIndexIf` は最初の一致で短絡する。
- Tests: empty、index順、index型、C array、initializer_list、const/non-const要素、not found
  out不変、最初の一致、callable/predicate例外伝播、C++11 compile-only。
- Do not implement: `AllOf`/`AnyOf`/`NoneOf`/`CountIf`/`FindIf` など `std::algorithm` の単なる別名、
  range framework、iterator pair overload。

### memory Module

- Purpose: alignment と object representation 読み取りを小さく扱う補助。Existing module のため、公開詳細は
  header Doxygen と実装を正本とし、この文書は採用範囲の要約を記録する。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  pointer alignment と object representation の意図を小さいAPIへ分離できる。`std::uintptr_t` を提供し、
  byte address alignment を整数下位bitで判定できる処理系を前提とする。
  非推奨版 なし。非推奨理由: secure zero と pointer alignment の直接代替は標準だけでは不足する。
- Drop-in files: `modules/memory/ket_memory.h`,
  `modules/memory/ket_memory_test.cpp`,
  `modules/memory/ket_memory_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::memory`）:
  - `bool IsAligned(const void* ptr, std::size_t alignment) noexcept;`
  - `bool TryAlignUp(const void* ptr, std::size_t alignment, const void*& out) noexcept;`
  - `bool TryAlignUp(void* ptr, std::size_t alignment, void*& out) noexcept;`
  - `template <typename T> bool TryAlignUp(T* ptr, std::size_t alignment, T*& out) noexcept;`
  - `bool TryAlignDown(const void* ptr, std::size_t alignment, const void*& out) noexcept;`
  - `bool TryAlignDown(void* ptr, std::size_t alignment, void*& out) noexcept;`
  - `template <typename T> bool TryAlignDown(T* ptr, std::size_t alignment, T*& out) noexcept;`
  - `void Zero(void* ptr, std::size_t size) noexcept;`
  - `void SecureZero(void* ptr, std::size_t size) noexcept;`
  - `template <typename T> const unsigned char* ObjectBytes(const T& object) noexcept;`
  - `template <typename T> constexpr std::size_t ObjectByteSize(const T& object) noexcept;`
- Behavior: pointer alignment と byte表現の読み取りに限定する。`alignment` は power-of-two 前提で、それ以外や
  `alignment == 0` は false。`TryAlignUp` / `TryAlignDown` の戻り値は address-level の結果であり、object
  bounds、lifetime、provenance、dereference可能性は保証しない。`SecureZero` は `volatile unsigned char*`
  経由で byte write し、最適化除去を防ぐ best-effort。暗号学的な完全消去保証はしない。`ObjectBytes` は
  object representation 読み取り用に `const unsigned char*` を返し、trivially copyable 型に限定する。padding、
  endian、layout を含む処理系依存の byte 列であり、serialization や安定比較には使わない。
- Failure/edge cases: `alignment == 0`、非power-of-two alignment、null pointer、切り上げoverflow、
  secure zero最適化除去対策。`Zero(nullptr, 0)` と `SecureZero(nullptr, 0)` は no-op、`nullptr` かつ非0 size
  は precondition 違反として `std::terminate`。
- Complexity/performance: alignment 判定は O(1)。zeroing は size に対し O(n)。`SecureZero` は最適化されない分
  `Zero` より遅い。
- Tests: aligned/unaligned、alignment 0、非power-of-two、切り上げoverflow、null+0、null+非0 terminate、
  partial zeroing、object byte pointer/size、C++11 compile-only。
- Do not implement: object lifetime 操作、placement new helper、allocator、type punning。

### pointer Module

- Purpose: null と ownership 誤解を減らす補助。Existing module のため、公開詳細は header Doxygen と実装を
  正本とし、このカードは採用境界を記録する。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  null許容性と所有権の有無を型名や関数名で明示できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/pointer/ket_pointer.h`,
  `modules/pointer/ket_pointer_test.cpp`,
  `modules/pointer/ket_pointer_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::pointer`）:
  - `template <typename T> class NotNull { public: constexpr explicit NotNull(T* ptr); template <typename U> constexpr operator NotNull<U>() const noexcept; constexpr T* Get() const noexcept; constexpr T& operator*() const noexcept; constexpr T* operator->() const noexcept; };`
  - `template <typename T> std::shared_ptr<T> LockWeak(const std::weak_ptr<T>& weak) noexcept;`
  - `template <typename T> T* AddressOf(T& value) noexcept;`
- Behavior: 主価値は raw pointer の null 不許可を型で表す `NotNull`。使用形は pointer 型そのものではなく
  pointee 型を渡す `NotNull<T>`。`NotNull` は所有権を持たない non-null wrapper で、構築時に null なら
  `std::invalid_argument` を投げる。参照先の lifetime は呼び出し側の責任で、`NotNull` は lifetime を延長せず追跡もしない。
  `NotNull<U>` は `U*` が `T*` へ暗黙変換できる場合だけ `NotNull<T>` へ変換できる。`void` は対象外。
  対象は raw pointer のみで、smart pointer は包まない。`LockWeak` は `weak.lock()` の結果取得を意図名で薄く包み、
  expired 時に空 `std::shared_ptr` を返す。成功時は返却 `shared_ptr` が共有所有権を持ち、参照先objectの
  lifetimeを延長する。`AddressOf` は C API 境界や debug 補助で overload された `operator&` を
  回避する意図を名前に出すために採用し、temporary objectのaddress取得は拒否する。
- Failure/edge cases: `NotNull(nullptr)` は `std::invalid_argument`、参照先lifetime不在は呼び出し側責任、
  `NotNull<void>` はclass instantiation時のhard error、weak expired/default constructed weak、
  overloaded `operator&`、`AddressOf` rvalue拒否。
- Complexity/performance: 全 API は O(1)。`LockWeak` は `weak_ptr::lock` 相当の atomic 操作。
- Tests: nullptr、dereference、operator->、const pointee、derived-to-base、base-to-derived拒否、
  weak alive/expired/default、AddressOf overloaded `operator&` のmutable/const、rvalue拒否、
  C++11 constexpr/noexcept/return type。
- Do not implement: `Owner<T*>`、smart pointer 再実装、lifetime tracking、
  `std::optional<std::reference_wrapper<T>>` を返す `GetOrNull`。

### testing Module

- Purpose: byte列比較の GoogleTest failure message を読みやすくする Existing module。
  公開API詳細は `modules/testing/ket_testing.h` のDoxygenと
  `modules/testing/ket_testing.cpp` の実装を正本とする。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  GoogleTest v1.17.0 のC++17要件に合わせ、byte列差分を読みやすく表示できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/testing/ket_testing.h`、
  `modules/testing/ket_testing.cpp`、
  `modules/testing/ket_testing_test.cpp`。
- Dependencies: GoogleTest と標準ライブラリ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::testing`）:
  - `::testing::AssertionResult BytesEqual(const std::uint8_t* expected, std::size_t expected_size, const std::uint8_t* actual, std::size_t actual_size);`
  - `::testing::AssertionResult HexEqual(std::string_view expected_hex, const std::uint8_t* actual, std::size_t actual_size);`
- Behavior: `::testing::AssertionResult` を返し、mismatch offset、expected/actual hex を表示する。
  `HexEqual` は ASCII whitespace を読み飛ばし、診断では正規化したhexを表示する。
- Failure/edge cases: null+0 は空として許可。null+非0 は failure。`HexEqual` の奇数hex桁、
  非hex文字、NUL は failure。
- Complexity/performance: 比較は O(n)。失敗時のみ診断 string を確保し、成功時は allocation を避ける。
- Tests: equal、success message empty、different size両方向、empty/non-empty、different byte、
  null+0、null+非0、invalid hex。
- Do not implement: production library dependency、matcher framework、snapshot framework。

### version Module

- Purpose: `major.minor.patch` の numeric version triplet parse/format/compare。
  公開API詳細は `modules/version/ket_version.h` のDoxygenを正本とする。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で parse失敗と比較を小さく固定できる。
  非推奨版 なし。非推奨理由: 標準ライブラリに numeric version triplet の直接APIなし。
- Drop-in files: `modules/version/ket_version.h`,
  `modules/version/ket_version.cpp`,
  `modules/version/ket_version_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::version`）:
  - `struct Triplet { std::uint32_t major = 0; std::uint32_t minor = 0; std::uint32_t patch = 0; };`
  - `std::optional<Triplet> Parse(std::string_view text) noexcept;`
  - `std::string Format(Triplet version);`
  - `int Compare(Triplet a, Triplet b) noexcept;`
- Behavior: `major.minor.patch` の numeric triplet のみ。各要素は `std::uint32_t`。`0` 自体は許可し、
  複数桁の leading zero は失敗。`Compare` は major→minor→patch 順で負/0/正を返す。
  型名は `ket::version::Triplet` とし、SemVer 2.0.0 の prerelease/build metadata、
  precedence rule、range syntax は扱わない。
- Failure/edge cases: 空、要素不足/過多、空要素、非数字、符号、空白、overflow、`01.2.3`、
  prerelease/build metadata。
- Complexity/performance: parse は入力長 O(n)。compare は O(1)、format は出力長 O(k) で 1回確保。
- Tests: `0.0.0`、normal parse、format、compare major/minor/patch、overflow、leading zero、metadata拒否。
- Do not implement: full SemVer、prerelease、build metadata、range constraint、package manager semantics、
  `Version` alias。

### ipv4 Module

- Purpose: IPv4 dotted decimal の parse/format と BE 32bit 変換。Existing module のため、公開詳細は
  header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で dotted decimal の失敗条件を明確に扱える。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/ipv4/ket_ipv4.h`、`modules/ipv4/ket_ipv4.cpp`、
  `modules/ipv4/ket_ipv4_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::ipv4`）:
  - `struct Address { std::uint8_t octets[4] = {0, 0, 0, 0}; };`
  - `std::optional<Address> Parse(std::string_view text) noexcept;`
  - `std::string Format(Address value);`
  - `constexpr std::uint32_t ToBe32(Address value) noexcept;`
  - `constexpr Address FromBe32(std::uint32_t value) noexcept;`
- Behavior: dotted decimal のみ。leading zero は失敗。`ToBe32` / `FromBe32` は `octets[0]` を
  上位8bitに置くbig-endian順の32bit数値表現を扱う。host/network byte order変換APIではない。
  型名は Google C++ Style に合わせ `ket::ipv4::Address`（proposal の `IpV4` ではない）を採用する。
- Failure/edge cases: octet > 255、負値、空要素、要素不足/過多、空白、制御文字、NUL、leading zero。
- Complexity/performance: parse/format は固定4 octet の O(1)。変換は O(1)、allocation は format のみ。
- Tests: `0.0.0.0`、`255.255.255.255`、format、BE32 golden値、constexpr roundtrip、不正入力。
- Do not implement: IPv6、CIDR、DNS、port、socket address。

### port Module

- Purpose: TCP/UDP port番号の小さい値型、parse、format。Existing module のため、公開詳細は
  header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で範囲外や不正文字を明確に扱える。
  非推奨版 なし。非推奨理由: 標準ライブラリに port番号の値型や parse/format の直接APIなし。
- Drop-in files: `modules/port/ket_port.h`、`modules/port/ket_port.cpp`、
  `modules/port/ket_port_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::port`）:
  - `struct Port { std::uint16_t value = 0; };`
  - `constexpr bool TryFromUInt(std::uintmax_t value, Port& out) noexcept;`
  - `std::optional<Port> Parse(std::string_view text) noexcept;`
  - `std::string Format(Port port);`
- Behavior: port値は0〜65535を許可する。0は OS による自動割り当て用途を表せる有効値とし、
  呼び出し側が必要なら別途禁止する。`TryFromUInt` は wide unsigned整数から範囲確認して `out` に詰める。
  `Parse` は10進表記のみを完全消費し、format は leading zero なしの10進文字列を返す。
- Failure/edge cases: 空文字列、leading/trailing whitespace、`+`/`-`、不正文字、65535超過、
  複数桁の leading zero は失敗。
- Complexity/performance: `TryFromUInt`/`Format` は O(1)、`Parse` は桁数 O(n)（≤5桁）。
- Tests: 0、1、65535、65536、`std::uintmax_t`最大値、空、空白、制御文字、NUL、
  符号、不正文字、leading zero、format。
- Do not implement: socket address、service name 解決、protocol別port型、ephemeral port 採番。

### mac Module

- Purpose: MAC address の parse/format。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で区切り文字とhex byteの失敗条件を固定できる。
  非推奨版 なし。非推奨理由: 標準ライブラリに MAC address parse/format の直接APIなし。
- Drop-in files: `modules/mac/ket_mac.h`,
  `modules/mac/ket_mac.cpp`, `modules/mac/ket_mac_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::mac`）:
  - `enum class LetterCase { kLower, kUpper };`
  - `struct Address { std::uint8_t bytes[6] = {0, 0, 0, 0, 0, 0}; };`
  - `std::optional<Address> Parse(std::string_view text) noexcept;`
  - `std::string Format(Address value, LetterCase letter_case = LetterCase::kLower);`
- Behavior: `AA:BB:CC:DD:EE:FF` と `aa-bb-cc-dd-ee-ff` を許可する。区切り文字の混在は失敗。
  `Format` は `letter_case` で大文字小文字を選び（既定は lower-case）、`:` 区切りで出力する。
- Failure/edge cases: byte数不足/過多、不正hex、区切り不足、混在区切り、Cisco形式は失敗。
- Complexity/performance: parse/format は固定6 byte の O(1)。allocation は format のみ。
- Tests: upper/lower、colon、hyphen、format lower/upper、不正hex、短い/長い入力。
- Do not implement: Cisco形式、OUI lookup、network device model。

### function Module

- Purpose: callable/visitor の小さい儀式を減らす。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  variadic using declaration で visitor overload set を小さく表現できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/function/ket_function.h`、`modules/function/ket_function_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::function`）:
  - `template <typename... Fs> struct Overload : Fs... { using Fs::operator()...; };`
  - `template <typename... Fs> Overload<Fs...> MakeOverload(Fs... fs);`
  - `struct Noop { template <typename... Args> void operator()(Args&&...) const noexcept; };`
- Behavior: `Overload` は複数 callable を値として保持し、`using Fs::operator()...` で overload set
  を作る。`Noop` は任意引数を無視し `noexcept`。
- Failure/edge cases: callable の copy/move 制約は型に従う。呼び出し例外は handler から伝播。
- Complexity/performance: `Overload` は callable を値合成するのみで O(1)、追加 allocation なし。
- Tests: `std::visit`、複数型、戻り値、Noop、copy/move。
- Do not implement: `FunctionRef`、`std::function` wrapper、signal/slot framework。

### variant Module

- Purpose: `std::variant` visitor 補助。Existing module のため、公開詳細は header Doxygen と実装を
  正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::variant` と visitor補助を標準ライブラリのみで薄く包める。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/variant/ket_variant.h`,
  `modules/variant/ket_variant_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::variant`）:
  - `template <typename Variant, typename... Handlers> constexpr decltype(auto) Match(Variant&& variant, Handlers&&... handlers);`
- Behavior: `Match` は内部の overload helper で handler 群を束ね `std::visit` に渡す。`function` module に
  依存せず、overload helper は本 module 内に閉じて持つ。reference category と const を保持し、戻り型は
  `std::visit` の規則に従う。lvalue handler は内部visitorへcopyし、rvalue handler はmoveする。
- Failure/edge cases: handler 例外は呼び出し元へ伝播。全 alternative を網羅しない handler 集合は
  compile error。valueless_by_exception は `std::visit` と同じく `std::bad_variant_access`。
- Complexity/performance: `std::visit` 相当の単一 dispatch。追加 allocation なし。
- Tests: const/non-const、lvalue/rvalue、handler copy/move、void/non-void、missing handler
  compile-time check、handler例外、valueless_by_exception。
- Do not implement: variant framework、pattern matching DSL、`function` module への依存、
  `std::holds_alternative` 別名の `Holds`、`std::get_if` 別名の `GetIf`。

### optional Module

- Purpose: `std::optional` の小さい合成。Existing module のため、公開詳細は header Doxygen と実装を正本とし、
  このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::optional` の小さい合成を、標準の値保持制約に従って扱える。
  非推奨版 API別。非推奨理由: APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない。
- Drop-in files: `modules/optional/ket_optional.h`,
  `modules/optional/ket_optional_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::optional`）:

  ```cpp
  template <typename T, typename F>
  constexpr auto Map(std::optional<T>& value, F&& f);

  template <typename T, typename F>
  constexpr auto Map(const std::optional<T>& value, F&& f);

  template <typename T, typename F>
  constexpr auto Map(std::optional<T>&& value, F&& f);

  template <typename T, typename F>
  constexpr auto Map(const std::optional<T>&& value, F&& f);

  template <typename T, typename F>
  constexpr auto AndThen(std::optional<T>& value, F&& f);

  template <typename T, typename F>
  constexpr auto AndThen(const std::optional<T>& value, F&& f);

  template <typename T, typename F>
  constexpr auto AndThen(std::optional<T>&& value, F&& f);

  template <typename T, typename F>
  constexpr auto AndThen(const std::optional<T>&& value, F&& f);

  template <typename T, typename F>
  constexpr T ValueOrEval(const std::optional<T>& value, F&& fallback_factory);

  template <typename T, typename F>
  constexpr T ValueOrEval(std::optional<T>&& value, F&& fallback_factory);
  ```

- Behavior: empty の場合は mapper/factory を呼ばない。mapper/factory は `std::invoke` で呼び、
  `F&&` として受ける。`Map` と `AndThen` は `std::optional<T>` の `&`、`const&`、`&&`、`const&&`
  value category に沿って保持値を渡す。`Map` の mapper 戻り型は non-void object type に限定し、
  戻り値は `std::decay_t` で保持する。参照を保持したい場合は mapper が `std::reference_wrapper<T>` を
  明示的に返す。`AndThen` の mapper は `std::optional<U>` を値として返すことを要求し、その optional
  値そのものを返す。`ValueOrEval` は empty のときだけ `fallback_factory` を評価する。
- Failure/edge cases: mapper 例外は伝播。`Map` の mapper が `void` または reference を直接返す場合は
  compile error。`ValueOrEval(const optional<T>&)` は `T` が copy constructible、rvalue overload は move
  constructible であることを要求する。`AndThen` の mapper が optional 以外、または optional 参照を返す場合は
  compile error。
- Complexity/performance: 各APIは1回の分岐と高々1回の mapper/factory 呼び出し。追加 allocation なし。
  単純な値変換は constexpr 評価可能。
- Tests: value/empty、mapper呼び出し回数、factory遅延、戻り型、rvalue move、reference_wrapper保持、
  mutable lvalue、std::invoke互換callable、rvalue empty、constexpr、mapper例外伝播、非optional mapper の
  compile-fail 相当確認。
- API別標準代替:
  - `Map`: C++23 `std::optional::transform`。
  - `AndThen`: C++23 `std::optional::and_then`。
  - `ValueOrEval`: 直接代替なし。factory遅延評価を固定する場合に採用。
- Do not implement: `Result`、`StatusOr`、monad framework、error accumulation、
  複数 optional 合成の `HasValueAll`。

### contract Module

- Purpose: precondition、postcondition、invariant を名前で明示する。Existing module のため、公開詳細は
  header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  契約違反時のプロジェクト方針を小さいAPIへ閉じ込められる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/contract/ket_contract.h`、`modules/contract/ket_contract_test.cpp`、
  `modules/contract/ket_contract_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::contract`、macro は global）:

  ```cpp
  enum class Kind { kExpects, kEnsures, kInvariant };

  [[noreturn]] void Fail(
      Kind kind,
      const char* expression,
      const char* file,
      int line) noexcept;

  void Expects(bool condition, const char* expression, const char* file, int line) noexcept;
  void Ensures(bool condition, const char* expression, const char* file, int line) noexcept;
  void AssertInvariant(bool condition, const char* expression, const char* file, int line) noexcept;

  template <typename T>
  T* RequireNonNull(T* ptr, const char* expression, const char* file, int line) noexcept;

  constexpr bool IsInBounds(std::size_t index, std::size_t size) noexcept;

  #define KET_EXPECTS(condition)
  #define KET_ENSURES(condition)
  #define KET_ASSERT_INVARIANT(condition)
  #define KET_REQUIRE_NON_NULL(ptr)
  ```

- Behavior: `KET_EXPECTS`、`KET_ENSURES`、`KET_ASSERT_INVARIANT` は対応する backend function へ
  condition、文字列化した式、`__FILE__`、`__LINE__` を渡す。statement として使う macro で、戻り値は持たない。
  `KET_REQUIRE_NON_NULL(ptr)` は ptr 式を1回だけ評価し、non-null なら同じ pointer 型の値を返す。
  違反時は `Fail` が `std::terminate` を呼び、戻らない。debug/release とも常時評価し、
  `NDEBUG` では消さない。`IsInBounds` は `index < size` のC++11 `constexpr`純粋判定。
- Failure/edge cases: 契約違反は例外ではなく process termination。差し替え可能 handler は持たない。
  macro は condition または ptr 式を1回だけ評価する。`expression`/`file` が null の場合でも terminate 方針は変えず、
  診断文字列生成に依存しない。
- Complexity/performance: 成功時は O(1) の分岐のみ。違反時は O(1) で terminate。allocation なし。
- Tests: valid path、`KET_EXPECTS`/`KET_ENSURES`/`KET_ASSERT_INVARIANT` death test、`KET_REQUIRE_NON_NULL` 成功/失敗、
  expression 1回評価、明示的bool変換、戻り値 pointer の型保持、`IsInBounds` 最大値境界とC++11
  `constexpr`、`NDEBUG` compile でも評価されること。
  death test は終了することを主に固定し、診断messageの完全一致には依存しない。
- Do not implement: release で消える契約、throw 方針、差し替え可能 handler、macro 大量追加、exception hierarchy、
  debug logging framework、`RequireInRange`。

### interop Module

- Purpose: C API 境界の errno、C buffer、handle cleanup 事故を減らす補助。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  errno保存、C buffer copy、handle cleanup の事故をC API境界に閉じ込められる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/interop/ket_interop.h`,
  `modules/interop/ket_interop_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::interop`）:
  - `class ErrnoGuard { public: ErrnoGuard() noexcept; ~ErrnoGuard() noexcept; int Saved() const noexcept; };`
  - `bool CopyStringToBuffer(char* dst, std::size_t dst_size, const char* src, std::size_t src_size) noexcept;`
  - `bool CopyBytesToBuffer(void* dst, std::size_t dst_size, const void* src, std::size_t src_size) noexcept;`

  ```cpp
  template <typename Handle, typename Deleter>
  class UniqueHandle {
  public:
      UniqueHandle() noexcept(std::is_nothrow_default_constructible<Deleter>::value);
      UniqueHandle(Handle handle, Deleter deleter) noexcept(std::is_nothrow_move_constructible<Deleter>::value);
      ~UniqueHandle() noexcept;
      UniqueHandle(const UniqueHandle&) = delete;
      UniqueHandle& operator=(const UniqueHandle&) = delete;
      UniqueHandle(UniqueHandle&& other) noexcept(std::is_nothrow_move_constructible<Deleter>::value);
      UniqueHandle& operator=(UniqueHandle&& other) noexcept;

      bool HasValue() const noexcept;
      Handle Get() const noexcept;
      Deleter& GetDeleter() noexcept;
      const Deleter& GetDeleter() const noexcept;
      Handle Release() noexcept;
      void Reset() noexcept;
      void Reset(Handle handle) noexcept;
  };
  ```

- Behavior: `ErrnoGuard` は構築時 errno を保存し destructor で復元する。`CopyStringToBuffer` は null終端を保証し、
  `src_size + 1 <= dst_size` のときだけ成功する。`src == nullptr && src_size == 0` は空文字列コピーとして扱い、
  `dst[0] = '\0'` を書ける場合だけ成功。`CopyBytesToBuffer` は `src == nullptr && src_size == 0` を no-op 成功、
  `dst == nullptr && dst_size == 0` を失敗として扱う。`UniqueHandle` は `Handle` と `Deleter` を値として保持し、
  sentinel値ではなく engaged flag で所有を表す。default constructor は default-constructed deleter と
  disengaged state を作り、`Deleter` が default constructible の場合だけ overload resolution に参加する。
  copy は禁止、move は所有権と deleter を移し、move元を disengaged にする。self-move assignment は no-op。
  engaged のときだけ destructor/`Reset()`/`Reset(handle)` が deleter を呼ぶ。
  `Release()` は engaged が precondition で、handle を返して disengaged にする。`Reset(handle)` は既存 handle
  を閉じてから新しい handle を engaged として保持する。
- Failure/edge cases: `dst_size == 0`、null pointer、src truncation、deleter noexcept、`Release` 後は非所有。
  `CopyStringToBuffer` と `CopyBytesToBuffer` は失敗時に `dst` を変更しない。deleter が例外を投げた場合は
  `std::terminate`。`UniqueHandle()` は `Deleter` が default constructible の場合だけ使用できる。
- Complexity/performance: copy は `src_size` に対し O(n) memcpy。`ErrnoGuard`/`UniqueHandle` は O(1)、
  allocation なし。
- Tests: errno復元、copy成功/不足、失敗時 buffer 不変、bytes copy、UniqueHandle default constructor 制約、
  reset/release/move/self-move、deleter例外時 terminate。
- Do not implement: OS handle 専用 wrapper、C API framework、ownership annotation体系。

### platform Module

- Purpose: errno/Windows error/environment variable の小さい platform 補助。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  platform API の差分を隠しすぎず、標準文字列で結果を扱える。
  非推奨版 なし。非推奨理由: 標準ライブラリだけでは errno/Windows error message の扱いが不十分。
- Drop-in files: `modules/platform/ket_platform.h`,
  `modules/platform/ket_platform.cpp`、
  `modules/platform/ket_platform_test.cpp`。
- Dependencies: 標準ライブラリと platform API。他の ket module への依存なし。
- Public API Signatures（`namespace ket::platform`）:
  - `std::string FormatErrno(int error_number);`
  - `std::optional<std::string> GetEnvironmentVariable(std::string_view name);`

  ```cpp
  #ifdef _WIN32
  using WindowsErrorCode = unsigned long;
  WindowsErrorCode GetLastErrorCode() noexcept;
  std::string FormatWindowsError(WindowsErrorCode code);
  #endif
  ```

- Behavior: `FormatErrno` は errno 番号を platform の thread-safe API で文字列化し、結果を
  `std::string` にコピーして返す。message 取得に失敗した場合も空文字列を返さず、`"Unknown error <n>"` 形式の
  ASCII fallback を返す。`GetEnvironmentVariable` は process environment から値を取得し、存在しなければ
  `std::nullopt`。name は非空かつ NUL を含まないことを要求し、違反時は `std::nullopt`。Windows では
  environment と error message の wide API を使い、戻り値は UTF-8 narrow string。Windows 専用 API は
  `#ifdef _WIN32` で宣言ごと隠す。POSIX では利用可能な thread-safe strerror 系 API を使い、GNU/POSIX の
  signature 差は `.cpp` 内 helper に閉じ込め、公開APIの戻り値や例外方針に差を出さない。
- Failure/edge cases: unknown errno、missing env、empty name、NUL を含む name、Windows wide/narrow 変換失敗。
  environment を他 thread が同時変更する場合の一貫性は platform API の規約に従う。Windows UTF-8 変換に失敗した
  場合は空文字列ではなく ASCII fallback を返す。
- Complexity/performance: error message と environment value の長さ O(n) で string を1回確保する。
  Windows UTF-8 変換も出力長 O(n)。
- Tests: known errno が非空、unknown errno fallback、missing env、present env、empty name、NUL name、
  non-Windows で Windows API が宣言されない conditional compile、POSIX/GNU strerror差の compile、
  Windows 環境では known `GetLastError` message。environment test は一意な `KET_` prefix の変数名を使い、
  test 前の値を保存して終了時に復元する。
- Do not implement: cross-platform error framework、logging、localization、error category wrapper、environment 設定/削除。

### state Module

- Purpose: 小さい状態遷移表 lookup。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::optional` で未定義遷移を明確にし、table-driven lookup を小さく書ける。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/state/ket_state.h`,
  `modules/state/ket_state_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::state`）:
  - `template <typename State, typename Event> struct Transition { State from; Event event; State to; };`
  - `template <typename State, typename Event, std::size_t N> constexpr bool IsAllowed(const State& current, const Event& event, const Transition<State, Event> (&table)[N]);`
  - `template <typename State, typename Event, std::size_t N> constexpr bool IsAllowed(const State& current, const Event& event, const std::array<Transition<State, Event>, N>& table);`
  - `template <typename State, typename Event, std::size_t N> constexpr std::optional<State> Next(const State& current, const Event& event, const Transition<State, Event> (&table)[N]);`
  - `template <typename State, typename Event, std::size_t N> constexpr std::optional<State> Next(const State& current, const Event& event, const std::array<Transition<State, Event>, N>& table);`
- Behavior: table は利用者が明示する。C配列または `std::array` を受け取り、先に一致した transition を採用する。
- Failure/edge cases: 未定義遷移は `false` または `std::nullopt`。duplicate は先勝ち。
  空の `std::array` table は未定義遷移と同じ失敗値。
- Complexity/performance: lookup は table 線形走査 O(N)。allocation なし。
  `IsAllowed` は一致行の `to` を `std::optional` へ格納しない。
- Tests: known transition、unknown、duplicate、enum class state/event、last row、unlisted enum value、
  scalar state/event、empty `std::array`、Doxygen例、constexpr lookup。
- Do not implement: FSM framework、entry/exit action、guard expression DSL。

### cache Module

- Purpose: once/lazy value の補助。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  lazy value の thread-safety と例外後状態を局所的に固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/cache/ket_cache.h`,
  `modules/cache/ket_cache_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::cache`）:

  ```cpp
  template <typename T>
  class Lazy {
  public:
      Lazy() noexcept;
      ~Lazy() noexcept;
      Lazy(const Lazy&) = delete;
      Lazy& operator=(const Lazy&) = delete;
      Lazy(Lazy&&) = delete;
      Lazy& operator=(Lazy&&) = delete;

      bool HasValue() const noexcept;
      void Reset() noexcept;

      template <typename Factory>
      T& GetOrCreate(Factory factory);
  };
  ```

- Behavior: non-thread-safe。C++11 で `std::aligned_storage<sizeof(T), alignof(T)>` による手動 storage を使い、
  heap allocation と `std::optional` は使わない。保持値への pointer は placement new 成功後だけ生成し、
  lifetime 外の object として読み書きしない。`GetOrCreate` は値が無い場合だけ placement new で構築し、以後は同じ値への
  参照を返す。`Reset` と destructor は保持値がある場合だけ破棄して empty に戻す。factory が例外を投げた場合は
  empty のまま。`Lazy<T>` の object address は `Reset` しない限り保持値の lifetime 中に変わらない。
- Failure/edge cases: factory例外後は empty、Reset後は再生成、move-only value、再入は precondition 違反。
  `Lazy` 自体は copy/move とも禁止し、保持値の address stability を保つ。`T` の destructor は例外を投げないことを
  要求し、破棄中の例外は `std::terminate`。`GetOrCreate` 中に同じ `Lazy` の `GetOrCreate`/`Reset` を呼ぶ再入は
  未対応で、Doxygen の precondition に明記する。
- Complexity/performance: `HasValue`/`Reset` と生成後の `GetOrCreate` は O(1)。factory は高々1回。
  storage は object 内に置くため追加 allocation なし。
- Tests: factory呼び出し回数、Reset、例外後状態、move-only value、保持値 address stability、
  copy/move 禁止 compile-only、destructor 例外時 terminate。
- Do not implement: thread-safe cache、global registry、memoization framework、
  単一値専用の `OnceValue`。

### tlv Module

- Purpose: 小さい TLV record の encode/decode。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  raw pointer、`std::vector<std::uint8_t>`、bool+out参照で wire format 境界を固定できる。
  非推奨版 なし。非推奨理由: 標準ライブラリに TLV encode/decode の直接APIなし。
- Drop-in files: `modules/tlv/ket_tlv.h`,
  `modules/tlv/ket_tlv.cpp`、
  `modules/tlv/ket_tlv_test.cpp`、
  `modules/tlv/ket_tlv_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::tlv`）:
  - `struct View { std::uint16_t type; const std::uint8_t* value; std::uint32_t value_size; };`
  - `struct DecodeResult { View view; std::size_t consumed; };`
  - `std::vector<std::uint8_t> Encode(std::uint16_t type, const std::uint8_t* value, std::size_t value_size);`
  - `void Append(std::vector<std::uint8_t>& dst, std::uint16_t type, const std::uint8_t* value, std::size_t value_size);`
  - `bool TryDecode(const std::uint8_t* data, std::size_t size, DecodeResult& out) noexcept;`
- Behavior: wire format は `type:uint16 big-endian` + `length:uint32 big-endian` + `value bytes`。
  header は常に6 bytes。`Encode` は1 record の vector を返し、`Append` は既存 vector 末尾へ1 record を追加する。
  `TryDecode` は入力先頭の1 record だけ decode し、`out.consumed` に header + value length を入れる。
  `View::value` は入力bufferへの non-owning pointer で、decode 後も入力buffer lifetime に依存する。
  `View::value_size` は wire 上の `uint32` length をそのまま保持する。
- Failure/edge cases: `value == nullptr && value_size == 0` は空 value。`value == nullptr && value_size > 0` は
  encode 側 precondition 違反。`TryDecode(nullptr, size, out)` は `size` によらず `false` で `out` 不変。
  `Encode`/`Append` の `value_size` が `std::uint32_t` 最大値を超える場合は `std::length_error`。
  入力が6 bytes未満、declared length が残り size を超える、`6 + length` が `std::size_t` で overflow する場合は
  `false` で `out` 不変。`Append` は `value` が `dst` 内部storageを指す場合も元valueを追加する。
- Complexity/performance: encode/append は value_size O(n) で vector へ byte copy。decode は value を copy せず
  header を読むだけの O(1)。allocation は encode/append の vector 成長のみ。
- Tests: empty value、1 byte value、length 256、length 65536、append empty value、roundtrip、複数record先頭decode、
  self-overlap append、short header、short value、null+0、null+非0 precondition Doxygen、length過大、
  big-endian golden bytes、max `uint32` length header、`std::size_t` が `uint32` 上限を超える場合、
  decode失敗時 out 不変、view lifetime は呼び出し側責任であることの Doxygen、C++11 compile-only。
- Do not implement: struct丸ごとbytes化、schema language、protocol framework、nested TLV、multiple-record iterator、
  endian選択 option、`EncodeLengthPrefixed`/`TryDecodeLengthPrefixed`。

### tuple Module

- Purpose: tuple の小さい反復・変換補助。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::apply` と fold expression を使い、tuple走査を小さく実装できる。
  非推奨版 なし。非推奨理由: 標準ライブラリだけでは tuple要素ごとの副作用呼び出しが冗長。
- Drop-in files: `modules/tuple/ket_tuple.h`、`modules/tuple/ket_tuple_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::tuple`）:
  - `template <typename Tuple, typename F> void ForEach(Tuple&& tuple, F&& f);`
  - `template <typename Tuple, typename F> auto Transform(Tuple&& tuple, F&& f);`
- Behavior: `ForEach` は index順に同じ callable object をcopyせず呼ぶ。`Transform` は各要素に callable を適用した
  tuple を返す。
- Failure/edge cases: callable 例外は伝播。empty tuple、const tuple、reference要素、tuple-like object、
  rvalue要素を扱う。`Transform` が返す参照要素のlifetimeは呼び出し側責任。
- Complexity/performance: 要素数 N に対し compile-time 展開で O(N) 回呼び出し。実行時 allocation なし。
- Tests: empty、heterogeneous、const、reference、戻りtuple型、呼び出し順、非copy callable、
  tuple-like object、rvalue要素。
- Do not implement: pair helper、tuple DSL、reflection。

### build_config Module

- Purpose: compiler、OS、standard library feature の小さい判定 macro。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  compiler/OS/feature macro の差分を小さい範囲へ閉じ込められる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/build_config/ket_build_config.h`、
  `modules/build_config/ket_build_config_test.cpp`。
- Dependencies: 標準ライブラリと compiler predefined macros。他の ket module への依存なし。
- Public API Signatures:
  - `#define KET_CXX_VERSION <long literal>`
  - `#define KET_CXX_AT_LEAST(value) <0-or-1 expression>`
  - `#define KET_HAS_STD_OPTIONAL <0-or-1>`
  - `#define KET_HAS_STD_STRING_VIEW <0-or-1>`
  - `#define KET_HAS_STD_SPAN <0-or-1>`
  - `#define KET_HAS_STD_FORMAT <0-or-1>`
  - `#define KET_COMPILER_CLANG <0-or-1>`
  - `#define KET_COMPILER_GCC <0-or-1>`
  - `#define KET_COMPILER_MSVC <0-or-1>`
  - `#define KET_OS_WINDOWS <0-or-1>`
  - `#define KET_OS_LINUX <0-or-1>`
  - `#define KET_OS_MACOS <0-or-1>`
- Behavior: 全 macro は `KET_` prefix とし、feature/compiler/OS 判定 macro の値は必ず `0` または `1`。
  `KET_CXX_VERSION` は有効な C++ 標準値を `201103L`、`201402L`、`201703L`、`202002L`、`202302L` 形式で表す。
  MSVC では利用可能なら `_MSVC_LANG` を優先し、それ以外は `__cplusplus`。`KET_CXX_AT_LEAST(value)` は
  `KET_CXX_VERSION >= value` の定数式。compiler 判定は clang を `__clang__`、GCC を `__GNUC__ && !__clang__`、
  MSVC を `_MSC_VER` で判定し、clang-cl では `KET_COMPILER_CLANG == 1` とし `KET_COMPILER_MSVC == 0`。
  OS 判定は Windows を `_WIN32`、Linux を `__linux__`、macOS を `__APPLE__ && __MACH__` で判定し、unknown OS は
  3 macro すべて 0 とする。macOS macro は `KET_OS_MACOS` を採用し、
  Classic Mac OS ではなく現在の macOS 判定であることを名前に出す。standard library feature は対象標準以上、
  `__has_include` で対象 header を確認可能、かつ `<version>` または対象 header include 後に `__cpp_lib_*`
  feature-test macro が定義される場合だけ 1。`__has_include` が使えない場合は header 存在を未知として扱い、
  `__cpp_lib_*` が確認できる場合だけ 1。header が存在しても feature-test macro が無い場合は 0。
  各 macro は C++11 compile-only でも定義済みであり、feature macro は利用不能な標準では 0 に倒す。
- Failure/edge cases: `__has_include` が無い compiler、MSVC の `__cplusplus` 未更新、libstdc++/libc++/MSVC STL の
  feature macro 差、unknown OS、clang-cl の compiler macro 重複。include 順に依存せず、他 header より先に include
  しても後に include しても同じ値。feature 判定のために標準 header を include する場合は、この header 自身が
  必要最小限を include し、利用者の include 順に依存しない。
- Complexity/performance: preprocessor と compile-time 定数のみ。runtime cost、allocation、link symbol なし。
- Tests: C++11 compile-only、macro が全て定義済み、0/1 値、`KET_CXX_AT_LEAST` 境界、compiler/OS macro の条件、
  clang/GCC/MSVC の相互排他条件、unknown OS が全0でも成立すること、optional/string_view/span/format header
  availability と feature-test macro に応じた値、単独 include と標準 header include 後の include 順確認。
- Do not implement: config framework、global behavior switch、module間の必須依存化、project policy macro、
  version string 生成、C++標準別の個別 has macro、`std::expected` feature macro、macOS alias。

### math Module

- Purpose: 補間、角度、byte単位変換など小さい数学補助。Existing module のため、公開詳細は header
  Doxygen と実装を正本とし、このカードは採用済み仕様の要約として保守する。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  小さい数学処理の丸め、overflow、単位名をAPIで固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/math/ket_math.h`, `modules/math/ket_math_test.cpp`,
  `modules/math/ket_math_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::math`）:
  - `template <typename T> constexpr T Lerp(T a, T b, T t) noexcept;`
  - `template <typename T> constexpr T ToRadians(T degrees) noexcept;`
  - `template <typename T> constexpr T ToDegrees(T radians) noexcept;`
  - `template <typename T> constexpr bool NearlyEqual(T a, T b, T epsilon) noexcept;`
  - `bool TryBytesFromKiB(std::uint64_t kib, std::uint64_t& out) noexcept;`
  - `bool TryBytesFromMiB(std::uint64_t mib, std::uint64_t& out) noexcept;`
  - `constexpr double BytesToKiB(std::uint64_t bytes) noexcept;`
  - `constexpr double BytesToMiB(std::uint64_t bytes) noexcept;`
- Behavior: statistics や units framework へ広げない。`Lerp`、角度変換、`NearlyEqual` は `static_assert` で
  floating-point 型に限定する。`Lerp` は `t == 0` で `a`、`t == 1` で `b` を返し、それ以外では
  `a + (b - a) * t` の通常浮動小数点演算に従う。`NearlyEqual` の epsilon は有限で正の絶対許容差。
  byte変換のうち overflow しうる KiB/MiB から bytes への変換は `TryXxx` とし、成功時だけ `out` を更新する。
  bytes から KiB/MiB への変換は `double` の近似値を返し、大きな整数値は `double` 精度に従って丸める。
- Failure/edge cases: byte変換 overflow は `false` で `out` 不変。`NearlyEqual` は `epsilon <= 0` または
  NaN/Inf の epsilon、NaN 入力なら `false`。同符号Inf同士は通常の比較結果に従い、それ以外のInf入力は
  `false`。
- Complexity/performance: 全 API は O(1)。floating-point API と `BytesToKiB`/`BytesToMiB` は `constexpr`。
  `TryBytesFromKiB`/`TryBytesFromMiB` は C++11 では `noexcept` のみ、C++14 以降で `constexpr` 化してよい。
- Tests: endpoints、midpoint、extrapolation、angle roundtrip、epsilon、NaN/Inf、large byte values、
  byte変換 overflow、C++11 compile-only。
- Do not implement: units framework、matrix/vector math、statistics。

### lang Module

- Purpose: C++言語の小さい儀式（未使用無視、配列長、const化）を名前付き API にする補助。Existing module のため、
  公開詳細は header Doxygen と実装を正本とし、このカードは現状仕様と追加禁止範囲の要約。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  C++11/14の欠落や冗長な言語儀式を小さいAPIで名前付けできる。
  非推奨版 API別。非推奨理由: APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない。
- Drop-in files: `modules/lang/ket_lang.h`,
  `modules/lang/ket_lang_test.cpp`、
  `modules/lang/ket_lang_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::lang`）:
  - `template <typename... Args> void IgnoreUnused(const Args&... args) noexcept;`
  - `template <typename T, std::size_t N> constexpr std::size_t ArraySize(const T (&array)[N]) noexcept;`
  - `template <typename T> constexpr typename std::add_const<T>::type& AsConst(T& value) noexcept;`
  - `template <typename T> void AsConst(const T&& value) = delete;`
- Behavior: 純粋な言語儀式に限定し、状態やコピー・ムーブ制御は `object` module に分ける。
  `std::exchange` のような既存標準と完全に重なる別名は採用しない。未到達動作は失敗方針が重いため、
  初回 API では扱わない。
- Failure/edge cases: 空配列と非配列は不可、`AsConst` はrvalueを拒否し、戻り値は引数寿命に従う。
- Complexity/performance: 全 API は O(1)。`ArraySize`/`AsConst` は constexpr で実行時コストなし。
- Tests: unused 無視、move-only値、配列長、非配列拒否、const 化、rvalue拒否、C++11 compile-only。
- API別標準代替:
  - `IgnoreUnused`: C++17 `[[maybe_unused]]` と一部重複。式の明示破棄用途で採用。
  - `ArraySize`: C++17 `std::size`。
  - `AsConst`: C++17 `std::as_const`。
- Do not implement: macro 大量追加、attribute framework、`std::xxx` の単なる別名、
  C++20 `std::type_identity` と重なる `Identity`、C++23 `std::unreachable` と重なる `Unreachable`。

### object Module

- Purpose: copy/move/regular 型の儀式（コピー禁止、move 専用、move 後リセット）を mixin と
  helper で明示する補助。Existing module のため、公開詳細は header Doxygen と実装を正本とし、
  このカードは採用済み仕様の要約として保守する。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  copy/move意図を型定義の近くへ集約できる。非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/object/ket_object.h`, `modules/object/ket_object_test.cpp`,
  `modules/object/ket_object_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::object`）:
  - `class NonCopyable { protected: NonCopyable() = default; NonCopyable(NonCopyable&&) = default; NonCopyable& operator=(NonCopyable&&) = default; ~NonCopyable() = default; public: NonCopyable(const NonCopyable&) = delete; NonCopyable& operator=(const NonCopyable&) = delete; };`
  - `class NonMovable { protected: NonMovable() = default; ~NonMovable() = default; public: NonMovable(const NonMovable&) = delete; NonMovable& operator=(const NonMovable&) = delete; NonMovable(NonMovable&&) = delete; NonMovable& operator=(NonMovable&&) = delete; };`
  - `class MoveOnly { protected: MoveOnly() = default; ~MoveOnly() = default; public: MoveOnly(const MoveOnly&) = delete; MoveOnly& operator=(const MoveOnly&) = delete; MoveOnly(MoveOnly&&) = default; MoveOnly& operator=(MoveOnly&&) = default; };`
  - `template <typename T> class ResetOnMove { public: ResetOnMove() = default; explicit ResetOnMove(T value); ResetOnMove(const ResetOnMove& other) = delete; ResetOnMove& operator=(const ResetOnMove& other) = delete; ResetOnMove(ResetOnMove&& other) noexcept(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_default_constructible<T>::value && std::is_nothrow_move_assignable<T>::value); ResetOnMove& operator=(ResetOnMove&& other) noexcept(std::is_nothrow_move_assignable<T>::value && std::is_nothrow_default_constructible<T>::value); T& Get() noexcept; const T& Get() const noexcept; };`
- Behavior: 継承用 mixin と小さい helper に限定する。`NonCopyable` はcopyだけを禁止し、moveは禁止しない。
  `NonMovable` はcopy/moveを禁止する。`MoveOnly` はcopyを禁止しmoveを許可する。`ResetOnMove` は move 後に
  source を `T{}` へ戻す。自己move代入は no-op。`T` は default constructible、move constructible、
  move assignable を満たす型に限定する。例外発生時の保持値は`T`のmove/reset操作の保証に従う。
  mixin は比較演算を宣言せず、利用側の比較方針と衝突させない。
- Failure/edge cases: empty base optimization、move 後の source 状態、derived の特殊メンバ生成、
  比較演算を宣言しないことの相互作用。`ResetOnMove` のmove構築`noexcept`は`T`のmove構築、default構築、
  move代入がすべてnoexceptの場合だけ成り立つ。move代入`noexcept`は`T`のdefault構築とmove代入が
  noexceptの場合だけ成り立つ。
- Complexity/performance: mixin は空 base で size 増加なし。`ResetOnMove` の move は T の move + 既定値代入。
- Tests: copy 禁止 compile-only、NonCopyableのmove許可、move、reset、自己move代入、空 base size、
  利用側比較演算との併用、C++11 compile-only。
- Do not implement: regular type framework、smart pointer 再実装、`=delete` で足りる範囲の wrapper、
  swap helper の `SwapAndReset`。

### meta Module

- Purpose: C++11/14 欠落 type traits の小補助。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  C++11/14で不足する小さいtraitsを局所的に補える。
  非推奨版 API別。非推奨理由: APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない。
- Drop-in files: `modules/meta/ket_meta.h`,
  `modules/meta/ket_meta_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::meta`）:
  - `template <typename T> using RemoveCvref = typename std::remove_cv<typename std::remove_reference<T>::type>::type;`
  - `template <typename T> struct TypeIdentity { using type = T; };`
  - `template <typename...> struct AlwaysFalse : std::false_type {};`
  - `template <typename...> using VoidT = void;`
- Behavior: template error を読みやすく保つ小さい alias/type に限定する。評価時の実行動作は持たない。
- Failure/edge cases: C++17 以降の標準代替、SFINAE 使用時の diagnostics。
- Complexity/performance: 全て compile-time の alias/trait。実行時コストと allocation なし。
- Tests: alias型一致、C++11 compile-only、SFINAE。
- API別標準代替:
  - `VoidT`: C++17 `std::void_t`。
  - `RemoveCvref`: C++20 `std::remove_cvref`。
  - `TypeIdentity`: C++20 `std::type_identity`。
  - `AlwaysFalse`: 直接代替なし。template diagnostics改善が必要な場合のみ採用。
- Do not implement: meta programming framework、concepts 代替体系、難読化する traits。

### concurrency Module

- Purpose: join 忘れと future timeout 確認程度の局所補助。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  thread join と future ready 判定の小さい儀式を局所化できる。
  非推奨版 API別。非推奨理由: APIごとに標準代替の有無が異なるため、module単位では非推奨にしない。
- Drop-in files: `modules/concurrency/ket_concurrency.h`,
  `modules/concurrency/ket_concurrency_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::concurrency`）:

  ```cpp
  class JoiningThread {
  public:
      JoiningThread() noexcept;
      explicit JoiningThread(std::thread thread) noexcept;
      ~JoiningThread() noexcept;
      JoiningThread(const JoiningThread&) = delete;
      JoiningThread& operator=(const JoiningThread&) = delete;
      JoiningThread(JoiningThread&& other) noexcept;
      JoiningThread& operator=(JoiningThread&& other) noexcept;

      std::thread& Get() noexcept;
      bool Joinable() const noexcept;
  };

  template <typename Future>
  bool IsReady(Future& future) noexcept;
  ```

- Behavior: `JoiningThread` は所有する `std::thread` を destructor で joinable なら join する。move 代入は
  旧 thread を join してから所有を移す。copy は禁止。`IsReady` は `wait_for(0)` の結果で ready 判定し、
  `std::future_status::deferred` は ready ではないため `false`。self-move assignment は no-op。`Get()` は内部
  `std::thread` への参照を返し、直接操作した場合の joinable 状態は利用者責任。
- Failure/edge cases: self-join は precondition 違反。destructor や move代入中に join できない状況、または
  join が例外を投げる状況では `std::terminate`。move代入時の既存threadは新しい thread を所有する前に join。
  `IsReady` は `future.valid()` を precondition とし、invalid future は呼び出さない。compile-only check では
  `std::future` と `std::shared_future` の両方で `wait_for(0)` が使えることを確認する。
- Complexity/performance: `Joinable`/`Get`/`IsReady` は O(1)。destructor と move 代入は join 完了まで blocking。
- Tests: default、joinable、move/self-move、move代入時の旧thread join、ready/not ready、deferred、
  invalid future precondition の Doxygen 明記、C++11 compile-only。
- API別標準代替:
  - `JoiningThread`: C++20 `std::jthread`。
  - `IsReady`: 直接代替なし。`future.wait_for(0)` の結果判定を名前にする場合のみ採用。
- Do not implement: thread pool、executor、lock framework、cancellation framework。

### uuid Module

- Purpose: UUID の parse/format。generation は扱わない。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で canonical UUID の失敗条件を固定できる。
  非推奨版 なし。非推奨理由: 標準ライブラリに UUID parse/format の直接APIなし。
- Drop-in files: `modules/uuid/ket_uuid.h`、`modules/uuid/ket_uuid.cpp`、
  `modules/uuid/ket_uuid_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::uuid`）:
  - `struct Uuid { std::uint8_t bytes[16] = {}; };`
  - `std::optional<Uuid> Parse(std::string_view text) noexcept;`
  - `std::string Format(Uuid value);`
- Behavior: canonical hyphen形式 `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx` を parse/format する。
  hex は upper/lower を受け付け、format は lower-case 固定。
- Failure/edge cases: 長さ不一致、hyphen位置不一致、不正hex、brace付き形式は失敗。
- Complexity/performance: parse/format は固定36文字/16 byte の O(1)。allocation は format のみ。
- Tests: zero uuid、normal uuid、upper input、bad length、bad hyphen、bad hex、format。
- Do not implement: UUID generation、version/variant validation、URN形式、OS乱数。

### color Module

- Purpose: RGB小値型。Existing module のため、公開詳細は header Doxygen と実装を正本とし、
  この仕様カードは採用範囲と境界方針を固定する。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  RGB小値型の許容表記と不正hexを小さいAPIで固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/color/ket_color.h`,
  `modules/color/ket_color_test.cpp`,
  `modules/color/ket_color_cxx11_check.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::color`）:
  - `struct Rgb { constexpr Rgb() noexcept; constexpr Rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept; ... };`
  - `struct FormatOptions { constexpr FormatOptions() noexcept; explicit constexpr FormatOptions(bool with_hash) noexcept; ... };`
  - `constexpr bool operator==(Rgb lhs, Rgb rhs) noexcept;`
  - `constexpr bool operator!=(Rgb lhs, Rgb rhs) noexcept;`
  - `bool TryParse(const char* text, std::size_t size, Rgb& out) noexcept;`
  - `bool TryParse(const std::string& text, Rgb& out) noexcept;`
  - `std::string Format(Rgb color);`
  - `std::string Format(Rgb color, FormatOptions options);`
- Behavior: 6桁hex `RRGGBB` を parse し、先頭 `#` は任意。alpha は扱わない。C++11 最小なので入力は
  `std::string_view` ではなく `const char*` + size または `const std::string&`、戻りは `std::optional` ではなく
  `out` 引数 + bool。
  `Format` は lower-case、`FormatOptions::with_hash` で `#` 前置を制御する。
- Failure/edge cases: null pointer、不正長、不正hex、3桁短縮形、符号付き表記は失敗、case
  混在は許容、alpha 付きは失敗。
- Complexity/performance: parse/format は固定3 byte の O(1)。pointer parse は allocation なし。
  string parse は入力stringを参照し、allocation は format のみ。
- Tests: default/channel constructor、等価比較、black/white、lower/upper、先頭 # 有無、std::string入力、
  null、sign、不正長、不正文字、blue channel での後段失敗、format、C++11 compile-only。
- Do not implement: color space 変換、CSS color name、alpha、theme system。

### percent Module

- Purpose: 0〜100% の小さい値型。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  percent小値型の範囲、丸め、NaN方針を局所的に固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/percent/ket_percent.h`、`modules/percent/ket_percent_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API Signatures（`namespace ket::percent`）:

  ```cpp
  class Percent {
  public:
      constexpr Percent() noexcept;
      static bool TryFromBasisPoints(std::uint32_t basis_points, Percent& out) noexcept;
      static bool TryFromPercent(double percent, Percent& out) noexcept;
      static bool TryFromRatio(double ratio, Percent& out) noexcept;
      static Percent FromPercentClamped(double percent) noexcept;

      constexpr std::uint16_t BasisPoints() const noexcept;
      constexpr double ToPercent() const noexcept;
      constexpr double ToRatio() const noexcept;
  };

  constexpr bool operator==(Percent a, Percent b) noexcept;
  constexpr bool operator!=(Percent a, Percent b) noexcept;
  constexpr bool operator<(Percent a, Percent b) noexcept;
  constexpr bool operator<=(Percent a, Percent b) noexcept;
  constexpr bool operator>(Percent a, Percent b) noexcept;
  constexpr bool operator>=(Percent a, Percent b) noexcept;
  ```

- Behavior: 内部表現は basis points。保持範囲は `0..10000` で、1 basis point は `0.01%`。
  default constructor は 0%。`TryFromBasisPoints` は `basis_points <= 10000` のみ成功。
  `TryFromPercent` は percent 単位の `0.0..100.0`、`TryFromRatio` は ratio 単位の `0.0..1.0` のみ成功し、
  nearest basis point へ丸める。丸めは非負値に対する `floor(value + 0.5)` 相当。`ToPercent` は
  `BasisPoints() / 100.0`、`ToRatio` は `BasisPoints() / 10000.0`。比較は basis points の整数比較。
  `FromPercentClamped` は percent 単位入力を `0.0..100.0` へ clamp してから同じ丸めで `Percent` を返す。
- Failure/edge cases: `TryXxx` は範囲外、NaN、Inf で `false`、`out` 不変。`FromPercentClamped` は
  NaN を 0%、`-Inf` を 0%、`+Inf` を 100% として扱う。`99.995%` のように丸めで
  10000 basis points になる値は成功。
- Complexity/performance: 全 API は O(1)。保持値は `std::uint16_t` 1個。allocation・例外なし。
  `TryFromPercent`/`TryFromRatio`/`FromPercentClamped` は floating-point 丸めを行うため `constexpr` ではない。
- Tests: default 0%、basis points 0/1/10000/10001、percent 0/12.345/99.995/100、ratio 0/0.12345/1、
  負値、100超過、NaN、Inf、FromPercentClamped、比較演算、out不変。
- Do not implement: units framework、progress UI、format localization、`std::optional` convenience、百分率文字列 parse/format。

## 6. recipes 仕様カード

### binary_payload Recipe

- Purpose: BCD、endian、byte_writer、hex を組み合わせた電文構築例。
- C++ version: mixed。利用する実moduleの要件に従う。
- Drop-in files: `recipes/binary_payload/README.md`、
  `recipes/binary_payload/binary_payload_example.cpp`。
- Dependencies: 実装済み ket module と標準ライブラリ。
- Public API: なし。recipe code のみ。
- Behavior: command byte、BE sequence、BCD date、単純 checksum、Dump 診断を例示する。
- Failure/edge cases: module API の失敗値処理を例内で示す。
- Tests: example が build でき、代表payloadを期待hexと比較できること。
- Do not implement: 新規 module API、業務固有 protocol、framework。

### command_parser Recipe

- Purpose: CLI、parse、enums を組み合わせた小さい command parser 例。
- C++ version: mixed。利用する実moduleの要件に従う。
- Drop-in files: `recipes/command_parser/README.md`、
  `recipes/command_parser/command_parser_example.cpp`。
- Dependencies: 実装済み ket module と標準ライブラリ。
- Public API: なし。recipe code のみ。
- Behavior: `--mode auto`、`--port 1234`、`--verbose`、enum table、parse失敗メッセージを示す。
- Failure/edge cases: missing option、invalid enum、invalid port、duplicate option。
- Tests: example が build でき、代表引数列を期待結果と比較できること。
- Do not implement: CLI framework、新規 module API、shell parser。

### c_api_wrapper Recipe

- Purpose: `interop` と `scope` を使った C API 境界の RAII 化例。
- C++ version: mixed。利用する実moduleの要件に従う。
- Drop-in files: `recipes/c_api_wrapper/README.md`、
  `recipes/c_api_wrapper/c_api_wrapper_example.cpp`。
- Dependencies: 実装済み ket module と標準ライブラリ。
- Public API: なし。recipe code のみ。
- Behavior: handle close、errno 保存、fixed C buffer への安全コピーを示す。
- Failure/edge cases: open失敗、copy不足、cleanup実行、errno復元。
- Tests: example が build でき、失敗経路を小さい fake C API で確認できること。
- Do not implement: OS固有大規模 wrapper、新規 module API、resource framework。

## 7. 実装完了条件チェックリスト

製造依頼を完了する前に、対象 module ごとに次を満たす。

```txt
[ ] modules/<name>/ket_<name>.h を作った
[ ] modules/<name>/ket_<name>.cpp を作った、または header-only と明記した
[ ] modules/<name>/ket_<name>_test.cpp を作った
[ ] ヘッダ先頭に Doxygen @file と drop-in 条件を書いた
[ ] C++バージョン要件と標準代替を書いた
[ ] 他のライブラリへの依存と ket module 非依存を書いた
[ ] 公開APIは namespace ket::<module>
[ ] 他の ket module に依存していない
[ ] 公開ヘッダが必要な標準ヘッダを自分で include している
[ ] 公開ヘッダの section banner が規約通り
[ ] 公開API関数 Doxygen に @brief / @param / @retval / @pre / @post / @code がある（constructor/destructorは @retval なし）
[ ] struct / class / enum の Doxygen に @brief がある
[ ] 失敗条件を戻り値・precondition・例外のどれで扱うか固定した
[ ] null / empty / overflow / size不足 / invalid input のテストがある
[ ] 各 TEST に @test / @brief / @details / @pre / @post がある
[ ] C++11/14 module は compile-only check を追加した
[ ] 未実装候補として catalog.md に痛み、候補API、失敗条件、テスト観点が記録済みである
[ ] progress.md を実moduleとして更新した
[ ] python3 tools/check_repository.py を実行した
[ ] python3 tools/check_python.py を実行した
[ ] python3 tools/check_layout.py を実行した
[ ] python3 tools/check_format.py を実行した
[ ] cmake --preset dev を実行した
[ ] cmake --build --preset dev を実行した
[ ] cmake --build --preset dev --target check-static を実行した
[ ] cmake --build --preset dev --target check-conventions を実行した
[ ] ctest --preset dev を実行した
[ ] cmake --preset sanitize を実行した
[ ] cmake --build --preset sanitize を実行した
[ ] ctest --preset sanitize を実行した
[ ] git diff --check を実行した
```
