#ifndef L_BAG_HPP
#define L_BAG_HPP
#include <vector>

template<class T, class DiffT>
class perma_ref
{
public:
	perma_ref(const perma_ref& other) = default;
	perma_ref(const perma_ref&& other) : it(other.it) {}
	perma_ref& operator=(const perma_ref& other) = default;
	BASIC_EQUALITY(perma_ref, it)

private:
	perma_ref(DiffT it) : it(it) {}
	DiffT it;

	template<class T, class Alloc>
	friend class l_bag;
};

//Vector with iterator-like objects that can't be invalidated
template<class T, class Alloc = std::allocator<T>>
class l_bag
{
public:
	using value_type = T;
	using size_type = typename std::vector<value_type, Alloc>::size_type;
	using difference_type = typename std::allocator_traits<Alloc>::difference_type;
private:
	using storety = std::vector<value_type, Alloc>;
    //trick for declaring integral constants in the header
	enum : difference_type { INVALID_IND = static_cast<difference_type>(-1) };
	using indty = difference_type;
    using indsty = std::vector<indty>;
public:

	l_bag()
		: inds(1, 0) //past-the-end
    {}
	l_bag(std::initializer_list<T> init)
	{
		for (auto& v : init)
			emplace_back(v);
	}
	l_bag(l_bag&& other)
		: store(std::move(other.store))
        , inds(std::move(other.inds))
	{}
	l_bag(const l_bag& other)
		: store(other.store)
        , inds(other.inds)
	{}

    l_bag& operator=(l_bag other)
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
    
    value_type& front() {return store.front();}
    value_type& back() {return store.back();}

	size_type size() const { return store.size(); }
	value_type* data() { return store.data(); }
	const value_type* data() const { return store.data(); }
    bool empty() const { return store.empty(); }
    const storety& vector() const {return store;}

	using perma_ref = perma_ref<T, difference_type>;

    iterator find(perma_ref r)
    {
        return store.begin() + inds[r.it];
    }

	const_iterator find(perma_ref r) const
    {
        return const_cast<l_bag*>(this)->get(r);
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
        indty my_ind = pos - store.begin();
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
    
    void resize(size_type count)
    {
        if (count == size())
            return;
        if (count < size())
        {
            store.resize(count);
			for (auto& ind : inds) if (ind >= count) ind = INVALID_IND;
        }
        else
        {
            auto oldCount = size();
            store.resize(count);
            inds.reserve(count);
            auto newBegin = store.begin() + oldCount;
            auto indIt = std::find(inds.begin(), inds.end(), INVALID_IND);
            
            //do this instead of new_ind because this is linear and that is quadratic
            //run out the existing ind spaces
            for (; indIt != inds.end() && newBegin != store.end();
                 indIt = std::find(indIt, inds.end(), INVALID_IND), ++newBegin)
                *indIt = newBegin - store.begin();
            
            //add new inds at the end
            for (; newBegin != store.end(); ++newBegin)
                inds.emplace_back(newBegin - store.begin());
        }
    }

    //does not guarantee order of other elements
    /*
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
    */

private:
    difference_type new_ind()
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
