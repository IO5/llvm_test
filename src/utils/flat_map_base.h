#pragma once

#include <vector>
#include <algorithm>
#include <ranges>
#include <functional>
#include <span>

template <typename T, auto Proj>
class flat_map_base {

public:
	using key = std::remove_cvref_t<std::invoke_result_t<decltype(Proj), T>>;
	static constexpr auto proj = Proj;

	constexpr flat_map_base() = default;

	// if there are equivalent elements it is undefined which one will be inserted
	template <std::ranges::range R> requires std::is_same_v<std::ranges::range_value_t<R>, T>
	constexpr flat_map_base(R&& v) {
		storage.assign(std::begin(v), std::end(v));
		std::ranges::sort(storage, {}, Proj);
		auto to_erase = std::ranges::unique(storage, {}, Proj);
		storage.erase(to_erase.begin(), to_erase.end());
	}

	constexpr flat_map_base(std::initializer_list<T> init) :
		flat_map_base(std::span(init)) {}

	constexpr auto begin(this auto& self) {
		return self.storage.begin();
	}

	constexpr auto end(this auto& self) {
		return self.storage.end();
	}

	constexpr std::size_t size() const {
		return storage.size();
	}

	constexpr bool empty() const {
		return storage.empty();
	}

	constexpr auto find(this auto& self, const key& key) {
		auto it = std::ranges::lower_bound(self.storage, key, {}, Proj);

		if (it != self.storage.end() && std::invoke(Proj, *it) != key)
			it = self.storage.end();

		return it;
	}

	constexpr bool contains(const key& key) const {
		return std::ranges::binary_search(storage, key, {}, Proj);
	}


	constexpr auto add(T item) {
		auto& key = std::invoke(Proj, item);
		auto it = std::ranges::lower_bound(storage, key, {}, Proj);
		if (it != storage.end() && std::invoke(Proj, *it) == key) {
			*it = item;
			return it;
		}
		return storage.insert(it, item);
	}

	template <std::ranges::range R>
	constexpr void add(const R& range) {
		storage.insert(storage.begin(), std::begin(range), std::end(range));
		std::ranges::sort(storage, {}, Proj);
		auto to_erase = std::ranges::unique(storage, {}, Proj);
		storage.erase(to_erase.begin(), to_erase.end());
	}

	constexpr void del(const key& key) {
		auto it = std::ranges::lower_bound(storage, key, {}, Proj);
		if (it != storage.end() && std::invoke(Proj, *it) == key)
			storage.erase(it);
	}

	constexpr auto operator<=>(const flat_map_base& other) const = default;

protected:
	std::vector<T> storage;
};
