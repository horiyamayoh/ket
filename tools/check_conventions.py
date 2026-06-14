#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from pathlib import Path

import ket_tooling


POLITE_WORDS = ("です", "ます", "ください")
TEST_TAGS = ("@test", "@brief", "@details", "@pre", "@post")
FUNCTION_TAGS = ("@brief", "@retval", "@pre", "@post")
TYPE_TAGS = ("@brief",)
ALLOWED_CONDITION_CALLS = {"alignof", "sizeof"}
SECTION_BANNER_LINE = "\t// -----------------------------------------------------------------------------"
HEADER_SECTIONS = (
	"Public API declarations",
	"Internal implementation details",
	"Public API definitions",
)
CONTROL_PREFIXES = ("if ", "while ", "for ", "switch ", "return ", "throw ", "static_assert")
MACRO_PREFIXES = ("TEST", "EXPECT_", "ASSERT_")
TYPE_DECLARATION_PATTERN = re.compile(
	r"^(?:template\s*<[^>]+>\s*)?(?:(?:struct|class)\s+[A-Za-z_][A-Za-z0-9_]*|enum(?:\s+class)?\s+[A-Za-z_][A-Za-z0-9_]*)\b"
)


def read_lines(path: Path) -> list[str]:
	return path.read_text(encoding="utf-8").splitlines()


def previous_doxygen(lines: list[str], index: int) -> str | None:
	cursor = index - 1
	while cursor >= 0 and lines[cursor].strip() == "":
		cursor -= 1

	if cursor < 0 or lines[cursor].strip() != "*/":
		return None

	end = cursor
	while cursor >= 0 and lines[cursor].strip() != "/**":
		cursor -= 1

	if cursor < 0:
		return None

	return "\n".join(lines[cursor : end + 1])


def add_error(errors: list[str], path: Path, line_number: int, message: str) -> None:
	errors.append(f"{ket_tooling.relative(path)}:{line_number}: {message}")


def is_module_public_header(path: Path) -> bool:
	parts = path.parts
	for index, part in enumerate(parts[:-2]):
		if part != "modules":
			continue

		module_name = parts[index + 1]
		header_name = parts[index + 2]
		if header_name == f"ket_{module_name}.h":
			return True

	return False


def has_namespace_ket(lines: list[str]) -> bool:
	return any(line.strip() == "namespace ket" for line in lines)


def check_test_comments(path: Path, lines: list[str], errors: list[str]) -> None:
	test_pattern = re.compile(r"^\s*TEST(?:_F|_P)?\s*\(")

	for index, line in enumerate(lines):
		if not test_pattern.search(line):
			continue

		comment = previous_doxygen(lines, index)
		if comment is None:
			add_error(errors, path, index + 1, "TEST requires a Doxygen test specification.")
			continue

		for tag in TEST_TAGS:
			if tag not in comment:
				add_error(errors, path, index + 1, f"TEST Doxygen comment requires {tag}.")


def normalize_signature(lines: list[str]) -> str:
	return " ".join(line.strip() for line in lines if line.strip())


def canonical_function_signature(signature: str) -> str:
	stripped = re.sub(r"\s+", " ", signature.strip())
	return stripped.rstrip("{;").strip()


def line_looks_like_function_signature(signature: str) -> bool:
	stripped = signature.strip()
	if "(" not in stripped or ")" not in stripped:
		return False
	if stripped.startswith(CONTROL_PREFIXES):
		return False
	if stripped.startswith(MACRO_PREFIXES):
		return False
	if stripped.startswith(("#", "*", "//")):
		return False

	return bool(
		re.match(
			r"^(?:template\s*<[^>]+>\s*)?(?:\[\[[^\]]+\]\]\s*)?(?:[\w:<>~*&,\[\],\s]+\s+)+[A-Za-z_][A-Za-z0-9_:]*\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept(?:\([^)]*\))?\s*)?(?:;)?\s*$",
			stripped,
		)
	)


def next_significant_line(lines: list[str], index: int) -> str:
	cursor = index + 1
	while cursor < len(lines) and lines[cursor].strip() == "":
		cursor += 1

	if cursor >= len(lines):
		return ""

	return lines[cursor].strip()


def function_has_parameters(signature: str) -> bool:
	match = re.search(r"\(([^()]*)\)", signature)
	if not match:
		return False

	parameters = match.group(1).strip()
	return parameters not in ("", "void")


def function_signature_is_definition(lines: list[str], signature_end: int) -> bool:
	stripped = lines[signature_end].strip()
	if stripped.endswith("{"):
		return True

	return next_significant_line(lines, signature_end) == "{"


def function_signature_at(lines: list[str], index: int) -> tuple[str, int] | None:
	stripped = lines[index].strip()
	if not stripped:
		return None
	if stripped.startswith(("#", "*", "//")):
		return None
	if stripped.startswith(CONTROL_PREFIXES) or stripped.startswith(MACRO_PREFIXES):
		return None

	signature_lines: list[str] = []
	paren_depth = 0
	saw_paren = False
	cursor = index

	while cursor < len(lines):
		current = lines[cursor].strip()
		if not current:
			cursor += 1
			continue
		if current.startswith(("#", "*", "//")):
			break

		signature_lines.append(current)
		paren_depth += current.count("(")
		paren_depth -= current.count(")")
		saw_paren = saw_paren or "(" in current

		signature = normalize_signature(signature_lines)
		ends_declaration = current.endswith(";")
		opens_definition = current.endswith("{")
		next_line = next_significant_line(lines, cursor)
		ends_before_definition = saw_paren and paren_depth == 0 and next_line == "{"

		if ends_declaration or opens_definition or ends_before_definition:
			if line_looks_like_function_signature(signature):
				return signature, cursor
			return None

		if saw_paren and paren_depth == 0 and not line_looks_like_function_signature(signature):
			return None

		cursor += 1

	return None


def doxygen_tag_has_text(comment: str, tag: str) -> bool:
	for line in comment.splitlines():
		if tag not in line:
			continue

		text = line.split(tag, 1)[1].strip()
		if text:
			return True

	return False


def doxygen_has_documented_param(comment: str) -> bool:
	pattern = re.compile(r"@param\[(?:in|out|in,out)\]\s+\S+\s+\S+")
	return any(pattern.search(line) for line in comment.splitlines())


def check_header_function_comments(path: Path, lines: list[str], errors: list[str]) -> None:
	index = 0
	documented_signatures: set[str] = set()
	while index < len(lines):
		signature = function_signature_at(lines, index)
		if signature is None:
			index += 1
			continue

		signature_text = signature[0]
		signature_key = canonical_function_signature(signature_text)
		is_definition = function_signature_is_definition(lines, signature[1])
		comment = previous_doxygen(lines, index)
		if comment is None:
			if is_definition and signature_key in documented_signatures:
				index = signature[1] + 1
				continue

			add_error(errors, path, index + 1, "function declaration requires a Doxygen comment.")
			index = signature[1] + 1
			continue

		if is_definition and signature_key in documented_signatures:
			add_error(
				errors,
				path,
				index + 1,
				"function definition must not duplicate Doxygen comment from its declaration.",
			)
			index = signature[1] + 1
			continue

		for tag in FUNCTION_TAGS:
			if not doxygen_tag_has_text(comment, tag):
				add_error(errors, path, index + 1, f"function Doxygen comment requires {tag}.")

		if function_has_parameters(signature_text) and not doxygen_has_documented_param(comment):
			add_error(
				errors,
				path,
				index + 1,
				"function Doxygen comment requires @param[in], @param[out], or @param[in,out].",
			)

		documented_signatures.add(signature_key)
		index = signature[1] + 1


def header_has_detail_namespace(lines: list[str]) -> bool:
	return any(line.strip() == "namespace detail" for line in lines)


def detail_namespace_end_index(lines: list[str]) -> int | None:
	for index, line in enumerate(lines):
		if line.strip() == "} // namespace detail":
			return index

	return None


def header_has_public_api_definitions(lines: list[str]) -> bool:
	detail_end = detail_namespace_end_index(lines)
	index = 0 if detail_end is None else detail_end + 1
	while index < len(lines):
		signature = function_signature_at(lines, index)
		if signature is None:
			index += 1
			continue

		if function_signature_is_definition(lines, signature[1]):
			return True

		index = signature[1] + 1

	return False


def required_header_sections(lines: list[str]) -> tuple[str, ...]:
	sections = ["Public API declarations"]

	if header_has_detail_namespace(lines):
		sections.append("Internal implementation details")

	if header_has_public_api_definitions(lines):
		sections.append("Public API definitions")

	return tuple(sections)


def section_title_indices(lines: list[str], title: str) -> list[int]:
	title_line = f"// {title}"
	return [index for index, line in enumerate(lines) if line.strip() == title_line]


def section_banner_uses_exact_format(lines: list[str], title_index: int, title: str) -> bool:
	if title_index == 0 or title_index + 1 >= len(lines):
		return False

	return (
		lines[title_index - 1] == SECTION_BANNER_LINE
		and lines[title_index] == f"\t// {title}"
		and lines[title_index + 1] == SECTION_BANNER_LINE
	)


def check_header_section_banners(path: Path, lines: list[str], errors: list[str]) -> None:
	if not is_module_public_header(path):
		return
	if not has_namespace_ket(lines):
		return

	required_sections = required_header_sections(lines)
	found_indices: dict[str, int] = {}

	for title in HEADER_SECTIONS:
		indices = section_title_indices(lines, title)
		if not indices:
			continue

		found_indices[title] = indices[0]
		if len(indices) > 1:
			add_error(errors, path, indices[1] + 1, f"section banner appears more than once: {title}.")

		for index in indices:
			if not section_banner_uses_exact_format(lines, index, title):
				add_error(errors, path, index + 1, f"section banner must use exact format: {title}.")

	for title in required_sections:
		if title not in found_indices:
			add_error(errors, path, 1, f"module header requires section banner: {title}.")

	ordered_found = [title for title in HEADER_SECTIONS if title in found_indices]
	found_in_file_order = sorted(ordered_found, key=lambda title: found_indices[title])
	if ordered_found != found_in_file_order:
		first_found_index = min(found_indices.values())
		add_error(errors, path, first_found_index + 1, "section banners must follow standard order.")


def type_declaration_at(lines: list[str], index: int) -> tuple[str, int] | None:
	stripped = lines[index].strip()
	if not stripped:
		return None
	if stripped.startswith(("#", "*", "//")):
		return None

	signature_lines: list[str] = []
	cursor = index

	while cursor < len(lines):
		current = lines[cursor].strip()
		if not current:
			cursor += 1
			continue
		if current.startswith(("#", "*", "//")):
			break

		signature_lines.append(current)
		signature = normalize_signature(signature_lines)
		if TYPE_DECLARATION_PATTERN.match(signature):
			return signature, cursor

		if cursor == index and not current.startswith("template "):
			return None

		cursor += 1

	return None


def check_header_type_comments(path: Path, lines: list[str], errors: list[str]) -> None:
	index = 0
	while index < len(lines):
		declaration = type_declaration_at(lines, index)
		if declaration is None:
			index += 1
			continue

		comment = previous_doxygen(lines, index)
		if comment is None:
			add_error(errors, path, index + 1, "type declaration requires a Doxygen comment.")
			index = declaration[1] + 1
			continue

		for tag in TYPE_TAGS:
			if not doxygen_tag_has_text(comment, tag):
				add_error(errors, path, index + 1, f"type Doxygen comment requires {tag}.")

		index = declaration[1] + 1


def check_comment_wording(path: Path, lines: list[str], errors: list[str]) -> None:
	in_block_comment = False

	for index, line in enumerate(lines):
		stripped = line.strip()
		is_comment = False

		if in_block_comment:
			is_comment = True
			if "*/" in stripped:
				in_block_comment = False
		elif stripped.startswith("//"):
			is_comment = True
		elif stripped.startswith("/*") or stripped.startswith("*"):
			is_comment = True
			if "*/" not in stripped:
				in_block_comment = True

		if not is_comment:
			continue

		for word in POLITE_WORDS:
			if word in stripped:
				add_error(errors, path, index + 1, f"C++ comment must not use '{word}'.")


def check_namespace_blank_lines(path: Path, lines: list[str], errors: list[str]) -> None:
	for index, line in enumerate(lines):
		if "// namespace" not in line:
			continue
		if index == 0 or lines[index - 1].strip() != "":
			add_error(errors, path, index + 1, "namespace end comment requires one blank line before it.")


def condition_text(line: str, macro_name: str) -> str | None:
	start = line.find(macro_name)
	if start < 0:
		return None

	open_paren = line.find("(", start + len(macro_name))
	close_paren = line.rfind(")")
	if open_paren < 0 or close_paren <= open_paren:
		return None

	return line[open_paren + 1 : close_paren]


def check_condition_calls(path: Path, lines: list[str], errors: list[str]) -> None:
	condition_macros = ("if", "while", "EXPECT_FALSE", "EXPECT_TRUE", "ASSERT_FALSE", "ASSERT_TRUE")
	call_pattern = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")

	for index, line in enumerate(lines):
		stripped = line.strip()
		macro_name = next(
			(
				name
				for name in condition_macros
				if stripped.startswith(f"{name} ") or stripped.startswith(f"{name}(")
			),
			None,
		)
		if macro_name is None:
			continue

		text = condition_text(stripped, macro_name)
		if text is None:
			continue

		for match in call_pattern.finditer(text):
			name = match.group(1)
			if name in ALLOWED_CONDITION_CALLS:
				continue
			add_error(errors, path, index + 1, "condition expression must not call an API directly.")
			break


def check_file(path: Path) -> list[str]:
	lines = read_lines(path)
	errors: list[str] = []

	check_comment_wording(path, lines, errors)
	check_namespace_blank_lines(path, lines, errors)
	check_condition_calls(path, lines, errors)

	if path.suffix in {".h", ".hh", ".hpp", ".hxx"}:
		check_header_section_banners(path, lines, errors)
		check_header_function_comments(path, lines, errors)
		check_header_type_comments(path, lines, errors)
	if path.name.endswith("_test.cpp") or path.parts[-2] == "tests":
		check_test_comments(path, lines, errors)

	return errors


def main() -> int:
	errors: list[str] = []
	for path in ket_tooling.iter_cpp_source_files():
		errors.extend(check_file(path))

	if errors:
		for error in errors:
			print(error, file=sys.stderr)
		return 1

	print("Convention check passed.")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
