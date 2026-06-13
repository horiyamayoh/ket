#!/usr/bin/env python3

from __future__ import annotations

import subprocess
import sys
import time
from dataclasses import dataclass

import ket_tooling


@dataclass(frozen=True)
class CheckStep:
	name: str
	command: list[str]


def check_steps() -> list[CheckStep]:
	python = sys.executable
	return [
		CheckStep("Python tooling", [python, "tools/check_python.py"]),
		CheckStep("Format", [python, "tools/check_format.py"]),
		CheckStep("Layout", [python, "tools/check_layout.py"]),
		CheckStep("Configure dev", ["cmake", "--preset", "dev"]),
		CheckStep("Build dev", ["cmake", "--build", "--preset", "dev"]),
		CheckStep("Static analysis", ["cmake", "--build", "--preset", "dev", "--target", "check-static"]),
		CheckStep(
			"Conventions",
			["cmake", "--build", "--preset", "dev", "--target", "check-conventions"],
		),
		CheckStep("CTest dev", ["ctest", "--preset", "dev"]),
		CheckStep("Configure sanitizer", ["cmake", "--preset", "sanitize"]),
		CheckStep("Build sanitizer", ["cmake", "--build", "--preset", "sanitize"]),
		CheckStep("CTest sanitizer", ["ctest", "--preset", "sanitize"]),
		CheckStep("Diff whitespace", ["git", "diff", "--check"]),
	]


def command_text(command: list[str]) -> str:
	return " ".join(command)


def run_step(step: CheckStep) -> bool:
	started_at = time.monotonic()
	print(f"==> {step.name}: {command_text(step.command)}", flush=True)
	result = subprocess.run(step.command, cwd=ket_tooling.ROOT)
	elapsed = time.monotonic() - started_at

	if result.returncode != 0:
		print(
			f"FAILED {step.name} after {elapsed:.1f}s: {command_text(step.command)}",
			file=sys.stderr,
		)
		return False

	print(f"<== {step.name} passed in {elapsed:.1f}s", flush=True)
	return True


def main() -> int:
	started_at = time.monotonic()

	for step in check_steps():
		if not run_step(step):
			return 1

	elapsed = time.monotonic() - started_at
	print(f"Repository check passed in {elapsed:.1f}s.")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
