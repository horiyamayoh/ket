#!/usr/bin/env python3

from __future__ import annotations

import html
from pathlib import Path
from typing import Any

from site_reference_model import ModuleDoc, SiteModel


def html_escape(value: Any) -> str:
	return html.escape(str(value), quote=True)


def rel_link(from_page: Path, target: str) -> str:
	return (
		Path(target).as_posix()
		if from_page.parent == Path(".")
		else (Path(*([".."] * len(from_page.parent.parts))) / target).as_posix()
	)


def module_by_name(model: SiteModel) -> dict[str, ModuleDoc]:
	return {module.name: module for module in model.modules}


def category_by_id(model: SiteModel) -> dict[str, dict[str, Any]]:
	return {category["id"]: category for category in model.categories}


def use_case_by_id(model: SiteModel) -> dict[str, dict[str, Any]]:
	return {use_case["id"]: use_case for use_case in model.use_cases}


def link_to_module(current: Path, module_name: str, text: str | None = None) -> str:
	return f'<a href="{html_escape(rel_link(current, f"modules/{module_name}.html"))}">{html_escape(text or module_name)}</a>'


def link_to_use_case(current: Path, use_case_id: str, text: str) -> str:
	return f'<a href="{html_escape(rel_link(current, f"use-cases/{use_case_id}.html"))}">{html_escape(text)}</a>'


def link_to_category(current: Path, category_id: str, text: str) -> str:
	return f'<a href="{html_escape(rel_link(current, f"categories/{category_id}.html"))}">{html_escape(text)}</a>'


def render_list(items: list[str] | tuple[str, ...]) -> str:
	if not items:
		return '<p class="meta">なし</p>'
	return "<ul>" + "".join(f"<li>{html_escape(item)}</li>" for item in items) + "</ul>"


def render_compact_list(items: list[str] | tuple[str, ...]) -> str:
	if not items:
		return '<span class="meta">なし</span>'
	return (
		'<ul class="compact-list">'
		+ "".join(f"<li>{html_escape(item)}</li>" for item in items)
		+ "</ul>"
	)


def render_table(headers: tuple[str, ...], rows: list[tuple[str, ...]], class_name: str = "") -> str:
	class_attr = f' class="{html_escape(class_name)}"' if class_name else ""
	header_html = "".join(f"<th>{html_escape(header)}</th>" for header in headers)
	row_html = []
	for row in rows:
		cells = "".join(f"<td>{cell}</td>" for cell in row)
		row_html.append(f"<tr>{cells}</tr>")
	return (
		'<div class="table-scroll">'
		f"<table{class_attr}>"
		f"<thead><tr>{header_html}</tr></thead>"
		f"<tbody>{''.join(row_html)}</tbody>"
		"</table>"
		"</div>"
	)


def render_record_table(rows: list[tuple[str, str]], class_name: str = "") -> str:
	table_class = "record-table"
	if class_name:
		table_class = f"{table_class} {class_name}"
	row_html = []
	for label, value in rows:
		row_html.append(f"<tr><th>{html_escape(label)}</th><td>{value}</td></tr>")
	return (
		'<div class="table-scroll">'
		f'<table class="{html_escape(table_class)}"><tbody>{"".join(row_html)}</tbody></table>'
		"</div>"
	)


def pill(content: str) -> str:
	return f'<span class="pill">{content}</span>'


def module_category_pills(model: SiteModel, current: Path, module: ModuleDoc) -> str:
	categories = category_by_id(model)
	return " ".join(
		pill(link_to_category(current, category_id, categories[category_id]["title"]))
		for category_id in module.sidecar["categories"]
	)


def module_use_case_pills(model: SiteModel, current: Path, module: ModuleDoc) -> str:
	use_cases = use_case_by_id(model)
	return " ".join(
		pill(link_to_use_case(current, use_case_id, use_cases[use_case_id]["title"]))
		for use_case_id in module.sidecar["use_cases"]
	)


def module_key_api_links(current: Path, module: ModuleDoc, limit: int = 4) -> str:
	links = []
	for api in module.apis[:limit]:
		target = f"modules/{module.name}.html#api-{api.kind}-{api.name}-{api.line}"
		links.append(
			f'<a class="mono" href="{html_escape(rel_link(current, target))}">{html_escape(api.name)}</a>'
		)
	if len(module.apis) > limit:
		links.append(f'<span class="meta">+{len(module.apis) - limit}</span>')
	return " ".join(links)


def render_module_table(
	model: SiteModel,
	current: Path,
	modules: tuple[ModuleDoc, ...],
	include_categories: bool,
	include_key_apis: bool,
) -> str:
	headers = ["Module"]
	if include_categories:
		headers.append("Category")
	headers.extend(("C++", "APIs", "Summary"))
	if include_key_apis:
		headers.append("Key APIs")

	rows: list[tuple[str, ...]] = []
	for module in modules:
		cells = [f'<span class="module-name">{link_to_module(current, module.name)}</span>']
		if include_categories:
			cells.append(f'<span class="pillrow">{module_category_pills(model, current, module)}</span>')
		cells.extend(
			(
				f'<span class="count-cell">{html_escape(module.progress.cxx_min)}</span>',
				f'<span class="count-cell">{len(module.apis)}</span>',
				html_escape(module.sidecar["summary"]),
			)
		)
		if include_key_apis:
			cells.append(module_key_api_links(current, module))
		rows.append(tuple(cells))

	return render_table(tuple(headers), rows, "module-table")


def render_module_card(model: SiteModel, current: Path, module: ModuleDoc) -> str:
	categories = category_by_id(model)
	category_links = " ".join(
		link_to_category(current, category_id, categories[category_id]["title"])
		for category_id in module.sidecar["categories"]
	)
	return "\n".join(
		(
			'<section class="card">',
			f"<h3>{link_to_module(current, module.name)}</h3>",
			f"<p>{html_escape(module.sidecar['summary'])}</p>",
			f'<p class="meta">{html_escape(module.progress.category)} / {html_escape(module.progress.cxx_min)} / {len(module.apis)} API</p>',
			f'<div class="pillrow">{category_links}</div>',
			"</section>",
		)
	)
