#ifndef PERSIST_HPP
#define PERSIST_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <thread>

#include "Object.hpp"
#include "Containers/WrappedIterator.hpp"

//requirements: standard layout, no pointer members
struct BinaryPersistTag {};

//requirements: std::string Name() const, Foo::Foo(std::string)
struct ResourcePersistTag {};

//requirements: std::string Name() const, Save(Persist&) const,
//Foo::Foo(PersistTraits<Foo>::data...)
struct EmbeddedResourcePersistTag {};

struct VectorPersistTag {};

#include "Persist_detail.hpp"

using Columns = const std::initializer_list<const char*>;

template<class Subsystem>
struct PersistSchema
{
	static const char* name;
	static Columns cols;
};

template<class Subsystem> struct PersistTraits;

class Persist
{
	template<class Subsystem>
	using data_t = typename PersistTraits<Subsystem>::data;
	template<class Subsystem>
	using key_t = typename PersistTraits<Subsystem>::key;

	using PreparedStmt = Persist_detail::PreparedStmt;
	template<typename... Types>
	using DataIteratorRange = Persist_detail::DataIteratorRange<Types...>;

public:
	Persist();

	template<class Subsystem>
	data_t<Subsystem> Get(key_t<Subsystem> k)
	{
		Track<Subsystem>();
		return *GetSome<Subsystem>(*PersistSchema<Subsystem>::cols.begin(), k).begin();
	}

	template<class Subsystem>
	bool Exists(key_t<Subsystem> k)
	{
		Track<Subsystem>();
		return database.MakeExistsStmt(PersistSchema<Subsystem>::name).Eval1<bool>(k);
	}

	template<class Subsystem>
	DataIteratorRange<data_t<Subsystem>> GetAll()
	{
		Track<Subsystem>();
		return PreparedStmt::StmtData<data_t<Subsystem>>(
			database.MakeSelectAllStmt(PersistSchema<Subsystem>::name));
	}

	//get matching rows
	template<class Subsystem, class Key>
	DataIteratorRange<data_t<Subsystem>> GetSome(const char* col, Key k)
	{
		Track<Subsystem>();
		return PreparedStmt::StmtData<data_t<Subsystem>>(
			std::move( //MSVC does not have ref-qualifiers
			database.MakeSelectSomeStmt(PersistSchema<Subsystem>::name, col).Bind(k)));
	}

	template<class Subsystem, typename... Args>
	void Set(key_t<Subsystem> k, const Args&... d)
	{
		Track<Subsystem>();
		//todo: implicitly convert
		static_assert(std::is_same<data_t<Subsystem>, std::tuple<decltype(k), Args...>>::value,
			"Wrong arugment types passed");

		auto tr = database.Begin();

		auto stmt = database.MakeInsertStmt(PersistSchema<Subsystem>::name);
		stmt.Bind(k, d...);
		stmt.Step();
		//std::thread(&PreparedStmt::Step, std::move(stmt)).detach();
	}

	template<class Subsystem>
	void Delete(key_t<Subsystem> k)
	{
		Track<Subsystem>();
		database.MakeDeleteStmt(PersistSchema<Subsystem>::name).Bind(k).Step();
	}

private:
	Persist_detail::Database database;

	template<class Subsystem>
	void Track()
	{
		using schema = PersistSchema<Subsystem>;
		database.Track(schema::name, schema::cols);
	}

	Object NextObject();
	friend class Object;
};

#endif