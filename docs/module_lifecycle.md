# Module lifecycle

ketはmoduleを1つずつ追加して育てます。最初から空のカテゴリフォルダを大量に作りません。
この段階的な進め方は採用範囲を小さく保つための方針であり、追加する関数の品質を下げる理由ではありません。

## 1. 候補をcatalogに置く

実装前に `catalog.md` に候補APIを書きます。

最低限、次を明確にします。

- 痛み
- 候補API
- C++サポート
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
`.cpp` は作りません。header-onlyでもdrop-in条件をファイル先頭のDoxygen `@file`コメントに書きます。
ヘッダのinclude guardには `#pragma once` を使います。

## 3. 実装する

- 公開APIは module ごとの入れ子 `namespace ket::<module>` に置きます。top-levelの `namespace ket` は1つだけにします。
- 公開ヘッダでは、公開APIのDoxygen付き宣言、`ket::<module>::detail` の内部helper、ヘッダ内に必要な公開API定義の順に書きます。
- 公開ヘッダの各sectionには、`Public API declarations`、`Internal implementation details`、`Public API definitions` のdashed bannerコメントを置きます。
- 最小APIや初期実装であっても、採用した関数は設計、実装、Doxygen、テスト、検証のすべてで高い忠実度を求めます。
- 非optionalの出力引数と入出力引数は参照型で受けます。optional出力やC API境界でポインタ型を使う場合は、nullable性と理由をDoxygenに書きます。
- 他のket moduleには原則依存しません。
- 必要な標準ヘッダは自分でincludeします。
- 小さな内部処理は重複してもよいです。
- C++標準要求、依存、namespace、drop-in条件をヘッダ先頭のDoxygen `@file`コメントに書きます。
- Doxygen `@file`コメントには `@brief`、`@details`、`@par プロジェクトへの適用方法`、`@par C++バージョン要件`、`@par 他のライブラリへの依存`、`@par namespace` を含めます。
- `@par C++バージョン要件` には `最小要件：`、`本ライブラリの適用を推奨する C++ バージョン：`、`推奨理由：`、`本ライブラリの適用を推奨しない C++ バージョン：`、`非推奨理由：` を書きます。
- `@par namespace` には `公開API：ket::<module>` と `内部実装：ket::<module>::detail` を書きます。
- ヘッダのinclude guardには `#pragma once` を使います。
- `template <...>` 宣言の後は必ず改行し、対象の宣言や定義を同じ行に置きません。
- 関数Doxygenコメントは宣言側に書き、通常の関数には `@brief`、`@param[in]` / `@param[out]` / `@param[in,out]`、`@retval`、`@pre`、`@post` を含めます。constructorとdestructorは戻り値を持たないため、`@retval` と `@return` を書かず、生成後または破棄後の性質を `@post` に書きます。
- 公開ヘッダ内の `struct`、`class`、`enum` にはDoxygenコメントを書き、少なくとも `@brief` を含めます。
- 公開API関数のDoxygenには `@code` と `@endcode` を必ず書きます。
  入力例と戻り値の形が一目で分かる短い例にします。
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
