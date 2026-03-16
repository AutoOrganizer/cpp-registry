#pragma once
#include <cpp-registry/pch.h>
#include <format>

// ============================================================
//  Internal test helpers
// ============================================================
namespace {
	constexpr const char* gc_testRoot = "Software\\cpp-registry-tests";

	std::string makeRelPath(const char* p_suiteName, const char* p_testName) {
		return std::format("{}\\{}-{}", gc_testRoot, p_suiteName, p_testName);
	}

	void deleteTreeSilent(HKEY p_root, const std::string& p_path) noexcept {
		::RegDeleteTreeA(p_root, p_path.c_str());
	}

	HKEY createWinKey(HKEY p_root, const std::string& p_path, REGSAM p_access = KEY_READ | KEY_WRITE) {
		HKEY    hKey = nullptr;
		LSTATUS lRes = ::RegCreateKeyExA(p_root, p_path.c_str(), 0, nullptr, 0, p_access, nullptr, &hKey, nullptr);
		if (lRes != ERROR_SUCCESS) {
			throw std::runtime_error(std::format("RegCreateKeyExA failed: {}", lRes));
		}
		return hKey;
	}

	void setSz(HKEY p_hKey, const char* p_name, const char* p_value) {
		const DWORD cbValue = static_cast<DWORD>(::strlen(p_value)) + 1u;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		::RegSetValueExA(p_hKey, p_name, 0, REG_SZ, reinterpret_cast<const BYTE*>(p_value), cbValue);
	}

	void setDword(HKEY p_hKey, const char* p_name, DWORD p_value) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		::RegSetValueExA(p_hKey, p_name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&p_value), sizeof(DWORD));
	}

	void setQword(HKEY p_hKey, const char* p_name, UINT64 p_value) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		::RegSetValueExA(p_hKey, p_name, 0, REG_QWORD, reinterpret_cast<const BYTE*>(&p_value), sizeof(UINT64));
	}

} // namespace
