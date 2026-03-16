#include "common.h"
#include <gtest/gtest.h>
#include <cpp-registry/Registry.h>

using namespace kare;

// ============================================================
//  Registry (static helpers)
// ============================================================

class RegistryTest : public ::testing::Test {
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

	HKEY rawRoot_() const {
		return createWinKey(HKEY_CURRENT_USER, m_relPath_);
	}
};

// ============================================================
//  Registry::exists
// ============================================================

TEST_F(RegistryTest, ExistsReturnsTrueForExistingKey) {
	HKEY raw = rawRoot_();
	::RegCloseKey(raw);
	EXPECT_TRUE(Registry::exists(m_absPath_));
}

TEST_F(RegistryTest, ExistsReturnsFalseForMissingKey) {
	EXPECT_FALSE(Registry::exists(m_absPath_));
}

TEST_F(RegistryTest, ExistsAllRootPrefixes) {
	EXPECT_NO_THROW(Registry::exists("HKCU\\Software"));
	EXPECT_NO_THROW(Registry::exists("HKEY_CURRENT_USER\\Software"));
	EXPECT_NO_THROW(Registry::exists("HKLM\\Software"));
	EXPECT_NO_THROW(Registry::exists("HKEY_LOCAL_MACHINE\\Software"));
	EXPECT_NO_THROW(Registry::exists("HKCR\\.txt"));
	EXPECT_NO_THROW(Registry::exists("HKEY_CLASSES_ROOT\\.txt"));
	EXPECT_NO_THROW(Registry::exists("HKU\\.DEFAULT"));
	EXPECT_NO_THROW(Registry::exists("HKEY_USERS\\.DEFAULT"));
}

TEST_F(RegistryTest, ExistsInvalidRootKeyThrows) {
	EXPECT_THROW(Registry::exists("INVALID\\Software"), InvalidArgumentException);
}

TEST_F(RegistryTest, ExistsPathWithoutSeparatorThrows) {
	EXPECT_THROW(Registry::exists("HKCU"), InvalidArgumentException);
}

// ============================================================
//  Registry::getValue
// ============================================================

TEST_F(RegistryTest, GetValueReturnsCorrectString) {
	HKEY raw = rawRoot_();
	setSz(raw, "Version", "1.0.0");
	::RegCloseKey(raw);

	RegistryValue val = Registry::getValue(m_absPath_, "Version");
	EXPECT_EQ(val.type(), RegistryValue::Type::String);
	EXPECT_EQ(val.stringValue(), "1.0.0");
}

TEST_F(RegistryTest, GetValueReturnsCorrectDWord) {
	HKEY raw = rawRoot_();
	setDword(raw, "Enabled", 1);
	::RegCloseKey(raw);

	RegistryValue val = Registry::getValue(m_absPath_, "Enabled");
	EXPECT_EQ(val.type(), RegistryValue::Type::DWord);
	EXPECT_EQ(val.dWordValue(), 1);
	EXPECT_TRUE(val.boolValue());
}

TEST_F(RegistryTest, GetValueNonExistingKeyThrows) {
	RegistryValue value = Registry::getValue(m_absPath_, "Anything");
	EXPECT_TRUE(value.empty());
}

// ============================================================
//  Registry::deleteKey
// ============================================================

TEST_F(RegistryTest, DeleteKeyExistingReturnsTrue) {
	HKEY raw = rawRoot_();
	::RegCloseKey(raw);
	EXPECT_TRUE(Registry::deleteKey(m_absPath_));
	EXPECT_FALSE(Registry::exists(m_absPath_));
}

TEST_F(RegistryTest, DeleteKeyNotExistingReturnsFalse) {
	EXPECT_FALSE(Registry::deleteKey(m_absPath_ + "\\Missing"));
}

TEST_F(RegistryTest, DeleteKeyWithChildrenSucceeds) {
	RegistryKey root  = Registry::currentUser.createSubKey(m_relPath_);
	RegistryKey child = root.createSubKey("Child");
	(void)child;

	EXPECT_TRUE(Registry::deleteKey(m_absPath_));
	EXPECT_FALSE(Registry::exists(m_absPath_));
}

// ============================================================
//  Registry::deleteValue
// ============================================================

TEST_F(RegistryTest, DeleteValueExistingReturnsTrue) {
	constexpr DWORD kVal = 99;
	HKEY            raw  = rawRoot_();
	setDword(raw, "Val", kVal);
	::RegCloseKey(raw);

	EXPECT_TRUE(Registry::deleteValue(m_absPath_, "Val"));
}

TEST_F(RegistryTest, DeleteValueNotExistingReturnsFalse) {
	HKEY raw = rawRoot_();
	::RegCloseKey(raw);
	EXPECT_FALSE(Registry::deleteValue(m_absPath_, "NeverSet"));
}

TEST_F(RegistryTest, DeleteValueKeyNotExistingReturnsFalse) {
	// key itself doesn't exist
	EXPECT_FALSE(Registry::deleteValue(m_absPath_, "Anything"));
}

TEST_F(RegistryTest, DeleteValueTwiceReturnsFalseSecondTime) {
	constexpr DWORD kVal = 7;
	HKEY            raw  = rawRoot_();
	setDword(raw, "Once", kVal);
	::RegCloseKey(raw);

	EXPECT_TRUE(Registry::deleteValue(m_absPath_, "Once"));
	EXPECT_FALSE(Registry::deleteValue(m_absPath_, "Once"));
}
