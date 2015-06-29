#ifndef RESOURCE_HPP
#define RESOURCE_HPP
#include <unordered_map>

//uses CRTP
template<class T, class K = std::string>
class Resource
{
private:
	static std::unordered_map<K, std::weak_ptr<T>>& Cache()
	{
		static std::unordered_map<K, std::weak_ptr<T>> cache;
		return cache;
	}

protected:
	const K key;

	Resource(const Resource&) = delete;
	Resource& operator=(const Resource&) = delete;

	Resource(Resource&& other)
		: key(std::move(other.key))
	{}

	Resource(const K &key)
		: key(key)
	{}

    //Removing this is a hack to allow resource holders to have static storage duration.
    //FindResource will still return null if there is no cached object, but the
    //map entry will hang around, causing a slight performace degradation. The real
    //fix here is to make the UI stuff non-static
    /*
	~Resource()
	{
		Cache().erase(key);
	}
     */

	typedef Resource<T, K> ResourceTy;

public:
	template<class... Args>
	static std::shared_ptr<T> MakeShared(Args&&... a)
	{
		auto ptr = std::make_shared<T>(std::forward<Args>(a)...);
		Cache()[ptr->key] = ptr;
		return ptr;
	}

	static std::shared_ptr<T> FindResource(const K &key)
	{
		auto it = Cache().find(key);
		return it == Cache().end() ? nullptr : it->second.lock();
	}

	//will fail if T's constructor doesn't take one argument of type const K&
	static std::shared_ptr<T> FindOrMake(const K &key)
	{
		auto ptr = FindResource(key);
		if (!ptr)
			ptr = MakeShared(key);
		return ptr;
	}

	K Key() const
	{
		return key;
	}
};

#endif