# ket progress

module単位の実装状況を追跡します。

候補APIは `catalog.md` に置きます。この表には、実際にmoduleとして作り始めたものだけを
追加します。空の予定表にしないことで、1つずつ育てる方針を保ちます。

Statusは次の値を使います。

- `in-progress`: 実装中
- `implemented`: 実装とテストがある
- `verified`: format、build、test、必要なcompile-only checkが通っている
- `deprecated`: 残しているが新規利用を推奨しない

| Module | Category         | Status   | C++ Min | Tests      | Format  | Notes                                  |
| ------ | ---------------- | -------- | ------- | ---------- | ------- | -------------------------------------- |
| bcd    | numeric / binary | verified | C++17   | GoogleTest | checked | packed BCDと10進整数・文字列を相互変換 |
