#include "cpp-registry/utils.h"

#include <array>


namespace kare::details {
	std::string getErrorMessage(LSTATUS p_lCode) {
		std::array<char, gc_nGenericBufferSize> szBuffer {};

		::FormatMessageA(
		    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		    nullptr,
		    p_lCode,
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    szBuffer.data(),
		    static_cast<DWORD>(szBuffer.size()),
		    nullptr
		);

		return std::string(szBuffer.data());
	}
}