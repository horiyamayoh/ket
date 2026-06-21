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

| Module      | Category         | Status   | C++ Min | Tests                              | Format  | Notes                                    |
| ----------- | ---------------- | -------- | ------- | ---------------------------------- | ------- | ---------------------------------------- |
| bcd         | numeric / binary | verified | C++17   | GoogleTest                         | checked | packed BCDと10進整数・文字列を相互変換   |
| bits        | numeric / binary | verified | C++11   | GoogleTest + C++11 compile-only    | checked | bit/nibble/mask/rotateの境界処理         |
| date        | date             | verified | C++11   | GoogleTest + C++11/14 compile-only | checked | Gregorian日付と時刻の妥当性判定          |
| deadline    | time             | verified | C++11   | GoogleTest + C++11 compile-only    | checked | steady_clockベースの経過時間と期限判定   |
| string      | string           | verified | C++17   | GoogleTest                         | checked | 複数文字列片の連結と既存文字列への追記   |
| lang        | language         | verified | C++11   | GoogleTest + C++11 compile-only    | checked | 未使用無視、raw配列長、const参照化       |
| tuple       | tuple            | verified | C++17   | GoogleTest                         | checked | tuple要素のindex順反復と変換             |
| optional    | optional         | verified | C++17   | GoogleTest                         | checked | optionalの値変換、合成、遅延fallback評価 |
| meta        | meta             | verified | C++11   | GoogleTest + C++11 compile-only    | checked | C++11/14欠落type traitsの小補助          |
| numeric     | numeric          | verified | C++11   | GoogleTest + C++11 compile-only    | checked | overflow、align、cast の境界確認         |
| byte_reader | binary           | verified | C++11   | GoogleTest + C++11 compile-only    | checked | 固定bufferからoffset保持で逐次読み取り   |
| endian      | binary           | verified | C++11   | GoogleTest + C++11 compile-only    | checked | byte orderを明示した16/32/64bit読み書き  |
| bytes       | binary           | verified | C++17   | GoogleTest                         | checked | 可変長byte payloadの構築                 |
| byte_writer | binary           | verified | C++11   | GoogleTest + C++11 compile-only    | checked | fixed bufferへの逐次書き込み             |
| byte_view   | view             | verified | C++11   | GoogleTest + C++11 compile-only    | checked | non-owning byte span                     |
