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

	def test_header_definition_after_documented_declaration_is_accepted(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief Value doubling.",
					" * @param[in] value Input value.",
					" * @retval value Doubled value.",
					" * @pre Caller provides an integer value.",
					" * @post No external state changes.",
					" */",
					"constexpr int Double(int value) noexcept;",
					"",
					"constexpr int Double(int value) noexcept",
					"{",
					"	return value * 2;",
					"}",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_class_qualified_header_definition_after_documented_declaration_is_accepted(
		self,
	) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API declarations",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\t/**",
					"\t * @brief Joining owner.",
					"\t */",
					"\tclass Owner",
					"\t{",
					"\t  public:",
					"\t\t/**",
					"\t\t * @brief Constructs an owner.",
					"\t\t * @param[in] value Initial value.",
					"\t\t * @pre Caller provides a valid value.",
					"\t\t * @post Owner stores value.",
					"\t\t * @code",
					"\t\t * ket::Owner owner(1);",
					"\t\t * @endcode",
					"\t\t */",
					"\t\texplicit Owner(int value) noexcept;",
					"",
					"\t\t/**",
					"\t\t * @brief Returns stored value.",
					"\t\t * @retval value Stored value.",
					"\t\t * @pre Owner is alive.",
					"\t\t * @post Owner state is unchanged.",
					"\t\t * @code",
					"\t\t * ket::Owner owner(1);",
					"\t\t * const auto value = owner.Get();",
					"\t\t * @endcode",
					"\t\t */",
					"\t\tint Get() const noexcept; // NOLINT(modernize-use-nodiscard)",
					"",
					"\t  private:",
					"\t\tint value_;",
					"\t};",
					"",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API definitions",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\tinline Owner::Owner(int value) noexcept",
					"\t\t: value_(value)",
					"\t{",
					"\t}",
					"",
					"\tinline int Owner::Get() const noexcept // NOLINT(modernize-use-nodiscard)",
					"\t{",
					"\t\treturn value_;",
					"\t}",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_header_definition_comment_duplicate_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief Value doubling.",
					" * @param[in] value Input value.",
					" * @retval value Doubled value.",
					" * @pre Caller provides an integer value.",
					" * @post No external state changes.",
					" */",
					"constexpr int Double(int value) noexcept;",
					"",
					"/**",
					" * @brief Value doubling.",
					" * @param[in] value Input value.",
					" * @retval value Doubled value.",
					" * @pre Caller provides an integer value.",
					" * @post No external state changes.",
					" */",
					"constexpr int Double(int value) noexcept",
					"{",
					"	return value * 2;",
					"}",
					"",
				)
			),
		)

		self.assertTrue(
			any(
				"function definition must not duplicate Doxygen comment from its declaration." in error
				for error in errors
			)
		)

	def test_class_qualified_header_definition_comment_duplicate_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t/**",
					"\t * @brief Sample owner.",
					"\t */",
					"\tclass Owner",
					"\t{",
					"\t  public:",
					"\t\t/**",
					"\t\t * @brief Returns stored value.",
					"\t\t * @retval value Stored value.",
					"\t\t * @pre Owner is alive.",
					"\t\t * @post Owner state is unchanged.",
					"\t\t * @code",
					"\t\t * ket::Owner owner;",
					"\t\t * const auto value = owner.Get();",
					"\t\t * @endcode",
					"\t\t */",
					"\t\tint Get() const noexcept;",
					"\t};",
					"",
					"\t/**",
					"\t * @brief Returns stored value.",
					"\t * @retval value Stored value.",
					"\t * @pre Owner is alive.",
					"\t * @post Owner state is unchanged.",
					"\t * @code",
					"\t * ket::Owner owner;",
					"\t * const auto value = owner.Get();",
					"\t * @endcode",
					"\t */",
					"\tinline int Owner::Get() const noexcept",
					"\t{",
					"\t\treturn 0;",
					"\t}",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertTrue(
			any(
				"function definition must not duplicate Doxygen comment from its declaration." in error
				for error in errors
			)
		)

	def test_header_section_banners_are_accepted(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API declarations",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\t/**",
					"\t * @brief Value doubling.",
					"\t * @param[in] value Input value.",
					"\t * @retval value Doubled value.",
					"\t * @pre Caller provides an integer value.",
					"\t * @post No external state changes.",
					"\t * @code",
					"\t * const auto value = Double(2);",
					"\t * // value == 4",
					"\t * @endcode",
					"\t */",
					"\tconstexpr int Double(int value) noexcept;",
					"",
					"\t// -----------------------------------------------------------------------------",
					"\t// Internal implementation details",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\tnamespace detail",
					"\t{",
					"\t\t/**",
					"\t\t * @brief Identity conversion.",
					"\t\t * @param[in] value Input value.",
					"\t\t * @retval value Input value.",
					"\t\t * @pre Caller provides an integer value.",
					"\t\t * @post No external state changes.",
					"\t\t */",
					"\t\tconstexpr int Identity(int value) noexcept",
					"\t\t{",
					"\t\t\treturn value;",
					"\t\t}",
					"",
					"\t} // namespace detail",
					"",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API definitions",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\tconstexpr int Double(int value) noexcept",
					"\t{",
					"\t\treturn detail::Identity(value) * 2;",
					"\t}",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_public_api_function_requires_code_example(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API declarations",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\t/**",
					"\t * @brief Value doubling.",
					"\t * @param[in] value Input value.",
					"\t * @retval value Doubled value.",
					"\t * @pre Caller provides an integer value.",
					"\t * @post No external state changes.",
					"\t */",
					"\tconstexpr int Double(int value) noexcept;",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertTrue(
			any(
				"public API function Doxygen comment requires @code and @endcode." in error
				for error in errors
			)
		)

	def test_package_public_api_function_requires_code_example(self) -> None:
		errors = self.check_text(
			"packages/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API declarations",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\t/**",
					"\t * @brief Value doubling.",
					"\t * @param[in] value Input value.",
					"\t * @retval value Doubled value.",
					"\t * @pre Caller provides an integer value.",
					"\t * @post No external state changes.",
					"\t */",
					"\tconstexpr int Double(int value) noexcept;",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertTrue(
			any(
				"public API function Doxygen comment requires @code and @endcode." in error
				for error in errors
			)
		)

	def test_private_member_in_public_section_does_not_require_code_example(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API declarations",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\t/**",
					"\t * @brief Sample value.",
					"\t */",
					"\tclass Value",
					"\t{",
					"\t  public:",
					"\t\t/**",
					"\t\t * @brief Value doubling.",
					"\t\t * @param[in] value Input value.",
					"\t\t * @retval value Doubled value.",
					"\t\t * @pre Caller provides an integer value.",
					"\t\t * @post No external state changes.",
					"\t\t * @code",
					"\t\t * const auto value = Value::Double(2);",
					"\t\t * // value == 4",
					"\t\t * @endcode",
					"\t\t */",
					"\t\tstatic int Double(int value) noexcept;",
					"",
					"\t  private:",
					"\t\t/**",
					"\t\t * @brief Private construction.",
					"\t\t * @pre Internal caller only.",
					"\t\t * @post No external state changes.",
					"\t\t */",
					"\t\tValue() noexcept;",
					"\t};",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertFalse(
			any("public API function Doxygen comment requires @code and @endcode." in error for error in errors)
		)

	def test_constructor_and_destructor_without_retval_are_accepted(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API declarations",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\t/**",
					"\t * @brief Sample value.",
					"\t */",
					"\tclass Value",
					"\t{",
					"\t  public:",
					"\t\t/**",
					"\t\t * @brief Empty value construction.",
					"\t\t * @pre Caller needs an empty value.",
					"\t\t * @post Value has no stored payload.",
					"\t\t * @code",
					"\t\t * ket::Value value;",
					"\t\t * @endcode",
					"\t\t */",
					"\t\tValue() = default;",
					"",
					"\t\t/**",
					"\t\t * @brief Copy construction.",
					"\t\t * @param[in] other Source value.",
					"\t\t * @pre `other` is a valid Value object.",
					"\t\t * @post New value holds the same payload as `other`.",
					"\t\t * @code",
					"\t\t * ket::Value source;",
					"\t\t * ket::Value copy(source);",
					"\t\t * @endcode",
					"\t\t */",
					"\t\tValue(const Value& other) = default;",
					"",
					"\t\t/**",
					"\t\t * @brief Value destruction.",
					"\t\t * @pre Object lifetime is ending.",
					"\t\t * @post Owned resources are released.",
					"\t\t * @code",
					"\t\t * {",
					"\t\t * \tket::Value value;",
					"\t\t * }",
					"\t\t * @endcode",
					"\t\t */",
					"\t\t~Value() = default;",
					"\t};",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_constructor_return_tag_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t/**",
					"\t * @brief Sample value.",
					"\t */",
					"\tclass Value",
					"\t{",
					"\t  private:",
					"\t\t/**",
					"\t\t * @brief Private construction.",
					"\t\t * @retval value Constructed value.",
					"\t\t * @pre Internal caller only.",
					"\t\t * @post No external state changes.",
					"\t\t */",
					"\t\tValue() noexcept;",
					"\t};",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertTrue(
			any(
				"constructor/destructor Doxygen comment must not use @retval or @return." in error
				for error in errors
			)
		)

	def test_missing_header_section_banner_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t/**",
					"\t * @brief Value doubling.",
					"\t * @param[in] value Input value.",
					"\t * @retval value Doubled value.",
					"\t * @pre Caller provides an integer value.",
					"\t * @post No external state changes.",
					"\t */",
					"\tconstexpr int Double(int value) noexcept;",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertTrue(
			any("module header requires section banner: Public API declarations." in error for error in errors)
		)

	def test_header_section_banner_order_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API definitions",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\t// -----------------------------------------------------------------------------",
					"\t// Public API declarations",
					"\t// -----------------------------------------------------------------------------",
					"",
					"\t/**",
					"\t * @brief Value doubling.",
					"\t * @param[in] value Input value.",
					"\t * @retval value Doubled value.",
					"\t * @pre Caller provides an integer value.",
					"\t * @post No external state changes.",
					"\t */",
					"\tconstexpr int Double(int value) noexcept;",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertTrue(any("section banners must follow standard order." in error for error in errors))

	def test_header_section_banner_format_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"namespace ket",
					"{",
					"\t// -----",
					"\t// Public API declarations",
					"\t// -----",
					"",
					"\t/**",
					"\t * @brief Value doubling.",
					"\t * @param[in] value Input value.",
					"\t * @retval value Doubled value.",
					"\t * @pre Caller provides an integer value.",
					"\t * @post No external state changes.",
					"\t */",
					"\tconstexpr int Double(int value) noexcept;",
					"",
					"} // namespace ket",
					"",
				)
			),
		)

		self.assertTrue(
			any("section banner must use exact format: Public API declarations." in error for error in errors)
		)

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

	def test_header_type_comment_before_template_specialization_is_accepted(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief Generic trait.",
					" * @tparam T Checked type.",
					" */",
					"template <typename T>",
					"struct Traits",
					"{",
					"};",
					"",
					"/**",
					" * @brief Integer trait specialization.",
					" */",
					"template <>",
					"struct Traits<int>",
					"{",
					"};",
					"",
				)
			),
		)

		self.assertEqual(errors, [])

	def test_missing_header_type_comment_before_template_specialization_is_reported(self) -> None:
		errors = self.check_text(
			"modules/sample/ket_sample.h",
			"\n".join(
				(
					"/**",
					" * @brief Generic trait.",
					" * @tparam T Checked type.",
					" */",
					"template <typename T>",
					"struct Traits",
					"{",
					"};",
					"",
					"template <>",
					"struct Traits<int>",
					"{",
					"};",
					"",
				)
			),
		)

		self.assertTrue(any("type declaration requires a Doxygen comment." in error for error in errors))

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
