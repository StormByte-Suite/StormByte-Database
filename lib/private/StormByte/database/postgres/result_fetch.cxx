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

#include <StormByte/database/postgres/result_fetch.hxx>

#include <cctype>
#include <charconv>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

using namespace StormByte::Database::Postgres;

StormByte::Database::ExpectedRows StormByte::Database::Postgres::StepResults(PGresult* result) noexcept {
	if (!result)
		return Unexpected<QueryException>(ExecuteError("Invalid PGresult provided."));

	const ExecStatusType status = PQresultStatus(result);
	if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK)
		return Unexpected<QueryException>(ExecuteError(PQresultErrorMessage(result) ? PQresultErrorMessage(result) : "Unknown PG error"));

	Rows rows;
	const int row_count = PQntuples(result);
	const int field_count = PQnfields(result);
	for (int row_index = 0; row_index < row_count; ++row_index) {
		Row row;
		for (int column_index = 0; column_index < field_count; ++column_index) {
			const char* column_name = PQfname(result, column_index);
			const std::string_view name{column_name ? column_name : ""};
			if (PQgetisnull(result, row_index, column_index)) {
				row.add(name, Value{});
				continue;
			}

			const Oid field_type = PQftype(result, column_index);
			const char* value = PQgetvalue(result, row_index, column_index);
			const int value_length = PQgetlength(result, row_index, column_index);
			switch (field_type) {
				case 16: {
					bool boolean_value = false;
					if (value) {
						if (value[0] == 't' || value[0] == '1') {
							boolean_value = true;
						} else {
						std::string normalized_value(value);
						for (auto& character : normalized_value)
							character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
						boolean_value = normalized_value == "true";
					}
					}
					row.add(name, Value{boolean_value});
					break;
				}
				case 20:
				case 21:
				case 23: {
					long long integer_value = 0;
					const auto parsed = std::from_chars(value, value + value_length, integer_value);
					if (parsed.ec != std::errc{} || parsed.ptr != value + value_length)
						return Unexpected<QueryException>(ExecuteError("Invalid integer result value."));
					if (integer_value > std::numeric_limits<int>::max() || integer_value < std::numeric_limits<int>::min())
						row.add(name, integer_value);
					else
						row.add(name, static_cast<int>(integer_value));
					break;
				}
				case 700:
				case 701: {
					double floating_value = 0.0;
					const auto parsed = std::from_chars(value, value + value_length, floating_value);
					if (parsed.ec != std::errc{} || parsed.ptr != value + value_length)
						return Unexpected<QueryException>(ExecuteError("Invalid floating-point result value."));
					row.add(name, floating_value);
					break;
				}
				case 17: {
					unsigned char* unescaped = nullptr;
					std::size_t unescaped_length = 0;
					unescaped = PQunescapeBytea(reinterpret_cast<const unsigned char*>(value), &unescaped_length);
					StormByte::Safe::Binary blob{
						reinterpret_cast<const std::byte*>(unescaped),
						StormByte::ByteSize{unescaped_length}
					};
					if (unescaped)
						PQfreemem(unescaped);
					row.add(name, Value{std::move(blob)});
					break;
				}
				default:
					row.add(name, std::string_view{value ? value : "", static_cast<std::size_t>(value_length)});
					break;
			}
		}
		rows.add(std::move(row));
	}

	return rows;
}
