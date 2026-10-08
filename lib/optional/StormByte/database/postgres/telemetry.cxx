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

#include <StormByte/database/postgres/telemetry.hxx>

using namespace StormByte::Database::Postgres;

Telemetry::Telemetry() = default;
Telemetry::~Telemetry() noexcept = default;

std::uint64_t Telemetry::SerializationConflicts() const noexcept { return m_serialization_conflicts.load(StormByte::Safe::MemoryOrder::Acquire); }
std::uint64_t Telemetry::Deadlocks() const noexcept { return m_deadlocks.load(StormByte::Safe::MemoryOrder::Acquire); }
std::uint64_t Telemetry::ConnectionErrors() const noexcept { return m_connection_errors.load(StormByte::Safe::MemoryOrder::Acquire); }

void Telemetry::RecordEvent(const BackendEvent event) noexcept {
	StormByte::Database::Telemetry::RecordEvent(event);
	switch (event) {
		case BackendEvent::Serialization: m_serialization_conflicts.fetch_add(std::uint64_t{1}, StormByte::Safe::MemoryOrder::Relaxed); break;
		case BackendEvent::Deadlock: m_deadlocks.fetch_add(std::uint64_t{1}, StormByte::Safe::MemoryOrder::Relaxed); break;
		case BackendEvent::Connection: m_connection_errors.fetch_add(std::uint64_t{1}, StormByte::Safe::MemoryOrder::Relaxed); break;
		default: break;
	}
}

void Telemetry::RecordSqlState(const std::string_view sql_state) noexcept {
	if (sql_state == "40001")
		RecordEvent(BackendEvent::Serialization);
	else if (sql_state == "40P01")
		RecordEvent(BackendEvent::Deadlock);
	else if (sql_state.size() >= 2 && sql_state.substr(0, 2) == "23")
		RecordEvent(BackendEvent::Constraint);
	else if (sql_state.size() >= 2 && sql_state.substr(0, 2) == "08")
		RecordEvent(BackendEvent::Connection);
	else if (sql_state == "55P03")
		RecordEvent(BackendEvent::Busy);
	else
		RecordEvent(BackendEvent::Other);
}

Telemetry::operator StormByte::Safe::String() const {
	std::string text{static_cast<std::string_view>(StormByte::Database::Telemetry::operator StormByte::Safe::String())};
	text += " PostgreSQL{serialization=" + std::to_string(SerializationConflicts());
	text += ",deadlocks=" + std::to_string(Deadlocks());
	text += ",connection_errors=" + std::to_string(ConnectionErrors()) + "}";
	return StormByte::Safe::String(std::string_view{text});
}
