# ket progress

module単位の実装状況を追跡します。

候補APIは `catalog.md` に置きます。この表には、実際にmoduleとして作り始めたものだけを
追加します。空の予定表にしないことで、1つずつ育てる方針を保ちます。
C++バージョン要件の詳細は、各moduleヘッダ先頭のDoxygen `@file`コメントを正とします。

Statusは次の値を使います。

- `in-progress`: 実装中
- `implemented`: 実装とテストがある
- `verified`: format、build、test、必要なcompile-only checkが通っている
- `deprecated`: 残しているが新規利用を推奨しない

| Module   | Category         | Status   | C++ Min | Tests                              | Format  | Notes                                       |
| -------- | ---------------- | -------- | ------- | ---------------------------------- | ------- | ------------------------------------------- |
| bcd      | numeric / binary | verified | C++17   | GoogleTest                         | checked | packed BCDと10進整数・文字列を相互変換      |
| date     | date             | verified | C++11   | GoogleTest + C++11/14 compile-only | checked | Gregorian日付と時刻の妥当性判定             |
| deadline | time             | verified | C++11   | GoogleTest + C++11 compile-only    | checked | steady_clockベースの経過時間と期限判定      |
| string   | string           | verified | C++17   | GoogleTest                         | checked | 複数文字列片の連結と既存文字列への追記      |
| ipv4     | network          | verified | C++17   | GoogleTest                         | checked | IPv4 dotted decimalとBE 32bit表現を相互変換 |
