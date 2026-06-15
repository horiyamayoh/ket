#pragma once

/**
 * @file ket_build_config.h
 * @brief compiler、OS、標準ライブラリfeatureの判定macro。
 *
 * @details C++標準、compiler、OS、標準ライブラリfeature-test macroの差分を小さい
 * `KET_` prefix付きmacroへ集約する。ヘッダオンリーmoduleのため、drop-in時はヘッダ単体で
 * 持ち出す。runtime cost、allocation、link symbolは発生しない。
 *
 * @par プロジェクトへの適用方法
 * `ket_build_config.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：compiler、OS、feature macroの差分を小さい範囲へ閉じ込められる。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリとcompiler predefined macroのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket
 * 内部実装：ket::detail
 */

// -----------------------------------------------------------------------------
// Public API declarations
// -----------------------------------------------------------------------------

/**
 * @def KET_CXX_VERSION
 * @brief 有効なC++標準値。
 * @retval 201103L C++11。
 * @retval 201402L C++14。
 * @retval 201703L C++17。
 * @retval 202002L C++20。
 * @retval 202302L C++23以降。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_CXX_AT_LEAST
 * @brief C++標準値の下限判定。
 * @param[in] value 判定対象のC++標準値。
 * @retval 1 `KET_CXX_VERSION >= value`。
 * @retval 0 `KET_CXX_VERSION < value`。
 * @pre `value`はpreprocessor整数式として評価可能な値。
 * @post compile-time定数式のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_HAS_STD_OPTIONAL
 * @brief `std::optional` feature利用可否。
 * @retval 1 C++17以降、header存在、feature-test macro確認済み。
 * @retval 0 利用不能、または確認不能。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_HAS_STD_STRING_VIEW
 * @brief `std::string_view` feature利用可否。
 * @retval 1 C++17以降、header存在、feature-test macro確認済み。
 * @retval 0 利用不能、または確認不能。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_HAS_STD_SPAN
 * @brief `std::span` feature利用可否。
 * @retval 1 C++20以降、header存在、feature-test macro確認済み。
 * @retval 0 利用不能、または確認不能。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_HAS_STD_FORMAT
 * @brief `std::format` feature利用可否。
 * @retval 1 C++20以降、header存在、feature-test macro確認済み。
 * @retval 0 利用不能、または確認不能。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_COMPILER_CLANG
 * @brief clang系compiler判定。
 * @retval 1 `__clang__`定義あり。
 * @retval 0 `__clang__`定義なし。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_COMPILER_GCC
 * @brief GCC判定。
 * @retval 1 `__GNUC__`定義あり、かつclangではない。
 * @retval 0 GCCではない。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_COMPILER_MSVC
 * @brief MSVC compiler判定。
 * @retval 1 `_MSC_VER`定義あり、かつclangではない。
 * @retval 0 MSVCではない。clang-clは0。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_OS_WINDOWS
 * @brief Windows OS判定。
 * @retval 1 `_WIN32`定義あり。
 * @retval 0 `_WIN32`定義なし。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_OS_LINUX
 * @brief Linux OS判定。
 * @retval 1 `__linux__`定義あり。
 * @retval 0 `__linux__`定義なし。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

/**
 * @def KET_OS_MACOS
 * @brief 現在のmacOS判定。
 * @retval 1 `__APPLE__`と`__MACH__`の定義あり。
 * @retval 0 現在のmacOSではない。Classic Mac OSは対象外。
 * @pre C++11以降のtranslation unit。
 * @post compile-time定数のみを提供し、runtime状態を変更しない。
 */

// -----------------------------------------------------------------------------
// Internal implementation details
// -----------------------------------------------------------------------------

#if defined(_MSVC_LANG)
#define KET_DETAIL_CXX_VERSION_VALUE _MSVC_LANG
#elif defined(__cplusplus)
#define KET_DETAIL_CXX_VERSION_VALUE __cplusplus
#else
#define KET_DETAIL_CXX_VERSION_VALUE 201103L
#endif

#if KET_DETAIL_CXX_VERSION_VALUE >= 202302L
#define KET_CXX_VERSION 202302L
#elif KET_DETAIL_CXX_VERSION_VALUE >= 202002L
#define KET_CXX_VERSION 202002L
#elif KET_DETAIL_CXX_VERSION_VALUE >= 201703L
#define KET_CXX_VERSION 201703L
#elif KET_DETAIL_CXX_VERSION_VALUE >= 201402L
#define KET_CXX_VERSION 201402L
#else
#define KET_CXX_VERSION 201103L
#endif

#define KET_CXX_AT_LEAST(value) (KET_CXX_VERSION >= (value))

#if defined(__has_include)
#define KET_DETAIL_HAS_INCLUDE_OPERATOR 1
#else
#define KET_DETAIL_HAS_INCLUDE_OPERATOR 0
#endif

#if KET_DETAIL_HAS_INCLUDE_OPERATOR
#if __has_include(<version>)
#include <version>
#endif
#endif

#if KET_CXX_AT_LEAST(201703L) && KET_DETAIL_HAS_INCLUDE_OPERATOR
#if __has_include(<optional>)
#define KET_DETAIL_HAS_STD_OPTIONAL_HEADER 1
#else
#define KET_DETAIL_HAS_STD_OPTIONAL_HEADER 0
#endif
#else
#define KET_DETAIL_HAS_STD_OPTIONAL_HEADER 0
#endif

#if KET_CXX_AT_LEAST(201703L) && KET_DETAIL_HAS_STD_OPTIONAL_HEADER && !defined(__cpp_lib_optional)
#include <optional>
#endif

#if KET_CXX_AT_LEAST(201703L) && KET_DETAIL_HAS_INCLUDE_OPERATOR
#if __has_include(<string_view>)
#define KET_DETAIL_HAS_STD_STRING_VIEW_HEADER 1
#else
#define KET_DETAIL_HAS_STD_STRING_VIEW_HEADER 0
#endif
#else
#define KET_DETAIL_HAS_STD_STRING_VIEW_HEADER 0
#endif

#if KET_CXX_AT_LEAST(201703L) && KET_DETAIL_HAS_STD_STRING_VIEW_HEADER && \
	!defined(__cpp_lib_string_view)
#include <string_view>
#endif

#if KET_CXX_AT_LEAST(202002L) && KET_DETAIL_HAS_INCLUDE_OPERATOR
#if __has_include(<span>)
#define KET_DETAIL_HAS_STD_SPAN_HEADER 1
#else
#define KET_DETAIL_HAS_STD_SPAN_HEADER 0
#endif
#else
#define KET_DETAIL_HAS_STD_SPAN_HEADER 0
#endif

#if KET_CXX_AT_LEAST(202002L) && KET_DETAIL_HAS_STD_SPAN_HEADER && !defined(__cpp_lib_span)
#include <span>
#endif

#if KET_CXX_AT_LEAST(202002L) && KET_DETAIL_HAS_INCLUDE_OPERATOR
#if __has_include(<format>)
#define KET_DETAIL_HAS_STD_FORMAT_HEADER 1
#else
#define KET_DETAIL_HAS_STD_FORMAT_HEADER 0
#endif
#else
#define KET_DETAIL_HAS_STD_FORMAT_HEADER 0
#endif

#if KET_CXX_AT_LEAST(202002L) && KET_DETAIL_HAS_STD_FORMAT_HEADER && !defined(__cpp_lib_format)
#include <format>
#endif

namespace ket
{
	namespace detail
	{

	} // namespace detail

} // namespace ket

// -----------------------------------------------------------------------------
// Public API definitions
// -----------------------------------------------------------------------------

#if KET_CXX_AT_LEAST(201703L) && \
	((KET_DETAIL_HAS_INCLUDE_OPERATOR && KET_DETAIL_HAS_STD_OPTIONAL_HEADER) || \
	 (!KET_DETAIL_HAS_INCLUDE_OPERATOR)) && \
	defined(__cpp_lib_optional)
#define KET_HAS_STD_OPTIONAL 1
#else
#define KET_HAS_STD_OPTIONAL 0
#endif

#if KET_CXX_AT_LEAST(201703L) && \
	((KET_DETAIL_HAS_INCLUDE_OPERATOR && KET_DETAIL_HAS_STD_STRING_VIEW_HEADER) || \
	 (!KET_DETAIL_HAS_INCLUDE_OPERATOR)) && \
	defined(__cpp_lib_string_view)
#define KET_HAS_STD_STRING_VIEW 1
#else
#define KET_HAS_STD_STRING_VIEW 0
#endif

#if KET_CXX_AT_LEAST(202002L) && \
	((KET_DETAIL_HAS_INCLUDE_OPERATOR && KET_DETAIL_HAS_STD_SPAN_HEADER) || \
	 (!KET_DETAIL_HAS_INCLUDE_OPERATOR)) && \
	defined(__cpp_lib_span)
#define KET_HAS_STD_SPAN 1
#else
#define KET_HAS_STD_SPAN 0
#endif

#if KET_CXX_AT_LEAST(202002L) && \
	((KET_DETAIL_HAS_INCLUDE_OPERATOR && KET_DETAIL_HAS_STD_FORMAT_HEADER) || \
	 (!KET_DETAIL_HAS_INCLUDE_OPERATOR)) && \
	defined(__cpp_lib_format)
#define KET_HAS_STD_FORMAT 1
#else
#define KET_HAS_STD_FORMAT 0
#endif

#if defined(__clang__)
#define KET_COMPILER_CLANG 1
#else
#define KET_COMPILER_CLANG 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define KET_COMPILER_GCC 1
#else
#define KET_COMPILER_GCC 0
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define KET_COMPILER_MSVC 1
#else
#define KET_COMPILER_MSVC 0
#endif

#if defined(_WIN32)
#define KET_OS_WINDOWS 1
#else
#define KET_OS_WINDOWS 0
#endif

#if defined(__linux__)
#define KET_OS_LINUX 1
#else
#define KET_OS_LINUX 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define KET_OS_MACOS 1
#else
#define KET_OS_MACOS 0
#endif

#undef KET_DETAIL_CXX_VERSION_VALUE
#undef KET_DETAIL_HAS_INCLUDE_OPERATOR
#undef KET_DETAIL_HAS_STD_OPTIONAL_HEADER
#undef KET_DETAIL_HAS_STD_STRING_VIEW_HEADER
#undef KET_DETAIL_HAS_STD_SPAN_HEADER
#undef KET_DETAIL_HAS_STD_FORMAT_HEADER
