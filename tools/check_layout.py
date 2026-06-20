#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path

import ket_tooling


MODULE_NAME_PATTERN = re.compile(r"^[a-z][a-z0-9_]*$")
INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
DETAIL_NAMESPACE_PATTERN = re.compile(r"^\s*namespace\s+detail\s*(?:\{|$)")
ANONYMOUS_NAMESPACE_PATTERN = re.compile(r"^\s*namespace\s*(?:\{|$)")
HEADER_PREAMBLE_LINE_LIMIT = 80
HEADER_PREAMBLE_MARKERS = (
	("@file", "header preamble must use Doxygen @file."),
	("@brief", "header preamble must describe module summary with @brief."),
	("@details", "header preamble must describe module details with @details."),
	("@par プロジェクトへの適用方法", "header preamble must describe project application method."),
	("@par C++バージョン要件", "header preamble must describe C++ version requirements."),
	("最小要件：", "header preamble must describe minimum C++ standard."),
	(
		"本ライブラリの適用を推奨する C++ バージョン：",
		"header preamble must describe recommended C++ standard.",
	),
	("推奨理由：", "header preamble must describe recommended C++ standard reason."),
	(
		"本ライブラリの適用を推奨しない C++ バージョン：",
		"header preamble must describe non-recommended C++ standards.",
	),
	("非推奨理由：", "header preamble must describe non-recommended C++ standards reason."),
	("@par 他のライブラリへの依存", "header preamble must describe dependencies."),
	("@par namespace", "header preamble must describe namespace."),
)


@dataclass(frozen=True)
class ModuleLayout:
	name: str
	directory: Path
	header: Path
	source: Path
	test: Path


def relative(root: Path, path: Path) -> str:
	return path.relative_to(root).as_posix()


def module_layout(directory: Path) -> ModuleLayout:
	name = directory.name
	return ModuleLayout(
		name=name,
		directory=directory,
		header=directory / f"ket_{name}.h",
		source=directory / f"ket_{name}.cpp",
		test=directory / f"ket_{name}_test.cpp",
	)


def module_directories(root: Path) -> list[Path]:
	modules = root / "modules"
	if not modules.exists():
		return []

	return sorted(path for path in modules.iterdir() if path.is_dir())


def read_text_if_present(path: Path) -> str:
	if not path.exists():
		return ""

	return path.read_text(encoding="utf-8")


def add_error(errors: list[str], root: Path, path: Path, message: str) -> None:
	errors.append(f"{relative(root, path)}: {message}")


def check_standard_files(root: Path, layout: ModuleLayout, errors: list[str]) -> None:
	if not MODULE_NAME_PATTERN.match(layout.name):
		add_error(errors, root, layout.directory, "module directory name must be lower_snake_case.")

	for path in (layout.header, layout.test):
		if not path.exists():
			add_error(errors, root, path, "required module file is missing.")


def include_target_basename(include_target: str) -> str:
	return include_target.replace("\\", "/").rsplit("/", 1)[-1]


def remove_block_comment_content(line: str, in_block_comment: bool) -> tuple[str, bool]:
	content_parts: list[str] = []
	cursor = 0

	while cursor < len(line):
		if in_block_comment:
			comment_end = line.find("*/", cursor)
			if comment_end < 0:
				return "".join(content_parts), True

			cursor = comment_end + 2
			in_block_comment = False
			continue

		comment_start = line.find("/*", cursor)
		if comment_start < 0:
			content_parts.append(line[cursor:])
			break

		content_parts.append(line[cursor:comment_start])
		cursor = comment_start + 2
		in_block_comment = True

	return "".join(content_parts), in_block_comment


def source_has_real_implementation(path: Path) -> bool:
	in_block_comment = False

	for line in path.read_text(encoding="utf-8").splitlines():
		content, in_block_comment = remove_block_comment_content(line, in_block_comment)
		stripped = content.strip()
		if not stripped:
			continue
		if stripped.startswith(("//", "*")):
			continue

		match = INCLUDE_PATTERN.match(stripped)
		if match is not None:
			continue

		return True

	return False


def uncommented_code_lines(path: Path) -> list[str]:
	if not path.exists():
		return []

	code_lines: list[str] = []
	in_block_comment = False

	for line in path.read_text(encoding="utf-8").splitlines():
		content, in_block_comment = remove_block_comment_content(line, in_block_comment)
		content = content.split("//", 1)[0]
		stripped = content.strip()
		if stripped:
			code_lines.append(stripped)

	return code_lines


def header_has_detail_namespace(path: Path) -> bool:
	return any(DETAIL_NAMESPACE_PATTERN.match(line) is not None for line in uncommented_code_lines(path))


def source_has_anonymous_namespace(path: Path) -> bool:
	return any(ANONYMOUS_NAMESPACE_PATTERN.match(line) is not None for line in uncommented_code_lines(path))


def check_no_placeholder_source(root: Path, layout: ModuleLayout, errors: list[str]) -> None:
	if not layout.source.exists():
		return

	source_has_implementation = source_has_real_implementation(layout.source)
	if not source_has_implementation:
		add_error(errors, root, layout.source, "header-only module must omit placeholder source file.")


def check_no_cross_module_includes(root: Path, layout: ModuleLayout, errors: list[str]) -> None:
	own_header = layout.header.name

	for path in (layout.header, layout.source, layout.test):
		if not path.exists():
			continue

		for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
			match = INCLUDE_PATTERN.match(line)
			if match is None:
				continue

			included_name = include_target_basename(match.group(1))
			includes_ket_header = included_name.startswith("ket_") and included_name.endswith(".h")
			if includes_ket_header and included_name != own_header:
				errors.append(
					f"{relative(root, path)}:{line_number}: module must not include another ket module."
				)


def check_cmake_registration(root: Path, layout: ModuleLayout, errors: list[str]) -> None:
	cmake_text = read_text_if_present(root / "CMakeLists.txt")
	for path in (layout.source, layout.test):
		path_text = relative(root, path)
		if path.exists() and path_text not in cmake_text:
			add_error(errors, root, path, "module file is not registered in CMakeLists.txt.")


def check_progress_registration(root: Path, layout: ModuleLayout, errors: list[str]) -> None:
	progress_text = read_text_if_present(root / "progress.md")
	progress_pattern = re.compile(rf"^\|\s*{re.escape(layout.name)}\s*\|", re.MULTILINE)
	if not progress_pattern.search(progress_text):
		add_error(errors, root, root / "progress.md", f"module '{layout.name}' is not listed.")


def first_doxygen_block(lines: list[str]) -> str:
	for index, line in enumerate(lines[:HEADER_PREAMBLE_LINE_LIMIT]):
		if line.strip() != "/**":
			continue

		block_lines: list[str] = []
		for block_line in lines[index:HEADER_PREAMBLE_LINE_LIMIT]:
			block_lines.append(block_line)
			if block_line.strip() == "*/":
				break

		return "\n".join(block_lines)

	return ""


def check_header_preamble(root: Path, layout: ModuleLayout, errors: list[str]) -> None:
	if not layout.header.exists():
		return

	lines = layout.header.read_text(encoding="utf-8").splitlines()
	preamble = first_doxygen_block(lines)
	first_non_empty = next((line.strip() for line in lines if line.strip()), "")

	if first_non_empty != "#pragma once":
		add_error(errors, root, layout.header, "header must start with #pragma once.")

	for marker, message in HEADER_PREAMBLE_MARKERS:
		if marker not in preamble:
			add_error(errors, root, layout.header, message)

	# 公開APIはmodule毎の入れ子namespace (ket::<module>) を要求
	public_namespace = f"公開API：ket::{layout.name}"
	if public_namespace not in preamble:
		add_error(errors, root, layout.header, "header preamble must describe namespace ket public API.")

	detail_namespace = f"内部実装：ket::{layout.name}::detail"
	cpp_anonymous_namespace = "内部実装：.cpp の無名 namespace"
	no_internal_implementation = "内部実装：なし"
	internal_descriptions = (
		detail_namespace in preamble,
		cpp_anonymous_namespace in preamble,
		no_internal_implementation in preamble,
	)
	if not any(internal_descriptions):
		add_error(errors, root, layout.header, "header preamble must describe internal implementation namespace.")
		return

	if sum(1 for description_is_present in internal_descriptions if description_is_present) > 1:
		add_error(errors, root, layout.header, "header preamble must describe exactly one internal implementation location.")
		return

	has_header_detail = header_has_detail_namespace(layout.header)
	has_source_anonymous_namespace = source_has_anonymous_namespace(layout.source)
	if detail_namespace in preamble and not has_header_detail:
		add_error(errors, root, layout.header, "header preamble says ket detail implementation, but header has no namespace detail.")
	if cpp_anonymous_namespace in preamble and not has_source_anonymous_namespace:
		add_error(errors, root, layout.header, "header preamble says .cpp anonymous namespace, but source has no anonymous namespace.")
	if no_internal_implementation in preamble:
		if has_header_detail:
			add_error(errors, root, layout.header, "header preamble says no internal implementation, but header has namespace detail.")
		if has_source_anonymous_namespace:
			add_error(errors, root, layout.header, "header preamble says no internal implementation, but source has anonymous namespace.")


def collect_layout_errors(root: Path = ket_tooling.ROOT) -> list[str]:
	errors: list[str] = []

	for directory in module_directories(root):
		layout = module_layout(directory)
		check_standard_files(root, layout, errors)
		check_no_placeholder_source(root, layout, errors)
		check_no_cross_module_includes(root, layout, errors)
		check_cmake_registration(root, layout, errors)
		check_progress_registration(root, layout, errors)
		check_header_preamble(root, layout, errors)

	return errors


def main() -> int:
	errors = collect_layout_errors()
	if errors:
		for error in errors:
			print(error, file=sys.stderr)
		return 1

	print("Layout check passed.")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
