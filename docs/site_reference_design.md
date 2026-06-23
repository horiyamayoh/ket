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

## 生成器の構成

`tools/site_reference.py` は互換用facadeと生成順序のorchestrationだけを持つ。既存の
`import site_reference` と `python3 tools/generate_site.py` は維持する。

- `tools/site_reference_model.py`: dataclass、JSON読込、Doxygen解析、model load/validate。
- `tools/site_reference_html.py`: escape、相対リンク、table、record table、pill、list helper。
- `tools/site_reference_assets.py`: inline CSSと検索JavaScript template。
- `tools/site_reference_render.py`: page shellと各HTML pageのrender関数。
- `tools/site_reference_check.py`: 生成物比較、リンク検査、HTML構造検査。

render層はHTML断片の組み立てに集中し、Doxygen解析や生成済みHTMLの検査を直接持たない。
CSSやJavaScriptを変更する場合も、render関数の長い文字列へ戻さない。

## Responsive contract

生成HTMLは次の構造を維持する。これは画面幅が狭い環境でも内容が欠けないための契約。

- 全pageにskip link、`main#main`、現在位置を示すnav内の `aria-current="page"` を置く。
- `table` は必ず `.table-scroll` の配下に置き、横に長いAPI signatureを局所scrollに閉じ込める。
- 検索input `#search-input` には `label for="search-input"` を対応させる。
- 旧レイアウト用の `.matrix`、`.diagram`、block型 `.result` は再導入しない。

この契約はブラウザ依存の検査ではなく、標準Pythonの `HTMLParser` による静的検査で守る。

## 更新手順

module追加または変更時の標準手順:

1. header Doxygen、test、`progress.md` を通常のmodule手順で更新。
2. `docs/site_src/modules/<module>.json` を追加または更新。
3. 必要に応じて `docs/site_src/categories.json` と `docs/site_src/use_cases/*.json` を更新。
4. `python3 tools/generate_site.py` を実行。
5. `python3 tools/check_site.py` を実行し、生成物のstale差分、リンク切れ、HTML構造を検査。

## 検査対象

`tools/check_site.py` は次を検査する。

- `progress.md` にある全moduleにsidecar JSONがあること。
- sidecar内のmodule、category、use-case参照が存在すること。
- 各moduleの公開API抽出結果が空でないこと。
- 一時生成したHTMLとコミット済み `site/` が一致すること。
- 生成HTML内の相対リンクが存在すること。
- 全HTMLにskip link、`main#main`、nav内のactive `aria-current` があること。
- 全tableが `.table-scroll` 配下にあること。
- 検索inputに対応するlabelがあること。
- obsoleteな `.matrix`、`.diagram`、block型 `.result` が混入していないこと。
