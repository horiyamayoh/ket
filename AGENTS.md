# AGENTS.md

このリポジトリで作業するコーディングエージェント向けの指示です。

## Source of truth

- 設計思想の原典は `ket_coding_agent_brief.md` です。
- 日常の入口は `README.md`、運用ルールは `docs/` を参照してください。
- 候補APIは `catalog.md`、実装状況は `progress.md` で管理します。

## Project direction

- ket は C++ の小さな定型処理を、短く安全に再利用できるAPIとして切り出す drop-in utility catalog です。
- 実moduleは1つずつ追加します。空のカテゴリフォルダや空のmoduleフォルダを先に大量作成しないでください。
- `modules/<name>/` は、そのmoduleを実装するタイミングで初めて作成してください。
- 段階的な開発は採用範囲を小さく保つための方針であり、実装品質を下げる理由ではありません。採用した関数は1つずつ、設計、実装、Doxygen、テスト、検証のすべてで高い忠実度を求めてください。
- 標準ライブラリを置き換えないでください。薄く包んで意図を読みやすくすることが目的です。
- 業務固有ロジック、巨大framework、深い内部依存は避けてください。

## Module rules

moduleの標準形は次です。header-onlyで十分なmoduleでは `.cpp` を置かず、空または
includeだけの `.cpp` は作らないでください。

```txt
modules/<name>/ket_<name>.h
modules/<name>/ket_<name>.cpp  # 実装がある場合だけ
modules/<name>/ket_<name>_test.cpp
```

- 公開APIは module ごとの入れ子namespace `namespace ket::<module>` に置きます。C++11/14のdrop-inを維持するため、`namespace ket { namespace <module> { ... } }` の入れ子block形式で書き、C++17の `namespace ket::<module>` 短縮形は使わないでください。top-levelの `namespace ket` は1つだけにします。
- module名はfolder名と一致させます。冗長なfolder名は短い別名にします（`parse_numeric`→`parse`、`string_ascii`→`ascii`、`semantic_version`→`version` など）。
- namespaceで対象moduleが明らかになるため、API名からmodule tokenと型tokenを落とします（`ParseIpv4Address`→`ket::ipv4::Parse`、`Ipv4Address`→`ket::ipv4::Address`）。正準動詞とfallback接尾辞の詳細は `docs/style.md` に従ってください。
- 公開ヘッダでは、公開APIのDoxygen付き宣言を先に並べ、header内helperが必要な場合はその後に `ket::<module>::detail` の内部helper、最後にinline、constexpr、templateなどの公開API定義を書いてください。
- 公開ヘッダの各sectionには、`Public API declarations`、`Internal implementation details`、`Public API definitions` のdashed bannerコメントを置いてください。
- 命名規則はGoogle C++ Styleに従ってください。enum値も `kUpperCamelCase` にします。
- 非optionalの出力引数と入出力引数は参照型で受けてください。`nullptr` が意味を持つoptional出力やC API境界だけポインタ型を使い、その理由をDoxygenに書いてください。
- 各moduleは原則として他のket moduleに依存しないでください。
- 小さい内部処理の重複は許容します。drop-in性を優先します。
- ヘッダ先頭にDoxygen `@file`コメントを書き、`@brief`、`@details`、`@par プロジェクトへの適用方法`、`@par C++バージョン要件`、`@par 他のライブラリへの依存`、`@par namespace` を含めてください。
- `@par C++バージョン要件` には `最小要件：`、`本ライブラリの適用を推奨する C++ バージョン：`、`推奨理由：`、`本ライブラリの適用を推奨しない C++ バージョン：`、`非推奨理由：` を書いてください。非推奨がない場合は `本ライブラリの適用を推奨しない C++ バージョン：なし。` と `非推奨理由：なし。` を書いてください。
- `@par namespace` には `公開API：ket::<module>` と内部helperの配置を書いてください。header内helperがある場合は `内部実装：ket::<module>::detail`、`.cpp` 内helperだけの場合は `内部実装：.cpp の無名 namespace`、内部helperがない場合は `内部実装：なし` と書いてください。
- ヘッダのinclude guardには `#pragma once` を使ってください。
- 公開ヘッダは include what you use を守り、自分が必要な標準ヘッダを自分でincludeしてください。
- `.cpp` 内helperは無名namespace、header内helperは `ket::<module>::detail` に置いてください。
- `template <...>` 宣言の後は必ず改行し、対象の `struct`、`class`、関数宣言、関数定義を同じ行に置かないでください。
- C++コメントでは `namespace` などC++ネイティブな語を和訳しないでください。
- C++コメントは「です」「ます」「ください」を避け、体言止めや簡潔な常体で書いてください。
- 関数Doxygenコメントは宣言側に書き、通常の関数には `@brief`、`@param[in]` / `@param[out]` / `@param[in,out]`、`@retval`、`@pre`、`@post` を含めてください。constructorとdestructorは戻り値を持たないため、`@retval` と `@return` を書かず、生成後または破棄後の性質を `@post` に書いてください。実装定義側に同じDoxygenを重複させないでください。
- 公開ヘッダ内の `struct`、`class`、`enum` にはDoxygenコメントを書き、少なくとも `@brief` を含めてください。
- 公開API関数のDoxygenには `@code` と `@endcode` を必ず書いてください。入力例と戻り値の形が一目で分かる短い例にしてください。
- `@note`、`@pre`、`@post` は定型句だけで済ませず、制約、保持する性質、副作用の有無が分かる文章にしてください。
- module testの各 `TEST` にはDoxygen形式の試験仕様を書き、`@test`、`@brief`、`@details`、`@pre`、`@post` を必ず含めてください。
- namespace終端コメントは `// namespace bcd`、`// namespace ket` のように書き、入れ子namespaceは内側から順に閉じてそれぞれに終端コメントを付け、その直前に空行を1行入れてください。
- `if`、`while`、`EXPECT_FALSE` などの条件式でAPI呼び出しを直接行わず、直前の一時変数へ退避してください。gdbで値を追いやすくするためです。

## Build, test, format

主環境はWSL/Linuxです。

```sh
npm ci
python3 tools/check_repository.py
```

整形を適用する場合:

```sh
python3 tools/format.py
```

- C++コードは `.clang-format` に従ってください。
- Markdown、YAML、JSONはPrettierに従ってください。
- formatterはAllman brace、tab indentation、表示幅4を前提にしています。
- 静的解析は `clang-tidy-18`、`cppcheck`、`iwyu` を使います。
- C++コメント規約は `docs/style.md` に従ってください。物理操作ではなく、論理的な意味、背景、目的、drop-in時の要件を書きます。
- functional testはGoogleTestで書きます。
- GoogleTestは `v1.17.0` 固定で、functional testはC++17以上です。
- C++11/14対応moduleは、GoogleTestとは別にcompile-only checkを追加してください。

## Before finishing a change

変更を終える前に次を確認してください。

- `python3 tools/check_repository.py`
- `python3 tools/check_python.py`
- `python3 tools/check_layout.py`
- `python3 tools/check_format.py`
- `cmake --preset dev`
- `cmake --build --preset dev`
- `cmake --build --preset dev --target check-static`
- `cmake --build --preset dev --target check-conventions`
- `ctest --preset dev`
- `cmake --preset sanitize`
- `cmake --build --preset sanitize`
- `ctest --preset sanitize`
- `git diff --check`

moduleを追加または変更した場合は、必要に応じて次も更新してください。

- `catalog.md`
- `progress.md`
- `docs/`

## Keep changes scoped

- ユーザーが求めていないmoduleやAPIをついでに追加しないでください。
- 既存の未コミット変更を勝手に戻さないでください。
- 便利さより、読みやすさ、安全性、持ち出しやすさを優先してください。
- `v1`、初期実装、最小APIという言葉を、品質を落とす理由にしないでください。未実装の機能を増やさないだけで、追加する関数自体は常に最高水準を目指してください。
