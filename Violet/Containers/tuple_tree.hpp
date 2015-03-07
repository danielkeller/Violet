#ifndef TUPLE_TREE_HPP
#define TUPLE_TREE_HPP

#include "l_bag.hpp"
#include "WrappedIterator.hpp"

//todo: don't aligned allocate everything

//This is a less nice way to do it, but the other way crashes MSVC
//(even when it compiles fine in clang)
//#define RECUR_TT
//#define FREE_TT

//work around some bugs in the way MSVC expands templates
#ifdef _MSC_VER
namespace std
{
	template<class First, class... Conts>
	struct tuple_element<size_t(-1), tuple<First, Conts...>>
	{
		typedef First type;
	};

	template<size_t Index>
	struct tuple_element<Index, tuple<>>
	{
		typedef int type;
	};
}
#endif

namespace tt_detail
{
	template<class X, class... XS>
	std::tuple<X, XS...> tuple_cons(X x, const std::tuple<XS...>& t)
	{
		return std::tuple_cat(std::make_tuple(x), t);
	}

	template<class X, class Tuple>
	using tuple_cons_t = decltype(tuple_cons(std::declval<X>(), std::declval<Tuple>()));

#if defined(FREE_TT)

	template<class... Conts>
	struct tuple_tree_level;

	template<class T>
	struct tuple_tree_level<T>
	{
		using data_t = l_bag<T, Eigen::aligned_allocator<T>>;
		data_t data;
	};

	template<class T, class... Rest>
	struct tuple_tree_level<T, Rest...> : public tuple_tree_level<Rest...>
	{
		using this_t = tuple_tree_level<T, Rest...>;
		using rest_t = tuple_tree_level<Rest...>;

		using value_t = std::pair<T, typename rest_t::perma_ref_t>;
		using data_t = l_bag<value_t, Eigen::aligned_allocator<value_t>>;
		data_t data;
	};

#elif defined(RECUR_TT)
	template<class T, class... Rest>
	struct tuple_tree_level : public tuple_tree_level<Rest...>
	{
		using rest_t = tuple_tree_level<Rest...>;

		using value_t = std::pair<T, typename rest_t::perma_ref_t>;
		/*struct value_t
		{
			T val;
			typename rest_t::perma_ref_t ref;
			template<class Arg>
			value_t(Arg&& a, typename rest_t::perma_ref_t ref_)
				: val(std::forward<Arg>(a)), ref(ref_) {}
			value_t(value_t&& other)
				: val(std::move(other.val)), ref(other.ref) {}
			value_t& operator=(value_t other)
			{
				swap(val, other.val);
				swap(ref, other.ref);
				return *this;
			}
		};*/

		using data_t = l_bag<value_t, Eigen::aligned_allocator<value_t>>;
		using perma_ref_t = typename data_t::perma_ref;
		using perma_refs_t = tuple_cons_t<perma_ref_t, typename rest_t::perma_refs_t>;
		using iter_t = typename data_t::iterator;
		using range_t = range<iter_t>;

		static const int bottom = sizeof...(Rest);

		data_t data;
		//rest_t rest;

		template<class Arg, class... Args>
		perma_refs_t emplace(range_t range, Arg&& a, Args&&... args)
		{
			//might just be able to change this call to lower_bound
			auto it = std::find_if(range.begin(), range.end(),
				[&](const value_t& val) {return val.first == a;});
			auto refs = rest_t::emplace(range_of(it), std::forward<Args>(args)...);

			if (it == range.end())
				return tuple_cons(data.emplace(it, std::forward<Arg>(a), std::get<0>(refs)), refs);
			else
				return tuple_cons(data.get_perma(it), refs);
			return tuple_cons(data.get_perma(data.begin()), refs);
		}

		typename rest_t::range_t range_of(typename data_t::iterator it)
		{
			if (it == data.end())
				return{ rest_t::data.end(), rest_t::data.end() };
			else if (it + 1 == data.end())
				return{ rest_t::data.find(it[0].second), rest_t::data.end() };
			else
				return{ rest_t::data.find(it[0].second), rest_t::data.find(it[1].second) };
		}

		template<int Level>
		struct level : public rest_t::template level<Level - 1> {};

		template<>
		struct level<0>
		{
			using type = tuple_tree_level<T, Rest...>;
		};

		template<int Level>
		using level_t = typename level<Level>::type;

		template<int Level>
		level_t<Level>& get_level()
		{
			return rest_t::template get_level<Level - 1>();
		}

		template<>
		level_t<0>& get_level<0>() { return *this; }
	};

	template<class T>
	struct tuple_tree_level<T>
	{
		using data_t = l_bag<T, Eigen::aligned_allocator<T>>;
#ifdef NO_HUGE_ERRORS
		struct perma_ref_t
		{
			typename data_t::perma_ref ref;
			perma_ref_t(typename data_t::perma_ref ref_) : ref(ref_) {}
			operator typename data_t::perma_ref() { return ref; }
		};
#else
		using perma_ref_t = typename data_t::perma_ref;
#endif
		using iter_t = typename data_t::iterator;
		using range_t = range<iter_t>;
		using perma_refs_t = std::tuple<perma_ref_t>;

		data_t data;

		template<class Arg>
		perma_refs_t emplace(range_t range, Arg&& a)
		{
			auto it = std::find(range.begin(), range.end(), a);
			if (it == range.end())
				return perma_refs_t(data.emplace(it, std::forward<Arg>(a)));
			else
				return perma_refs_t(data.get_perma(it));
		}

		template<int Level>
		struct level
		{
			static_assert(Level == 0, "Invalid level requested");
			using type = tuple_tree_level<T>;
		};

		template<int Level>
		using level_t = typename level<Level>::type;

		template<int Level>
		level_t<Level>& get_level()
		{
			static_assert(false, "Gone past the bottom of the tree");
		}

		template<>
		level_t<0>& get_level<0>() { return *this; }
	};

#else

	template<class... Contents>
	struct conts;

	template<class First, class Next, class... Rest>
	struct conts<First, Next, Rest...>
	{
		using lower_conts = conts<Next, Rest...>;
		using lower_data_t = typename lower_conts::data_t;
		using value_t = std::pair<First, typename lower_data_t::perma_ref>;
		using data_t = l_bag<value_t>; //, Eigen::aligned_allocator<value_t>>;
		using tuple_t = tuple_cons_t<data_t, typename lower_conts::tuple_t>;
		using perma_refs_t = tuple_cons_t<
			typename data_t::perma_ref, typename lower_conts::perma_refs_t>;
		using emplace_ret_t = tuple_cons_t<perma_refs_t,
			typename lower_conts::emplace_ret_t>;
	};

	template<class Last>
	struct conts<Last>
	{
		using data_t = l_bag<Last>; //, Eigen::aligned_allocator<Last>>;
		using tuple_t = std::tuple<data_t>;
		using perma_refs_t = std::tuple<typename data_t::perma_ref>;
		using emplace_ret_t = std::tuple<perma_refs_t>;
	};
#endif
}

#if defined(FREE_TT)

#elif defined(RECUR_TT)
template<class... Contents>
struct tuple_tree
{
	using data_t = tt_detail::tuple_tree_level<Contents...>;
	data_t data;

	template<int Level>
	using level_t = typename data_t::level<Level>::type;

public:
	using perma_refs_t = typename data_t::perma_refs_t;
	template<int Level>
	using level_data_t = typename data_t::level<Level>::type::data_t;
	template<int Level>
	using iter_t = typename data_t::level<Level>::type::data_t::iterator;
	template<int Level>
	using perma_ref_t = typename level_data_t<Level>::perma_ref;
	template<int Level>
	using range_t = range<iter_t<Level>>;
	template<int Level>
	using value_t = typename level_data_t<Level>::value_t;

	iter_t<0> begin()
	{
		return data.data.begin();
	}

	iter_t<0> end()
	{
		return data.data.end();
	}

	template<int Level, class PR>
	iter_t<Level> find(PR pr)
		//decltype(get_level<Level>().find(pr))
	{
		return get_level<Level>().find(pr);
	}

	//regular return type position causes compiler crash
	template<int Level>
	auto /*level_data_t<Level>&*/ get_level()
		-> decltype(data.template get_level<Level>().data)&
	{
		return data.template get_level<Level>().data;
	}

	template<class... Args>
	perma_refs_t emplace(Args&&... args)
	{
		return data.emplace({ begin(), end() }, std::forward<Args>(args)...);
	}
};

#else

//type that stores tuple-like elements sorted lexicographically, but keeping
//each level linear in memory, and minimizing duplicate items
template<class... Contents>
struct tuple_tree
{
	using conts_t = tt_detail::conts<Contents...>;
	using data_t = typename conts_t::tuple_t;
	data_t data;

	static const size_t bottom = sizeof...(Contents)-1;
	template<size_t Level>
	using emplace_ret_t = typename std::tuple_element<
		Level, typename conts_t::emplace_ret_t>::type;

public:
	using perma_refs_t = typename conts_t::perma_refs_t;
	template<size_t Level>
	using level_data_t = typename std::tuple_element<Level, data_t>::type;
	template<size_t Level>
	using iter_t = typename std::tuple_element<Level, data_t>::type::iterator;
	template<size_t Level>
	using perma_ref_t = typename std::tuple_element<Level, data_t>::type::perma_ref;
	template<size_t Level>
	using range_t = range<iter_t<Level>>;
	template<size_t Level>
	using value_t = typename std::tuple_element<Level, data_t>::type::value_type;

	iter_t<0> begin()
	{
		return std::get<0>(data).begin();
	}

	iter_t<0> end()
	{
		return std::get<0>(data).end();
	}

	template<size_t Level>
	iter_t<Level> find(perma_ref_t<Level> pr)
	{
		return std::get<Level>(data).find(pr);
	}

	template<size_t Level>
	level_data_t<Level>& get_level()
	{
		return std::get<Level>(data);
	}

	template<class... Args>
	perma_refs_t emplace(Args&&... args)
	{
		return emplace<0>({begin(), end()}, std::forward<Args>(args)...);
	}
	
	template<size_t Level>
	range_t<Level> children(const value_t<Level - 1>& node)
	{
		static_assert(Level > 0, "No children at this level");
		static_assert(Level <= bottom, "Past the bottom of the tree");
		auto& dat = std::get<Level - 1>(data);
		auto it = dat.begin() + (&node - &*dat.begin());
		return range_of<Level - 1>(it);
	}

private:
	template<size_t Level, class Arg, class... Args, typename = std::enable_if_t<Level != bottom>>
	emplace_ret_t<Level> emplace(range_t<Level> range, Arg&& a, Args&&... args)
	{
		auto& level = std::get<Level>(data);
		//might just be able to change this call to lower_bound
		auto it = std::find_if(range.begin(), range.end(),
			[&](const value_t<Level>& val) {return val.first == a; });
		auto refs = emplace<Level + 1>(range_of<Level>(it), std::forward<Args>(args)...);

		if (it == range.end())
			return tt_detail::tuple_cons(
				level.emplace(it, std::forward<Arg>(a), std::get<0>(refs)),
				refs);
		else
			return tt_detail::tuple_cons(level.get_perma(it), refs);
	}

	template<size_t, class Arg>
	emplace_ret_t<bottom> emplace(range_t<bottom> range, Arg&& a)
	{
		auto& level = std::get<bottom>(data);
		auto it = std::find(range.begin(), range.end(), a);
		if (it == range.end())
			return std::make_tuple(level.emplace(it, std::forward<Arg>(a)));
		else
			return std::make_tuple(level.get_perma(it));
	}

	template<size_t Level>
	range_t<Level + 1> range_of(iter_t<Level> it)
	{
		auto& level = std::get<Level>(data);
		auto& below = std::get<Level + 1>(data);
		if (it == level.end())
			return{ below.end(), below.end() };
		else if (it + 1 == level.end())
			return{ below.find(it[0].second), below.end() };
		else
			return{ below.find(it[0].second), below.find(it[1].second) };
	}
};
#endif

#endif