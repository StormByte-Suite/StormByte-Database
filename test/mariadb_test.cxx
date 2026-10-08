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

#include <StormByte/database/mariadb/mariadb.hxx>
#include <StormByte/database/transaction.hxx>
#include <StormByte/logger/log.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/test_handlers.h>
#include "backend_contract.hxx"
#include <memory>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <limits>
#include <atomic>
#include <future>
using ExpectedRows = StormByte::Database::ExpectedRows;
using namespace StormByte::Database::MariaDB;
using StormByte::Database::IsolationLevel;
using StormByte::Database::Transaction;
using StormByte::Database::ColumnNotFound;
using StormByte::Database::OutOfBounds;
using StormByte::Database::SslMode;
StormByte::Safe::Shared<StormByte::Logger::Log> logger =
	StormByte::Safe::Shared<StormByte::Logger::Log>::MakePointer<StormByte::Logger::ThreadedLog>(std::cout, StormByte::Logger::Level::Info);
class TestDatabase : public MariaDB {
	public:
		TestDatabase()
			: MariaDB("127.0.0.1", "testuser", "testpass", "stormbyte_test", 3306, logger) {
			SslMode(SslMode::Disable);
		}

		const ExpectedRows get_users() { return ExecuteSTMT("select_users"); }
		const ExpectedRows get_products() { return ExecuteSTMT("select_products"); }
		const ExpectedRows get_orders() { return ExecuteSTMT("select_orders"); }
		const ExpectedRows get_joined_data() { return ExecuteSTMT("select_join"); }
		const ExpectedRows get_blob() { return ExecuteSTMT("select_blob"); }
	private:
		void DoPostConnect() noexcept override {
			DoSilentQuery("CREATE TABLE IF NOT EXISTS users (id INT PRIMARY KEY AUTO_INCREMENT, name TEXT NOT NULL, email TEXT NOT NULL UNIQUE);");
			DoSilentQuery("CREATE TABLE IF NOT EXISTS products (id INT PRIMARY KEY AUTO_INCREMENT, name TEXT NOT NULL, price DOUBLE NOT NULL);");
			DoSilentQuery("CREATE TABLE IF NOT EXISTS orders (id INT PRIMARY KEY AUTO_INCREMENT, user_id INT, product_id INT, quantity INT NOT NULL, FOREIGN KEY (user_id) REFERENCES users(id), FOREIGN KEY (product_id) REFERENCES products(id));");
			DoSilentQuery("CREATE TABLE IF NOT EXISTS blobs (id INT PRIMARY KEY AUTO_INCREMENT, data BLOB);");
			DoSilentQuery("CREATE TABLE IF NOT EXISTS nulls (id INT PRIMARY KEY AUTO_INCREMENT, value TEXT);");
			DoSilentQuery("CREATE TABLE IF NOT EXISTS required_values (id INT PRIMARY KEY AUTO_INCREMENT, value TEXT NOT NULL);");
			DoSilentQuery("CREATE TABLE IF NOT EXISTS unsigned_values (id INT PRIMARY KEY AUTO_INCREMENT, value INT UNSIGNED NOT NULL);");
			DoSilentQuery("CREATE TABLE IF NOT EXISTS concurrent (id INT PRIMARY KEY AUTO_INCREMENT, value INTEGER);");
			DoSilentQuery("CREATE TABLE IF NOT EXISTS scalar_types (id INT PRIMARY KEY AUTO_INCREMENT, signed_integer INT NOT NULL, unsigned_integer BIGINT NOT NULL, signed_long BIGINT NOT NULL, unsigned_long BIGINT NOT NULL, real_number DOUBLE NOT NULL, text_value TEXT NOT NULL, blob_value LONGBLOB NOT NULL, flag BOOLEAN NOT NULL, nullable_value TEXT NULL);");
			DoSilentQuery("DELETE FROM orders;");
			DoSilentQuery("DELETE FROM blobs;");
			DoSilentQuery("DELETE FROM nulls;");
			DoSilentQuery("DELETE FROM required_values;");
			DoSilentQuery("DELETE FROM unsigned_values;");
			DoSilentQuery("DELETE FROM concurrent;");
			DoSilentQuery("DELETE FROM scalar_types;");
			DoSilentQuery("DELETE FROM users;");
			DoSilentQuery("DELETE FROM products;");
			DoSilentQuery("ALTER TABLE orders AUTO_INCREMENT=1;");
			DoSilentQuery("ALTER TABLE blobs AUTO_INCREMENT=1;");
			DoSilentQuery("ALTER TABLE nulls AUTO_INCREMENT=1;");
			DoSilentQuery("ALTER TABLE required_values AUTO_INCREMENT=1;");
			DoSilentQuery("ALTER TABLE unsigned_values AUTO_INCREMENT=1;");
			DoSilentQuery("ALTER TABLE concurrent AUTO_INCREMENT=1;");
			DoSilentQuery("ALTER TABLE scalar_types AUTO_INCREMENT=1;");
			DoSilentQuery("ALTER TABLE users AUTO_INCREMENT=1;");
			DoSilentQuery("ALTER TABLE products AUTO_INCREMENT=1;");
			DoSilentQuery("INSERT INTO users (name, email) VALUES ('Alice', 'alice@example.com');");
			DoSilentQuery("INSERT INTO users (name, email) VALUES ('Bob', 'bob@example.com');");
			DoSilentQuery("INSERT INTO products (name, price) VALUES ('Laptop', 999.99);");
			DoSilentQuery("INSERT INTO products (name, price) VALUES ('Mouse', 19.99);");
			DoSilentQuery("INSERT INTO orders (user_id, product_id, quantity) VALUES (1, 1, 1);");
			DoSilentQuery("INSERT INTO orders (user_id, product_id, quantity) VALUES (2, 2, 2);");
			DoSilentQuery("INSERT INTO nulls (value) VALUES (NULL);");
			DoPrepareSTMT("select_users", "SELECT name, email FROM users;");
			DoPrepareSTMT("select_products", "SELECT name, price FROM products;");
			DoPrepareSTMT("select_orders", "SELECT user_id, product_id, quantity FROM orders;");
			DoPrepareSTMT("select_join", "SELECT users.name, products.name, orders.quantity FROM orders JOIN users ON orders.user_id = users.id JOIN products ON orders.product_id = products.id;");
			DoPrepareSTMT("insert_blob", "INSERT INTO blobs (data) VALUES (?);");
			DoPrepareSTMT("select_blob", "SELECT data FROM blobs ORDER BY id DESC LIMIT 1;");
			DoPrepareSTMT("insert_null", "INSERT INTO nulls (value) VALUES (?);");
			DoPrepareSTMT("select_nulls", "SELECT value FROM nulls;");
			DoPrepareSTMT("insert_required", "INSERT INTO required_values (value) VALUES (?);");
			DoPrepareSTMT("insert_unsigned", "INSERT INTO unsigned_values (value) VALUES (?);");
			DoPrepareSTMT("insert_concurrent", "INSERT INTO concurrent (value) VALUES (?);");
			DoPrepareSTMT("count_concurrent", "SELECT COUNT(*) FROM concurrent;");
			DoPrepareSTMT("insert_scalar_types", "INSERT INTO scalar_types (signed_integer, unsigned_integer, signed_long, unsigned_long, real_number, text_value, blob_value, flag, nullable_value) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);");
		}
};
class ConcurrentDatabase : public MariaDB {
	public:
		ConcurrentDatabase()
			: MariaDB("127.0.0.1", "testuser", "testpass", "stormbyte_test", 3306, logger) {
			SslMode(SslMode::Disable);
		}

	private:
		void DoPostConnect() noexcept override {
			DoSilentQuery("CREATE TABLE IF NOT EXISTS concurrent (id INT PRIMARY KEY AUTO_INCREMENT, value INTEGER);");
			DoPrepareSTMT("insert_concurrent", "INSERT INTO concurrent (value) VALUES (?);");
			DoPrepareSTMT("count_concurrent", "SELECT COUNT(*) FROM concurrent;");
		}
};
int not_connected_query() {
	TestDatabase db;
	auto res = db.Query("SELECT 1;");
	ASSERT_FALSE(res.has_value());
	auto telemetry = db.Telemetry();
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::Query).Failures);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Events(StormByte::Database::BackendEvent::Connection));
	RETURN_TEST(0);
}

int connected_database_move() {
	std::unique_ptr<TestDatabase> moved;
	{
		TestDatabase source;
		ASSERT_TRUE(source.Connect());
		moved = std::make_unique<TestDatabase>(std::move(source));
	}

	ASSERT_TRUE(moved->IsConnected());
	ASSERT_TRUE(moved->Query("SELECT 1;").has_value());
	TestDatabase reassigned;
	{
		TestDatabase source;
		ASSERT_TRUE(source.Connect());
		reassigned = std::move(source);
	}

	ASSERT_TRUE(reassigned.IsConnected());
	ASSERT_TRUE(reassigned.Query("SELECT 1;").has_value());
	RETURN_TEST(0);
}

int not_connected_silent() {
	TestDatabase db;
	ASSERT_FALSE(db.SilentQuery("SELECT 1;"));
	RETURN_TEST(0);
}

int not_connected_execute() {
	TestDatabase db;
	auto res = db.ExecuteSTMT("select_users");
	ASSERT_FALSE(res.has_value());
	RETURN_TEST(0);
}

int not_connected_transaction() {
	TestDatabase db;
	auto tx = db.BeginTransaction();
	ASSERT_FALSE(tx.has_value());
	ASSERT_TRUE(tx.error() != nullptr);
	ASSERT_CONTAINS(std::string{tx.error()->what()}, "Unable to begin transaction");
	RETURN_TEST(0);
}

int is_connected_test() {
	TestDatabase db;
	ASSERT_FALSE(db.IsConnected());
	db.Connect();
	ASSERT_TRUE(db.IsConnected());
	db.Disconnect();
	ASSERT_FALSE(db.IsConnected());
	db.Disconnect();
	ASSERT_TRUE(db.Connect());
	db.Disconnect();
	RETURN_TEST(0);
}

int double_connect() {
	TestDatabase db;
	ASSERT_TRUE(db.Connect());
	ASSERT_FALSE(db.Connect());
	RETURN_TEST(0);
}

int verify_inserted_users() {
	TestDatabase db;
	db.Connect();
	auto expected_rows = db.get_users();
	ASSERT_TRUE(expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(2, rows.Count());
	ASSERT_EQUAL(2, rows[0].Count());
	ASSERT_EQUAL("Alice", rows[0][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL("alice@example.com", rows[0][1].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(2, rows[1].Count());
	ASSERT_EQUAL("Bob", rows[1][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL("bob@example.com", rows[1][1].Get<StormByte::Safe::String>());
	RETURN_TEST(0);
}

int verify_inserted_products() {
	TestDatabase db;
	db.Connect();
	auto expected_rows = db.get_products();
	ASSERT_TRUE(expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(2, rows.Count());
	ASSERT_EQUAL(2, rows[0].Count());
	ASSERT_EQUAL("Laptop", rows[0][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(999.99, rows[0][1].Get<double>());
	ASSERT_EQUAL(2, rows[1].Count());
	ASSERT_EQUAL("Mouse", rows[1][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(19.99, rows[1][1].Get<double>());
	RETURN_TEST(0);
}

int verify_inserted_orders() {
	TestDatabase db;
	db.Connect();
	auto expected_rows = db.get_orders();
	ASSERT_TRUE(expected_rows.has_value());
	const auto rows = expected_rows.value();
	ASSERT_EQUAL(2, rows.Count());
	ASSERT_EQUAL(3, rows[0].Count());
	ASSERT_EQUAL(1, rows[0][0].Get<long long int>());
	ASSERT_EQUAL(1, rows[0][1].Get<long long int>());
	ASSERT_EQUAL(1, rows[0][2].Get<long long int>());
	ASSERT_EQUAL(3, rows[1].Count());
	ASSERT_EQUAL(2, rows[1][0].Get<long long int>());
	ASSERT_EQUAL(2, rows[1][1].Get<long long int>());
	ASSERT_EQUAL(2, rows[1][2].Get<long long int>());
	RETURN_TEST(0);
}

int verify_relationships() {
	TestDatabase db;
	db.Connect();
	auto expected_rows = db.get_joined_data();
	ASSERT_TRUE(expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(2, rows.Count());
	ASSERT_EQUAL(3, rows[0].Count());
	ASSERT_EQUAL("Alice", rows[0][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL("Laptop", rows[0][1].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(1, rows[0][2].Get<long long int>());
	ASSERT_EQUAL(3, rows[1].Count());
	ASSERT_EQUAL("Bob", rows[1][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL("Mouse", rows[1][1].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(2, rows[1][2].Get<long long int>());
	RETURN_TEST(0);
}

int query_test() {
	TestDatabase db;
	db.Connect();
	auto expected_rows = db.Query("SELECT COUNT(*) FROM users;");
	ASSERT_TRUE(expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(1, rows.Count());
	ASSERT_EQUAL(1, rows[0].Count());
	ASSERT_EQUAL(2, rows[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int empty_result_test() {
	TestDatabase db;
	db.Connect();
	auto expected_rows = db.Query("SELECT * FROM users WHERE name = 'NonExistent';");
	ASSERT_TRUE(expected_rows.has_value());
	ASSERT_EQUAL(0, expected_rows.value().Count());
	RETURN_TEST(0);
}

int syntax_error_test() {
	TestDatabase db;
	db.Connect();
	auto res = db.Query("SELEC * FROM users;");
	ASSERT_FALSE(res.has_value());
	RETURN_TEST(0);
}

int silent_syntax_error_preserves_connection() {
	TestDatabase db;
	ASSERT_TRUE(db.Connect());
	ASSERT_FALSE(db.SilentQuery("SELEC * FROM users;"));
	auto rows = db.Query("SELECT COUNT(*) FROM users;");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(2, rows.value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int missing_required_bind_is_error() {
	TestDatabase db;
	ASSERT_TRUE(db.Connect());
	auto result = db.ExecuteSTMT("insert_required");
	ASSERT_FALSE(result.has_value());
	ASSERT_FALSE(db.ExecuteSTMT("insert_required", "extra", "argument").has_value());
	ASSERT_TRUE(db.ExecuteSTMT("insert_required", "valid").has_value());
	auto rows = db.Query("SELECT COUNT(*) FROM required_values;");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(1, rows.value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int unsigned_bind_preserves_value() {
	TestDatabase db;
	ASSERT_TRUE(db.Connect());
	const unsigned int value = std::numeric_limits<unsigned int>::max();
	ASSERT_TRUE(db.ExecuteSTMT("insert_unsigned", value).has_value());
	auto rows = db.Query("SELECT CAST(value AS CHAR) FROM unsigned_values;");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(StormByte::Safe::String{std::to_string(value)}, rows.value()[0][0].Get<StormByte::Safe::String>());
	RETURN_TEST(0);
}

int constraint_violation_preserves_connection() {
	TestDatabase db;
	ASSERT_TRUE(db.Connect());
	ASSERT_FALSE(db.SilentQuery("INSERT INTO users (name, email) VALUES ('Mallory', 'alice@example.com');"));
	auto rows = db.Query("SELECT COUNT(*) FROM users;");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(2, rows.value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int invalid_row_index_throws() {
	TestDatabase db;
	ASSERT_TRUE(db.Connect());
	auto rows = db.get_users();
	ASSERT_TRUE(rows.has_value());
	bool threw = false;
	try {
		(void)rows.value()[0][99];
	} catch (const OutOfBounds&) {
		threw = true;
	}

	ASSERT_TRUE(threw);
	RETURN_TEST(0);
}

int bool_test() {
	TestDatabase db;
	db.Connect();
	auto expected_rows = db.Query("SELECT COUNT(*) > 0 FROM users;");
	ASSERT_TRUE(expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(1, rows.Count());
	ASSERT_EQUAL(1, rows[0].Count());
	ASSERT_TRUE(rows[0][0].Get<long long int>() != 0);
	RETURN_TEST(0);
}

int scalar_backend_contract() {
	TestDatabase db;
	int result = verify_scalar_backend_contract(db);
	TestDatabase binary_db;
	result += verify_binary_backend_contract(binary_db);
	return result;
}

int verify_blobs() {
	TestDatabase db;
	db.Connect();
	StormByte::Safe::Binary data{std::byte{0}, std::byte{1}, std::byte{2}, std::byte{0xFF}};
	auto insert_res = db.ExecuteSTMT("insert_blob", data);
	ASSERT_TRUE(insert_res.has_value());
	auto expected_rows = db.get_blob();
	ASSERT_TRUE(expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(1, rows.Count());
	ASSERT_EQUAL(1, rows[0].Count());
	const auto& blob = rows[0][0].Get<StormByte::Safe::Binary>();
	ASSERT_EQUAL(4, static_cast<int>(blob.size()));
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(blob.data());
	ASSERT_EQUAL(0, static_cast<int>(bytes[0]));
	ASSERT_EQUAL(1, static_cast<int>(bytes[1]));
	ASSERT_EQUAL(2, static_cast<int>(bytes[2]));
	ASSERT_EQUAL(255, static_cast<int>(bytes[3]));
	RETURN_TEST(0);
}

int empty_blob_test() {
	TestDatabase db;
	db.Connect();
	StormByte::Safe::Binary empty;
	auto insert_res = db.ExecuteSTMT("insert_blob", empty);
	ASSERT_TRUE(insert_res.has_value());
	auto rows = db.get_blob();
	ASSERT_TRUE(rows.has_value());
	ASSERT_FALSE(rows.value()[0][0].IsNull());
	ASSERT_TRUE(rows.value()[0][0].Get<StormByte::Safe::Binary>().empty());
	RETURN_TEST(0);
}

int null_value_test() {
	TestDatabase db;
	db.Connect();
	auto rows = db.ExecuteSTMT("select_nulls");
	ASSERT_TRUE(rows.has_value());
	ASSERT_TRUE(rows.value()[0][0].IsNull());
	RETURN_TEST(0);
}

int bind_null_test() {
	TestDatabase db;
	db.Connect();
	auto res = db.ExecuteSTMT("insert_null", nullptr);
	ASSERT_TRUE(res.has_value());
	auto rows = db.Query("SELECT value FROM nulls ORDER BY id DESC LIMIT 1;");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(1, rows.value().Count());
	ASSERT_TRUE(rows.value()[0][0].IsNull());
	RETURN_TEST(0);
}

int unknown_stmt_test() {
	TestDatabase db;
	db.Connect();
	auto res = db.ExecuteSTMT("non_existent_stmt");
	ASSERT_FALSE(res.has_value());
	ASSERT_CONTAINS(std::string{res.error()->what()}, "not found");
	RETURN_TEST(0);
}

int name_access_test() {
	TestDatabase db;
	db.Connect();
	auto expected_rows = db.get_users();
	ASSERT_TRUE(expected_rows.has_value());
	ASSERT_EQUAL("Alice", expected_rows.value()[0]["name"].Get<StormByte::Safe::String>());
	ASSERT_EQUAL("alice@example.com", expected_rows.value()[0]["email"].Get<StormByte::Safe::String>());
	RETURN_TEST(0);
}

int name_access_missing_column() {
	TestDatabase db;
	db.Connect();
	auto expected_rows = db.get_users();
	ASSERT_TRUE(expected_rows.has_value());
	bool threw = false;
	try {
		(void)expected_rows.value()[0]["non_existent_column"];
	} catch (const ColumnNotFound&) {
		threw = true;
	}

	ASSERT_TRUE(threw);
	RETURN_TEST(0);
}

int transaction_commit_test() {
	TestDatabase db;
	db.Connect();
	{
		auto tx_result = db.BeginTransaction();
		ASSERT_TRUE(tx_result.has_value());
		auto tx = std::move(*tx_result);
		ASSERT_TRUE(tx.IsActive());
		db.SilentQuery("INSERT INTO users (name, email) VALUES ('Charlie', 'charlie@example.com');");
		tx.Commit();
		ASSERT_FALSE(tx.IsActive());
		tx.Commit();
	}

	auto rows = db.Query("SELECT COUNT(*) FROM users;");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(3, rows.value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int transaction_rollback_explicit() {
	TestDatabase db;
	db.Connect();
	{
		auto tx_result = db.BeginTransaction();
		ASSERT_TRUE(tx_result.has_value());
		auto tx = std::move(*tx_result);
		db.SilentQuery("INSERT INTO users (name, email) VALUES ('David', 'david@example.com');");
		db.SilentQuery("INSERT INTO users (name, email) VALUES ('David Two', 'david2@example.com');");
		tx.Rollback();
		ASSERT_FALSE(tx.IsActive());
		tx.Rollback();
	}

	auto rows = db.Query("SELECT COUNT(*) FROM users WHERE name = 'David';");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(0, rows.value()[0][0].Get<long long int>());
	rows = db.Query("SELECT COUNT(*) FROM users WHERE email = 'david2@example.com';");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(0, rows.value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int transaction_rollback_after_statement_error() {
	TestDatabase db;
	ASSERT_TRUE(db.Connect());
	auto tx_result = db.BeginTransaction();
	ASSERT_TRUE(tx_result.has_value());
	auto tx = std::move(*tx_result);
	ASSERT_TRUE(db.SilentQuery("INSERT INTO users (name, email) VALUES ('Transient', 'transient@example.com');"));
	ASSERT_FALSE(db.SilentQuery("INSERT INTO users (name, email) VALUES ('Conflict', 'alice@example.com');"));
	tx.Rollback();
	auto rows = db.Query("SELECT COUNT(*) FROM users WHERE email = 'transient@example.com';");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(0, rows.value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int transaction_rollback_auto() {
	TestDatabase db;
	db.Connect();
	{
		auto tx_result = db.BeginTransaction();
		ASSERT_TRUE(tx_result.has_value());
		auto tx = std::move(*tx_result);
		db.SilentQuery("INSERT INTO users (name, email) VALUES ('Eve', 'eve@example.com');");
	}

	auto rows = db.Query("SELECT COUNT(*) FROM users WHERE name = 'Eve';");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(0, rows.value()[0][0].Get<long long int>());
	RETURN_TEST(0);
}

int isolation_default() {
	TestDatabase db;
	db.Connect();
	auto tx_result = db.BeginTransaction(IsolationLevel::Default);
	ASSERT_TRUE(tx_result.has_value());
	auto tx = std::move(*tx_result);
	tx.Commit();
	RETURN_TEST(0);
}

int isolation_serializable() {
	TestDatabase db;
	db.Connect();
	auto tx_result = db.BeginTransaction(IsolationLevel::Serializable);
	ASSERT_TRUE(tx_result.has_value());
	auto tx = std::move(*tx_result);
	tx.Commit();
	RETURN_TEST(0);
}

int isolation_repeatable_read() {
	TestDatabase db;
	db.Connect();
	auto tx_result = db.BeginTransaction(IsolationLevel::RepeatableRead);
	ASSERT_TRUE(tx_result.has_value());
	auto tx = std::move(*tx_result);
	tx.Commit();
	RETURN_TEST(0);
}

int concurrent_multiple_connections() {
	constexpr int num_threads = 6;
	constexpr int inserts_per_thread = 40;
	{
		ConcurrentDatabase setup;
		ASSERT_TRUE(setup.Connect());
		ASSERT_TRUE(setup.SilentQuery("DELETE FROM concurrent;"));
	}

	std::vector<std::thread> threads;
	std::atomic<int> failures{};
	for (int t = 0; t < num_threads; ++t) {
		threads.emplace_back([t, &failures]() {
			ConcurrentDatabase local_db;
			if (!local_db.Connect()) {
				++failures;
				return;
			}
			for (int i = 0; i < inserts_per_thread; ++i) {
				bool inserted = false;
				for (int attempt = 0; attempt < 50; ++attempt) {
					auto res = local_db.ExecuteSTMT("insert_concurrent", t * 1000 + i);
					if (res.has_value()) {
						inserted = true;
						break;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
				if (!inserted)
					++failures;
			}
		});
	}

	for (auto& th : threads)
		th.join();
	ASSERT_EQUAL(0, failures.load());
	ConcurrentDatabase check_db;
	ASSERT_TRUE(check_db.Connect());
	auto rows = check_db.ExecuteSTMT("count_concurrent");
	ASSERT_TRUE(rows.has_value());
	ASSERT_EQUAL(num_threads * inserts_per_thread, rows.value()[0][0].Get<long long int>());
	check_db.SilentQuery("DELETE FROM concurrent;");
	RETURN_TEST(0);
}

int concurrent_shared_connection_and_transaction() {
	constexpr int thread_count = 4;
	constexpr int inserts_per_thread = 50;
	ConcurrentDatabase db;
	ASSERT_TRUE(db.Connect());
	ASSERT_TRUE(db.SilentQuery("DELETE FROM concurrent;"));
	std::atomic<int> failures{};
	std::vector<std::thread> threads;
	for (int thread_index{}; thread_index < thread_count; ++thread_index) {
		threads.emplace_back([thread_index, &db, &failures]() {
			for (int insert_index{}; insert_index < inserts_per_thread; ++insert_index) {
				if (!db.ExecuteSTMT("insert_concurrent", thread_index * 1000 + insert_index).has_value())
					++failures;
			}
		});
	}
	for (auto& thread : threads)
		thread.join();
	ASSERT_EQUAL(0, failures.load());
	auto count_rows = db.ExecuteSTMT("count_concurrent");
	ASSERT_TRUE(count_rows.has_value());
	ASSERT_EQUAL(thread_count * inserts_per_thread, count_rows.value()[0][0].Get<long long int>());
	ASSERT_TRUE(db.SilentQuery("DELETE FROM concurrent;"));

	auto tx_result = db.BeginTransaction();
	ASSERT_TRUE(tx_result.has_value());
	auto tx = std::move(*tx_result);
	ASSERT_TRUE(db.SilentQuery("INSERT INTO concurrent (value) VALUES (1);"));
	std::promise<void> started;
	auto started_signal = started.get_future();
	auto worker = std::async(std::launch::async, [&db, started = std::move(started)]() mutable {
		started.set_value();
		return db.SilentQuery("INSERT INTO concurrent (value) VALUES (2);");
	});
	started_signal.wait();
	ASSERT_TRUE(worker.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout);
	tx.Rollback();
	ASSERT_TRUE(worker.get());
	count_rows = db.ExecuteSTMT("count_concurrent");
	ASSERT_TRUE(count_rows.has_value());
	ASSERT_EQUAL(1, count_rows.value()[0][0].Get<long long int>());
	ASSERT_TRUE(db.SilentQuery("DELETE FROM concurrent;"));
	RETURN_TEST(0);
}

int telemetry_tracks_mariadb_operations_and_survives_database() {
	StormByte::Safe::Shared<StormByte::Database::Telemetry> retained;
	std::uint64_t warnings_before{};
	{
		TestDatabase db;
		retained = db.Telemetry();
		ASSERT_TRUE(retained != nullptr);
		ASSERT_TRUE(dynamic_cast<StormByte::Database::MariaDB::Telemetry*>(retained.get()) != nullptr);
		ASSERT_TRUE(db.Connect());
		warnings_before = dynamic_cast<StormByte::Database::MariaDB::Telemetry*>(retained.get())->Warnings();
		ASSERT_TRUE(db.Query("SELECT 1;").has_value());
		ASSERT_FALSE(db.Query("SELEC 1;").has_value());
		ASSERT_TRUE(db.ExecuteSTMT("select_users").has_value());
		ASSERT_FALSE(db.ExecuteSTMT("missing_telemetry_statement").has_value());
		ASSERT_FALSE(db.SilentQuery("INSERT INTO users (name, email) VALUES ('Duplicate', 'alice@example.com');"));
		ASSERT_TRUE(db.SilentQuery("INSERT IGNORE INTO users (name, email) VALUES ('Duplicate', 'alice@example.com');"));
		auto transaction = db.BeginTransaction();
		ASSERT_TRUE(transaction.has_value());
		transaction->Rollback();
		auto committed_transaction = db.BeginTransaction();
		ASSERT_TRUE(committed_transaction.has_value());
		committed_transaction->Commit();
		db.Disconnect();
	}

	const auto* telemetry = dynamic_cast<const StormByte::Database::MariaDB::Telemetry*>(retained.get());
	ASSERT_TRUE(telemetry != nullptr);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::Connect).Successes);
	ASSERT_EQUAL(std::uint64_t{2}, telemetry->Metrics(StormByte::Database::Operation::Disconnect).Successes);
	const auto query = telemetry->Metrics(StormByte::Database::Operation::Query);
	ASSERT_EQUAL(std::uint64_t{2}, query.Attempts);
	ASSERT_EQUAL(std::uint64_t{1}, query.Successes);
	ASSERT_EQUAL(std::uint64_t{1}, query.Failures);
	ASSERT_TRUE(query.MinimumNanoseconds <= query.MeanNanoseconds());
	ASSERT_TRUE(query.MeanNanoseconds() <= query.MaximumNanoseconds);
	ASSERT_EQUAL(std::uint64_t{3}, telemetry->RowsReturned());
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::PreparedStatement).Failures);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::PreparedStatement).Successes);
	const auto prepare_metrics = telemetry->Metrics(StormByte::Database::Operation::PrepareStatement);
	ASSERT_TRUE(prepare_metrics.Attempts > 0);
	ASSERT_EQUAL(prepare_metrics.Attempts, prepare_metrics.Successes);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::SilentQuery).Failures);
	ASSERT_EQUAL(std::uint64_t{2}, telemetry->Metrics(StormByte::Database::Operation::BeginTransaction).Successes);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::CommitTransaction).Successes);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::RollbackTransaction).Successes);
	ASSERT_EQUAL(std::uint64_t{1}, telemetry->Events(StormByte::Database::BackendEvent::Constraint));
	ASSERT_TRUE(telemetry->Deadlocks() == 0);
	ASSERT_EQUAL(warnings_before + 1, telemetry->Warnings());
	ASSERT_CONTAINS(static_cast<std::string>(*retained), "MariaDB{");
	RETURN_TEST(0);
}

int main() {
	int result = 0;
	result += not_connected_query();
	result += connected_database_move();
	result += not_connected_silent();
	result += not_connected_execute();
	result += not_connected_transaction();
	result += is_connected_test();
	result += double_connect();
	result += verify_inserted_users();
	result += verify_inserted_products();
	result += verify_inserted_orders();
	result += verify_relationships();
	result += query_test();
	result += empty_result_test();
	result += syntax_error_test();
	result += silent_syntax_error_preserves_connection();
	result += missing_required_bind_is_error();
	result += unsigned_bind_preserves_value();
	result += constraint_violation_preserves_connection();
	result += invalid_row_index_throws();
	result += bool_test();
	result += scalar_backend_contract();
	result += verify_blobs();
	result += empty_blob_test();
	result += null_value_test();
	result += bind_null_test();
	result += unknown_stmt_test();
	result += name_access_test();
	result += name_access_missing_column();
	result += transaction_commit_test();
	result += transaction_rollback_explicit();
	result += transaction_rollback_after_statement_error();
	result += transaction_rollback_auto();
	result += isolation_default();
	result += isolation_serializable();
	result += isolation_repeatable_read();
	result += concurrent_multiple_connections();
	result += concurrent_shared_connection_and_transaction();
	result += telemetry_tracks_mariadb_operations_and_survives_database();
	if (result == 0) {
		std::cout << "All tests passed successfully.\n";
	} else {
		std::cout << result << " tests failed.\n";
	}

	return result;
}
