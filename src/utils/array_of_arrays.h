#pragma once

#include <array>
#include <span>

template <typename T, std::size_t... Sizes>
struct array_of_arrays {

	constexpr auto operator[](this auto& self, std::size_t n) {
		return std::span(self.data() + offsets[n], self.data() + offsets[n + 1]);
	}

	template <std::size_t N>
	constexpr auto get() {
		static_assert(N <= sizeof...(Sizes));
		constexpr size_t size = offsets[N + 1] - offsets[N];
		return std::span<T, size>(data() + offsets[N], size);
	}

	template <std::size_t N>
	constexpr auto get() const {
		static_assert(N <= sizeof...(Sizes));
		constexpr size_t size = offsets[N + 1] - offsets[N];
		return std::span<const T, size>(data() + offsets[N], size);
	}

	constexpr auto* data(this auto& self) {
		return self.storage.data();
	}

	static constexpr std::size_t size() {
		return sizeof...(Sizes);
	}

	template <typename U>
	struct iterator_template {

		constexpr auto& operator++() {
			++offset;
			return *this;
		}

		constexpr auto& operator*() const {
			return *(data + *offset);
		}

		constexpr bool operator==(const iterator_template&) const = default;

		U* data;
		const std::size_t* offset;
	};

	using iterator = iterator_template<T>;
	using const_iterator = iterator_template<const T>;

	constexpr auto begin() {
		return iterator{ data(), offsets.data() };
	}

	constexpr auto begin() const {
		return const_iterator{ data(), offsets.data() };
	}

	constexpr auto end() {
		return iterator{ data(), offsets.data() + size() };
	}

	constexpr auto end() const {
		return const_iterator{ data(), offsets.data() + size() };
	}

private:
	template <std::size_t... Is>
	static constexpr std::size_t nth_offset(std::size_t n, std::index_sequence<Is...>) {
		return ((Is < n ? Sizes : 0) + ...);
	}

	template <std::size_t... Is>
	static constexpr auto make_offsets(std::index_sequence<Is...> seq) {
		return std::array<std::size_t, size() + 1>{
			nth_offset(Is, seq)...,
			nth_offset(sizeof...(Is), seq)
		};
	}

	static constexpr auto offsets = make_offsets(std::make_index_sequence<sizeof...(Sizes)>{});
	
	std::array<T, (Sizes + ...)> storage;
};
