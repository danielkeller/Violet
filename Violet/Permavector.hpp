#ifndef PERMAVECTOR_HPP
#define PERMAVECTOR_HPP
#include <vector>
#include <WrappedIterator.hpp>

//Vector with iterator-like objects that can't be invalidated
template<class T, class Alloc = std::allocator<T>>
class Permavector
{
public:
	using value_type = T;
	using size_type = typename std::vector<value_type, Alloc>::size_type;
private:
	using storety = std::vector<value_type, Alloc>;
    //trick for declaring integral constants in the header
    enum : size_type { INVALID_IND = static_cast<size_type>(-1) };
    using indty = size_type;
    using indsty = std::vector<indty>;
public:

	Permavector()
        : inds({0}) //past-the-end
    {}
	Permavector(std::initializer_list<T> init)
	{
		for (auto& v : init)
			emplace_back(v);
	}
	Permavector(Permavector&& other)
		: store(std::move(other.store))
        , inds(std::move(other.inds))
	{}
	Permavector(const Permavector& other)
		: store(other.store)
        , inds(other.inds)
	{}

    Permavector& operator=(Permavector other)
    {
        swap(store, other.store);
        swap(inds, other.inds);
        return *this;
    }

	using iterator = typename storety::iterator;
	using const_iterator = typename storety::const_iterator;

	iterator begin() {return store.begin();}
	iterator end() {return store.end();}
	const_iterator begin() const {return store.cbegin();}
	const_iterator end() const {return store.cend();}
	const_iterator cbegin() const {return store.cbegin();}
	const_iterator cend() const {return store.cend();}

	size_type size() const { return store.size(); }
	value_type* data() { return store.data(); }
	const value_type* data() const { return store.data(); }

	class perma_ref
	{
	public:
		perma_ref(const perma_ref& other) : it(other.it) {}
		perma_ref& operator=(perma_ref other)
		{
			it = other.it;
			return *this;
		}

	private:
		perma_ref(typename indsty::difference_type it) : it(it) {}
		typename indsty::difference_type it;
		friend class Permavector;
	};

    iterator get(perma_ref r)
    {
        return store.begin() + inds[r.it];
    }

    const_iterator get(perma_ref r) const
    {
        return const_cast<Permavector*>(this)->get(r);
    }

    //linear time
    perma_ref get_perma(const_iterator pos) const
    {
        return perma_ref{
            std::find(inds.begin(), inds.end(), pos - store.begin())
            - inds.begin()};
    }

	template<class... Args>
	perma_ref emplace_back(Args&&... args)
	{
        auto indIt = new_ind();
        inds[indIt] = store.size();
		store.emplace_back(std::forward<Args>(args)...);
		return perma_ref{ indIt };
	}

    //this should be const_iterator but libstdc++ has a bug
	template<class... Args>
	perma_ref emplace(iterator pos, Args&&... args)
	{
        auto indIt = new_ind();
        auto my_ind = pos - store.begin();
        for(auto& ind : inds) if (ind >= my_ind) ++ind;
        inds[indIt] = my_ind;
		store.emplace(pos, std::forward<Args>(args)...);
		return perma_ref{ indIt };
	}

    iterator erase(const_iterator pos)
    {
        auto my_ind = pos - store.begin();
        for(auto& ind : inds)
        {
            if (ind > my_ind) --ind; //shift over ones to the right
            else if (ind == my_ind) ind = INVALID_IND;
        }
        return store.erase(pos);
    }

    //does not guarantee order of other elements
    iterator fast_erase(const_iterator pos)
    {
        auto my_ind = pos - store.begin();
        for(auto& ind : inds)
        {
            if (ind == my_ind) ind = INVALID_IND;
            else if (ind == store.size()-1) ind = my_ind;
        }
        swap(*pos, store.back());
        store.pop_back();
    }

private:
    typename indsty::difference_type new_ind()
    {
        auto indIt = std::find(inds.begin(), inds.end(), INVALID_IND);
        if (indIt == inds.end())
        {
            inds.emplace_back();
            indIt = inds.end() - 1;
        }
        return indIt - inds.begin();
    }

	storety store;
    indsty inds;
};

#endif
