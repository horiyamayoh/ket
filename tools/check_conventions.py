#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from pathlib import Path

import ket_tooling


POLITE_WORDS = ("です", "ます", "ください")
TEST_TAGS = ("@test", "@brief", "@details", "@pre", "@post")
FUNCTION_TAGS = ("@brief", "@retval", "@pre", "@post")
ALLOWED_CONDITION_CALLS = {"alignof", "sizeof"}


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


def line_looks_like_function_signature(line: str) -> bool:
	stripped = line.strip()
	if "(" not in stripped or ")" not in stripped:
		return False
	if stripped.startswith(("if ", "while ", "for ", "switch ", "return ", "static_assert")):
		return False
	if stripped.startswith(("TEST", "EXPECT_", "ASSERT_")):
		return False
	if stripped.startswith(("#", "*", "//")):
		return False

	return bool(
		re.match(
			r"^(?:template\s*<[^>]+>\s*)?(?:[\w:<>~*&,\s]+\s+)+[A-Za-z_][A-Za-z0-9_:]*\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:;)?\s*$",
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


def function_has_parameters(line: str) -> bool:
	match = re.search(r"\((.*)\)", line)
	if not match:
		return False

	parameters = match.group(1).strip()
	return parameters not in ("", "void")


def check_header_function_comments(path: Path, lines: list[str], errors: list[str]) -> None:
	for index, line in enumerate(lines):
		if not line_looks_like_function_signature(line):
			continue

		stripped = line.strip()
		next_line = next_significant_line(lines, index)
		is_definition = next_line == "{"
		is_declaration = stripped.endswith(";")
		if not is_definition and not is_declaration:
			continue

		comment = previous_doxygen(lines, index)
		if comment is None:
			add_error(errors, path, index + 1, "function declaration requires a Doxygen comment.")
			continue

		for tag in FUNCTION_TAGS:
			if tag not in comment:
				add_error(errors, path, index + 1, f"function Doxygen comment requires {tag}.")

		if function_has_parameters(stripped) and "@param[in]" not in comment and "@param[out]" not in comment:
			add_error(errors, path, index + 1, "function Doxygen comment requires @param[in] or @param[out].")


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
		check_header_function_comments(path, lines, errors)
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
