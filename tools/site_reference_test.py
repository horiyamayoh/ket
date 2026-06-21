#!/usr/bin/env python3

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

import site_reference


class SiteReferenceTest(unittest.TestCase):
	def test_parse_doxygen_collects_contract_and_code(self) -> None:
		comment = "\n".join(
			(
				"/**",
				" * @brief 値を読み取る。",
				" * @param[out] out 読み取り結果。",
				" * @retval true 成功。",
				" * @retval false 失敗。",
				" * @pre outは有効な参照。",
				" * @post 成功時だけoutを変更。",
				" * @note 失敗時は状態不変。",
				" * @code",
				" * int value = 0;",
				" * const auto ok = Read(value);",
				" * @endcode",
				" */",
			)
		)

		parsed = site_reference.parse_doxygen(comment)

		self.assertEqual(parsed.brief, "値を読み取る。")
		self.assertIn("out:", parsed.params[0])
		self.assertEqual(len(parsed.retvals), 2)
		self.assertEqual(parsed.pre, "outは有効な参照。")
		self.assertEqual(parsed.post, "成功時だけoutを変更。")
		self.assertEqual(parsed.notes, ("失敗時は状態不変。",))
		self.assertIn("const auto ok", parsed.code_blocks[0])

	def test_extract_public_api_keeps_public_members_aliases_and_macros(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			root = Path(root_name)
			header = root / "modules" / "demo" / "ket_demo.h"
			header.parent.mkdir(parents=True)
			header.write_text(self.demo_header(), encoding="utf-8")

			_, apis = site_reference.extract_public_api(header)
			api_names = {(api.kind, api.name) for api in apis}

			self.assertIn(("type", "Example"), api_names)
			self.assertIn(("function", "Example"), api_names)
			self.assertIn(("function", "Read"), api_names)
			self.assertIn(("alias", "Identity"), api_names)
			self.assertIn(("macro", "KET_DEMO"), api_names)
			self.assertNotIn(("function", "Hidden"), api_names)

	def test_validate_model_reports_unknown_references(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			root = Path(root_name)
			self.write_minimal_site_root(root, unknown_use_case_module=True)

			model = site_reference.load_site_model(root)
			errors = site_reference.validate_model(model)

			self.assertTrue(any("unknown module 'missing'" in error for error in errors))

	def test_compare_directories_detects_stale_output(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			root = Path(root_name)
			generated = root / "generated"
			committed = root / "committed"
			generated.mkdir()
			committed.mkdir()
			(generated / "index.html").write_text("new", encoding="utf-8")
			(committed / "index.html").write_text("old", encoding="utf-8")

			errors = site_reference.compare_directories(generated, committed)

			self.assertIn("committed site differs from generated output: index.html.", errors)

	def test_validate_html_links_detects_broken_relative_link(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			site = Path(root_name)
			(site / "index.html").write_text(
				'<a href="missing.html">missing</a>',
				encoding="utf-8",
			)

			errors = site_reference.validate_html_links(site)

			self.assertIn("index.html has broken link: missing.html", errors)

	def write_minimal_site_root(self, root: Path, unknown_use_case_module: bool) -> None:
		(root / "modules" / "demo").mkdir(parents=True)
		(root / "modules" / "demo" / "ket_demo.h").write_text(self.demo_header(), encoding="utf-8")
		(root / "progress.md").write_text(
			"\n".join(
				(
					"| Module | Category | Status | C++ Min | Tests | Format | Notes |",
					"| ------ | -------- | ------ | ------- | ----- | ------ | ----- |",
					"| demo | demo | verified | C++17 | test | checked | demo module |",
				)
			),
			encoding="utf-8",
		)
		src = root / "docs" / "site_src"
		(src / "modules").mkdir(parents=True)
		(src / "use_cases").mkdir()
		(src / "site.json").write_text(
			json.dumps(
				{
					"title": "demo",
					"subtitle": "demo",
					"description": "demo",
					"module_order": ["demo"],
					"home_use_cases": ["demo-case"],
				}
			),
			encoding="utf-8",
		)
		(src / "categories.json").write_text(
			json.dumps(
				{
					"categories": [
						{
							"id": "demo-cat",
							"title": "Demo",
							"summary": "Demo category.",
							"modules": ["demo"],
						}
					]
				}
			),
			encoding="utf-8",
		)
		(src / "modules" / "demo.json").write_text(
			json.dumps(
				{
					"module": "demo",
					"title": "Demo",
					"summary": "Demo module.",
					"categories": ["demo-cat"],
					"use_cases": ["demo-case"],
					"when_to_use": ["demo"],
					"when_not_to_use": ["demo"],
					"related_modules": [],
					"notes": [],
				}
			),
			encoding="utf-8",
		)
		use_case_module = "missing" if unknown_use_case_module else "demo"
		(src / "use_cases" / "demo-case.json").write_text(
			json.dumps(
				{
					"id": "demo-case",
					"title": "Demo use case",
					"summary": "Demo summary.",
					"flow": ["step"],
					"modules": [{"module": use_case_module, "role": "role"}],
					"recipe": ["recipe"],
					"see_also": [],
				}
			),
			encoding="utf-8",
		)

	def demo_header(self) -> str:
		return "\n".join(
			(
				"#pragma once",
				"",
				"/**",
				" * @file ket_demo.h",
				" * @brief demo module。",
				" * @details demo details。",
				" * @par プロジェクトへの適用方法",
				" * `ket_demo.h` をコピー。",
				" * @par C++バージョン要件",
				" * 最小要件：C++17。",
				" * @par 他のライブラリへの依存",
				" * 標準ライブラリのみ。",
				" * @par namespace",
				" * 公開API：ket::demo",
				" * 内部実装：なし",
				" */",
				"namespace ket",
				"{",
				"\tnamespace demo",
				"\t{",
				"\t\t// -----------------------------------------------------------------------------",
				"\t\t// Public API declarations",
				"\t\t// -----------------------------------------------------------------------------",
				"",
				"\t\t/**",
				"\t\t * @brief demo type。",
				"\t\t */",
				"\t\tclass Example",
				"\t\t{",
				"\t\t  public:",
				"\t\t\t/**",
				"\t\t\t * @brief demo constructor。",
				"\t\t\t * @pre なし。",
				"\t\t\t * @post 構築済み。",
				"\t\t\t * @code",
				"\t\t\t * ket::demo::Example value;",
				"\t\t\t * @endcode",
				"\t\t\t */",
				"\t\t\tExample() noexcept;",
				"",
				"\t\t\t/**",
				"\t\t\t * @brief read value。",
				"\t\t\t * @param[out] out 出力。",
				"\t\t\t * @retval true 成功。",
				"\t\t\t * @retval false 失敗。",
				"\t\t\t * @pre outは有効な参照。",
				"\t\t\t * @post 成功時だけoutを変更。",
				"\t\t\t * @code",
				"\t\t\t * int out = 0;",
				"\t\t\t * value.Read(out);",
				"\t\t\t * @endcode",
				"\t\t\t */",
				"\t\t\tbool Read(int& out) noexcept;",
				"",
				"\t\t  private:",
				"\t\t\tvoid Hidden() noexcept;",
				"\t\t};",
				"",
				"\t\t/**",
				"\t\t * @brief identity alias。",
				"\t\t * @tparam T 対象型。",
				"\t\t * @code",
				"\t\t * using Value = ket::demo::Identity<int>;",
				"\t\t * @endcode",
				"\t\t */",
				"\t\ttemplate <typename T>",
				"\t\tusing Identity = T;",
				"",
				"\t} // namespace demo",
				"",
				"} // namespace ket",
				"",
				"/**",
				" * @brief demo macro。",
				" * @param value 入力。",
				" * @code",
				" * KET_DEMO(1);",
				" * @endcode",
				" */",
				"#define KET_DEMO(value) (value)",
				"",
			)
		)


if __name__ == "__main__":
	raise SystemExit(unittest.main())
