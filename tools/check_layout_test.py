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

	def header_with_detail_namespace(self) -> str:
		return self.header_preamble() + "\n".join(
			(
				"namespace ket",
				"{",
				"\tnamespace bcd",
				"\t{",
				"\t\tnamespace detail",
				"\t\t{",
				"\t\t\tconstexpr int kValue = 1;",
				"",
				"\t\t} // namespace detail",
				"",
				"\t} // namespace bcd",
				"",
				"} // namespace ket",
				"",
			)
		)

	def header_without_detail_namespace(self) -> str:
		return self.header_preamble() + "\n".join(
			(
				"namespace ket",
				"{",
				"\tnamespace bcd",
				"\t{",
				"",
				"\t} // namespace bcd",
				"",
				"} // namespace ket",
				"",
			)
		)

	def source_with_anonymous_namespace(self) -> str:
		return "\n".join(
			(
				'#include "ket_bcd.h"',
				"",
				"namespace",
				"{",
				"\tconstexpr int kValue = 1;",
				"",
				"} // namespace",
				"",
				"int KetBcdAnchor()",
				"{",
				"\treturn kValue;",
				"}",
				"",
			)
		)

	def package_header_preamble(self) -> str:
		return self.header_preamble().replace("ket_bcd.h", "ket_wire.h").replace(
			"ket::bcd", "ket::wire"
		)

	def make_repo(self) -> tempfile.TemporaryDirectory[str]:
		repository = tempfile.TemporaryDirectory()
		root = Path(repository.name)
		(root / "modules" / "bcd").mkdir(parents=True)
		(root / "modules" / "bcd" / "ket_bcd.h").write_text(
			self.header_with_detail_namespace(),
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

	def make_package_repo(self) -> tempfile.TemporaryDirectory[str]:
		repository = tempfile.TemporaryDirectory()
		root = Path(repository.name)
		(root / "packages" / "wire").mkdir(parents=True)
		(root / "docs" / "packages").mkdir(parents=True)
		(root / "packages" / "wire" / "ket_wire.h").write_text(
			self.package_header_preamble()
			+ "\n".join(
				(
					'#include "ket_bcd.h"',
					"",
					"namespace ket",
					"{",
					"\tnamespace wire",
					"\t{",
					"\t\tnamespace detail",
					"\t\t{",
					"\t\t\tconstexpr int kValue = 1;",
					"",
					"\t\t} // namespace detail",
					"",
					"\t} // namespace wire",
					"",
					"} // namespace ket",
					"",
				)
			),
			encoding="utf-8",
		)
		(root / "packages" / "wire" / "ket_wire_test.cpp").write_text(
			'#include "ket_wire.h"\n',
			encoding="utf-8",
		)
		(root / "CMakeLists.txt").write_text(
			"\n".join(
				(
					"ket_add_module_test(",
					"    ket_wire_test",
					"    SOURCES",
					"        packages/wire/ket_wire_test.cpp",
					")",
					"",
				)
			),
			encoding="utf-8",
		)
		(root / "progress.md").write_text(
			"| Component | Status |\n| --------- | ------ |\n| wire | verified |\n",
			encoding="utf-8",
		)
		(root / "docs" / "packages" / "ket_wire_design.md").write_text(
			"packages/wire/ket_wire.h\npackages/wire/ket_wire_test.cpp\n",
			encoding="utf-8",
		)
		return repository

	def test_valid_module_layout_has_no_errors(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)

			errors = check_layout.collect_layout_errors(root)

			self.assertEqual(errors, [])

	def test_valid_package_layout_has_no_errors(self) -> None:
		with self.make_package_repo() as root_name:
			root = Path(root_name)

			errors = check_layout.collect_layout_errors(root)

			self.assertEqual(errors, [])

	def test_package_missing_design_doc_is_reported(self) -> None:
		with self.make_package_repo() as root_name:
			root = Path(root_name)
			(root / "docs" / "packages" / "ket_wire_design.md").unlink()

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"docs/packages/ket_wire_design.md: package 'wire' design document is missing.",
				errors,
			)

	def test_package_missing_standard_file_is_reported(self) -> None:
		with self.make_package_repo() as root_name:
			root = Path(root_name)
			(root / "packages" / "wire" / "ket_wire_test.cpp").unlink()

			errors = check_layout.collect_layout_errors(root)

			self.assertIn("packages/wire/ket_wire_test.cpp: required package file is missing.", errors)

	def test_source_file_anonymous_namespace_layout_has_no_errors(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = root / "modules" / "bcd" / "ket_bcd.h"
			header.write_text(
				self.header_without_detail_namespace().replace(
					"内部実装：ket::bcd::detail",
					"内部実装：.cpp の無名 namespace",
				),
				encoding="utf-8",
			)
			(root / "modules" / "bcd" / "ket_bcd.cpp").write_text(
				self.source_with_anonymous_namespace(),
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertEqual(errors, [])

	def test_no_internal_implementation_layout_has_no_errors(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = root / "modules" / "bcd" / "ket_bcd.h"
			header.write_text(
				self.header_without_detail_namespace().replace(
					"内部実装：ket::bcd::detail",
					"内部実装：なし",
				),
				encoding="utf-8",
			)

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

	def test_build_config_macro_only_layout_has_no_errors(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			root = Path(root_name)
			(root / "modules" / "build_config").mkdir(parents=True)
			(root / "modules" / "build_config" / "ket_build_config.h").write_text(
				"\n".join(
					(
						"#pragma once",
						"",
						"/**",
						" * @file ket_build_config.h",
						" * @brief build config macro。",
						" *",
						" * @details macro-only module。",
						" *",
						" * @par プロジェクトへの適用方法",
						" * `ket_build_config.h` を対象プロジェクトへコピー。",
						" *",
						" * @par C++バージョン要件",
						" * 最小要件：C++11。",
						" * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。",
						" * 推奨理由：試験用の理由。",
						" * 本ライブラリの適用を推奨しない C++ バージョン：なし。",
						" * 非推奨理由：なし。",
						" *",
						" * @par 他のライブラリへの依存",
						" * 標準ライブラリのみ。",
						" * 他のket moduleへの依存なし。",
						" *",
						" * @par namespace",
						" * 公開API：global KET_* macros（C++ namespaceなし）。",
						" * 内部実装：KET_DETAIL_* macros（ヘッダ末尾で#undef）。",
						" */",
						"",
						"#define KET_DETAIL_CXX_VERSION_VALUE 201703L",
						"#define KET_CXX_VERSION KET_DETAIL_CXX_VERSION_VALUE",
						"#undef KET_DETAIL_CXX_VERSION_VALUE",
						"",
					)
				),
				encoding="utf-8",
			)
			(root / "modules" / "build_config" / "ket_build_config_test.cpp").write_text(
				'#include "ket_build_config.h"\n',
				encoding="utf-8",
			)
			(root / "CMakeLists.txt").write_text(
				"\n".join(
					(
						"ket_add_module_test(",
						"    ket_build_config_test",
						"    SOURCES",
						"        modules/build_config/ket_build_config_test.cpp",
						")",
						"",
					)
				),
				encoding="utf-8",
			)
			(root / "progress.md").write_text(
				"| Module | Status |\n| ------ | ------ |\n| build_config | verified |\n",
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertEqual(errors, [])

	def test_build_config_macro_only_layout_reports_namespace(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			root = Path(root_name)
			(root / "modules" / "build_config").mkdir(parents=True)
			(root / "modules" / "build_config" / "ket_build_config.h").write_text(
				"\n".join(
					(
						"#pragma once",
						"",
						"/**",
						" * @file ket_build_config.h",
						" * @brief build config macro。",
						" *",
						" * @details macro-only module。",
						" *",
						" * @par プロジェクトへの適用方法",
						" * `ket_build_config.h` を対象プロジェクトへコピー。",
						" *",
						" * @par C++バージョン要件",
						" * 最小要件：C++11。",
						" * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。",
						" * 推奨理由：試験用の理由。",
						" * 本ライブラリの適用を推奨しない C++ バージョン：なし。",
						" * 非推奨理由：なし。",
						" *",
						" * @par 他のライブラリへの依存",
						" * 標準ライブラリのみ。",
						" * 他のket moduleへの依存なし。",
						" *",
						" * @par namespace",
						" * 公開API：global KET_* macros（C++ namespaceなし）。",
						" * 内部実装：KET_DETAIL_* macros（ヘッダ末尾で#undef）。",
						" */",
						"",
						"namespace ket",
						"{",
						"}",
						"#define KET_DETAIL_CXX_VERSION_VALUE 201703L",
						"#define KET_CXX_VERSION KET_DETAIL_CXX_VERSION_VALUE",
						"#undef KET_DETAIL_CXX_VERSION_VALUE",
						"",
					)
				),
				encoding="utf-8",
			)
			(root / "modules" / "build_config" / "ket_build_config_test.cpp").write_text(
				'#include "ket_build_config.h"\n',
				encoding="utf-8",
			)
			(root / "CMakeLists.txt").write_text(
				"ket_add_module_test(\n"
				"    ket_build_config_test\n"
				"    SOURCES\n"
				"        modules/build_config/ket_build_config_test.cpp\n"
				")\n",
				encoding="utf-8",
			)
			(root / "progress.md").write_text(
				"| Module | Status |\n| ------ | ------ |\n| build_config | verified |\n",
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/build_config/ket_build_config.h: macro-only module must not declare a C++ namespace.",
				errors,
			)

	def test_build_config_macro_only_layout_reports_missing_detail_macro(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			root = Path(root_name)
			(root / "modules" / "build_config").mkdir(parents=True)
			(root / "modules" / "build_config" / "ket_build_config.h").write_text(
				"\n".join(
					(
						"#pragma once",
						"",
						"/**",
						" * @file ket_build_config.h",
						" * @brief build config macro。",
						" *",
						" * @details macro-only module。",
						" *",
						" * @par プロジェクトへの適用方法",
						" * `ket_build_config.h` を対象プロジェクトへコピー。",
						" *",
						" * @par C++バージョン要件",
						" * 最小要件：C++11。",
						" * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。",
						" * 推奨理由：試験用の理由。",
						" * 本ライブラリの適用を推奨しない C++ バージョン：なし。",
						" * 非推奨理由：なし。",
						" *",
						" * @par 他のライブラリへの依存",
						" * 標準ライブラリのみ。",
						" * 他のket moduleへの依存なし。",
						" *",
						" * @par namespace",
						" * 公開API：global KET_* macros（C++ namespaceなし）。",
						" * 内部実装：KET_DETAIL_* macros（ヘッダ末尾で#undef）。",
						" */",
						"",
						"#define KET_CXX_VERSION 201703L",
						"",
					)
				),
				encoding="utf-8",
			)
			(root / "modules" / "build_config" / "ket_build_config_test.cpp").write_text(
				'#include "ket_build_config.h"\n',
				encoding="utf-8",
			)
			(root / "CMakeLists.txt").write_text(
				"ket_add_module_test(\n"
				"    ket_build_config_test\n"
				"    SOURCES\n"
				"        modules/build_config/ket_build_config_test.cpp\n"
				")\n",
				encoding="utf-8",
			)
			(root / "progress.md").write_text(
				"| Module | Status |\n| ------ | ------ |\n| build_config | verified |\n",
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/build_config/ket_build_config.h: macro-only module must keep KET_DETAIL macro implementation local to the header.",
				errors,
			)

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
				"modules/bcd/ket_bcd.h: header preamble must describe internal implementation namespace.",
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
				"modules/bcd/ket_bcd.h: header preamble must describe internal implementation namespace.",
				errors,
			)

	def test_detail_namespace_description_requires_header_detail_namespace(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(
				self.header_without_detail_namespace(),
				encoding="utf-8",
			)

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble says ket detail implementation, but header has no namespace detail.",
				errors,
			)

	def test_anonymous_namespace_description_requires_source_anonymous_namespace(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_without_detail_namespace().replace(
				"内部実装：ket::bcd::detail",
				"内部実装：.cpp の無名 namespace",
			)
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble says .cpp anonymous namespace, but source has no anonymous namespace.",
				errors,
			)

	def test_no_internal_implementation_rejects_existing_helpers(self) -> None:
		with self.make_repo() as root_name:
			root = Path(root_name)
			header = self.header_with_detail_namespace().replace(
				"内部実装：ket::bcd::detail",
				"内部実装：なし",
			)
			(root / "modules" / "bcd" / "ket_bcd.h").write_text(header, encoding="utf-8")

			errors = check_layout.collect_layout_errors(root)

			self.assertIn(
				"modules/bcd/ket_bcd.h: header preamble says no internal implementation, but header has namespace detail.",
				errors,
			)


if __name__ == "__main__":
	unittest.main()
