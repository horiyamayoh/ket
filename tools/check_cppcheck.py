#!/usr/bin/env python3

from __future__ import annotations

import subprocess

import ket_tooling


def find_cppcheck() -> str:
	return ket_tooling.find_tool(
		"CPPCHECK",
		("cppcheck",),
		"Set CPPCHECK or install cppcheck.",
	)


def main() -> int:
	try:
		cppcheck = find_cppcheck()
		compile_commands = ket_tooling.require_compile_commands()
	except RuntimeError as error:
		return ket_tooling.fail(error)

	files = ket_tooling.iter_analysis_files()
	if not files:
		print("No C++ analysis files found.")
		return 0

	command = [
		cppcheck,
		f"--project={compile_commands}",
		"--enable=warning,style,performance,portability,information,missingInclude",
		"--inconclusive",
		"--error-exitcode=1",
		"--inline-suppr",
		"--template=gcc",
		"--suppress=missingIncludeSystem",
		"--suppress=checkersReport",
		"--file-filter=*/modules/*",
		"--file-filter=*/packages/*",
		"--file-filter=*/tests/*",
	]
	result = subprocess.run(command, cwd=ket_tooling.ROOT)
	if result.returncode != 0:
		return result.returncode

	print(f"cppcheck passed for {len(files)} file(s).")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
