# ket

ket は、C++で何度も出会う小さな定型処理に、短く安全な名前を与える
drop-in utility catalog です。

このリポジトリは、最初から大量の空フォルダを作らず、moduleを1つずつ追加して育てます。
設計思想の原典は [ket_coding_agent_brief.md](ket_coding_agent_brief.md) です。

## 現在の状態

- 実moduleはまだありません。
- 候補APIは [catalog.md](catalog.md) に蓄積します。
- 実装状況は [progress.md](progress.md) で追跡します。
- `modules/` 配下のフォルダは、実際にmoduleを作るタイミングで追加します。

## 開発コマンド

WSL/Linux環境を主対象にします。

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
python3 tools/check_format.py
```

整形を適用する場合:

```sh
python3 tools/format.py
```

`clang-format-18` を明示したい場合:

```sh
CLANG_FORMAT=clang-format-18 python3 tools/check_format.py
```

## module追加の基本形

moduleは原則として次の3ファイルで追加します。

```txt
modules/<name>/ket_<name>.h
modules/<name>/ket_<name>.cpp
modules/<name>/ket_<name>_test.cpp
```

moduleを追加するときは、実装、境界値テスト、最低C++標準の確認、`progress.md`更新を
1セットとして扱います。

詳細は [docs/module_lifecycle.md](docs/module_lifecycle.md) を参照してください。

## 方針

- 他のket moduleへの依存は原則として増やしません。
- 標準ライブラリは置き換えず、意図が読みにくい小さな儀式を薄く包みます。
- 小さい重複は許容し、drop-in性を優先します。
- 公開APIは `namespace ket` に置き、名前を見れば中身が想像できるものだけを採用します。
- formatter、test、CIを通ったものだけを進捗上の完了扱いにします。
