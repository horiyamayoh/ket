#!/usr/bin/env python3

from __future__ import annotations

import html
import json
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import check_conventions
import ket_tooling


SITE_SRC_DIR = "docs/site_src"
SITE_OUTPUT_DIR = "site"


@dataclass(frozen=True)
class ProgressEntry:
	name: str
	category: str
	status: str
	cxx_min: str
	tests: str
	format_status: str
	notes: str


@dataclass(frozen=True)
class ParsedDoxygen:
	brief: str
	details: str
	params: tuple[str, ...]
	retvals: tuple[str, ...]
	pre: str
	post: str
	notes: tuple[str, ...]
	code_blocks: tuple[str, ...]
	pars: dict[str, str]


@dataclass(frozen=True)
class ApiDoc:
	name: str
	kind: str
	signature: str
	brief: str
	details: str
	params: tuple[str, ...]
	retvals: tuple[str, ...]
	pre: str
	post: str
	notes: tuple[str, ...]
	code_blocks: tuple[str, ...]
	line: int


@dataclass(frozen=True)
class ModuleDoc:
	name: str
	header: Path
	progress: ProgressEntry
	sidecar: dict[str, Any]
	file_doc: ParsedDoxygen
	apis: tuple[ApiDoc, ...]


@dataclass(frozen=True)
class SiteModel:
	root: Path
	config: dict[str, Any]
	categories: tuple[dict[str, Any], ...]
	use_cases: tuple[dict[str, Any], ...]
	modules: tuple[ModuleDoc, ...]


def html_escape(value: Any) -> str:
	return html.escape(str(value), quote=True)


def read_json(path: Path) -> dict[str, Any]:
	with path.open(encoding="utf-8") as file:
		value = json.load(file)
	if not isinstance(value, dict):
		raise ValueError(f"{ket_tooling.relative(path)} must contain a JSON object.")
	return value


def parse_progress(root: Path = ket_tooling.ROOT) -> dict[str, ProgressEntry]:
	progress_path = root / "progress.md"
	entries: dict[str, ProgressEntry] = {}

	for line in progress_path.read_text(encoding="utf-8").splitlines():
		stripped = line.strip()
		if not stripped.startswith("|"):
			continue
		if "---" in stripped:
			continue

		cells = [cell.strip() for cell in stripped.strip("|").split("|")]
		if len(cells) < 7 or cells[0] == "Module":
			continue

		name = cells[0]
		entries[name] = ProgressEntry(
			name=name,
			category=cells[1],
			status=cells[2],
			cxx_min=cells[3],
			tests=cells[4],
			format_status=cells[5],
			notes=cells[6],
		)

	return entries


def clean_doxygen_lines(comment: str) -> list[str]:
	lines: list[str] = []
	for raw_line in comment.splitlines():
		line = raw_line.strip()
		if line in ("/**", "*/"):
			continue
		if line.startswith("*"):
			line = line[1:]
			if line.startswith(" "):
				line = line[1:]
		lines.append(line.rstrip())
	return lines


def _append_text(current: str, addition: str) -> str:
	if not addition:
		return current
	if not current:
		return addition
	return f"{current} {addition}"


def parse_doxygen(comment: str | None) -> ParsedDoxygen:
	if comment is None:
		comment = ""

	brief = ""
	details = ""
	params: list[str] = []
	retvals: list[str] = []
	pre = ""
	post = ""
	notes: list[str] = []
	code_blocks: list[str] = []
	pars: dict[str, str] = {}

	current_kind: str | None = None
	current_key: str | None = None
	in_code = False
	code_lines: list[str] = []

	for line in clean_doxygen_lines(comment):
		stripped = line.strip()

		if stripped.startswith("@code"):
			in_code = True
			code_lines = []
			current_kind = None
			current_key = None
			continue
		if stripped.startswith("@endcode"):
			code_blocks.append("\n".join(code_lines).strip("\n"))
			in_code = False
			continue
		if in_code:
			code_lines.append(line)
			continue

		if stripped.startswith("@brief"):
			brief = stripped[len("@brief") :].strip()
			current_kind = "brief"
			current_key = None
			continue
		if stripped.startswith("@details"):
			details = stripped[len("@details") :].strip()
			current_kind = "details"
			current_key = None
			continue
		if stripped.startswith("@param"):
			match = re.match(r"@param(?:\[([^\]]+)\])?\s+(\S+)\s*(.*)", stripped)
			if match is not None:
				direction = match.group(1) or ""
				name = match.group(2)
				text = match.group(3).strip()
				prefix = f"{name}"
				if direction:
					prefix = f"{direction} {prefix}"
				params.append(_append_text(prefix + ":", text))
				current_kind = "params"
				current_key = None
			continue
		if stripped.startswith("@retval"):
			match = re.match(r"@retval\s+(\S+)\s*(.*)", stripped)
			if match is not None:
				retvals.append(_append_text(match.group(1) + ":", match.group(2).strip()))
				current_kind = "retvals"
				current_key = None
			continue
		if stripped.startswith("@return"):
			text = stripped[len("@return") :].strip()
			retvals.append(_append_text("return:", text))
			current_kind = "retvals"
			current_key = None
			continue
		if stripped.startswith("@pre"):
			pre = stripped[len("@pre") :].strip()
			current_kind = "pre"
			current_key = None
			continue
		if stripped.startswith("@post"):
			post = stripped[len("@post") :].strip()
			current_kind = "post"
			current_key = None
			continue
		if stripped.startswith("@note"):
			notes.append(stripped[len("@note") :].strip())
			current_kind = "notes"
			current_key = None
			continue
		if stripped.startswith("@par"):
			key = stripped[len("@par") :].strip()
			pars[key] = ""
			current_kind = "par"
			current_key = key
			continue
		if stripped.startswith("@"):
			current_kind = None
			current_key = None
			continue

		if current_kind == "brief":
			brief = _append_text(brief, stripped)
		elif current_kind == "details":
			details = _append_text(details, stripped)
		elif current_kind == "params" and params:
			params[-1] = _append_text(params[-1], stripped)
		elif current_kind == "retvals" and retvals:
			retvals[-1] = _append_text(retvals[-1], stripped)
		elif current_kind == "pre":
			pre = _append_text(pre, stripped)
		elif current_kind == "post":
			post = _append_text(post, stripped)
		elif current_kind == "notes" and notes:
			notes[-1] = _append_text(notes[-1], stripped)
		elif current_kind == "par" and current_key is not None:
			pars[current_key] = _append_text(pars[current_key], stripped)

	return ParsedDoxygen(
		brief=brief,
		details=details,
		params=tuple(params),
		retvals=tuple(retvals),
		pre=pre,
		post=post,
		notes=tuple(notes),
		code_blocks=tuple(code_blocks),
		pars=pars,
	)


def first_doxygen_block(lines: list[str]) -> str | None:
	for index, line in enumerate(lines[:80]):
		if line.strip() != "/**":
			continue

		block: list[str] = []
		for block_line in lines[index:]:
			block.append(block_line)
			if block_line.strip() == "*/":
				return "\n".join(block)
	return None


def doxygen_block_at(lines: list[str], index: int) -> tuple[str, int] | None:
	if lines[index].strip() != "/**":
		return None

	block: list[str] = []
	for cursor in range(index, len(lines)):
		block.append(lines[cursor])
		if lines[cursor].strip() == "*/":
			return "\n".join(block), cursor

	return None


def def_signature_from_doxygen(comment: str) -> str | None:
	for line in clean_doxygen_lines(comment):
		match = re.match(r"@def\s+(KET_[A-Za-z_][A-Za-z0-9_]*(?:\([^)]*\))?)\s*$", line.strip())
		if match is not None:
			return f"#define {match.group(1)}"
	return None


def extract_name_from_signature(signature: str) -> str:
	without_suffix = signature.rstrip(";{").strip()
	macro_match = re.match(r"#define\s+([A-Za-z_][A-Za-z0-9_]*)", without_suffix)
	if macro_match is not None:
		return macro_match.group(1)

	if "(" in without_suffix:
		before_paren = without_suffix.rsplit("(", 1)[0].strip()
		return before_paren.split()[-1].split("::")[-1]

	using_match = re.search(r"\busing\s+([A-Za-z_][A-Za-z0-9_]*)\s*=", without_suffix)
	if using_match is not None:
		return using_match.group(1)

	type_match = re.search(
		r"\b(?:struct|class)\s+([A-Za-z_][A-Za-z0-9_]*)\b|\benum(?:\s+class)?\s+([A-Za-z_][A-Za-z0-9_]*)\b",
		without_suffix,
	)
	if type_match is not None:
		return next(group for group in type_match.groups() if group is not None)

	return without_suffix.split()[0] if without_suffix else ""


def alias_declaration_at(lines: list[str], index: int) -> tuple[str, int] | None:
	stripped = lines[index].strip()
	if not stripped or stripped.startswith(("#", "*", "//")):
		return None

	signature_lines: list[str] = []
	cursor = index
	if stripped.startswith("template "):
		signature_lines.append(stripped)
		cursor += 1
		while cursor < len(lines) and lines[cursor].strip() == "":
			cursor += 1
		if cursor >= len(lines):
			return None

	current = lines[cursor].strip()
	if not current.startswith("using "):
		return None

	while cursor < len(lines):
		current = lines[cursor].strip()
		if current.startswith(("#", "*", "//")):
			break
		signature_lines.append(current)
		if current.endswith(";"):
			signature = check_conventions.normalize_signature(signature_lines)
			if re.search(r"\busing\s+[A-Za-z_][A-Za-z0-9_]*\s*=", signature):
				return signature, cursor
			return None
		cursor += 1

	return None


def macro_declaration_at(lines: list[str], index: int) -> tuple[str, int] | None:
	stripped = lines[index].strip()
	if not stripped.startswith("#define KET_"):
		return None

	signature_lines = [stripped.rstrip("\\").strip()]
	cursor = index
	while lines[cursor].rstrip().endswith("\\") and cursor + 1 < len(lines):
		cursor += 1
		continuation = lines[cursor].strip().rstrip("\\").strip()
		if continuation:
			signature_lines.append(continuation)

	return " ".join(signature_lines), cursor


def api_doc_from_signature(
	name: str,
	kind: str,
	signature: str,
	comment: str | None,
	line: int,
) -> ApiDoc:
	parsed = parse_doxygen(comment)
	return ApiDoc(
		name=name,
		kind=kind,
		signature=signature.rstrip(";").strip(),
		brief=parsed.brief,
		details=parsed.details,
		params=parsed.params,
		retvals=parsed.retvals,
		pre=parsed.pre,
		post=parsed.post,
		notes=parsed.notes,
		code_blocks=parsed.code_blocks,
		line=line,
	)


def public_section_ranges(lines: list[str]) -> list[tuple[int, int]]:
	ranges: list[tuple[int, int]] = []
	for title in ("Public API declarations", "Public API definitions"):
		indices = check_conventions.section_title_indices(lines, title)
		if not indices:
			continue

		start = indices[0]
		next_sections = [
			index
			for section_title in check_conventions.HEADER_SECTIONS
			for index in check_conventions.section_title_indices(lines, section_title)
			if index > start
		]
		end = min(next_sections) if next_sections else len(lines)
		ranges.append((start, end))
	return ranges


def index_in_ranges(index: int, ranges: list[tuple[int, int]]) -> bool:
	return any(start < index < end for start, end in ranges)


def extract_public_api(header: Path) -> tuple[ParsedDoxygen, tuple[ApiDoc, ...]]:
	lines = check_conventions.read_lines(header)
	file_doc = parse_doxygen(first_doxygen_block(lines))
	ranges = public_section_ranges(lines)
	apis: list[ApiDoc] = []
	seen: set[tuple[str, str, str]] = set()
	index = 0

	while index < len(lines):
		if not index_in_ranges(index, ranges):
			index += 1
			continue

		doxygen = doxygen_block_at(lines, index)
		if doxygen is not None:
			comment, end_index = doxygen
			signature = def_signature_from_doxygen(comment)
			if signature is not None:
				name = extract_name_from_signature(signature)
				key = ("macro", name, signature)
				if key not in seen:
					apis.append(api_doc_from_signature(name, "macro", signature, comment, index + 1))
					seen.add(key)
			index = end_index + 1
			continue

		alias = alias_declaration_at(lines, index)
		if alias is not None:
			signature, end_index = alias
			comment = check_conventions.previous_doxygen(lines, index)
			name = extract_name_from_signature(signature)
			key = ("alias", name, signature)
			if comment is not None and key not in seen:
				apis.append(api_doc_from_signature(name, "alias", signature, comment, index + 1))
				seen.add(key)
			index = end_index + 1
			continue

		type_declaration = check_conventions.type_declaration_at(lines, index)
		if type_declaration is not None:
			signature, end_index = type_declaration
			access = check_conventions.member_access_before(lines, index)
			if access in (None, "public"):
				comment = check_conventions.previous_doxygen(lines, index)
				name = extract_name_from_signature(signature)
				key = ("type", name, signature)
				if comment is not None and key not in seen:
					apis.append(api_doc_from_signature(name, "type", signature, comment, index + 1))
					seen.add(key)
			index = end_index + 1
			continue

		function = check_conventions.function_signature_at(lines, index)
		if function is not None:
			if check_conventions.is_public_api_function(header, lines, index):
				comment = check_conventions.previous_doxygen(lines, index)
				name = extract_name_from_signature(function.text)
				key = ("function", name, check_conventions.canonical_function_signature(function.text))
				if comment is not None and key not in seen:
					apis.append(
						api_doc_from_signature(name, "function", function.text, comment, index + 1)
					)
					seen.add(key)
			index = function.end_index + 1
			continue

		index += 1

	index = 0
	while index < len(lines):
		macro = macro_declaration_at(lines, index)
		if macro is None:
			index += 1
			continue

		signature, end_index = macro
		comment = check_conventions.previous_doxygen(lines, index)
		name = extract_name_from_signature(signature)
		key = ("macro", name, signature)
		if comment is not None and key not in seen:
			apis.append(api_doc_from_signature(name, "macro", signature, comment, index + 1))
			seen.add(key)
		index = end_index + 1

	return file_doc, tuple(apis)


def load_site_model(root: Path = ket_tooling.ROOT) -> SiteModel:
	src_dir = root / SITE_SRC_DIR
	config = read_json(src_dir / "site.json")
	category_root = read_json(src_dir / "categories.json")
	categories = tuple(category_root.get("categories", []))
	use_cases = tuple(
		read_json(path) for path in sorted((src_dir / "use_cases").glob("*.json"))
	)
	progress = parse_progress(root)

	module_docs: list[ModuleDoc] = []
	module_order = config.get("module_order", sorted(progress))
	for module_name in module_order:
		progress_entry = progress[module_name]
		sidecar = read_json(src_dir / "modules" / f"{module_name}.json")
		header = root / "modules" / module_name / f"ket_{module_name}.h"
		file_doc, apis = extract_public_api(header)
		module_docs.append(
			ModuleDoc(
				name=module_name,
				header=header,
				progress=progress_entry,
				sidecar=sidecar,
				file_doc=file_doc,
				apis=apis,
			)
		)

	return SiteModel(
		root=root,
		config=config,
		categories=categories,
		use_cases=use_cases,
		modules=tuple(module_docs),
	)


def require_string_list(value: Any, path: str, errors: list[str]) -> tuple[str, ...]:
	if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
		errors.append(f"{path} must be a list of strings.")
		return ()
	return tuple(value)


def validate_model(model: SiteModel) -> list[str]:
	errors: list[str] = []
	progress = parse_progress(model.root)
	progress_modules = set(progress)
	model_modules = {module.name for module in model.modules}
	module_sidecars = {
		path.stem for path in (model.root / SITE_SRC_DIR / "modules").glob("*.json")
	}

	missing_sidecars = sorted(progress_modules - module_sidecars)
	for module_name in missing_sidecars:
		errors.append(f"docs/site_src/modules/{module_name}.json is missing.")

	extra_sidecars = sorted(module_sidecars - progress_modules)
	for module_name in extra_sidecars:
		errors.append(f"docs/site_src/modules/{module_name}.json has no progress.md module.")

	for module in model.modules:
		sidecar_module = module.sidecar.get("module")
		if sidecar_module != module.name:
			errors.append(f"{module.name}: sidecar module field must be '{module.name}'.")
		if not module.apis:
			errors.append(f"{module.name}: public API extraction returned no entries.")

	category_ids = {
		category.get("id")
		for category in model.categories
		if isinstance(category.get("id"), str)
	}
	use_case_ids = {
		use_case.get("id")
		for use_case in model.use_cases
		if isinstance(use_case.get("id"), str)
	}

	categorized_modules: set[str] = set()
	for category in model.categories:
		category_id = category.get("id", "<missing>")
		modules = require_string_list(category.get("modules"), f"category {category_id}.modules", errors)
		for module_name in modules:
			if module_name not in progress_modules:
				errors.append(f"category {category_id} references unknown module '{module_name}'.")
			categorized_modules.add(module_name)

	for module in model.modules:
		for category_id in require_string_list(
			module.sidecar.get("categories"), f"module {module.name}.categories", errors
		):
			if category_id not in category_ids:
				errors.append(f"module {module.name} references unknown category '{category_id}'.")
		for use_case_id in require_string_list(
			module.sidecar.get("use_cases"), f"module {module.name}.use_cases", errors
		):
			if use_case_id not in use_case_ids:
				errors.append(f"module {module.name} references unknown use-case '{use_case_id}'.")
		for related_module in require_string_list(
			module.sidecar.get("related_modules"), f"module {module.name}.related_modules", errors
		):
			if related_module not in progress_modules:
				errors.append(f"module {module.name} references unknown module '{related_module}'.")

	for use_case in model.use_cases:
		use_case_id = use_case.get("id", "<missing>")
		if use_case_id not in use_case_ids:
			errors.append(f"use-case file has invalid id '{use_case_id}'.")
		for item in use_case.get("modules", []):
			if not isinstance(item, dict) or not isinstance(item.get("module"), str):
				errors.append(f"use-case {use_case_id}.modules must contain module objects.")
				continue
			module_name = item["module"]
			if module_name not in progress_modules:
				errors.append(f"use-case {use_case_id} references unknown module '{module_name}'.")
		for other_use_case in require_string_list(
			use_case.get("see_also", []), f"use-case {use_case_id}.see_also", errors
		):
			if other_use_case not in use_case_ids:
				errors.append(f"use-case {use_case_id} references unknown use-case '{other_use_case}'.")

	for module_name in sorted(progress_modules - categorized_modules):
		errors.append(f"module {module_name} is not present in any category.")

	return errors


def rel_link(from_page: Path, target: str) -> str:
	return Path(target).as_posix() if from_page.parent == Path(".") else (
		Path(*([".."] * len(from_page.parent.parts))) / target
	).as_posix()


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
		return "<p class=\"meta\">なし</p>"
	return "<ul>" + "".join(f"<li>{html_escape(item)}</li>" for item in items) + "</ul>"


def render_compact_list(items: list[str] | tuple[str, ...]) -> str:
	if not items:
		return '<span class="meta">なし</span>'
	return '<ul class="compact-list">' + "".join(f"<li>{html_escape(item)}</li>" for item in items) + "</ul>"


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
	script = """
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
	return page_shell(model, current, "Search", "\n".join(body), script)


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


def iter_files(root: Path) -> list[Path]:
	if not root.exists():
		return []
	return sorted(path for path in root.rglob("*") if path.is_file())


def compare_directories(expected: Path, actual: Path) -> list[str]:
	errors: list[str] = []
	expected_files = {path.relative_to(expected).as_posix(): path for path in iter_files(expected)}
	actual_files = {path.relative_to(actual).as_posix(): path for path in iter_files(actual)}

	for relative_path in sorted(expected_files.keys() - actual_files.keys()):
		errors.append(f"committed site is missing generated file {relative_path}.")
	for relative_path in sorted(actual_files.keys() - expected_files.keys()):
		errors.append(f"committed site has stale extra file {relative_path}.")
	for relative_path in sorted(expected_files.keys() & actual_files.keys()):
		if expected_files[relative_path].read_bytes() != actual_files[relative_path].read_bytes():
			errors.append(f"committed site differs from generated output: {relative_path}.")

	return errors


HREF_PATTERN = re.compile(r'href="([^"]+)"')


def validate_html_links(site_dir: Path) -> list[str]:
	errors: list[str] = []
	for path in iter_files(site_dir):
		if path.suffix != ".html":
			continue
		text = path.read_text(encoding="utf-8")
		for href in HREF_PATTERN.findall(text):
			if "${" in href:
				continue
			if not href or href.startswith("#") or ":" in href:
				continue
			target_part = href.split("#", 1)[0]
			if not target_part:
				continue
			target = (path.parent / target_part).resolve()
			try:
				target.relative_to(site_dir.resolve())
			except ValueError:
				errors.append(f"{path.relative_to(site_dir)} links outside site: {href}")
				continue
			if not target.exists():
				errors.append(f"{path.relative_to(site_dir)} has broken link: {href}")
	return errors
