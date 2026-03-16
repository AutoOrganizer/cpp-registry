#include "common.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <cpp-registry/Registry.h>

using namespace kare;

// ============================================================
//  Fixture
// ============================================================
class RegistryKeyTest : public ::testing::Test {
protected:
	// NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
	// relative to HKEY_CURRENT_USER,  e.g. "Software\cpp-registry-tests\Suite-Test"
	std::string m_relPath_;
	// full path understood by Registry::, e.g. "HKCU\Software\..."
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

	// Opens an RAII RegistryKey for the current test key (creates if needed)
	RegistryKey testRoot_(int p_rights = RegistryKey::Read | RegistryKey::Write) const {
		return Registry::currentUser.createSubKey(m_relPath_, p_rights);
	}

	// Returns a raw Win32 handle. Caller is responsible for ::RegCloseKey.
	HKEY rawRoot_() const {
		return createWinKey(HKEY_CURRENT_USER, m_relPath_);
	}
};

// ============================================================
//  RegistryKey default construction / empty state
// ============================================================

TEST_F(RegistryKeyTest, DefaultConstructedIsEmpty) {
	RegistryKey key;
	EXPECT_TRUE(key.empty());
	EXPECT_EQ(key.nativeHandle(), nullptr);
}

TEST_F(RegistryKeyTest, NonEmptyKeyIsNotEmpty) {
	RegistryKey key = testRoot_();
	EXPECT_FALSE(key.empty());
	EXPECT_NE(key.nativeHandle(), nullptr);
}

// ============================================================
//  RegistryKey path / name
// ============================================================

TEST_F(RegistryKeyTest, RootKeyPaths) {
	EXPECT_EQ(Registry::currentUser.getPath(), "HKCU");
	EXPECT_EQ(Registry::localMachine.getPath(), "HKLM");
	EXPECT_EQ(Registry::classesRoot.getPath(), "HKCR");
	EXPECT_EQ(Registry::users.getPath(), "HKU");
}

TEST_F(RegistryKeyTest, GetPathReturnsFullAbsolutePath) {
	RegistryKey key = testRoot_();
	EXPECT_EQ(key.getPath(), m_absPath_);
}

TEST_F(RegistryKeyTest, GetNameReturnsLastSegment) {
	RegistryKey key      = testRoot_();
	std::string expected = m_relPath_.substr(m_relPath_.rfind('\\') + 1);
	EXPECT_EQ(key.getName(), expected);
}

TEST_F(RegistryKeyTest, RootKeyNameIsItselfWithoutBackslash) {
	EXPECT_EQ(Registry::currentUser.getName(), "HKCU");
}

TEST_F(RegistryKeyTest, OpenSubKeyPathRecorded) {
	RegistryKey root = testRoot_();
	HKEY        sub  = nullptr;
	::RegCreateKeyExA(root.nativeHandle(), "Child", 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &sub, nullptr);
	::RegCloseKey(sub);

	RegistryKey child = root.openSubKey("Child");
	EXPECT_EQ(child.getPath(), m_absPath_ + "\\Child");
	EXPECT_EQ(child.getName(), std::string_view("Child"));
}

// ============================================================
//  RegistryKey openSubKey
// ============================================================

TEST_F(RegistryKeyTest, OpenSubKeyExistingReturnsNonEmpty) {
	HKEY raw = rawRoot_();
	::RegCloseKey(raw);

	RegistryKey key = Registry::currentUser.openSubKey(m_relPath_);
	EXPECT_FALSE(key.empty());
	EXPECT_EQ(key.getPath(), m_absPath_);
}

TEST_F(RegistryKeyTest, OpenSubKeyNonExistingReturnsEmpty) {
	RegistryKey key = Registry::currentUser.openSubKey(m_relPath_ + "\\DoesNotExist");
	EXPECT_TRUE(key.empty());
}

TEST_F(RegistryKeyTest, OpenSubKeyOnEmptyKeyThrows) {
	RegistryKey empty;
	EXPECT_THROW(empty.openSubKey("Anything"), NullPointerReferenceException);
}

TEST_F(RegistryKeyTest, OpenSubKeyEmptyNameThrows) {
	EXPECT_THROW(Registry::currentUser.openSubKey(""), InvalidArgumentException);
}

// ============================================================
//  RegistryKey createSubKey
// ============================================================

TEST_F(RegistryKeyTest, CreateSubKeyCreatesAndReturnsNonEmpty) {
	RegistryKey key = Registry::currentUser.createSubKey(m_relPath_);
	EXPECT_FALSE(key.empty());
	EXPECT_EQ(key.getPath(), m_absPath_);
}

TEST_F(RegistryKeyTest, CreateSubKeyExistingOpensIt) {
	Registry::currentUser.createSubKey(m_relPath_);
	RegistryKey second = Registry::currentUser.createSubKey(m_relPath_);
	EXPECT_FALSE(second.empty());
	EXPECT_EQ(second.getPath(), m_absPath_);
}

TEST_F(RegistryKeyTest, CreateSubKeyNestedPath) {
	RegistryKey root  = testRoot_();
	RegistryKey child = root.createSubKey("Level1\\Level2");
	EXPECT_FALSE(child.empty());
	EXPECT_EQ(child.getPath(), m_absPath_ + "\\Level1\\Level2");
}

// ============================================================
//  RegistryKey deleteSubKey
// ============================================================

TEST_F(RegistryKeyTest, DeleteSubKeyExistingReturnsTrue) {
	RegistryKey root = testRoot_();
	root.createSubKey("ToDelete");
	EXPECT_TRUE(root.deleteSubKey("ToDelete"));
}

TEST_F(RegistryKeyTest, DeleteSubKeyNotExistingReturnsFalse) {
	RegistryKey root = testRoot_();
	EXPECT_FALSE(root.deleteSubKey("NeverCreated"));
}

TEST_F(RegistryKeyTest, DeleteSubKeyWithChildrenSucceeds) {
	RegistryKey root  = testRoot_();
	RegistryKey child = root.createSubKey("Parent");
	child.createSubKey("Child");

	EXPECT_TRUE(root.deleteSubKey("Parent"));
	EXPECT_TRUE(root.openSubKey("Parent").empty());
}

TEST_F(RegistryKeyTest, DeleteSubKeyOnEmptyKeyThrows) {
	RegistryKey empty;
	EXPECT_THROW(empty.deleteSubKey("Anything"), NullPointerReferenceException);
}

// ============================================================
//  RegistryKey getValue
// ============================================================

TEST_F(RegistryKeyTest, GetValueStringType) {
	HKEY raw = rawRoot_();
	setSz(raw, "Name", "hello");
	::RegCloseKey(raw);

	RegistryKey   key = testRoot_();
	RegistryValue val = key.getValue("Name");
	EXPECT_EQ(val.type(), RegistryValue::Type::String);
	EXPECT_EQ(val.stringValue(), "hello");
}

TEST_F(RegistryKeyTest, GetValueDWordType) {
	constexpr DWORD kExpected = 42;
	HKEY            raw       = rawRoot_();
	setDword(raw, "Count", kExpected);
	::RegCloseKey(raw);

	RegistryKey   key = testRoot_();
	RegistryValue val = key.getValue("Count");
	EXPECT_EQ(val.type(), RegistryValue::Type::DWord);
	EXPECT_EQ(val.dWordValue(), static_cast<int32_t>(kExpected));
	EXPECT_EQ(val.qWordValue(), static_cast<int64_t>(kExpected)); // dword promotes to qword
	EXPECT_TRUE(val.boolValue());
}

TEST_F(RegistryKeyTest, GetValueQWordType) {
	constexpr UINT64 kBig = 0xDEADBEEFCAFEBABEULL;
	HKEY             raw  = rawRoot_();
	setQword(raw, "Big", kBig);
	::RegCloseKey(raw);

	RegistryKey   key = testRoot_();
	RegistryValue val = key.getValue("Big");
	EXPECT_EQ(val.type(), RegistryValue::Type::QWord);
	EXPECT_EQ(static_cast<UINT64>(val.qWordValue()), kBig);
}

TEST_F(RegistryKeyTest, GetValueNotFoundReturnsEmpty) {
	RegistryKey   key = testRoot_();
	RegistryValue val = key.getValue("NoSuchValue");
	EXPECT_TRUE(val.empty());
	EXPECT_EQ(val.type(), RegistryValue::Type::None);
}

TEST_F(RegistryKeyTest, GetValueOnEmptyKeyThrows) {
	RegistryKey empty;
	EXPECT_THROW(empty.getValue("Anything"), NullPointerReferenceException);
}

TEST_F(RegistryKeyTest, GetValueEmptyNameReturnsDefaultValue) {
	constexpr DWORD expected = 1234;

	HKEY raw = rawRoot_();
	// Empty value name maps to the key's default value on Windows.
	::RegSetValueExA(raw, "", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&expected), sizeof(expected)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	::RegCloseKey(raw);

	RegistryKey   key   = testRoot_();
	RegistryValue value = key.getValue("");
	EXPECT_EQ(value.type(), RegistryValue::Type::DWord);
	EXPECT_EQ(value.dWordValue(), static_cast<int32_t>(expected));
}

TEST_F(RegistryKeyTest, GetValueEmptyNameMissingDefaultReturnsEmpty) {
	RegistryKey   key   = testRoot_();
	RegistryValue value = key.getValue("");
	EXPECT_TRUE(value.empty());
	EXPECT_EQ(value.type(), RegistryValue::Type::None);
}

// ============================================================
//  RegistryKey deleteValue
// ============================================================

TEST_F(RegistryKeyTest, DeleteValueExistingReturnsTrue) {
	HKEY raw = rawRoot_();
	setDword(raw, "ToDelete", 1);
	::RegCloseKey(raw);

	RegistryKey key = testRoot_();
	EXPECT_TRUE(key.deleteValue("ToDelete"));
}

TEST_F(RegistryKeyTest, DeleteValueNotExistingReturnsFalse) {
	RegistryKey key = testRoot_();
	EXPECT_FALSE(key.deleteValue("NeverSet"));
}

TEST_F(RegistryKeyTest, DeleteValueOnEmptyKeyThrows) {
	RegistryKey empty;
	EXPECT_THROW(empty.deleteValue("Anything"), NullPointerReferenceException);
}

TEST_F(RegistryKeyTest, DeleteValueTwiceReturnsFalseSecondTime) {
	HKEY raw = rawRoot_();
	setDword(raw, "Once", 1);
	::RegCloseKey(raw);

	RegistryKey key = testRoot_();
	EXPECT_TRUE(key.deleteValue("Once"));
	EXPECT_FALSE(key.deleteValue("Once"));
}

// ============================================================
//  RegistryKey setValue / operator[]
// ============================================================

TEST_F(RegistryKeyTest, SetValueWritesDword) {
	RegistryKey key = testRoot_();

	EXPECT_TRUE(key.setValue("Count", RegistryValue(55)));
	EXPECT_EQ(key.getValue("Count").dWordValue(), 55);
}

TEST_F(RegistryKeyTest, SetValueWritesDouble) {
	constexpr double expected = 6.283185307179586;
	RegistryKey      key      = testRoot_();

	EXPECT_TRUE(key.setValue("Pi2", RegistryValue(expected)));
	RegistryValue value = key.getValue("Pi2");
	EXPECT_EQ(value.type(), RegistryValue::Type::QWord);
	EXPECT_DOUBLE_EQ(value.doubleValue(), expected);
}

TEST_F(RegistryKeyTest, SetValueOverwritesExistingValue) {
	RegistryKey key = testRoot_();

	EXPECT_TRUE(key.setValue("Count", RegistryValue(1)));
	EXPECT_TRUE(key.setValue("Count", RegistryValue(9)));
	EXPECT_EQ(key.getValue("Count").dWordValue(), 9);
}

TEST_F(RegistryKeyTest, SetValueWritesString) {
	constexpr size_t textLength = 8;

	RegistryKey key = testRoot_();

	EXPECT_TRUE(key.setValue("Name", "registry"));
	EXPECT_EQ(key.getValue("Name").stringValue(), "registry");
}

TEST_F(RegistryKeyTest, SetValueEmptyNameWritesDefaultValue) {
	constexpr DWORD expected = 777;
	RegistryKey     key      = testRoot_();

	EXPECT_TRUE(key.setValue("", RegistryValue(expected)));

	HKEY       raw  = rawRoot_();
	DWORD      type = 0;
	DWORD      data = 0;
	DWORD      size = sizeof(data);
	const LONG lRes = ::RegQueryValueExA(raw, "", nullptr, &type, reinterpret_cast<BYTE*>(&data), &size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	::RegCloseKey(raw);

	ASSERT_EQ(lRes, ERROR_SUCCESS);
	EXPECT_EQ(type, REG_DWORD);
	EXPECT_EQ(data, expected);
}

TEST_F(RegistryKeyTest, SetValueWritesEmptyStringWithTerminator) {
	RegistryKey key = testRoot_();

	EXPECT_TRUE(key.setValue("EmptyName", ""));
	auto value = key.getValue("EmptyName").stringValue();
	EXPECT_EQ(value.size(), 0u);
	EXPECT_EQ(value, "");
}

TEST_F(RegistryKeyTest, SetValuePreservesEmbeddedNullInStringPayload) {
	RegistryKey key = testRoot_();

	EXPECT_TRUE(key.setValue("Payload", std::string_view { "x\0y", 3 }));
	auto value = key.getValue("Payload").stringValue();

	ASSERT_EQ(value.size(), 3u);
	EXPECT_EQ(value[0], 'x');
	EXPECT_EQ(value[1], '\0');
	EXPECT_EQ(value[2], 'y');
	EXPECT_EQ(value[3], '\0');
}

TEST_F(RegistryKeyTest, SetValueOnEmptyKeyThrows) {
	RegistryKey empty;
	EXPECT_THROW(empty.setValue("Any", RegistryValue(static_cast<DWORD>(1))), NullPointerReferenceException);
}

TEST_F(RegistryKeyTest, BracketOperatorAssignmentWritesValue) {
	RegistryKey key = testRoot_();

	key["Enabled"] = RegistryValue(static_cast<DWORD>(1));
	EXPECT_EQ(key.getValue("Enabled").dWordValue(), 1);
}

TEST_F(RegistryKeyTest, BracketOperatorAssignmentWithEmptyNameWritesDefaultValue) {
	constexpr DWORD expected = 2048;
	RegistryKey     key      = testRoot_();

	key[""] = RegistryValue(expected);

	HKEY       raw  = rawRoot_();
	DWORD      type = 0;
	DWORD      data = 0;
	DWORD      size = sizeof(data);
	const LONG lRes = ::RegQueryValueExA(raw, "", nullptr, &type, reinterpret_cast<BYTE*>(&data), &size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	::RegCloseKey(raw);

	ASSERT_EQ(lRes, ERROR_SUCCESS);
	EXPECT_EQ(type, REG_DWORD);
	EXPECT_EQ(data, expected);
}

TEST_F(RegistryKeyTest, BracketOperatorCanReadValue) {
	constexpr DWORD expectedPort = 8080;

	RegistryKey key = testRoot_();
	key.setValue("Port", RegistryValue(expectedPort));

	RegistryValue value = key["Port"];
	EXPECT_EQ(value.dWordValue(), static_cast<int32_t>(expectedPort));
}

TEST_F(RegistryKeyTest, BracketOperatorReadWithEmptyNameReadsDefaultValue) {
	constexpr DWORD expected = 4096;

	HKEY raw = rawRoot_();
	::RegSetValueExA(raw, "", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&expected), sizeof(expected)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	::RegCloseKey(raw);

	RegistryKey   key   = testRoot_();
	RegistryValue value = key[""];
	EXPECT_EQ(value.type(), RegistryValue::Type::DWord);
	EXPECT_EQ(value.dWordValue(), static_cast<int32_t>(expected));
}

TEST_F(RegistryKeyTest, BracketOperatorAssignmentWritesDouble) {
	constexpr double expected = 2.718281828459045;
	RegistryKey      key      = testRoot_();

	key["Euler"]        = RegistryValue(expected);
	RegistryValue value = key["Euler"];
	EXPECT_EQ(value.type(), RegistryValue::Type::QWord);
	EXPECT_DOUBLE_EQ(value.doubleValue(), expected);
}

TEST_F(RegistryKeyTest, BracketOperatorAssignmentWritesStringWithTerminator) {
	RegistryKey key = testRoot_();

	key["Status"] = "ok";
	auto value    = static_cast<RegistryValue>(key["Status"]).stringValue();

	EXPECT_EQ(value.size(), 2u);
	EXPECT_EQ(value, "ok");
}

TEST_F(RegistryKeyTest, ConstBracketOperatorCanReadValue) {
	RegistryKey key = testRoot_();
	key.setValue("Version", RegistryValue(static_cast<DWORD>(3)));

	const RegistryKey& ckey  = key;
	RegistryValue      value = ckey["Version"];
	EXPECT_EQ(value.dWordValue(), 3);
}

TEST_F(RegistryKeyTest, BracketOperatorMissingValueReturnsEmpty) {
	RegistryKey key = testRoot_();

	RegistryValue value = key["NoSuchValue"];
	EXPECT_TRUE(value.empty());
	EXPECT_EQ(value.type(), RegistryValue::Type::None);
}

TEST_F(RegistryKeyTest, BracketOperatorAssignmentOnEmptyKeyThrows) {
	RegistryKey empty;
	EXPECT_THROW((empty["Any"] = RegistryValue(static_cast<DWORD>(1))), NullPointerReferenceException);
}

TEST_F(RegistryKeyTest, BracketOperatorReadOnEmptyKeyThrows) {
	RegistryKey empty;
	EXPECT_THROW(
	    {
		    RegistryValue value = empty["Any"];
		    (void)value;
	    },
	    NullPointerReferenceException
	);
}

// ============================================================
//  RegistryKey subKeyCount / valueCount / subKeyNames / valueNames
// ============================================================

TEST_F(RegistryKeyTest, SubKeyCount) {
	RegistryKey root = testRoot_();
	root.createSubKey("A");
	root.createSubKey("B");
	root.createSubKey("C");
	EXPECT_EQ(root.subKeyCount(), 3u);
}

TEST_F(RegistryKeyTest, ValueCount) {
	HKEY raw = rawRoot_();
	setDword(raw, "X", 1);
	setDword(raw, "Y", 2);
	::RegCloseKey(raw);

	RegistryKey key = testRoot_();
	EXPECT_EQ(key.valueCount(), 2u);
}

TEST_F(RegistryKeyTest, SubKeyNames) {
	RegistryKey root = testRoot_();
	root.createSubKey("Alpha");
	root.createSubKey("Beta");

	std::vector<std::string> names = root.subKeyNames();
	std::sort(names.begin(), names.end());
	ASSERT_EQ(names.size(), 2u);
	EXPECT_EQ(names[0], "Alpha");
	EXPECT_EQ(names[1], "Beta");
}

TEST_F(RegistryKeyTest, ValueNames) {
	HKEY raw = rawRoot_();
	setDword(raw, "Foo", 1);
	setSz(raw, "Bar", "hello");
	::RegCloseKey(raw);

	RegistryKey              key   = testRoot_();
	std::vector<std::string> names = key.valueNames();
	std::sort(names.begin(), names.end());
	ASSERT_EQ(names.size(), 2u);
	EXPECT_EQ(names[0], "Bar");
	EXPECT_EQ(names[1], "Foo");
}

TEST_F(RegistryKeyTest, EmptyKeyHasZeroSubKeys) {
	RegistryKey root = testRoot_();
	EXPECT_EQ(root.subKeyCount(), 0u);
	EXPECT_EQ(root.valueCount(), 0u);
}

// ============================================================
//  RegistryKey close
// ============================================================

TEST_F(RegistryKeyTest, CloseBecomesEmpty) {
	RegistryKey key = testRoot_();
	ASSERT_FALSE(key.empty());
	EXPECT_TRUE(key.close());
	EXPECT_TRUE(key.empty());
}

TEST_F(RegistryKeyTest, CloseOnNonOwningCopyReturnsFalse) {
	// Copy constructor marks m_bNeedDestruct_ = false
	RegistryKey original = testRoot_();
	RegistryKey copy     = original; // NOLINT(performance-unnecessary-copy-initialization)
	EXPECT_FALSE(copy.close());      // copy doesn't own the handle
}

// ============================================================
//  RegistryKey move semantics
// ============================================================

TEST_F(RegistryKeyTest, MoveConstructorTransfersOwnership) {
	RegistryKey original  = testRoot_();
	HKEY        rawHandle = original.nativeHandle();

	RegistryKey moved = std::move(original);
	EXPECT_TRUE(original.empty()); // NOLINT(bugprone-use-after-move)
	EXPECT_FALSE(moved.empty());
	EXPECT_EQ(moved.nativeHandle(), rawHandle);
	EXPECT_EQ(moved.getPath(), m_absPath_);
}

TEST_F(RegistryKeyTest, MoveAssignmentTransfersOwnership) {
	RegistryKey src = testRoot_();
	HKEY        raw = src.nativeHandle();
	RegistryKey dest;
	dest = std::move(src);
	EXPECT_TRUE(src.empty()); // NOLINT(bugprone-use-after-move)
	EXPECT_FALSE(dest.empty());
	EXPECT_EQ(dest.nativeHandle(), raw);
}
