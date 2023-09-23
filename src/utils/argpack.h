#pragma once

#include <cstddef>
#include <utility>
#include <type_traits>

template <typename... T>
struct argpack;

template <std::size_t I, typename Head, typename... Tail>
struct argpack_element {
	using type = argpack_element<I - 1, Tail...>::type;
};
template <typename Head, typename... Tail>
struct argpack_element<0, Head, Tail...> {
	using type = Head;
};

template <typename T, typename Head, typename... Tail>
struct argpack_index {
	static constexpr std::size_t value = 1 + argpack_index<T, Tail...>::value;
};
template <typename Head, typename... Tail>
struct argpack_index<Head, Head, Tail...> {
	static constexpr std::size_t value = 0;
};

template <typename T, typename U>
struct argpack_join;
template <typename... T, typename... U>
struct argpack_join<argpack<T...>, argpack<U...>> {
	using type = argpack<T..., U...>;
};

template <template <typename> typename Filter, typename Head, typename... Tail>
struct argpack_filter {
	using type = std::conditional_t<
		Filter<Head>::value,
		typename argpack_join<argpack<Head>, typename argpack_filter<Filter, Tail...>::type>::type,
		typename argpack_filter<Filter, Tail...>::type
	>;
};
template <template <typename> typename Filter, typename Head>
struct argpack_filter<Filter, Head> {
	using type = std::conditional_t<
		Filter<Head>::value,
		argpack<Head>,
		argpack<>
	>;
};

template <template <typename> typename Pred, typename Head, typename... Tail>
struct argpack_select {
	using type = std::conditional_t<
		Pred<Head>::value,
		Head,
		typename argpack_select<Pred, Tail...>::type
	>;
};
template <template <typename> typename Pred, typename Head>
struct argpack_select<Pred, Head> {
	using type = std::conditional_t<
		Pred<Head>::value,
		Head,
		void
	>;
};

template <typename... T>
struct argpack {

	static constexpr size_t size = sizeof...(T);

	template <std::size_t I>
	using element = argpack_element<I, T...>::type;

	template <typename U>
	static constexpr std::size_t index = argpack_index<U, T...>::value;

	static constexpr auto index_sequence = std::index_sequence_for<T...>{};

	template <typename... U>
	using add = argpack<T..., U...>;

	template <typename Other>
	using join = argpack_join<argpack<T...>, Other>::type;

	template <template <typename> typename Filter>
	using filter = argpack_filter<Filter, T...>::type;

	template <template <typename> typename Pred>
	using select = argpack_select<Pred, T...>::type;
};

