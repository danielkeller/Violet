#include "stdafx.h"
#include "Persist.hpp"
#define SQLITE_OMIT_DEPRECATED
#include "sqlite/sqlite3.h"
#include "Profiling.hpp"

#define SQLITE_CHECK_OK(op) do { \
	int result = op; \
	if (result != SQLITE_OK) \
	throw std::logic_error(std::string(#op) + " " + sqlite3_errstr(result)); \
	} while (false)

using namespace Persist_detail;

PreparedStmtImpl::PreparedStmtImpl()
	: stmt(nullptr, &sqlite3_finalize), lastResult(-1)
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

PreparedStmt::PreparedStmt(sqlite3* db, const std::string& sql)
{
	auto p = Profile::Profile("sql compilation");

	sqlite3_stmt *stmtPtr;

	int result = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmtPtr, nullptr);

	if (result != SQLITE_OK)
		throw std::logic_error("'" + sql + "'\n" + sqlite3_errmsg(db));

	//std::cerr << sql << '\n';

	stmt.reset(stmtPtr);
}

PreparedStmtImpl::PreparedStmtImpl(PreparedStmtImpl&& other)
	: stmt(std::move(other.stmt)), lastResult(other.lastResult)
{}

PreparedStmt::PreparedStmt(PreparedStmt&& other)
	: Impl(std::move(other))
{}

PreparedStmtImpl& PreparedStmtImpl::operator=(PreparedStmtImpl other)
{
	swap(stmt, other.stmt);
	swap(lastResult, other.lastResult);
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

void PreparedStmtImpl::Bind1(int num, Object val)
{
	SQLITE_CHECK_OK(sqlite3_bind_int(stmt.get(), num, val.Id()));
}

void PreparedStmtImpl::Bind1(int num, std::string val)
{
	SQLITE_CHECK_OK(sqlite3_bind_text64(stmt.get(), num,
		val.data(), val.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
}

void PreparedStmtImpl::Bind1(int num, const void* val, std::uint64_t len)
{
	SQLITE_CHECK_OK(sqlite3_bind_blob64(stmt.get(), num, val, len, SQLITE_TRANSIENT));
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

Database::Database(std::string file)
	: db(nullptr, &sqlite3_close)
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
	return{ db.get(), sql };
}

Persist::Persist()
	: database("data.db")
{
}

const char* TypeName(Column::Affinity type)
{
	switch (type)
	{
	case Column::INTEGER:
		return "INTEGER";
	case Column::REAL:
		return "REAL";
	case Column::TEXT:
		return "TEXT";
	case Column::BLOB:
		return "BLOB";
	case Column::NONE:
		return "NONE";
	default:
		throw std::domain_error("Invalid column affinity");
	}
}

void Persist::Track(const char* name, std::initializer_list<Column> cols)
{
	schema[name] = cols;

	const Column& key = *cols.begin();
	bool has_fk = false;

	std::stringstream command;
	command << "create table if not exists " << name << " (";

	for (const auto& col : cols)
	{
		if (&col != cols.begin()) command << ", ";

		command << col.name << ' ' << TypeName(col.affinity); 
		
		if (&col == cols.begin()) command << " primary key not null";

		if (col.name == std::string("object"))
		{
			command << " references object(object) on delete cascade"; //?
			has_fk = true;
		}
	}

	command << ")";

	database.MakeStmt(command.str()).Step();

	if (has_fk)
	{
		//automatically deal with the foreign key
		//the goofy syntax is to get around trigger on conflict overriding
		std::stringstream trigger;
		trigger << "create trigger if not exists " << name << "_object "
			"before insert on " << name << " begin "
			"insert into object(object) select NEW.object "
			"where NEW.object not in (select object from object); end";
		database.MakeStmt(trigger.str()).Step();
	}
}


PreparedStmt Persist::MakeSelectAllStmt(const char* subsystem)
{
	std::initializer_list<Column> cols = schema[subsystem];
	auto rest = make_range(cols.begin() + 1, cols.end());

	std::stringstream command;
	command << "select " << cols.begin()->name;
	for (const auto& col : rest)
		command << ", " << col.name;
	command << " from " << subsystem;

	return database.MakeStmt(command.str());
}

PreparedStmt Persist::MakeSelectSomeStmt(const char* subsystem, const char* col)
{
	std::initializer_list<Column> cols = schema[subsystem];
	const Column& key = *schema[subsystem].begin();
	auto rest = make_range(cols.begin() + 1, cols.end());

	std::stringstream command;
	command << "select " << key.name;
	for (const auto& col : rest)
		command << ", " << col.name;
	command << " from " << subsystem
		<< " where " << col << " = ?";

	return database.MakeStmt(command.str());
}

PreparedStmt Persist::MakeInsertStmt(const char* subsystem)
{
	std::initializer_list<Column> cols = schema[subsystem];
	auto rest = make_range(cols.begin() + 1, cols.end());

	std::stringstream command;
	command << "insert or replace into " << subsystem << " (" << cols.begin()->name;

	for (const auto& col : rest)
		command << ", " << col.name;

	command << ") values (?";

	for (const auto& col : rest)
		command << ", ?";
	command << ")";

	return database.MakeStmt(command.str());
}

PreparedStmt Persist::MakeExistsStmt(const char* subsystem)
{
	const Column& key = *schema[subsystem].begin();

	std::stringstream command;
	command << "select (exists (select * from " << subsystem
		<< " where " << key.name << " = ?))";

	return database.MakeStmt(command.str());
}

Object Persist::NextObject()
{
	return database.MakeStmt("select max(object) + 1 from object").Eval1<Object>();
}