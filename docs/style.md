# Style

ketのC++コードは `.clang-format` で統一します。

## 基本

- brace styleはAllmanにします。
- インデントはtab文字を使い、表示幅は4 spacesにします。
- 連続する代入、宣言、macro、operandの縦揃えはしません。
- trailing commentはclang-formatで揃えます。
- pointer/referenceは左寄せにします。
- 公開APIは `namespace ket` に置き、UpperCamelCaseにします。
- ヘッダのinclude guardには `#pragma once` を使います。
- 命名規則はGoogle C++ Styleに従います。

例:

```cpp
#pragma once

namespace ket
{
	class ByteReader
	{
	public:
		bool ReadU8(std::uint8_t* out) noexcept;

	private:
		std::uint8_t const* data_ = nullptr; // non-owning
		std::size_t size_ = 0;
	};
} // namespace ket
```

## 命名

Google C++ Styleに従い、ketでは次の形を標準にします。

- 型名、class名、struct名、enum名、関数名: `UpperCamelCase`
- 変数名、引数名、local変数名: `lower_snake_case`
- private/protected data member: `lower_snake_case_`
- 定数、constexpr値、enum値: `kUpperCamelCase`
- macro: 必要最小限にし、使う場合は `KET_UPPER_SNAKE_CASE`
- file/module名: ketのdrop-in規約に合わせて `ket_<name>.h`, `ket_<name>.cpp`

## 内部helper

- `.cpp` 内だけで使うhelperは無名namespaceに置きます。
- header内で必要なhelperは `ket::detail` に置きます。
- `detail` は公開APIではありません。drop-in性と可読性のため、必要最小限にします。

## コメント

C++コードのコメントは、物理的に何をしているかではなく、論理的な意味、背景、目的を補足するために書きます。

### 論理単位のコメント

まとまった処理には、論理的な処理単位ごとに短いコメントを置きます。
コメントは原則として体言止めにします。

```cpp
// 妥当性確認
if (!IsValidHeader(header))
{
	return false;
}

// 受信ID取得
auto const id = LoadBe16(payload);

// コマンド実行
auto const ret = ExecuteCommand(id);

// 結果判定
if (!ret)
{
	return false;
}
```

コメントは、次のような論理的な意味を表す名前にします。

- `妥当性確認`
- `XXX 取得`
- `YYY 実行`
- `結果判定`
- `後始末登録`
- `境界条件確認`

避けるコメント:

```cpp
// if文
// 変数に代入
// 関数呼び出し
// ループ
```

コードを読めば分かる物理操作をなぞらないでください。

### 分岐内のコメント

分岐処理では、自明でない条件について「何が起きていたのか」をコメントします。
条件式の機械的な読み替えではなく、業務的・論理的な意味を「XX だった」の形で書きます。

```cpp
if (!ret)
{
	// 受信電文不正だった
	return false;
}

if (remaining < kHeaderSize)
{
	// ヘッダを読み切れない長さだった
	return false;
}
```

避けるコメント:

```cpp
if (!ret)
{
	// ret が false
	return false;
}
```

条件式そのものが十分に自明な場合は、無理にコメントを追加しません。

```cpp
if (ptr == nullptr)
{
	return false;
}
```

### コメントを書く基準

- 処理のまとまりを読むための見出しになる
- なぜその確認や処理が必要なのかが分かる
- 条件式だけでは分からない失敗理由が分かる
- 将来の修正者が背景を推測しなくて済む

コメントがコードの言い換えにしかならない場合は、コメントより関数名や変数名を改善してください。

## コマンド

確認:

```sh
python3 tools/check_format.py
```

適用:

```sh
python3 tools/format.py
```

CIでは `clang-format-18` を使います。手元で別名のclang-formatを使う場合は
`CLANG_FORMAT` で指定します。

```sh
CLANG_FORMAT=clang-format-18 python3 tools/check_format.py
```
