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

#include <StormByte/database/sqlite/result_fetch.hxx>

#include <limits>
#include <string_view>
#include <utility>

using namespace StormByte::Database::SQLite;

StormByte::Database::ExpectedRows StormByte::Database::SQLite::StepResults(sqlite3_stmt* stmt) noexcept {
	if (!stmt)
		return Unexpected<QueryException>(ExecuteError("Invalid SQLite statement provided."));

	Rows rows;
	int result_code;
	while ((result_code = sqlite3_step(stmt)) == SQLITE_ROW) {
		Row row;
		const int column_count = sqlite3_column_count(stmt);
		for (int column_index = 0; column_index < column_count; ++column_index) {
			const char* column_name = sqlite3_column_name(stmt, column_index);
			const std::string_view name{column_name ? column_name : ""};
			switch (sqlite3_column_type(stmt, column_index)) {
				case SQLITE_INTEGER: {
					const sqlite3_int64 value = sqlite3_column_int64(stmt, column_index);
					if (value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min())
						row.add(name, static_cast<long long int>(value));
					else
						row.add(name, static_cast<int>(value));
					break;
				}
				case SQLITE_FLOAT:
					row.add(name, sqlite3_column_double(stmt, column_index));
					break;
				case SQLITE_TEXT: {
					const auto* text = sqlite3_column_text(stmt, column_index);
					const int text_size = sqlite3_column_bytes(stmt, column_index);
					const char* text_data = reinterpret_cast<const char*>(text);
					if (!text_data)
						text_data = "";
					row.add(name, std::string_view{text_data, static_cast<std::size_t>(text_size)});
					break;
				}
				case SQLITE_BLOB: {
					const auto* blob_data = reinterpret_cast<const std::byte*>(sqlite3_column_blob(stmt, column_index));
					const int blob_size = sqlite3_column_bytes(stmt, column_index);
					StormByte::Safe::Binary blob{blob_data, StormByte::ByteSize{blob_size}};
					row.add(name, Value{std::move(blob)});
					break;
				}
				case SQLITE_NULL:
				default:
					row.add(name, Value{});
					break;
			}
		}
		rows.add(std::move(row));
	}

	if (result_code == SQLITE_DONE)
		return rows;

	const char* error_message = "Unknown SQLite error";
	if (sqlite3_db_handle(stmt))
		error_message = sqlite3_errmsg(sqlite3_db_handle(stmt));
	return Unexpected<QueryException>(ExecuteError(error_message ? error_message : "Unknown SQLite error"));
}
