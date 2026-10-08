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

#include <StormByte/database/mariadb/mariadb.hxx>
#include <StormByte/database/mariadb/result_fetch.hxx>
#include <StormByte/database/engine_handles.hxx>
#include <StormByte/database/mariadb/prepared_stmt.hxx>
#include <mysql.h>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
using namespace StormByte::Database::MariaDB;
namespace {
	unsigned int LogMariaDBWarnings(MYSQL* conn, const StormByte::Safe::Shared<StormByte::Logger::Log>& logger) {
		if (!conn)
			return 0;
		const unsigned int count = mysql_warning_count(conn);
		if (count == 0)
			return 0;
		if (!logger)
			return count;
		if (mysql_query(conn, "SHOW WARNINGS") != 0)
			return count;
		MYSQL_RES* res = mysql_store_result(conn);
		if (!res)
			return count;
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(res)) != nullptr) {
			const char* level = row[0] ? row[0] : "";
			const char* code  = row[1] ? row[1] : "";
			const char* msg   = row[2] ? row[2] : "";
			*logger << StormByte::Logger::Level::Notice
					<< "MariaDB " << level << " (" << code << "): " << msg << std::endl;
		}

		mysql_free_result(res);
		return count;
	}

	void ApplySslMode(MYSQL* conn, StormByte::Database::SslMode mode) {
		if (!conn)
			return;
#if defined(MYSQL_OPT_SSL_MODE)
		int ssl_mode = 0;
		switch (mode) {
			case StormByte::Database::SslMode::Disable:
#if defined(SSL_MODE_DISABLED)
				ssl_mode = SSL_MODE_DISABLED;
				mysql_options(conn, MYSQL_OPT_SSL_MODE, &ssl_mode);
#endif
				break;
			case StormByte::Database::SslMode::Require:
#if defined(SSL_MODE_REQUIRED)
				ssl_mode = SSL_MODE_REQUIRED;
				mysql_options(conn, MYSQL_OPT_SSL_MODE, &ssl_mode);
#endif
				break;
			case StormByte::Database::SslMode::Prefer:
#if defined(SSL_MODE_PREFERRED)
				ssl_mode = SSL_MODE_PREFERRED;
				mysql_options(conn, MYSQL_OPT_SSL_MODE, &ssl_mode);
#endif
				break;
			case StormByte::Database::SslMode::Default:
			default:
				break;
		}
#elif defined(MYSQL_OPT_SSL_ENFORCE)
		if (mode == StormByte::Database::SslMode::Disable) {
			my_bool enforce = 0;
			mysql_options(conn, MYSQL_OPT_SSL_ENFORCE, &enforce);
		} else if (mode == StormByte::Database::SslMode::Require) {
			my_bool enforce = 1;
			mysql_options(conn, MYSQL_OPT_SSL_ENFORCE, &enforce);
		}
#else
		(void)mode;
#endif
	}
}

MariaDB::~MariaDB() noexcept {
	if (m_logger)
		*m_logger << Logger::Level::LowLevel << "MariaDB dtor" << std::endl;
	Disconnect();
}

MariaDB::MariaDB(std::string_view host, std::string_view user, std::string_view password,
				std::string_view db_name, int port, const StormByte::Safe::Shared<Logger::Log>& logger)
	: Database(logger), m_host(host), m_user(user), m_password(password),
	m_dbname(db_name), m_port(port),
	m_connection_handle(StormByte::Safe::Unique<ConnectionHandle>::MakePointer<ConnectionHandle>()) {
	Telemetry(StormByte::Safe::Shared<StormByte::Database::Telemetry>::MakePointer<StormByte::Database::MariaDB::Telemetry>());
}
MariaDB::MariaDB(MariaDB&& db) noexcept
	: Database(std::move(db)), m_host(std::move(db.m_host)), m_user(std::move(db.m_user)),
	m_password(std::move(db.m_password)), m_dbname(std::move(db.m_dbname)),
	m_port(db.m_port), m_connection_handle(std::move(db.m_connection_handle)) {
	db.m_connected = false;
}

MariaDB& MariaDB::operator=(MariaDB&& db) noexcept {
	if (this != &db) {
		Disconnect();
		Database::operator=(std::move(db));
		m_host = std::move(db.m_host);
		m_user = std::move(db.m_user);
		m_password = std::move(db.m_password);
		m_dbname = std::move(db.m_dbname);
		m_port = db.m_port;
		m_connection_handle = std::move(db.m_connection_handle);
		db.m_connected = false;
	}

	return *this;
}

bool MariaDB::DoConnect() noexcept {
	if (m_logger)
		*m_logger << Logger::Level::LowLevel << "MariaDB::DoConnect enter" << std::endl;
	if (m_connected)
		return false;
	if (static_cast<std::string_view>(m_host).find('\0') != std::string_view::npos ||
		static_cast<std::string_view>(m_user).find('\0') != std::string_view::npos ||
		static_cast<std::string_view>(m_password).find('\0') != std::string_view::npos ||
		static_cast<std::string_view>(m_dbname).find('\0') != std::string_view::npos)
		return false;
	MYSQL* conn = mysql_init(nullptr);
	if (!conn) {
		if (m_logger)
			*m_logger << Logger::Level::Error << "mysql_init failed" << std::endl;
		return false;
	}

	ApplySslMode(conn, m_ssl_mode);
	unsigned int port = static_cast<unsigned int>(m_port);
	if (!mysql_real_connect(conn,
							m_host.empty() ? nullptr : m_host.Bytes(),
							m_user.empty() ? nullptr : m_user.Bytes(),
							m_password.empty() ? nullptr : m_password.Bytes(),
							m_dbname.empty() ? nullptr : m_dbname.Bytes(),
							port, nullptr, 0)) {
		if (m_logger) {
			*m_logger << Logger::Level::Error
					<< "MariaDB connection error: "
					<< (mysql_error(conn) ? mysql_error(conn) : "Unknown error")
					<< std::endl;
		}

		mysql_close(conn);
		m_connection_handle->m_native_connection = nullptr;
		return false;
	}

	m_connection_handle->m_native_connection = conn;
	if (m_logger)
		*m_logger << Logger::Level::LowLevel << "MariaDB::DoConnect leave (ok)" << std::endl;
	return true;
}

void MariaDB::DoPreDisconnect() noexcept {
	ClearPreparedSTMTs();
}

void MariaDB::DoDisconnect() noexcept {
	MYSQL* conn = static_cast<MYSQL*>(m_connection_handle->m_native_connection);
	if (conn) {
		mysql_close(conn);
		m_connection_handle->m_native_connection = nullptr;
	}
}

StormByte::Database::ExpectedRows MariaDB::Query(std::string_view query) noexcept {
	auto telemetry = TrackOperation(Operation::Query);
	OperationGuard lock{*this};
	if (m_logger)
		*m_logger << Logger::Level::Debug << "Executing query: " << query << std::endl;
	MYSQL* conn = static_cast<MYSQL*>(m_connection_handle->m_native_connection);
	if (!m_connected || !conn) {
		RecordBackendEvent(BackendEvent::Connection);
		telemetry.Complete(false);
		return Unexpected<ExecuteError>("Database not connected");
	}
	if (query.size() > std::numeric_limits<unsigned long>::max()) {
		telemetry.Complete(false);
		return Unexpected<ExecuteError>("Query exceeds MariaDB's supported length");
	}
	if (mysql_real_query(conn, query.data(), static_cast<unsigned long>(query.size())) != 0) {
		if (auto* mariadb_telemetry = dynamic_cast<class Telemetry*>(m_telemetry.get()))
			mariadb_telemetry->RecordMariaDBError(mysql_errno(conn));
		telemetry.Complete(false);
		return Unexpected<ExecuteError>(mysql_error(conn) ? mysql_error(conn) : "Unknown MySQL error");
	}

	if (const unsigned int warnings = LogMariaDBWarnings(conn, m_logger); warnings > 0) {
		if (auto* mariadb_telemetry = dynamic_cast<class Telemetry*>(m_telemetry.get()))
			mariadb_telemetry->RecordMariaDBWarnings(warnings);
	}
	MYSQL_RES* res = mysql_store_result(conn);
	if (!res) {
		if (mysql_field_count(conn) == 0) {
			telemetry.Complete(true);
			return Rows();
		}
		if (auto* mariadb_telemetry = dynamic_cast<class Telemetry*>(m_telemetry.get()))
			mariadb_telemetry->RecordMariaDBError(mysql_errno(conn));
		telemetry.Complete(false);
		return Unexpected<ExecuteError>(mysql_error(conn) ? mysql_error(conn) : "Unknown MySQL error");
	}

	StormByte::Database::ExpectedRows rows = StormByte::Database::MariaDB::StepResults(res);
	mysql_free_result(res);
	telemetry.Complete(rows.has_value(), rows ? static_cast<std::uint64_t>(rows->Count()) : 0);
	return rows;
}

bool MariaDB::SilentQuery(std::string_view query) noexcept {
	auto telemetry = TrackOperation(Operation::SilentQuery);
	OperationGuard lock{*this};
	const bool result = DoSilentQuery(query);
	telemetry.Complete(result);
	return result;
}

bool MariaDB::DoSilentQuery(std::string_view query) noexcept {
	OperationGuard lock{*this};
	if (m_logger)
		*m_logger << Logger::Level::Debug << "Executing silent query: " << query << std::endl;
	MYSQL* conn = static_cast<MYSQL*>(m_connection_handle->m_native_connection);
	if (!m_connected || !conn) {
		RecordBackendEvent(BackendEvent::Connection);
		return false;
	}
	if (query.size() > std::numeric_limits<unsigned long>::max())
		return false;
	if (mysql_real_query(conn, query.data(), static_cast<unsigned long>(query.size())) != 0) {
		if (auto* mariadb_telemetry = dynamic_cast<class Telemetry*>(m_telemetry.get()))
			mariadb_telemetry->RecordMariaDBError(mysql_errno(conn));
		if (m_logger) {
			*m_logger << Logger::Level::Error
					<< "MariaDB SilentQuery error: "
					<< (mysql_error(conn) ? mysql_error(conn) : "Unknown MySQL error")
					<< std::endl;
		}

		return false;
	}

	if (const unsigned int warnings = LogMariaDBWarnings(conn, m_logger); warnings > 0) {
		if (auto* mariadb_telemetry = dynamic_cast<class Telemetry*>(m_telemetry.get()))
			mariadb_telemetry->RecordMariaDBWarnings(warnings);
	}
	return true;
}

StormByte::Safe::Unique<StormByte::Database::PreparedSTMT>
MariaDB::CreatePreparedSTMT(std::string_view name, std::string_view query) noexcept {
	MYSQL* conn = static_cast<MYSQL*>(m_connection_handle->m_native_connection);
	if (!conn)
		return nullptr;
	auto statement = StormByte::Safe::Unique<PreparedSTMT>::MakePointer<PreparedSTMT>(
		PreparedSTMT::ConstructionKey{}, name, query, m_logger, m_telemetry);
	statement->m_statement_handle->m_native_connection = conn;
	if (!statement->Initialize())
		return nullptr;
	return statement;
}

void MariaDB::DoBeginTransaction(IsolationLevel level) {
	const char* isolation_query = nullptr;
	switch (level) {
		case IsolationLevel::ReadUncommitted:
			isolation_query = "SET TRANSACTION ISOLATION LEVEL READ UNCOMMITTED;";
			break;
		case IsolationLevel::ReadCommitted:
			isolation_query = "SET TRANSACTION ISOLATION LEVEL READ COMMITTED;";
			break;
		case IsolationLevel::RepeatableRead:
			isolation_query = "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ;";
			break;
		case IsolationLevel::Serializable:
			isolation_query = "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE;";
			break;
		case IsolationLevel::Default:
		default:
			break;
	}

	if (isolation_query && !DoSilentQuery(isolation_query))
		throw ExecuteError("Unable to set transaction isolation level.");
	if (!DoSilentQuery("BEGIN;"))
		throw ExecuteError("Unable to begin transaction.");
}
