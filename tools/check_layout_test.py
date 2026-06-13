#!/usr/bin/env python3

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import check_layout


class CheckLayoutTest(unittest.TestCase):
	def make_repo(self) -> tempfile.TemporaryDirectory[str]:
		repository = tempfile.TemporaryDirectory()
		root = Path(repository.name)
		(root / "modules" / "bcd").mkdir(parents=True)
		(root / "modules" / "bcd" / "ket_bcd.h").write_text(
			"\n".join(
				(
					"#pragma once",
					"",
					"// Drop-in module:",
					"// Dependencies:",
					"// Namespace:",
					"//   ket",
					"",
				)
			),
			encoding="utf-8",
		)
		(root / "modules" / "bcd" / "ket_bcd.cpp").write_text(
			'#include "ket_bcd.h"\n',
			encoding="utf-8",
		)
		(root / "modules" / "bcd" / "ket_bcd_test.cpp").write_text(
			'#include "ket_bcd.h"\n',
			encoding="utf-8",
		)
		(root / "CMakeLists.txt").write_text(
			"\n".join(
				(
					"ket_add_module_test(",
					"    ket_bcd_test",
					"    SOURCES",
					"        modules/bcd/ket_bcd.cpp",
					"        modules/bcd/ket_bcd_test.cpp",
					")",
					"",
				)
			),
			encoding="utf-8",
		)
		(root / "progress.md").write_text(
			"| Module | Status |\n| ------ | ------ |\n| bcd | verified |\n",
			encoding="utf-8",
		)
		return repository

	def test_valid_module_layout_has_no_errors(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)

			errors = check_layout.collect_layout_errors(root)

			self.assertEqual(errors, [])

	def test_missing_standard_file_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "bcd" / "ket_bcd_test.cpp").unlink()

			errors = check_layout.collect_layout_errors(root)

			self.assertIn("modules/bcd/ket_bcd_test.cpp: required module file is missing.", errors)

	def test_cross_module_include_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "bcd" / "ket_bcd.cpp").write_text(
				'#include "ket_bits.h"\n',
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.cpp:1: module must not include another ket module.",
				errors,
			)

	def test_invalid_module_name_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "Bcd").mkdir()

			errors = check_layout.collect_layout_errors(root)

			self.assertIn("modules/Bcd: module directory name must be lower_snake_case.", errors)

	def test_missing_cmake_registration_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "CMakeLists.txt").write_text("", encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn("modules/bcd/ket_bcd.cpp: module file is not registered in CMakeLists.txt.", errors)
			self.assertIn(
				"modules/bcd/ket_bcd_test.cpp: module file is not registered in CMakeLists.txt.",
				errors,
			)

	def test_missing_progress_registration_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "progress.md").write_text("| Module | Status |\n", encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn("progress.md: module 'bcd' is not listed.", errors)

	def test_missing_header_preamble_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(
				"#pragma once\n",
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertIn("modules/bcd/ket_bcd.h: header preamble must describe drop-in conditions.", errors)
			self.assertIn("modules/bcd/ket_bcd.h: header preamble must describe dependencies.", errors)
			self.assertIn("modules/bcd/ket_bcd.h: header preamble must describe namespace ket.", errors)


if __name__ == "__main__":
	unittest.main()
