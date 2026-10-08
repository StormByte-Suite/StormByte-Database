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

#include <StormByte/database/sqlite/prepared_stmt.hxx>
#include <StormByte/database/sqlite/result_fetch.hxx>
#include <StormByte/database/sqlite/telemetry.hxx>
#include <StormByte/database/engine_handles.hxx>

#include <limits>
#include <string_view>
#include <utility>

using namespace StormByte::Database::SQLite;
PreparedSTMT::PreparedSTMT(ConstructionKey, std::string_view name, std::string_view query,
		const StormByte::Safe::Shared<Logger::Log>& logger,
		const StormByte::Safe::Shared<StormByte::Database::Telemetry>& telemetry)
	: StormByte::Database::PreparedSTMT(name, query, logger, telemetry),
	  m_statement_handle(StormByte::Safe::Unique<StatementHandle>::MakePointer<StatementHandle>()), m_bind_error(false) {}

PreparedSTMT::PreparedSTMT(PreparedSTMT&& other) noexcept:
	StormByte::Database::PreparedSTMT(std::move(other)), m_statement_handle(std::move(other.m_statement_handle)),
	m_bind_error(std::exchange(other.m_bind_error, false)) {}

PreparedSTMT::~PreparedSTMT() noexcept {
	auto* statement = m_statement_handle ? static_cast<sqlite3_stmt*>(m_statement_handle->m_native_statement) : nullptr;
	if (statement) {
		sqlite3_finalize(statement);
		m_statement_handle->m_native_statement = nullptr;
	}
}

PreparedSTMT& PreparedSTMT::operator=(PreparedSTMT&& other) noexcept {
	if (this != &other) {
		auto* statement = m_statement_handle ? static_cast<sqlite3_stmt*>(m_statement_handle->m_native_statement) : nullptr;
		if (statement)
			sqlite3_finalize(statement);
		StormByte::Database::PreparedSTMT::operator=(std::move(other));
		m_statement_handle = std::move(other.m_statement_handle);
		m_bind_error = std::exchange(other.m_bind_error, false);
	}
	return *this;
}

void PreparedSTMT::Binder(StormByte::Size index, Value&& value) noexcept {
	auto* statement = static_cast<sqlite3_stmt*>(m_statement_handle->m_native_statement);
	if (!statement) return;
	if (index >= StormByte::Size{sqlite3_bind_parameter_count(statement)}) {
		m_bind_error = true;
		return;
	}
	const int col = static_cast<int>(index) + 1;
	int result = SQLITE_OK;
	if (value.IsNull()) {
		result = sqlite3_bind_null(statement, col);
		m_bind_error = m_bind_error || result != SQLITE_OK;
		return;
	}

	switch (value.Type()) {
		case Value::Type::Integer:
			result = sqlite3_bind_int(statement, col, value.Get<int>());
			break;
		case Value::Type::UnsignedInteger:
			result = sqlite3_bind_int64(statement, col, static_cast<sqlite3_int64>(value.Get<unsigned int>()));
			break;
		case Value::Type::LongInteger:
			result = sqlite3_bind_int64(statement, col, value.Get<long long int>());
			break;
		case Value::Type::UnsignedLongInteger:
			if (value.Get<unsigned long long int>() > static_cast<unsigned long long int>(std::numeric_limits<sqlite3_int64>::max())) {
				m_bind_error = true;
				return;
			}

			result = sqlite3_bind_int64(statement, col, static_cast<sqlite3_int64>(value.Get<unsigned long long int>()));
			break;
		case Value::Type::Double:
			result = sqlite3_bind_double(statement, col, value.Get<double>());
			break;
		case Value::Type::Boolean:
			result = sqlite3_bind_int(statement, col, value.Get<bool>() ? 1 : 0);
			break;
		case Value::Type::Text: {
			const auto text = value.Get<StormByte::Safe::String>();
			const std::string_view text_view = text;
			if (text_view.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
				m_bind_error = true;
				return;
			}
			result = sqlite3_bind_text(statement, col, text_view.data(), static_cast<int>(text_view.size()), SQLITE_TRANSIENT);
			break;
		}

		case Value::Type::Blob: {
			auto blob = value.Get<StormByte::Safe::Binary>();
			if (blob.empty()) {
				result = sqlite3_bind_zeroblob(statement, col, 0);
			} else {
				if (blob.size() > StormByte::ByteSize{std::numeric_limits<int>::max()}) {
					m_bind_error = true;
					return;
				}
				result = sqlite3_bind_blob(statement, col, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT);
			}

			break;
		}

		default:
			result = sqlite3_bind_null(statement, col);
			break;
	}

	m_bind_error = m_bind_error || result != SQLITE_OK;
}

void PreparedSTMT::Reset() noexcept {
	m_bind_error = false;
	auto* statement = static_cast<sqlite3_stmt*>(m_statement_handle->m_native_statement);
	if (statement) {
		sqlite3_clear_bindings(statement);
		sqlite3_reset(statement);
	}
}

StormByte::Database::ExpectedRows PreparedSTMT::DoExecute() {
	auto* statement = static_cast<sqlite3_stmt*>(m_statement_handle->m_native_statement);
	if (m_bind_error) {
		const int result_code = sqlite3_errcode(sqlite3_db_handle(statement));
		if (result_code != SQLITE_OK) {
			if (auto* sqlite_telemetry = dynamic_cast<Telemetry*>(m_telemetry.get()))
				sqlite_telemetry->RecordSQLiteResult(result_code);
		}
		return Unexpected<ExecuteError>("Invalid SQLite statement bind.");
	}
	ExpectedRows result = StepResults(statement);
	if (!result) {
		if (auto* sqlite_telemetry = dynamic_cast<Telemetry*>(m_telemetry.get()))
			sqlite_telemetry->RecordSQLiteResult(sqlite3_errcode(sqlite3_db_handle(statement)));
	}
	return result;
}
