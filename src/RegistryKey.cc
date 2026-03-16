#include "cpp-registry/RegistryKey.h"

#include "cpp-registry/error.h"
#include "cpp-registry/utils.h"
#include "cpp-registry/RegistryIterator.h"

#include <array>


namespace kare {
	template <IterateFlag IterFlag_>
	RegistryConstIterator<IterFlag_> RegistryConstRangeIterator<IterFlag_>::begin() const {
		return RegistryConstIterator<IterFlag_>(m_pKey_);
	}

	template <IterateFlag IterFlag_>
	RegistryConstIterator<IterFlag_> RegistryConstRangeIterator<IterFlag_>::end() const {
		return RegistryConstIterator<IterFlag_>(m_pKey_, std::numeric_limits<uint16_t>::max());
	}

	template class RegistryConstRangeIterator<IterateFlag::Key>;
	template class RegistryConstRangeIterator<IterateFlag::Value>;

	template <IterateFlag IterFlag_>
	RegistryIterator<IterFlag_> RegistryRangeIterator<IterFlag_>::begin() {
		return RegistryIterator<IterFlag_>(m_pKey_);
	}

	template <IterateFlag IterFlag_>
	RegistryIterator<IterFlag_> RegistryRangeIterator<IterFlag_>::end() {
		return RegistryIterator<IterFlag_>(m_pKey_, std::numeric_limits<uint16_t>::max());
	}

	template class RegistryRangeIterator<IterateFlag::Key>;
	template class RegistryRangeIterator<IterateFlag::Value>;

	RegistryConstValueWrapper::operator RegistryValue() const {
		if (m_pKey_ == nullptr) {
			throw NullPointerReferenceException();
		}

		return m_pKey_->getValue(m_strValueName_);
	}

	RegistryValueWrapper& RegistryValueWrapper::operator=(const RegistryValue& p_value) {
		if (m_pKey_ == nullptr) {
			throw NullPointerReferenceException();
		}

		const_cast<RegistryKey*>(m_pKey_)->setValue(m_strValueName_, p_value); // NOLINT(cppcoreguidelines-pro-type-const-cast)
		return *this;
	}

	RegistryKey::RegistryKey(HKEY p_hKey)
	    : m_hKey_(p_hKey) {
		if (m_hKey_ == HKEY_CLASSES_ROOT) {
			m_strPath_ = "HKCR";
		} else if (m_hKey_ == HKEY_CURRENT_USER) {
			m_strPath_ = "HKCU";
		} else if (m_hKey_ == HKEY_LOCAL_MACHINE) {
			m_strPath_ = "HKLM";
		} else if (m_hKey_ == HKEY_USERS) {
			m_strPath_ = "HKU";
		} else if (m_hKey_ == HKEY_PERFORMANCE_DATA) {
			m_strPath_ = "HKPD";
		} else {
			throw InvalidArgumentException("p_hKey");
		}
	}

	RegistryKey::RegistryKey(const RegistryKey& p_key)
	    : m_bNeedDestruct_(false)
	    , m_hKey_(p_key.m_hKey_)
	    , m_strPath_(p_key.m_strPath_) {
	}

	RegistryKey::RegistryKey(RegistryKey&& p_right) noexcept
	    : m_bNeedDestruct_(p_right.m_bNeedDestruct_)
	    , m_hKey_(p_right.m_hKey_)
	    , m_strPath_(std::move(p_right.m_strPath_)) {
		p_right.m_bNeedDestruct_ = false;
		p_right.m_hKey_          = nullptr;
	}

	RegistryKey::~RegistryKey() noexcept {
		if (m_bNeedDestruct_ && m_hKey_ != nullptr) {
			::RegCloseKey(m_hKey_);
		}
		m_hKey_ = nullptr;
	}

	RegistryKey& RegistryKey::operator=(RegistryKey&& p_right) noexcept {
		if (this != &p_right) {
			this->close();

			m_bNeedDestruct_ = p_right.m_bNeedDestruct_;
			m_hKey_          = p_right.m_hKey_;
			m_strPath_       = std::move(p_right.m_strPath_);

			p_right.m_bNeedDestruct_ = false;
			p_right.m_hKey_          = nullptr;
		}
		return *this;
	}

	RegistryKey& RegistryKey::operator=(const RegistryKey& p_right) noexcept {
		if (this != &p_right) {
			this->close();

			m_bNeedDestruct_ = false;
			m_hKey_          = p_right.m_hKey_;
			m_strPath_       = p_right.m_strPath_;
		}
		return *this;
	}

	std::string_view RegistryKey::getName() const noexcept {
		std::string_view strName { m_strPath_ };

		size_t nIdx = strName.find_last_of('\\');
		if (nIdx == std::string::npos) {
			return strName;
		}

		return strName.substr(nIdx + 1);
	}

	RegistryKey RegistryKey::openSubKey(std::string_view p_strSubKeyName, int p_rights) const {
		if (nullptr == m_hKey_) {
			throw NullPointerReferenceException();
		}

		if (p_strSubKeyName.empty()) {
			throw InvalidArgumentException("p_strSubKeyName");
		}

		std::array<char, details::gc_nGenericBufferSize> szBuffer {};
		p_strSubKeyName.copy(szBuffer.data(), szBuffer.size() - 1);

		RegistryKey key;
		LSTATUS     lRes = ::RegOpenKeyEx(m_hKey_, szBuffer.data(), 0, p_rights, &key.m_hKey_);

		switch (lRes) {
			case ERROR_SUCCESS:
				break;
			case ERROR_FILE_NOT_FOUND:
				return key;
			default:
				throw RegistryException(lRes);
		}

		key.m_strPath_.reserve(m_strPath_.size() + 1 + p_strSubKeyName.size());
		key.m_strPath_.append(m_strPath_).append("\\").append(p_strSubKeyName);
		return key;
	}

	RegistryKey RegistryKey::createSubKey(std::string_view p_strSubKeyName, int p_rights) {
		RegistryKey key;
		key = this->openSubKey(p_strSubKeyName, p_rights);
		if (!key.empty()) {
			return key;
		}

		std::array<char, details::gc_nGenericBufferSize> szBuffer {};
		p_strSubKeyName.copy(szBuffer.data(), szBuffer.size() - 1);

		LSTATUS lRes = RegCreateKeyEx(m_hKey_, szBuffer.data(), 0, NULL, 0, p_rights, NULL, &key.m_hKey_, NULL);
		switch (lRes) {
			case ERROR_SUCCESS:
				break;
			default:
				throw RegistryException(lRes);
		}

		key.m_strPath_.reserve(m_strPath_.size() + 1 + p_strSubKeyName.size());
		key.m_strPath_.append(m_strPath_).append("\\").append(p_strSubKeyName);
		return key;
	}

	bool RegistryKey::deleteSubKey(std::string_view p_strSubKeyName) {
		if (nullptr == m_hKey_) {
			throw NullPointerReferenceException();
		}

		if (p_strSubKeyName.empty()) {
			throw InvalidArgumentException("p_strSubKeyName");
		}

		std::array<char, details::gc_nGenericBufferSize> szBuffer {};
		p_strSubKeyName.copy(szBuffer.data(), szBuffer.size() - 1);

		LSTATUS lRes = RegDeleteTreeA(m_hKey_, szBuffer.data());

		switch (lRes) {
			case ERROR_FILE_NOT_FOUND:
				return false;
			case ERROR_SUCCESS:
				return true;
			default:
				throw RegistryException(lRes);
		}
	}

	RegistryValue RegistryKey::getValue(std::string_view p_strValueName) const {
		if (nullptr == m_hKey_) {
			throw NullPointerReferenceException();
		}

		RegistryValue value;

		std::array<char, details::gc_nGenericBufferSize> szBuffer {};
		if (!p_strValueName.empty()) {
			p_strValueName.copy(szBuffer.data(), szBuffer.size() - 1);
		}

		DWORD   dwRawLen = 0;
		LSTATUS lRes     = ::RegQueryValueEx(m_hKey_, szBuffer.data(), NULL, &value.m_dwType_, NULL, &dwRawLen);
		if (ERROR_FILE_NOT_FOUND == lRes || 0 == dwRawLen) {
			return value;
		}

		if (lRes != ERROR_SUCCESS) {
			throw RegistryException(lRes);
		}

		value.m_vBuffer_.resize(dwRawLen);
		lRes = RegQueryValueEx(m_hKey_, szBuffer.data(), NULL, NULL, value.m_vBuffer_.data(), &dwRawLen);
		if (lRes != ERROR_SUCCESS) {
			throw RegistryException(lRes);
		}

		return value;
	}

	bool RegistryKey::deleteValue(std::string_view p_strValueName) {
		if (nullptr == m_hKey_) {
			throw NullPointerReferenceException();
		}

		std::array<char, details::gc_nGenericBufferSize> szBuffer {};
		if (!p_strValueName.empty()) {
			p_strValueName.copy(szBuffer.data(), szBuffer.size() - 1);
		}

		LSTATUS lRes = RegDeleteValue(m_hKey_, szBuffer.data());
		switch (lRes) {
			case ERROR_FILE_NOT_FOUND:
				return false;
			case ERROR_SUCCESS:
				return true;
			default:
				throw RegistryException(lRes);
		}
	}

	size_t RegistryKey::valueCount() const {
		if (nullptr == m_hKey_) {
			throw NullPointerReferenceException();
		}

		DWORD   dwValueCount = 0;
		LSTATUS lRes         = ::RegQueryInfoKey(m_hKey_, NULL, NULL, NULL, NULL, NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL);
		if (lRes != ERROR_SUCCESS) {
			throw RegistryException(lRes);
		}

		return static_cast<size_t>(dwValueCount);
	}

	size_t RegistryKey::subKeyCount() const {
		if (nullptr == m_hKey_) {
			throw NullPointerReferenceException();
		}

		DWORD   dwSubKeyCount = 0;
		LSTATUS lRes          = ::RegQueryInfoKey(m_hKey_, NULL, NULL, NULL, &dwSubKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		if (lRes != ERROR_SUCCESS) {
			throw RegistryException(lRes);
		}

		return static_cast<size_t>(dwSubKeyCount);
	}

	std::vector<std::string> RegistryKey::valueNames() const {
		if (nullptr == m_hKey_) {
			throw NullPointerReferenceException();
		}

		std::vector<std::string> vstrValueNames;
		vstrValueNames.reserve(this->valueCount());

		DWORD   dwMaxLen = 0;
		LSTATUS lRes     = RegQueryInfoKey(m_hKey_, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dwMaxLen, NULL, NULL, NULL);
		if (lRes != ERROR_SUCCESS) {
			throw RegistryException(lRes);
		}

		DWORD dwIndex = 0;
		while (dwIndex < vstrValueNames.capacity()) {
			auto  iter  = vstrValueNames.emplace(vstrValueNames.end());
			DWORD dwLen = dwMaxLen + 1;
			iter->resize(dwLen);

			lRes = RegEnumValue(m_hKey_, dwIndex, iter->data(), &dwLen, NULL, NULL, NULL, NULL);
			if (lRes == ERROR_NO_MORE_ITEMS) {
				break;
			}
			if (lRes != ERROR_SUCCESS) {
				throw RegistryException(lRes);
			}

			// the RegEnumValue returns the length of the value name with the null terminator, but the string constructor doesn't expect it, so we need to
			// resize it again.
			iter->resize(dwLen);
			dwIndex++;
		}

		return vstrValueNames;
	}

	std::vector<std::string> RegistryKey::subKeyNames() const {
		if (nullptr == m_hKey_) {
			throw NullPointerReferenceException();
		}

		std::vector<std::string> vstrSubKeyNames;
		vstrSubKeyNames.reserve(this->subKeyCount());

		DWORD   dwMaxLen = 0;
		LSTATUS lRes     = RegQueryInfoKey(m_hKey_, NULL, NULL, NULL, NULL, &dwMaxLen, NULL, NULL, NULL, NULL, NULL, NULL);
		if (lRes != ERROR_SUCCESS) {
			throw RegistryException(lRes);
		}

		DWORD dwIndex = 0;
		while (dwIndex < vstrSubKeyNames.capacity()) {
			auto  iter  = vstrSubKeyNames.emplace(vstrSubKeyNames.end());
			DWORD dwLen = dwMaxLen + 1;
			iter->resize(dwLen);

			lRes = RegEnumKeyEx(m_hKey_, dwIndex, iter->data(), &dwLen, NULL, NULL, NULL, NULL);
			if (lRes == ERROR_NO_MORE_ITEMS) {
				break;
			}
			if (lRes != ERROR_SUCCESS) {
				throw RegistryException(lRes);
			}

			// the RegEnumValue returns the length of the value name with the null terminator, but the string constructor doesn't expect it, so we need to
			// resize it again.
			iter->resize(dwLen);
			dwIndex++;
		}
		return vstrSubKeyNames;
	}

	bool RegistryKey::close() {
		if (!m_bNeedDestruct_) {
			return false;
		}

		if (m_hKey_ != nullptr) {
			::RegCloseKey(m_hKey_);
		}
		m_hKey_ = nullptr;
		return true;
	}

	RegistryIterator<IterateFlag::All> RegistryKey::begin() {
		return RegistryIterator<IterateFlag::All>(this);
	}

	RegistryIterator<IterateFlag::All> RegistryKey::end() {
		return RegistryIterator<IterateFlag::All>(this, std::numeric_limits<uint16_t>::max());
	}

	RegistryConstIterator<IterateFlag::All> RegistryKey::cbegin() const {
		return RegistryConstIterator<IterateFlag::All>(this);
	}

	RegistryConstIterator<IterateFlag::All> RegistryKey::cend() const {
		return RegistryConstIterator<IterateFlag::All>(this, std::numeric_limits<uint16_t>::max());
	}

	bool RegistryKey::setValue(std::string_view p_strValueName, const RegistryValue& p_value) {
		if (nullptr == m_hKey_) {
			throw NullPointerReferenceException();
		}

		std::array<char, details::gc_nGenericBufferSize> szBuffer {};
		if (!p_strValueName.empty()) {
			p_strValueName.copy(szBuffer.data(), szBuffer.size() - 1);
		}

		LSTATUS lRes = RegSetValueEx(m_hKey_, szBuffer.data(), 0, p_value.rawType(), p_value.m_vBuffer_.data(), static_cast<DWORD>(p_value.m_vBuffer_.size()));
		if (lRes != ERROR_SUCCESS) {
			throw RegistryException(lRes);
		}
		return true;
	}
}