#!/usr/bin/env python3

from __future__ import annotations

import json
import os
import shutil
import shlex
import subprocess
import sys
from pathlib import Path

import ket_tooling


def output_has_iwyu_suggestion(output: str) -> bool:
	suggestion_markers = (
		" should add these lines:",
		" should remove these lines:",
		"The full include-list for ",
	)
	return any(marker in output for marker in suggestion_markers)


def find_iwyu_tool() -> str | None:
	configured = os.environ.get("IWYU_TOOL")
	if configured:
		return configured

	for candidate in ("iwyu_tool.py", "iwyu_tool"):
		path = shutil.which(candidate)
		if path:
			return path

	return None


def find_iwyu() -> str:
	return ket_tooling.find_tool(
		"IWYU",
		("iwyu", "include-what-you-use"),
		"Set IWYU or install iwyu.",
	)


def load_compile_commands() -> list[dict[str, object]]:
	with ket_tooling.require_compile_commands().open(encoding="utf-8") as file:
		data = json.load(file)

	if not isinstance(data, list):
		raise RuntimeError("compile_commands.json must contain a JSON array.")

	return data


def selected_files() -> list[Path]:
	return [path.resolve() for path in ket_tooling.iter_analysis_files()]


def run_iwyu_tool(iwyu_tool: str, files: list[Path]) -> int:
	command = [
		iwyu_tool,
		"-p",
		str(ket_tooling.BUILD_DIR),
		*[str(path) for path in files],
		"--",
		"-Xiwyu",
		"--no_comments",
		"-Wno-unknown-warning-option",
		"-Wno-error=unknown-warning-option",
	]
	result = subprocess.run(
		command,
		cwd=ket_tooling.ROOT,
		text=True,
		stdout=subprocess.PIPE,
		stderr=subprocess.STDOUT,
	)
	has_suggestion = output_has_iwyu_suggestion(result.stdout)
	if result.returncode != 0 or has_suggestion:
		print(result.stdout, end="")
	if result.returncode != 0 or has_suggestion:
		return 1

	return 0


def compile_arguments(entry: dict[str, object]) -> list[str]:
	arguments = entry.get("arguments")
	if isinstance(arguments, list):
		return [str(argument) for argument in arguments]

	command = entry.get("command")
	if isinstance(command, str):
		return shlex.split(command)

	raise RuntimeError("compile command entry must contain command or arguments.")


def strip_compile_only_outputs(arguments: list[str]) -> list[str]:
	stripped: list[str] = []
	skip_next = False
	flags_with_values = {"-MF", "-MT", "-MQ", "-o"}

	for argument in arguments:
		if skip_next:
			skip_next = False
			continue

		if argument in flags_with_values:
			skip_next = True
			continue

		if argument.startswith("-MF") or argument.startswith("-MT") or argument.startswith("-MQ"):
			continue
		if argument.startswith("-o"):
			continue
		if argument in {"-c", "-MD", "-MMD", "-MP"}:
			continue

		stripped.append(argument)

	return stripped


def run_iwyu_binary(iwyu: str, files: list[Path]) -> int:
	commands = load_compile_commands()
	selected = set(files)
	seen: set[Path] = set()
	failed = False

	for entry in commands:
		file_value = entry.get("file")
		if not isinstance(file_value, str):
			continue

		source = Path(file_value).resolve()
		if source not in selected:
			continue

		arguments = strip_compile_only_outputs(compile_arguments(entry))
		if len(arguments) < 2:
			raise RuntimeError(f"compile command for {source} is incomplete.")

		command = [
			iwyu,
			*arguments[1:],
			"-Xiwyu",
			"--no_comments",
			"-Wno-unknown-warning-option",
			"-Wno-error=unknown-warning-option",
		]
		directory = Path(str(entry.get("directory", ket_tooling.ROOT)))
		result = subprocess.run(
			command,
			cwd=directory,
			text=True,
			stdout=subprocess.PIPE,
			stderr=subprocess.STDOUT,
		)
		seen.add(source)
		has_suggestion = output_has_iwyu_suggestion(result.stdout)
		if result.returncode != 0 or has_suggestion:
			print(result.stdout, end="")
		if result.returncode != 0 or has_suggestion:
			failed = True

	missing = selected - seen
	if missing:
		for path in sorted(missing):
			print(f"compile command not found for {ket_tooling.relative(path)}", file=sys.stderr)
		failed = True

	return 1 if failed else 0


def main() -> int:
	try:
		ket_tooling.require_compile_commands()
	except RuntimeError as error:
		return ket_tooling.fail(error)

	files = selected_files()
	if not files:
		print("No C++ analysis files found.")
		return 0

	iwyu_tool = find_iwyu_tool()
	if iwyu_tool:
		result = run_iwyu_tool(iwyu_tool, files)
	else:
		try:
			iwyu = find_iwyu()
		except RuntimeError as error:
			return ket_tooling.fail(error)

		result = run_iwyu_binary(iwyu, files)

	if result != 0:
		return result

	print(f"IWYU passed for {len(files)} file(s).")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
