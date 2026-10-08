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
#include <StormByte/database/mssql/prepared_stmt.hxx>
#include <StormByte/database/engine_handles.hxx>
#include <sybdb.h>

#include <limits>
#include <utility>

using namespace StormByte::Database::MSSQL;

PreparedSTMT::PreparedSTMT(ConstructionKey, const std::string_view name, const std::string_view query,
		const StormByte::Safe::Shared<Logger::Log>& logger,
		const StormByte::Safe::Shared<StormByte::Database::Telemetry>& telemetry):
	StormByte::Database::PreparedSTMT(name, query, logger, telemetry),
	m_statement_handle(StormByte::Safe::Unique<StatementHandle>::MakePointer<StatementHandle>()),
	m_bind_error(false) {}

PreparedSTMT::PreparedSTMT(PreparedSTMT&& other) noexcept:
	StormByte::Database::PreparedSTMT(std::move(other)),
	m_statement_handle(std::move(other.m_statement_handle)),
	m_parameters(std::move(other.m_parameters)), m_bind_error(other.m_bind_error) {}

PreparedSTMT::~PreparedSTMT() noexcept = default;

PreparedSTMT& PreparedSTMT::operator=(PreparedSTMT&& other) noexcept {
	if (this != &other) {
		StormByte::Database::PreparedSTMT::operator=(std::move(other));
		m_statement_handle = std::move(other.m_statement_handle);
		m_parameters = std::move(other.m_parameters);
		m_bind_error = other.m_bind_error;
	}
	return *this;
}

void PreparedSTMT::Binder(const StormByte::Size index, Value&& value) noexcept {
	if (index >= StormByte::Size{std::numeric_limits<int>::max()}) {
		m_bind_error = true;
		return;
	}
	try {
		const std::size_t position = static_cast<std::size_t>(index);
		if (position >= m_parameters.size())
			m_parameters.resize(position + 1);
		m_parameters[position] = std::move(value);
	} catch (...) {
		m_bind_error = true;
	}
}

void PreparedSTMT::Reset() noexcept {
	m_parameters.clear();
	m_bind_error = false;
}

StormByte::Database::ExpectedRows PreparedSTMT::DoExecute() {
	if (m_bind_error)
		return StormByte::Unexpected<QueryException>(ExecuteError("MSSQL prepared statement has invalid bindings"));
	if (!m_statement_handle || !m_statement_handle->m_native_connection)
		return StormByte::Unexpected<QueryException>(ExecuteError("MSSQL prepared statement has no connection"));
	DBPROCESS* process = static_cast<DBPROCESS*>(m_statement_handle->m_native_connection);
	auto* owner = reinterpret_cast<MSSQL*>(dbgetuserdata(process));
	if (!owner)
		return StormByte::Unexpected<QueryException>(ExecuteError("MSSQL prepared statement connection is no longer available"));
	return owner->ExecuteParameterized(Query(), m_parameters);
}
