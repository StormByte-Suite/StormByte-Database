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

#include <StormByte/database/exception.hxx>

using namespace StormByte::Database;

Exception::Exception(std::string_view message):
	StormByte::Exception(StormByte::Exception::Path{"Database"}, "{}", message) {}

Exception::~Exception() noexcept = default;

StormByte::Safe::String Exception::ComponentPath(const std::string_view prefix, const std::string_view component) {
	StormByte::Safe::String path{prefix};
	path.append(component);
	return path;
}

ConnectionError::ConnectionError(std::string_view error):
	Exception(StormByte::Exception::Path{"Database.Connection"}, "{}", error) {}

ConnectionError::~ConnectionError() noexcept = default;

WrongValueType::WrongValueType(std::string_view error):
	Exception(StormByte::Exception::Path{"Database.WrongValueType"}, "{}", error) {}

WrongValueType::~WrongValueType() noexcept = default;

ColumnNotFound::ColumnNotFound(std::string_view column):
	Exception(StormByte::Exception::Path{"Database.ColumnNotFound"}, "Column '{}' not found", column) {}

ColumnNotFound::~ColumnNotFound() noexcept = default;

OutOfBounds::OutOfBounds(StormByte::Size pos, StormByte::Size size):
	Exception(StormByte::Exception::Path{"Database.OutOfBounds"}, "Position {} is out of bounds for size {}",
		static_cast<std::size_t>(pos), static_cast<std::size_t>(size)) {}

OutOfBounds::~OutOfBounds() noexcept = default;

QueryException::QueryException(std::string_view error):
	Exception(StormByte::Exception::Path{"Database.Query"}, "{}", error) {}

QueryException::~QueryException() noexcept = default;

UnknownSTMT::UnknownSTMT(std::string_view name):
	QueryException("PreparedSTMT", "Statement '{}' not found", name) {}

UnknownSTMT::~UnknownSTMT() noexcept = default;

ExecuteError::ExecuteError(std::string_view error):
	QueryException("Execute", "Error executing query: {}", error) {}

ExecuteError::~ExecuteError() noexcept = default;

TransactionError::TransactionError(std::string_view error):
	Exception(StormByte::Exception::Path{"Database.Transaction"}, "Unable to begin transaction: {}", error) {}

TransactionError::~TransactionError() noexcept = default;
