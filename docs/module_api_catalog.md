# ket module/API catalog

作成日: 2026-06-14

## 1. 正本の目的と使い方

この文書は、ket の module/API 製造依頼に使う正本である。
[`module_api_proposal.md`](proposals/module_api_proposal.md) を入力資料として整理し、
実装依頼時に必要な API、境界条件、テスト観点、禁止事項を module 単位で固定する。

この文書は `catalog.md` の置き換えではない。`catalog.md` は候補APIの保管場所、
`progress.md` は実装状況、各 module header の Doxygen は実装済みAPIの詳細仕様を管理する。
未実装 module の最終的な公開API、C++要件、失敗方針は、この文書の module別仕様カードを正とする。

製造依頼では、対象 module の仕様カードだけでなく、この文書の共通ルールと
`AGENTS.md`、`README.md`、`docs/module_lifecycle.md`、`docs/style.md`、
`docs/testing.md` を同時に守る。

## 2. module製造の共通ルール

- 実装する module だけ `modules/<name>/` を作る。空フォルダや空 `.cpp` は作らない。
- 標準形は `modules/<name>/ket_<name>.h`、
  `modules/<name>/ket_<name>.cpp`、`modules/<name>/ket_<name>_test.cpp`。
  header-only で十分な場合は `.cpp` を置かない。
- 公開APIは `namespace ket` に置く。内部 helper は header 内なら `ket::detail`、
  `.cpp` 内なら無名 namespace。
- 各 module は原則として他の ket module に依存しない。小さい内部処理の重複は許容する。
- 公開ヘッダは include what you use を守り、自分が必要な標準ヘッダを自分で include する。
- 公開ヘッダは Doxygen `@file` コメント、公開API宣言、内部実装、公開API定義の順に書く。
- 非optionalの出力引数と入出力引数は参照型で受ける。`nullptr` が意味を持つ optional
  出力や C API 境界だけポインタ型を使い、その理由を Doxygen に書く。
- proposal に `T* out` と書かれていた非optional出力は、製造時に `T& out` へ正規化する。
- C++11/14 対応 module では、GoogleTest とは別に最低標準の compile-only check を追加する。
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

## 3. module/API 一覧表

`C++ Min` は最小要件、`C++ 推奨` は適用を推奨する版、`C++ 非推奨` は標準ライブラリで
容易かつ明確に代替できるため適用を推奨しない版を表す。3列は各 module header の Doxygen
`@par C++バージョン要件` と一致させる。`C++ 非推奨` の `なし` は、対象範囲の標準ではこの
module の中核 API に同等の標準代替がないことを意味する。`Representative API` は一覧用の代表名であり、
完全な公開APIは module別仕様カードを正とする。

| Module              | Priority | Manufacturing Status | C++ Min | C++ 推奨  | C++ 非推奨 | Files                     | Kind        | Purpose                             | Representative API                                           | Failure/Boundary Policy                           | Tests                                      | Notes                               |
| ------------------- | -------- | -------------------- | ------- | --------- | ---------- | ------------------------- | ----------- | ----------------------------------- | ------------------------------------------------------------ | ------------------------------------------------- | ------------------------------------------ | ----------------------------------- |
| `bcd`               | done     | `Existing`           | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test      | binary      | packed BCD 変換                     | `ParseBcd`, `ToBcd8`, `BcdToDecimalString`                   | 不正nibble、空入力、overflow は失敗値             | 実装済み境界値テスト                       | 追加は BCD 妥当性判定まで           |
| `string`            | done     | `Existing`           | C++17   | C++17以降 | なし       | header-only + test        | string      | 文字列片の連結と追記                | `StrCat`, `StrAppend`                                        | raw C string は非null、allocation例外あり         | 実装済み境界値テスト                       | format API ではない                 |
| `bits`              | P0       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test        | numeric     | bit/nibble/mask の事故防止          | `HighNibble`, `LowNibble`, `HasBit`, `TryMask`               | unsigned integral 限定、範囲外 bit は失敗/false   | nibble、bit幅、mask境界                    | C++20 `<bit>` は一部のみ重複        |
| `numeric`           | P0       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test        | numeric     | overflow、align、cast の小さい正解  | `TryAlignUp`, `TryCheckedAdd`, `TryCheckedCast`              | 0除算、alignment 0、overflow は失敗               | min/max、overflow、signed/unsigned         | optional convenience は初回なし     |
| `endian`            | P0       | `Ready`              | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test      | binary      | unaligned/endian 読み書き           | `LoadBe32`, `LoadLe32`, `StoreBe16`, `TryLoadBe32`           | plain load/store は precondition、Try は失敗値    | BE/LE、null、size不足                      | reinterpret cast 禁止               |
| `hex`               | P0       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test      | diagnostic  | bytes と16進文字列/hex dump         | `BytesToHex`, `HexToBytes`, `HexDump`, `ToHexString`         | null+非0 size は precondition、奇数桁は失敗       | 空入力、separator、不正文字                | dump形式は固定                      |
| `parse_numeric`     | P0       | `Ready`              | C++17   | C++17以降 | なし       | header-only + test        | parsing     | `from_chars` 周りの儀式除去         | `TryParseUInt`, `ParseUInt`, `ParseBool`, `ParseHex`         | 完全消費、whitespaceなし、overflow失敗            | 空、最大値、overflow、prefix               | bool は case-sensitive              |
| `enum_table`        | P0       | `Ready`              | C++17   | C++17以降 | なし       | header-only + test        | enum        | enum class と文字列変換             | `EnumEntry`, `EnumName`, `ParseEnum`, `HasFlag`              | table完全一致、重複は先勝ち                       | known/unknown、duplicate、flags            | reflection はしない                 |
| `container`         | P0       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | container   | map/vector の小さい儀式             | `ContainsKey`, `AtOrNull`, `GetOrDefault`, `EraseIf`         | 見つからない場合は null/default/0                 | key有無、factory呼び出し、削除件数         | `Contains` 方針を固定必要           |
| `string_ascii`      | P0       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test      | string      | ASCII 前提の文字列処理              | `TrimAscii`, `SplitView`, `ToLowerAscii`, `ReplaceAll`       | ASCII whitespaceのみ、view lifetime明記           | 空要素、UTF-8 byte保持、case変換           | Unicode処理ではない                 |
| `scope`             | P0       | `Ready`              | C++11   | C++11以降 | なし       | header-only + test        | RAII        | cleanup と復元漏れ防止              | `ScopeExit`, `MakeScopeExit`, `RestoreOnExit`                | destructor 例外は terminate、move後 inactive      | dismiss、move、二重実行なし                | `Finally` は作らない                |
| `byte_reader`       | P0       | `Ready`              | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test      | binary      | byte列の安全な逐次読み取り          | `ByteReader`, `ReadU8`, `ReadBe16`, `ReadBytes`              | 成功時だけ offset 更新、invalid reader は失敗     | 空、ぴったり、size不足、offset保持         | endian module 非依存                |
| `byte_writer`       | P0       | `Ready`              | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test      | binary      | fixed buffer への安全な逐次書き込み | `ByteWriter`, `WriteU8`, `WriteLe32`, `WriteBytes`           | 成功時だけ offset と buffer 更新                  | 空、ぴったり、size不足、buffer不変         | endian module 非依存                |
| `bytes_builder`     | P0       | `Needs Spec Split`   | C++17   | C++17以降 | なし       | 未製造                    | binary      | owning payload builder              | `BytesBuilder`, `AppendU8`, `AppendBe16`, `Build`            | allocation例外あり、null+非0 size は precondition | BE/LE、reserve、Build後move                | fluent/free API 境界を固定必要      |
| `date`              | P0       | `Needs Spec Split`   | C++11   | C++11〜17 | C++20以降  | 未製造                    | date        | 日付・時刻の妥当性                  | `IsLeapYear`, `IsValidDate`, `TryDaysInMonth`                | Gregorian、year >= 1、leap secondなし             | 2000/1900、2/29、month 0/13                | C++20 `std::chrono` 境界を固定必要  |
| `deadline`          | P0       | `Ready`              | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test      | time        | timeout と elapsed time             | `Stopwatch`, `Deadline`                                      | `steady_clock`のみ、負timeoutは期限切れ           | zero/future/remaining/restart              | system_clock と混ぜない             |
| `cli`               | P0       | `Needs Spec Split`   | C++17   | C++17以降 | なし       | 未製造                    | CLI         | 小さい CLI option 取得              | `ArgvView`, `HasOption`, `GetOption`, `Positional`           | `--key value`/`--key=value`、重複は先勝ち         | flag、missing value、positional            | option値 parse 方針を固定必要       |
| `byte_view`         | P1       | `Needs Spec Split`   | C++11   | C++11〜17 | C++20以降  | 未製造                    | view        | non-owning byte span                | `ByteView`, `MutableByteView`, `TrySubView`                  | `nullptr+0` は空、`nullptr+非0` は invalid        | lifetime、subview、bounds                  | method/free function 境界を固定必要 |
| `utf8`              | P1       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test      | text        | UTF-8 検査の隔離                    | `ValidateUtf8`, `IsUtf8`, `Utf8Length`                       | normalizationなし、不正offsetを返す               | ASCII、多byte、不正sequence                | grapheme数ではない                  |
| `file`              | P1       | `Needs Spec Split`   | C++17   | C++17以降 | なし       | 未製造                    | filesystem  | ファイル全読み/全書き               | `TryReadAllText`, `WriteAllBytes`, `FileSize`                | `std::error_code*` は optional detail             | not found、空、binary、error_code          | error detail API を固定必要         |
| `io_stream`         | P1       | `Ready`              | C++11   | C++11以降 | なし       | `.h` + `.cpp` + test      | stream      | stream の確実な読み書き             | `ReadExactly`, `WriteAll`, `StreamStateSaver`                | requested size を読み切った時だけ成功             | short read、state復元、line trim           | ASCII trim のみ                     |
| `format_value`      | P1       | `Needs Spec Split`   | C++17   | C++17以降 | なし       | 未製造                    | diagnostic  | 診断用文字列化                      | `FormatByteCount`, `FormatDuration`, `FormatBool`            | 表記形式を追加仕様で固定                          | 単位境界、負duration、幅指定               | `ToHexString` は `hex` に集約       |
| `algorithm_range`   | P1       | `Needs Spec Split`   | C++11   | C++11〜17 | C++20以降  | 未製造                    | algorithm   | iterator pair の儀式除去            | `AllOf`, `FindIf`, `ForEachIndex`                            | `std::algorithm` 別名だけなら不採用               | empty、predicate副作用、iterator           | C++20以降は `std::ranges` を優先    |
| `memory`            | P1       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | memory      | alignment/object bytes              | `IsAligned`, `SecureZeroMemory`, `ObjectBytes`               | secure zero の保証範囲を追加仕様で固定            | null、alignment 0、volatile消去            | object lifetime へ踏み込まない      |
| `pointer`           | P1       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | pointer     | null/ownership の明示               | `NotNull`, `LockWeak`, `AddressOf`                           | `NotNull(nullptr)` の失敗方針を固定必要           | nullptr、weak expired、operator            | ownership型は初回なし               |
| `testing_bytes`     | P1       | `Ready`              | C++17   | C++17以降 | なし       | test-helper header + test | testing     | bytes系テスト補助                   | `BytesEq`, `HexEq`                                           | GoogleTest 依存を明記、mismatch情報を返す         | offset差分、hex差分、不正hex               | library本体ではない                 |
| `semantic_version`  | P1       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test      | parsing     | semver-like 値の parse/compare      | `SemanticVersion`, `ParseSemanticVersion`, `Compare...`      | `major.minor.patch`のみ、leading zero失敗         | 0.0.0、compare、overflow                   | prerelease/buildなし                |
| `ipv4`              | P1       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test      | network     | IPv4 parse/format                   | `Ipv4Address`, `ParseIpv4Address`, `FormatIpv4Address`       | dotted decimalのみ、leading zero失敗              | octet境界、個数不足/過多、format           | IPv6/CIDRなし                       |
| `port`              | P1       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test      | network     | TCP/UDP port parse/format           | `Port`, `TryMakePort`, `ParsePort`, `FormatPort`             | 0〜65535、空白・符号・leading zero失敗            | 0/65535、overflow、不正文字                | socket addressなし                  |
| `mac_address`       | P1       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test      | network     | MAC address parse/format            | `MacAddress`, `ParseMacAddress`, `FormatMacAddress`          | `:` と `-` を許可、混在とCisco形式は失敗          | upper/lower、不正hex、区切り               | Cisco形式なし                       |
| `function`          | P2       | `Ready`              | C++17   | C++17以降 | なし       | header-only + test        | callable    | callable/visitor の儀式除去         | `Overload`, `MakeOverload`, `Noop`                           | `Noop` は例外なし、`Overload` は所有              | visit、overload解決、copy/move             | `FunctionRef` は初回なし            |
| `variant_match`     | P2       | `Needs Spec Split`   | C++17   | C++17以降 | なし       | 未製造                    | variant     | `std::variant` visitor 補助         | `Match`, `Holds`, `GetIf`                                    | `function` との重複整理が必要                     | value/ref、const、exception伝播            | `Overload` の配置を固定必要         |
| `optional_ext`      | P2       | `Needs Spec Split`   | C++17   | C++17以降 | なし       | 未製造                    | optional    | optional の小さい合成               | `MapOptional`, `AndThen`, `ValueOrEval`, `HasValueAll`       | factory は必要時だけ呼ぶ                          | empty/value、参照、factory回数             | API別標準代替を固定必要             |
| `contract`          | P2       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | contract    | precondition 明示                   | `Expects`, `Ensures`, `RequireNonNull`                       | assert/abort/terminate 方針を固定必要             | debug/release、nullptr、bounds             | macro 過多にしない                  |
| `c_interop`         | P2       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | interop     | C API 境界の事故防止                | `ErrnoGuard`, `CopyToCBuffer`, `UniqueHandle`                | handle invalid 値と deleter 方針を固定必要        | errno復元、buffer不足、release             | OS handle専用化しない               |
| `platform_error`    | P2       | `Needs Spec Split`   | C++17   | C++17以降 | なし       | 未製造                    | platform    | errno/Windows error の文字列化      | `ErrnoMessage`, `WindowsErrorMessage`, `EnvironmentVariable` | OS差を隠しすぎない、`#ifdef` 条件明記             | known errno、missing env、Windows guard    | platform別仕様が必要                |
| `state_table`       | P2       | `Ready`              | C++17   | C++17以降 | なし       | header-only + test        | state       | 小さい状態遷移表                    | `Transition`, `IsValidTransition`, `NextState`               | table先頭一致、未定義遷移は失敗                   | known/unknown、duplicate、enum             | FSM frameworkなし                   |
| `cache_once`        | P2       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | cache       | once/lazy value                     | `Lazy`, `OnceValue`, `GetOrCreate`                           | thread-safe有無を名前か仕様で固定必要             | factory回数、reset、例外後状態             | 初回は non-thread-safe 推奨         |
| `serialization_tlv` | P2       | `Needs Spec Split`   | C++17   | C++17以降 | なし       | 未製造                    | binary      | length-prefix/TLV                   | `EncodeTlv`, `TryDecodeTlv`, `TlvView`                       | length幅、endian、最大長を固定必要                | 短い入力、length超過、roundtrip            | struct丸ごとbytes化禁止             |
| `tuple`             | P2       | `Ready`              | C++17   | C++17以降 | なし       | header-only + test        | tuple       | tuple/pair の小さい補助             | `TupleForEach`, `TupleTransform`                             | evaluation order と戻り型を固定                   | empty、heterogeneous、const                | structured binding 競合は避ける     |
| `build_config`      | P2       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | config      | feature detection                   | `KET_HAS_STD_OPTIONAL`, `KET_OS_LINUX`                       | macro汚染の範囲を固定必要                         | compiler/OS 条件、include順                | 必要になるまで保留                  |
| `math_small`        | P2       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | math        | 補間・角度・単位変換                | `Lerp`, `NearlyEqual`, `KiBToBytes`                          | 精度、overflow、型制約を固定必要                  | endpoints、epsilon、large values           | units frameworkなし                 |
| `language`          | P2       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | language    | C++言語の小さい儀式                 | `IgnoreUnused`, `Unreachable`, `ArraySize`, `AsConst`        | 標準代替の登場版をAPI別に固定必要                 | unused無視、配列長、const化、C++11 compile | module単位では非推奨にしない        |
| `object`            | P2       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | object      | copy/move/regular型の儀式           | `NonCopyable`, `NonMovable`, `MovableOnly`, `ResetOnMove`    | defaulted comparison と衝突しない範囲を固定必要   | copy禁止、move、reset、swap                | =deleteで足りる範囲は入れない       |
| `meta`              | P3       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | meta        | type traits 補助                    | `RemoveCvref`, `TypeIdentity`, `VoidT`                       | C++標準別の標準代替をAPI別に整理必要              | alias、SFINAE、C++11 compile               | module単位では非推奨にしない        |
| `concurrency_small` | P3       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未製造                    | concurrency | join/lock/timeout の局所補助        | `JoiningThread`, `FutureReady`                               | destructor join/terminate 方針を固定必要          | move、joinable、ready timeout              | module単位では非推奨にしない        |
| `uuid`              | P3       | `Ready`              | C++17   | C++17以降 | なし       | `.h` + `.cpp` + test      | parsing     | UUID parse/format                   | `Uuid`, `ParseUuid`, `FormatUuid`                            | canonical hyphen形式のみ、generationなし          | upper/lower、不正長、不正hex               | 乱数/OS APIなし                     |
| `color_rgb`         | P3       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未定                      | value       | RGB小値型                           | `ParseColorRgb`, `FormatColorRgbHex`                         | 許容表記と alpha有無を固定必要                    | hex長、不正文字、format                    | proposalに詳細仕様なし              |
| `percent`           | P3       | `Needs Spec Split`   | C++11   | C++11以降 | なし       | 未定                      | value       | percent小値型                       | `Percent::FromRatio`, `ClampPercent`                         | 範囲、丸め、NaN方針を固定必要                     | 0/100、負値、>100、ratio                   | proposalに詳細仕様なし              |
| `binary_payload`    | P3       | `Recipe`             | mixed   | —         | —          | `recipes/...`             | recipe      | binary payload 構築例               | recipe code                                                  | module API は追加しない                           | example build/test                         | module製造依頼ではない              |
| `command_parser`    | P3       | `Recipe`             | mixed   | —         | —          | `recipes/...`             | recipe      | CLI/parser/enum の組み合わせ例      | recipe code                                                  | module API は追加しない                           | example build/test                         | module製造依頼ではない              |
| `c_api_wrapper`     | P3       | `Recipe`             | mixed   | —         | —          | `recipes/...`             | recipe      | C API 境界 RAII 化例                | recipe code                                                  | module API は追加しない                           | example build/test                         | module製造依頼ではない              |

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

- Purpose: packed BCD と10進整数・10進文字列の相互変換。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  packed BCDの直接代替が標準ライブラリになく、`std::optional`で失敗値を明確に扱える。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/bcd/ket_bcd.h`、`modules/bcd/ket_bcd.cpp`、
  `modules/bcd/ket_bcd_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `ParseBcd(std::uint8_t)`, `ParseBcd(std::uint16_t)`,
  `ParseBcd(std::uint32_t)`, `ToBcd8`, `ToBcd16`, `ToBcd32`,
  `BcdToDecimalString`, `DecimalStringToBcd`。
- Behavior: 固定幅 packed BCD は整数へ変換し、任意バイト長 packed BCD は桁数と先頭ゼロを
  10進文字列として保持する。
- Failure/edge cases: nibble > 9、`nullptr`、空入力、非10進数字、整数 overflow、負数、
  固定幅桁数超過は失敗値。
- Tests: 0x00、0x09、0x10、0x99、不正nibble、先頭ゼロ保持、奇数桁文字列、非数字。
- Do not implement: `BcdDate`、`BcdTime`、業務固有BCD解釈。nibble単体判定の公開API追加は避ける。

### string Module

- Purpose: format ではない文字列片の連結と既存文字列への追記。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view`を利用でき、文字列片連結を標準ライブラリのみで安全に薄く包める。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/string/ket_string.h`、`modules/string/ket_string_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `StrCat(...)`, `StrAppend(std::string&, ...)`。
- Behavior: `std::string_view` に変換可能な文字列片と `char` を入力順に連結する。embedded
  NUL は length-aware に保持する。
- Failure/edge cases: raw C string は非nullかつ null 終端が precondition。合計長が
  `std::string::max_size()` を超える場合は `std::length_error`。
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
- Public API:
  - `constexpr bool IsNibble(std::uint8_t value) noexcept`
  - `constexpr std::uint8_t HighNibble(std::uint8_t value) noexcept`
  - `constexpr std::uint8_t LowNibble(std::uint8_t value) noexcept`
  - `constexpr bool TryMakeByteFromNibbles(std::uint8_t high, std::uint8_t low, std::uint8_t& out) noexcept`
  - `template <typename T> constexpr unsigned BitWidth() noexcept`
  - `template <typename T> constexpr bool HasBit(T value, unsigned bit_index) noexcept`
  - `template <typename T> constexpr bool TrySetBit(T value, unsigned bit_index, T& out) noexcept`
  - `template <typename T> constexpr bool TryClearBit(T value, unsigned bit_index, T& out) noexcept`
  - `template <typename T> constexpr bool TryToggleBit(T value, unsigned bit_index, T& out) noexcept`
  - `template <typename T> constexpr bool TryMask(unsigned width, T& out) noexcept`
  - `template <typename T> constexpr unsigned PopCount(T value) noexcept`
  - `template <typename T> constexpr bool IsPowerOfTwo(T value) noexcept`
  - `template <typename T> constexpr T Rotl(T value, unsigned count) noexcept`
  - `template <typename T> constexpr T Rotr(T value, unsigned count) noexcept`
- Behavior: template API は unsigned integral 対象。rotate は count を bit幅で剰余化する。
- Failure/edge cases: bit index 範囲外の `HasBit` は `false`。`TryXxx` は範囲外や不正nibbleで
  `false`、出力不変。`TryMask(0)` は 0、`TryMask(BitWidth<T>())` は全bit 1。
- Tests: nibble境界、byte生成、bit 0/最上位/範囲外、mask 0/full/超過、rotate 0/幅以上。
- Do not implement: signed integral 対応、bitset wrapper、BCD固有API。

### numeric Module

- Purpose: alignment、rounding、overflow、narrowing cast の小さい正解を提供。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  overflow と範囲外を戻り値で固定し、手書き算術の未定義動作を避けられる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/numeric/ket_numeric.h`、`modules/numeric/ket_numeric_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API:
  - `template <typename To, typename From> constexpr bool InRange(From value) noexcept`
  - `template <typename T> constexpr T Clamp(T value, T min_value, T max_value) noexcept`
  - `template <typename T> constexpr typename std::make_unsigned<T>::type AbsDiff(T a, T b) noexcept`
  - `template <typename T> constexpr bool TryDivideRoundUp(T value, T divisor, T& out) noexcept`
  - `template <typename T> constexpr bool TryAlignUp(T value, T alignment, T& out) noexcept`
  - `template <typename T> constexpr bool TryAlignDown(T value, T alignment, T& out) noexcept`
  - `template <typename T> constexpr bool TryCheckedAdd(T a, T b, T& out) noexcept`
  - `template <typename T> constexpr bool TryCheckedSub(T a, T b, T& out) noexcept`
  - `template <typename T> constexpr bool TryCheckedMul(T a, T b, T& out) noexcept`
  - `template <typename T> constexpr T SaturatingAdd(T a, T b) noexcept`
  - `template <typename T> constexpr T SaturatingSub(T a, T b) noexcept`
  - `template <typename To, typename From> constexpr bool TryCheckedCast(From value, To& out) noexcept`
- Behavior: alignment と divide-round-up は unsigned integral のみ。checked arithmetic は integral
  型を対象にし、signed overflow を発生させない実装にする。
- Failure/edge cases: `alignment == 0`、`divisor == 0`、overflow、cast範囲外は `false`。
  `Clamp` は `min_value <= max_value` を precondition。
- Tests: align 0/1/exact/overflow、divide 0/1/exact、checked add/sub/mul の min/max、
  cast 255/256、signed境界。
- Do not implement: arbitrary precision、numeric framework、C++17 optional convenience の初回追加。

### endian Module

- Purpose: byte order の読み書きを unaligned access や strict aliasing に頼らず安全に行う。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  endian と unaligned access の意図を名前に出し、strict aliasing 依存を避けられる。
  非推奨版 なし。非推奨理由: C++20 `std::endian` は判定であり、byte列読み書きの直接代替ではない。
- Drop-in files: `modules/endian/ket_endian.h`、`modules/endian/ket_endian.cpp`、
  `modules/endian/ket_endian_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `ByteSwap16`, `ByteSwap32`, `ByteSwap64`, `LoadBe16`, `LoadBe32`,
  `LoadBe64`, `LoadLe16`, `LoadLe32`, `LoadLe64`, `StoreBe16`, `StoreBe32`,
  `StoreBe64`, `StoreLe16`, `StoreLe32`, `StoreLe64`, `TryLoadBe16`, `TryLoadBe32`,
  `TryLoadBe64`, `TryLoadLe16`, `TryLoadLe32`, `TryLoadLe64`, `TryStoreBe16`,
  `TryStoreBe32`, `TryStoreBe64`, `TryStoreLe16`, `TryStoreLe32`, `TryStoreLe64`。
- Behavior: `LoadXxx`/`StoreXxx` は pointer が十分な長さの buffer を指すことを precondition。
  `TryXxx` は null、size不足を `false` で扱う。plain と `Try` は 16/32/64 すべてで対称に揃える。
- Failure/edge cases: `TryLoadXxx` は失敗時に出力不変。`TryStoreXxx` は失敗時に buffer 不変。
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
- Public API: `HexCase`, `HexFormatOptions`, `ToHexString`, `BytesToHex`, `HexToBytes`,
  `HexDump(const std::uint8_t*, std::size_t)`, `HexDump(const void*, std::size_t)`。
- Behavior: `BytesToHex` は upper/lower と任意 separator を指定可能。`HexToBytes` は hex digit と
  ASCII whitespace を受け付け、完全な偶数桁だけ変換する。`HexDump` は offset、hex bytes、
  ASCII preview の固定形式。
- Failure/edge cases: `BytesToHex(nullptr, 0)` は空文字列。`nullptr` かつ非0 size は precondition
  違反。`HexToBytes` の奇数桁、不正文字は `std::nullopt`。
- Tests: 空入力、lower/upper、separator、ASCII whitespace、不正文字、奇数桁、dump空入力。
- Do not implement: Base64、binary viewer framework、任意 separator の parse 自動対応。

### parse_numeric Module

- Purpose: `std::from_chars` 周辺の境界条件を固定し、数値 parse を短く安全にする。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::from_chars` の完全消費、空白、overflow処理を小さいAPIで固定できる。
  非推奨版 なし。非推奨理由: `std::from_chars` は部品であり、ketの境界方針の直接代替ではない。
- Drop-in files: `modules/parse_numeric/ket_parse_numeric.h`、
  `modules/parse_numeric/ket_parse_numeric_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `TryParseInt<T>`, `TryParseUInt<T>`, `TryParseHex<T>`, `TryParseBool`,
  `ParseInt<T>`, `ParseUInt<T>`, `ParseHex<T>`, `ParseBool`, `ParseIntOr<T>`,
  `ParseUIntOr<T>`。
- Behavior: 成功条件は完全消費。`TryParseInt` は signed integral、`TryParseUInt` は unsigned
  integral。`TryParseHex` は `0x`/`0X` prefix あり/なし両方を許可する。
- Failure/edge cases: 空文字列、leading/trailing whitespace、部分消費、overflow、`"-1"` の
  unsigned parse は失敗。bool は `true`、`false`、`1`、`0` のみで case-sensitive。
- Tests: 0、最大値、overflow、空、`" 1"`、`"1 "`、`"1x"`、hex prefix、bool全候補。
- Do not implement: locale対応、trim込みparse、`yes/no/on/off`、浮動小数点 parse。

### enum_table Module

- Purpose: `enum class` と文字列・整数・flags の変換を table-based で明示する。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と class template argument deduction を使い、table-based変換を短く書ける。
  非推奨版 なし。非推奨理由: 標準ライブラリに enum reflection や文字列変換の直接APIなし。
- Drop-in files: `modules/enum_table/ket_enum_table.h`、
  `modules/enum_table/ket_enum_table_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `ToUnderlying`, `EnumEntry<E>`, `TryEnumName`, `EnumName`, `TryParseEnum`,
  `ParseEnum`, `IsValidEnumValue`, `HasFlag`, `SetFlag`, `ClearFlag`, `AnyFlag`, `AllFlags`。
- Behavior: table は利用者が明示する。name と parse は完全一致。重複 entry は先に出たものを返す。
- Failure/edge cases: unknown enum value、unknown text は失敗。flags は underlying type へ変換して
  bit operation する。
- Tests: known/unknown value、known/unknown text、duplicate table、flags set/clear/has/all/any。
- Do not implement: reflection、case-insensitive parse、enum registration framework。

### container Module

- Purpose: `find`、`end`、erase-remove、default値取得など標準コンテナ利用時の儀式を短くする。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  map/vector の反復的な lookup と erase-remove 手順を標準ライブラリのみで薄く包める。
  非推奨版 なし。非推奨理由: C++20以降の `contains` や `std::erase_if` と一部重なるが、
  module候補全体の直接代替ではない。
- Drop-in files: 未製造。`Contains` の対象範囲、`Append` の self append、C++17 optional
  convenience を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `Contains`, `ContainsKey`, `AtOrNull`, `GetOrDefault`, `GetOrCreate`,
  `IndexOf`, `EraseIf`, `SortUnique`, `Append`。
- Behavior: `AtOrNull` は copy を避ける pointer API。`GetOrDefault` は値を返す。
  `GetOrCreate` は key が無い場合だけ factory を呼ぶ。`EraseIf` は削除件数を返す。
- Failure/edge cases: keyなし、空container、factory例外、重複値、self append 方針を製造前に固定する。
- Tests: keyあり/なし、factory呼び出し回数、EraseIf削除件数、SortUnique、Append。
- Do not implement: 独自container、range framework、`std::erase_if` 完全互換の追求。

### string_ascii Module

- Purpose: ASCII 前提の小さい文字列処理を、Unicode と誤解されない名前で提供する。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` で non-owning な文字列処理を短く扱える。
  非推奨版 なし。非推奨理由: 標準ライブラリに ASCII 限定の trim/case/split 方針を固定した直接APIなし。
- Drop-in files: `modules/string_ascii/ket_string_ascii.h`、
  `modules/string_ascii/ket_string_ascii.cpp`、`modules/string_ascii/ket_string_ascii_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `StartsWith`, `EndsWith`, `ContainsSubstring`, `TrimAscii`, `TrimLeftAscii`,
  `TrimRightAscii`, `RemovePrefix`, `RemoveSuffix`, `SplitView`, `Split`, `Join`,
  `ReplaceAll`, `ToLowerAscii`, `ToUpperAscii`, `EqualsIgnoreCaseAscii`。
- Behavior: trim は ASCII whitespace のみ。`SplitView` は空要素を保持し、戻り値は元文字列を参照する。
  `RemovePrefix`/`RemoveSuffix` は一致しなければ元 view を返す。
- Failure/edge cases: `ReplaceAll(text, "", to)` は precondition 違反。UTF-8 は byte列として保持し、
  ASCII case 変換対象外 byte は変更しない。
- Tests: empty、delimiterなし、leading/trailing delimiter、空要素、ASCII case、UTF-8 byte保持。
- Do not implement: Unicode normalization、locale、regex、巨大 string utility 集。

### scope Module

- Purpose: cleanup 漏れ、早期 return 時の復元漏れを防ぐ。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  RAII cleanup を小さい header-only API として持ち出せる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/scope/ket_scope.h`、`modules/scope/ket_scope_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `ScopeExit<F>`, `MakeScopeExit`, `RestoreOnExit<T>`, `MakeRestoreOnExit`。
- Behavior: `ScopeExit` は move-only。move 後 source は inactive。`Dismiss()` 後は callback を呼ばない。
  `RestoreOnExit` は構築時の値へ destructor で復元する。
- Failure/edge cases: destructor から例外を外へ出さない。callback や復元代入が例外を投げた場合は
  `std::terminate`。
- Tests: scope exit、dismiss、move、二重実行なし、restore、early return 相当。
- Do not implement: `Finally` alias、scope success/failure 分岐、defer macro。

### byte_reader Module

- Purpose: 固定 buffer からの逐次読み取りを、offset/remaining を明示しながら安全に行う。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  buffer lifetime と offset 更新条件を明示し、size不足時の状態不変を固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/byte_reader/ket_byte_reader.h`、
  `modules/byte_reader/ket_byte_reader.cpp`、`modules/byte_reader/ket_byte_reader_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `ByteReader`, `Size`, `Offset`, `Remaining`, `Empty`, `Skip`, `ReadU8`,
  `ReadBe16`, `ReadBe32`, `ReadLe16`, `ReadLe32`, `ReadBytes`。
- Behavior: `data == nullptr && size == 0` は有効な空 reader。`data == nullptr && size > 0` は
  invalid reader。成功時だけ offset を進める。`ReadBytes` は non-owning pointer を返す。
- Failure/edge cases: 失敗時は offset と出力不変。元 buffer lifetime は利用者責任。
- Tests: 空buffer、ぴったり読み切り、1 byte不足、失敗時offset不変、BE/LE値、invalid reader。
- Do not implement: endian module 依存、owning buffer、protocol parser。

### byte_writer Module

- Purpose: fixed buffer への逐次書き込みを overflow なしで行う。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  fixed buffer 書き込みのsize確認とoffset更新条件を小さいAPIで固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/byte_writer/ket_byte_writer.h`、
  `modules/byte_writer/ket_byte_writer.cpp`、`modules/byte_writer/ket_byte_writer_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `ByteWriter`, `Size`, `Offset`, `Remaining`, `Full`, `Skip`, `WriteU8`,
  `WriteBe16`, `WriteBe32`, `WriteLe16`, `WriteLe32`, `WriteBytes`。
- Behavior: `data == nullptr && size == 0` は有効な空 writer。`data == nullptr && size > 0` は
  invalid writer。成功時だけ offset と buffer を更新する。
- Failure/edge cases: 失敗時は offset と既存 buffer を変更しない。サイズ確認後に書き込む。
- Tests: 空buffer書き込み失敗、ぴったり書き切り、1 byte不足、失敗時offset/buffer不変、BE/LE値。
- Do not implement: endian module 依存、可変長 vector builder、stream writer。

### bytes_builder Module

- Purpose: 可変長 payload を `std::vector<std::uint8_t>` へ読みやすく組み立てる。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::vector<std::uint8_t>` を所有するpayload構築を、標準ライブラリのみで薄く包める。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未製造。fluent member API と free function API の採用範囲を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `BytesBuilder`, `BytesBuilder::U8`, `Be16`, `Be32`, `Le16`, `Le32`,
  `Bytes`, `StringAscii`, `View`, `Build`, `Clear`, `AppendU8`, `AppendBe16`, `AppendBe32`,
  `AppendLe16`, `AppendLe32`, `AppendBytes`。
- Behavior: fluent API は `*this` を返す。free function は既存 vector へ追加する。`Build() &&` は
  内部 vector を move して返す。
- Failure/edge cases: allocation があるため `noexcept` なし。`Bytes(nullptr, 0)` は no-op。
  `Bytes(nullptr, size > 0)` は precondition 違反。
- Tests: U8/BE/LE append、reserve constructor、Clear、View、Build move、null+0。
- Do not implement: serializer framework、field schema、checksum、protocol固有処理。

### date Module

- Purpose: 日付・時刻の小さい妥当性判定を calendar framework なしで提供する。
- C++ version: 最小要件 C++11。推奨版 C++11〜17。推奨理由:
  C++17以前では標準calendar APIがなく、Gregorian妥当性判定を小さく持ち出せる。
  非推奨版 C++20以降。非推奨理由: C++20 `std::chrono` calendar を使える環境では、
  日付表現と妥当性判定を標準型へ寄せられる。
- Drop-in files: 未製造。C++20 `std::chrono` calendar との境界と time API の切り方を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `IsLeapYear`, `IsValidMonth`, `TryDaysInMonth`, `IsValidDate`, `IsValidTime`,
  `IsValidTimeMillis`。
- Behavior: Gregorian calendar、`year >= 1`。timezone と leap second は扱わない。
- Failure/edge cases: month 0/13、day 0、月末超過、`second >= 60`、`millisecond >= 1000`。
- Tests: 2000 leap、1900 not leap、2024-02-29、2023-02-29、month境界、hour 24。
- Do not implement: timezone、calendar conversion、date arithmetic、BCD date。

### deadline Module

- Purpose: `steady_clock` ベースの timeout と elapsed time 計算を読みやすくする。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  `steady_clock` と timeout の扱いを明示し、`system_clock` 混在を避けられる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/deadline/ket_deadline.h`、`modules/deadline/ket_deadline.cpp`、
  `modules/deadline/ket_deadline_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `Stopwatch::StartNew`, `Stopwatch::Restart`, `Stopwatch::Elapsed`,
  `Stopwatch::ElapsedMillis`, `Deadline::After`, `Deadline::At`, `Deadline::Expired`,
  `Deadline::Remaining`, `Deadline::TimePoint`。
- Behavior: `steady_clock` のみ。`Remaining()` は期限切れなら zero を返す。負 timeout は即 expired。
- Failure/edge cases: clock差し替えは初回なし。sleep依存テストは許容誤差を持たせる。
- Tests: zero timeout、future deadline、Remaining非負、Restart、At。
- Do not implement: `system_clock` deadline、scheduler、timer thread。

### cli Module

- Purpose: 小さい社内 CLI で `argc/argv` の option 取得を短くする。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` で `argv` lifetime に依存する値を明示しながら扱える。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未製造。option値 parse API と他 module 非依存の実装方針を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `ArgvView`, `ArgvView::Size`, `At`, `ProgramName`, `HasOption`,
  `GetOption`, `GetOptionOr`, `OptionUInt<T>`, `Positional`。
- Behavior: `--key value` と `--key=value` を両方許す。`--flag` は `HasOption`。
  duplicated option は最初を返す。戻り値の `std::string_view` は `argv` lifetime に依存する。
- Failure/edge cases: `GetOption(args, "--id")` で次要素が別 option なら失敗。shell quote 展開なし。
- Tests: `--help`、`--id 123`、`--id=123`、missing value、duplicate、positional。
- Do not implement: subcommand framework、help generator、shell parser、config file。

### byte_view Module

- Purpose: C++11〜17 向けの薄い non-owning byte span。
- C++ version: 最小要件 C++11。推奨版 C++11〜17。推奨理由:
  C++17以前で non-owning byte span の寿命と境界確認を小さく表現できる。
  非推奨版 C++20以降。非推奨理由: C++20以降は `std::span` を優先できる。
- Drop-in files: 未製造。method/free function 境界、invalid view の表現、mutable API を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `ByteView`, `MutableByteView`, `MakeByteView`, `MakeMutableByteView`。
- Behavior: `Data`, `Size`, `Empty`, `TryAt`, `TrySubView` を提供する。view は所有権を持たず、
  元buffer lifetime に依存する。
- Failure/edge cases: `nullptr+0` は空 view。`nullptr+非0` は invalid view とし、access は失敗。
  subview 範囲超過は失敗。
- Tests: default、empty、invalid、bounds、subview、mutable set。
- Do not implement: `std::span` 置き換え、iterator 完備、owning bytes。

### utf8 Module

- Purpose: UTF-8 検査を小さく隔離する。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で UTF-8 検査結果と失敗位置を小さく扱える。
  非推奨版 なし。非推奨理由: 標準ライブラリに UTF-8 byte列検証の直接APIなし。
- Drop-in files: `modules/utf8/ket_utf8.h`、`modules/utf8/ket_utf8.cpp`、
  `modules/utf8/ket_utf8_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `Utf8ValidationResult`, `IsAscii`, `ValidateUtf8`, `IsUtf8`, `Utf8Length`。
- Behavior: `Utf8Length` は code point 数を返す。invalid UTF-8 は `std::nullopt`。
- Failure/edge cases: overlong、surrogate、truncated sequence、continuation byte 不正、範囲外 code
  point は invalid。`error_offset` は最初の不正 byte。
- Tests: ASCII、2/3/4 byte、空、overlong、truncated、surrogate、bad continuation。
- Do not implement: normalization、case folding、locale、encoding conversion、grapheme cluster。

### file Module

- Purpose: ファイル全読み/全書きの小さい定型処理を安全にまとめる。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::filesystem` と `std::string_view` を使い、path と bytes/text の扱いを標準型へ寄せられる。
  非推奨版 なし。非推奨理由: 標準ライブラリだけでは全読み/全書きの失敗方針が長くなりやすい。
- Drop-in files: 未製造。`std::error_code*` の optional detail 形、例外有無、短い write の扱いを固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `TryReadAllText`, `TryReadAllBytes`, `WriteAllText`, `WriteAllBytes`,
  `FileExists`, `DirectoryExists`, `FileSize`。
- Behavior: `std::filesystem::path` を受ける。text encoding は変換せず、bytes をそのまま
  `std::string` に読む。optional detail として `std::error_code*` を受ける。
- Failure/edge cases: not found、permission、directory path、短い write、巨大 file、error pointer null。
- Tests: 空file、text、binary、missing file、directory、error_code設定。
- Do not implement: recursive copy、watcher、encoding conversion、path normalization framework。

### io_stream Module

- Purpose: stream の確実な読み書きと stream state 保存を小さく提供する。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  iostream の short read/write と state復元を小さいAPIで固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/io_stream/ket_io_stream.h`、`modules/io_stream/ket_io_stream.cpp`、
  `modules/io_stream/ket_io_stream_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `ReadExactly`, `WriteAll`, `ReadLineTrimmedAscii`, `StreamStateSaver`。
- Behavior: `ReadExactly` は requested size を読み切った場合のみ `true`。`StreamStateSaver` は
  flags、precision、fill を保存・復元する。
- Failure/edge cases: short read/write、stream error、null data with nonzero size は precondition 違反。
- Tests: exact read、short read、write、state復元、trimmed line、EOF。
- Do not implement: async I/O、filesystem wrapper、binary serialization。

### format_value Module

- Purpose: 診断用の短い文字列化候補。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  診断用の固定表記を標準ライブラリのみで小さく提供できる。
  非推奨版 なし。非推奨理由: `std::format` は書式化部品であり、ketの固定診断表記の直接代替ではない。
- Drop-in files: 未製造。仕様分割後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `FormatBool`, `FormatSignedDecimal`, `FormatUnsignedDecimal`,
  `ToBinaryString`, `FormatByteCount`, `FormatDuration`。
- Behavior: 製造前に byte 単位表記、duration 単位、binary prefix、幅、符号の扱いを固定する。
- Failure/edge cases: allocation 例外あり。duration の負値、極小/極大値、丸めを仕様化する。
- Tests: 単位境界、min_width、負duration、0、最大値。
- Do not implement: `std::format` 再実装、logging framework、locale対応、`ToHexString`。

### algorithm_range Module

- Purpose: iterator pair を毎回書く煩わしさを減らす候補。
- C++ version: 最小要件 C++11。推奨版 C++11〜17。推奨理由:
  C++17以前で range object から standard algorithm を呼ぶ儀式を短くできる。
  非推奨版 C++20以降。非推奨理由: C++20以降は `std::ranges` を優先できる。
- Drop-in files: 未製造。採用 API を絞ってから決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `AllOf`, `AnyOf`, `NoneOf`, `CountIf`, `FindIf`, `ForEachIndex`。
- Behavior: range は `std::begin`/`std::end` が使える object。iterator pair を受ける overload は初回なし。
- Failure/edge cases: predicate 例外は伝播。空rangeの standard algorithm と同じ真理値。
- Tests: empty、all/any/none、mutable find、index順。
- Do not implement: `std::algorithm` の単なる全量別名、ranges framework。

### memory Module

- Purpose: alignment と object representation 読み取りを小さく扱う候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  pointer alignment と object representation の意図を小さいAPIへ分離できる。
  非推奨版 なし。非推奨理由: secure zero と pointer alignment の直接代替は標準だけでは不足する。
- Drop-in files: 未製造。`SecureZeroMemory` の保証範囲を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `IsAligned`, `TryAlignUpPtr`, `TryAlignDownPtr`, `ZeroMemory`,
  `SecureZeroMemory`, `ObjectBytes`, `ObjectByteSize`。
- Behavior: pointer alignment と byte表現の読み取りに限定する。
- Failure/edge cases: `alignment == 0`、非power-of-two alignment、null pointer、secure zero最適化除去対策。
- Tests: aligned/unaligned、alignment 0、null+0、object byte size、zeroing。
- Do not implement: object lifetime 操作、placement new helper、allocator、type punning。

### pointer Module

- Purpose: null と ownership 誤解を減らす候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  null許容性と所有権の有無を型名や関数名で明示できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未製造。`NotNull` の失敗方針を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `NotNull<Ptr>`, `LockWeak`, `GetOrNull`, `AddressOf`。
- Behavior: `NotNull` は所有権を持たない non-null wrapper。`LockWeak` は expired 時に空
  `std::shared_ptr`。
- Failure/edge cases: `NotNull(nullptr)` を assert/terminate/throw のどれで扱うか製造前に固定する。
- Tests: nullptr、dereference、operator->、weak alive/expired、AddressOf overloaded `operator&`。
- Do not implement: `Owner<T*>`、smart pointer 再実装、lifetime tracking。

### testing_bytes Module

- Purpose: byte列比較の GoogleTest failure message を読みやすくする。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  GoogleTest v1.17.0 のC++17要件に合わせ、byte列差分を読みやすく表示できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/testing_bytes/ket_testing_bytes.h`、
  `modules/testing_bytes/ket_testing_bytes.cpp`、
  `modules/testing_bytes/ket_testing_bytes_test.cpp`。
- Dependencies: GoogleTest と標準ライブラリ。他の ket module への依存なし。
- Public API: `BytesEq`, `HexEq`。
- Behavior: `::testing::AssertionResult` を返し、mismatch offset、actual/expected hex を表示する。
- Failure/edge cases: null+0 は空として許可。null+非0 は failure。`HexEq` の不正hexは failure。
- Tests: equal、different size、different byte、null+0、null+非0、invalid hex。
- Do not implement: production library dependency、matcher framework、snapshot framework。

### semantic_version Module

- Purpose: semver-like な `major.minor.patch` 値の parse/format/compare。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で parse失敗と比較を小さく固定できる。
  非推奨版 なし。非推奨理由: 標準ライブラリに semantic version の直接APIなし。
- Drop-in files: `modules/semantic_version/ket_semantic_version.h`,
  `modules/semantic_version/ket_semantic_version.cpp`,
  `modules/semantic_version/ket_semantic_version_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `SemanticVersion`, `ParseSemanticVersion`, `FormatSemanticVersion`,
  `CompareSemanticVersion`。
- Behavior: 初回は `major.minor.patch` のみ。各要素は `std::uint32_t`。`0` 自体は許可し、
  複数桁の leading zero は失敗。
- Failure/edge cases: 空、要素不足/過多、非数字、overflow、`01.2.3`。
- Tests: `0.0.0`、normal parse、format、compare major/minor/patch、overflow、leading zero。
- Do not implement: prerelease、build metadata、range constraint、package manager semantics。

### ipv4 Module

- Purpose: IPv4 dotted decimal の parse/format と BE 32bit 変換。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で dotted decimal の失敗条件を明確に扱える。
  非推奨版 なし。非推奨理由: 標準ライブラリに IPv4 dotted decimal parse/format の直接APIなし。
- Drop-in files: `modules/ipv4/ket_ipv4.h`、`modules/ipv4/ket_ipv4.cpp`、
  `modules/ipv4/ket_ipv4_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `Ipv4Address`, `ParseIpv4Address`, `FormatIpv4Address`, `Ipv4AddressToU32Be`,
  `U32BeToIpv4Address`。
- Behavior: dotted decimal のみ。leading zero は失敗。`Ipv4AddressToU32Be` は network byte order の整数表現。
- Failure/edge cases: octet > 255、負値、空要素、要素不足/過多、空白、leading zero。
- Tests: `0.0.0.0`、`255.255.255.255`、format、u32 roundtrip、不正入力。
- Do not implement: IPv6、CIDR、DNS、port、socket address。

### port Module

- Purpose: TCP/UDP port番号の小さい値型、parse、format。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で範囲外や不正文字を明確に扱える。
  非推奨版 なし。非推奨理由: 標準ライブラリに port番号の値型や parse/format の直接APIなし。
- Drop-in files: `modules/port/ket_port.h`、`modules/port/ket_port.cpp`、
  `modules/port/ket_port_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `Port`, `TryMakePort`, `ParsePort`, `FormatPort`。
- Behavior: port値は0〜65535を許可する。0は OS による自動割り当て用途を表せる有効値とし、
  呼び出し側が必要なら別途禁止する。`ParsePort` は10進表記のみを完全消費し、format は leading
  zero なしの10進文字列を返す。
- Failure/edge cases: 空文字列、leading/trailing whitespace、`+`/`-`、不正文字、65535超過、
  複数桁の leading zero は失敗。
- Tests: 0、1、65535、65536、空、空白、符号、不正文字、leading zero、format。
- Do not implement: socket address、service name 解決、protocol別port型、ephemeral port 採番。

### mac_address Module

- Purpose: MAC address の parse/format。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で区切り文字とhex byteの失敗条件を固定できる。
  非推奨版 なし。非推奨理由: 標準ライブラリに MAC address parse/format の直接APIなし。
- Drop-in files: `modules/mac_address/ket_mac_address.h`,
  `modules/mac_address/ket_mac_address.cpp`, `modules/mac_address/ket_mac_address_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `MacAddress`, `ParseMacAddress`, `FormatMacAddress`, `FormatMacAddressUpper`。
- Behavior: `AA:BB:CC:DD:EE:FF` と `aa-bb-cc-dd-ee-ff` を許可する。区切り文字の混在は失敗。
- Failure/edge cases: byte数不足/過多、不正hex、区切り不足、混在区切り、Cisco形式は失敗。
- Tests: upper/lower、colon、hyphen、format lower/upper、不正hex、短い/長い入力。
- Do not implement: Cisco形式、OUI lookup、network device model。

### function Module

- Purpose: callable/visitor の小さい儀式を減らす。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  variadic using declaration で visitor overload set を小さく表現できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: `modules/function/ket_function.h`、`modules/function/ket_function_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `Overload<Fs...>`, `MakeOverload`, `Noop`。
- Behavior: `Overload` は複数 callable を値として保持し、`using Fs::operator()...` で overload set
  を作る。`Noop` は任意引数を無視し `noexcept`。
- Failure/edge cases: callable の copy/move 制約は型に従う。呼び出し例外は handler から伝播。
- Tests: `std::visit`、複数型、戻り値、Noop、copy/move。
- Do not implement: `FunctionRef`、`std::function` wrapper、signal/slot framework。

### variant_match Module

- Purpose: `std::variant` visitor 補助候補。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::variant` と visitor補助を標準ライブラリのみで薄く包める。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未製造。`function` module との重複方針を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `Match`, `Holds`, `GetIf`。
- Behavior: `Match` は `std::visit` と overload helper を短くする。
- Failure/edge cases: reference category、const、戻り型、exception propagation を製造前に固定する。
- Tests: const/non-const、lvalue/rvalue、void/non-void、exception。
- Do not implement: variant framework、pattern matching DSL、`function` module への依存。

### optional_ext Module

- Purpose: `std::optional` の小さい合成。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::optional` の小さい合成を、標準の値保持制約に従って扱える。
  非推奨版 なし。非推奨理由: APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない。
- Drop-in files: 未製造。C++23 monadic API との重複、optional-like 戻り型、参照保持方針を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `MapOptional`, `AndThen`, `ValueOrEval`, `HasValueAll`。
- Behavior: empty の場合は mapper/factory を必要時以外呼ばない。`AndThen` の戻り型は mapper が返す
  optional-like 値そのもの。
- Failure/edge cases: mapper 例外は伝播。reference wrapper など値保持型は標準 optional の制約に従う。
- Tests: value/empty、mapper呼び出し回数、factory遅延、複数optional、戻り型。
- API別標準代替:
  - `MapOptional`: C++23 `std::optional::transform`。
  - `AndThen`: C++23 `std::optional::and_then`。
  - `ValueOrEval`: 直接代替なし。factory遅延評価を固定する場合のみ採用。
  - `HasValueAll`: 直接代替なし。複数 optional の可読性改善が必要な場合のみ採用。
- Do not implement: `Result`、`StatusOr`、monad framework、error accumulation。

### contract Module

- Purpose: precondition、postcondition、invariant を名前で明示する候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  契約違反時のプロジェクト方針を小さいAPIへ閉じ込められる。
  非推奨版 なし。非推奨理由: C++ contracts は標準化状況と利用可能性が安定していない。
- Drop-in files: 未製造。違反時ポリシーを固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `Expects`, `Ensures`, `AssertInvariant`, `RequireNonNull`, `CheckBounds`,
  `RequireInRange`。
- Behavior: 契約違反時に assert、abort、terminate、NDEBUG 連動のどれを採るか製造前に固定する。
- Failure/edge cases: release build、side-effect条件式、nullptr、bounds。
- Tests: policyに応じた death test、valid path、bounds。
- Do not implement: macro 大量追加、exception hierarchy、debug logging framework。

### c_interop Module

- Purpose: C API 境界の errno、C buffer、handle cleanup 事故を減らす候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  errno保存、C buffer copy、handle cleanup の事故をC API境界に閉じ込められる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未製造。`UniqueHandle` の invalid value 方針を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `ErrnoGuard`, `CopyToCBuffer`, `CopyBytesToCBuffer`, `UniqueHandle`。
- Behavior: `ErrnoGuard` は構築時 errno を保存し destructor で復元する。C buffer copy は null終端と
  size不足を明確に扱う。
- Failure/edge cases: `dst_size == 0`、null pointer、src truncation、deleter noexcept、handle release。
- Tests: errno復元、copy成功/不足、bytes copy、UniqueHandle reset/release。
- Do not implement: OS handle 専用 wrapper、C API framework、ownership annotation体系。

### platform_error Module

- Purpose: errno/Windows error/environment variable の小さい platform 補助候補。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  platform API の差分を隠しすぎず、標準文字列で結果を扱える。
  非推奨版 なし。非推奨理由: 標準ライブラリだけでは errno/Windows error message の扱いが不十分。
- Drop-in files: 未製造。platform別仕様と検証環境を固定後に決定。
- Dependencies: 標準ライブラリと platform API。他の ket module への依存なし。
- Public API: 候補は `ErrnoMessage`, `_WIN32` 限定 `WindowsErrorMessage`, `GetLastErrorCode`,
  `EnvironmentVariable`。
- Behavior: POSIX と Windows の差を隠しすぎず、OS専用APIは `#ifdef` と Doxygen に明記する。
- Failure/edge cases: unknown errno、missing env、encoding、Windows wide/narrow 変換。
- Tests: known errno、missing env、present env、Windows conditional compile。
- Do not implement: cross-platform error framework、logging、localization。

### state_table Module

- Purpose: 小さい状態遷移表 lookup。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::optional` で未定義遷移を明確にし、table-driven lookup を小さく書ける。
  非推奨版 なし。非推奨理由: 標準ライブラリに状態遷移表 lookup の直接APIなし。
- Drop-in files: `modules/state_table/ket_state_table.h`,
  `modules/state_table/ket_state_table_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `Transition<State, Event>`, `IsValidTransition`, `NextState`。
- Behavior: table は利用者が明示する。先に一致した transition を採用する。
- Failure/edge cases: 未定義遷移は `false` または `std::nullopt`。duplicate は先勝ち。
- Tests: known transition、unknown、duplicate、enum class state/event。
- Do not implement: FSM framework、entry/exit action、guard expression DSL。

### cache_once Module

- Purpose: once/lazy value の候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  lazy value の thread-safety と例外後状態を局所的に固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未製造。thread-safety と例外後状態を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `Lazy<T>`, `OnceValue<T>`。
- Behavior: 初回候補は non-thread-safe。`GetOrCreate`/`Get` は値が無い場合だけ factory を呼ぶ。
- Failure/edge cases: factory例外後に値を保持するかどうか、Reset有無、copy/move制約を固定する。
- Tests: factory呼び出し回数、Reset、例外後状態、move-only value。
- Do not implement: thread-safe cache、global registry、memoization framework。

### serialization_tlv Module

- Purpose: length-prefix/TLV の小さい serialize/parse 候補。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view`/`std::optional` 相当の失敗表現でwire format境界を固定できる。
  非推奨版 なし。非推奨理由: 標準ライブラリに TLV encode/decode の直接APIなし。
- Drop-in files: 未製造。wire format を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `TlvView`, `EncodeLengthPrefixed`, `TryDecodeLengthPrefixed`,
  `EncodeTlv`, `TryDecodeTlv`。
- Behavior: field 単位で encode/decode する。`TlvView` は入力bufferへの non-owning view。
- Failure/edge cases: length幅、type幅、endian、最大長、短い入力、overflow を製造前に固定する。
- Tests: roundtrip、short input、length過大、empty value、multiple records は初回対象外。
- Do not implement: struct丸ごとbytes化、schema language、protocol framework。

### tuple Module

- Purpose: tuple の小さい反復・変換補助。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::apply` と fold expression を使い、tuple走査を小さく実装できる。
  非推奨版 なし。非推奨理由: 標準ライブラリだけでは tuple要素ごとの副作用呼び出しが冗長。
- Drop-in files: `modules/tuple/ket_tuple.h`、`modules/tuple/ket_tuple_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `TupleForEach`, `TupleTransform`。
- Behavior: `TupleForEach` は index順に callable を呼ぶ。`TupleTransform` は各要素に callable を適用した
  tuple を返す。
- Failure/edge cases: callable 例外は伝播。empty tuple、const tuple、reference要素を扱う。
- Tests: empty、heterogeneous、const、reference、戻りtuple型、呼び出し順。
- Do not implement: pair helper、tuple DSL、reflection。

### build_config Module

- Purpose: feature detection macro 候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  compiler/OS/feature macro の差分を小さい範囲へ閉じ込められる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未製造。macro 汚染と必要性を再確認後に決定。
- Dependencies: 標準ライブラリと compiler predefined macros。他の ket module への依存なし。
- Public API: 候補は `KET_CXX_VERSION`, `KET_HAS_STD_OPTIONAL`, `KET_HAS_STD_STRING_VIEW`,
  `KET_HAS_STD_SPAN`, `KET_HAS_STD_FORMAT`, `KET_COMPILER_*`, `KET_OS_*`。
- Behavior: feature detection だけを行い、policy を持たない。
- Failure/edge cases: compiler差、standard library feature macro差、include順。
- Tests: compile-only checks、expected macro shape。
- Do not implement: config framework、global behavior switch、module間の必須依存化。

### math_small Module

- Purpose: 補間、角度、byte単位変換など小さい数学候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  小さい数学処理の丸め、overflow、単位名をAPIで固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未製造。型制約と丸め方を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `Lerp`, `DegreesToRadians`, `RadiansToDegrees`, `NearlyEqual`, `KiBToBytes`,
  `MiBToBytes`, `BytesToKiB`, `BytesToMiB`。
- Behavior: statistics や units framework へ広げない。constexpr 可能な範囲に留める。
- Failure/edge cases: integer overflow、floating precision、NaN、epsilon負値を製造前に固定する。
- Tests: endpoints、midpoint、angle roundtrip、epsilon、large byte values。
- Do not implement: units framework、matrix/vector math、statistics。

### language Module

- Purpose: C++言語の小さい儀式（未使用無視、到達不能、配列長、const化）を名前付き API にする候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  C++11/14の欠落や冗長な言語儀式を小さいAPIで名前付けできる。
  非推奨版 なし。非推奨理由: APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない。
- Drop-in files: 未製造。採用 API と標準代替の登場版を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `IgnoreUnused`, `Unreachable`, `ArraySize`, `AsConst`, `Identity`。
- Behavior: 純粋な言語儀式に限定し、状態やコピー・ムーブ制御は `object` module に分ける。
  `std::exchange` のような既存標準と完全に重なる別名は採用しない。
- Failure/edge cases: `Unreachable` 到達時の未定義動作、空配列、参照寿命を製造前に固定する。
- Tests: unused 無視、配列長、const 化、C++11 compile-only、Unreachable death/assert。
- API別標準代替:
  - `IgnoreUnused`: C++17 `[[maybe_unused]]` と一部重複。式の明示破棄用途は製造前に要確認。
  - `ArraySize`: C++17 `std::size`。
  - `AsConst`: C++17 `std::as_const`。
  - `Identity`: C++20 `std::type_identity` と一部重複。関数APIとして採用するか要確認。
  - `Unreachable`: C++23 `std::unreachable`。
- Do not implement: macro 大量追加、attribute framework、`std::xxx` の単なる別名。

### object Module

- Purpose: copy/move/regular 型の儀式（コピー禁止、move 専用、move 後リセット）を mixin と
  helper で明示する候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  copy/move意図を型定義の近くへ集約できる。非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未製造。defaulted comparison との衝突回避方針を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `NonCopyable`, `NonMovable`, `MovableOnly`, `ResetOnMove<T>`, `SwapAndReset`。
- Behavior: 継承用 mixin と小さい helper に限定する。NonCopyable/NonMovable はコピー・ムーブの
  意図を型に出す。`ResetOnMove` は move 後に source を既定値へ戻す。
- Failure/edge cases: empty base optimization、自己 swap、move 後の状態、C++20 defaulted
  comparison との相互作用を製造前に固定する。
- Tests: copy 禁止 compile-fail、move、reset、自己 swap、空 base size。
- Do not implement: regular type framework、smart pointer 再実装、`=delete` で足りる範囲の wrapper。

### meta Module

- Purpose: C++11/14 欠落 type traits の小補助候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  C++11/14で不足する小さいtraitsを局所的に補える。
  非推奨版 なし。非推奨理由: APIごとに標準代替の登場版が異なるため、module単位では非推奨にしない。
- Drop-in files: 未製造。標準代替との重複を整理後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `RemoveCvref`, `TypeIdentity`, `AlwaysFalse`, `VoidT`。
- Behavior: template error を読みやすく保つ小さい alias/type に限定する。
- Failure/edge cases: C++17 以降の標準代替、SFINAE 使用時の diagnostics。
- Tests: alias型一致、C++11 compile-only、SFINAE。
- API別標準代替:
  - `VoidT`: C++17 `std::void_t`。
  - `RemoveCvref`: C++20 `std::remove_cvref`。
  - `TypeIdentity`: C++20 `std::type_identity`。
  - `AlwaysFalse`: 直接代替なし。template diagnostics改善が必要な場合のみ採用。
- Do not implement: meta programming framework、concepts 代替体系、難読化する traits。

### concurrency_small Module

- Purpose: join 忘れと future timeout 確認程度の局所補助候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  thread join と future ready 判定の小さい儀式を局所化できる。
  非推奨版 なし。非推奨理由: APIごとに標準代替の有無が異なるため、module単位では非推奨にしない。
- Drop-in files: 未製造。destructor 方針とテスト方法を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `JoiningThread`, `FutureReady`。
- Behavior: `JoiningThread` は所有する `std::thread` を destructor で join する候補。
  `FutureReady` は zero timeout wait で ready 判定する。
- Failure/edge cases: self-join、move代入時の既存thread、join例外、deferred future。
- Tests: default、joinable、move、ready/not ready、deferred。
- API別標準代替:
  - `JoiningThread`: C++20 `std::jthread`。
  - `FutureReady`: 直接代替なし。`future.wait_for(0)` の結果判定を名前にする場合のみ採用。
- Do not implement: thread pool、executor、lock framework、cancellation framework。

### uuid Module

- Purpose: UUID の parse/format。generation は扱わない。
- C++ version: 最小要件 C++17。推奨版 C++17以降。推奨理由:
  `std::string_view` と `std::optional` で canonical UUID の失敗条件を固定できる。
  非推奨版 なし。非推奨理由: 標準ライブラリに UUID parse/format の直接APIなし。
- Drop-in files: `modules/uuid/ket_uuid.h`、`modules/uuid/ket_uuid.cpp`、
  `modules/uuid/ket_uuid_test.cpp`。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: `Uuid`, `ParseUuid`, `FormatUuid`。
- Behavior: canonical hyphen形式 `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx` を parse/format する。
  hex は upper/lower を受け付け、format は lower-case 固定。
- Failure/edge cases: 長さ不一致、hyphen位置不一致、不正hex、brace付き形式は失敗。
- Tests: zero uuid、normal uuid、upper input、bad length、bad hyphen、bad hex、format。
- Do not implement: UUID generation、version/variant validation、URN形式、OS乱数。

### color_rgb Module

- Purpose: RGB小値型候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  RGB小値型の許容表記と不正hexを小さいAPIで固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未定。仕様分割後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `ParseColorRgb`, `FormatColorRgbHex`。
- Behavior: 製造前に `#RRGGBB` だけにするか、`RRGGBB` も許すか、alpha を除外するか固定する。
- Failure/edge cases: 不正長、不正hex、prefix有無、case、alpha。
- Tests: black/white、lower/upper、不正長、不正文字。
- Do not implement: color space 変換、CSS color name、alpha、theme system。

### percent Module

- Purpose: percent小値型候補。
- C++ version: 最小要件 C++11。推奨版 C++11以降。推奨理由:
  percent小値型の範囲、丸め、NaN方針を局所的に固定できる。
  非推奨版 なし。非推奨理由: なし。
- Drop-in files: 未定。丸めと範囲を固定後に決定。
- Dependencies: 標準ライブラリのみ。他の ket module への依存なし。
- Public API: 候補は `Percent::FromRatio`, `ClampPercent`。
- Behavior: 0〜100 を値範囲にするか、0.0〜1.0 ratio を内部表現にするか製造前に固定する。
- Failure/edge cases: 負値、100超過、NaN、丸め、整数/浮動小数点入力。
- Tests: 0、100、負値、100超過、ratio境界、clamp。
- Do not implement: units framework、progress UI、format localization。

## 6. recipes 仕様カード

### binary_payload Recipe

- Purpose: BCD、endian、byte_writer、hex を組み合わせた電文構築例。
- C++ version: mixed。利用する実moduleの要件に従う。
- Drop-in files: `recipes/binary_payload/README.md`、
  `recipes/binary_payload/binary_payload_example.cpp`。
- Dependencies: 実装済み ket module と標準ライブラリ。
- Public API: なし。recipe code のみ。
- Behavior: command byte、BE sequence、BCD date、単純 checksum、HexDump 診断を例示する。
- Failure/edge cases: module API の失敗値処理を例内で示す。
- Tests: example が build でき、代表payloadを期待hexと比較できること。
- Do not implement: 新規 module API、業務固有 protocol、framework。

### command_parser Recipe

- Purpose: CLI、parse_numeric、enum_table を組み合わせた小さい command parser 例。
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

- Purpose: `c_interop` と `scope` を使った C API 境界の RAII 化例。
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
[ ] 公開APIは namespace ket
[ ] 他の ket module に依存していない
[ ] 公開ヘッダが必要な標準ヘッダを自分で include している
[ ] 公開ヘッダの section banner が規約通り
[ ] 関数 Doxygen に @brief / @param / @retval / @pre / @post がある
[ ] struct / class / enum の Doxygen に @brief がある
[ ] 失敗条件を戻り値・precondition・例外のどれで扱うか固定した
[ ] null / empty / overflow / size不足 / invalid input のテストがある
[ ] 各 TEST に @test / @brief / @details / @pre / @post がある
[ ] C++11/14 module は compile-only check を追加した
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
