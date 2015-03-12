#ifndef PERSIST_HPP
#define PERSIST_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <thread>

#include "Object.hpp"
#include "Containers/WrappedIterator.hpp"

#include "Persist_detail.hpp"

template<typename... Types>
class DataIterator;

template<typename... Types>
class DataIteratorRange;

class PreparedStmt : private Persist_detail::PreparedStmtImpl
{
public:
	template<typename... Values>
	PreparedStmt& Bind(const Values&... vs) { BindImpl(vs...); return *this; }

	PreparedStmt& Step();

	template<typename... Values>
	std::tuple<Values...> Get() { return GetImpl<Values...>(); }

	PreparedStmt& Reset();

	//helper function for one row queries
	template<typename... Return, typename... Params>
	std::tuple<Return...> Eval(const Params&... vs)
	{ Reset(); Bind(vs...); Step(); return Get<Return...>();}

	//helper for one row one column queries
	template<typename Return, typename... Params>
	Return Eval1(const Params&... vs)
	{ return std::get<0>(Eval<Return>(vs...)); }

	PreparedStmt();
	PreparedStmt(PreparedStmt&) = delete;
	PreparedStmt(PreparedStmt&&);
	PreparedStmt& operator=(PreparedStmt other);

	BASIC_EQUALITY(PreparedStmt, stmt);

	explicit operator bool();

	template<typename Tuple>
	friend DataIteratorRange<Tuple> StmtData(PreparedStmt&& stmt)
	{ return{ std::move(stmt) }; }

private:
	PreparedStmt(sqlite3* db, const std::string& sql);

	using Impl = PreparedStmtImpl;
	friend class Database;
};

template<typename... Types>
class DataIteratorRange<std::tuple<Types...>>
{
public:
	using iterator = DataIterator<std::tuple<Types...>>;
	iterator begin()
	{
		stmt.Reset();
		//starts before the beginning
		return ++iterator(&stmt);
	}
	iterator end() { return{}; }

	DataIteratorRange(PreparedStmt&& stmt) : stmt(std::move(stmt)) {}
private:
	PreparedStmt stmt;
};

template<typename... Types>
class DataIterator<std::tuple<Types...>>
	: public std::iterator<std::input_iterator_tag, std::tuple<Types...>>
{
	struct arrow_helper
	{
		value_type temp;
		value_type* operator->() { return &temp; }
	};
public:
	DataIterator() : stmt(nullptr) {}
	DataIterator(DataIterator&) = default;
	DataIterator(DataIterator&& other) : stmt(other.stmt) {}
	DataIterator(PreparedStmt* stmt) : stmt(stmt) {}

	BASIC_EQUALITY(DataIterator, stmt);

	value_type operator*() { return stmt->Get<Types...>(); }

	arrow_helper operator->() { return{ operator*() }; }

	DataIterator& operator++()
	{
		stmt->Step();
		if (!*stmt) //release it at the end to be equal to the end iterator
			stmt = nullptr;
		return *this;
	}

	void operator++(int) { operator++(); }

private:
	PreparedStmt* stmt;
};

class Database
{
public:
	Database(std::string file);

	PreparedStmt MakeStmt(const std::string& sql);

private:
	std::unique_ptr<sqlite3, decltype(&::sqlite3_close)> db;
};

struct Column
{
	const char* name;
	enum Affinity
	{
		//numbers come from sqlite docs
		INTEGER = 1, REAL = 2, TEXT = 3, BLOB = 4, NONE = 5
	} affinity;
};

static const Column objKey = { "object", Column::INTEGER };

template<class Subsystem>
struct PersistSchema
{
	static const char* name;
	static const std::initializer_list<Column> cols;
};

template<class Subsystem> struct PersistTraits;

class Persist
{
	template<class Subsystem>
	using data_t = typename PersistTraits<Subsystem>::data;
	template<class Subsystem>
	using key_t = typename PersistTraits<Subsystem>::key;

public:
	Persist();

	template<class Subsystem>
	void Track()
	{
		using schema = PersistSchema<Subsystem>;
		Track(schema::name, schema::cols);
	}

	template<class Subsystem>
	data_t<Subsystem> Get(key_t<Subsystem> k)
	{
		return *GetSome<Subsystem>(PersistSchema<Subsystem>::cols.begin()->name, k)
			.begin();
	}

	template<class Subsystem>
	bool Exists(key_t<Subsystem> k)
	{
		return MakeExistsStmt(PersistSchema<Subsystem>::name).Eval1<bool>(k);
	}

	template<class Subsystem>
	DataIteratorRange<data_t<Subsystem>> GetAll()
	{
		return StmtData<data_t<Subsystem>>(MakeSelectAllStmt(PersistSchema<Subsystem>::name));
	}

	//get matching rows
	template<class Subsystem, class Key>
	DataIteratorRange<data_t<Subsystem>> GetSome(const char* col, Key k)
	{
		return StmtData<data_t<Subsystem>>(std::move( //MSVC does not have ref-qualifiers
			MakeSelectSomeStmt(PersistSchema<Subsystem>::name, col).Bind(k)));
	}

	template<class Subsystem, typename... Args>
	void Set(key_t<Subsystem> k, const Args&... d)
	{
		static_assert(std::is_same<data_t<Subsystem>, std::tuple<decltype(k), Args...>>::value,
			"Wrong arugment types passed");

		MakeInsertStmt(PersistSchema<Subsystem>::name).Bind(k, d...).Step();
		//std::thread(&PreparedStmt::Step, std::move(stmt)).detach();
	}

private:
	Database database;
	std::unordered_map<const char*, std::initializer_list<Column>> schema;

	void Track(const char* name, std::initializer_list<Column> cols);
	PreparedStmt MakeInsertStmt(const char* subsystem);
	PreparedStmt MakeSelectAllStmt(const char* subsystem);
	PreparedStmt MakeSelectSomeStmt(const char* subsystem, const char* col);
	PreparedStmt MakeExistsStmt(const char* subsystem);

	Object NextObject();
	friend class Object;
};

#endif