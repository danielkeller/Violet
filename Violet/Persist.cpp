#include "stdafx.h"
#include "Persist.hpp"
#include "Profiling.hpp"

#define SQLITE_OMIT_DEPRECATED
#include "sqlite/sqlite3.h"

#include <iostream>

#define SQLITE_CHECK_OK(op) do { \
	int result = op; \
	if (result != SQLITE_OK) \
	throw std::logic_error(std::string(#op) + " " + sqlite3_errstr(result)); \
	} while (false)

using namespace Persist_detail;

PreparedStmtImpl::PreparedStmtImpl()
	: stmt(nullptr, &sqlite3_finalize), lastResult(-1), persist(nullptr)
{}

#ifndef NDEBUG
PreparedStmtImpl::~PreparedStmtImpl()
{
	//catch stupid errors
	if (stmt && lastResult == -1)
		std::cerr << "Warning: prepared stmt '" << sqlite3_sql(stmt.get())
			<< "' deleted without being evaluated\n";
}
#endif

PreparedStmt::PreparedStmt(Persist* persist_, sqlite3* db, const std::string& sql)
{
	auto p = Profile::Profile("sql compilation");

	persist = persist_;

	sqlite3_stmt *stmtPtr;
	int result = sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmtPtr, nullptr);
	if (result != SQLITE_OK)
		throw std::logic_error("'" + sql + "'\n" + sqlite3_errmsg(db));

	//std::cerr << sql << '\n';

	stmt.reset(stmtPtr);
}

PreparedStmtImpl::PreparedStmtImpl(PreparedStmtImpl&& other)
	: stmt(std::move(other.stmt)), lastResult(other.lastResult), persist(other.persist)
{}

PreparedStmt::PreparedStmt(PreparedStmt&& other)
	: Impl(std::move(other))
{}

PreparedStmtImpl& PreparedStmtImpl::operator=(PreparedStmtImpl other)
{
	swap(stmt, other.stmt);
	swap(lastResult, other.lastResult);
	swap(persist, other.persist);
	return *this;
}

PreparedStmt& PreparedStmt::operator=(PreparedStmt other)
{
	swap(stmt, other.stmt);
	return *this;
}

PreparedStmt& PreparedStmt::Step()
{
	if (stmt)
	{
		auto p = Profile::Profile("sql evaluation");

		lastResult = sqlite3_step(stmt.get());
		if (lastResult != SQLITE_DONE && lastResult != SQLITE_ROW)
			throw std::logic_error(std::string("sqlite3_step ") + sqlite3_errstr(lastResult));
	}
	return *this;
}

PreparedStmt& PreparedStmt::Reset()
{
	SQLITE_CHECK_OK(sqlite3_reset(stmt.get()));
	return *this;
}

PreparedStmt::operator bool()
{
	return lastResult == SQLITE_ROW;
}

void PreparedStmtImpl::Bind1(int num, std::int64_t val)
{
	SQLITE_CHECK_OK(sqlite3_bind_int64(stmt.get(), num, val));
}

void PreparedStmtImpl::Bind1(int num, bool val)
{
	Bind1(num, val ? 1ll : 0ll);
}

void PreparedStmtImpl::Bind1(int num, Object val)
{
	SQLITE_CHECK_OK(sqlite3_bind_int(stmt.get(), num, val.Id()));
}

void PreparedStmtImpl::Bind1(int num, std::string val)
{
	SQLITE_CHECK_OK(sqlite3_bind_text64(stmt.get(), num,
		val.data(), val.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
}

void PreparedStmtImpl::Bind1(int num, std::vector<char> val)
{
	Bind1(num, range<const char*>{ &*val.begin(), &*val.end() });
}

void PreparedStmtImpl::Bind1(int num, std::vector<std::string> val)
{
	std::string concat;
	for (const auto& str : val)
		concat += str + '\t';

	concat.erase(concat.end() - 1, concat.end());

	Bind1(num, concat);
}

void PreparedStmtImpl::Bind1(int num, range<const char*> val)
{
	SQLITE_CHECK_OK(sqlite3_bind_blob64(stmt.get(), num,
		val.begin(), val.size(), SQLITE_TRANSIENT));
}

template<>
std::int64_t PreparedStmtImpl::Get1<std::int64_t>(int num)
{
	return sqlite3_column_int64(stmt.get(), num);
}

template<>
Object PreparedStmtImpl::Get1<Object>(int num)
{
	return Object(sqlite3_column_int(stmt.get(), num));
}

template<>
bool PreparedStmtImpl::Get1<bool>(int num)
{
	return sqlite3_column_int(stmt.get(), num) == 1;
}

template<>
std::vector<char> PreparedStmtImpl::Get1<std::vector<char>>(int num)
{
	auto r = GetBlob(num);
	return{ r.begin(), r.end() };
}

template<>
std::vector<std::string> PreparedStmtImpl::Get1<std::vector<std::string>>(int num)
{
	auto str = Get1<std::string>(num);
	std::vector<std::string> ret;
	std::string temp;

	std::stringstream ss(str);
	while (std::getline(ss, temp, '\t'))
		ret.push_back(temp);
	
	return ret;
}

template<>
std::string PreparedStmtImpl::Get1<std::string>(int num)
{
	const unsigned char* text = sqlite3_column_text(stmt.get(), num);
	int len = sqlite3_column_bytes(stmt.get(), num);
	return{ text, text + len };
}

range<const char*> PreparedStmtImpl::GetBlob(int num)
{
	const char* blob = static_cast<const char*>(sqlite3_column_blob(stmt.get(), num));
	int len = sqlite3_column_bytes(stmt.get(), num);
	return{ blob, blob + len };
}

Database::Database(Persist* persist, std::string file)
	: db(nullptr, &sqlite3_close), persist(persist), transactDepth(0)
{
	EXCEPT_INFO_BEGIN

	sqlite3* dbPtr;
	SQLITE_CHECK_OK(sqlite3_open(file.c_str(), &dbPtr));
	db.reset(dbPtr);

	MakeStmt("PRAGMA journal_mode=WAL").Step();
	MakeStmt("PRAGMA foreign_keys=ON").Step();
	
	//The object table is given special treatment
	MakeStmt("create table if not exists object"
		"(object integer primary key not null)").Step();

	EXCEPT_INFO_END(file)
}

PreparedStmt Database::MakeStmt(const std::string& sql)
{
	return{ persist, db.get(), sql };
}

void Database::Track(const char* name, Columns cols)
{
	if (schema.count(name))
		return;

	schema[name] = cols;

	bool has_fk = false;

	std::stringstream command;
	command << "create table if not exists " << name << " (";

	for (const auto& col : cols)
	{
		if (&col != cols.begin()) command << ", ";

		command << col; 
		
		if (&col == cols.begin()) command << " primary key not null";

		if (col == std::string("object"))
		{
			command << " references object(object) on delete cascade"; //?
			has_fk = true;
		}
	}

	command << ")";

	MakeStmt(command.str()).Step();

	if (has_fk)
	{
		//automatically deal with the foreign key
		//the goofy syntax is to get around trigger on conflict overriding
		std::stringstream trigger;
		trigger << "create trigger if not exists " << name << "_object "
			"before insert on " << name << " begin "
			"insert into object(object) select NEW.object "
			"where NEW.object not in (select object from object); end";
		MakeStmt(trigger.str()).Step();
	}
}


PreparedStmt Database::MakeSelectAllStmt(const char* subsystem)
{
	Columns cols = schema[subsystem];
	auto rest = make_range(cols.begin() + 1, cols.end());

	std::stringstream command;
	command << "select " << *cols.begin();
	for (const auto& col : rest)
		command << ", " << col;
	command << " from " << subsystem;

	return MakeStmt(command.str());
}

PreparedStmt Database::MakeSelectSomeStmt(const char* subsystem, const char* col)
{
	Columns cols = schema[subsystem];
	const char * const key = *schema[subsystem].begin();
	auto rest = make_range(cols.begin() + 1, cols.end());

	std::stringstream command;
	command << "select " << key;
	for (const auto& col : rest)
		command << ", " << col;
	command << " from " << subsystem
		<< " where " << col << " = ?";

	return MakeStmt(command.str());
}

PreparedStmt Database::MakeInsertStmt(const char* subsystem)
{
	Columns cols = schema[subsystem];
	auto rest = make_range(cols.begin() + 1, cols.end());

	std::stringstream command;
	command << "insert or replace into " << subsystem << " (" << *cols.begin();

	for (const auto& col : rest)
		command << ", " << col;

	command << ") values (?";

	for (const auto& col : rest)
		command << ", ?";
	command << ")";

	return MakeStmt(command.str());
}

PreparedStmt Database::MakeExistsStmt(const char* subsystem)
{
	const char* const key = *schema[subsystem].begin();

	std::stringstream command;
	command << "select (exists (select * from " << subsystem
		<< " where " << key << " = ?))";

	return MakeStmt(command.str());
}

PreparedStmt Database::MakeDeleteStmt(const char* subsystem)
{
	const char* const key = *schema[subsystem].begin();

	std::stringstream command;
	command << "delete from " << subsystem << " where " << key << " = ?";

	return MakeStmt(command.str());
}

Transaction Database::Begin()
{
	return{ this };
}

Transaction::Transaction(Database* db)
	: db(db)
{
	if (db->transactDepth++ == 0)
		db->MakeStmt("begin transaction").Step();
}

Transaction::~Transaction()
{
	auto p = Profile::Profile("sql commit");

	if (--db->transactDepth == 0)
		db->MakeStmt("end transaction").Step();
}

Persist::Persist()
: database(this, "data.db")
{
}

Object Persist::NextObject()
{
	return database.MakeStmt("select max(object) + 1 from object").Eval1<Object>();
}