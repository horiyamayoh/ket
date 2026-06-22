#!/usr/bin/env python3

from __future__ import annotations

import json


def site_css() -> str:
	return """
:root {
  color-scheme: light;
  --bg: #f6f7f8;
  --panel: #ffffff;
  --panel-soft: #fafbfb;
  --ink: #202326;
  --muted: #5c646b;
  --line: #d7dce0;
  --line-soft: #e7eaed;
  --link: #075e9f;
  --accent: #17745b;
  --accent-soft: #edf6f2;
  --code: #f1f3f5;
}
* { box-sizing: border-box; }
html { font-size: 13.5px; }
body { margin: 0; background: var(--bg); color: var(--ink); font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; line-height: 1.32; }
.skip-link { position: absolute; left: 8px; top: -40px; z-index: 4; padding: 4px 8px; background: var(--ink); color: #ffffff; border-radius: 4px; }
.skip-link:focus { top: 8px; }
header { position: sticky; top: 0; z-index: 2; display: grid; grid-template-columns: auto 1fr; gap: 12px; align-items: center; border-bottom: 1px solid var(--line); background: rgba(255, 255, 255, 0.96); padding: 6px 12px; }
.brand a { color: var(--ink); font-weight: 700; text-decoration: none; }
nav { display: flex; flex-wrap: wrap; gap: 4px; font-size: 0.92rem; }
a { color: var(--link); text-underline-offset: 2px; }
a:focus-visible, input:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
nav a { border-radius: 4px; padding: 2px 6px; text-decoration: none; }
nav a[aria-current="page"] { background: var(--accent-soft); color: var(--accent); font-weight: 650; }
main { max-width: 1240px; margin: 0 auto; padding: 12px 12px 28px; }
footer { border-top: 1px solid var(--line); color: var(--muted); padding: 8px 12px; font-size: 0.86rem; }
h1, h2, h3, h4 { line-height: 1.2; letter-spacing: 0; }
h1 { margin: 0 0 8px; font-size: 1.75rem; }
h2 { margin: 18px 0 6px; padding-bottom: 3px; border-bottom: 1px solid var(--line); font-size: 1.25rem; }
h3 { margin: 12px 0 4px; font-size: 1.05rem; }
h4 { margin: 8px 0 3px; font-size: 1rem; }
p { margin: 4px 0 8px; }
ul, ol { margin: 4px 0 8px; padding-left: 18px; }
li { margin: 2px 0; }
.lead { color: var(--muted); max-width: 90ch; }
.summary-band { display: grid; gap: 6px; margin: 6px 0 12px; border: 1px solid var(--line); border-left: 3px solid var(--accent); border-radius: 6px; background: var(--panel); padding: 8px 10px; }
.summary-copy { max-width: 96ch; }
.summary-actions { display: flex; flex-wrap: wrap; gap: 6px; }
.summary-actions a { border: 1px solid var(--line); border-radius: 4px; background: var(--panel-soft); padding: 3px 8px; text-decoration: none; }
.stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(110px, 1fr)); gap: 4px; margin: 0; }
.stats div { border: 1px solid var(--line-soft); border-radius: 4px; background: var(--panel-soft); padding: 4px 6px; }
.stats dt { color: var(--muted); font-size: 0.84rem; }
.stats dd { margin: 1px 0 0; font-size: 1.16rem; font-weight: 700; }
.grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(190px, 1fr)); gap: 6px; }
.card { min-width: 0; border: 1px solid var(--line); border-radius: 6px; background: var(--panel); padding: 6px; overflow-wrap: anywhere; }
.card h3 { margin: 0 0 4px; }
.meta { color: var(--muted); font-size: 0.88rem; }
.pillrow { display: flex; flex-wrap: wrap; gap: 4px; margin-top: 4px; }
.pill { display: inline-block; border: 1px solid var(--line); border-radius: 999px; padding: 1px 6px; background: var(--panel-soft); color: var(--muted); font-size: 0.82rem; text-decoration: none; }
.pill a { color: inherit; text-decoration: none; }
.table-scroll { max-width: 100%; overflow-x: auto; border: 1px solid var(--line); border-radius: 4px; background: var(--panel); }
table { width: 100%; border-collapse: collapse; background: var(--panel); font-size: 0.96rem; line-height: 1.22; }
.table-scroll table { border: 0; }
th, td { border: 0; border-right: 1px solid var(--line-soft); border-bottom: 1px solid var(--line-soft); padding: 3px 5px; vertical-align: top; overflow-wrap: anywhere; }
th:last-child, td:last-child { border-right: 0; }
tr:last-child td { border-bottom: 0; }
th { background: #edf2f3; text-align: left; font-weight: 650; }
tbody tr:nth-child(even) td { background: var(--panel-soft); }
tbody tr:hover td { background: #f2f6f8; }
.module-name, .api-name, .kind-cell, .count-cell { white-space: nowrap; overflow-wrap: normal; }
.signature-cell code { white-space: normal; overflow-wrap: anywhere; word-break: break-word; }
.record-table th { width: 9.5rem; }
.api-table, .module-table, .search-table { min-width: 700px; }
.api-index-table { min-width: 2200px; }
.api-detail-table { min-width: 760px; }
.api-table .signature-cell code, .api-index-table .signature-cell code { white-space: nowrap; overflow-wrap: normal; word-break: normal; }
.compact-list { margin: 0; padding-left: 16px; }
code, pre, .mono { font-family: ui-monospace, SFMono-Regular, Consolas, "Liberation Mono", monospace; }
code { font-size: 0.94em; }
pre { max-width: 100%; overflow: auto; margin: 0; background: var(--code); border: 1px solid var(--line); border-radius: 4px; padding: 5px 6px; font-size: 0.94rem; line-height: 1.24; }
.signature { background: #f8fafb; border-left: 2px solid var(--accent); }
.api-entry { margin: 8px 0; border-top: 1px solid var(--line); padding-top: 6px; }
.api-entry h3 { display: flex; flex-wrap: wrap; gap: 5px; align-items: baseline; margin: 0 0 4px; }
.kind-mark { display: inline-block; border: 1px solid var(--line); border-radius: 4px; background: var(--panel-soft); color: var(--muted); padding: 0 4px; font-size: 0.78rem; font-weight: 500; }
.mark { text-align: center; color: var(--accent); font-weight: 700; }
.search-form { display: grid; gap: 4px; max-width: 720px; margin-bottom: 8px; }
.searchbox { width: 100%; padding: 5px 6px; border: 1px solid var(--line); border-radius: 4px; font-size: 1rem; }
.result-count { margin: 4px 0; }
.sr-only { position: absolute; width: 1px; height: 1px; overflow: hidden; clip: rect(0, 0, 0, 0); white-space: nowrap; }
@media (max-width: 720px) {
  html { font-size: 13px; }
  header { position: static; grid-template-columns: 1fr; gap: 5px; }
  nav { display: flex; flex-wrap: nowrap; width: 100%; overflow-x: auto; padding-bottom: 1px; }
  nav a { flex: 0 0 auto; }
  main { padding: 10px 8px 24px; }
  h1 { font-size: 1.55rem; }
  .stats { grid-template-columns: repeat(4, minmax(0, 1fr)); }
  .module-table, .search-table { min-width: 560px; }
  .api-table { min-width: 760px; }
  .api-index-table { min-width: 2200px; }
  .record-table { min-width: 500px; }
  .api-detail-table { min-width: 760px; }
}
""".strip()


def search_script(index: list[dict[str, str]]) -> str:
	return """
<script>
const SEARCH_INDEX = __SEARCH_INDEX__;
const input = document.getElementById("search-input");
const results = document.getElementById("search-results");
function render() {
  const query = input.value.trim().toLowerCase();
  const selected = SEARCH_INDEX.filter((item) => {
    if (!query) return true;
    return (item.title + " " + item.text + " " + item.type).toLowerCase().includes(query);
  }).slice(0, 80);
  if (!selected.length) {
    results.innerHTML = '<p class="meta">該当なし</p>';
    return;
  }
  const rows = selected.map((item) => `
    <tr>
      <td><a href="${escapeAttr(item.url)}">${escapeHtml(item.title)}</a></td>
      <td><span class="kind-mark">${escapeHtml(item.type)}</span></td>
      <td>${escapeHtml(item.text)}</td>
    </tr>
  `).join("");
  results.innerHTML = `
    <p class="meta result-count">${selected.length} / ${SEARCH_INDEX.length}</p>
    <div class="table-scroll">
      <table class="search-table">
        <thead><tr><th>Item</th><th>Type</th><th>Text</th></tr></thead>
        <tbody>${rows}</tbody>
      </table>
    </div>
  `;
}
function escapeHtml(value) {
  return value.replace(/[&<>"']/g, (ch) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;"
  }[ch]));
}
function escapeAttr(value) {
  return escapeHtml(value);
}
input.addEventListener("input", render);
render();
</script>
""".strip().replace("__SEARCH_INDEX__", json.dumps(index, ensure_ascii=False))
