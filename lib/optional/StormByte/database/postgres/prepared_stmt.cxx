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

#include <StormByte/database/postgres/prepared_stmt.hxx>
#include <StormByte/database/postgres/result_fetch.hxx>
#include <StormByte/database/postgres/telemetry.hxx>
#include <libpq-fe.h>
#include <limits>
#include <string_view>
#include <utility>

using namespace StormByte::Database::Postgres;

PreparedSTMT::PreparedSTMT(ConstructionKey, std::string_view name, std::string_view query,
		const StormByte::Safe::Shared<Logger::Log>& logger,
		const StormByte::Safe::Shared<StormByte::Database::Telemetry>& telemetry)
	: StormByte::Database::PreparedSTMT(name, query, logger, telemetry), m_conn(nullptr), m_stmt_name(name) {}

PreparedSTMT::PreparedSTMT(PreparedSTMT&& other) noexcept:
	StormByte::Database::PreparedSTMT(std::move(other)), m_conn(std::exchange(other.m_conn, nullptr)),
	m_stmt_name(std::move(other.m_stmt_name)), m_params(std::move(other.m_params)) {}

PreparedSTMT::~PreparedSTMT() noexcept = default;

PreparedSTMT& PreparedSTMT::operator=(PreparedSTMT&& other) noexcept {
	if (this != &other) {
		StormByte::Database::PreparedSTMT::operator=(std::move(other));
		m_conn = std::exchange(other.m_conn, nullptr);
		m_stmt_name = std::move(other.m_stmt_name);
		m_params = std::move(other.m_params);
	}
	return *this;
}

void PreparedSTMT::Binder(StormByte::Size index, Value&& value) noexcept {
	if (index >= StormByte::Size{m_params.size()}) {
		if (index >= StormByte::Size{std::numeric_limits<int>::max()})
			return;
		m_params.resize(static_cast<std::size_t>(index) + 1);
	}
	m_params[static_cast<std::size_t>(index)] = std::move(value);
}

void PreparedSTMT::Reset() noexcept {
	m_params.clear();
}

StormByte::Database::ExpectedRows PreparedSTMT::DoExecute() {
	if (!m_conn)
		return Unexpected<ExecuteError>("No connection available for prepared statement");
	const StormByte::Size parameter_count{m_params.size()};
	if (parameter_count > StormByte::Size{std::numeric_limits<int>::max()})
		return Unexpected<ExecuteError>("Too many PostgreSQL bind parameters");
	const int parameter_count_int = static_cast<int>(parameter_count);
	const std::size_t parameter_count_stl = static_cast<std::size_t>(parameter_count);
	std::vector<const char*> params(parameter_count_stl);
	std::vector<int> lengths(parameter_count_stl);
	std::vector<int> formats(parameter_count_stl);
	std::vector<std::string> string_storage(parameter_count_stl);
	std::vector<StormByte::BinaryData> blob_storage(parameter_count_stl);
	for (StormByte::Size index{}; index < parameter_count; ++index) {
		const std::size_t stl_index = static_cast<std::size_t>(index);
		const Value& value = m_params[stl_index];
		if (value.IsNull())
			continue;
		switch (value.Type()) {
			case Value::Type::Integer:
				string_storage[stl_index] = std::to_string(value.Get<int>());
				break;
			case Value::Type::UnsignedInteger:
				string_storage[stl_index] = std::to_string(value.Get<unsigned int>());
				break;
			case Value::Type::LongInteger:
				string_storage[stl_index] = std::to_string(value.Get<long long int>());
				break;
			case Value::Type::UnsignedLongInteger:
				string_storage[stl_index] = std::to_string(value.Get<unsigned long long int>());
				break;
			case Value::Type::Double:
				string_storage[stl_index] = std::to_string(value.Get<double>());
				break;
			case Value::Type::Boolean:
				string_storage[stl_index] = value.Get<bool>() ? "true" : "false";
				break;
			case Value::Type::Text: {
				const auto text = value.Get<StormByte::Safe::String>();
				const std::string_view text_view = text;
				if (text_view.find('\0') != std::string_view::npos)
					return Unexpected<ExecuteError>("PostgreSQL text bind contains an embedded NUL character");
				string_storage[stl_index] = text_view;
				break;
			}
			case Value::Type::Blob: {
				blob_storage[stl_index] = value.Get<StormByte::BinaryData>();
				if (blob_storage[stl_index].size() > StormByte::ByteSize{std::numeric_limits<int>::max()})
					return Unexpected<ExecuteError>("PostgreSQL bind blob exceeds supported length");
				params[stl_index] = blob_storage[stl_index].empty()
					? ""
					: reinterpret_cast<const char*>(blob_storage[stl_index].data());
				lengths[stl_index] = static_cast<int>(blob_storage[stl_index].size());
				formats[stl_index] = 1;
				continue;
			}

			case Value::Type::Null:
			default:
				continue;
		}

		params[stl_index] = string_storage[stl_index].c_str();
	}

	PGresult* res = PQexecPrepared(m_conn, m_stmt_name.Bytes(), parameter_count_int, params.data(), lengths.data(), formats.data(), 0);
	if (!res) {
		RecordBackendEvent(BackendEvent::Connection);
		return Unexpected<ExecuteError>("Null PGresult from PQexecPrepared");
	}

	ExecStatusType st = PQresultStatus(res);
	if (st != PGRES_TUPLES_OK && st != PGRES_COMMAND_OK) {
		const char* sql_state = PQresultErrorField(res, PG_DIAG_SQLSTATE);
		if (auto* postgres_telemetry = dynamic_cast<Telemetry*>(m_telemetry.get()))
			postgres_telemetry->RecordSqlState(sql_state ? std::string_view{sql_state} : std::string_view{});
		std::string err = PQerrorMessage(m_conn) ? PQerrorMessage(m_conn) : "Unknown Postgres error";
		PQclear(res);
		return Unexpected<ExecuteError>(err);
	}

	ExpectedRows rows = StepResults(res);
	PQclear(res);
	return rows;
}
