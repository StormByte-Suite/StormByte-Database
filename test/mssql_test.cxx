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
#include <StormByte/database/mssql/telemetry.hxx>
#include <StormByte/database/transaction.hxx>
#include <StormByte/test_handlers.h>
#include "backend_contract.hxx"

#include <atomic>
#include <cstdlib>
#include <chrono>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace StormByte::Database;
using namespace StormByte::Database::MSSQL;

namespace {
	std::string EnvironmentValue(const char* const name, const char* const fallback) {
	#ifdef _WIN32
		char* value = nullptr;
		std::size_t length = 0;
		if (_dupenv_s(&value, &length, name) != 0 || !value)
			return fallback;
		std::string result{value};
		std::free(value);
		return result;
	#else
		const char* const value = std::getenv(name);
		return value ? value : fallback;
	#endif
	}

	class TestDatabase final : public MSSQL {
		public:
			TestDatabase(): MSSQL(
				EnvironmentValue("MSSQL_HOST", ""),
				EnvironmentValue("MSSQL_USER", "testuser"),
				EnvironmentValue("MSSQL_PASSWORD", "testpass"),
				EnvironmentValue("MSSQL_DATABASE", "master"),
				std::stoi(EnvironmentValue("MSSQL_PORT", "1433")), nullptr) {}

			const StormByte::Database::ExpectedRows Count(const std::string_view table) {
				return Query("SELECT CAST(COUNT(*) AS bigint) AS value FROM " + std::string{table} + ";");
			}

		private:
			void DoPostConnect() noexcept override {
				DoSilentQuery("CREATE TABLE #sb_scalar (id int IDENTITY PRIMARY KEY, signed_integer int NOT NULL, unsigned_integer bigint NOT NULL, signed_long bigint NOT NULL, unsigned_long decimal(20,0) NOT NULL, real_number float NOT NULL, text_value nvarchar(max) NOT NULL, blob_value varbinary(max) NOT NULL, flag bit NOT NULL, nullable_value nvarchar(max) NULL);");
				DoSilentQuery("CREATE TABLE #sb_blobs (id int IDENTITY PRIMARY KEY, data varbinary(max) NULL);");
				DoSilentQuery("CREATE TABLE #sb_nulls (id int IDENTITY PRIMARY KEY, value nvarchar(max) NULL);");
				DoSilentQuery("CREATE TABLE #sb_required (id int IDENTITY PRIMARY KEY, value nvarchar(64) NOT NULL);");
				DoSilentQuery("CREATE TABLE #sb_unique (id int IDENTITY PRIMARY KEY, value nvarchar(64) NOT NULL UNIQUE);");
				DoSilentQuery("CREATE TABLE #sb_concurrent (id int IDENTITY PRIMARY KEY, value int NOT NULL);");
				DoPrepareSTMT("insert_scalar_types", "INSERT INTO #sb_scalar (signed_integer, unsigned_integer, signed_long, unsigned_long, real_number, text_value, blob_value, flag, nullable_value) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);");
				DoPrepareSTMT("insert_blob", "INSERT INTO #sb_blobs (data) VALUES (?);");
				DoPrepareSTMT("select_blob", "SELECT TOP (1) data FROM #sb_blobs ORDER BY id DESC;");
				DoPrepareSTMT("insert_null", "INSERT INTO #sb_nulls (value) VALUES (?);");
				DoPrepareSTMT("select_nulls", "SELECT value FROM #sb_nulls ORDER BY id;");
				DoPrepareSTMT("insert_required", "INSERT INTO #sb_required (value) VALUES (?);");
				DoPrepareSTMT("insert_unique", "INSERT INTO #sb_unique (value) VALUES (?);");
				DoPrepareSTMT("insert_concurrent", "INSERT INTO #sb_concurrent (value) VALUES (?);");
				DoPrepareSTMT("count_concurrent", "SELECT CAST(COUNT(*) AS bigint) AS value FROM #sb_concurrent;");
				DoPrepareSTMT("integer_parameter", "SELECT CAST(? AS int) AS value;");
				DoPrepareSTMT("text_parameter", "SELECT CAST(? AS nvarchar(64)) AS value;");
				DoPrepareSTMT("two_parameters", "SELECT CAST(? AS int) + CAST(? AS int) AS value;");
				DoPrepareSTMT("scanner", "SELECT ? AS bound_value, '?' AS literal_value, [question?] AS bracket_value, \"double?\" AS double_value FROM (SELECT 9 AS [question?], 11 AS [double?]) AS source WHERE ? = ? -- ?\n/* outer ? /* nested ? */ ? */;");
			}
	};
}

int not_connected_operations() {
	TestDatabase database;
	ASSERT_FALSE(database.Query("SELECT 1;").has_value());
	ASSERT_FALSE(database.SilentQuery("SELECT 1;"));
	ASSERT_FALSE(database.ExecuteSTMT("integer_parameter", 1).has_value());
	ASSERT_FALSE(database.BeginTransaction().has_value());
	RETURN_TEST(0);
}

int connection_lifecycle_and_move() {
	std::unique_ptr<TestDatabase> moved;
	{
		TestDatabase source;
		ASSERT_TRUE(source.Connect());
		ASSERT_FALSE(source.Connect());
		moved = std::make_unique<TestDatabase>(std::move(source));
	}
	ASSERT_TRUE(moved->IsConnected());
	ASSERT_TRUE(moved->Query("SELECT 1 AS value;").has_value());
	moved->Disconnect();
	ASSERT_FALSE(moved->IsConnected());
	ASSERT_TRUE(moved->Connect());
	RETURN_TEST(0);
}

int shared_backend_contract() {
	TestDatabase scalar_database;
	const int scalar_result = verify_scalar_backend_contract(scalar_database,
		"SELECT signed_integer, unsigned_integer, signed_long, unsigned_long, real_number, text_value, blob_value, flag, nullable_value FROM #sb_scalar ORDER BY id;");
	TestDatabase binary_database;
	const int binary_result = verify_binary_backend_contract(binary_database,
		"SELECT TOP (1) value FROM #sb_nulls ORDER BY id DESC;");
	return scalar_result + binary_result;
}

int prepared_statement_parameter_types() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	ASSERT_TRUE(database.ExecuteSTMT("integer_parameter", std::numeric_limits<int>::min()).has_value());
	ASSERT_EQUAL(std::numeric_limits<int>::min(), database.ExecuteSTMT("integer_parameter", std::numeric_limits<int>::min()).value()[0][0].Get<int>());
	ASSERT_EQUAL(StormByte::Safe::String{"quoted ' text with \\ backslashes"},
		database.ExecuteSTMT("text_parameter", "quoted ' text with \\ backslashes").value()[0][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(std::numeric_limits<unsigned long long int>::max(),
		database.Query("SELECT CAST(18446744073709551615 AS decimal(20,0)) AS value;").value()[0][0].Get<unsigned long long int>());
	ASSERT_FALSE(database.ExecuteSTMT("two_parameters", 1).has_value());
	ASSERT_FALSE(database.ExecuteSTMT("two_parameters", 1, 2, 3).has_value());
	ASSERT_EQUAL(7, database.ExecuteSTMT("two_parameters", 3, 4).value()[0][0].Get<int>());
	RETURN_TEST(0);
}

int prepared_text_is_transmitted_as_data() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	const std::string_view payload{"x'); SELECT 1 / 0;--"};
	auto rows = database.ExecuteSTMT("text_parameter", payload);
	ASSERT_TRUE(rows.has_value());
	ASSERT_TRUE(static_cast<std::string_view>(rows.value()[0][0].Get<StormByte::Safe::String>()) == payload);
	auto follow_up = database.Query("SELECT 17 AS value;");
	ASSERT_TRUE(follow_up.has_value());
	ASSERT_EQUAL(17, follow_up.value()[0][0].Get<int>());
	RETURN_TEST(0);
}

int placeholder_scanner_ignores_sql_literals_and_comments() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	auto rows = database.ExecuteSTMT("scanner", 23, 7, 7);
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(1, rows.value().Count());
	ASSERT_EQUAL(23, rows.value()[0]["bound_value"].Get<int>());
	ASSERT_EQUAL(StormByte::Safe::String{"?"}, rows.value()[0]["literal_value"].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(9, rows.value()[0]["bracket_value"].Get<int>());
	ASSERT_EQUAL(11, rows.value()[0]["double_value"].Get<int>());
	RETURN_TEST(0);
}

int empty_and_null_values_are_distinct() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());

	ASSERT_TRUE(database.ExecuteSTMT("insert_blob", StormByte::Safe::Binary{}).has_value());
	auto empty_blob = database.ExecuteSTMT("select_blob");
	ASSERT_TRUE(empty_blob.has_value());
	ASSERT_FALSE(empty_blob.value()[0][0].IsNull());
	ASSERT_TRUE(empty_blob.value()[0][0].Get<StormByte::Safe::Binary>().empty());

	ASSERT_TRUE(database.ExecuteSTMT("insert_blob", nullptr).has_value());
	auto null_blob = database.ExecuteSTMT("select_blob");
	ASSERT_TRUE(null_blob.has_value());
	ASSERT_TRUE(null_blob.value()[0][0].IsNull());

	auto empty_text = database.ExecuteSTMT("text_parameter", "");
	ASSERT_TRUE(empty_text.has_value());
	ASSERT_FALSE(empty_text.value()[0][0].IsNull());
	ASSERT_TRUE(empty_text.value()[0][0].Get<StormByte::Safe::String>().empty());
	ASSERT_TRUE(database.ExecuteSTMT("insert_null", nullptr).has_value());
	auto null_text = database.Query("SELECT value FROM #sb_nulls ORDER BY id DESC;");
	ASSERT_TRUE(null_text.has_value());
	ASSERT_TRUE(null_text.value()[0][0].IsNull());
	RETURN_TEST(0);
}

int embedded_nul_sql_is_rejected_without_truncation() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	constexpr char sql[] = "SELECT 1;\0 SELECT 2;";
	const std::string_view query{sql, sizeof(sql) - 1};
	ASSERT_FALSE(database.Query(query).has_value());
	ASSERT_FALSE(database.SilentQuery(query));
	auto follow_up = database.Query("SELECT 42 AS value;");
	ASSERT_TRUE(follow_up.has_value());
	ASSERT_EQUAL(42, follow_up.value()[0][0].Get<int>());
	RETURN_TEST(0);
}

int multiple_result_sets_and_silent_drain() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	auto rows = database.Query("SELECT 10 AS value; SELECT 20 AS value;");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(2, rows.value().Count());
	ASSERT_EQUAL(10, rows.value()[0]["value"].Get<int>());
	ASSERT_EQUAL(20, rows.value()[1]["value"].Get<int>());
	ASSERT_TRUE(database.SilentQuery("SELECT 1 AS value; SELECT 2 AS value;"));
	ASSERT_EQUAL(30, database.Query("SELECT 30 AS value;").value()[0][0].Get<int>());
	RETURN_TEST(0);
}

int query_errors_preserve_connection() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	ASSERT_FALSE(database.Query("SELEC 1;").has_value());
	ASSERT_FALSE(database.SilentQuery("SELEC 1;"));
	ASSERT_TRUE(database.SilentQuery("SELECT 1;"));
	auto rows = database.Query("SELECT 42 AS value;");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(42, rows.value()[0]["value"].Get<int>());
	RETURN_TEST(0);
}

int constraint_failure_preserves_connection() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	ASSERT_TRUE(database.ExecuteSTMT("insert_unique", "same").has_value());
	ASSERT_FALSE(database.ExecuteSTMT("insert_unique", "same").has_value());
	ASSERT_EQUAL(1, database.Count("#sb_unique").value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int transactions_commit_and_rollback() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	{
		auto transaction = database.BeginTransaction(IsolationLevel::ReadCommitted);
		ASSERT_TRUE(transaction.has_value());
		ASSERT_TRUE(database.ExecuteSTMT("insert_required", "commit").has_value());
		transaction->Commit();
	}
	{
		auto transaction = database.BeginTransaction(IsolationLevel::ReadUncommitted);
		ASSERT_TRUE(transaction.has_value());
		transaction->Commit();
	}
	{
		auto transaction = database.BeginTransaction(IsolationLevel::RepeatableRead);
		ASSERT_TRUE(transaction.has_value());
		transaction->Commit();
	}
	ASSERT_EQUAL(1, database.Count("#sb_required").value()[0][0].Get<long long int>());
	{
		auto transaction = database.BeginTransaction(IsolationLevel::Serializable);
		ASSERT_TRUE(transaction.has_value());
		ASSERT_TRUE(database.ExecuteSTMT("insert_required", "rollback").has_value());
		transaction->Rollback();
	}
	ASSERT_EQUAL(1, database.Count("#sb_required").value()[0][0].Get<long long int>());
	{
		auto transaction = database.BeginTransaction();
		ASSERT_TRUE(transaction.has_value());
		ASSERT_TRUE(database.ExecuteSTMT("insert_required", "auto rollback").has_value());
	}
	ASSERT_EQUAL(1, database.Count("#sb_required").value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int failed_statement_rolls_back_transaction() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	ASSERT_TRUE(database.ExecuteSTMT("insert_unique", "duplicate").has_value());
	auto transaction = database.BeginTransaction();
	ASSERT_TRUE(transaction.has_value());
	ASSERT_TRUE(database.ExecuteSTMT("insert_required", "transient").has_value());
	ASSERT_FALSE(database.ExecuteSTMT("insert_unique", "duplicate").has_value());
	transaction->Rollback();
	ASSERT_EQUAL(0, database.Query("SELECT COUNT(*) AS value FROM #sb_required WHERE value = 'transient';").value()[0][0].Get<int>());
	ASSERT_EQUAL(1, database.Count("#sb_unique").value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int same_connection_concurrency_and_transaction_exclusion() {
	TestDatabase database;
	ASSERT_TRUE(database.Connect());
	constexpr int thread_count = 4;
	constexpr int writes_per_thread = 20;
	std::atomic<int> failures{};
	std::vector<std::thread> workers;
	for (int thread_index{}; thread_index < thread_count; ++thread_index) {
		workers.emplace_back([thread_index, &database, &failures] {
			for (int write_index{}; write_index < writes_per_thread; ++write_index)
				if (!database.ExecuteSTMT("insert_concurrent", thread_index * 1000 + write_index).has_value())
					++failures;
		});
	}
	for (auto& worker : workers)
		worker.join();
	ASSERT_EQUAL(0, failures.load());
	ASSERT_EQUAL(thread_count * writes_per_thread,
		database.ExecuteSTMT("count_concurrent").value()[0][0].Get<long long int>());

	ASSERT_TRUE(database.SilentQuery("DELETE FROM #sb_concurrent;"));
	auto transaction = database.BeginTransaction();
	ASSERT_TRUE(transaction.has_value());
	ASSERT_TRUE(database.ExecuteSTMT("insert_concurrent", 1).has_value());
	std::promise<void> worker_started;
	auto worker_started_future = worker_started.get_future();
	auto worker = std::async(std::launch::async, [&database, &worker_started] {
		worker_started.set_value();
		return database.ExecuteSTMT("insert_concurrent", 2).has_value();
	});
	ASSERT_EQUAL(std::future_status::ready, worker_started_future.wait_for(std::chrono::seconds{1}));
	ASSERT_TRUE(worker.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout);
	transaction->Rollback();
	ASSERT_TRUE(worker.get());
	ASSERT_EQUAL(1, database.ExecuteSTMT("count_concurrent").value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int separate_connections_run_concurrently() {
	constexpr int connection_count = 4;
	std::atomic<int> failures{};
	std::vector<std::thread> workers;
	for (int connection_index{}; connection_index < connection_count; ++connection_index) {
		workers.emplace_back([connection_index, &failures] {
			TestDatabase database;
			if (!database.Connect()
					|| !database.ExecuteSTMT("insert_concurrent", connection_index).has_value()
					|| !database.ExecuteSTMT("count_concurrent").has_value()
					|| database.ExecuteSTMT("count_concurrent").value()[0][0].Get<long long int>() != 1)
				++failures;
		});
	}
	for (auto& worker : workers)
		worker.join();
	ASSERT_EQUAL(0, failures.load());
	RETURN_TEST(0);
}

int telemetry_tracks_errors_and_survives_database() {
	StormByte::Safe::Shared<StormByte::Database::Telemetry> retained;
	{
		TestDatabase database;
		retained = database.Telemetry();
		ASSERT_TRUE(dynamic_cast<StormByte::Database::MSSQL::Telemetry*>(retained.get()) != nullptr);
		ASSERT_TRUE(database.Connect());
		ASSERT_TRUE(database.Query("SELECT 1 AS value;").has_value());
		ASSERT_FALSE(database.Query("SELEC 1;").has_value());
		ASSERT_TRUE(database.ExecuteSTMT("integer_parameter", 1).has_value());
		ASSERT_FALSE(database.ExecuteSTMT("missing_statement").has_value());
		database.Disconnect();
	}
	const auto* telemetry = dynamic_cast<const StormByte::Database::MSSQL::Telemetry*>(retained.get());
	ASSERT_TRUE(telemetry != nullptr);
	ASSERT_EQUAL(std::uint64_t{2}, telemetry->Metrics(Operation::Query).Attempts);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(Operation::Query).Successes);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(Operation::Query).Failures);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(Operation::PreparedStatement).Successes);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(Operation::PreparedStatement).Failures);
	ASSERT_TRUE(telemetry->Errors() > 0);
	ASSERT_CONTAINS(static_cast<std::string>(*retained), "MSSQL{");
	RETURN_TEST(0);
}

int main() {
	if (EnvironmentValue("MSSQL_HOST", "").empty()) {
		std::cout << "MSSQL_HOST is unset; skipping MSSQL integration test.\n";
		return 77;
	}
	int result{};
	result += not_connected_operations();
	result += connection_lifecycle_and_move();
	result += shared_backend_contract();
	result += prepared_statement_parameter_types();
	result += prepared_text_is_transmitted_as_data();
	result += placeholder_scanner_ignores_sql_literals_and_comments();
	result += empty_and_null_values_are_distinct();
	result += embedded_nul_sql_is_rejected_without_truncation();
	result += multiple_result_sets_and_silent_drain();
	result += query_errors_preserve_connection();
	result += constraint_failure_preserves_connection();
	result += transactions_commit_and_rollback();
	result += failed_statement_rolls_back_transaction();
	result += same_connection_concurrency_and_transaction_exclusion();
	result += separate_connections_run_concurrently();
	result += telemetry_tracks_errors_and_survives_database();
	return result;
}
