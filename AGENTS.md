# AGENTS.md

このリポジトリで作業するコーディングエージェント向けの指示です。

## Source of truth

- 設計思想の原典は `ket_coding_agent_brief.md` です。
- 日常の入口は `README.md`、運用ルールは `docs/` を参照してください。
- 候補APIは `catalog.md`、実装状況は `progress.md` で管理します。

## Project direction

- ket は C++ の小さな定型処理に名前を与える drop-in utility catalog です。
- 実moduleは1つずつ追加します。空のカテゴリフォルダや空のmoduleフォルダを先に大量作成しないでください。
- `modules/<name>/` は、そのmoduleを実装するタイミングで初めて作成してください。
- 標準ライブラリを置き換えないでください。薄く包んで意図を読みやすくすることが目的です。
- 業務固有ロジック、巨大framework、深い内部依存、過度なtemplate magicは避けてください。

## Module rules

moduleの標準形は次です。

```txt
modules/<name>/ket_<name>.h
modules/<name>/ket_<name>.cpp
modules/<name>/ket_<name>_test.cpp
```

- 公開APIは `namespace ket` に置きます。
- 命名規則はGoogle C++ Styleに従ってください。enum値も `kUpperCamelCase` にします。
- 各moduleは原則として他のket moduleに依存しないでください。
- 小さい内部処理の重複は許容します。drop-in性を優先します。
- ヘッダ先頭にC++標準要求、依存、drop-in条件を書いてください。
- ヘッダのinclude guardには `#pragma once` を使ってください。
- 公開ヘッダは include what you use を守り、自分が必要な標準ヘッダを自分でincludeしてください。
- `.cpp` 内helperは無名namespace、header内helperは `ket::detail` に置いてください。
- C++コメントでは `namespace` などC++ネイティブな語を和訳しないでください。
- C++コメントは「です」「ます」「ください」を避け、体言止めや簡潔な常体で書いてください。
- 関数Doxygenコメントは宣言側に書き、`@brief`、`@param[in/out]`、`@retval`、`@pre`、`@post` を必ず含めてください。実装定義側に同じDoxygenを重複させないでください。
- `@code` は必須ではありませんが、自明なgetterや単純な判定以外では強く推奨します。入力例と戻り値の形が一目で分かる短い例にしてください。
- `@note`、`@pre`、`@post` は定型句だけで済ませず、制約、保持する性質、副作用の有無が分かる文章にしてください。
- module testの各 `TEST` にはDoxygen形式の試験仕様を書き、`@test`、`@brief`、`@details`、`@pre`、`@post` を必ず含めてください。
- namespace終端コメントは `// namespace ket` のように書き、その直前に空行を1行入れてください。
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
- C++コメント規約は `docs/style.md` に従ってください。物理操作ではなく、論理的な意味、背景、目的を書きます。
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
