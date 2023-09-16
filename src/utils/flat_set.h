#pragma once

#include "flat_map_base.h"

template <typename T>
class flat_set : public flat_map_base<T, std::identity{}> {
public:
	using flat_map_base<T, std::identity{}>::flat_map_base;
};

