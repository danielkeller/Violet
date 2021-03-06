#ifndef UTILS_TEMPLATE_HPP
#define UTILS_TEMPLATE_HPP

#include <algorithm>

template<class X, class... XS>
std::tuple<X, XS...> tuple_cons(X x, const std::tuple<XS...>& t)
{
	return std::tuple_cat(std::make_tuple(x), t);
}

template<class X, class Tuple>
using tuple_cons_t = decltype(tuple_cons(std::declval<X>(), std::declval<Tuple>()));

template <typename T, T... ints>
struct integer_sequence
{ };

// using aliases for cleaner syntax
template<class T> using Invoke = typename T::type;

template<size_t...> struct seq{ using type = seq; };

template<class S1, class S2> struct concat;

template<size_t... I1, size_t... I2>
struct concat<seq<I1...>, seq<I2...>>
	: seq<I1..., (sizeof...(I1)+I2)...>{};

template<class S1, class S2>
using Concat = Invoke<concat<S1, S2>>;

template<size_t N> struct gen_seq;
template<size_t N> using GenSeq = Invoke<gen_seq<N>>;

template<size_t N>
struct gen_seq : Concat<GenSeq<N / 2>, GenSeq<N - N / 2>>{};

template<> struct gen_seq<0> : seq<>{};
template<> struct gen_seq<1> : seq<0>{};

template<class T>
struct FromBytes
{
	T operator()(const char* ptr)
	{
		static_assert(std::is_standard_layout<T>::value, "Type must be standard layout");
		T ret;
		std::memcpy(&ret, ptr, sizeof(ret));
		return ret;
	}
};

template<class Scalar, int Rows, int Cols>
struct FromBytes<Eigen::Matrix<Scalar, Rows, Cols>>
{
	Eigen::Matrix<Scalar, Rows, Cols> operator()(const char* ptr)
	{
		return Eigen::Map<const Eigen::Matrix<Scalar, Rows, Cols>>
			(reinterpret_cast<const Scalar*>(ptr));
	}
};

template<typename F, typename... Args, size_t... inds>
std::result_of_t<F(Args...)> invokeImpl(F f, std::tuple<Args...> args, seq<inds...>)
{
	return f(std::forward<Args>(std::get<inds>(args))...);
}

template<typename F, typename... Args>
std::result_of_t<F(Args...)> invoke(F f, std::tuple<Args...> args)
{
	return invokeImpl(f, args, gen_seq<sizeof...(Args)>{});
}

inline std::size_t hash_combine()
{
	return 0;
}

template <class T, class... Args>
inline std::size_t hash_combine(const T& first, Args... args)
{
    size_t h = std::hash<T>()(first);
    return h ^ hash_combine(args...) + 0x9e3779b9 + (h << 6) + (h >> 2);
}

template<class... Types>
struct std::hash<std::tuple<Types...>>
{
	size_t operator()(const std::tuple<Types...>& tup)
	{
		return invoke(hash_combine, tup);
	}
};

template<class A, class B>
struct std::hash<std::pair<A, B>>
{
    size_t operator()(const std::pair<A, B>& pair)
    {
        return hash_combine(pair.first, pair.second);
    }
};

#endif