#pragma once

#include <algorithm>
#include <utility>
#include <array>

// TODO
template <typename K, typename V, size_t N>
struct static_map {
	consteval static_map(const std::pair<K, V>(&init)[N]) {
		std::copy(init, init + N, data.data());
	}

	template <typename T>
	constexpr const V* try_get(const T& k) const {
		if (auto it = std::find_if(data.begin(), data.end(), [&](auto& e) { return e.first == k; }); it != data.end())
			return &it->second;
		return nullptr;
	}

	template <typename T>
	consteval const V& get(const T& k) const {
		return *try_get(k);
	}

	std::array<std::pair<K, V>, N> data;
};

template <typename K, typename V, size_t N>
consteval auto make_static_map(const std::pair<K, V>(&init)[N]) {
	return static_map<K, V, N>(init);
}

