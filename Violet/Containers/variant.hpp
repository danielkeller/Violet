#ifndef OPTIONAL_HPP
#define OPTIONAL_HPP

template <class T>
class optional
{
private:
    union
    {
        T value;
        char dummy;
    };
    
    bool present;
    
public:
    using value_type = T;
    
    optional() : present(false) {}
    optional(const optional& other)
    {
        if (other) ::new(&value) T(*other);
    }
    optional(optional&& other)
    {
        if (other) ::new(&value) T(std::move(*other));
    }
    optional(const T& value) : present(true), value(value) {}
    optional(T&& value) : present(true), value(std::move(value)) {}
    ~optional() { if (present) value.~T(); }
    
    optional& operator=(optional other)
    {
        swap(*this, other); return *this;
    }
    friend void swap(optional& l, optional& r)
    {
        if (l && r)
            swap(*l, *r);
        else if (r)
            swap(r, l);
        else if (l)
        {
            ::new(&r.value) T(std::move(*l));
            l.value.~T();
            l.present = false;
        }
    }
    
    explicit operator bool() const {return present;}
    const T& operator*() const {return value;}
    T& operator*() {return value;}
};

template<class... Ts> union variant_h {};
template<class T, class... Ts> union variant_h<T, Ts...>
{
    variant_h() {}
    ~variant_h() {}
    T val;
    variant_h<Ts...> rest;
};

template<class T, class... Ts> constexpr int var_get(const variant_h<T, Ts...>&)
{
    return sizeof...(Ts);
}
template<class T, class... Ts> constexpr int var_get(const variant_h<Ts...>& v)
{
    static_assert(sizeof...(Ts), "variant does not contain this type");
    return var_get<T>(v.rest);
}

struct none {};

template <class... Ts>
class variant {
    variant_h<Ts...> data;
    char which;
    
    template<class T> void destroy()
    {
        if (which == var_get<T>(data)) reinterpret_cast<T*>(&data)->~T();
    }
    template<class T> void construct(T&& val)
    {
        using ObjTy = std::remove_const_t<std::remove_reference_t<T>>;
        if (which == var_get<ObjTy>(data)) ::new(&data) ObjTy(std::forward<T>(val));
    }
    
public:
    variant()
        : which(var_get<none>(data))
    {}

    template<class T>
    variant(const T& val)
        : which(var_get<T>(data))
    {
        construct(val);
    }
    
    variant(const variant& other)
        : which(other.which)
    {
        std::make_tuple((construct(other.get<Ts>()), 0)...);
    }
    
    variant(variant&& other)
        : which(other.which)
    {
        std::make_tuple((construct(std::move(other.get<Ts>())), 0)...);
    }
    
    ~variant()
    {
        std::make_tuple((destroy<Ts>(), 0)...);
    }
    
    variant& operator=(variant other)
    {
        swap(*this, other);
        return *this;
    }
    
    friend void swap(variant& l, variant& r) noexcept
    {
        variant temp(std::move(l));
        l.~variant();
        ::new (&l) variant(std::move(r));
        r.~variant();
        ::new (&r) variant(std::move(l));
    }
    
    template<class T>
    bool is() const
    {
        return which == var_get<T>(data);
    }
    
    template<class T>
    T& get()
    {
        assert(which == var_get<T>(data));
        return *reinterpret_cast<T*>(&data);
    }
    template<class T>
    const T& get() const
    {
        assert(which == var_get<T>(data));
        return *reinterpret_cast<const T*>(&data);
    }
    
    template<class T>
    T* try_get()
    {
        if (is<T>())
            return reinterpret_cast<T*>(&data);
        else
            return nullptr;
    }
    template<class T>
    const T* try_get() const
    {
        return const_cast<variant<Ts...>>(this)->try_get<T>();
    }
    
    explicit operator bool() const {return is<none>();}
};

template<class T>
struct nullable
{
    using type = optional<T>;
};

template<class T>
struct nullable<optional<T>>
{
    using type = optional<T>;
};

template<>
struct nullable<variant<>>
{
    using type = variant<none>;
};

template<class... Ts>
struct nullable<variant<none, Ts...>>
{
    using type = variant<none, Ts...>;
};

template<class... Ts>
struct nullable<variant<Ts...>>
{
    using type = variant<none, Ts...>;
};

template<class T>
using nullable_t = typename nullable<T>::type;

#endif
