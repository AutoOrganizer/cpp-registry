#pragma once
#include <cpp-registry/pch.h> // fix clangd errors
#include <cpp-registry/RegistryKey.h>

#include <array>
#include <variant>


namespace kare {
	template <IterateFlag Flag_>
	class RegistryEntry;

	template <>
	class RegistryEntry<IterateFlag::Key> : public RegistryKey {
		using RegistryKey::RegistryKey;

	public:
		RegistryEntry(RegistryKey&& p_key)
		    : RegistryKey(std::move(p_key)) {
		}
	};

	template <>
	class RegistryEntry<IterateFlag::Value> {
	public:
		std::string   name;
		RegistryValue value;
	};

	template <>
	class RegistryEntry<IterateFlag::All> {
		template <IterateFlag Flag_>
		friend class RegistryConstIterator;

	private:
		std::variant<RegistryEntry<IterateFlag::Key>, RegistryEntry<IterateFlag::Value>> m_value_;

	public:
		bool isKey() const noexcept {
			return std::holds_alternative<RegistryEntry<IterateFlag::Key>>(m_value_);
		}

		bool isValue() const noexcept {
			return !isKey();
		}

		RegistryEntry<IterateFlag::Key>& key() & {
			return std::get<RegistryEntry<IterateFlag::Key>>(m_value_);
		}

		RegistryEntry<IterateFlag::Value>& value() & {
			return std::get<RegistryEntry<IterateFlag::Value>>(m_value_);
		}

		RegistryEntry<IterateFlag::Key>&& key() && {
			return std::move(std::get<RegistryEntry<IterateFlag::Key>>(m_value_));
		}

		RegistryEntry<IterateFlag::Value>&& value() && {
			return std::move(std::get<RegistryEntry<IterateFlag::Value>>(m_value_));
		}
	};

	template <IterateFlag IterFlag_>
	class RegistryConstIterator {
	private:
		enum class IterateStages : uint8_t { Keys, Values, End };

	public:
		// NOLINTBEGIN
		using value_type        = RegistryEntry<IterFlag_>;
		using reference         = const value_type&;
		using pointer           = const value_type*;
		using iterator_category = std::forward_iterator_tag;
		// NOLINTEND

	private:
		// the key object to iterate
		const RegistryKey* m_pKey_ { nullptr };
		// the index of current iterating item
		uint16_t m_nIdx_ { 0 };
		// the stage of iterating, only used when iterate both keys and values
		IterateStages m_stage_ { IterFlag_ == IterateFlag::Value ? IterateStages::Values : IterateStages::Keys };
		// current iterating item
		value_type m_cur_;

	private:
		bool doCurrent_() {
			std::array<char, details::gc_nGenericBufferSize> aszBuffer {};

			// only query current state, don't update m_nIdx_ and m_stage_ here.
			size_t nIdx  = m_nIdx_;
			DWORD  dwLen = static_cast<DWORD>(aszBuffer.size());
			auto   stage = m_stage_;

			if constexpr (IterFlag_ != IterateFlag::Value) {
				while (stage == IterateStages::Keys) {
					LSTATUS lRes = ::RegEnumKeyEx(m_pKey_->nativeHandle(), nIdx, aszBuffer.data(), &dwLen, NULL, NULL, NULL, NULL);
					if (lRes == ERROR_NO_MORE_ITEMS) {
						if constexpr (IterFlag_ == IterateFlag::All) {
							stage = IterateStages::Values;
							nIdx  = 0;
							break;
						}
						return false;
					}

					if (lRes != ERROR_SUCCESS) {
						throw RegistryException(lRes);
					}

					if constexpr (IterFlag_ == IterateFlag::All) {
						m_cur_.m_value_ = m_pKey_->openSubKey(std::string_view { aszBuffer.data(), dwLen });
					} else {
						m_cur_ = m_pKey_->openSubKey(std::string_view { aszBuffer.data(), dwLen });
					}
					return true;
				}
			}

			if constexpr (IterFlag_ != IterateFlag::Key) {
				if (stage == IterateStages::Values) {
					dwLen = static_cast<DWORD>(aszBuffer.size());

					LSTATUS lRes = ::RegEnumValue(m_pKey_->nativeHandle(), nIdx, aszBuffer.data(), &dwLen, NULL, NULL, NULL, NULL);
					if (lRes == ERROR_NO_MORE_ITEMS) {
						m_cur_.~value_type();
						return false;
					}

					if (lRes != ERROR_SUCCESS) {
						throw RegistryException(lRes);
					}

					std::string_view                  strValueName { aszBuffer.data(), dwLen };
					RegistryEntry<IterateFlag::Value> entry {
						.name  = std::string { strValueName },
						.value = m_pKey_->getValue(strValueName),
					};

					if constexpr (IterFlag_ == IterateFlag::All) {
						m_cur_.m_value_ = entry;
					} else {
						m_cur_ = entry;
					}
					return true;
				}
			}
			throw OutOfRangeException("Iterator out of range");
		}

	public:
		RegistryConstIterator() = default;
		RegistryConstIterator(const RegistryKey* p_pKey, uint16_t p_nIdx = 0)
		    : m_pKey_(p_pKey)
		    , m_nIdx_(p_nIdx)
		    , m_stage_(IterFlag_ == IterateFlag::Value ? IterateStages::Values : IterateStages::Keys) {
			if (m_nIdx_ == std::numeric_limits<uint16_t>::max()) {
				if constexpr (IterFlag_ == IterateFlag::All) {
					m_stage_ = IterateStages::End;
				}
				return;
			}

			if (!this->doCurrent_()) {
				m_nIdx_ = std::numeric_limits<uint16_t>::max();
				if constexpr (IterFlag_ == IterateFlag::All) {
					m_stage_ = IterateStages::End;
				}
			} else if constexpr (IterFlag_ == IterateFlag::All) {
				if (m_cur_.isValue()) {
					m_stage_ = IterateStages::Values;
				}
			}
		}

	public:
		RegistryConstIterator& operator++() {
			if (m_nIdx_ == std::numeric_limits<uint16_t>::max()) {
				return *this;
			}

			m_nIdx_++;
			if (this->doCurrent_()) {
				if constexpr (IterFlag_ == IterateFlag::All) {
					if (m_cur_.isValue() && m_stage_ == IterateStages::Keys) {
						m_stage_ = IterateStages::Values;
						m_nIdx_  = 0;
					}
				}
			} else {
				m_nIdx_ = std::numeric_limits<uint16_t>::max();
				m_cur_.~value_type();

				if constexpr (IterFlag_ == IterateFlag::All) {
					m_stage_ = IterateStages::End;
				}
			}
			return *this;
		}

		reference operator*() {
			return m_cur_;
		}

		pointer operator->() {
			return &m_cur_;
		}

		RegistryConstIterator operator++(int) {
			RegistryConstIterator tmp = *this;
			this->operator++();
			return tmp;
		}

		bool operator==(const RegistryConstIterator& p_other) const noexcept {
			return m_pKey_ == p_other.m_pKey_ && m_nIdx_ == p_other.m_nIdx_ && m_stage_ == p_other.m_stage_;
		}
		bool operator!=(const RegistryConstIterator& p_other) const noexcept {
			return !(*this == p_other);
		}
	};

	template <IterateFlag IterFlag_>
	class RegistryIterator : public RegistryConstIterator<IterFlag_> {
	public:
		using RegistryConstIterator<IterFlag_>::RegistryConstIterator;

	public:
		// NOLINTBEGIN
		using value_type        = RegistryEntry<IterFlag_>;
		using reference         = value_type&;
		using pointer           = value_type*;
		using iterator_category = std::forward_iterator_tag;
		// NOLINTEND

	public:
		reference operator*() {
			return const_cast<reference>(RegistryConstIterator<IterFlag_>::operator*());
		}

		pointer operator->() {
			return const_cast<pointer>(RegistryConstIterator<IterFlag_>::operator->());
		}
	};
}