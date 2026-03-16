#include "cpp-registry/error.h"
#include "cpp-registry/utils.h"

namespace kare {
	RegistryException::RegistryException(LSTATUS p_lCode)
	    : Exception(details::getErrorMessage(p_lCode).c_str())
	    , m_lInternalCode_(p_lCode) {
	}

	InvalidArgumentException::InvalidArgumentException(std::string_view p_strArgumentName)
	    : Exception(std::string("Invalid argument: ") + std::string(p_strArgumentName) + "!") {
	}
}