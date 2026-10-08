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

#include <StormByte/database/mariadb/result_fetch.hxx>

#include <charconv>
#include <limits>
#include <string_view>
#include <utility>

using namespace StormByte::Database::MariaDB;

StormByte::Database::ExpectedRows StormByte::Database::MariaDB::StepResults(MYSQL_RES* result) noexcept {
	if (!result)
		return Unexpected<QueryException>(ExecuteError("Invalid MYSQL_RES provided."));

	Rows rows;
	const StormByte::Size row_count{mysql_num_rows(result)};
	const unsigned int field_count = mysql_num_fields(result);
	for (StormByte::Size row_index{}; row_index < row_count; ++row_index) {
		MYSQL_ROW row = mysql_fetch_row(result);
		unsigned long* lengths = mysql_fetch_lengths(result);
		Row output_row;
		for (unsigned int column_index = 0; column_index < field_count; ++column_index) {
			MYSQL_FIELD* field = mysql_fetch_field_direct(result, column_index);
			const char* column_name = field ? field->name : nullptr;
			const std::string_view name{column_name ? column_name : ""};
			if (!row[column_index]) {
				output_row.add(name, Value{});
				continue;
			}

			const unsigned long length = lengths ? lengths[column_index] : 0;
			const enum_field_types field_type = field ? field->type : MYSQL_TYPE_STRING;
			switch (field_type) {
				case MYSQL_TYPE_TINY: {
					if (field && (field->flags & UNSIGNED_FLAG) == 0 && field->length == 1) {
						output_row.add(name, row[column_index][0] != '0');
					} else {
						long long value = 0;
						const auto parsed = std::from_chars(row[column_index], row[column_index] + length, value);
						if (parsed.ec != std::errc{} || parsed.ptr != row[column_index] + length)
							return Unexpected<QueryException>(ExecuteError("Invalid integer result value."));
						if (value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min())
							output_row.add(name, value);
						else
							output_row.add(name, static_cast<int>(value));
					}
					break;
				}
				case MYSQL_TYPE_SHORT:
				case MYSQL_TYPE_LONG:
				case MYSQL_TYPE_INT24: {
					long long value = 0;
					const auto parsed = std::from_chars(row[column_index], row[column_index] + length, value);
					if (parsed.ec != std::errc{} || parsed.ptr != row[column_index] + length)
						return Unexpected<QueryException>(ExecuteError("Invalid integer result value."));
					if (value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min())
						output_row.add(name, value);
					else
						output_row.add(name, static_cast<int>(value));
					break;
				}
				case MYSQL_TYPE_LONGLONG: {
					long long value = 0;
					const auto parsed = std::from_chars(row[column_index], row[column_index] + length, value);
					if (parsed.ec != std::errc{} || parsed.ptr != row[column_index] + length)
						return Unexpected<QueryException>(ExecuteError("Invalid integer result value."));
					output_row.add(name, value);
					break;
				}
				case MYSQL_TYPE_FLOAT:
				case MYSQL_TYPE_DOUBLE:
				case MYSQL_TYPE_DECIMAL:
				case MYSQL_TYPE_NEWDECIMAL: {
					double value = 0.0;
					const auto parsed = std::from_chars(row[column_index], row[column_index] + length, value);
					if (parsed.ec != std::errc{} || parsed.ptr != row[column_index] + length)
						return Unexpected<QueryException>(ExecuteError("Invalid floating-point result value."));
					output_row.add(name, value);
					break;
				}
				case MYSQL_TYPE_TINY_BLOB:
				case MYSQL_TYPE_MEDIUM_BLOB:
				case MYSQL_TYPE_LONG_BLOB:
				case MYSQL_TYPE_BLOB: {
					if (field && field->charsetnr == 63) {
						StormByte::Safe::Binary blob{
							reinterpret_cast<const std::byte*>(row[column_index]),
							StormByte::ByteSize{length}
						};
						output_row.add(name, Value{std::move(blob)});
					} else {
						output_row.add(name, std::string_view{row[column_index], static_cast<std::size_t>(length)});
					}
					break;
				}
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_STRING:
				case MYSQL_TYPE_VARCHAR:
				default:
					output_row.add(name, std::string_view{row[column_index], static_cast<std::size_t>(length)});
					break;
			}
		}
		rows.add(std::move(output_row));
	}
	return rows;
}
