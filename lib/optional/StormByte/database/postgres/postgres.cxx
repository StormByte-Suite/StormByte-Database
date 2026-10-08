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

#include <StormByte/database/postgres/postgres.hxx>
#include <StormByte/database/postgres/result_fetch.hxx>
#include <StormByte/database/postgres/prepared_stmt.hxx>
#include <StormByte/database/engine_handles.hxx>
#include <libpq-fe.h>
#include <cctype>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
using namespace StormByte::Database::Postgres;
namespace {
	void PostgresNoticeProcessor(void* arg, const char* message) {
		auto* log = static_cast<StormByte::Logger::Log*>(arg);
		if (!log || !message)
			return;
		std::string msg(message);
		while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
			msg.pop_back();
		if (!msg.empty())
			*log << StormByte::Logger::Level::Notice << std::string_view{msg} << std::endl;
	}
}

Postgres::Postgres(std::string_view host, std::string_view user, std::string_view password,
				std::string_view db_name, const StormByte::Safe::Shared<Logger::Log>& logger)
	: Database(logger), m_host(host), m_user(user), m_password(password),
	m_dbname(db_name),
	m_connection_handle(StormByte::Safe::Unique<ConnectionHandle>::MakePointer<ConnectionHandle>()) {
	Telemetry(StormByte::Safe::Shared<StormByte::Database::Telemetry>::MakePointer<StormByte::Database::Postgres::Telemetry>());
}

Postgres::Postgres(Postgres&& db) noexcept
	: Database(std::move(db)), m_host(std::move(db.m_host)), m_user(std::move(db.m_user)),
	m_password(std::move(db.m_password)), m_dbname(std::move(db.m_dbname)),
	m_connection_handle(std::move(db.m_connection_handle)) {
	db.m_connected = false;
}

Postgres& Postgres::operator=(Postgres&& db) noexcept {
	if (this != &db) {
		Disconnect();
		Database::operator=(std::move(db));
		m_host = std::move(db.m_host);
		m_user = std::move(db.m_user);
		m_password = std::move(db.m_password);
		m_dbname = std::move(db.m_dbname);
		m_connection_handle = std::move(db.m_connection_handle);
		db.m_connected = false;
	}

	return *this;
}

Postgres::~Postgres() noexcept {
	if (m_logger)
		*m_logger << Logger::Level::LowLevel << "Postgres dtor" << std::endl;
	Disconnect();
}

bool Postgres::DoConnect() noexcept {
	if (m_logger)
		*m_logger << Logger::Level::LowLevel << "Postgres::DoConnect enter" << std::endl;
	if (m_connected)
		return false;
	if (static_cast<std::string_view>(m_host).find('\0') != std::string_view::npos ||
		static_cast<std::string_view>(m_user).find('\0') != std::string_view::npos ||
		static_cast<std::string_view>(m_password).find('\0') != std::string_view::npos ||
		static_cast<std::string_view>(m_dbname).find('\0') != std::string_view::npos)
		return false;
	const char* ssl_mode = nullptr;
	switch (m_ssl_mode) {
		case SslMode::Disable:
			ssl_mode = "disable";
			break;
		case SslMode::Prefer:
			ssl_mode = "prefer";
			break;
		case SslMode::Require:
			ssl_mode = "require";
			break;
		case SslMode::Default:
		default:
			break;
	}

	const char* keywords[] = {"host", "user", "password", "dbname", "sslmode", nullptr};
	const char* values[] = {
		m_host.empty() ? nullptr : m_host.Bytes(),
		m_user.empty() ? nullptr : m_user.Bytes(),
		m_password.empty() ? nullptr : m_password.Bytes(),
		m_dbname.empty() ? nullptr : m_dbname.Bytes(),
		ssl_mode,
		nullptr
	};
	PGconn* conn = PQconnectdbParams(keywords, values, 0);
	if (!conn) {
		if (m_logger)
			*m_logger << Logger::Level::Error << "PQconnectdb returned null" << std::endl;
		return false;
	}

	if (PQstatus(conn) != CONNECTION_OK) {
		if (m_logger) {
			*m_logger << Logger::Level::Error
					<< "Postgres connection error: "
					<< (PQerrorMessage(conn) ? PQerrorMessage(conn) : "Unknown error")
					<< std::endl;
		}

		PQfinish(conn);
		m_connection_handle->m_native_connection = nullptr;
		return false;
	}

	m_connection_handle->m_native_connection = conn;
	if (m_logger)
		PQsetNoticeProcessor(conn, PostgresNoticeProcessor, m_logger.get());
	if (m_logger)
		*m_logger << Logger::Level::LowLevel << "Postgres::DoConnect leave (ok)" << std::endl;
	return true;
}

void Postgres::DoPreDisconnect() noexcept {
	ClearPreparedSTMTs();
}

void Postgres::DoDisconnect() noexcept {
	PGconn* conn = static_cast<PGconn*>(m_connection_handle->m_native_connection);
	if (conn) {
		PQfinish(conn);
		m_connection_handle->m_native_connection = nullptr;
	}
}

StormByte::Database::ExpectedRows Postgres::Query(std::string_view query) noexcept {
	auto telemetry = TrackOperation(Operation::Query);
	OperationGuard lock{*this};
	if (m_logger)
		*m_logger << Logger::Level::Debug << "Executing query: " << query << std::endl;
	PGconn* conn = static_cast<PGconn*>(m_connection_handle->m_native_connection);
	if (!m_connected || !conn) {
		RecordBackendEvent(BackendEvent::Connection);
		telemetry.Complete(false);
		return Unexpected<ExecuteError>("Database not connected");
	}
	if (query.find('\0') != std::string_view::npos) {
		telemetry.Complete(false);
		return Unexpected<ExecuteError>("Query contains an embedded NUL character");
	}
	const std::string query_text{query};
	PGresult* res = PQexec(conn, query_text.c_str());
	if (!res) {
		RecordBackendEvent(BackendEvent::Connection);
		telemetry.Complete(false);
		return Unexpected<ExecuteError>("Null PGresult");
	}
	ExecStatusType st = PQresultStatus(res);
	if (st != PGRES_TUPLES_OK && st != PGRES_COMMAND_OK) {
		const char* sql_state = PQresultErrorField(res, PG_DIAG_SQLSTATE);
		if (auto* postgres_telemetry = dynamic_cast<class Telemetry*>(m_telemetry.get()))
			postgres_telemetry->RecordSqlState(sql_state ? std::string_view{sql_state} : std::string_view{});
		std::string err = PQerrorMessage(conn)
						? PQerrorMessage(conn)
						: "Unknown Postgres error";
		PQclear(res);
		return Unexpected<ExecuteError>(err);
	}

	ExpectedRows rows = StepResults(res);
	PQclear(res);
	telemetry.Complete(rows.has_value(), rows ? static_cast<std::uint64_t>(rows->Count()) : 0);
	return rows;
}

bool Postgres::SilentQuery(std::string_view query) noexcept {
	auto telemetry = TrackOperation(Operation::SilentQuery);
	OperationGuard lock{*this};
	const bool result = DoSilentQuery(query);
	telemetry.Complete(result);
	return result;
}

bool Postgres::DoSilentQuery(std::string_view query) noexcept {
	OperationGuard lock{*this};
	if (m_logger)
		*m_logger << Logger::Level::Debug << "Executing silent query: " << query << std::endl;
	PGconn* conn = static_cast<PGconn*>(m_connection_handle->m_native_connection);
	if (!m_connected || !conn) {
		RecordBackendEvent(BackendEvent::Connection);
		return false;
	}
	if (query.find('\0') != std::string_view::npos)
		return false;
	const std::string query_text{query};
	PGresult* res = PQexec(conn, query_text.c_str());
	if (!res) {
		RecordBackendEvent(BackendEvent::Connection);
		return false;
	}
	ExecStatusType st = PQresultStatus(res);
	if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK) {
		const char* sql_state = PQresultErrorField(res, PG_DIAG_SQLSTATE);
		if (auto* postgres_telemetry = dynamic_cast<class Telemetry*>(m_telemetry.get()))
			postgres_telemetry->RecordSqlState(sql_state ? std::string_view{sql_state} : std::string_view{});
		if (m_logger) {
			*m_logger << Logger::Level::Error
					<< "Postgres SilentQuery error: "
					<< (PQerrorMessage(conn)
							? PQerrorMessage(conn)
							: "Unknown error")
					<< std::endl;
		}

		PQclear(res);
		return false;
	}

	PQclear(res);
	return true;
}

StormByte::Safe::Unique<StormByte::Database::PreparedSTMT>
Postgres::CreatePreparedSTMT(std::string_view name, std::string_view query) noexcept {
	PGconn* conn = static_cast<PGconn*>(m_connection_handle->m_native_connection);
	if (!conn)
		return nullptr;
	if (name.find('\0') != std::string_view::npos || query.find('\0') != std::string_view::npos)
		return nullptr;
	const std::string name_copy{name};
	std::string qcopy{query};
	while (!qcopy.empty() &&
		(qcopy.back() == ';' || isspace(static_cast<unsigned char>(qcopy.back())))) {
		qcopy.pop_back();
	}

	PGresult* res = PQprepare(conn, name_copy.c_str(), qcopy.c_str(), 0, nullptr);
	if (!res) {
		if (m_logger) {
			*m_logger << Logger::Level::Error
					<< "PQprepare returned null for statement '" << std::string_view{name_copy} << "'"
					<< std::endl;
		}

		return nullptr;
	}

	ExecStatusType st = PQresultStatus(res);
	if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK) {
		if (m_logger) {
			*m_logger << Logger::Level::Error
					<< "PQprepare error for statement '" << std::string_view{name_copy} << "': "
					<< (PQresultErrorMessage(res) ? PQresultErrorMessage(res) : "Unknown")
					<< std::endl;
		}

		PQclear(res);
		return nullptr;
	}

	PQclear(res);
	StormByte::Safe::Unique<PreparedSTMT> stmt = StormByte::Safe::Unique<PreparedSTMT>::MakePointer<PreparedSTMT>(
		PreparedSTMT::ConstructionKey{}, name, query, m_logger, m_telemetry);
	stmt->m_statement_handle->m_native_connection = conn;
	return stmt;
}

void Postgres::DoBeginTransaction(IsolationLevel level) {
	const char* query = "BEGIN;";
	switch (level) {
		case IsolationLevel::ReadUncommitted:
			query = "BEGIN ISOLATION LEVEL READ UNCOMMITTED;";
			break;
		case IsolationLevel::ReadCommitted:
			query = "BEGIN ISOLATION LEVEL READ COMMITTED;";
			break;
		case IsolationLevel::RepeatableRead:
			query = "BEGIN ISOLATION LEVEL REPEATABLE READ;";
			break;
		case IsolationLevel::Serializable:
			query = "BEGIN ISOLATION LEVEL SERIALIZABLE;";
			break;
		case IsolationLevel::Default:
		default:
			break;
	}

	if (!DoSilentQuery(query))
		throw ExecuteError("Unable to begin transaction.");
}
