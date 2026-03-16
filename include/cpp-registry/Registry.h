#pragma once
#include <cpp-registry/RegistryKey.h>
#include <cpp-registry/RegistryIterator.h>


namespace kare {
	class Registry {
	public:
		// NOLINTBEGIN
		inline static RegistryKey classesRoot { HKEY_CLASSES_ROOT };
		inline static RegistryKey currentUser { HKEY_CURRENT_USER };
		inline static RegistryKey localMachine { HKEY_LOCAL_MACHINE };
		inline static RegistryKey users { HKEY_USERS };
		inline static RegistryKey performanceData { HKEY_PERFORMANCE_DATA };
		// NOLINTEND

	public:
		/**
		 * @brief Deletes a registry key and all of its subkeys and values.
		 * @param p_strPath The path of the registry key to delete. e.g. "HKCU\\Software\\MyApp"
		 * @date 2026-03-11 15:46
		 */
		static bool deleteKey(std::string_view p_strPath);

		/**
		 * @brief Deletes a registry value from a specified registry key.
		 * @param p_strPath The path of the registry key containing the value to delete. e.g. "HKCU\\Software\\MyApp"
		 * @param p_strValueName The name of the registry value to delete
		 * @date 2026-03-11 15:46
		 */
		static bool deleteValue(std::string_view p_strPath, std::string_view p_strValueName, int p_rights = 0);

		/**
		 * @brief Checks if a registry key exists at the specified path.
		 * @param p_strPath The path of the registry key to check for existence. e.g. "HKCU\\Software\\MyApp"
		 * @date 2026-03-11 15:47
		 */
		static bool exists(std::string_view p_strPath);

		/**
		 * @brief Retrieves the value of a registry entry from the specified path and value name.
		 * @param p_strPath The path of the registry key containing the value to retrieve. e.g. "HKCU\\Software\\MyApp"
		 * @param p_strValueName The name of the registry value to retrieve
		 * @date 2026-03-11 15:47
		 */
		static RegistryValue getValue(std::string_view p_strPath, std::string_view p_strValueName);
	};
}