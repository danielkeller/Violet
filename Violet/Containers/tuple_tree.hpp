#ifndef TUPLE_TREE_HPP
#define TUPLE_TREE_HPP

#include "l_bag.hpp"
#include "WrappedIterator.hpp"

//todo: don't aligned allocate everything

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
		using data_t = l_bag<Last, Eigen::aligned_allocator<Last>>;
		using tuple_t = std::tuple<data_t>;
		using perma_refs_t = std::tuple<typename data_t::perma_ref>;
		using emplace_ret_t = std::tuple<perma_refs_t>;
	};
}

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

	template<size_t Level>
	iter_t<Level> begin()
	{
		return std::get<Level>(data).begin();
	}

	template<size_t Level>
	iter_t<Level> end()
	{
		return std::get<Level>(data).end();
	}

	iter_t<0> begin() { return begin<0>(); }
	iter_t<0> end() { return end<0>(); }

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
			return tuple_cons(
				level.emplace(it, std::forward<Arg>(a), std::get<0>(refs)),
				refs);
		else
			return tuple_cons(level.get_perma(it), refs);
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


/*
This bug needs to be submitted to MS when their website starts working

struct test
{
using tup = std::tuple<std::vector<int>, std::vector<char>, std::vector<float>>;

template<int pos>
using tup_elem = typename std::tuple_element<pos, tup>::type;

template<int pos>
using tup_elem_elem = typename tup_elem<pos>::value_type;

template<int pos>
void bar(tup_elem_elem<pos> val)
{}
};

void foo()
{
test t;
t.bar<1>('c');
}
*/