#include "ket_bytes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

/**
 * @test
 * @brief free functionによる固定幅値追記確認。
 * @details
 * U8、big-endian、little-endianの代表値を既存vectorへ追加し、期待するbyte順になることを確認。
 * @pre C++17以降。
 * @post destinationは期待するbyte列に変化。destination以外の外部状態の変更なし。
 */
TEST(KetBytesTest, AppendsFixedWidthIntegersToVector)
{
	std::vector<std::uint8_t> destination;

	ket::bytes::AppendU8(destination, std::uint8_t{0x12U});
	ket::bytes::AppendBe16(destination, std::uint16_t{0x3456U});
	ket::bytes::AppendBe32(destination, std::uint32_t{0x789ABCDEU});
	ket::bytes::AppendBe64(destination, std::uint64_t{0x0102030405060708ULL});
	ket::bytes::AppendLe16(destination, std::uint16_t{0x1234U});
	ket::bytes::AppendLe32(destination, std::uint32_t{0x56789ABCU});
	ket::bytes::AppendLe64(destination, std::uint64_t{0x1122334455667788ULL});
	ket::bytes::AppendFill(destination, std::uint8_t{0xEEU}, 3U);
	ket::bytes::AppendFill(destination, std::uint8_t{0xDDU}, 0U);

	const auto expected = std::vector<std::uint8_t>{
		std::uint8_t{0x12U}, std::uint8_t{0x34U}, std::uint8_t{0x56U}, std::uint8_t{0x78U},
		std::uint8_t{0x9AU}, std::uint8_t{0xBCU}, std::uint8_t{0xDEU}, std::uint8_t{0x01U},
		std::uint8_t{0x02U}, std::uint8_t{0x03U}, std::uint8_t{0x04U}, std::uint8_t{0x05U},
		std::uint8_t{0x06U}, std::uint8_t{0x07U}, std::uint8_t{0x08U}, std::uint8_t{0x34U},
		std::uint8_t{0x12U}, std::uint8_t{0xBCU}, std::uint8_t{0x9AU}, std::uint8_t{0x78U},
		std::uint8_t{0x56U}, std::uint8_t{0x88U}, std::uint8_t{0x77U}, std::uint8_t{0x66U},
		std::uint8_t{0x55U}, std::uint8_t{0x44U}, std::uint8_t{0x33U}, std::uint8_t{0x22U},
		std::uint8_t{0x11U}, std::uint8_t{0xEEU}, std::uint8_t{0xEEU}, std::uint8_t{0xEEU},
	};

	EXPECT_EQ(destination, expected);
}

/**
 * @test
 * @brief fluent APIによるpayload構築確認。
 * @details
 * builderの各appendが同じbuilder参照を返し、Bufferから構築途中のbyte列を参照できることを確認。
 * @pre C++17以降。
 * @post builderは期待するbyte列を保持。builder以外の外部状態の変更なし。
 */
TEST(KetBytesTest, BuildsPayloadWithFluentApi)
{
	ket::bytes::Builder builder;

	auto& returned = builder.AppendU8(std::uint8_t{0xA5U})
						 .AppendBe16(std::uint16_t{0x1234U})
						 .AppendBe32(std::uint32_t{0x01020304U})
						 .AppendBe64(std::uint64_t{0x1112131415161718ULL})
						 .AppendLe16(std::uint16_t{0x5678U})
						 .AppendLe32(std::uint32_t{0x0A0B0C0DU});
	returned.AppendLe64(std::uint64_t{0x2122232425262728ULL})
		.AppendFill(std::uint8_t{0xEEU}, 2U)
		.AppendFill(std::uint8_t{0xDDU}, 0U);

	const auto same_object = &returned == &builder;
	const auto buffer = builder.Buffer();
	const auto expected = std::vector<std::uint8_t>{
		std::uint8_t{0xA5U}, std::uint8_t{0x12U}, std::uint8_t{0x34U}, std::uint8_t{0x01U},
		std::uint8_t{0x02U}, std::uint8_t{0x03U}, std::uint8_t{0x04U}, std::uint8_t{0x11U},
		std::uint8_t{0x12U}, std::uint8_t{0x13U}, std::uint8_t{0x14U}, std::uint8_t{0x15U},
		std::uint8_t{0x16U}, std::uint8_t{0x17U}, std::uint8_t{0x18U}, std::uint8_t{0x78U},
		std::uint8_t{0x56U}, std::uint8_t{0x0DU}, std::uint8_t{0x0CU}, std::uint8_t{0x0BU},
		std::uint8_t{0x0AU}, std::uint8_t{0x28U}, std::uint8_t{0x27U}, std::uint8_t{0x26U},
		std::uint8_t{0x25U}, std::uint8_t{0x24U}, std::uint8_t{0x23U}, std::uint8_t{0x22U},
		std::uint8_t{0x21U}, std::uint8_t{0xEEU}, std::uint8_t{0xEEU},
	};

	EXPECT_TRUE(same_object);
	EXPECT_EQ(buffer, expected);
}

/**
 * @test
 * @brief 任意byte列と空入力の追記確認。
 * @details raw byte列をfree functionとbuilderで追加し、nullptrかつsize 0はno-opになることを確認。
 * @pre C++17以降。nullかつ非0 sizeは呼び出さない。
 * @post 追記先は期待するbyte列に変化。追記元byte列の変更なし。
 */
TEST(KetBytesTest, AppendsRawDataAndTreatsNullEmptyAsNoOp)
{
	const auto raw = std::array<std::uint8_t, 3>{
		std::uint8_t{0x01U},
		std::uint8_t{0x02U},
		std::uint8_t{0x03U},
	};

	std::vector<std::uint8_t> destination{std::uint8_t{0xF0U}};
	ket::bytes::Append(destination, nullptr, std::size_t{0U});
	ket::bytes::Append(destination, raw.data(), raw.size());

	ket::bytes::Builder builder;
	builder.Append(nullptr, std::size_t{0U}).Append(raw.data(), raw.size());

	const auto expected_destination = std::vector<std::uint8_t>{
		std::uint8_t{0xF0U},
		std::uint8_t{0x01U},
		std::uint8_t{0x02U},
		std::uint8_t{0x03U},
	};
	const auto expected_builder = std::vector<std::uint8_t>{
		std::uint8_t{0x01U},
		std::uint8_t{0x02U},
		std::uint8_t{0x03U},
	};
	const auto builder_buffer = builder.Buffer();

	EXPECT_EQ(destination, expected_destination);
	EXPECT_EQ(builder_buffer, expected_builder);
}

/**
 * @test
 * @brief ASCII文字列片のbyte列追記確認。
 * @details 埋め込みNULを含むASCII文字列片を渡し、encoding変換なしで各byteが追加されることを確認。
 * @pre C++17以降。入力文字列片はASCII byte列。
 * @post builderは期待するbyte列を保持。入力文字列片の変更なし。
 */
TEST(KetBytesTest, AppendsAsciiBytesWithoutEncodingConversion)
{
	const auto storage = std::string("A\0Z", std::size_t{3U});
	const auto text = std::string_view(storage.data(), storage.size());
	ket::bytes::Builder builder;

	builder.AppendAscii(text);

	const auto expected = std::vector<std::uint8_t>{
		std::uint8_t{0x41U},
		std::uint8_t{0x00U},
		std::uint8_t{0x5AU},
	};
	const auto buffer = builder.Buffer();

	EXPECT_EQ(buffer, expected);
}

/**
 * @test
 * @brief reserve constructorとClear確認。
 * @details
 * reserve済みbuilderのcapacityが指定値以上になり、Clear後に空となって再利用できることを確認。
 * @pre C++17以降。
 * @post builderはClear後の再追記結果を保持。builder以外の外部状態の変更なし。
 */
TEST(KetBytesTest, ReservesAndClearsBuilder)
{
	ket::bytes::Builder builder(std::size_t{8U});

	const auto reserved_capacity = builder.Buffer().capacity();
	builder.AppendU8(std::uint8_t{0xAAU}).AppendU8(std::uint8_t{0xBBU});
	builder.Clear();
	const auto buffer_is_empty = builder.Buffer().empty();
	builder.AppendU8(std::uint8_t{0xCCU});

	const auto expected = std::vector<std::uint8_t>{std::uint8_t{0xCCU}};
	const auto buffer = builder.Buffer();

	EXPECT_GE(reserved_capacity, std::size_t{8U});
	EXPECT_TRUE(buffer_is_empty);
	EXPECT_EQ(buffer, expected);
}

/**
 * @test
 * @brief Buildによるpayload move取得確認。
 * @details Buildで構築済みvectorがmove取得され、Build前のbyte列と一致することを確認。
 * @pre C++17以降。
 * @post payloadはBuild前のbyte列を保持。外部状態の変更なし。
 */
TEST(KetBytesTest, BuildMovesPayload)
{
	ket::bytes::Builder builder;
	builder.AppendBe16(std::uint16_t{0xCAFEU}).AppendLe16(std::uint16_t{0x1234U});

	auto payload = std::move(builder).Build();
	const auto expected_payload = std::vector<std::uint8_t>{
		std::uint8_t{0xCAU},
		std::uint8_t{0xFEU},
		std::uint8_t{0x34U},
		std::uint8_t{0x12U},
	};

	EXPECT_EQ(payload, expected_payload);
}
