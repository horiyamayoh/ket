#!/usr/bin/env python3

from __future__ import annotations

import os
import subprocess
import sys

import ket_tooling


ROOT = ket_tooling.ROOT


def find_clang_format() -> str:
	return ket_tooling.find_tool(
		"CLANG_FORMAT",
		("clang-format-18", "clang-format"),
		"Set CLANG_FORMAT or install clang-format-18.",
	)


def find_prettier() -> str:
	configured = os.environ.get("PRETTIER")
	if configured:
		return configured

	local_prettier = ROOT / "node_modules" / ".bin" / "prettier"
	if local_prettier.exists():
		return str(local_prettier)

	raise RuntimeError("Prettier was not found. Run npm ci or set PRETTIER.")


def main() -> int:
	try:
		clang_format = find_clang_format()
	except RuntimeError as error:
		print(error, file=sys.stderr)
		return 1

	try:
		prettier = find_prettier()
	except RuntimeError as error:
		print(error, file=sys.stderr)
		return 1

	cpp_files = ket_tooling.iter_cpp_source_files()
	if not cpp_files:
		print("No C++ source files found.")
	else:
		command = [clang_format, "-i", *[str(path) for path in cpp_files]]
		subprocess.run(command, cwd=ROOT, check=True)
		print(f"Formatted {len(cpp_files)} C++ file(s).")

	prettier_files = ket_tooling.iter_prettier_files()
	if not prettier_files:
		print("No Prettier-managed files found.")
	else:
		command = [prettier, "--write", *[str(path) for path in prettier_files]]
		subprocess.run(command, cwd=ROOT, check=True)
		print(f"Formatted {len(prettier_files)} Prettier-managed file(s).")

	return 0


if __name__ == "__main__":
	raise SystemExit(main())
