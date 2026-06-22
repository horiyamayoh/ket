#!/usr/bin/env python3

from __future__ import annotations

import shutil
from pathlib import Path

import ket_tooling
from site_reference_assets import search_script, site_css
from site_reference_check import (
	HREF_PATTERN,
	compare_directories,
	iter_files,
	validate_html_links,
	validate_html_structure,
)
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
	render_module_card,
	render_module_table,
	render_record_table,
	render_table,
	use_case_by_id,
)
from site_reference_model import (
	SITE_OUTPUT_DIR,
	SITE_SRC_DIR,
	ApiDoc,
	ModuleDoc,
	ParsedDoxygen,
	ProgressEntry,
	SiteModel,
	_append_text,
	alias_declaration_at,
	api_doc_from_signature,
	clean_doxygen_lines,
	def_signature_from_doxygen,
	doxygen_block_at,
	extract_name_from_signature,
	extract_public_api,
	first_doxygen_block,
	index_in_ranges,
	load_site_model,
	macro_declaration_at,
	parse_doxygen,
	parse_progress,
	public_section_ranges,
	read_json,
	require_string_list,
	validate_model,
)
from site_reference_render import (
	page_shell,
	render_api_detail,
	render_api_index,
	render_api_summary,
	render_categories_index,
	render_category_page,
	render_diagrams,
	render_index,
	render_module_page,
	render_modules_index,
	render_search,
	render_use_case_page,
	render_use_cases_index,
	search_index,
)


def write_page(output_dir: Path, relative_path: str, content: str) -> None:
	path = output_dir / relative_path
	path.parent.mkdir(parents=True, exist_ok=True)
	path.write_text(content, encoding="utf-8")


def generate_site(output_dir: Path | None = None, root: Path = ket_tooling.ROOT) -> None:
	model = load_site_model(root)
	errors = validate_model(model)
	if errors:
		raise RuntimeError("\n".join(errors))

	if output_dir is None:
		output_dir = root / SITE_OUTPUT_DIR
	if output_dir.exists():
		shutil.rmtree(output_dir)
	output_dir.mkdir(parents=True)

	write_page(output_dir, "index.html", render_index(model))
	write_page(output_dir, "modules/index.html", render_modules_index(model))
	for module in model.modules:
		write_page(output_dir, f"modules/{module.name}.html", render_module_page(model, module))
	write_page(output_dir, "api/index.html", render_api_index(model))
	write_page(output_dir, "use-cases/index.html", render_use_cases_index(model))
	for use_case in model.use_cases:
		write_page(output_dir, f"use-cases/{use_case['id']}.html", render_use_case_page(model, use_case))
	write_page(output_dir, "categories/index.html", render_categories_index(model))
	for category in model.categories:
		write_page(output_dir, f"categories/{category['id']}.html", render_category_page(model, category))
	write_page(output_dir, "diagrams.html", render_diagrams(model))
	write_page(output_dir, "search.html", render_search(model))
