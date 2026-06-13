#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path

import ket_tooling


MODULE_NAME_PATTERN = re.compile(r"^[a-z][a-z0-9_]*$")
INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')


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

	for path in (layout.header, layout.source, layout.test):
		if not path.exists():
			add_error(errors, root, path, "required module file is missing.")


def include_target_basename(include_target: str) -> str:
	return include_target.replace("\\", "/").rsplit("/", 1)[-1]


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


def check_header_preamble(root: Path, layout: ModuleLayout, errors: list[str]) -> None:
	if not layout.header.exists():
		return

	lines = layout.header.read_text(encoding="utf-8").splitlines()
	preamble = "\n".join(lines[:40])
	first_non_empty = next((line.strip() for line in lines if line.strip()), "")

	if first_non_empty != "#pragma once":
		add_error(errors, root, layout.header, "header must start with #pragma once.")

	if "Drop-in" not in preamble and "持ち出し条件" not in preamble:
		add_error(errors, root, layout.header, "header preamble must describe drop-in conditions.")

	if "Dependencies" not in preamble and "依存" not in preamble:
		add_error(errors, root, layout.header, "header preamble must describe dependencies.")

	if "Namespace" not in preamble or "ket" not in preamble:
		add_error(errors, root, layout.header, "header preamble must describe namespace ket.")


def collect_layout_errors(root: Path = ket_tooling.ROOT) -> list[str]:
	errors: list[str] = []

	for directory in module_directories(root):
		layout = module_layout(directory)
		check_standard_files(root, layout, errors)
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
