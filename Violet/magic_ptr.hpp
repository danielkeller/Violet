#ifndef MAGIC_PTR_HPP
#define MAGIC_PTR_HPP

#include <memory>
#include <map>

//basically, a machine-word worth of something
using key_ty = std::aligned_storage_t<sizeof(nullptr)>;

template<class T>
class accessor_impl
{
protected:
	struct acc_base
	{
		virtual void setter(const key_ty& key, const T& val) = 0;
		virtual T getter(const key_ty& key) = 0;
		virtual ~acc_base() {}
	};

	struct null_acc_derived : acc_base
	{
		void setter(const key_ty& key, const T& val) {}
		T getter(const key_ty& key) { return{}; }
		static std::shared_ptr<null_acc_derived> instance;
	};

	template<class Getter, class Setter>
	struct acc_derived : acc_base
	{
		Getter myGetter;
		Setter mySetter;
		acc_derived(Getter g, Setter s)
			: myGetter(g), mySetter(s)
		{}

		void setter(const key_ty& key, const T& val)
		{
			mySetter(val);
		}

		T getter(const key_ty& key)
		{
			return myGetter();
		}
	};

	template<class Getter, class Setter, class Key>
	struct keyed_acc_derived : acc_base
	{
		Getter myGetter;
		Setter mySetter;
		keyed_acc_derived(Getter g, Setter s)
			: myGetter(g), mySetter(s)
		{}

		void setter(const key_ty& key, const T& val)
		{
			mySetter(*static_cast<const Key*>(static_cast<const void*>(&key)), val);
		}

		T getter(const key_ty& key)
		{
			return myGetter(*static_cast<const Key*>(static_cast<const void*>(&key)));
		}
	};

	accessor_impl(std::shared_ptr<acc_base> watcher)
		: watcher(watcher)
	{}

	//consider making this another special kind of pointer that copies
	//its pointee when it is copied
	std::shared_ptr<acc_base> watcher;
};
template<class T, class Key = key_ty>
class accessor : accessor_impl<T>
{
	template<class T1, class Key1>
	friend class accessor;

public:
	//null magic pointers are valid and do nothing
	accessor()
		: accessor_impl<T>(null_acc_derived::instance)
	{}
	
	template<class Getter, class Setter,
		typename = decltype(std::declval<Getter>()()),
		typename = decltype(std::declval<Setter>()(std::declval<T>()))>
		accessor(Getter g, Setter s)
		: accessor_impl<T>(std::make_shared<acc_derived<Getter, Setter>>(g, s))
	{}
	
	template<class Getter, class Setter,
		typename = decltype(std::declval<Getter>()(std::declval<Key>())),
		typename = decltype(std::declval<Setter>()(std::declval<Key>(), std::declval<T>()))>
		accessor(Getter g, Setter s, int = 0)
		: accessor_impl<T>(std::make_shared<keyed_acc_derived<Getter, Setter, Key>>(g, s))
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
		return watcher != null_acc_derived::instance;
	}

	bool operator==(const accessor<T, Key>& other) const
	{
		return watcher == other.watcher;
	}

	bool operator<(const accessor<T, Key>& other) const
	{
		return watcher < other.watcher;
	}

	void set(const key_ty& key, const T& val)
	{
		watcher->setter(key, val);
	}

	T get(const key_ty& key) const
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
			: temp(temp), owner(owner) {}
		T temp;
		magic_ptr& owner;
		T* operator->() { return &temp; }
		~arrow_helper() { owner.set(temp); }
	};

public:
	magic_ptr() = default;

	template<typename Key>
	magic_ptr(accessor<T, Key> acc, const Key& k)
		: acc(acc)
	{
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
			return{ it->second, key_ty{} };
		}

		auto thiscopy = *this;
		return{ accessor<T, int>{
			[thiscopy]() -> T {return *thiscopy; },
			[thiscopy, other](const T& v) mutable
			{
				*thiscopy = v;
				*other = v;
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
	//returns a temporary value
	T operator*() const { return acc.get(key); }
	//returns a special value that gives member access, and calls set() at the
	//end of the temporary's lifetime.
	arrow_helper operator->() { return{ *this, operator*() }; }
};

template<class T>
std::shared_ptr<typename accessor_impl<T>::null_acc_derived>
accessor_impl<T>::null_acc_derived::instance =
std::make_shared<typename accessor_impl<T>::null_acc_derived>();

#endif