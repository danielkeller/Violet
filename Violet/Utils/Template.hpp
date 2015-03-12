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

template<unsigned...> struct seq{ using type = seq; };

template<class S1, class S2> struct concat;

template<unsigned... I1, unsigned... I2>
struct concat<seq<I1...>, seq<I2...>>
	: seq<I1..., (sizeof...(I1)+I2)...>{};

template<class S1, class S2>
using Concat = Invoke<concat<S1, S2>>;

template<unsigned N> struct gen_seq;
template<unsigned N> using GenSeq = Invoke<gen_seq<N>>;

template<unsigned N>
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
		std::copy(ptr, ptr + sizeof(T), reinterpret_cast<char*>(&ret));
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

#endif