#!/usr/bin/env python3

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import check_layout


class CheckLayoutTest(unittest.TestCase):
	def header_preamble(self) -> str:
		return "\n".join(
			(
				"#pragma once",
				"",
				"/**",
				" * @file ket_bcd.h",
				" * @brief BCD変換module。",
				" *",
				" * @details layout check用の試験module。",
				" *",
				" * @par プロジェクトへの適用方法",
				" * `ket_bcd.h` と `ket_bcd.cpp` を対象プロジェクトへコピー。",
				" *",
				" * @par C++バージョン要件",
				" * 最小要件：C++17。",
				" * 本ライブラリの適用を推奨する C++ バージョン：C++17以降。",
				" * 推奨理由：試験用の理由。",
				" * 本ライブラリの適用を推奨しない C++ バージョン：なし。",
				" * 非推奨理由：なし。",
				" *",
				" * @par 他のライブラリへの依存",
				" * 標準ライブラリのみ。",
				" * 他のket moduleへの依存なし。",
				" *",
				" * @par namespace",
				" * 公開API：ket::bcd",
				" * 内部実装：ket::bcd::detail",
				" */",
				"",
			)
		)

	def make_repo(self) -> tempfile.TemporaryDirectory[str]:
		repository = tempfile.TemporaryDirectory()
		root = Path(repository.name)
		(root / "modules" / "bcd").mkdir(parents=True)
		(root / "modules" / "bcd" / "ket_bcd.h").write_text(
			self.header_preamble(),
			encoding="utf-8",
		)
		(root / "modules" / "bcd" / "ket_bcd.cpp").write_text(
			'#include "ket_bcd.h"\n\nint KetBcdAnchor()\n{\n\treturn 0;\n}\n',
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

	def test_header_only_module_layout_has_no_errors(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "bcd" / "ket_bcd.cpp").unlink()
			(root / "CMakeLists.txt").write_text(
				"\n".join(
					(
						"ket_add_module_test(",
						"    ket_bcd_test",
						"    SOURCES",
						"        modules/bcd/ket_bcd_test.cpp",
						")",
						"",
					)
				),
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertEqual(errors, [])

	def test_missing_standard_file_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "bcd" / "ket_bcd_test.cpp").unlink()

			errors = check_layout.collect_layout_errors(root)

			self.assertIn("modules/bcd/ket_bcd_test.cpp: required module file is missing.", errors)

	def test_placeholder_source_file_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "bcd" / "ket_bcd.cpp").write_text(
				'#include "ket_bcd.h"\n\n// 実装なし\n',
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.cpp: header-only module must omit placeholder source file.",
				errors,
			)

	def test_include_only_source_file_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "bcd" / "ket_bcd.cpp").write_text(
				'#include "ket_bcd.h"\n#include <string>\n',
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.cpp: header-only module must omit placeholder source file.",
				errors,
			)

	def test_block_comment_only_source_file_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "bcd" / "ket_bcd.cpp").write_text(
				'#include "ket_bcd.h"\n\n/*\n * 実装なし\n */\n',
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.cpp: header-only module must omit placeholder source file.",
				errors,
			)

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

			self.assertIn("modules/bcd/ket_bcd.h: header preamble must use Doxygen @file.", errors)
			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe project application method.",
				errors,
			)
			self.assertIn("modules/bcd/ket_bcd.h: header preamble must describe dependencies.", errors)
			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe namespace ket public API.",
				errors,
			)
			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe namespace ket detail implementation.",
				errors,
			)

	def test_missing_file_tag_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_preamble().replace(" * @file ket_bcd.h\n", "")
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn("modules/bcd/ket_bcd.h: header preamble must use Doxygen @file.", errors)

	def test_missing_cpp_version_requirements_section_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_preamble().replace(" * @par C++バージョン要件\n", "")
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe C++ version requirements.",
				errors,
			)

	def test_missing_recommended_standard_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_preamble().replace(
				"本ライブラリの適用を推奨する C++ バージョン：", "推奨："
			)
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe recommended C++ standard.",
				errors,
			)

	def test_missing_recommended_reason_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_preamble().replace("推奨理由：", "理由：")
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe recommended C++ standard reason.",
				errors,
			)

	def test_missing_not_recommended_standard_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_preamble().replace(
				"本ライブラリの適用を推奨しない C++ バージョン：", "非推奨："
			)
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe non-recommended C++ standards.",
				errors,
			)

	def test_missing_not_recommended_reason_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_preamble().replace("非推奨理由：", "理由：")
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe non-recommended C++ standards reason.",
				errors,
			)

	def test_missing_namespace_section_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_preamble().replace(" * @par namespace\n", "")
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn("modules/bcd/ket_bcd.h: header preamble must describe namespace.", errors)

	def test_missing_namespace_name_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_preamble().replace(" * 公開API：ket::bcd\n", "")
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe namespace ket public API.",
				errors,
			)

	def test_missing_detail_namespace_name_is_reported(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_preamble().replace(" * 内部実装：ket::bcd::detail\n", "")
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble must describe namespace ket detail implementation.",
				errors,
			)


if __name__ == "__main__":
	unittest.main()
