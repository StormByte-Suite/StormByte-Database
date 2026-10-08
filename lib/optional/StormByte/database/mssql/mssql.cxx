/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Database.
 *
 * StormByte-Database original source is dual-licensed:
 *
 * 1. GNU Lesser General Public License v3.0 (or later)
 *    You may redistribute and/or modify this file under the terms of the
 *    GNU Lesser General Public License as published by the Free Software
 *    Foundation, either version 3 of the License, or (at your option)
 *    any later version.
 *
 * 2. Commercial license
 *    Alternatively, this file may be used under the terms of a commercial
 *    license agreement with the copyright holder
 *    (David C. Manuelda <StormByte@gmail.com>).
 *
 * Both licenses apply only to original StormByte-Database source in this
 * repository. They do not cover other StormByte modules or any third-party
 * material shipped with this repository (including everything under
 * thirdparty/, and in particular the bundled StormByte-Logger tree and
 * the PostgreSQL, MariaDB and SQLite trees), which remain under their own
 * licenses.
 *
 * Neither license grants any patent rights. Any patent licenses required
 * to use this software or third-party components must be obtained separately
 * from the patent holders.
 *
 * StormByte-Database is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with StormByte-Database. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-StormByte-Commercial
 */

#include <StormByte/database/mssql/mssql.hxx>
#include <StormByte/database/mssql/result_fetch.hxx>
#include <StormByte/database/mssql/prepared_stmt.hxx>
#include <StormByte/database/value.hxx>
#include <StormByte/database/engine_handles.hxx>

#include <sybdb.h>

#include <algorithm>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

using namespace StormByte::Database::MSSQL;

namespace {
	using StormByte::Database::Value;
	std::once_flag db_library_once;

	bool LoginString(LOGINREC* login, const char* value, const int option) {
		return !value || !*value || dbsetlname(login, value, option) != FAIL;
	}

	bool LoginPort(LOGINREC* login, const int port) {
		if (port <= 0 || port > std::numeric_limits<unsigned short>::max())
			return false;
	#ifdef DBSETPORT
		return dbsetlshort(login, port, DBSETPORT) != FAIL;
	#else
		(void)login;
		return true;
	#endif
	}

	std::string ErrorText(const char* message, const std::string_view fallback) {
		if (message && *message)
			return message;
		if (!fallback.empty())
			return std::string{fallback};
		return "Unknown FreeTDS DB-Library error";
	}

	struct STORMBYTE_DATABASE_PRIVATE RpcParameter {
		int type{SYBVARCHAR};
		DBINT length{};
		std::string text;
		StormByte::Safe::Binary binary;
		DBINT int32_value{};
		DBBIGINT int64_value{};
		DBFLT8 float_value{};
		DBBIT bit_value{};
		bool empty_value{};
		bool has_empty_marker{};
		BYTE* Data() noexcept {
			if (!text.empty()) return reinterpret_cast<BYTE*>(text.data());
			if (!binary.empty()) return reinterpret_cast<BYTE*>(binary.begin());
			switch (type) {
				case SYBINT4: return reinterpret_cast<BYTE*>(&int32_value);
				case SYBINT8: return reinterpret_cast<BYTE*>(&int64_value);
				case SYBFLT8: return reinterpret_cast<BYTE*>(&float_value);
				case SYBBIT: return &bit_value;
				default: return nullptr;
			}
		}
	};

	std::string SqlType(const Value& value) {
		switch (value.Type()) {
			case Value::Type::Integer: return "int";
			case Value::Type::UnsignedInteger: return "bigint";
			case Value::Type::LongInteger: return "bigint";
			case Value::Type::UnsignedLongInteger: return "decimal(20,0)";
			case Value::Type::Double: return "float";
			case Value::Type::Boolean: return "bit";
			case Value::Type::Text: return "nvarchar(max)";
			case Value::Type::Blob: return "varbinary(max)";
			case Value::Type::Null: return "nvarchar(max)";
		}
		return "sql_variant";
	}

	bool TranslatePlaceholders(const std::string_view query, const StormByte::Safe::Vector<Value>& parameters,
			std::string& translated, std::string& declarations) {
		enum class State { Normal, SingleQuote, DoubleQuote, Bracket, LineComment, BlockComment };
		State state = State::Normal;
		unsigned int block_depth = 0;
		std::size_t parameter_index = 0;
		translated.clear();
		declarations.clear();
		translated.reserve(query.size() + parameters.size() * 4);
		for (std::size_t index = 0; index < query.size(); ++index) {
			const char current = query[index];
			const char next = index + 1 < query.size() ? query[index + 1] : '\0';
			if (state == State::Normal) {
				if (current == '\'') state = State::SingleQuote;
				else if (current == '"') state = State::DoubleQuote;
				else if (current == '[') state = State::Bracket;
				else if (current == '-' && next == '-') { state = State::LineComment; translated.push_back(current); translated.push_back(next); ++index; continue; }
				else if (current == '/' && next == '*') { state = State::BlockComment; block_depth = 1; translated.push_back(current); translated.push_back(next); ++index; continue; }
				else if (current == '?') {
					if (parameter_index >= parameters.size())
						return false;
					if (!declarations.empty()) declarations += ", ";
					const std::string parameter_name = "@p" + std::to_string(parameter_index);
					const Value& parameter = parameters[parameter_index];
					declarations += parameter_name + " " + SqlType(parameter);
					if (parameter.Type() == Value::Type::Text) {
						declarations += ", " + parameter_name + "_empty bit";
						translated += "(CASE WHEN " + parameter_name + "_empty = 1 THEN CAST(N'' AS nvarchar(max)) ELSE " + parameter_name + " END)";
					} else if (parameter.Type() == Value::Type::Blob) {
						declarations += ", " + parameter_name + "_empty bit";
						translated += "(CASE WHEN " + parameter_name + "_empty = 1 THEN CAST(0x AS varbinary(max)) ELSE " + parameter_name + " END)";
					} else if (parameter.IsNull()) {
						// An untyped NULL converts implicitly to any column type.
						translated += "NULL";
					} else {
						translated += parameter_name;
					}
					++parameter_index;
					continue;
				}
			} else if (state == State::SingleQuote && current == '\'') {
				if (next == '\'') { translated.push_back(current); translated.push_back(next); ++index; continue; }
				state = State::Normal;
			} else if (state == State::DoubleQuote && current == '"') {
				if (next == '"') { translated.push_back(current); translated.push_back(next); ++index; continue; }
				state = State::Normal;
			} else if (state == State::Bracket && current == ']') {
				if (next == ']') { translated.push_back(current); translated.push_back(next); ++index; continue; }
				state = State::Normal;
			} else if (state == State::LineComment && (current == '\n' || current == '\r')) {
				state = State::Normal;
			} else if (state == State::BlockComment) {
				if (current == '/' && next == '*') { ++block_depth; translated.push_back(current); translated.push_back(next); ++index; continue; }
				if (current == '*' && next == '/') {
					if (--block_depth == 0) state = State::Normal;
					translated.push_back(current); translated.push_back(next); ++index; continue;
				}
			}
			translated.push_back(current);
		}
		return parameter_index == parameters.size();
	}

	bool FitsRpcLength(const std::size_t length) noexcept {
		return length <= static_cast<std::size_t>(std::numeric_limits<DBINT>::max());
	}
}

namespace StormByte::Database::MSSQL {
	struct STORMBYTE_DATABASE_PRIVATE CallbackHandlers {
		static int Error(DBPROCESS* const process, const int severity, const int database_error,
			const int operating_system_error, char* database_message, char* operating_system_message) {
			(void)severity;
			(void)operating_system_error;
		auto* self = process ? reinterpret_cast<MSSQL*>(dbgetuserdata(process)) : nullptr;
			if (self && !(database_error == SYBESMSG && !self->m_last_error.empty()))
				self->m_last_error = ErrorText(database_message, operating_system_message ? operating_system_message : "");
			return INT_CANCEL;
		}

		static int Message(DBPROCESS* const process, const int message_number, const int state,
			const int severity, char* text, char* server, char* procedure, const int line) {
			(void)message_number;
			(void)state;
			(void)server;
			(void)procedure;
			(void)line;
			if (severity <= 10)
				return INT_CONTINUE;
			auto* self = process ? reinterpret_cast<MSSQL*>(dbgetuserdata(process)) : nullptr;
			if (self) {
				self->m_last_error = ErrorText(text, "SQL Server reported an error");
				static_cast<StormByte::Database::MSSQL::Telemetry*>(self->Telemetry().get())->RecordError();
			}
			return INT_CANCEL;
		}
	};
}

MSSQL::MSSQL(std::string_view host, std::string_view user, std::string_view password,
		std::string_view database, const int port, const StormByte::Safe::Shared<Logger::Log>& logger):
	Database(logger), m_host(host), m_user(user), m_password(password), m_database(database),
	m_port(port), m_connection_handle(StormByte::Safe::Unique<ConnectionHandle>::MakePointer<ConnectionHandle>()) {
	Telemetry(StormByte::Safe::Shared<StormByte::Database::Telemetry>::MakePointer<class Telemetry>());
}

MSSQL::MSSQL(MSSQL&& other) noexcept:
	Database(std::move(other)), m_host(std::move(other.m_host)), m_user(std::move(other.m_user)),
	m_password(std::move(other.m_password)), m_database(std::move(other.m_database)),
	m_port(other.m_port), m_connection_handle(std::move(other.m_connection_handle)),
	m_last_error(std::move(other.m_last_error)) {
	DBPROCESS* process = m_connection_handle ? static_cast<DBPROCESS*>(m_connection_handle->m_native_connection) : nullptr;
	if (process)
		dbsetuserdata(process, reinterpret_cast<BYTE*>(this));
	other.m_connected = false;
}

MSSQL& MSSQL::operator=(MSSQL&& other) noexcept {
	if (this != &other) {
		Disconnect();
		Database::operator=(std::move(other));
		m_host = std::move(other.m_host);
		m_user = std::move(other.m_user);
		m_password = std::move(other.m_password);
		m_database = std::move(other.m_database);
		m_port = other.m_port;
		m_connection_handle = std::move(other.m_connection_handle);
		m_last_error = std::move(other.m_last_error);
		DBPROCESS* process = m_connection_handle ? static_cast<DBPROCESS*>(m_connection_handle->m_native_connection) : nullptr;
		if (process)
			dbsetuserdata(process, reinterpret_cast<BYTE*>(this));
		other.m_connected = false;
	}
	return *this;
}

MSSQL::~MSSQL() noexcept {
	Disconnect();
}

bool MSSQL::DoConnect() noexcept {
	if (m_connected || m_host.empty() || m_port <= 0 || m_port > std::numeric_limits<unsigned short>::max())
		return false;
	if (static_cast<std::string_view>(m_host).find('\0') != std::string_view::npos ||
		static_cast<std::string_view>(m_user).find('\0') != std::string_view::npos ||
		static_cast<std::string_view>(m_password).find('\0') != std::string_view::npos ||
		static_cast<std::string_view>(m_database).find('\0') != std::string_view::npos)
		return false;
	try {
		std::call_once(db_library_once, [] {
			dbinit();
			dberrhandle(&CallbackHandlers::Error);
			dbmsghandle(&CallbackHandlers::Message);
		});
		m_last_error.clear();
		LOGINREC* login = dblogin();
		if (!login) {
			m_last_error = std::string_view{"DB-Library could not allocate a login record"};
			return false;
		}

		bool configured = LoginString(login, m_host.Bytes(), DBSETHOST)
			&& LoginString(login, m_user.Bytes(), DBSETUSER)
			&& LoginString(login, m_password.Bytes(), DBSETPWD)
			&& LoginString(login, "StormByte-Database", DBSETAPP)
			&& LoginString(login, m_database.Bytes(), DBSETDBNAME)
			&& LoginString(login, "UTF-8", DBSETCHARSET)
			&& LoginPort(login, m_port);
		const char* encryption = nullptr;
		switch (m_ssl_mode) {
			case SslMode::Disable: encryption = "off"; break;
			case SslMode::Prefer: encryption = "request"; break;
			case SslMode::Require: encryption = "require"; break;
			case SslMode::Default: break;
		}
	#ifdef DBSETENCRYPTION
		if (configured && encryption)
			configured = dbsetlname(login, encryption, DBSETENCRYPTION) != FAIL;
	#else
		if (encryption) {
			m_last_error = std::string_view{"This DB-Library does not support per-connection TLS modes"};
			dbloginfree(login);
			return false;
		}
	#endif
		if (!configured) {
			m_last_error = std::string_view{"DB-Library rejected an MSSQL connection option"};
			dbloginfree(login);
			return false;
		}

		std::string server_name{static_cast<std::string_view>(m_host)};
	#ifndef DBSETPORT
		if (m_port != 1433)
			server_name += ":" + std::to_string(m_port);
	#endif
		DBPROCESS* process = dbopen(login, server_name.c_str());
		dbloginfree(login);
		if (!process) {
			RecordBackendEvent(BackendEvent::Connection);
			if (m_logger)
				*m_logger << Logger::Level::Error << "MSSQL connection failed: " << ErrorText(nullptr, m_last_error) << std::endl;
			return false;
		}
		if (dbsetopt(process, DBTEXTSIZE, "2147483647", -1) == FAIL) {
			m_last_error = std::string_view{"DB-Library could not configure the MSSQL text size"};
			dbclose(process);
			return false;
		}
		// Older DB-Library releases leave QUOTED_IDENTIFIER OFF, which turns "identifiers" into strings.
		bool session_configured = dbcmd(process, "SET QUOTED_IDENTIFIER ON;") != FAIL
			&& dbsqlexec(process) != FAIL;
		while (session_configured) {
			const RETCODE result_status = dbresults(process);
			if (result_status == NO_MORE_RESULTS)
				break;
			session_configured = result_status != FAIL && dbcanquery(process) != FAIL;
		}
		if (!session_configured) {
			m_last_error = std::string_view{"DB-Library could not configure the MSSQL session options"};
			dbclose(process);
			return false;
		}
		m_connection_handle->m_native_connection = process;
		dbsetuserdata(process, reinterpret_cast<BYTE*>(this));
		return true;
	} catch (...) {
		m_last_error = std::string_view{"Unknown exception while connecting to SQL Server"};
		return false;
	}
}

void MSSQL::DoPreDisconnect() noexcept {
	ClearPreparedSTMTs();
}

void MSSQL::DoDisconnect() noexcept {
	DBPROCESS* process = static_cast<DBPROCESS*>(m_connection_handle->m_native_connection);
	if (process) {
		dbsetuserdata(process, nullptr);
		dbclose(process);
		m_connection_handle->m_native_connection = nullptr;
	}
}

StormByte::Database::ExpectedRows MSSQL::Query(const std::string_view query) noexcept {
	auto telemetry = TrackOperation(Operation::Query);
	OperationGuard lock{*this};
	DBPROCESS* process = static_cast<DBPROCESS*>(m_connection_handle->m_native_connection);
	if (!m_connected || !process) {
		RecordBackendEvent(BackendEvent::Connection);
		telemetry.Complete(false);
		return StormByte::Unexpected<QueryException>(ExecuteError("Database not connected"));
	}
	if (query.find('\0') != std::string_view::npos) {
		telemetry.Complete(false);
		return StormByte::Unexpected<QueryException>(ExecuteError("Query contains an embedded NUL character"));
	}
	try {
		const std::string sql{query};
		m_last_error.clear();
		if (dbcmd(process, sql.c_str()) == FAIL || dbsqlexec(process) == FAIL) {
			RecordBackendEvent(BackendEvent::Other);
			telemetry.Complete(false);
			return StormByte::Unexpected<QueryException>(ExecuteError(ErrorText(nullptr, m_last_error)));
		}
		auto rows = StepResults(process);
		telemetry.Complete(rows.has_value(), rows ? static_cast<std::uint64_t>(rows->Count()) : 0);
		return rows;
	} catch (const StormByte::Exception& error) {
		telemetry.Complete(false);
		return StormByte::Unexpected<QueryException>(ExecuteError(error.what()));
	} catch (...) {
		telemetry.Complete(false);
		return StormByte::Unexpected<QueryException>(ExecuteError("Unknown exception while executing MSSQL query"));
	}
}

bool MSSQL::SilentQuery(const std::string_view query) noexcept {
	auto telemetry = TrackOperation(Operation::SilentQuery);
	OperationGuard lock{*this};
	const bool result = DoSilentQuery(query);
	telemetry.Complete(result);
	return result;
}

bool MSSQL::DoSilentQuery(const std::string_view query) noexcept {
	OperationGuard lock{*this};
	DBPROCESS* process = static_cast<DBPROCESS*>(m_connection_handle->m_native_connection);
	if (!m_connected || !process || query.find('\0') != std::string_view::npos)
		return false;
	try {
		const std::string sql{query};
		m_last_error.clear();
		if (dbcmd(process, sql.c_str()) == FAIL || dbsqlexec(process) == FAIL)
			return false;
		for (;;) {
			const RETCODE result_status = dbresults(process);
			if (result_status == NO_MORE_RESULTS)
				break;
			if (result_status == FAIL)
				return false;
			for (;;) {
				const STATUS row_status = dbnextrow(process);
				if (row_status == NO_MORE_ROWS)
					break;
				if (row_status == FAIL)
					return false;
			}
		}
		return m_last_error.empty();
	} catch (...) {
		return false;
	}
}

StormByte::Safe::Unique<StormByte::Database::PreparedSTMT> MSSQL::CreatePreparedSTMT(
		const std::string_view name, const std::string_view query) noexcept {
	if (query.find('\0') != std::string_view::npos || name.find('\0') != std::string_view::npos)
		return nullptr;
	try {
		auto statement = StormByte::Safe::Unique<PreparedSTMT>::MakePointer<PreparedSTMT>(
			PreparedSTMT::ConstructionKey{}, name, query, m_logger, m_telemetry);
		statement->m_statement_handle->m_native_connection = m_connection_handle->m_native_connection;
		return statement;
	} catch (...) {
		return nullptr;
	}
}

void MSSQL::DoBeginTransaction(const IsolationLevel level) {
	const char* isolation = nullptr;
	switch (level) {
		case IsolationLevel::ReadUncommitted: isolation = "SET TRANSACTION ISOLATION LEVEL READ UNCOMMITTED; "; break;
		case IsolationLevel::ReadCommitted: isolation = "SET TRANSACTION ISOLATION LEVEL READ COMMITTED; "; break;
		case IsolationLevel::RepeatableRead: isolation = "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ; "; break;
		case IsolationLevel::Serializable: isolation = "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE; "; break;
		case IsolationLevel::Default: break;
	}
	if (isolation && !DoSilentQuery(isolation))
		throw TransactionError(ErrorText(nullptr, m_last_error));
	if (!DoSilentQuery("BEGIN TRANSACTION;"))
		throw TransactionError(ErrorText(nullptr, m_last_error));
}

StormByte::Database::ExpectedRows MSSQL::ExecuteParameterized(const std::string_view query,
		const StormByte::Safe::Vector<Value>& parameters) {
	DBPROCESS* process = static_cast<DBPROCESS*>(m_connection_handle->m_native_connection);
	if (!m_connected || !process)
		return StormByte::Unexpected<QueryException>(ExecuteError("Database not connected"));
	std::string translated_query;
	std::string declarations;
	if (!TranslatePlaceholders(query, parameters, translated_query, declarations))
		return StormByte::Unexpected<QueryException>(ExecuteError("MSSQL prepared statement parameter count does not match the SQL placeholders"));
	if (!FitsRpcLength(translated_query.size()) || !FitsRpcLength(declarations.size()))
		return StormByte::Unexpected<QueryException>(ExecuteError("MSSQL prepared statement SQL exceeds DB-Library limits"));

	std::vector<RpcParameter> rpc_parameters(parameters.size());
	for (std::size_t index = 0; index < parameters.size(); ++index) {
		const Value& value = parameters[index];
		RpcParameter& parameter = rpc_parameters[index];
		if (value.IsNull()) {
			parameter.type = SYBVARCHAR;
			parameter.length = 0;
			continue;
		}
		switch (value.Type()) {
			case Value::Type::Integer:
				parameter.type = SYBINT4;
				parameter.int32_value = static_cast<DBINT>(value.Get<int>());
				parameter.length = -1;
				break;
			case Value::Type::UnsignedInteger:
				parameter.type = SYBINT8;
				parameter.int64_value = static_cast<DBBIGINT>(value.Get<unsigned int>());
				parameter.length = -1;
				break;
			case Value::Type::LongInteger:
				parameter.type = SYBINT8;
				parameter.int64_value = static_cast<DBBIGINT>(value.Get<long long int>());
				parameter.length = -1;
				break;
			case Value::Type::UnsignedLongInteger: {
				parameter.type = SYBVARCHAR;
				parameter.text = std::to_string(value.Get<unsigned long long int>());
				if (!FitsRpcLength(parameter.text.size()))
					return StormByte::Unexpected<QueryException>(ExecuteError("MSSQL prepared statement integer exceeds DB-Library limits"));
				parameter.length = static_cast<DBINT>(parameter.text.size());
				break;
			}
			case Value::Type::Double:
				parameter.type = SYBFLT8;
				parameter.float_value = static_cast<DBFLT8>(value.Get<double>());
				parameter.length = -1;
				break;
			case Value::Type::Boolean:
				parameter.type = SYBBIT;
				parameter.bit_value = static_cast<DBBIT>(value.Get<bool>() ? 1 : 0);
				parameter.length = -1;
				break;
			case Value::Type::Text: {
				parameter.type = SYBVARCHAR;
				const auto text = value.Get<StormByte::Safe::String>();
				parameter.text = static_cast<std::string_view>(text);
				parameter.has_empty_marker = true;
				parameter.empty_value = parameter.text.empty();
				if (!FitsRpcLength(parameter.text.size()))
					return StormByte::Unexpected<QueryException>(ExecuteError("MSSQL prepared statement text exceeds DB-Library limits"));
				parameter.length = static_cast<DBINT>(parameter.text.size());
				break;
			}
			case Value::Type::Blob:
				parameter.type = SYBIMAGE;
				parameter.binary = value.Get<StormByte::Safe::Binary>();
				parameter.has_empty_marker = true;
				parameter.empty_value = parameter.binary.empty();
				if (!FitsRpcLength(parameter.binary.size()))
					return StormByte::Unexpected<QueryException>(ExecuteError("MSSQL prepared statement binary value exceeds DB-Library limits"));
				parameter.length = static_cast<DBINT>(parameter.binary.size());
				break;
			case Value::Type::Null:
			default:
				break;
		}
	}

	if (dbrpcinit(process, "sp_executesql", DBRPCNORETURN) == FAIL)
		return StormByte::Unexpected<QueryException>(ExecuteError(ErrorText(nullptr, m_last_error)));
	auto add_rpc_parameter = [process](const char* name, const int type, const DBINT length, BYTE* data) {
		return dbrpcparam(process, name, 0, type, -1, length, data) != FAIL;
	};
	if (!add_rpc_parameter(nullptr, SYBVARCHAR, static_cast<DBINT>(translated_query.size()),
			reinterpret_cast<BYTE*>(translated_query.data())))
		return StormByte::Unexpected<QueryException>(ExecuteError(ErrorText(nullptr, m_last_error)));
	if (!parameters.empty()
			&& !add_rpc_parameter(nullptr, SYBVARCHAR, static_cast<DBINT>(declarations.size()),
				reinterpret_cast<BYTE*>(declarations.data())))
		return StormByte::Unexpected<QueryException>(ExecuteError(ErrorText(nullptr, m_last_error)));
	for (std::size_t index = 0; index < rpc_parameters.size(); ++index) {
		const std::string name = "@p" + std::to_string(index);
		RpcParameter& parameter = rpc_parameters[index];
		if (!add_rpc_parameter(name.c_str(), parameter.type, parameter.length, parameter.Data()))
			return StormByte::Unexpected<QueryException>(ExecuteError(ErrorText(nullptr, m_last_error)));
		if (parameter.has_empty_marker) {
			const std::string empty_name = name + "_empty";
			BYTE empty_value = static_cast<BYTE>(parameter.empty_value ? 1 : 0);
			if (!add_rpc_parameter(empty_name.c_str(), SYBBIT, -1, &empty_value))
				return StormByte::Unexpected<QueryException>(ExecuteError(ErrorText(nullptr, m_last_error)));
		}
	}
	if (dbrpcsend(process) == FAIL || dbsqlok(process) == FAIL) {
		RecordBackendEvent(BackendEvent::Other);
		return StormByte::Unexpected<QueryException>(ExecuteError(ErrorText(nullptr, m_last_error)));
	}
	return StepResults(process);
}
