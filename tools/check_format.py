#!/usr/bin/env python3

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {
	".c",
	".cc",
	".cpp",
	".cxx",
	".h",
	".hh",
	".hpp",
	".hxx",
}
SKIP_DIRS = {
	".cache",
	".git",
	".github",
	".idea",
	".vscode",
	"build",
	"CMakeFiles",
	"_deps",
}


def find_clang_format() -> str:
	configured = os.environ.get("CLANG_FORMAT")
	if configured:
		return configured

	for candidate in ("clang-format-18", "clang-format"):
		path = shutil.which(candidate)
		if path:
			return path

	raise RuntimeError("clang-format was not found. Set CLANG_FORMAT or install clang-format.")


def iter_source_files() -> list[Path]:
	files: list[Path] = []

	for path in ROOT.rglob("*"):
		if any(part in SKIP_DIRS for part in path.parts):
			continue
		if path.is_file() and path.suffix in SOURCE_SUFFIXES:
			files.append(path)

	return sorted(files)


def main() -> int:
	try:
		clang_format = find_clang_format()
	except RuntimeError as error:
		print(error, file=sys.stderr)
		return 1

	files = iter_source_files()
	if not files:
		print("No C++ source files found.")
		return 0

	command = [clang_format, "--dry-run", "--Werror", *[str(path) for path in files]]
	result = subprocess.run(command, cwd=ROOT)
	if result.returncode != 0:
		print("Formatting check failed. Run python3 tools/format.py.", file=sys.stderr)
		return result.returncode

	print(f"Format check passed for {len(files)} file(s).")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
