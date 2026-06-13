#!/usr/bin/env python3

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import check_conventions
import ket_tooling


class CheckConventionsTest(unittest.TestCase):
	def check_text(self, relative_path: str, text: str) -> list[str]:
		with tempfile.TemporaryDirectory(dir=ket_tooling.ROOT) as root_name:
			path = Path(root_name) / relative_path
			path.parent.mkdir(parents=True, exist_ok=True)
			path.write_text(text, encoding="utf-8")

			return check_conventions.check_file(path)

	def test_multiline_header_function_comment_is_accepted(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief Value readiness check.",
					" * @param[in] value Input value.",
					" * @retval true Value is ready.",
					" * @retval false Value is not ready.",
					" * @pre Caller provides an integer value.",
					" * @post No external state changes.",
					" */",
					"bool IsReady(",
					"	int value) noexcept;",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_missing_function_tag_text_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief",
					" * @param[in] value Input value.",
					" * @retval true Ready.",
					" * @pre Caller provides an integer value.",
					" * @post No external state changes.",
					" */",
					"bool IsReady(int value) noexcept;",
					"",
				)
			),
		)

		self.assertTrue(any("function Doxygen comment requires @brief." in error for error in errors))

	def test_parameter_without_description_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief Value readiness check.",
					" * @param[in] value",
					" * @retval true Ready.",
					" * @retval false Not ready.",
					" * @pre Caller provides an integer value.",
					" * @post No external state changes.",
					" */",
					"bool IsReady(int value) noexcept;",
					"",
				)
			),
		)

		self.assertTrue(
			any(
				"function Doxygen comment requires @param[in], @param[out], or @param[in,out]." in error
				for error in errors
			)
		)

	def test_inout_parameter_comment_is_accepted(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief Value update.",
					" * @param[in,out] value Updated value.",
					" * @retval void Return value none.",
					" * @pre Caller provides a valid reference.",
					" * @post Value may be changed.",
					" */",
					"void Update(int& value);",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_legacy_inout_parameter_comment_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief Value update.",
					" * @param[in/out] value Updated value.",
					" * @retval void Return value none.",
					" * @pre Caller provides a valid reference.",
					" * @post Value may be changed.",
					" */",
					"void Update(int& value);",
					"",
				)
			),
		)

		self.assertTrue(
			any(
				"function Doxygen comment requires @param[in], @param[out], or @param[in,out]." in error
				for error in errors
			)
		)

	def test_header_type_comment_is_accepted(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief Value trait.",
					" * @tparam T Checked type.",
					" */",
					"template <typename T>",
					"struct IsValue",
					"{",
					"};",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_missing_header_type_comment_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"struct Value",
					"{",
					"};",
					"",
				)
			),
		)

		self.assertTrue(any("type declaration requires a Doxygen comment." in error for error in errors))

	def test_header_type_comment_requires_brief(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @note Internal type.",
					" */",
					"struct Value",
					"{",
					"};",
					"",
				)
			),
		)

		self.assertTrue(any("type Doxygen comment requires @brief." in error for error in errors))

	def test_test_comment_requires_details_tag(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample_test.cpp",
			"\n".join(
				(
					"/**",
					" * @test",
					" * @brief Minimal check.",
					" * @pre Test harness is configured.",
					" * @post No external state changes.",
					" */",
					"TEST(SampleTest, Works)",
					"{",
					"	const auto value = true;",
					"	EXPECT_TRUE(value);",
					"}",
					"",
				)
			),
		)

		self.assertTrue(any("TEST Doxygen comment requires @details." in error for error in errors))

	def test_condition_api_call_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.cpp",
			"\n".join(
				(
					"void Check()",
					"{",
					"	if (value.has_value())",
					"	{",
					"	}",
					"}",
					"",
				)
			),
		)

		self.assertTrue(any("condition expression must not call an API directly." in error for error in errors))

	def test_sizeof_condition_is_allowed(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.cpp",
			"\n".join(
				(
					"void Check()",
					"{",
					"	if (sizeof(int) > 0)",
					"	{",
					"	}",
					"}",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_throw_expression_is_not_treated_as_function(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"#include <stdexcept>",
					"",
					"/**",
					" * @brief Error raise.",
					" * @retval void Return value none.",
					" * @pre Error path selected.",
					" * @post Always throws std::runtime_error.",
					" */",
					"inline void Raise()",
					"{",
					'	throw std::runtime_error("sample");',
					"}",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_namespace_end_comment_requires_blank_line(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.cpp",
			"\n".join(
				(
					"namespace ket",
					"{",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertTrue(
			any("namespace end comment requires one blank line before it." in error for error in errors)
		)

	def test_polite_comment_wording_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.cpp",
			"\n".join(
				(
					"// 値を確認します。",
					"void Check()",
					"{",
					"}",
					"",
				)
			),
		)

		self.assertTrue(any("C++ comment must not use 'ます'." in error for error in errors))


if __name__ == "__main__":
	unittest.main()
