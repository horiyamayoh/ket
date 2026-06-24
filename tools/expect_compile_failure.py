#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shlex
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description="Expect a C++ source file to fail compilation.")
	parser.add_argument("--compiler", required=True)
	parser.add_argument("--standard", required=True)
	parser.add_argument("--source", required=True)
	parser.add_argument("--output", required=True)
	parser.add_argument("--include", action="append", default=[])
	parser.add_argument(
		"--expect-diagnostic",
		action="append",
		default=[],
		required=True,
		help="Substring that must appear in compiler stdout or stderr. Repeat to require more.",
	)
	return parser.parse_args()


def build_command(args: argparse.Namespace) -> list[str]:
	command = [
		args.compiler,
		f"-std={args.standard}",
		"-x",
		"c++",
		"-c",
		str(Path(args.source)),
		"-o",
		str(Path(args.output)),
	]
	for include_dir in args.include:
		command.extend(("-I", include_dir))
	return command


def print_failure_details(
	message: str,
	command: list[str],
	completed: subprocess.CompletedProcess[str],
	missing_diagnostics: list[str],
) -> None:
	print(message, file=sys.stderr)
	if missing_diagnostics:
		print("missing expected diagnostic substring(s):", file=sys.stderr)
		for diagnostic in missing_diagnostics:
			print(f"  {diagnostic}", file=sys.stderr)
	print("command:", " ".join(shlex.quote(part) for part in command), file=sys.stderr)
	print("stdout:", file=sys.stderr)
	print(completed.stdout, file=sys.stderr, end="" if completed.stdout.endswith("\n") else "\n")
	print("stderr:", file=sys.stderr)
	print(completed.stderr, file=sys.stderr, end="" if completed.stderr.endswith("\n") else "\n")


def run(args: argparse.Namespace) -> int:
	source = Path(args.source)
	output = Path(args.output)
	output.parent.mkdir(parents=True, exist_ok=True)

	command = build_command(args)
	completed = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
	if completed.returncode == 0:
		print_failure_details(
			f"expected compilation failure, but succeeded: {source}",
			command,
			completed,
			[],
		)
		return 1

	compiler_output = f"{completed.stdout}\n{completed.stderr}"
	missing_diagnostics = [
		diagnostic for diagnostic in args.expect_diagnostic if diagnostic not in compiler_output
	]
	if missing_diagnostics:
		print_failure_details(
			f"compilation failed for an unexpected reason: {source}",
			command,
			completed,
			missing_diagnostics,
		)
		return 1

	return 0


def main() -> int:
	return run(parse_args())


if __name__ == "__main__":
	raise SystemExit(main())
