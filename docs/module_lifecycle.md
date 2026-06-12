# Module lifecycle

ketはmoduleを1つずつ追加して育てます。最初から空のカテゴリフォルダを大量に作りません。

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
modules/<name>/ket_<name>.cpp
modules/<name>/ket_<name>_test.cpp
```

header-onlyで十分な場合も、drop-in条件をファイル先頭に書きます。
ヘッダのinclude guardには `#pragma once` を使います。

## 3. 実装する

- 公開APIは `namespace ket` に置きます。
- 他のket moduleには原則依存しません。
- 必要な標準ヘッダは自分でincludeします。
- 小さな内部処理は重複してもよいです。
- C++標準要求、依存、namespaceをヘッダ先頭に書きます。
- ヘッダのinclude guardには `#pragma once` を使います。

## 4. テストする

functional testはGoogleTestで書きます。

C++11/14対応をうたうmoduleは、GoogleTestとは別にcompile-only checkを追加します。
これはGoogleTest v1.17.0がC++17以上を要求するためです。

## 5. progressを更新する

`progress.md` に1行追加または更新します。

完了扱いにする条件:

- 実装がある
- 境界値テストがある
- format checkが通る
- buildとCTestが通る
- 必要な最低C++標準のcompile-only checkが通る
