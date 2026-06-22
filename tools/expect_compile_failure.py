#!/usr/bin/env python3

from __future__ import annotations

import argparse
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
	return parser.parse_args()


def main() -> int:
	args = parse_args()
	source = Path(args.source)
	output = Path(args.output)
	output.parent.mkdir(parents=True, exist_ok=True)

	command = [
		args.compiler,
		f"-std={args.standard}",
		"-x",
		"c++",
		"-c",
		str(source),
		"-o",
		str(output),
	]
	for include_dir in args.include:
		command.extend(("-I", include_dir))

	completed = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
	if completed.returncode == 0:
		print(f"expected compilation failure, but succeeded: {source}", file=sys.stderr)
		return 1

	return 0


if __name__ == "__main__":
	raise SystemExit(main())
