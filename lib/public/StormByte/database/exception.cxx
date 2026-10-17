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

#include <cstdint>
#include <string>

using namespace StormByte::Database;

namespace {
	std::string FullPath(std::string_view extra) {
		std::string path = "Database";
		if (!extra.empty()) {
			path.push_back('.');
			path.append(extra);
		}
		return path;
	}
}

Exception::Exception(std::string_view message)
	: StormByte::Exception(StormByte::Exception::Path{"Database"}, message) {}

Exception::Exception(Path path, std::string_view message)
	: StormByte::Exception(StormByte::Exception::Path{FullPath(path.segments)}, message) {}

Exception::~Exception() noexcept = default;

ConnectionError::ConnectionError(std::string_view error)
	: Exception(Path{"Connection"}, error) {}

ConnectionError::~ConnectionError() noexcept = default;

WrongValueType::WrongValueType(std::string_view error)
	: Exception(Path{"WrongValueType"}, error) {}

WrongValueType::~WrongValueType() noexcept = default;

ColumnNotFound::ColumnNotFound(std::string_view column)
	: Exception(Path{"ColumnNotFound"}, static_cast<std::string_view>(StormByte::Safe::String::ReplaceToken("Column '{}' not found", column))) {}

ColumnNotFound::~ColumnNotFound() noexcept = default;

OutOfBounds::OutOfBounds(StormByte::Size pos, StormByte::Size size)
	: Exception(Path{"OutOfBounds"}, static_cast<std::string_view>(StormByte::Safe::String::ReplaceToken(
		"Position {} is out of bounds for size {}",
		static_cast<std::uint64_t>(pos),
		static_cast<std::uint64_t>(size)))) {}

OutOfBounds::~OutOfBounds() noexcept = default;

QueryException::QueryException(std::string_view error)
	: Exception(Path{"Query"}, error) {}

QueryException::~QueryException() noexcept = default;

UnknownSTMT::UnknownSTMT(std::string_view name)
	: QueryException("PreparedSTMT", "Statement '{}' not found", name) {}

UnknownSTMT::~UnknownSTMT() noexcept = default;

ExecuteError::ExecuteError(std::string_view error)
	: QueryException("Execute", "Error executing query: {}", error) {}

ExecuteError::~ExecuteError() noexcept = default;

TransactionError::TransactionError(std::string_view error)
	: Exception(Path{"Transaction"}, static_cast<std::string_view>(StormByte::Safe::String::ReplaceToken("Unable to begin transaction: {}", error))) {}

TransactionError::~TransactionError() noexcept = default;
