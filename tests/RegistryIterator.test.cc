#include "common.h"

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include <cpp-registry/Registry.h>
#include <cpp-registry/RegistryIterator.h>

using namespace kare;

class RegistryIteratorTest : public ::testing::Test {
protected:
	// NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
	std::string m_relPath_;
	std::string m_absPath_;
	// NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

	void SetUp() override {
		const auto* info = testing::UnitTest::GetInstance()->current_test_info();
		m_relPath_       = makeRelPath(info->test_suite_name(), info->name());
		m_absPath_       = "HKCU\\" + m_relPath_;
		deleteTreeSilent(HKEY_CURRENT_USER, m_relPath_);
	}

	void TearDown() override {
		deleteTreeSilent(HKEY_CURRENT_USER, m_relPath_);
	}

	RegistryKey testRoot_() const {
		return Registry::currentUser.createSubKey(m_relPath_);
	}
};

TEST_F(RegistryIteratorTest, SubKeysRangeIteratesAllSubKeys) {
	RegistryKey root = testRoot_();
	root.createSubKey("Gamma");
	root.createSubKey("Alpha");
	root.createSubKey("Beta");

	std::vector<std::string> names;
	for (const auto& entry : root.subKeys()) {
		names.emplace_back(entry.getName());
	}

	std::sort(names.begin(), names.end());
	ASSERT_EQ(names.size(), 3u);
	EXPECT_EQ(names[0], "Alpha");
	EXPECT_EQ(names[1], "Beta");
	EXPECT_EQ(names[2], "Gamma");
}

TEST_F(RegistryIteratorTest, ValuesRangeIteratesAllValues) {
	HKEY raw = createWinKey(HKEY_CURRENT_USER, m_relPath_);
	setSz(raw, "Name", "registry");
	setDword(raw, "Count", 3);
	::RegCloseKey(raw);

	RegistryKey              root = testRoot_();
	std::vector<std::string> names;
	for (auto&& entry : root.values()) {
		names.emplace_back(entry.name);
	}

	std::sort(names.begin(), names.end());
	ASSERT_EQ(names.size(), 2u);
	EXPECT_EQ(names[0], "Count");
	EXPECT_EQ(names[1], "Name");
}

TEST_F(RegistryIteratorTest, ValuesRangeReturnsTypedValues) {
	constexpr DWORD expectedCount = 7;

	HKEY raw = createWinKey(HKEY_CURRENT_USER, m_relPath_);
	setSz(raw, "Name", "registry");
	setDword(raw, "Count", expectedCount);
	::RegCloseKey(raw);

	RegistryKey root       = testRoot_();
	bool        foundName  = false;
	bool        foundCount = false;
	for (const auto& entry : root.values()) {
		if (entry.name == "Name") {
			foundName = true;
			EXPECT_EQ(entry.value.type(), RegistryValue::Type::String);
			EXPECT_EQ(entry.value.stringValue(), "registry");
		}
		if (entry.name == "Count") {
			foundCount = true;
			EXPECT_EQ(entry.value.type(), RegistryValue::Type::DWord);
			EXPECT_EQ(entry.value.dWordValue(), static_cast<int32_t>(expectedCount));
		}
	}

	EXPECT_TRUE(foundName);
	EXPECT_TRUE(foundCount);
}

TEST_F(RegistryIteratorTest, AllIteratorVisitsKeysThenValues) {
	constexpr DWORD expectedCount = 11;

	RegistryKey root = testRoot_();
	root.createSubKey("ChildA");
	root.createSubKey("ChildB");

	HKEY raw = createWinKey(HKEY_CURRENT_USER, m_relPath_);
	setSz(raw, "Name", "registry");
	setDword(raw, "Count", expectedCount);
	::RegCloseKey(raw);

	std::vector<std::string> keys;
	std::vector<std::string> values;
	for (auto it = root.begin(); it != root.end(); ++it) {
		auto& entry = *it;
		if (entry.isKey()) {
			keys.emplace_back(entry.key().getName());
		} else {
			values.emplace_back(entry.value().name);
		}
	}

	std::sort(keys.begin(), keys.end());
	std::sort(values.begin(), values.end());
	ASSERT_EQ(keys.size(), 2u);
	ASSERT_EQ(values.size(), 2u);
	EXPECT_EQ(keys[0], "ChildA");
	EXPECT_EQ(keys[1], "ChildB");
	EXPECT_EQ(values[0], "Count");
	EXPECT_EQ(values[1], "Name");
}

TEST_F(RegistryIteratorTest, EmptyKeyAllIteratorBeginEqualsEnd) {
	RegistryKey root = testRoot_();
	EXPECT_EQ(root.begin(), root.end());
}

TEST_F(RegistryIteratorTest, EmptyKeyRangeIteratorsAreEmpty) {
	RegistryKey root = testRoot_();
	EXPECT_EQ(root.subKeys().begin(), root.subKeys().end());
	EXPECT_EQ(root.values().begin(), root.values().end());
}

TEST_F(RegistryIteratorTest, PostfixIncrementTraversesWithoutDuplication) {
	RegistryKey root = testRoot_();
	root.createSubKey("A");
	root.createSubKey("B");

	size_t visited = 0;
	for (auto it = root.subKeys().begin(); it != root.subKeys().end(); it++) {
		visited++;
	}
	EXPECT_EQ(visited, 2u);
}
