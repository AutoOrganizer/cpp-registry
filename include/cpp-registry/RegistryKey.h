#pragma once
#include <cpp-registry/error.h>
#include <cpp-registry/utils.h>
#include <cpp-registry/RegistryValue.h>


namespace kare {
	enum class IterateFlag : uint8_t { Key, Value, All };


	// forward declaration
	template <IterateFlag>
	class RegistryIterator;

	template <IterateFlag>
	class RegistryConstIterator;

	template <IterateFlag>
	class RegistryEntry;


	template <IterateFlag IterFlag_>
	class RegistryConstRangeIterator {
	private:
		const RegistryKey* m_pKey_;

	public:
		explicit RegistryConstRangeIterator(const RegistryKey* p_pKey)
		    : m_pKey_(p_pKey) {
		}

		RegistryConstIterator<IterFlag_> begin() const;
		RegistryConstIterator<IterFlag_> end() const;
	};

	template <IterateFlag IterFlag_>
	class RegistryRangeIterator {
	private:
		RegistryKey* m_pKey_;

	public:
		explicit RegistryRangeIterator(RegistryKey* p_pKey)
		    : m_pKey_(p_pKey) {
		}

		RegistryIterator<IterFlag_> begin();
		RegistryIterator<IterFlag_> end();
	};


	class RegistryConstValueWrapper {
		friend class RegistryKey;

	protected:
		// NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
		const RegistryKey* m_pKey_;
		std::string        m_strValueName_;
		// NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

	protected:
		RegistryConstValueWrapper(const RegistryKey* p_pKey, std::string_view p_strValueName)
		    : m_pKey_(p_pKey)
		    , m_strValueName_(p_strValueName) {
		}

	public:
		operator RegistryValue() const;
	};

	class RegistryValueWrapper : public RegistryConstValueWrapper {
		friend class RegistryKey;
		using RegistryConstValueWrapper::RegistryConstValueWrapper;

	public:
		RegistryValueWrapper& operator=(const RegistryValue& p_value);

		template <typename... ArgsT_>
		    requires(std::is_constructible_v<RegistryValue, ArgsT_...> && !std::is_same_v<std::decay_t<ArgsT_>..., RegistryValue>)
		RegistryValueWrapper& operator=(ArgsT_&&... p_args) {
			this->operator=(RegistryValue(std::forward<ArgsT_>(p_args)...));
			return *this;
		}
	};


	class RegistryKey {
	public:
		// NOLINTBEGIN(readability-identifier-naming)
		using iterator       = RegistryIterator<IterateFlag::All>;
		using const_iterator = RegistryConstIterator<IterateFlag::All>;
		using value_type     = RegistryEntry<IterateFlag::All>;
		// NOLINTEND(readability-identifier-naming)

	public:
		enum AccessRights {
			Read        = KEY_READ,
			Write       = KEY_WRITE,
			Wow64_32key = KEY_WOW64_32KEY, // NOLINT
			Wow64_64key = KEY_WOW64_64KEY, // NOLINT
		};

	private:
		// indicates whether the destructor needs to close the system handle, if false, the destructor will do nothing
		bool m_bNeedDestruct_ = true;
		// the native system handle of current registry key, it will be closed in destructor if m_bNeedDestruct_ is true
		HKEY m_hKey_ = nullptr;
		// the full path of current registry key, for example, "HKCU\Software\Microsoft", it will be used for getPath() method, and it will be set when creating
		// or opening a registry key
		std::string m_strPath_;

	private:
		/**
		 * @brief Internal constructor, only used for Registry class to create a RegistryKey object with a system handle, the created object will be responsible
		 * for closing the handle
		 * @param p_hKey the system handle of the registry key
		 * @date 2026-02-06 00:17
		 */
		explicit RegistryKey(HKEY p_hKey);

	public:
		RegistryKey() = default;
		RegistryKey(const RegistryKey& p_key);
		RegistryKey(RegistryKey&& p_right) noexcept;
		RegistryKey& operator=(RegistryKey&& p_right) noexcept;
		RegistryKey& operator=(const RegistryKey& p_right) noexcept;
		~RegistryKey() noexcept;

		/**
		 * @brief Get the Native Handle object
		 * @date 2026-02-06 00:18
		 */
		HKEY nativeHandle() const noexcept {
			return m_hKey_;
		}

		/**
		 * @brief Judge whether the current key is empty, which means the system handle is null, and all operations on an empty key will throw
		 * NullPointerReferenceException
		 * @date 2026-02-06 00:19
		 */
		bool empty() const noexcept {
			return m_hKey_ == NULL;
		}

		/**
		 * @brief Get the Name object, which is the name of current key, for example, if the full path of current key is "HKEY_CURRENT_USER\Software\Microsoft",
		 * then the name is "Microsoft"
		 * @date 2026-02-06 00:20
		 */
		std::string_view getName() const noexcept;

		/**
		 * @brief Get the Path object, which is the full path of current key, for example, if the full path of current key is
		 * "HKEY_CURRENT_USER\Software\Microsoft", then the path is "HKCU\Software\Microsoft"
		 * @date 2026-02-06 00:20
		 * @return const std::string&
		 */
		const std::string& getPath() const noexcept {
			return m_strPath_;
		}

		/**
		 * @brief Open the specified sub key with the given access right, if the sub key doesn't exist or failed to open, return an empty RegistryKey object
		 * which system handle is null, and all operations on it will throw NullPointerReferenceException
		 * @param p_strSubKeyName the name of the sub key to open
		 * @param p_rights the access right to open the sub key, default is AccessRights::Read, which means open the sub key with read access right
		 * @date 2026-02-06 00:23
		 * @return RegistryKey a new RegistryKey object which system handle is the opened sub key, if the sub key doesn't exist or failed to open, return an
		 * empty RegistryKey object which system handle is null
		 */
		RegistryKey openSubKey(std::string_view p_strSubKeyName, int p_rights = AccessRights::Read) const;

		/**
		 * @brief Create the specified sub key with the given access right, if the sub key already exists, return the existing sub key, otherwise create a new
		 * sub key and return it, if failed to create or open the sub key, throw RegistryException
		 * @param p_strSubKeyName the name of the sub key to create
		 * @param p_rights the access right to create the sub key, default is AccessRights::Read | AccessRights::Write, which means create the sub key with read
		 * and write access right
		 * @date 2026-02-06 00:44
		 * @return RegistryKey a new RegistryKey object which system handle is the created or existing sub key, if failed to create or open the sub key, throw
		 * RegistryException
		 */
		RegistryKey createSubKey(std::string_view p_strSubKeyName, int p_rights = AccessRights::Read | AccessRights::Write);

		/**
		 * @brief Delete the specified sub key, if failed to delete the sub key, throw RegistryException
		 * @param p_strSubKeyName the name of the sub key to delete
		 * @date 2026-02-07 11:07
		 * @return true if the sub key was deleted successfully
		 * @return false if the sub key does not exist
		 */
		bool deleteSubKey(std::string_view p_strSubKeyName);

		/**
		 * @brief Get the value of specified value name, if failed to get the value, throw
		 * RegistryException
		 * @param p_strValueName the name of the value to get, if empty, will use "DEFAULT" as the value name, which is a special value name in Windows
		 * Registry, and it can be used to store the default value of a registry key
		 * @date 2026-02-07 11:08
		 * @return RegistryValue
		 * @date 2026-03-10 18:42
		 */
		RegistryValue getValue(std::string_view p_strValueName) const;

		/**
		 * @brief Delete the specified value, if failed to delete the value, throw RegistryException, if the value doesn't exist, return false
		 * @param p_strValueName the name of the value to delete, if empty, will use "DEFAULT" as the value name, which is a special value name in Windows
		 * Registry, and it can be used to store the default value of a registry key
		 * @date 2026-03-10 19:23
		 */
		bool deleteValue(std::string_view p_strValueName);

		/**
		 * @brief Get the count of sub keys under current key
		 * @date 2026-03-11 13:43
		 */
		size_t subKeyCount() const;

		/**
		 * @brief Get the count of values under current key
		 * @date 2026-03-11 13:44
		 */
		size_t valueCount() const;

		/**
		 * @brief Get the names of all values under current key
		 * @date 2026-03-11 13:43
		 */
		std::vector<std::string> valueNames() const;

		/**
		 * @brief Get the names of all sub keys under current key
		 * @date 2026-03-11 13:43
		 */
		std::vector<std::string> subKeyNames() const;

		/**
		 * @brief Compare two RegistryKey objects, they are considered equal if their system handles are the same
		 * @param p_other the other RegistryKey object to compare with
		 * @date 2026-03-11 13:46
		 */
		inline bool operator==(const RegistryKey& p_other) const noexcept {
			return m_hKey_ == p_other.m_hKey_;
		}

		/**
		 * @brief Get the iterator of current registry key, the iterator will iterate all sub keys and values under current key, and the iterating item is a
		 * variant type which can be either a RegistryKey object or a pair of value name and value data, you can use
		 * @date 2026-03-11 14:33
		 */
		RegistryIterator<IterateFlag::All> begin();

		/**
		 * @brief Get the end iterator of current registry key, the end iterator is a special iterator which indicates the end of iterating, it doesn't point to
		 * any item, and all operations on it will throw, you can use it to compare with the iterating iterator to judge
		 * @date 2026-03-11 14:34
		 */
		RegistryIterator<IterateFlag::All> end();

		/**
		 * @brief
		 * @return RegistryConstIterator<IterateFlag::All>
		 * @date 2026-03-12 15:47
		 */
		RegistryConstIterator<IterateFlag::All> cbegin() const;

		/**
		 * @brief
		 * @return RegistryConstIterator<IterateFlag::All>
		 * @date 2026-03-12 15:47
		 */
		RegistryConstIterator<IterateFlag::All> cend() const;

		/**
		 * @brief Get the iterator of sub keys under current registry key
		 * @date 2026-03-11 15:27
		 */
		RegistryRangeIterator<IterateFlag::Key> subKeys() {
			return RegistryRangeIterator<IterateFlag::Key>(this);
		}
		RegistryConstRangeIterator<IterateFlag::Key> subKeys() const {
			return RegistryConstRangeIterator<IterateFlag::Key>(this);
		}

		/**
		 * @brief Get the iterator of values under current registry key
		 * @date 2026-03-11 15:28
		 */
		RegistryRangeIterator<IterateFlag::Value> values() {
			return RegistryRangeIterator<IterateFlag::Value>(this);
		}
		RegistryConstRangeIterator<IterateFlag::Value> values() const {
			return RegistryConstRangeIterator<IterateFlag::Value>(this);
		}

		/**
		 * @brief Set the value of specified value name, if failed to set the value, throw RegistryException
		 * @param p_strValueName the name of the value to set, if empty, will use "DEFAULT" as the value name, which is a special value name in Windows
		 * Registry, and it can be used to store the default value of a registry key
		 * @param p_value the value data to set
		 * @date 2026-03-12 10:31
		 */
		bool setValue(std::string_view p_strValueName, const RegistryValue& p_value);

		/**
		 * @brief Set the value of specified value name with the given arguments
		 * @tparam ArgsT_ the type of the arguments to construct the RegistryValue, it should be constructible to RegistryValue
		 * @param p_strValueName the name of the value to set, if empty, will use "DEFAULT" as the value name, which is a special value name in Windows
		 * Registry, and it can be used to store the default value of a registry key
		 * @param p_args the arguments to construct the RegistryValue
		 * @date 2026-03-16 10:44
		 */
		template <typename... ArgsT_>
		    requires(std::is_constructible_v<RegistryValue, ArgsT_...> && !std::is_same_v<std::decay_t<ArgsT_>..., RegistryValue>)
		bool setValue(std::string_view p_strValueName, ArgsT_&&... p_args) {
			return setValue(p_strValueName, RegistryValue(std::forward<ArgsT_>(p_args)...));
		}

		/**
		 * @brief Get the value of specified value name
		 * @param p_strValueName the name of the value to get
		 * @date 2026-03-12 10:41
		 */
		RegistryValueWrapper operator[](std::string_view p_strValueName) {
			return RegistryValueWrapper(this, p_strValueName);
		}

		/**
		 * @brief Get the value of specified value name
		 * @param p_strValueName the name of the value to get
		 * @date 2026-03-12 10:41
		 */
		RegistryConstValueWrapper operator[](std::string_view p_strValueName) const {
			return RegistryConstValueWrapper(this, p_strValueName);
		}

		/**
		 * @brief Close the current registry key, after calling this method, the system handle of current object will be null, and all operations on it will
		 * throw
		 * @date 2026-03-11 13:47
		 */
		bool close();

		friend class Registry;
	};
}