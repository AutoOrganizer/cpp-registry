#pragma once
#include <string>

namespace kare::details {
	template <typename TT_, typename... TypesT_>
	inline constexpr bool is_any_of_v = std::disjunction_v<std::is_same<TT_, TypesT_>...>; // NOLINT

	// the size of the generic buffer used for various operations
	inline constexpr size_t gc_nGenericBufferSize = 256;

	/**
	 * @brief Format the LSTATUS code into a human-readable error message.
	 * @param p_lCode The LSTATUS code to format.
	 * @date 2026-02-06 00:15
	 */
	std::string getErrorMessage(LSTATUS p_lCode);
}