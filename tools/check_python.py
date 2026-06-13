#!/usr/bin/env python3

from __future__ import annotations

import sys
import unittest
from pathlib import Path

import ket_tooling


TOOLS_DIR = ket_tooling.ROOT / "tools"


def iter_python_files() -> list[Path]:
	return sorted(path for path in TOOLS_DIR.glob("*.py") if path.is_file())


def check_syntax(files: list[Path]) -> bool:
	succeeded = True

	for path in files:
		try:
			source = path.read_text(encoding="utf-8")
			compile(source, str(path), "exec")
		except SyntaxError as error:
			print(f"{path}: {error}", file=sys.stderr)
			succeeded = False

	return succeeded


def run_unittests() -> bool:
	loader = unittest.TestLoader()
	suite = loader.discover(str(TOOLS_DIR), pattern="*_test.py")
	result = unittest.TextTestRunner(stream=sys.stdout, verbosity=2).run(suite)
	return result.wasSuccessful()


def main() -> int:
	python_files = iter_python_files()
	syntax_ok = check_syntax(python_files)
	tests_ok = run_unittests()

	if not syntax_ok or not tests_ok:
		return 1

	print(f"Python tooling check passed for {len(python_files)} file(s).")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
