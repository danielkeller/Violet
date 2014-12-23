#ifndef PERMAVECTOR_HPP
#define PERMAVECTOR_HPP
#include <vector>
#include <WrappedIterator.hpp>

static const size_t INVALID_IND = -1;

//Vector with iterator-like objects that can't be invalidated
template<class T, class Alloc = std::allocator<T>>
class Permavector
{
public:
	using value_type = T;
private:
	using storety = std::vector<value_type, Alloc>;
    using indty = size_t;
    using indsty = std::vector<indty>;
public:

	Permavector() = default;
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

	using size_type = typename storety::size_type;
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
		perma_ref(size_t it) : it(it) {}
		size_t it;
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

	template<class... Args>
	perma_ref emplace_back(Args&&... args)
	{
        return emplace(end(), std::forward<Args>(args)...);
	}

    //this should be const_iterator but libstdc++ has a bug
	template<class... Args>
	perma_ref emplace(iterator pos, Args&&... args)
	{
        auto indIt = new_ind();
        inds[indIt] = pos - store.begin();
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
    size_t new_ind()
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
