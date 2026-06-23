#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
from typing import Any

from site_reference_assets import search_script, site_css
from site_reference_html import (
	category_by_id,
	html_escape,
	link_to_category,
	link_to_module,
	link_to_use_case,
	module_by_name,
	module_category_pills,
	module_key_api_links,
	module_use_case_pills,
	pill,
	rel_link,
	render_compact_list,
	render_list,
	render_module_table,
	render_record_table,
	render_table,
	use_case_by_id,
)
from site_reference_model import ApiDoc, ModuleDoc, SiteModel


def page_shell(model: SiteModel, current: Path, title: str, body: str, extra_script: str = "") -> str:
	nav_items = (
		("index.html", "概要"),
		("modules/index.html", "Modules"),
		("api/index.html", "API"),
		("use-cases/index.html", "Use cases"),
		("categories/index.html", "Categories"),
		("diagrams.html", "Diagrams"),
		("search.html", "Search"),
	)
	current_text = current.as_posix()
	nav_parts = []
	for href, label in nav_items:
		section = href.split("/", 1)[0]
		is_current = current_text == href or (
			"/" in href and current_text.startswith(f"{section}/")
		)
		current_attr = ' aria-current="page"' if is_current else ""
		nav_parts.append(
			f'<a href="{html_escape(rel_link(current, href))}"{current_attr}>{html_escape(label)}</a>'
		)
	nav_html = "".join(nav_parts)
	css = site_css()
	return "\n".join(
		(
			"<!doctype html>",
			'<html lang="ja">',
			"<head>",
			'<meta charset="utf-8">',
			'<meta name="viewport" content="width=device-width, initial-scale=1">',
			f"<title>{html_escape(title)} - {html_escape(model.config['title'])}</title>",
			f"<style>{css}</style>",
			"</head>",
			"<body>",
			'<a class="skip-link" href="#main">本文へ移動</a>',
			"<header>",
			f'<div class="brand"><a href="{html_escape(rel_link(current, "index.html"))}">{html_escape(model.config["title"])}</a></div>',
			f"<nav>{nav_html}</nav>",
			"</header>",
			'<main id="main">',
			body,
			"</main>",
			"<footer>Generated from header Doxygen, progress.md, and docs/site_src. Do not edit site HTML by hand.</footer>",
			extra_script,
			"</body>",
			"</html>",
			"",
		)
	)


def render_api_summary(current: Path, module: ModuleDoc) -> str:
	rows: list[tuple[str, ...]] = []
	for api in module.apis:
		target = f"modules/{module.name}.html#api-{api.kind}-{api.name}-{api.line}"
		rows.append(
			(
				f'<span class="api-name"><a href="{html_escape(rel_link(current, target))}">{html_escape(module.name)}::{html_escape(api.name)}</a></span>',
				f'<span class="kind-mark">{html_escape(api.kind)}</span>',
				f'<span class="signature-cell"><code>{html_escape(api.signature)}</code></span>',
				html_escape(api.brief),
			)
		)
	return render_table(("API", "Kind", "Signature", "Brief"), rows, "api-table")


def render_api_detail(api: ApiDoc) -> str:
	anchor = f"api-{api.kind}-{api.name}-{api.line}"
	parts = [
		f'<section class="api-entry" id="{html_escape(anchor)}">',
		f'<h3>{html_escape(api.name)} <span class="kind-mark">{html_escape(api.kind)}</span></h3>',
	]
	rows = [
		("Signature", f'<pre class="signature"><code>{html_escape(api.signature)}</code></pre>'),
		("Brief", html_escape(api.brief)),
	]
	if api.details:
		rows.append(("Details", html_escape(api.details)))
	if api.params:
		rows.append(("Parameters", render_compact_list(api.params)))
	if api.retvals:
		rows.append(("Return / retval", render_compact_list(api.retvals)))
	if api.pre:
		rows.append(("Pre", html_escape(api.pre)))
	if api.post:
		rows.append(("Post", html_escape(api.post)))
	if api.notes:
		rows.append(("Notes", render_compact_list(api.notes)))
	for index, code in enumerate(api.code_blocks, start=1):
		label = "Example" if len(api.code_blocks) == 1 else f"Example {index}"
		rows.append((label, f"<pre><code>{html_escape(code)}</code></pre>"))
	parts.append(render_record_table(rows, "api-detail-table"))
	parts.append("</section>")
	return "\n".join(parts)


def render_index(model: SiteModel) -> str:
	current = Path("index.html")
	use_cases = use_case_by_id(model)
	module_count = len(model.modules)
	api_count = sum(len(module.apis) for module in model.modules)
	body = [
		f"<h1>{html_escape(model.config['title'])}</h1>",
		'<section class="summary-band" aria-label="reference summary">',
		f'<p class="summary-copy"><strong>{html_escape(model.config["subtitle"])}</strong>。{html_escape(model.config["description"])}</p>',
		'<p class="summary-actions">'
		f'<a href="{html_escape(rel_link(current, "use-cases/index.html"))}">Use cases</a>'
		f'<a href="{html_escape(rel_link(current, "modules/index.html"))}">Modules</a>'
		f'<a href="{html_escape(rel_link(current, "api/index.html"))}">API</a>'
		f'<a href="{html_escape(rel_link(current, "search.html"))}">Search</a>'
		"</p>",
		'<dl class="stats">',
		f"<div><dt>Modules</dt><dd>{module_count}</dd></div>",
		f"<div><dt>APIs</dt><dd>{api_count}</dd></div>",
		f"<div><dt>Use cases</dt><dd>{len(model.use_cases)}</dd></div>",
		f"<div><dt>Categories</dt><dd>{len(model.categories)}</dd></div>",
		"</dl>",
		"</section>",
		"<h2>Use Case Entrances</h2>",
		'<div class="grid">',
	]
	for use_case_id in model.config["home_use_cases"]:
		use_case = use_cases[use_case_id]
		body.append(
			'<section class="card">'
			f"<h3>{link_to_use_case(current, use_case_id, use_case['title'])}</h3>"
			f"<p>{html_escape(use_case['summary'])}</p>"
			"</section>"
		)
	body.extend(
		(
			"</div>",
			"<h2>Module Map</h2>",
		)
	)
	body.append(render_module_table(model, current, model.modules, True, False))
	body.extend(
		(
			"<h2>Quick Index</h2>",
			"<p>分類、ユースケース、API名、module名から探せる。</p>",
			'<p><a href="search.html">Search page</a> / <a href="diagrams.html">Diagrams</a></p>',
		)
	)
	return page_shell(model, current, model.config["title"], "\n".join(body))


def render_modules_index(model: SiteModel) -> str:
	current = Path("modules/index.html")
	body = [
		"<h1>Modules</h1>",
		'<p class="lead">実装済みmoduleをcategory、C++要件、API数、主要APIから俯瞰。</p>',
		render_module_table(model, current, model.modules, True, True),
	]
	return page_shell(model, current, "Modules", "\n".join(body))


def render_module_page(model: SiteModel, module: ModuleDoc) -> str:
	current = Path(f"modules/{module.name}.html")
	related_links = " ".join(
		pill(link_to_module(current, related))
		for related in module.sidecar["related_modules"]
	)
	body = [
		f"<h1>ket::{html_escape(module.name)}</h1>",
		f"<p class=\"lead\">{html_escape(module.sidecar['summary'])}</p>",
		"<h2>Module Facts</h2>",
		render_record_table(
			[
				("Header", html_escape(module.header.relative_to(model.root).as_posix())),
				("Status", html_escape(module.progress.status)),
				("C++ Min", html_escape(module.progress.cxx_min)),
				("Tests", html_escape(module.progress.tests)),
				("Progress note", html_escape(module.progress.notes)),
				("Drop-in", html_escape(module.file_doc.pars.get("プロジェクトへの適用方法", ""))),
				("Dependencies", html_escape(module.file_doc.pars.get("他のライブラリへの依存", ""))),
				("Namespace", html_escape(module.file_doc.pars.get("namespace", ""))),
				("Categories", f'<span class="pillrow">{module_category_pills(model, current, module)}</span>'),
				("Use cases", f'<span class="pillrow">{module_use_case_pills(model, current, module)}</span>'),
			]
		),
		"<h2>Use This When</h2>",
		render_list(tuple(module.sidecar["when_to_use"])),
		"<h2>Do Not Use This When</h2>",
		render_list(tuple(module.sidecar["when_not_to_use"])),
		"<h2>Related Modules</h2>",
		f'<p class="pillrow">{related_links}</p>' if related_links else '<p class="meta">なし</p>',
		"<h2>Notes</h2>",
		render_list(tuple(module.sidecar.get("notes", []))),
		"<h2>API Summary</h2>",
		render_api_summary(current, module),
		"<h2>API Details</h2>",
	]
	for api in module.apis:
		body.append(render_api_detail(api))
	return page_shell(model, current, f"ket::{module.name}", "\n".join(body))


def render_api_index(model: SiteModel) -> str:
	current = Path("api/index.html")
	body = ["<h1>API Index</h1>", "<p class=\"lead\">公開headerから抽出したAPI一覧。</p>"]
	rows: list[tuple[str, ...]] = []
	for module in model.modules:
		for api in module.apis:
			target = f"modules/{module.name}.html#api-{api.kind}-{api.name}-{api.line}"
			rows.append(
				(
					f'<span class="module-name">{link_to_module(current, module.name, "ket::" + module.name)}</span>',
					f'<span class="api-name"><a href="{html_escape(rel_link(current, target))}">{html_escape(api.name)}</a></span>',
					f'<span class="kind-mark">{html_escape(api.kind)}</span>',
					f'<span class="signature-cell"><code>{html_escape(api.signature)}</code></span>',
					html_escape(api.brief),
				)
			)
	body.append(render_table(("Module", "API", "Kind", "Signature", "Brief"), rows, "api-index-table"))
	return page_shell(model, current, "API Index", "\n".join(body))


def render_use_cases_index(model: SiteModel) -> str:
	current = Path("use-cases/index.html")
	rows: list[tuple[str, ...]] = []
	modules = module_by_name(model)
	for use_case in model.use_cases:
		module_links = " ".join(
			pill(link_to_module(current, item["module"]))
			for item in use_case["modules"]
			if item["module"] in modules
		)
		rows.append(
			(
				link_to_use_case(current, use_case["id"], use_case["title"]),
				html_escape(use_case["summary"]),
				f'<span class="pillrow">{module_links}</span>',
			)
		)
	body = [
		"<h1>Use Cases</h1>",
		'<p class="lead">用途からmodule候補へ進むための入口一覧。</p>',
		render_table(("Use case", "Summary", "Modules"), rows, "module-table"),
	]
	return page_shell(model, current, "Use Cases", "\n".join(body))


def render_use_case_page(model: SiteModel, use_case: dict[str, Any]) -> str:
	current = Path(f"use-cases/{use_case['id']}.html")
	use_cases = use_case_by_id(model)
	body = [
		f"<h1>{html_escape(use_case['title'])}</h1>",
		f"<p class=\"lead\">{html_escape(use_case['summary'])}</p>",
		"<h2>Decision Flow</h2>",
		render_list(tuple(use_case["flow"])),
		"<h2>Modules</h2>",
	]
	rows: list[tuple[str, ...]] = []
	for item in use_case["modules"]:
		module = module_by_name(model)[item["module"]]
		rows.append(
			(
				f'<span class="module-name">{link_to_module(current, item["module"])}</span>',
				html_escape(item["role"]),
				html_escape(module.sidecar["summary"]),
				module_key_api_links(current, module),
			)
		)
	body.append(render_table(("Module", "Role", "Summary", "Key APIs"), rows, "module-table"))
	body.extend(
		(
			"<h2>Recipe</h2>",
			render_list(tuple(use_case["recipe"])),
			"<h2>See Also</h2>",
			'<p class="pillrow">'
			+ " ".join(
				pill(link_to_use_case(current, use_case_id, use_cases[use_case_id]["title"]))
				for use_case_id in use_case.get("see_also", [])
			)
			+ "</p>",
		)
	)
	return page_shell(model, current, use_case["title"], "\n".join(body))


def render_categories_index(model: SiteModel) -> str:
	current = Path("categories/index.html")
	rows: list[tuple[str, ...]] = []
	for category in model.categories:
		module_links = " ".join(
			pill(link_to_module(current, module_name)) for module_name in category["modules"]
		)
		rows.append(
			(
				link_to_category(current, category["id"], category["title"]),
				html_escape(category["summary"]),
				str(len(category["modules"])),
				f'<span class="pillrow">{module_links}</span>',
			)
		)
	body = [
		"<h1>Categories</h1>",
		'<p class="lead">論理分類ごとのmodule集合。</p>',
		render_table(("Category", "Summary", "Modules", "Module list"), rows, "module-table"),
	]
	return page_shell(model, current, "Categories", "\n".join(body))


def render_category_page(model: SiteModel, category: dict[str, Any]) -> str:
	current = Path(f"categories/{category['id']}.html")
	modules = module_by_name(model)
	category_modules = tuple(modules[module_name] for module_name in category["modules"])
	body = [
		f"<h1>{html_escape(category['title'])}</h1>",
		f"<p class=\"lead\">{html_escape(category['summary'])}</p>",
		render_module_table(model, current, category_modules, False, True),
	]
	return page_shell(model, current, category["title"], "\n".join(body))


def render_diagrams(model: SiteModel) -> str:
	current = Path("diagrams.html")
	modules = module_by_name(model)
	body = [
		"<h1>Diagrams</h1>",
		"<p class=\"lead\">依存図ではなく、用途からmoduleへ進むための図表。</p>",
		"<h2>Input Decision Guide</h2>",
	]
	body.append(
		render_table(
			("入口", "見るmodule", "判断の軸"),
			[
				(
					"文字列入力",
					" ".join(
						pill(link_to_module(current, module_name))
						for module_name in ("parse", "cli", "ascii", "utf8")
					),
					"外部入力、ASCII制約、UTF-8境界、CLI引数。",
				),
				(
					"byte列",
					" ".join(
						pill(link_to_module(current, module_name))
						for module_name in ("byte_view", "byte_reader", "byte_writer", "endian")
					),
					"buffer所有権、offset、endian、逐次読み書き。",
				),
				(
					"値domain",
					" ".join(
						pill(link_to_module(current, module_name))
						for module_name in ("port", "ipv4", "version", "color")
					),
					"入力値をdomain型や検証済み値へ落とす境界。",
				),
				(
					"処理flow",
					" ".join(
						pill(link_to_module(current, module_name))
						for module_name in ("scope", "optional", "state", "contract")
					),
					"寿命管理、fallback、状態遷移、契約明示。",
				),
			],
			"module-table",
		)
	)
	body.append("<h2>Use Case Routes</h2>")
	rows: list[tuple[str, ...]] = []
	for use_case in model.use_cases:
		module_links = " ".join(
			pill(link_to_module(current, item["module"])) for item in use_case["modules"]
		)
		category_ids = sorted(
			{
				category_id
				for item in use_case["modules"]
				for category_id in modules[item["module"]].sidecar["categories"]
			}
		)
		category_links = " ".join(
			pill(link_to_category(current, category_id, category_by_id(model)[category_id]["title"]))
			for category_id in category_ids
		)
		rows.append(
			(
				link_to_use_case(current, use_case["id"], use_case["title"]),
				html_escape(use_case["summary"]),
				f'<span class="pillrow">{module_links}</span>',
				f'<span class="pillrow">{category_links}</span>',
			)
		)
	body.append(render_table(("Use case", "Summary", "Modules", "Categories"), rows, "module-table"))
	body.append("<h2>Category Landscape</h2>")
	rows = []
	for category in model.categories:
		module_links = " ".join(
			pill(link_to_module(current, module_name))
			for module_name in category["modules"]
		)
		rows.append(
			(
				link_to_category(current, category["id"], category["title"]),
				html_escape(category["summary"]),
				str(len(category["modules"])),
				f'<span class="pillrow">{module_links}</span>',
			)
		)
	body.append(render_table(("Category", "Summary", "Modules", "Module list"), rows, "module-table"))
	return page_shell(model, current, "Diagrams", "\n".join(body))


def search_index(model: SiteModel) -> list[dict[str, str]]:
	items: list[dict[str, str]] = []
	for module in model.modules:
		items.append(
			{
				"type": "module",
				"title": f"ket::{module.name}",
				"text": module.sidecar["summary"],
				"url": f"modules/{module.name}.html",
			}
		)
		for api in module.apis:
			items.append(
				{
					"type": "api",
					"title": f"ket::{module.name}::{api.name}",
					"text": f"{api.brief} {api.signature}",
					"url": f"modules/{module.name}.html#api-{api.kind}-{api.name}-{api.line}",
				}
			)
	for use_case in model.use_cases:
		items.append(
			{
				"type": "use-case",
				"title": use_case["title"],
				"text": use_case["summary"],
				"url": f"use-cases/{use_case['id']}.html",
			}
		)
	for category in model.categories:
		items.append(
			{
				"type": "category",
				"title": category["title"],
				"text": category["summary"],
				"url": f"categories/{category['id']}.html",
			}
		)
	return items


def render_search(model: SiteModel) -> str:
	current = Path("search.html")
	index = search_index(model)
	body = [
		"<h1>Search</h1>",
		'<div class="search-form">',
		'<label class="sr-only" for="search-input">検索語</label>',
		'<input class="searchbox" id="search-input" type="search" placeholder="module, API, use case を検索" autofocus>',
		"</div>",
		'<div id="search-results"></div>',
		"<noscript>",
		"<h2>Static Index</h2>",
	]
	rows: list[tuple[str, ...]] = []
	for item in index:
		rows.append(
			(
				f'<a href="{html_escape(item["url"])}">{html_escape(item["title"])}</a>',
				f'<span class="kind-mark">{html_escape(item["type"])}</span>',
				html_escape(item["text"]),
			)
		)
	body.append(render_table(("Item", "Type", "Text"), rows, "search-table"))
	body.append("</noscript>")
	return page_shell(model, current, "Search", "\n".join(body), search_script(index))
