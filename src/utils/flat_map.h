#pragma once

#include "flat_map_base.h"

template <typename K, typename V>
using flat_map_base_impl = flat_map_base<std::pair<K, V>, &std::pair<K, V>::first>;

template <typename K, typename V>
class flat_map : public flat_map_base_impl<K, V> {
public:
	using flat_map_base_impl<K, V>::flat_map_base;

	constexpr auto* get(this auto& self, const K& key) {
		auto it = std::ranges::lower_bound(self.storage, key, {}, self.proj);
		if (it != self.storage.end() && it->first == key)
			return &it->second;
		return (V*)nullptr;
	}
	
	constexpr auto& operator[](const K& key) const {

		return *find(key);
	}

	constexpr auto& operator[](const K& key) {

		auto it = this->find(key);
		if (it != this->end())
			it = this->add({ key, {} });
		return *it;
	}
};
