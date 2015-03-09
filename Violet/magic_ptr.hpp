#ifndef MAGIC_PTR_HPP
#define MAGIC_PTR_HPP

#include <memory>
#include <map>

//basically, a machine-word worth of something
using key_ty = std::aligned_storage_t<sizeof(nullptr)>;

namespace magic_detail
{
	template<class T>
	struct acc_base
	{
		virtual void setter(key_ty key, const T& val) = 0;
		virtual T getter(key_ty key) = 0;
		virtual ~acc_base() {}
	};

	template<class T>
	struct null_acc_derived : acc_base<T>
	{
		void setter(key_ty key, const T& val) {}
		T getter(key_ty key) { assert(false && "Reading null magic_ptr"); return{}; }
		static std::shared_ptr<null_acc_derived> instance;
	};

	template<class T, class Getter, class Setter>
	struct acc_derived : acc_base<T>
	{
		Getter myGetter;
		Setter mySetter;
		acc_derived(Getter g, Setter s)
			: myGetter(g), mySetter(s)
		{}

		void setter(key_ty key, const T& val)
		{
			mySetter(val);
		}

		T getter(key_ty key)
		{
			return myGetter();
		}
	};

	template<class T, class Getter, class Setter, class Key>
	struct keyed_acc_derived : acc_base<T>
	{
		Getter myGetter;
		Setter mySetter;
		keyed_acc_derived(Getter g, Setter s)
			: myGetter(g), mySetter(s)
		{}

		void setter(key_ty key, const T& val)
		{
			mySetter(*reinterpret_cast<const Key*>(&key), val);
		}

		T getter(key_ty key)
		{
			return myGetter(*reinterpret_cast<const Key*>(&key));
		}
	};

	template<class T, class Getter, class Setter, class Class>
	struct member_acc_derived : acc_base<T>
	{
		Getter myGetter;
		Setter mySetter;
		member_acc_derived(Getter g, Setter s)
			: myGetter(g), mySetter(s)
		{}

		void setter(key_ty key, const T& val)
		{
			((*reinterpret_cast<Class*>(&key))->*mySetter)(val);
		}

		T getter(key_ty key)
		{
			return ((*reinterpret_cast<Class*>(&key))->*myGetter)();
		}
	};
};
template<class T, class Key = key_ty>
class accessor
{
	template<class T1, class Key1>
	friend class accessor;

	std::shared_ptr<magic_detail::acc_base<T>> watcher;

public:
	//null magic pointers are valid and do nothing
	accessor()
		: watcher(magic_detail::null_acc_derived<T>::instance)
	{}
	
	template<class Getter, class Setter,
		typename = decltype(std::declval<Getter>()()),
		typename = decltype(std::declval<Setter>()(std::declval<T>()))>
		accessor(Getter g, Setter s)
		: watcher(std::make_shared<magic_detail::acc_derived<T, Getter, Setter>>(g, s))
	{}
	
	template<class Getter, class Setter,
		typename = decltype(std::declval<Getter>()(std::declval<Key>())),
		typename = decltype(std::declval<Setter>()(std::declval<Key>(), std::declval<T>()))>
		accessor(Getter g, Setter s, int = 0)
		: watcher(std::make_shared<magic_detail::keyed_acc_derived<T, Getter, Setter, Key>>(g, s))
	{}

	template<class Getter, class Setter,
		typename = decltype((std::declval<Key>()->*std::declval<Getter>())()),
		typename = decltype((std::declval<Key>()->*std::declval<Setter>())(std::declval<T>()))>
		accessor(Getter g, Setter s, int = 0, int = 0)
		: watcher(std::make_shared<magic_detail::member_acc_derived<T, Getter, Setter, Key>>(g, s))
	{}

	//erase the key type
	operator accessor<T>()
	{
		accessor<T> ret;
		ret.watcher = watcher;
		return ret;
	}

	explicit operator bool()
	{
		return watcher != magic_detail::null_acc_derived<T>::instance;
	}

	bool operator==(const accessor<T, Key>& other) const
	{
		return watcher == other.watcher;
	}

	bool operator<(const accessor<T, Key>& other) const
	{
		return watcher < other.watcher;
	}

	void set(key_ty key, const T& val)
	{
		watcher->setter(key, val);
	}

	T get(key_ty key) const
	{
		return watcher->getter(key);
	}
};

template<class T>
class magic_ptr
{
	key_ty key;
	accessor<T> acc;

	//lets us use operator-> without returning the address of a stack variable,
	//even when getter() returns a value.
	struct arrow_helper
	{
		//msvc is broken
		arrow_helper(magic_ptr& owner, const T& temp)
			: temp(temp), owner(owner)
#ifndef NDEBUG
			, orig(temp)
#endif
		{}
		T temp;
		magic_ptr& owner;
#ifndef NDEBUG
		T orig;
#endif
		T* operator->() { return &temp; }
		~arrow_helper()
		{
			assert(orig == *owner &&
				"magic_ptr::operator-> used more than once in an expression!");
			owner.set(temp);
		}
	};

	struct const_arrow_helper
	{
		T temp;
		const T* operator->() { return &temp; }
	};

public:
	magic_ptr()
	{
		//clear out the key
		new (&key) nullptr_t();
	}

	magic_ptr(accessor<T> acc)
		: acc(acc)
	{}

	template<typename Key>
	magic_ptr(accessor<T, Key> acc, const Key& k)
		: acc(acc)
	{
		new (&key) nullptr_t();
		static_assert(sizeof(k) <= sizeof(key),// && __alignof(k) <= __alignof(key),
			"Key type does not fit size and alignment requirements");
		static_assert(std::is_trivially_copyable<Key>::value
			&& std::is_trivially_destructible<Key>::value,
			"Key type is not trivially copyable and destructable");
		new(&key) Key(k);
	}

	//converting magic_ptr
	template<class U, typename = typename std::enable_if_t<
		std::is_convertible<T, U>::value && std::is_convertible<U, T>::value >>
		magic_ptr(magic_ptr<U> other)
		: accessor([other]() -> T { return *other; },
		[other](const T& v) mutable { *other = v; })
	{}

	explicit operator bool()
	{
		return bool(acc);
	}

	//combine magic_ptrs. only calls getter of lhs
	magic_ptr operator+(magic_ptr other)
	{
		//optimize if one is null
		if (!other)
			return *this;
		if (!*this)
			return other;

		if (std::memcmp(&key, &other.key, sizeof(key)) == 0)
		{
			auto thisacc = acc;
			auto otheracc = other.acc;
			//avoid creating redundant accessors
			static std::map<std::pair<accessor<T>, accessor<T>>, accessor<T>> cache;
			auto it = cache.find(std::make_pair(acc, otheracc));
			if (it == cache.end())
			{
				it = cache.emplace(std::make_pair(acc, otheracc), accessor<T> {
					[thisacc](const key_ty& k) { return thisacc.get(k); },
					[thisacc, otheracc](const key_ty& k, const T& val) mutable
					{
						thisacc.set(k, val);
						otheracc.set(k, val);
					}}).first;
			}
			return{ it->second, key };
		}

		auto thiscopy = *this;
		return{ accessor<T, int>{
			[thiscopy]() -> T {return *thiscopy; },
			[thiscopy, other](const T& v) mutable
			{
				thiscopy.set(v);
				other.set(v);
			}},
			int{ 0 }
		};
	}

	void operator+=(magic_ptr other)
	{
		*this = operator+(other);
	}

	magic_ptr& operator=(magic_ptr other)
	{
		std::swap(key, other.key);
		std::swap(acc, other.acc);
		return *this;
	}

	magic_ptr(const magic_ptr&) = default;

	void set(const T& val) { acc.set(key, val); }
	const T get() const { return acc.get(key); }

	//the const on the return type is a compromise; it prevents us from doing *ptr = foo,
	//which doesn't work, but it also prevents move semantics from working. it might make
	//sense to disable in release builds

	//returns a temporary value
	const T operator*() const { return acc.get(key); }
	const_arrow_helper operator->() const { return{ operator*() }; }

	//returns a special value that gives member access, and calls set() at the
	//end of the temporary's lifetime.
	arrow_helper operator->() { return{ *this, operator*() }; }
};

template<class T>
std::shared_ptr<magic_detail::null_acc_derived<T>>
magic_detail::null_acc_derived<T>::instance =
std::make_shared<magic_detail::null_acc_derived<T>>();

//deduce type from arguments
template<class T, class Key>
magic_ptr<T> make_magic(accessor<T, Key> acc, Key k)
{
	return{ acc, k };
}

#endif