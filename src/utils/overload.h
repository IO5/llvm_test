#pragma once

template<typename... T>
struct overload : T ... { 
    using T::operator() ...;
};
template<typename... T> overload(T...) -> overload<T...>;

