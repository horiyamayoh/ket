# Static reference site design

この文書は、`site/` に生成する ket 静的リファレンスサイトの設計と運用ルールを固定する。
サイトは `file://` で開けるコミット済み成果物だが、正本ではない。

## 正本

サイト生成の正本は次の3種類に分ける。

- `modules/*/ket_*.h`: 公開API、Doxygen契約、drop-in条件、C++要件。
- `progress.md`: 実装済みmodule、status、検証状態、短い説明。
- `docs/site_src/`: 用途別分類、ユースケース、関連module、サイト構成。

`site/` 配下のHTMLは手編集しない。内容を変える場合は上記正本を編集し、
`python3 tools/generate_site.py` で再生成する。

## 生成方針

- 標準Pythonのみを使う。Doxygen、Node系SSG、外部CDN、外部fetchは使わない。
- 複数HTMLを生成し、相対リンクだけで遷移する。
- CSSと検索用JavaScriptはHTMLへinline埋め込みし、`file://` で閲覧可能にする。
- 本文は日本語中心。API名、signature、namespace、C++用語は英語表記を維持する。
- 図表はHTML表またはinline SVGとして生成する。

## 更新手順

module追加または変更時の標準手順:

1. header Doxygen、test、`progress.md` を通常のmodule手順で更新。
2. `docs/site_src/modules/<module>.json` を追加または更新。
3. 必要に応じて `docs/site_src/categories.json` と `docs/site_src/use_cases/*.json` を更新。
4. `python3 tools/generate_site.py` を実行。
5. `python3 tools/check_site.py` を実行し、生成物のstale差分とリンク切れを検査。

## 検査対象

`tools/check_site.py` は次を検査する。

- `progress.md` にある全moduleにsidecar JSONがあること。
- sidecar内のmodule、category、use-case参照が存在すること。
- 各moduleの公開API抽出結果が空でないこと。
- 一時生成したHTMLとコミット済み `site/` が一致すること。
- 生成HTML内の相対リンクが存在すること。
