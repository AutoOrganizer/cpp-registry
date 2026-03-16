#pragma once
#include <exception>
#include <string_view>


namespace kare {
	class Exception : public std::exception {
	public:
		Exception(std::string_view p_strMessage)
		    : std::exception(p_strMessage.empty() ? "Empty error message!" : p_strMessage.data()) {
		}
	};

	class NullPointerReferenceException : public Exception {
	public:
		NullPointerReferenceException()
		    : Exception("Attempt to dereference a null pointer!") {
		}
	};

	class InvalidArgumentException : public Exception {
	public:
		InvalidArgumentException()
		    : Exception("Invalid argument!") {
		}
		InvalidArgumentException(std::string_view p_strArgumentName);
	};

	class RegistryException : public Exception {
	private:
		LSTATUS m_lInternalCode_;

	public:
		RegistryException(LSTATUS p_lCode);

		/**
		 * @brief Get native error code.
		 * @date 2026-02-05 21:17
		 */
		LSTATUS getInternalCode() const {
			return m_lInternalCode_;
		}
	};

	class InvalidAccessException : public Exception {
		using Exception::Exception;
	};

	class OutOfRangeException : public Exception {
		using Exception::Exception;
	};
} // namespace kare

#ifdef CPP_REGISTRY_HEADER_ONLY
#include <cpp-registry/error.cc>
#endif