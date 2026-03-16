#include "cpp-registry/Registry.h"


namespace kare {
	static RegistryKey& parseRootKey(std::string_view& p_strPath) {
		size_t nPos = p_strPath.find_first_of('\\');
		if (nPos == std::string_view::npos) {
			throw InvalidArgumentException("p_strPath");
		}

		std::string_view strRoot = p_strPath.substr(0, nPos);
		p_strPath.remove_prefix(nPos + 1);

		if (strRoot == "HKCR" || strRoot == "HKEY_CLASSES_ROOT") {
			return Registry::classesRoot;
		}
		if (strRoot == "HKCU" || strRoot == "HKEY_CURRENT_USER") {
			return Registry::currentUser;
		}
		if (strRoot == "HKLM" || strRoot == "HKEY_LOCAL_MACHINE") {
			return Registry::localMachine;
		}
		if (strRoot == "HKU" || strRoot == "HKEY_USERS") {
			return Registry::users;
		}
		if (strRoot == "HKPD" || strRoot == "HKEY_PERFORMANCE_DATA") {
			return Registry::performanceData;
		}

		throw InvalidArgumentException("p_strPath");
	}

	bool Registry::deleteKey(std::string_view p_strPath) {
		RegistryKey& rootKey = parseRootKey(p_strPath);
		return rootKey.deleteSubKey(p_strPath);
	}

	// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
	bool Registry::deleteValue(std::string_view p_strPath, std::string_view p_strValueName, int p_rights) {
		RegistryKey& rootKey = parseRootKey(p_strPath);
		p_rights |= RegistryKey::AccessRights::Write;
		RegistryKey subKey = rootKey.openSubKey(p_strPath, p_rights);
		// if the key doesn't exist, then the value doesn't exist either => return false
		if (subKey.empty()) {
			return false;
		}

		return subKey.deleteValue(p_strValueName);
	}

	bool Registry::exists(std::string_view p_strPath) {
		RegistryKey& rootKey = parseRootKey(p_strPath);
		RegistryKey  subKey  = rootKey.openSubKey(p_strPath);
		return !subKey.empty();
	}

	// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
	RegistryValue Registry::getValue(std::string_view p_strPath, std::string_view p_strValueName) {
		RegistryKey& rootKey = parseRootKey(p_strPath);
		RegistryKey  subKey  = rootKey.openSubKey(p_strPath);
		if (subKey.empty()) {
			return {};
		}

		return subKey.getValue(p_strValueName);
	}
}