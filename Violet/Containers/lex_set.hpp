#ifndef LEX_SET_HPP
#define LEX_SET_HPP

#include "l_bag.hpp"
#include "WrappedIterator.hpp"
#include <tuple>

template<class... Contents> struct lex_set_impl;

template <class Last>
struct lex_set_impl<Last>
{
    using data_t = l_bag<Last>;
    using perma_ref_t = typename data_t::perma_ref;
    using local_it_t = typename data_t::iterator;
    data_t data;
};

template<class First, class... Rest>
struct lex_set_impl<First, Rest...>
{
    using rest_t = lex_set_impl<Rest...>;
    using data_t = l_bag<std::pair<First, typename rest_t::perma_ref_t>>;
    using perma_ref_t = typename data_t::perma_ref;
    using local_it_t = typename data_t::iterator;
    using lower_it_t = typename rest_t::local_it_t;
    
    rest_t rest;
    data_t data;
    
    //return the past-the-end of lower elements that belong to it
    lower_it_t last_of(local_it_t it)
    {
        return it + 1 >= rest.data.end() ? rest.data.end() : it[1].second;
    }
    
    //return the range of lower elements that belong to it
    std::pair<lower_it_t, lower_it_t> range_of(local_it_t it)
    {
        return {it == data.end() ? rest.data.end() : rest.data.get(it[0].second),
            last_of(it)};
    }
    
    perma_ref_t insert_first(local_it_t pos, const First& firstins, const Rest&... restins)
    {
        return data.emplace(pos, firstins, rest.insert_first(last_of(pos), restins...));
    }
    
    void insert(std::pair<local_it_t, local_it_t> range, const First& firstins, const Rest&... restins)
    {
        local_it_t found = std::find(range.first, range.second, firstins);
        if (found == range.second)
            insert_first(found, firstins, restins...);
        else
            rest.insert(range_of(found), restins...);
    }
    
    class iterator
    {
        local_it_t it;
        typename rest_t::iterator restit;
    };
};

//A lexicographic set. Stores its contents grouped left to right, with minumum copies of
//each element
//compares them to 
template <class... Contents>
class lex_set {
    lex_set_impl<Contents...> contents;
};

#endif