# ket

ket は、C++で何度も書く小さな定型処理を、短く安全に再利用できるAPIとして
切り出す drop-in utility catalog です。

このリポジトリは、最初から大量の空フォルダを作らず、moduleを1つずつ追加して育てます。
設計思想の原典は [ket_coding_agent_brief.md](ket_coding_agent_brief.md) です。

## 現在の状態

- 実装済みmoduleの一覧と検証状況は [progress.md](progress.md) で追跡します。
- 候補APIは [catalog.md](catalog.md) に蓄積します。
- module/API製造依頼用の正本は [docs/module_api_catalog.md](docs/module_api_catalog.md)
  で管理します。
- `modules/` 配下のフォルダは、実際にmoduleを作るタイミングで追加します。

## 開発コマンド

WSL/Linux環境を主対象にします。

```sh
npm ci
python3 tools/check_repository.py
```

整形を適用する場合:

```sh
python3 tools/format.py
```

`clang-format-18` を明示したい場合:

```sh
CLANG_FORMAT=clang-format-18 python3 tools/check_format.py
```

静的解析では `clang-tidy-18`、`cppcheck`、`iwyu` を使います。
未導入の場合はWSL/Linux環境で次を入れます。

```sh
sudo apt-get install -y clang-tidy-18 cppcheck iwyu
```

個別の確認を分けて実行する場合:

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

## module追加の基本形

moduleは原則として次の3ファイルで追加します。

```txt
modules/<name>/ket_<name>.h
modules/<name>/ket_<name>.cpp  # 実装がある場合だけ
modules/<name>/ket_<name>_test.cpp
```

header-only moduleでは `.cpp` を置きません。moduleを追加するときは、実装、境界値テスト、
最低C++標準の確認、`progress.md`更新を1セットとして扱います。

詳細は [docs/module_lifecycle.md](docs/module_lifecycle.md) を参照してください。

## 方針

- 他のket moduleへの依存は原則として増やしません。
- 標準ライブラリは置き換えず、意図が読みにくい小さな儀式を薄く包みます。
- 小さい重複は許容し、drop-in性を優先します。
- moduleを1つずつ育てる方針はスコープ管理であり、品質を下げる理由ではありません。
- 追加する関数は1つずつ、設計、実装、ドキュメント、テスト、検証のすべてで高い忠実度を求めます。
- 公開APIは module ごとの入れ子 `namespace ket::<module>` に置き、名前を見れば中身が想像できるものだけを採用します。
- formatter、静的解析、test、CIを通ったものだけを進捗上の完了扱いにします。
