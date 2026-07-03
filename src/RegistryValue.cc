#include "cpp-registry/RegistryValue.h"


namespace kare {
	RegistryValue::RegistryValue(std::string_view p_strValue, bool p_bExpand)
	    : m_dwType_(p_bExpand ? REG_EXPAND_SZ : REG_SZ) {
		bool bHasNullTerminator = !p_strValue.empty() && p_strValue.back() == '\0';

		if (!bHasNullTerminator) {
			m_vBuffer_.reserve(p_strValue.size() + 1);
		}
		m_vBuffer_.assign(p_strValue.begin(), p_strValue.end());
		if (!bHasNullTerminator) {
			m_vBuffer_.push_back('\0');
		}
	}

	RegistryValue::RegistryValue(const std::vector<std::string>& p_vstrValues)
	    : m_dwType_(REG_MULTI_SZ) {
		size_t totalSize = 0;
		for (const auto& str : p_vstrValues) {
			totalSize += str.size() + 1; // +1 for null terminator
		}
		if (totalSize == 0) {
			totalSize = 1; // at least one null terminator if the list is empty
		}
		totalSize++; // additional null terminator at the end

		m_vBuffer_.reserve(totalSize);
		for (const auto& str : p_vstrValues) {
			m_vBuffer_.insert(m_vBuffer_.end(), str.begin(), str.end());
			m_vBuffer_.push_back('\0');
		}
		if (m_vBuffer_.empty()) {
			m_vBuffer_.push_back('\0');
		}
		m_vBuffer_.push_back('\0');
	}

	RegistryValue::RegistryValue(std::span<unsigned char> p_vBuffer, DWORD p_dwType)
	    : m_dwType_(p_dwType) {
		if (p_dwType == REG_NONE) {
			throw InvalidArgumentException("p_dwType");
		}

		if (this->type() == Type::String) {
			bool bHasNullTerminator = !p_vBuffer.empty() && p_vBuffer.back() == '\0';
			if (!bHasNullTerminator) {
				m_vBuffer_.reserve(p_vBuffer.size() + 1);
			}
			m_vBuffer_.assign(p_vBuffer.begin(), p_vBuffer.end());
			if (!bHasNullTerminator) {
				m_vBuffer_.push_back('\0');
			}
		} else if (this->type() == Type::StringList) {
			bool bHasDoubleNullTerminator = p_vBuffer.size() >= 2 && p_vBuffer[p_vBuffer.size() - 1] == '\0' && p_vBuffer[p_vBuffer.size() - 2] == '\0';
			if (!bHasDoubleNullTerminator) {
				m_vBuffer_.reserve(p_vBuffer.size() + 2);
			}
			m_vBuffer_.assign(p_vBuffer.begin(), p_vBuffer.end());
			if (!bHasDoubleNullTerminator) {
				m_vBuffer_.push_back('\0');
				m_vBuffer_.push_back('\0');
			}
		} else {
			m_vBuffer_.assign(p_vBuffer.begin(), p_vBuffer.end());
		}
	}

	RegistryValue::Type RegistryValue::type() const {
		switch (m_dwType_) {
			case REG_DWORD:
			case REG_DWORD_BIG_ENDIAN:
				return Type::DWord;
			case REG_QWORD:
				return Type::QWord;
			case REG_SZ:
			case REG_EXPAND_SZ:
			case REG_LINK:
				return Type::String;
			case REG_MULTI_SZ:
			case REG_RESOURCE_LIST:
			case REG_FULL_RESOURCE_DESCRIPTOR:
			case REG_RESOURCE_REQUIREMENTS_LIST:
				return Type::StringList;
			case REG_BINARY:
				return Type::Binary;
			default:
				return Type::None;
		}
	}

	bool RegistryValue::boolValue() const {
		if (this->type() == Type::DWord) {
			return this->dWordValue() != 0;
		}

		if (this->type() == Type::QWord) {
			return this->qWordValue() != 0;
		}

		throw InvalidAccessException("The registry value is not of an arithmetic type");
	}

	int32_t RegistryValue::dWordValue() const {
		if (this->type() == Type::DWord) {
			DWORD value = 0;
			::memcpy_s(&value, sizeof(DWORD), m_vBuffer_.data(), m_vBuffer_.size());
			if (m_dwType_ == REG_DWORD_BIG_ENDIAN) {
				value = _byteswap_ulong(value);
			}
			return static_cast<int32_t>(value);
		}

		throw InvalidAccessException("The registry value is not of an arithmetic type");
	}

	int64_t RegistryValue::qWordValue() const {
		if (this->type() == Type::QWord) {
			int64_t value = 0;
			::memcpy_s(&value, sizeof(ULONGLONG), m_vBuffer_.data(), m_vBuffer_.size());
			return static_cast<int64_t>(value);
		}

		if (this->type() == Type::DWord) {
			return static_cast<int64_t>(this->dWordValue());
		}

		throw InvalidAccessException("The registry value is not of an arithmetic type");
	}

	double RegistryValue::doubleValue() const {
		if (this->type() == Type::QWord) {
			double value = 0;
			::memcpy_s(&value, sizeof(double), m_vBuffer_.data(), m_vBuffer_.size());
			return value;
		}

		if (this->type() == Type::DWord) {
			return static_cast<double>(this->dWordValue());
		}

		throw InvalidAccessException("The registry value is not of an arithmetic type");
	}

	std::string_view RegistryValue::stringValue() const& {
		if (this->type() == Type::String) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			return std::string_view { reinterpret_cast<const char*>(m_vBuffer_.data()), m_vBuffer_.size() - 1 };
		}

		throw InvalidAccessException("The registry value is not of a string type");
	}

	std::vector<std::string_view> RegistryValue::multiStringValue() const& {
		if (this->type() == Type::StringList) {
			std::vector<std::string_view> result;

			size_t nStart = 0;
			for (size_t nIdx = 0; nIdx < m_vBuffer_.size(); ++nIdx) {
				if (m_vBuffer_[nIdx] == '\0') {
					if (nIdx > nStart) {
						// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
						result.emplace_back(reinterpret_cast<const char*>(&m_vBuffer_[nStart]), nIdx - nStart);
					}
					nStart = nIdx + 1;
				}
			}
			return result;
		}

		throw InvalidAccessException("The registry value is not of a multi string type");
	}

	const std::vector<unsigned char>& RegistryValue::binaryValue() const {
		if (this->type() == Type::Binary) {
			return m_vBuffer_;
		}

		throw InvalidAccessException("The registry value is not of a binary type");
	}
}