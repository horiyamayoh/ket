#!/usr/bin/env python3

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build" / "dev"
COMPILE_COMMANDS = BUILD_DIR / "compile_commands.json"

CPP_SOURCE_SUFFIXES = {
	".c",
	".cc",
	".cpp",
	".cxx",
	".h",
	".hh",
	".hpp",
	".hxx",
}

PRETTIER_SUFFIXES = {
	".json",
	".md",
	".yaml",
	".yml",
}

SKIP_DIRS = {
	".cache",
	".git",
	".idea",
	".vscode",
	"build",
	"CMakeFiles",
	"node_modules",
	"_deps",
}


def relative(path: Path) -> str:
	return path.relative_to(ROOT).as_posix()


def find_tool(env_name: str, candidates: tuple[str, ...], install_hint: str) -> str:
	configured = os.environ.get(env_name)
	if configured:
		return configured

	for candidate in candidates:
		path = shutil.which(candidate)
		if path:
			return path

	raise RuntimeError(f"{candidates[0]} was not found. {install_hint}")


def iter_files_with_suffixes(suffixes: set[str]) -> list[Path]:
	files: list[Path] = []

	for path in ROOT.rglob("*"):
		if any(part in SKIP_DIRS for part in path.parts):
			continue
		if path.is_file() and path.suffix in suffixes:
			files.append(path)

	return sorted(files)


def iter_cpp_source_files() -> list[Path]:
	return iter_files_with_suffixes(CPP_SOURCE_SUFFIXES)


def iter_prettier_files() -> list[Path]:
	return iter_files_with_suffixes(PRETTIER_SUFFIXES)


def iter_analysis_files() -> list[Path]:
	files: list[Path] = []

	for directory_name in ("modules", "packages", "tests"):
		directory = ROOT / directory_name
		if not directory.exists():
			continue
		files.extend(path for path in directory.rglob("*.cpp") if path.is_file())

	return sorted(files)


def iter_module_implementation_files() -> list[Path]:
	directory = ROOT / "modules"
	if not directory.exists():
		return []

	return sorted(
		path for path in directory.rglob("*.cpp") if path.is_file() and not path.name.endswith("_test.cpp")
	)


def require_compile_commands() -> Path:
	if not COMPILE_COMMANDS.exists():
		raise RuntimeError("compile_commands.json was not found. Run cmake --preset dev first.")

	return COMPILE_COMMANDS


def run(command: list[str]) -> int:
	result = subprocess.run(command, cwd=ROOT)
	return result.returncode


def run_checked(command: list[str]) -> None:
	result = subprocess.run(command, cwd=ROOT)
	if result.returncode != 0:
		raise subprocess.CalledProcessError(result.returncode, command)


def fail(error: Exception) -> int:
	print(error, file=sys.stderr)
	return 1
