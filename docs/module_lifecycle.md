# Module lifecycle

ketはmoduleを1つずつ追加して育てます。最初から空のカテゴリフォルダを大量に作りません。
この段階的な進め方は採用範囲を小さく保つための方針であり、追加する関数の品質を下げる理由ではありません。

## 1. 候補をcatalogに置く

実装前に `catalog.md` に候補APIを書きます。

最低限、次を明確にします。

- 痛み
- 候補API
- C++バージョン
- 失敗条件と境界条件
- 依存
- テストしたいケース

## 2. moduleフォルダを作る

実装するタイミングでだけ、次の形でフォルダを追加します。

```txt
modules/<name>/ket_<name>.h
modules/<name>/ket_<name>.cpp  # 実装がある場合だけ
modules/<name>/ket_<name>_test.cpp
```

header-onlyで十分な場合は `.cpp` を置きません。空、または自分のヘッダをincludeするだけの
`.cpp` は作りません。header-onlyでもdrop-in条件をファイル先頭に書きます。
ヘッダのinclude guardには `#pragma once` を使います。

## 3. 実装する

- 公開APIは `namespace ket` に置きます。
- 最小APIや初期実装であっても、採用した関数は設計、実装、Doxygen、テスト、検証のすべてで高い忠実度を求めます。
- 非optionalの出力引数と入出力引数は参照型で受けます。optional出力やC API境界でポインタ型を使う場合は、nullable性と理由をDoxygenに書きます。
- 他のket moduleには原則依存しません。
- 必要な標準ヘッダは自分でincludeします。
- 小さな内部処理は重複してもよいです。
- C++標準要求、依存、namespaceをヘッダ先頭に書きます。
- ヘッダのinclude guardには `#pragma once` を使います。
- `template <...>` 宣言の後は必ず改行し、対象の宣言や定義を同じ行に置きません。
- 関数Doxygenコメントは宣言側に書き、`@brief`、`@param[in]` / `@param[out]` / `@param[in,out]`、`@retval`、`@pre`、`@post` を含めます。
- 公開ヘッダ内の `struct`、`class`、`enum` にはDoxygenコメントを書き、少なくとも `@brief` を含めます。
- `@code` は必須ではありませんが、自明なgetterや単純な判定以外では強く推奨します。
- `@note`、`@pre`、`@post` は制約、保持する性質、副作用の有無が分かる文章にします。
- 実装定義側に同じDoxygenコメントを重複させません。`.cpp`内だけで使うhelperにもDoxygenコメントは書きません。
- C++コメントでは `namespace` などのC++ネイティブな語を和訳せず、「です」「ます」「ください」を使いません。
- namespace終端コメントの直前には空行を1行入れます。
- 制御構文の条件式でAPI呼び出しを直接行わず、直前の一時変数に退避します。

## 4. テストする

functional testはGoogleTestで書きます。

C++11/14対応をうたうmoduleは、GoogleTestとは別にcompile-only checkを追加します。
これはGoogleTest v1.17.0がC++17以上を要求するためです。

## 5. progressを更新する

`progress.md` に1行追加または更新します。

完了扱いにする条件:

- 実装がある
- 境界値テストがある
- 採用した関数について、性能、安全性、可読性、持ち出しやすさのトレードオフを意図して選んでいる
- format checkが通る
- static analysisとconvention checkが通る
- buildとCTestが通る
- 必要な最低C++標準のcompile-only checkが通る
