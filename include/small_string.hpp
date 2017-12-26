#ifndef GREGJM_SMALL_STRING_SMALL_STRING_HPP
#define GREGJM_SMALL_STRING_SMALL_STRING_HPP

#include <cstddef> // size_t
#include <string> // char_traits
#include <memory> // allocator
#include <iterator> // reverse_iterator, prev
#include <algorithm> // min
#include <stdexcept> // out_of_range
#include <new> // bad_alloc

namespace gregjm {

// SmallString class: a string class, but even more optimized for size
// if you have SSE or NEON, this will be pretty zippy
// relies on char being 8 bits wide
template <typename Traits = std::char_traits<char>,
          typename Allocator = std::allocator<char>>
class SmallString {
public:
	using traits_type = Traits;
	using value_type = char;
	using allocator_type = Allocator;
	using size_type = typename std::allocator_traits<allocator_type>::size_type;
	using difference_type =
		typename std::allocator_traits<allocator_type>::difference_type;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = typename std::allocator_traits<allocator_type>::pointer;
	using const_pointer =
		typename std::allocator_traits<allocator_type>::const_pointer;
	using iterator = char*;
	using const_iterator = const char*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	SmallString() noexcept = default;

	SmallString(const char *const string) {
		const auto string_length = traits_type::length(string);

		reserve(string_length);
		traits_type::copy(data(), string, string_length);

		if (data_) {
			size_.size = string_length;
		}
	}

	~SmallString() {
		clear();
	}

	auto operator=(const char *const string) -> SmallString& {
		const auto string_length = traits_type::length(string);

		reserve(string_length);
		traits_type::copy(data(), string, string_length);

		if (data_) {
			size_.size = string_length;
		}

		return *this;
	}

	auto at(const size_type index) -> reference {
		bounds_check(index);
		return (*this)[index];
	}

	auto at(const size_type index) const -> const_reference {
		bounds_check(index);
		return (*this)[index];
	}

	auto operator[](const size_type index) noexcept -> reference {
		return data()[index];
	}

	auto operator[](const size_type index) const noexcept -> const_reference {
		return data()[index];
	}

	auto front() noexcept -> reference {
		return *begin();
	}

	auto front() const noexcept -> const_reference {
		return *cbegin();
	}

	auto back() noexcept -> reference {
		return *std::prev(end());
	}

	auto back() const noexcept -> const_reference {
		return *std::prev(cend());
	}

	auto data() noexcept -> pointer {
		if (not data_) {
			return small_data_;
		}

		return data_;
	}

	auto data() const noexcept -> const_pointer {
		if (not data_) {
			return small_data_;
		}

		return data_;
	}

	auto c_str() const noexcept -> const char* {
		if (not data()) {
			return nullptr;
		}

		return data();
	}

	auto begin() noexcept -> iterator {
		return data();
	}

	auto begin() const noexcept -> const_iterator {
		return cbegin();
	}

	auto cbegin() const noexcept -> const_iterator {
		return data();
	}

	auto end() noexcept -> iterator {
		return data() + size();
	}

	auto end() const noexcept -> const_iterator {
		return cend();
	}

	auto cend() const noexcept -> const_iterator {
		return data() + size();
	}

	auto rbegin() noexcept -> reverse_iterator {
		return { end() };
	}

	auto rbegin() const noexcept -> const_reverse_iterator {
		return crbegin();
	}

	auto crbegin() const noexcept -> const_reverse_iterator {
		return { cend() };
	}

	auto rend() noexcept -> reverse_iterator {
		return { begin() };
	}

	auto rend() const noexcept -> const_reverse_iterator {
		return crend();
	}

	auto crend() const noexcept -> const_reverse_iterator {
		return { cbegin() };
	}

	auto empty() const noexcept -> bool {
		return size() == 0;
	}

	auto size() const noexcept -> size_type {
		if (not data_) {
			return small_size();
		}

		return size_.size;
	}

	auto length() const noexcept -> size_type {
		return size();
	}

	auto max_size() const noexcept -> size_type {
		return std::allocator_traits<allocator_type>::max_size(alloc_);
	}

	auto reserve(const size_type new_capacity = 0) -> void {
		if (new_capacity <= capacity()) {
			return;
		}

		if (new_capacity > SMALL_CAPACITY) {
			const auto new_data = AllocTraits_t::allocate(alloc_,
			                                              new_capacity + 1);

			if (not new_data) {
				throw std::bad_alloc{ };
			}

			const auto current_size = size();

			traits_type::copy(new_data, begin(), current_size);
			deallocate();
			data_ = new_data;
			data_[current_size] = '\0';

			size_.size = current_size;
			size_.capacity = new_capacity;
		} else if (not data_) {
			small_data_[new_capacity] = '\0';
		}
	}

	auto capacity() const noexcept -> size_type {
		if (not data_) {
			return SMALL_CAPACITY - 1;
		}

		return size_.capacity;
	}

	auto clear() -> void {
		size_.size = 0;
		size_.capacity = 0;

		deallocate();
	}

	template <typename Allocator2>
	auto compare(const SmallString<Traits, Allocator2> &other) const -> int {
		const auto rlen = std::min(size(), other.size());

		const auto compared = traits_type::compare(data(), other.data(), rlen);

		if (compared == 0) {
			return static_cast<int>(size() - other.size());
		}

		return compared;
	}

	auto compare(const char *const other) const -> int {
		const auto string_length = traits_type::length(other);
		const auto rlen = std::min(size(), string_length);

		const auto compared = traits_type::compare(data(), other, rlen);

		if (compared == 0) {
			return static_cast<int>(size() - string_length);
		}

		return compared;
	}

	auto resize(const size_type new_size,
	            const value_type character = '\0') -> void
	{
		if (data_) {
			if (new_size > size_.size) {
				reserve(new_size);

				for (auto i = end() + 1; i < begin() + new_size; ++i) {
					AllocTraits_t::construct(alloc_, i, character);
				}

				data_[new_size] = '\0';
				size_.size = new_size;

				return;
			}

			for (auto i = begin() + new_size + 1; i < end(); ++i) {
				AllocTraits_t::destroy(alloc_, i);
			}

			data_[new_size] = '\0';
			size_.size = new_size;

			return;
		}

		if (new_size > small_size()) {
			reserve(new_size);

			for (auto i = end(); i != begin() + new_size; ++i) {
				AllocTraits_t::construct(alloc_, i, character);
			}

			if (new_size > SMALL_CAPACITY) {
				data_[new_size] = '\0';
				size_.size = new_size;
			} else {
				small_data_[new_size] = '\0';
			}

			return;
		}

		small_data_[new_size] = '\0';
	}

private:
	using AllocTraits_t = std::allocator_traits<allocator_type>;

	auto bounds_check(const size_type index) const -> void {
		if (index >= size()) {
			throw std::out_of_range{ "SmallString::bounds_check" };
		}
	}

	// invariant: class is in a small state
	auto small_size() const noexcept -> size_type {
		return traits_type::length(small_data_);
	}

	auto deallocate() -> void {
		if (data_) {
			std::allocator_traits<allocator_type>::destroy(alloc_, data_);
			data_ = nullptr;
		}
	}

	static constexpr auto SMALL_CAPACITY = 2 * sizeof(size_type);

	pointer data_ = nullptr;

	struct SizePair {
		size_type size = 0;
		size_type capacity = 0;
	};

	union {
		SizePair size_{ 0, 0 };
		char small_data_[SMALL_CAPACITY];
	};

	allocator_type alloc_;
};

template <typename Traits, typename Allocator1, typename Allocator2>
auto operator==(const SmallString<Traits, Allocator1> &lhs,
                const SmallString<Traits, Allocator2> &rhs) -> bool
{
	return lhs.compare(rhs) == 0;
}

template <typename Traits, typename Allocator>
auto operator==(const SmallString<Traits, Allocator> &lhs,
                const char *const rhs) -> bool
{
	return lhs.compare(rhs) == 0;
}

template <typename Traits, typename Allocator>
auto operator==(const char *const lhs,
                const SmallString<Traits, Allocator> &rhs) -> bool
{
	return rhs.compare(lhs) == 0;
}

template <typename Traits, typename Allocator1, typename Allocator2>
auto operator!=(const SmallString<Traits, Allocator1> &lhs,
                const SmallString<Traits, Allocator2> &rhs) -> bool
{
	return not (lhs == rhs);
}

template <typename Traits, typename Allocator>
auto operator!=(const SmallString<Traits, Allocator> &lhs,
                const char *const rhs) -> bool
{
	return lhs.compare(rhs) != 0;
}

template <typename Traits, typename Allocator>
auto operator!=(const char *const lhs,
                const SmallString<Traits, Allocator> &rhs) -> bool
{
	return rhs.compare(lhs) != 0;
}

template <typename Traits, typename Allocator1, typename Allocator2>
auto operator<(const SmallString<Traits, Allocator1> &lhs,
               const SmallString<Traits, Allocator2> &rhs) -> bool
{
	return lhs.compare(rhs) < 0;
}

template <typename Traits, typename Allocator>
auto operator<(const SmallString<Traits, Allocator> &lhs,
               const char *const rhs) -> bool
{
	return lhs.compare(rhs) < 0;
}

template <typename Traits, typename Allocator>
auto operator<(const char *const lhs,
               const SmallString<Traits, Allocator> &rhs) -> bool
{
	return rhs.compare(lhs) > 0;
}

template <typename Traits, typename Allocator1, typename Allocator2>
auto operator<=(const SmallString<Traits, Allocator1> &lhs,
                const SmallString<Traits, Allocator2> &rhs) -> bool
{
	return not (rhs < lhs);
}

template <typename Traits, typename Allocator>
auto operator<=(const SmallString<Traits, Allocator> &lhs,
                const char *const rhs) -> bool
{
	return not (rhs < lhs);
}

template <typename Traits, typename Allocator>
auto operator<=(const char *const lhs,
                const SmallString<Traits, Allocator> &rhs) -> bool
{
	return not (rhs < lhs);
}

template <typename Traits, typename Allocator1, typename Allocator2>
auto operator>(const SmallString<Traits, Allocator1> &lhs,
               const SmallString<Traits, Allocator2> &rhs) -> bool
{
	return rhs < lhs;
}

template <typename Traits, typename Allocator>
auto operator>(const SmallString<Traits, Allocator> &lhs,
               const char *const rhs) -> bool
{
	return rhs < lhs;
}

template <typename Traits, typename Allocator>
auto operator>(const char *const lhs,
               const SmallString<Traits, Allocator> &rhs) -> bool
{
	return rhs < lhs;
}

template <typename Traits, typename Allocator1, typename Allocator2>
auto operator>=(const SmallString<Traits, Allocator1> &lhs,
                const SmallString<Traits, Allocator2> &rhs) -> bool
{
	return not (lhs < rhs);
}

template <typename Traits, typename Allocator>
auto operator>=(const SmallString<Traits, Allocator> &lhs,
                const char *const rhs) -> bool
{
	return not (lhs < rhs);
}

template <typename Traits, typename Allocator>
auto operator>=(const char *const lhs,
                const SmallString<Traits, Allocator> &rhs) -> bool
{
	return not (lhs < rhs);
}

} // namespace gregjm

#endif
