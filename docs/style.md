# Style

ketのC++コードは `.clang-format` で統一します。

## 基本

- brace styleはAllmanにします。
- インデントはtab文字を使い、表示幅は4 spacesにします。
- 連続する代入、宣言、macro、operandの縦揃えはしません。
- trailing commentはclang-formatで揃えます。
- pointer/referenceは左寄せにします。
- `const` は左寄せにします。ポインタ自体のconstなど意味がある場合だけ `* const` のように右側へ置きます。
- 公開APIは `namespace ket` に置き、UpperCamelCaseにします。
- ヘッダのinclude guardには `#pragma once` を使います。
- 命名規則はGoogle C++ Styleに従います。
- `template <...>` 宣言の後は必ず改行し、対象の宣言や定義を同じ行に置きません。
- 非optionalの出力引数と入出力引数は参照型で受けます。`nullptr` が意味を持つoptional出力やC
  API境界だけポインタ型を使い、その理由をDoxygenに書きます。

## 品質方針

- `v1`、初期実装、最小APIは採用範囲を小さくする意味であり、実装品質を下げる意味ではありません。
- 追加する関数は1つずつ、性能、安全性、可読性、ドキュメント、テスト、検証のすべてで高い忠実度を求めます。
- 機能を増やさない判断と、採用した関数の品質を落とす判断を混同しません。

## noexcept / constexpr

- 純粋な変換、判定、ビット操作は、できるだけ `constexpr` にします。
- 例外で失敗を表現しない小さな関数は、できるだけ `noexcept` にします。
- 文字列生成やメモリ確保を伴うAPIは、無理に `noexcept` にしません。

例:

```cpp
#pragma once

namespace ket
{
	class ByteReader
	{
	public:
		bool ReadU8(std::uint8_t& out) noexcept;

	private:
		const std::uint8_t* data_ = nullptr; // non-owning
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
- header-only moduleは `.cpp` を置かず、空またはincludeだけの `.cpp` は作りません。

## 出力引数

- 非optionalの出力引数と入出力引数は参照型で受けます。
- `nullptr` に「出力不要」「対象なし」の意味を持たせたい場合だけ、ポインタ型を使います。
- C API境界でraw pointerをそのまま扱うほうが自然な場合は、ポインタ型を使ってよいです。
- ポインタ型を使う公開APIでは、`@pre`、`@note`、または引数説明にnullable性と理由を書きます。

例:

```cpp
bool TryReadU8(std::uint8_t& out) noexcept;
bool TryReadOptionalU8(std::uint8_t* out_or_null) noexcept;
```

## template宣言

`template <...>` 宣言は、続く `struct`、`class`、関数宣言、関数定義と同じ行に置きません。
`.clang-format` の `AlwaysBreakTemplateDeclarations: Yes` で自動整形します。

例:

```cpp
template <typename Part>
struct IsStringPart;

template <typename T>
constexpr bool OptionalIsEmpty(std::optional<T> value) noexcept;
```

## 内部helper

- `.cpp` 内だけで使うhelperは無名namespaceに置きます。
- header内で必要なhelperは `ket::detail` に置きます。
- `detail` は公開APIではありません。drop-in性と可読性のため、必要最小限にします。

## コメント

C++コードのコメントは、物理的に何をしているかではなく、論理的な意味、背景、目的を補足するために書きます。

- `namespace`、`constexpr`、`noexcept`、`nullptr`、`std::nullopt` などC++ネイティブな語は和訳しません。
- コメント本文では「です」「ます」「ください」を使いません。体言止め、または簡潔な常体にします。
- namespace終端コメントは `// namespace ket` のように標準的な英語表記にします。
- namespace終端コメントの直前には空行を1行入れます。

### 関数Doxygenコメント

関数の契約は宣言側にDoxygen形式で書きます。
実装定義側に同じDoxygenコメントを重複させません。
ヘッダ内のinline関数やconstexpr関数は宣言兼定義のため、ヘッダ側にDoxygenコメントを書きます。
`.cpp`内だけで使うhelperは実装詳細のため、Doxygenコメントを書きません。

必須タグ:

- `@brief`
- `@param[in]`、`@param[out]`、または `@param[in,out]`
- `@retval`
- `@pre`
- `@post`

必要に応じて `@note`、`@remark` を追加します。
`@code` は必須ではありませんが、自明なgetterや単純な判定以外では強く推奨します。
入力例、戻り値、失敗時の形が一目で分かる短い例にします。

`@note`、`@pre`、`@post` は、読み手が制約や副作用を判断できる文章にします。
`なし` と書く場合は、本当に事前条件や副作用がない場合だけにします。
保持する性質を書く場合は、何をどう保持するのかまで具体化します。

例:

```cpp
/**
 * @brief 任意バイト長パックBCDの10進文字列変換。
 * @param[in] data 変換対象のパックBCD列。
 * @param[in] size `data`のバイト数。
 * @retval value 変換後の10進文字列。
 * @retval std::nullopt `nullptr`、空入力、または不正nibble。
 * @pre `data`は`size`バイト以上読み取り可能な配列を指す。`nullptr` と空入力は失敗値として扱う。
 * @post 引数と外部状態の変更なし。
 * @note 入力BCDの桁数を保ち、先頭の0も文字'0'として出力。
 * @code
 * const std::uint8_t data[] = {0x00U, 0x42U};
 * const auto value = ket::BcdToDecimalString(data, 2U);
 * // value == std::optional<std::string>("0042")
 * @endcode
 */
std::optional<std::string> BcdToDecimalString(const std::uint8_t* data, std::size_t size);
```

### 型Doxygenコメント

公開ヘッダ内の `struct`、`class`、`enum` にはDoxygenコメントを書きます。
必須タグは `@brief` です。template型では必要に応じて `@tparam` も書きます。
`ket::detail` 配下の内部型でも、ヘッダに置く場合は読み手が役割を追える短い説明を残します。

例:

```cpp
/**
 * @brief 文字列片として受け付ける型の判定。
 * @tparam Part 判定対象の型。
 * @note detail配下の型は公開APIではない。
 */
template <typename Part>
struct IsStringPart;
```

### 試験仕様Doxygenコメント

GoogleTestの各 `TEST` には、試験仕様としてDoxygenコメントを書きます。
`TEST` macroには通常の関数引数がないため、関数用の `@param` と `@retval` は要求しません。
`@pre`、`@post` は、テストの前提とテスト後に変わらない性質が分かる文章にします。

必須タグ:

- `@test`
- `@brief`
- `@details`
- `@pre`
- `@post`

例:

```cpp
/**
 * @test
 * @brief 2桁固定幅パックBCDの正常系確認。
 * @details 代表値と境界値を入力し、10進整数へ変換されることを確認。
 * @pre C++17以降。
 * @post テスト対象APIと外部状態の変更なし。
 */
TEST(KetBcdTest, ParsesUint8Bcd)
{
	// ...
}
```

### 制御構文の条件式

`if`、`while`、`EXPECT_FALSE` などの条件式では、API呼び出しやメソッド呼び出しを直接行いません。
直前の一時変数に退避し、gdbで値を確認しやすくします。

```cpp
const auto header_is_valid = IsValidHeader(header);
if (!header_is_valid)
{
	return false;
}
```

### 論理単位のコメント

まとまった処理には、論理的な処理単位ごとに短いコメントを置きます。
コメントは原則として体言止めにします。

```cpp
// 妥当性確認
const auto header_is_valid = IsValidHeader(header);
if (!header_is_valid)
{
	return false;
}

// 受信ID取得
const auto id = LoadBe16(payload);

// コマンド実行
const auto ret = ExecuteCommand(id);

// 結果判定
const auto succeeded = static_cast<bool>(ret);
if (!succeeded)
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
npm ci
python3 tools/check_format.py
cmake --build --preset dev --target check-conventions
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

Markdown、YAML、JSONはPrettierで確認します。Prettierは `package-lock.json` で固定し、
手元では `npm ci` 後に `python3 tools/check_format.py` から実行します。
