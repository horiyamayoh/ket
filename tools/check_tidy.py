#!/usr/bin/env python3

from __future__ import annotations

import os
import re
import subprocess
from concurrent.futures import ThreadPoolExecutor

import ket_tooling


def find_clang_tidy() -> str:
	return ket_tooling.find_tool(
		"CLANG_TIDY",
		("clang-tidy-18", "clang-tidy"),
		"Set CLANG_TIDY or install clang-tidy-18.",
	)


def run_clang_tidy_file(
	clang_tidy: str,
	path: str,
	header_filter: str,
	noise_patterns: tuple[re.Pattern[str], ...],
) -> tuple[bool, str]:
	command = [
		clang_tidy,
		"-p",
		str(ket_tooling.BUILD_DIR),
		path,
		"--quiet",
		f"--header-filter={header_filter}",
		"--extra-arg=-Wno-unknown-warning-option",
		"--extra-arg=-Wno-error=unknown-warning-option",
	]
	result = subprocess.run(
		command,
		cwd=ket_tooling.ROOT,
		text=True,
		stdout=subprocess.PIPE,
		stderr=subprocess.STDOUT,
	)
	output_lines = result.stdout.splitlines()
	visible_lines = [
		line for line in output_lines if not any(pattern.match(line) for pattern in noise_patterns)
	]
	return result.returncode == 0, "\n".join(visible_lines)


def main() -> int:
	try:
		clang_tidy = find_clang_tidy()
		ket_tooling.require_compile_commands()
	except RuntimeError as error:
		return ket_tooling.fail(error)

	files = ket_tooling.iter_analysis_files()
	if not files:
		print("No C++ analysis files found.")
		return 0

	header_filter = r".*/(modules|packages)/.*"
	noise_patterns = (
		re.compile(r"^\d+ warnings generated\.$"),
		re.compile(r"^Suppressed \d+ warnings .*$"),
		re.compile(r"^Use -header-filter=.*$"),
	)
	worker_count = min(len(files), max(1, os.cpu_count() or 1))
	failed = False

	with ThreadPoolExecutor(max_workers=worker_count) as executor:
		results = executor.map(
			lambda path: run_clang_tidy_file(
				clang_tidy,
				str(path),
				header_filter,
				noise_patterns,
			),
			files,
		)

		for succeeded, output in results:
			if output:
				print(output)
			if not succeeded:
				failed = True

	if failed:
		return 1

	print(f"clang-tidy passed for {len(files)} file(s).")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
