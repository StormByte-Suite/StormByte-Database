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

#include <StormByte/database/sqlite/sqlite3.hxx>
#include <StormByte/database/sqlite/result_fetch.hxx>
#include <StormByte/database/sqlite/prepared_stmt.hxx>
#include <sqlite3.h>
#include <atomic>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
using namespace StormByte::Database::SQLite;
namespace {
	std::atomic<int> g_sqlite_refcount{0};
	std::mutex g_sqlite_init_mutex;
}

SQLite3::SQLite3(const StormByte::Safe::Shared<Logger::Log>& logger)
	: SQLite3(StormByte::Safe::String(":memory:"), logger, Utf8Path{}) {}
SQLite3::SQLite3(const StormByte::Safe::String& dbfile, const StormByte::Safe::Shared<Logger::Log>& logger, Utf8Path)
	: Database(logger), m_database_file(dbfile), m_database(nullptr) {
	SetTelemetry(StormByte::Safe::Shared<StormByte::Database::Telemetry>::MakePointer<StormByte::Database::SQLite::Telemetry>());
}

SQLite3::SQLite3(SQLite3&& db) noexcept
	: Database(std::move(db)), m_database_file(std::move(db.m_database_file)),
	m_database(std::exchange(db.m_database, nullptr)) {
	db.m_connected = false;
}

SQLite3& SQLite3::operator=(SQLite3&& db) noexcept {
	if (this != &db) {
		Disconnect();
		Database::operator=(std::move(db));
		m_database_file = std::move(db.m_database_file);
		m_database = std::exchange(db.m_database, nullptr);
		db.m_connected = false;
	}

	return *this;
}

SQLite3::~SQLite3() noexcept {
	if (m_logger)
		*m_logger << Logger::Level::LowLevel << "SQLite3 dtor" << std::endl;
	Disconnect();
}

bool SQLite3::DoConnect() noexcept {
	if (m_logger)
		*m_logger << Logger::Level::LowLevel << "SQLite3::DoConnect enter" << std::endl;
	if (m_connected)
		return false;
	{
		std::lock_guard<std::mutex> lock(g_sqlite_init_mutex);
		if (g_sqlite_refcount == 0) {
			if (sqlite3_initialize() != SQLITE_OK) {
				if (m_logger)
					*m_logger << Logger::Level::Error << "sqlite3_initialize failed" << std::endl;
				return false;
			}
		}

		++g_sqlite_refcount;
	}

	if (sqlite3_open(static_cast<const char*>(m_database_file), &m_database) != SQLITE_OK) {
		if (m_logger) {
			*m_logger << Logger::Level::Error << "sqlite3_open failed: "
					<< (m_database ? sqlite3_errmsg(m_database) : "unknown") << std::endl;
		}

		if (m_database) {
			sqlite3_close(m_database);
			m_database = nullptr;
		}

		std::lock_guard<std::mutex> lock(g_sqlite_init_mutex);
		if (--g_sqlite_refcount == 0)
			sqlite3_shutdown();
		return false;
	}

	sqlite3_busy_timeout(m_database, 30000);
	if (m_logger)
		*m_logger << Logger::Level::LowLevel << "SQLite3::DoConnect leave (ok)" << std::endl;
	return true;
}

void SQLite3::DoPreDisconnect() noexcept {
	if (m_database)
		ClearPreparedSTMTs();
}

void SQLite3::DoDisconnect() noexcept {
	if (m_database) {
		sqlite3_close(m_database);
		m_database = nullptr;
	}
}

void SQLite3::DoPostDisconnect() noexcept {
	std::lock_guard<std::mutex> lock(g_sqlite_init_mutex);
	if (g_sqlite_refcount > 0) {
		if (--g_sqlite_refcount == 0)
			sqlite3_shutdown();
	}
}

StormByte::Database::ExpectedRows SQLite3::Query(std::string_view query) noexcept {
	auto telemetry = TrackOperation(Operation::Query);
	std::lock_guard<std::recursive_mutex> lock(*m_operation_mutex);
	if (m_logger)
		*m_logger << Logger::Level::Debug << "Executing query: " << query << std::endl;
	if (!m_connected) {
		RecordBackendEvent(BackendEvent::Connection);
		telemetry.Complete(false);
		return Unexpected<ExecuteError>("Database not connected");
	}
	if (query.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
		telemetry.Complete(false);
		return Unexpected<ExecuteError>("Query exceeds SQLite's supported length");
	}
	sqlite3_stmt* stmt = nullptr;
	const char* query_data = query.empty() ? "" : query.data();
	int rc = sqlite3_prepare_v2(m_database, query_data, static_cast<int>(query.size()), &stmt, nullptr);
	if (rc != SQLITE_OK) {
		if (auto* sqlite_telemetry = dynamic_cast<Telemetry*>(m_telemetry.get()))
			sqlite_telemetry->RecordSQLiteResult(rc);
		const std::string errorStr = sqlite3_errmsg(m_database);
		if (stmt)
			sqlite3_finalize(stmt);
		telemetry.Complete(false);
		return Unexpected<ExecuteError>(errorStr);
	}

	ExpectedRows result = StepResults(stmt);
	if (!result) {
		if (auto* sqlite_telemetry = dynamic_cast<Telemetry*>(m_telemetry.get()))
			sqlite_telemetry->RecordSQLiteResult(sqlite3_errcode(m_database));
	}
	sqlite3_finalize(stmt);
	telemetry.Complete(result.has_value(), result ? static_cast<std::uint64_t>(result->Count()) : 0);
	return result;
}

bool SQLite3::SilentQuery(std::string_view query) noexcept {
	auto telemetry = TrackOperation(Operation::SilentQuery);
	std::lock_guard<std::recursive_mutex> lock(*m_operation_mutex);
	const bool result = DoSilentQuery(query);
	telemetry.Complete(result);
	return result;
}

bool SQLite3::DoSilentQuery(std::string_view query) noexcept {
	std::lock_guard<std::recursive_mutex> lock(*m_operation_mutex);
	if (m_logger)
		*m_logger << Logger::Level::Debug << "Executing silent query: " << query << std::endl;
	if (!m_connected) {
		RecordBackendEvent(BackendEvent::Connection);
		return false;
	}
	const std::string query_text{query};
	char* errMsg = nullptr;
	int rc = sqlite3_exec(m_database, query_text.c_str(), nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK) {
		if (auto* sqlite_telemetry = dynamic_cast<Telemetry*>(m_telemetry.get()))
			sqlite_telemetry->RecordSQLiteResult(rc);
		if (errMsg) {
			if (m_logger) {
				*m_logger << Logger::Level::Error
						<< "SQLite3 SilentQuery error: " << errMsg << std::endl;
			}

			sqlite3_free(errMsg);
		}

		return false;
	}

	return true;
}

void SQLite3::EnableForeignKeys() {
	DoSilentQuery("PRAGMA foreign_keys = ON;");
}

StormByte::Safe::Unique<StormByte::Database::PreparedSTMT>
SQLite3::CreatePreparedSTMT(std::string_view name, std::string_view query) noexcept {
	StormByte::Safe::Unique<PreparedSTMT> stmt = StormByte::Safe::Unique<PreparedSTMT>::MakePointer<PreparedSTMT>(
		PreparedSTMT::ConstructionKey{}, name, query, m_logger, m_telemetry);
	if (stmt->Query().size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
		return nullptr;
	sqlite3_prepare_v2(m_database, stmt->Query().data(),
					static_cast<int>(stmt->Query().size()),
					&(stmt->m_stmt), nullptr);
	if (!stmt->m_stmt) {
		if (m_logger)
			*m_logger << Logger::Level::Error << "Failed to prepare statement" << std::endl;
		return nullptr;
	}

	return stmt;
}

void SQLite3::DoBeginTransaction(IsolationLevel level) {
	const char* query = "BEGIN DEFERRED;";
	switch (level) {
		case IsolationLevel::ReadUncommitted:
		case IsolationLevel::ReadCommitted:
		case IsolationLevel::Default:
			break;
		case IsolationLevel::RepeatableRead:
			query = "BEGIN IMMEDIATE;";
			break;
		case IsolationLevel::Serializable:
			query = "BEGIN EXCLUSIVE;";
			break;
	}

	if (!DoSilentQuery(query))
		throw ExecuteError("Unable to begin transaction.");
}
