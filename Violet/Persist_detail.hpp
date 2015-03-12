//declare some sqlite things to avoid including it
struct sqlite3;
struct sqlite3_stmt;
extern "C" int sqlite3_close(sqlite3*);
extern "C" int sqlite3_finalize(sqlite3_stmt *pStmt);

#include "Utils/Template.hpp"

namespace Persist_detail
{
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

		template<typename... Values>
		void BindImpl(const Values&... vs)
		{
			//bind indices are 1 based and get indices are 0 based
			BindAll(GenSeq<sizeof...(vs)+1>(), vs...);
		}

		template<typename... Values, size_t... Inds>
		void BindAll(seq<0, Inds...>, const Values&... vs)
		{
			std::make_tuple((Bind1(Inds, vs), 0)...);
		}

		void Bind1(int num, std::int64_t val);
		void Bind1(int num, Object val);
		void Bind1(int num, std::string val);
		void Bind1(int num, const void* val, std::uint64_t len);

		template<typename Other>
		void Bind1(int num, const Other& val)
		{
			static_assert(std::is_standard_layout<Other>::value, "Values must be standard layout");
			Bind1(num, static_cast<const void*>(&val), sizeof(Other));
		}

		template<typename... Values>
		std::tuple<Values...> GetImpl()
		{
			//get indices are 0 based and bind indices are 1 based
			return GetAll<Values...>(GenSeq<sizeof...(Values)>());
		}

		template<typename... Values, size_t... Inds>
		std::tuple<Values...> GetAll(seq<Inds...>)
		{
			return std::make_tuple(Get1<Values>(Inds)...);
		}

		template<typename Other>
		Other Get1(int num)
		{
			static_assert(std::is_standard_layout<Other>::value, "Values must be standard layout");
			auto r = GetBlob(num);
			if (r.begin() == nullptr)
				throw std::logic_error("NULL blob");
			if (r.size() != sizeof(Other))
				throw std::logic_error("Wrong size for blob");
			return FromBytes<Other>()(r.begin());
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
}