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

#pragma once

#include <StormByte/database/database.hxx>
#include <StormByte/test_handlers.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>

/**
 * @file backend_contract.hxx
 * @brief Shared behavioral tests for all built-in SQL backends.
 */

/**
 * @brief Verify round-trips for all database value categories and boundary values.
 * @tparam DatabaseType Built-in backend test fixture.
 * @param db Connected database fixture.
 * @param query SQL query selecting the scalar test table.
 * @return Test-handler result.
 */
template <typename DatabaseType>
int verify_scalar_backend_contract(DatabaseType& db,
		const std::string_view query = "SELECT signed_integer, unsigned_integer, signed_long, unsigned_long, real_number, text_value, blob_value, flag, nullable_value FROM scalar_types ORDER BY id;") {
	const int signed_int = std::numeric_limits<int>::min();
	const unsigned int unsigned_int = std::numeric_limits<unsigned int>::max();
	const long long int signed_long = std::numeric_limits<long long int>::min();
	const unsigned long long int unsigned_long = static_cast<unsigned long long int>(std::numeric_limits<long long int>::max());
	const double floating = -12345.625;
	const std::string text = "quoted ' text with \\\\ backslashes";
	const std::array<std::byte, 5> bytes{std::byte{0}, std::byte{0xFF}, std::byte{0}, std::byte{0x7F}, std::byte{0x80}};
	const StormByte::Safe::Binary blob{bytes.data(), StormByte::ByteSize{bytes.size()}};

	ASSERT_TRUE(db.Connect());
	ASSERT_TRUE(db.ExecuteSTMT("insert_scalar_types", signed_int, unsigned_int, signed_long, unsigned_long, floating, text, blob, true, nullptr).has_value());
	ASSERT_TRUE(db.ExecuteSTMT("insert_scalar_types", signed_int, unsigned_int, signed_long, unsigned_long, floating, text, blob, false, "not null").has_value());

	auto result = db.Query(query);
	ASSERT_TRUE(result.has_value());
	ASSERT_EQUAL(2, result.value().Count());

	for (StormByte::Size row_index{}; row_index < result.value().Count(); ++row_index) {
		const auto& row = result.value()[row_index];
		ASSERT_EQUAL(signed_int, row[0].template Get<int>());
		ASSERT_EQUAL(unsigned_int, row[1].template Get<unsigned int>());
		ASSERT_EQUAL(signed_long, row[2].template Get<long long int>());
		ASSERT_EQUAL(unsigned_long, row[3].template Get<unsigned long long int>());
		ASSERT_EQUAL(floating, row[4].template Get<double>());
		ASSERT_EQUAL(StormByte::Safe::String{text}, row[5].template Get<StormByte::Safe::String>());
		const auto& returned_blob = row[6].template Get<StormByte::Safe::Binary>();
		ASSERT_EQUAL(bytes.size(), returned_blob.size());
		for (std::size_t byte_index{}; byte_index < bytes.size(); ++byte_index)
			ASSERT_EQUAL(bytes[byte_index], returned_blob[byte_index]);
		ASSERT_EQUAL(row_index == 0, row[7].template Get<bool>());
	}

	ASSERT_TRUE(result.value()[0][8].IsNull());
	ASSERT_EQUAL(StormByte::Safe::String{"not null"}, result.value()[1][8].template Get<StormByte::Safe::String>());
	auto empty_text = db.Query("SELECT '' AS empty_text;");
	ASSERT_TRUE(empty_text.has_value());
	ASSERT_EQUAL(1, empty_text.value().Count());
	ASSERT_TRUE(empty_text.value()[0][0].template Get<StormByte::Safe::String>().empty());
	auto no_rows = db.Query("SELECT 1 WHERE 1 = 0;");
	ASSERT_TRUE(no_rows.has_value());
	ASSERT_TRUE(no_rows.value().empty());
	RETURN_TEST(0);
}

/**
 * @brief Verify zero-length BLOB, large BLOB and SQL NULL remain distinct.
 * @tparam DatabaseType Built-in backend test fixture.
 * @param db Connected database fixture.
 * @param null_query SQL query selecting the newest NULL test row.
 * @return Test-handler result.
 */
template <typename DatabaseType>
int verify_binary_backend_contract(DatabaseType& db,
		const std::string_view null_query = "SELECT value FROM nulls ORDER BY id DESC LIMIT 1;") {
	ASSERT_TRUE(db.Connect());
	ASSERT_TRUE(db.ExecuteSTMT("insert_blob", StormByte::Safe::Binary{}).has_value());
	auto empty_blob_rows = db.ExecuteSTMT("select_blob");
	ASSERT_TRUE(empty_blob_rows.has_value());
	ASSERT_EQUAL(1, empty_blob_rows.value().Count());
	ASSERT_FALSE(empty_blob_rows.value()[0][0].IsNull());
	ASSERT_TRUE(empty_blob_rows.value()[0][0].template Get<StormByte::Safe::Binary>().empty());

	StormByte::Safe::Binary large_blob{StormByte::ByteSize{48 * 1024}};
	for (std::size_t index{}; index < large_blob.size(); ++index)
		large_blob.begin()[index] = static_cast<std::byte>(index % 251);
	ASSERT_TRUE(db.ExecuteSTMT("insert_blob", large_blob).has_value());
	auto large_blob_rows = db.ExecuteSTMT("select_blob");
	ASSERT_TRUE(large_blob_rows.has_value());
	const auto& returned_blob = large_blob_rows.value()[0][0].template Get<StormByte::Safe::Binary>();
	ASSERT_EQUAL(large_blob.size(), returned_blob.size());
	ASSERT_TRUE(std::equal(large_blob.begin(), large_blob.end(), returned_blob.begin()));

	ASSERT_TRUE(db.ExecuteSTMT("insert_null", nullptr).has_value());
	auto null_rows = db.Query(null_query);
	ASSERT_TRUE(null_rows.has_value());
	ASSERT_EQUAL(1, null_rows.value().Count());
	ASSERT_TRUE(null_rows.value()[0][0].IsNull());
	RETURN_TEST(0);
}
