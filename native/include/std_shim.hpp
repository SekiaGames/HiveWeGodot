#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <cstddef>

namespace std {
template <class Signature>
using move_only_function = function<Signature>;
}

namespace std::ranges {

template <class Container>
class enumerate_view_stub {
  public:
	explicit enumerate_view_stub(Container& container): container_(container) {}

	class iterator {
	  public:
		iterator(decltype(std::declval<Container&>().begin()) it, std::size_t index): it_(it), index_(index) {}

		auto operator*() const { return std::tuple<std::size_t, decltype(*it_)>(this->index_, *this->it_); }

		iterator& operator++() {
			++this->it_;
			++this->index_;
			return *this;
		}

		bool operator!=(const iterator& other) const { return this->it_ != other.it_; }

	  private:
		decltype(std::declval<Container&>().begin()) it_;
		std::size_t index_;
	};

	iterator begin() { return iterator(this->container_.begin(), 0); }
	iterator end() { return iterator(this->container_.end(), 0); }

  private:
	Container& container_;
};

template <class Container>
enumerate_view_stub<Container> enumerate_view(Container& container) {
	return enumerate_view_stub<Container>(container);
}

}

inline std::string operator+(const char* lhs, std::string_view rhs) {
	return std::string(lhs) + std::string(rhs);
}

inline std::string operator+(std::string_view lhs, const char* rhs) {
	return std::string(lhs) + std::string(rhs);
}
