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
#include <StormByte/database/row.hxx>
#include <StormByte/database/rows.hxx>
#include <StormByte/database/telemetry.hxx>
#include <StormByte/database/value.hxx>
#include <StormByte/safe/optional.hxx>
#include <StormByte/safe/vector.hxx>
#include <StormByte/test_handlers.h>

#include <iostream>
#include <atomic>
#include <cmath>
#include <limits>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace StormByte::Database;

static_assert(StormByte::Type::MaybeSafe<Value>);
static_assert(StormByte::Type::MaybeSafe<NamedValue>);
static_assert(StormByte::Type::SafeValue<Row>);
static_assert(StormByte::Type::SafeValue<Rows>);
static_assert(StormByte::Type::SafeComponent<StormByte::Safe::Shared<Telemetry>>);

class TestTelemetry : public Telemetry {
	public:
		TestTelemetry() noexcept = default;
};

int test_component_prefixed_exceptions() {
	int result = 0;
	ASSERT_EQUAL("test_component_prefixed_exceptions", std::string("StormByte.Database: generic error"), std::string(Exception("generic error").what()));
	ASSERT_EQUAL("test_component_prefixed_exceptions", std::string("StormByte.Database.Connection: connection failed"), std::string(ConnectionError("connection failed").what()));
	ASSERT_EQUAL("test_component_prefixed_exceptions", std::string("StormByte.Database.WrongValueType.Value: expected integer"), std::string(WrongValueType("Value", "expected {}", "integer").what()));
	ASSERT_EQUAL("test_component_prefixed_exceptions", std::string("StormByte.Database.ColumnNotFound: Column 'id' not found"), std::string(ColumnNotFound("id").what()));
	ASSERT_EQUAL("test_component_prefixed_exceptions", std::string("StormByte.Database.OutOfBounds: Position 3 is out of bounds for size 2"), std::string(OutOfBounds(3, 2).what()));
	ASSERT_EQUAL("test_component_prefixed_exceptions", std::string("StormByte.Database.Query.PreparedSTMT: Statement 'users' not found"), std::string(UnknownSTMT("users").what()));
	ASSERT_EQUAL("test_component_prefixed_exceptions", std::string("StormByte.Database.Query.Execute: Error executing query: syntax error"), std::string(ExecuteError("syntax error").what()));
	ASSERT_EQUAL("test_component_prefixed_exceptions", std::string("StormByte.Database.Transaction: Unable to begin transaction: disconnected"), std::string(TransactionError("disconnected").what()));
	RETURN_TEST("test_component_prefixed_exceptions", result);
}

int test_invalid_value_conversions_throw() {
	int result = 0;
	bool threw = false;
	try {
		(void)Value().Get<int>();
	} catch (const WrongValueType&) {
		threw = true;
	}

	ASSERT_TRUE("test_invalid_value_conversions_throw", threw);

	threw = false;
	try {
		(void)Value("text").Get<int>();
	} catch (const WrongValueType&) {
		threw = true;
	}

	ASSERT_TRUE("test_invalid_value_conversions_throw", threw);

	threw = false;
	try {
		(void)Value(-1).Get<unsigned int>();
	} catch (const WrongValueType&) {
		threw = true;
	}

	ASSERT_TRUE("test_invalid_value_conversions_throw", threw);

	threw = false;
	try {
		(void)Value(std::numeric_limits<unsigned long int>::max()).Get<int>();
	} catch (const WrongValueType&) {
		threw = true;
	}

	ASSERT_TRUE("test_invalid_value_conversions_throw", threw);

	threw = false;
	try {
		(void)Value(1.5).Get<int>();
	} catch (const WrongValueType&) {
		threw = true;
	}

	ASSERT_TRUE("test_invalid_value_conversions_throw", threw);

	threw = false;
	try {
		(void)Value(std::numeric_limits<double>::max()).Get<long long int>();
	} catch (const WrongValueType&) {
		threw = true;
	}

	ASSERT_TRUE("test_invalid_value_conversions_throw", threw);

	threw = false;
	try {
		(void)Value(-std::numeric_limits<double>::max()).Get<unsigned long long int>();
	} catch (const WrongValueType&) {
		threw = true;
	}

	ASSERT_TRUE("test_invalid_value_conversions_throw", threw);

	threw = false;
	try {
		(void)Value(std::numeric_limits<double>::infinity()).Get<int>();
	} catch (const WrongValueType&) {
		threw = true;
	}

	ASSERT_TRUE("test_invalid_value_conversions_throw", threw);

	threw = false;
	try {
		(void)Value(std::numeric_limits<double>::quiet_NaN()).Get<int>();
	} catch (const WrongValueType&) {
		threw = true;
	}

	ASSERT_TRUE("test_invalid_value_conversions_throw", threw);
	RETURN_TEST("test_invalid_value_conversions_throw", result);
}

int test_value_variants_and_numeric_boundaries() {
	constexpr std::string_view fn_name = "test_value_variants_and_numeric_boundaries";
	ASSERT_EQUAL(fn_name, Value::Type::Null, Value().Type());
	ASSERT_EQUAL(fn_name, Value::Type::Integer, Value(std::numeric_limits<int>::min()).Type());
	ASSERT_EQUAL(fn_name, Value::Type::UnsignedInteger, Value(std::numeric_limits<unsigned int>::max()).Type());
	ASSERT_EQUAL(fn_name, Value::Type::LongInteger, Value(std::numeric_limits<long int>::min()).Type());
	ASSERT_EQUAL(fn_name, Value::Type::UnsignedLongInteger, Value(std::numeric_limits<unsigned long int>::max()).Type());
	ASSERT_EQUAL(fn_name, Value::Type::LongInteger, Value(std::numeric_limits<long long int>::min()).Type());
	ASSERT_EQUAL(fn_name, Value::Type::UnsignedLongInteger, Value(std::numeric_limits<unsigned long long int>::max()).Type());
	ASSERT_EQUAL(fn_name, Value::Type::Double, Value(std::numeric_limits<double>::lowest()).Type());
	ASSERT_EQUAL(fn_name, Value::Type::Text, Value(std::string_view{}).Type());
	ASSERT_EQUAL(fn_name, Value::Type::Blob, Value(StormByte::BinaryData{}).Type());
	ASSERT_EQUAL(fn_name, Value::Type::Boolean, Value(true).Type());
	ASSERT_EQUAL(fn_name, std::numeric_limits<int>::min(), Value(std::numeric_limits<int>::min()).Get<int>());
	ASSERT_EQUAL(fn_name, std::numeric_limits<unsigned int>::max(), Value(std::numeric_limits<unsigned int>::max()).Get<unsigned int>());
	ASSERT_EQUAL(fn_name, static_cast<long long int>(std::numeric_limits<long int>::min()), Value(std::numeric_limits<long int>::min()).Get<long long int>());
	ASSERT_EQUAL(fn_name, static_cast<unsigned long long int>(std::numeric_limits<unsigned long int>::max()), Value(std::numeric_limits<unsigned long int>::max()).Get<unsigned long long int>());
	ASSERT_EQUAL(fn_name, std::numeric_limits<long long int>::min(), Value(std::numeric_limits<long long int>::min()).Get<long long int>());
	ASSERT_EQUAL(fn_name, std::numeric_limits<unsigned long long int>::max(), Value(std::numeric_limits<unsigned long long int>::max()).Get<unsigned long long int>());
	ASSERT_EQUAL(fn_name, std::numeric_limits<int>::max(), Value(static_cast<double>(std::numeric_limits<int>::max())).Get<int>());
	ASSERT_EQUAL(fn_name, std::numeric_limits<long long int>::min(), Value(-std::ldexp(1.0, std::numeric_limits<long long int>::digits)).Get<long long int>());
	ASSERT_EQUAL(fn_name, true, Value(1).Get<bool>());
	ASSERT_EQUAL(fn_name, 0, Value(false).Get<int>());
	ASSERT_EQUAL(fn_name, false, Value(0.0).Get<bool>());
	RETURN_TEST(fn_name, 0);
}

int test_row_and_rows_value_semantics() {
	constexpr std::string_view fn_name = "test_row_and_rows_value_semantics";
	Row row;
	ASSERT_TRUE(fn_name, row.empty());
	ASSERT_EQUAL(fn_name, row.begin(), row.end());
	row.add("id", Value{42});
	row.add("name", Value{std::string_view{"Ada"}});
	ASSERT_EQUAL(fn_name, 2, row.size());
	ASSERT_EQUAL(fn_name, 42, row["id"].Get<int>());
	ASSERT_EQUAL(fn_name, "Ada", row[1].Get<StormByte::Safe::String>());

	Row copied_row{row};
	ASSERT_TRUE(fn_name, copied_row == row);
	Row assigned_row;
	assigned_row = row;
	ASSERT_TRUE(fn_name, assigned_row == row);
	copied_row["id"] = Value{7};
	ASSERT_EQUAL(fn_name, 42, row["id"].Get<int>());
	ASSERT_EQUAL(fn_name, 7, copied_row["id"].Get<int>());
	Row moved_row{std::move(copied_row)};
	ASSERT_EQUAL(fn_name, 7, moved_row["id"].Get<int>());

	Rows rows;
	ASSERT_TRUE(fn_name, rows.empty());
	ASSERT_EQUAL(fn_name, rows.begin(), rows.end());
	rows.add(row);
	rows.add(std::move(moved_row));
	ASSERT_EQUAL(fn_name, 2, rows.size());
	ASSERT_TRUE(fn_name, rows.has_item(row));
	ASSERT_EQUAL(fn_name, 2, std::distance(rows.begin(), rows.end()));
	Rows copied_rows{rows};
	ASSERT_TRUE(fn_name, copied_rows == rows);
	Rows assigned_rows;
	assigned_rows = rows;
	ASSERT_TRUE(fn_name, assigned_rows == rows);
	Rows moved_rows{std::move(copied_rows)};
	ASSERT_EQUAL(fn_name, 2, moved_rows.Count());
	bool out_of_bounds = false;
	try {
		(void)moved_rows[2];
	} catch (const OutOfBounds&) {
		out_of_bounds = true;
	}
	ASSERT_TRUE(fn_name, out_of_bounds);
	RETURN_TEST(fn_name, 0);
}

int test_telemetry_operation_metrics() {
	constexpr std::string_view fn_name = "test_telemetry_operation_metrics";
	auto telemetry = StormByte::Safe::Shared<TestTelemetry>::MakePointer<TestTelemetry>();
	{
		Telemetry::OperationScope operation{telemetry, Operation::Query};
		operation.Complete(true, 3);
	}
	{
		Telemetry::OperationScope operation{telemetry, Operation::Query};
		operation.Complete(false);
	}
	const OperationMetrics metrics = telemetry->Metrics(Operation::Query);
	ASSERT_EQUAL(fn_name, 2, metrics.Attempts);
	ASSERT_EQUAL(fn_name, 1, metrics.Successes);
	ASSERT_EQUAL(fn_name, 1, metrics.Failures);
	ASSERT_TRUE(fn_name, metrics.MinimumNanoseconds <= metrics.MeanNanoseconds());
	ASSERT_TRUE(fn_name, metrics.MeanNanoseconds() <= metrics.MaximumNanoseconds);
	ASSERT_EQUAL(fn_name, 3, telemetry->RowsReturned());
	ASSERT_TRUE(fn_name, static_cast<std::string>(*telemetry).find("Query{calls=2") != std::string::npos);

	auto concurrent_telemetry = StormByte::Safe::Shared<TestTelemetry>::MakePointer<TestTelemetry>();
	std::atomic<bool> stop_reader{false};
	std::atomic<bool> invalid_minimum{false};
	std::thread reader([&]() {
		while (!stop_reader.load(std::memory_order_acquire)) {
			const auto snapshot = concurrent_telemetry->Metrics(Operation::PreparedStatement);
			if (snapshot.MinimumNanoseconds == std::numeric_limits<std::uint64_t>::max())
				invalid_minimum.store(true, std::memory_order_relaxed);
		}
	});
	constexpr int thread_count = 8;
	constexpr int operations_per_thread = 500;
	std::vector<std::thread> threads;
	for (int thread_index{}; thread_index < thread_count; ++thread_index) {
		threads.emplace_back([&concurrent_telemetry]() {
			for (int operation_index{}; operation_index < operations_per_thread; ++operation_index) {
				Telemetry::OperationScope operation{concurrent_telemetry, Operation::PreparedStatement};
				operation.Complete(true, 1);
			}
		});
	}
	for (auto& thread : threads)
		thread.join();
	stop_reader.store(true, std::memory_order_release);
	reader.join();
	ASSERT_TRUE(fn_name, !invalid_minimum.load(std::memory_order_relaxed));
	const OperationMetrics concurrent_metrics = concurrent_telemetry->Metrics(Operation::PreparedStatement);
	ASSERT_EQUAL(fn_name, thread_count * operations_per_thread, concurrent_metrics.Attempts);
	ASSERT_EQUAL(fn_name, thread_count * operations_per_thread, concurrent_metrics.Successes);
	ASSERT_EQUAL(fn_name, thread_count * operations_per_thread, concurrent_telemetry->RowsReturned());
	RETURN_TEST(fn_name, 0);
}

int test_telemetry_overlapping_samples() {
	constexpr std::string_view fn_name = "test_telemetry_overlapping_samples";
	auto telemetry = StormByte::Safe::Shared<TestTelemetry>::MakePointer<TestTelemetry>();
	{
		Telemetry::OperationScope outer{telemetry, Operation::Query};
		ASSERT_EQUAL(fn_name, std::uint64_t{0}, telemetry->Metrics(Operation::Query).Attempts);
		{
			Telemetry::OperationScope inner{telemetry, Operation::Query};
			inner.Complete(true, 2);
		}
		ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->Metrics(Operation::Query).Attempts);
		outer.Complete(false);
	}
	const auto metrics = telemetry->Metrics(Operation::Query);
	ASSERT_EQUAL(fn_name, std::uint64_t{2}, metrics.Attempts);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, metrics.Successes);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, metrics.Failures);
	ASSERT_EQUAL(fn_name, std::uint64_t{2}, telemetry->RowsReturned());
	{
		Telemetry::OperationScope ignored{telemetry, Operation::Count};
		Telemetry::OperationScope empty{{}, Operation::Query};
	}
	ASSERT_EQUAL(fn_name, std::uint64_t{2}, telemetry->Metrics(Operation::Query).Attempts);
	RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;
	result += test_component_prefixed_exceptions();
	result += test_invalid_value_conversions_throw();
	result += test_value_variants_and_numeric_boundaries();
	result += test_row_and_rows_value_semantics();
	result += test_telemetry_operation_metrics();
	result += test_telemetry_overlapping_samples();
	if (result == 0) {
		std::cout << "All tests passed successfully.\n";
	} else {
		std::cout << result << " tests failed.\n";
	}

	return result;
}
