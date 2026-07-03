#include <gtest/gtest.h>

#include <cpp-registry/pch.h>
#include <cpp-registry/RegistryValue.h>

using namespace kare;

TEST(RegistryValueTest, DefaultValueIsEmptyNoneType) {
	RegistryValue value;
	EXPECT_TRUE(value.empty());
	EXPECT_EQ(value.type(), RegistryValue::Type::None);
	EXPECT_FALSE(static_cast<bool>(value));
}

TEST(RegistryValueTest, DwordConstructorLittleEndian) {
	constexpr DWORD expected = 0x12345678;
	RegistryValue   value(expected);

	EXPECT_EQ(value.type(), RegistryValue::Type::DWord);
	EXPECT_EQ(value.rawType(), REG_DWORD);
	EXPECT_EQ(static_cast<DWORD>(value.dWordValue()), expected);
	EXPECT_TRUE(value.boolValue());
}

TEST(RegistryValueTest, DwordConstructorBigEndian) {
	constexpr DWORD expected = 0x12345678;
	RegistryValue   value(expected, true);

	EXPECT_EQ(value.type(), RegistryValue::Type::DWord);
	EXPECT_EQ(value.rawType(), REG_DWORD_BIG_ENDIAN);
	EXPECT_EQ(static_cast<DWORD>(value.dWordValue()), expected);
}

TEST(RegistryValueTest, QwordConstructorAndAccess) {
	constexpr UINT64 expected = 0x1122334455667788ULL;
	RegistryValue    value(expected);

	EXPECT_EQ(value.type(), RegistryValue::Type::QWord);
	EXPECT_EQ(value.rawType(), REG_QWORD);
	EXPECT_EQ(static_cast<UINT64>(value.qWordValue()), expected);
	EXPECT_TRUE(value.boolValue());
}

TEST(RegistryValueTest, DoubleConstructorStoresAsQwordAndReadsBackAsDouble) {
	constexpr double expected = 3.141592653589793;
	RegistryValue    value(expected);

	EXPECT_EQ(value.type(), RegistryValue::Type::QWord);
	EXPECT_EQ(value.rawType(), REG_QWORD);
	EXPECT_DOUBLE_EQ(value.doubleValue(), expected);
}

TEST(RegistryValueTest, StringConstructorRegSz) {
	RegistryValue    value { "hello" };
	std::string_view textValue = value.stringValue();

	EXPECT_EQ(value.type(), RegistryValue::Type::String);
	EXPECT_EQ(value.rawType(), REG_SZ);
	EXPECT_EQ(textValue.size(), 5u);
	EXPECT_EQ(textValue, "hello");
}

TEST(RegistryValueTest, StringConstructorRegExpandSz) {
	RegistryValue    value { "%USERPROFILE%", true };
	std::string_view textValue = value.stringValue();

	EXPECT_EQ(value.type(), RegistryValue::Type::String);
	EXPECT_EQ(value.rawType(), REG_EXPAND_SZ);
	EXPECT_EQ(textValue, "%USERPROFILE%");
}

TEST(RegistryValueTest, MultiStringConstructorAndAccess) {
	const std::vector<std::string> values { "alpha", "beta", "gamma" };
	RegistryValue                  value(values);

	EXPECT_EQ(value.type(), RegistryValue::Type::StringList);
	EXPECT_EQ(value.rawType(), REG_MULTI_SZ);

	std::vector<std::string_view> multiStrValue = value.multiStringValue();
	ASSERT_EQ(multiStrValue.size(), values.size());
	for (size_t i = 0; i < values.size(); i++) {
		EXPECT_EQ(multiStrValue[i], values[i]);
	}
}

TEST(RegistryValueTest, EmptyStringConstructorStillStoresTerminator) {
	RegistryValue    value { "" };
	std::string_view textValue = value.stringValue();

	EXPECT_EQ(value.type(), RegistryValue::Type::String);
	EXPECT_EQ(textValue.size(), 0u);
	EXPECT_EQ(textValue, "");
}

TEST(RegistryValueTest, StringConstructorPreservesEmbeddedNullAndAppendsTerminator) {
	RegistryValue    value { std::string_view("a\0b", 3) };
	std::string_view textValue = value.stringValue();

	ASSERT_EQ(textValue.size(), 3u);
	EXPECT_EQ(textValue[0], 'a');
	EXPECT_EQ(textValue[1], '\0');
	EXPECT_EQ(textValue[2], 'b');
}

TEST(RegistryValueTest, MultiStringFromRawDoubleTerminatorParsesAsEmptyList) {
	std::vector   raw { static_cast<unsigned char>('\0'), static_cast<unsigned char>('\0') };
	RegistryValue value(std::span<unsigned char>(raw.data(), raw.size()), REG_MULTI_SZ);

	EXPECT_TRUE(value.multiStringValue().empty());
}

TEST(RegistryValueTest, MultiStringFromRawWithEmbeddedEmptySegmentSkipsEmptyItems) {
	std::vector raw {
		static_cast<unsigned char>('a'), static_cast<unsigned char>('\0'), static_cast<unsigned char>('\0'),
		static_cast<unsigned char>('b'), static_cast<unsigned char>('\0'), static_cast<unsigned char>('\0'),
	};
	RegistryValue value(std::span<unsigned char>(raw.data(), raw.size()), REG_MULTI_SZ);

	const std::vector<std::string> expected { "a", "b" };
	std::vector<std::string_view>  actual = value.multiStringValue();
	ASSERT_EQ(actual.size(), expected.size());
	for (size_t i = 0; i < expected.size(); i++) {
		EXPECT_EQ(actual[i], expected[i]);
	}
}

TEST(RegistryValueTest, BinaryConstructorAndAccess) {
	constexpr unsigned char    firstbyte  = static_cast<unsigned char>(0x10);
	constexpr unsigned char    secondbyte = static_cast<unsigned char>(0x20);
	constexpr unsigned char    thirdbyte  = static_cast<unsigned char>(0x30);
	std::vector<unsigned char> buffer { firstbyte, secondbyte, thirdbyte };
	RegistryValue              value(std::span<unsigned char>(buffer.data(), buffer.size()));

	EXPECT_EQ(value.type(), RegistryValue::Type::Binary);
	EXPECT_EQ(value.rawType(), REG_BINARY);
	EXPECT_EQ(value.binaryValue(), buffer);
}

TEST(RegistryValueTest, BinaryConstructorWithCustomType) {
	std::vector      buffer { static_cast<unsigned char>('x'), static_cast<unsigned char>('y') };
	RegistryValue    value(std::span<unsigned char>(buffer.data(), buffer.size()), REG_LINK);
	std::string_view textValue = value.stringValue();

	EXPECT_EQ(value.type(), RegistryValue::Type::String);
	EXPECT_EQ(value.rawType(), REG_LINK);
	EXPECT_EQ(textValue, "xy");
}

TEST(RegistryValueTest, BinaryConstructorRejectsRegNoneType) {
	std::vector<unsigned char> buffer { static_cast<unsigned char>(0x01) };
	EXPECT_THROW((RegistryValue(std::span<unsigned char>(buffer.data(), buffer.size()), REG_NONE)), InvalidArgumentException);
}

TEST(RegistryValueTest, InvalidTypeAccessThrows) {
	constexpr DWORD expected = 123;
	RegistryValue   dwordValue(expected);
	RegistryValue   stringValue { "abc" };

	EXPECT_THROW(dwordValue.stringValue(), InvalidAccessException);
	EXPECT_THROW(stringValue.dWordValue(), InvalidAccessException);
	EXPECT_THROW(stringValue.boolValue(), InvalidAccessException);
}

TEST(RegistryValueTest, DwordAlsoSupportsQwordAndDoubleAccess) {
	constexpr DWORD expected = 42;
	RegistryValue   value(expected);

	EXPECT_EQ(value.qWordValue(), static_cast<int64_t>(expected));
	EXPECT_DOUBLE_EQ(value.doubleValue(), static_cast<double>(expected));
}
