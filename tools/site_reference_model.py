#!/usr/bin/env python3

from __future__ import annotations

import json
import re
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
