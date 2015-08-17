#ifndef MAGIC_PTR_HPP
#define MAGIC_PTR_HPP

#include <map>

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

namespace magic_detail
{
	//basically, a machine-word worth of something
	using key_ty = std::aligned_storage_t<sizeof(nullptr)>;

	template<class T>
	using getter_t = std::function<T(key_ty key)>;
	template<class T>
	using setter_t = std::function<void(key_ty key, const T& val)>;

	//Trivial getter
	template<class T, class Key>
	getter_t<T> make_getter()
	{
		return [](key_ty) { assert(false && "get called on set-only accessor"); return T{}; };
	}

	//plain callable
	template<class Key, class Getter,
		class T = decltype(std::declval<Getter>()())>
	getter_t<T> make_getter(Getter g)
	{
		return [g] (key_ty) { return g(); };
	}

	//callable taking key
	template<class Key, class Getter,
		typename T = decltype(std::declval<Getter>()(std::declval<Key>()))>
	getter_t<T> make_getter(Getter g, int = 0)
	{
		return [g] (key_ty key) { return g(*reinterpret_cast<const Key*>(&key)); };
	}
	
	//pointer to member function
	template<class Class, class Getter,
		typename T = decltype(CALL_MEMBER_FN(*std::declval<Class>(), std::declval<Getter>())())>
		getter_t<T> make_getter(Getter g, int = 0, int = 0)
	{
		return [g](key_ty key)	{ return CALL_MEMBER_FN(**reinterpret_cast<Class*>(&key), g)(); };
	}

	//pointer to data member
	template<class Class, class Getter,
		typename T = std::remove_reference_t
		<decltype(std::declval<Class>()->*std::declval<Getter>())>>
		getter_t<T> make_getter(Getter g, int = 0, int = 0, int = 0)
	{
		return [g](key_ty key)	{ return *reinterpret_cast<Class*>(&key)->*g; };
	}

	//plain callable
	template<class T, class Key, class Setter,
	class = decltype(std::declval<Setter>()(std::declval<T>()))>
		setter_t<T> make_setter(Setter s)
	{
		return [s](key_ty key, const T& val) mutable { s(val); };
	}

	//callable taking key
	template<class T, class Key, class Setter,
		typename = decltype(std::declval<Setter>()(std::declval<Key>(), std::declval<T>()))>
		setter_t<T> make_setter(Setter s, int = 0)
	{
		return [s](key_ty key, const T& val) mutable
		{ s(*reinterpret_cast<const Key*>(&key), val); };
	}

	//pointer to member function
	template<class T, class Class, class Setter,
		typename = decltype(CALL_MEMBER_FN(*std::declval<Class>(), std::declval<Setter>())
			(std::declval<T>()))>
		setter_t<T> make_setter(Setter s, int = 0, int = 0)
	{
		return [s](key_ty key, const T& val) mutable
		{ CALL_MEMBER_FN(**reinterpret_cast<Class*>(&key), s)(val); };
	}

	//pointer to data member
	template<class T, class Class, class Setter>
		auto make_setter(Setter s, int = 0, int = 0, int = 0) ->
			setter_t<std::remove_reference_t<decltype(std::declval<Class>()->*s)>>
	{
		return [s](key_ty key, const T& val) mutable
		{ *reinterpret_cast<Class*>(&key)->*s = val; };
	}

	template<class T>
	struct acc_heap_obj
	{
		setter_t<T> setter;
		getter_t<T> getter;

		acc_heap_obj()
			: setter([](key_ty, const T&){}), getter(make_getter<T, key_ty>())
		{}
		acc_heap_obj(setter_t<T> s, getter_t<T> g)
			: setter(s), getter(g)
		{}

		static std::shared_ptr<magic_detail::acc_heap_obj<T>> null_acc_heap_obj;
	};
};

template<class T, class Key = magic_detail::key_ty>
class accessor
{
	using this_ty = accessor<T, Key>;
	using key_ty = magic_detail::key_ty;
	template<class T1, class Key1>
	friend class accessor;

	std::shared_ptr<magic_detail::acc_heap_obj<T>> watcher;

	accessor(std::shared_ptr<magic_detail::acc_heap_obj<T>> w)
		: watcher(w)
	{}

public:
	//null magic pointers are valid and do nothing
	accessor()
		: watcher(magic_detail::acc_heap_obj<T>::null_acc_heap_obj)
	{}

	template<class Setter>
	accessor(Setter s)
		: watcher(std::make_shared<magic_detail::acc_heap_obj<T>>(
		magic_detail::make_setter<T, Key>(s), magic_detail::make_getter<T, Key>()))
	{}

	template<class Getter, class Setter>
	accessor(Getter g, Setter s)
		: watcher(std::make_shared<magic_detail::acc_heap_obj<T>>(
		magic_detail::make_setter<T, Key>(s), magic_detail::make_getter<Key>(g)))
	{}

	//erase the key type
	accessor<T> eraseType()
	{
		return{ watcher };
	}

	explicit operator bool() const
	{
		return watcher != magic_detail::acc_heap_obj<T>::null_acc_heap_obj;
	}

	BASIC_EQUALITY(this_ty, watcher)

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
	using key_ty = magic_detail::key_ty;

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
			//causes problems with NaN
			//assert(orig == *owner &&
			//	"magic_ptr::operator-> used more than once in an expression!");
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
		new (&key) std::nullptr_t(nullptr);
	}
	
	magic_ptr(accessor<T> acc)
		: acc(acc)
	{}

	template<typename Key>
	magic_ptr(accessor<T, Key> acc, const Key& k)
		: acc(acc.eraseType())
	{
		new (&key) std::nullptr_t(nullptr);
		static_assert(sizeof(k) <= sizeof(key),// || __alignof(k) <= __alignof(key),
			"Key type does not fit size and alignment requirements");
		static_assert(std::is_trivially_copyable<Key>::value
			&& std::is_trivially_destructible<Key>::value,
			"Key type is not trivially copyable and destructable");
		new(&key) Key(k);
	}

	template<class Class>
	magic_ptr(void (Class::* memPtr)(const T&), Class* classPtr)
		: magic_ptr(accessor<T, Class*>(memPtr), classPtr)
	{}

	template<class Class>
	magic_ptr(T Class::* memPtr, Class* classPtr)
		: magic_ptr(accessor<T, Class*>(memPtr, memPtr), classPtr)
	{}

	//converting magic_ptr
	template<class U, typename = typename std::enable_if_t<
		std::is_convertible<T, U>::value && std::is_convertible<U, T>::value>>
		magic_ptr(magic_ptr<U> other)
		: acc([other]() -> T { return *other; },
		[other](const T& v) mutable { *other = v; })
	{}

	explicit operator bool() const
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
std::shared_ptr<magic_detail::acc_heap_obj<T>>
magic_detail::acc_heap_obj<T>::null_acc_heap_obj =
std::make_shared<magic_detail::acc_heap_obj<T>>();

//deduce type from arguments
template<class T, class Key>
magic_ptr<T> make_magic(accessor<T, Key> acc, Key k = magic_detail::key_ty())
{
	return{ acc, k };
}

template<class Class, class Mem>
magic_ptr<Mem> make_magic(Mem Class::* memPtr, Class* classPtr)
{
	return{ memPtr, classPtr };
}

template<class Class, class T>
magic_ptr<T> make_magic(void (Class::* memPtr)(T), Class* classPtr)
{
	return{ memPtr, classPtr };
}

#endif