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
		command = [clang_format, "--dry-run", "--Werror", *[str(path) for path in cpp_files]]
		result = subprocess.run(command, cwd=ROOT)
		if result.returncode != 0:
			print("Formatting check failed. Run python3 tools/format.py.", file=sys.stderr)
			return result.returncode

	prettier_files = ket_tooling.iter_prettier_files()
	if not prettier_files:
		print("No Prettier-managed files found.")
	else:
		command = [prettier, "--check", *[str(path) for path in prettier_files]]
		result = subprocess.run(command, cwd=ROOT)
		if result.returncode != 0:
			print("Prettier check failed. Run python3 tools/format.py.", file=sys.stderr)
			return result.returncode

	print(f"Format check passed for {len(cpp_files)} C++ file(s).")
	print(f"Prettier check passed for {len(prettier_files)} file(s).")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
