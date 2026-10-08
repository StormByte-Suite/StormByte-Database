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

#include <StormByte/database/sqlite/telemetry.hxx>
#include <sqlite3.h>

using namespace StormByte::Database::SQLite;

Telemetry::Telemetry() = default;
Telemetry::~Telemetry() noexcept = default;

std::uint64_t Telemetry::BusyErrors() const noexcept { return m_busy_errors.load(StormByte::Safe::MemoryOrder::Acquire); }
std::uint64_t Telemetry::ConstraintErrors() const noexcept { return m_constraint_errors.load(StormByte::Safe::MemoryOrder::Acquire); }
std::uint64_t Telemetry::IoErrors() const noexcept { return m_io_errors.load(StormByte::Safe::MemoryOrder::Acquire); }
std::uint64_t Telemetry::CorruptionErrors() const noexcept { return m_corruption_errors.load(StormByte::Safe::MemoryOrder::Acquire); }

void Telemetry::RecordEvent(const BackendEvent event) noexcept {
	StormByte::Database::Telemetry::RecordEvent(event);
	switch (event) {
		case BackendEvent::Busy: m_busy_errors.fetch_add(std::uint64_t{1}, StormByte::Safe::MemoryOrder::Relaxed); break;
		case BackendEvent::Constraint: m_constraint_errors.fetch_add(std::uint64_t{1}, StormByte::Safe::MemoryOrder::Relaxed); break;
		case BackendEvent::Io: m_io_errors.fetch_add(std::uint64_t{1}, StormByte::Safe::MemoryOrder::Relaxed); break;
		default: break;
	}
}

void Telemetry::RecordSQLiteResult(const int result_code) noexcept {
	switch (result_code & 0xff) {
		case SQLITE_BUSY:
		case SQLITE_LOCKED: RecordEvent(BackendEvent::Busy); break;
		case SQLITE_CONSTRAINT: RecordEvent(BackendEvent::Constraint); break;
		case SQLITE_IOERR: RecordEvent(BackendEvent::Io); break;
		case SQLITE_CORRUPT:
		case SQLITE_NOTADB:
			m_corruption_errors.fetch_add(std::uint64_t{1}, StormByte::Safe::MemoryOrder::Relaxed);
			RecordEvent(BackendEvent::Other);
			break;
		default: RecordEvent(BackendEvent::Other); break;
	}
}

Telemetry::operator StormByte::Safe::String() const {
	std::string text{static_cast<std::string_view>(StormByte::Database::Telemetry::operator StormByte::Safe::String())};
	text += " SQLite{busy=" + std::to_string(BusyErrors());
	text += ",constraints=" + std::to_string(ConstraintErrors());
	text += ",io=" + std::to_string(IoErrors());
	text += ",corruption=" + std::to_string(CorruptionErrors()) + "}";
	return StormByte::Safe::String(std::string_view{text});
}
