//declare some sqlite things to avoid including it
struct sqlite3;
struct sqlite3_stmt;
extern "C" int sqlite3_close(sqlite3*);
extern "C" int sqlite3_finalize(sqlite3_stmt *pStmt);

#include "Utils/Template.hpp"

namespace Persist_detail
{
	using namespace std::placeholders;

	template<class Other>
	struct PersistTraits
	{
		using Category = typename Other::PersistCategory;
	};

	template<class Other>
	struct PersistTraits<std::vector<Other>>
	{
		using Category = VectorPersistTag;
	};

	template<class Other>
	using cat_t = typename PersistTraits<Other>::Category;

	struct PreparedStmtImpl
	{
		PreparedStmtImpl();
		PreparedStmtImpl(PreparedStmtImpl&) = delete;
		PreparedStmtImpl(PreparedStmtImpl&&);
		PreparedStmtImpl& operator=(PreparedStmtImpl other);
#ifndef NDEBUG
		~PreparedStmtImpl();
#endif

		std::unique_ptr<sqlite3_stmt, decltype(&::sqlite3_finalize)> stmt;
		int lastResult;
		Persist* persist;

		template<typename... Values>
		void BindImpl(const Values&... vs)
		{
			//bind indices are 1 based and get indices are 0 based
			BindAll(GenSeq<sizeof...(vs)+1>(), vs...);
		}

		template<typename... Values, unsigned... Inds>
		void BindAll(seq<0, Inds...>, const Values&... vs)
		{
			std::make_tuple((Bind1(Inds, vs), 0)...);
		}

		void Bind1(int num, std::int64_t val);
		void Bind1(int num, bool val);
		void Bind1(int num, Object val);
		void Bind1(int num, std::string val);
		void Bind1(int num, std::vector<char> val);
		void Bind1(int num, std::vector<std::string> val);
		void Bind1(int num, range<const char*> val);

		template<typename Other>
		void Bind1(int num, const Other& val)
		{
			Bind1(num, Prepare(val, cat_t<Other>()));
		}

		template<typename Other>
		std::vector<std::string> Prepare(const std::vector<Other>& val, VectorPersistTag)
		{
			auto strings = MapRange(val,
				[this](const Other& v) {return Prepare(v, cat_t<Other>()); });
			return{ strings.begin(), strings.end() };
		}

		template<typename Other>
		static range<const char*> Prepare(const Other& val, BinaryPersistTag)
		{
			auto ptr = reinterpret_cast<const char*>(&val);
			return{ ptr, ptr + sizeof(Other) };
		}

		template<typename Other>
		std::string Prepare(const Other& val, ResourcePersistTag)
		{
			return val.Name();
		}

		template<typename Other>
		std::string Prepare(const Other& val, EmbeddedResourcePersistTag)
		{
			val.Save(*persist);
			return val.Name();
		}

		template<typename... Values>
		std::tuple<Values...> GetImpl()
		{
			//get indices are 0 based and bind indices are 1 based
			return GetAll<Values...>(GenSeq<sizeof...(Values)>());
		}

		template<typename... Values, unsigned... Inds>
		std::tuple<Values...> GetAll(seq<Inds...>)
		{
			return std::make_tuple(Get1<Values>(Inds)...);
		}

		template<typename Other>
		Other Get1(int num)
		{
			return Get1<Other>(num, cat_t<Other>());
		}

		template<typename Vector>
		Vector Get1(int num, VectorPersistTag)
		{
			using Other = Vector::value_type;
			auto strs = Get1<std::vector<std::string>>(num);
			/*auto vals = MapRange(strs, [this](std::string name)
				{return Decode<Other>(name, cat_t<Other>()); });
			return{ vals.begin(), vals.end() };*/
			Vector ret;
			std::transform(strs.begin(), strs.end(), std::back_inserter(ret),
				[this](std::string name) { return Decode<Other>(name, cat_t<Other>()); });
			return ret;
		}

		template<typename Other>
		Other Get1(int num, BinaryPersistTag)
		{
			auto r = GetBlob(num);
			if (r.begin() == nullptr)
				throw std::logic_error("NULL blob");
			if (r.size() != sizeof(Other))
				throw std::logic_error("Wrong size for blob");
			return FromBytes<Other>()(r.begin());
		}

		template<typename Other, typename OtherTag>
		Other Get1(int num, OtherTag)
		{
			return Decode<Other>(Get1<std::string>(num), OtherTag());
		}

		template<typename Other>
		Other Decode(std::string name, ResourcePersistTag)
		{
			return{ name };
		}

		template<typename Other>
		Other Decode(std::string name, EmbeddedResourcePersistTag)
		{
			//this is the only way I can think of to construct an object from a tuple
			return std::pair<Other, int>(std::piecewise_construct,
				persist->Get<Other>(name), std::make_tuple(0)).first;
		}

		range<const char*> GetBlob(int num);
	};

	template<>
	std::int64_t PreparedStmtImpl::Get1<std::int64_t>(int num);
	template<>
	Object PreparedStmtImpl::Get1<Object>(int num);
	template<>
	bool PreparedStmtImpl::Get1<bool>(int num);
	template<>
	std::string PreparedStmtImpl::Get1<std::string>(int num);
	template<>
	std::vector<char> PreparedStmtImpl::Get1<std::vector<char>>(int num);
	template<>
	std::vector<std::string> PreparedStmtImpl::Get1<std::vector<std::string>>(int num);

	template<typename... Types>
	class DataIterator;

	template<typename... Types>
	class DataIteratorRange;

	class PreparedStmt : private PreparedStmtImpl
	{
	public:
		PreparedStmt();
		PreparedStmt(PreparedStmt&) = delete;
		PreparedStmt(PreparedStmt&&);
		PreparedStmt& operator=(PreparedStmt other);

		BASIC_EQUALITY(PreparedStmt, stmt);

		template<typename... Values>
		PreparedStmt& Bind(const Values&... vs) { BindImpl(vs...); return *this; }

		PreparedStmt& Step();

		template<typename... Values>
		std::tuple<Values...> Get() { return GetImpl<Values...>(); }

		PreparedStmt& Reset();

		//helper function for one row queries
		template<typename... Return, typename... Params>
		std::tuple<Return...> Eval(const Params&... vs)
		{
			Reset(); Bind(vs...); Step(); return Get<Return...>();
		}

		//helper for one row one column queries
		template<typename Return, typename... Params>
		Return Eval1(const Params&... vs)
		{
			return std::get<0>(Eval<Return>(vs...));
		}

		explicit operator bool();

		template<typename Tuple>
		static DataIteratorRange<Tuple> StmtData(PreparedStmt&& stmt)
		{
			return{ std::move(stmt) };
		}

	private:
		PreparedStmt(Persist* persist, sqlite3* db, const std::string& sql);

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
		using Base = std::iterator<std::input_iterator_tag, std::tuple<Types...>>;
		using value_type = std::tuple<Types...>;
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

		value_type operator*()
		{
			if (!stmt) throw std::logic_error("No more data in result set");
			return stmt->Get<Types...>();
		}

		arrow_helper operator->() { return{ operator*() }; }

		DataIterator& operator++()
		{
			stmt->Step();
			if (!*stmt) //release it at the end to be equal to the end iterator
				stmt = nullptr;
			return *this;
		}

		void operator++(int) { operator++(); }

		explicit operator bool() { return stmt != nullptr; }

	private:
		PreparedStmt* stmt;
	};

	class Database
	{
	public:
		Database(Persist* persist, std::string file);

		void Track(const char* name, std::initializer_list<const char*> cols);

		PreparedStmt MakeStmt(const std::string& sql);

		PreparedStmt MakeInsertStmt(const char* subsystem);
		PreparedStmt MakeSelectAllStmt(const char* subsystem);
		PreparedStmt MakeSelectSomeStmt(const char* subsystem, const char* col);
		PreparedStmt MakeExistsStmt(const char* subsystem);
		PreparedStmt MakeDeleteStmt(const char* subsystem);

	private:
		std::unique_ptr<sqlite3, decltype(&::sqlite3_close)> db;
		std::unordered_map<const char*, std::initializer_list<const char*>> schema;
		Persist* persist;
	};
}