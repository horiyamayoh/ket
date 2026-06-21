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

| Module      | Category            | Status   | C++ Min | Tests                              | Format  | Notes                                    |
| ----------- | ------------------- | -------- | ------- | ---------------------------------- | ------- | ---------------------------------------- |
| bcd         | numeric / binary    | verified | C++17   | GoogleTest                         | checked | packed BCDと10進整数・文字列を相互変換   |
| bits        | numeric / binary    | verified | C++11   | GoogleTest + C++11 compile-only    | checked | bit/nibble/mask/rotateの境界処理         |
| container   | container           | verified | C++11   | GoogleTest + C++11 compile-only    | checked | map/vector の小さいlookup、生成、削除補助 |
| ascii       | string              | verified | C++17   | GoogleTest                         | checked | ASCII前提のtrim、split、case変換、置換   |
| cli         | CLI                 | verified | C++17   | GoogleTest                         | checked | argc/argvから小さいCLI optionを取得      |
| date        | date                | verified | C++11   | GoogleTest + C++11/14 compile-only | checked | Gregorian日付と時刻の妥当性判定          |
| deadline    | time                | verified | C++11   | GoogleTest + C++11 compile-only    | checked | steady_clockベースの経過時間と期限判定   |
| scope       | RAII                | verified | C++11   | GoogleTest + C++11 compile-only    | checked | scope exit cleanupと構築時の値復元        |
| parse       | parsing             | verified | C++17   | GoogleTest                         | checked | from_chars境界条件とbool parseを固定     |
| string      | string              | verified | C++17   | GoogleTest                         | checked | 複数文字列片の連結と既存文字列への追記   |
| color       | value               | verified | C++11   | GoogleTest + C++11 compile-only    | checked | RGB小値型と6桁hex文字列を相互変換         |
| contract    | contract            | verified | C++11   | GoogleTest + C++11 compile-only    | checked | pre/postcondition/invariantを常時評価     |
| memory      | memory              | verified | C++11   | GoogleTest + C++11 compile-only    | checked | alignment判定、secure zero、object bytes |
| version     | parsing             | verified | C++17   | GoogleTest                         | checked | numeric version tripletのparse/format/compare |
| testing     | testing / binary    | verified | C++17   | GoogleTest                         | checked | byte列比較用GoogleTest assertion helper |
| ipv4        | network             | verified | C++17   | GoogleTest                         | checked | IPv4 dotted decimalとBE 32bit表現を相互変換 |
| port        | network / value     | verified | C++17   | GoogleTest                         | checked | TCP/UDP port番号の値型とparse/format     |
| variant     | variant             | verified | C++17   | GoogleTest                         | checked | std::variant visitor補助                  |
| utf8        | text                | verified | C++17   | GoogleTest                         | checked | UTF-8 byte列の妥当性検査とcode point数取得 |
| tuple       | tuple               | verified | C++17   | GoogleTest                         | checked | tuple要素のindex順反復と変換             |
| optional    | optional            | verified | C++17   | GoogleTest                         | checked | optionalの値変換、合成、遅延fallback評価 |
| meta        | meta                | verified | C++11   | GoogleTest + C++11 compile-only    | checked | C++11/14欠落type traitsの小補助          |
| numeric     | numeric             | verified | C++11   | GoogleTest + C++11 compile-only    | checked | overflow、align、cast の境界確認         |
| byte_reader | binary              | verified | C++11   | GoogleTest + C++11 compile-only    | checked | 固定bufferからoffset保持で逐次読み取り   |
| endian      | binary              | verified | C++11   | GoogleTest + C++11 compile-only    | checked | byte orderを明示した16/32/64bit読み書き  |
| bytes       | binary              | verified | C++17   | GoogleTest                         | checked | 可変長byte payloadの構築                 |
| byte_writer | binary              | verified | C++11   | GoogleTest + C++11 compile-only    | checked | fixed bufferへの逐次書き込み             |
| byte_view   | view                | verified | C++11   | GoogleTest + C++11 compile-only    | checked | non-owning byte span                     |
| hex         | diagnostic / binary | verified | C++17   | GoogleTest                         | checked | byte列、整数、診断用hexdumpのhex文字列化 |
| enums       | enum                | verified | C++17   | GoogleTest                         | checked | enum classのtable-based変換とflags操作   |
