#pragma once
#include <vector>
#include <string>
#include <span>


namespace kare {
	/**
	 * @brief A class that represents a registry value
	 * Note:
	 * The type of the registry value is mapped to the following types:
	 * - None: REG_NONE
	 * - Dword: REG_DWORD, REG_DWORD_LITTLE_ENDIAN, REG_DWORD_BIG_ENDIAN
	 * - Qword: REG_QWORD, REG_QWORD_LITTLE_ENDIAN
	 * - String: REG_SZ, REG_EXPAND_SZ
	 * - StringList: REG_MULTI_SZ, REG_RESOURCE_LIST, REG_FULL_RESOURCE_DESCRIPTOR, REG_RESOURCE_REQUIREMENTS_LIST
	 * - Binary: REG_BINARY
	 * Accessing the value will throw an exception if the type is not correct
	 * @date 2026-03-10 18:36
	 */
	class RegistryValue {
		friend class RegistryKey;
	public:
		enum class Type : uint8_t {
			None,
			DWord,
			QWord,
			String,
			StringList,
			Binary,
		};

	private:
		DWORD             m_dwType_;
		std::vector<byte> m_vBuffer_;

	public:
		RegistryValue()
		    : m_dwType_(REG_NONE) {
		}

		template <typename ArgT_>
		    requires(std::is_arithmetic_v<ArgT_> && sizeof(ArgT_) <= sizeof(DWORD))
		RegistryValue(ArgT_ p_value, bool p_bBigEndian = false)
		    : m_dwType_(p_bBigEndian ? REG_DWORD_BIG_ENDIAN : REG_DWORD) {
			m_vBuffer_.resize(sizeof(ArgT_));
			if (p_bBigEndian) {
				for (size_t i = 0; i < sizeof(ArgT_); i++) {
					m_vBuffer_[sizeof(ArgT_) - 1 - i] = static_cast<byte>((p_value >> (i * 8)) & 0xFF); // NOLINT(cppcoreguidelines-avoid-magic-numbers)
				}
			} else {
				::memcpy(m_vBuffer_.data(), &p_value, sizeof(ArgT_));
			}
		}

		template <typename ArgT_>
		    requires(std::is_arithmetic_v<ArgT_> && sizeof(ArgT_) > sizeof(DWORD) && sizeof(ArgT_) <= sizeof(UINT64))
		RegistryValue(ArgT_ p_value)
		    : m_dwType_(REG_QWORD) {
			m_vBuffer_.resize(sizeof(ArgT_));
			::memcpy(m_vBuffer_.data(), &p_value, sizeof(ArgT_));
		}

		/**
		 * @brief Construct a new Registry Value object from a string, the type will be REG_SZ or REG_EXPAND_SZ depending on p_bExpand
		 * @param p_strValue The string value to be stored in the registry
		 * @param p_bExpand Whether the string should be treated as an expandable string (REG_EXPAND_SZ)
		 * @date 2026-03-13 14:18
		 */
		RegistryValue(std::string_view p_strValue, bool p_bExpand = false);

		/**
		 * @brief Construct a new Registry Value object from a list of strings, the type will be REG_MULTI_SZ
		 * @param p_vstrValues The list of strings to be stored in the registry
		 * @date 2026-03-13 14:19
		 */
		RegistryValue(const std::vector<std::string>& p_vstrValues);

		/**
		 * @brief Construct a new Registry Value object from a binary buffer
		 * @param p_vBuffer The binary buffer to be stored in the registry
		 * @param p_dwType The raw registry type
		 * @date 2026-03-13 14:21
		 */
		RegistryValue(std::span<byte> p_vBuffer, DWORD p_dwType = REG_BINARY);

		/**
		 * @brief Get the type of the registry value
		 * @date 2026-03-10 18:30
		 */
		Type type() const;

		/**
		 * @brief Get the bool value, reinterpret the dword or qword value as a bool
		 * @date 2026-03-10 17:42
		 */
		bool boolValue() const;

		/**
		 * @brief Get the dWord value
		 * @date 2026-03-10 17:42
		 */
		int32_t dWordValue() const;

		/**
		 * @brief Get the qWord value, if is a dword value, reinterpret it as a qword value
		 * @date 2026-03-10 17:42
		 */
		int64_t qWordValue() const;

		/**
		 * @brief Get the double value, if is a dword or qword value, reinterpret it as a double value
		 * @date 2026-03-10 17:42
		 */
		double doubleValue() const;

		/**
		 * @brief Get the string value
		 * @date 2026-03-10 17:42
		 */
		std::string_view stringValue() const&;
		std::string stringValue() const&& {
			return std::string(this->stringValue());
		}

		/**
		 * @brief Get the multi string value
		 * @return std::vector<std::string_view>
		 * @date 2026-03-10 17:42
		 */
		std::vector<std::string_view> multiStringValue() const&;
		std::vector<std::string> multiStringValue() const&& {
			std::vector<std::string> result;
			for (const auto& str : this->multiStringValue()) {
				result.emplace_back(str);
			}
			return result;
		}

		/**
		 * @brief Get the binary value
		 * @return std::vector<byte>
		 * @date 2026-03-10 17:42
		 */
		const std::vector<byte>& binaryValue() const;

		/**
		 * @brief Get the raw type of the registry value
		 * @date 2026-03-10 18:27
		 */
		DWORD rawType() const {
			return m_dwType_;
		}

		/**
		 * @brief Check if the registry value is empty
		 * @date 2026-03-10 18:44
		 */
		bool empty() const {
			return this->type() == Type::None;
		}

		/**
		 * @brief Check if the registry value is empty
		 * @date 2026-03-10 18:44
		 */
		operator bool() const {
			return !this->empty();
		}
	};
}