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

#include <StormByte/database/sqlite/sqlite3.hxx>
#include <StormByte/exception.hxx>
#include <StormByte/database/transaction.hxx>
#include <StormByte/logger/log.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/test_handlers.h>
#include "backend_contract.hxx"
#include <StormByte/uuid.hxx>
#include <memory>
#include <atomic>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>
#include <limits>
#include <future>
using ExpectedRows = StormByte::Database::ExpectedRows;
using namespace StormByte::Database::SQLite;
using StormByte::Database::IsolationLevel;
using StormByte::Database::Transaction;
using StormByte::Database::ColumnNotFound;
using StormByte::Database::OutOfBounds;
StormByte::Safe::Shared<StormByte::Logger::Log> logger =
	StormByte::Safe::Shared<StormByte::Logger::Log>::MakePointer<StormByte::Logger::ThreadedLog>(std::cout, StormByte::Logger::Level::Info);

namespace {
	std::filesystem::path TemporaryDatabasePath() {
		const std::string uuid{static_cast<std::string_view>(StormByte::GenerateUUIDv4())};
		return std::filesystem::temp_directory_path() / ("stormbyte_sqlite_concurrent_" + uuid + ".db");
	}
}

class TestMemoryDatabase : public SQLite3 {
	public:
		TestMemoryDatabase() : SQLite3(logger) {}
		void RegisterStatement(std::string_view name, std::string_view query) { DoPrepareSTMT(name, query); }
		const ExpectedRows get_users() { return ExecuteSTMT("select_users"); }
		const ExpectedRows get_products() { return ExecuteSTMT("select_products"); }
		const ExpectedRows get_orders() { return ExecuteSTMT("select_orders"); }
		const ExpectedRows get_joined_data() { return ExecuteSTMT("select_join"); }
		const ExpectedRows get_blob() { return ExecuteSTMT("select_blob"); }
	private:
		void DoPostConnect() noexcept override {
			DoSilentQuery("PRAGMA foreign_keys = ON;");
			DoSilentQuery("CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, email TEXT NOT NULL UNIQUE);");
			DoSilentQuery("CREATE TABLE products (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, price REAL NOT NULL);");
			DoSilentQuery("CREATE TABLE orders (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, product_id INTEGER, quantity INTEGER NOT NULL, FOREIGN KEY (user_id) REFERENCES users(id), FOREIGN KEY (product_id) REFERENCES products(id));");
			DoSilentQuery("CREATE TABLE blobs (id INTEGER PRIMARY KEY AUTOINCREMENT, data BLOB);");
			DoSilentQuery("CREATE TABLE nulls (id INTEGER PRIMARY KEY AUTOINCREMENT, value TEXT);");
			DoSilentQuery("CREATE TABLE required_values (id INTEGER PRIMARY KEY AUTOINCREMENT, value TEXT NOT NULL);");
			DoSilentQuery("CREATE TABLE unsigned_values (id INTEGER PRIMARY KEY AUTOINCREMENT, value INTEGER NOT NULL);");
			DoSilentQuery("CREATE TABLE concurrent (id INTEGER PRIMARY KEY AUTOINCREMENT, value INTEGER);");
			DoSilentQuery("CREATE TABLE scalar_types (id INTEGER PRIMARY KEY AUTOINCREMENT, signed_integer INTEGER NOT NULL, unsigned_integer INTEGER NOT NULL, signed_long INTEGER NOT NULL, unsigned_long INTEGER NOT NULL, real_number REAL NOT NULL, text_value TEXT NOT NULL, blob_value BLOB NOT NULL, flag INTEGER NOT NULL, nullable_value TEXT);");
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

class ThrowingBaseExceptionDatabase final : public SQLite3 {
	public:
		ThrowingBaseExceptionDatabase() : SQLite3(logger) {}

	private:
		void DoBeginTransaction(IsolationLevel) override {
			throw StormByte::Exception(std::string_view{"base begin failure"});
		}
};
class TestFileDatabase : public SQLite3 {
	public:
		TestFileDatabase(const std::filesystem::path& path)
			: SQLite3(path, logger) {}
		void DoPostConnect() noexcept override {
			DoSilentQuery("PRAGMA foreign_keys = ON;");
			DoSilentQuery("PRAGMA journal_mode=WAL;");
			DoSilentQuery("PRAGMA synchronous=NORMAL;");
			DoSilentQuery("PRAGMA busy_timeout=30000;");
			DoSilentQuery("CREATE TABLE IF NOT EXISTS concurrent (id INTEGER PRIMARY KEY AUTOINCREMENT, value INTEGER);");
			DoPrepareSTMT("insert_concurrent", "INSERT INTO concurrent (value) VALUES (?);");
			DoPrepareSTMT("count_concurrent", "SELECT COUNT(*) FROM concurrent;");
		}
};
int not_connected_query() {
	const std::string fn_name = "not_connected_query";
	TestMemoryDatabase db;
	auto res = db.Query("SELECT 1;");
	ASSERT_FALSE(fn_name, res.has_value());
	const auto telemetry = db.Telemetry();
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::Query).Failures);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->Events(StormByte::Database::BackendEvent::Connection));
	RETURN_TEST(fn_name, 0);
}

int not_connected_silent() {
	const std::string fn_name = "not_connected_silent";
	TestMemoryDatabase db;
	ASSERT_FALSE(fn_name, db.SilentQuery("SELECT 1;"));
	RETURN_TEST(fn_name, 0);
}

int not_connected_execute() {
	const std::string fn_name = "not_connected_execute";
	TestMemoryDatabase db;
	auto res = db.ExecuteSTMT("select_users");
	ASSERT_FALSE(fn_name, res.has_value());
	RETURN_TEST(fn_name, 0);
}

int not_connected_transaction() {
	const std::string fn_name = "not_connected_transaction";
	TestMemoryDatabase db;
	auto tx = db.BeginTransaction();
	ASSERT_FALSE(fn_name, tx.has_value());
	ASSERT_TRUE(fn_name, tx.error() != nullptr);
	ASSERT_TRUE(fn_name, std::string{tx.error()->what()}.find("Unable to begin transaction") != std::string::npos);
	RETURN_TEST(fn_name, 0);
}

int is_connected_test() {
	const std::string fn_name = "is_connected_test";
	TestMemoryDatabase db;
	ASSERT_FALSE(fn_name, db.IsConnected());
	db.Connect();
	ASSERT_TRUE(fn_name, db.IsConnected());
	db.Disconnect();
	ASSERT_FALSE(fn_name, db.IsConnected());
	db.Disconnect();
	ASSERT_TRUE(fn_name, db.Connect());
	db.Disconnect();
	RETURN_TEST(fn_name, 0);
}

int double_connect() {
	const std::string fn_name = "double_connect";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	ASSERT_FALSE(fn_name, db.Connect());
	RETURN_TEST(fn_name, 0);
}

int verify_inserted_users() {
	const std::string fn_name = "verify_inserted_users";
	TestMemoryDatabase db;
	db.Connect();
	auto expected_rows = db.get_users();
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(fn_name, 2, rows.Count());
	ASSERT_EQUAL(fn_name, "Alice", rows[0][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, "alice@example.com", rows[0][1].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, "Bob", rows[1][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, "bob@example.com", rows[1][1].Get<StormByte::Safe::String>());
	RETURN_TEST(fn_name, 0);
}

int verify_inserted_products() {
	const std::string fn_name = "verify_inserted_products";
	TestMemoryDatabase db;
	db.Connect();
	auto expected_rows = db.get_products();
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(fn_name, 2, rows.Count());
	ASSERT_EQUAL(fn_name, "Laptop", rows[0][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, 999.99, rows[0][1].Get<double>());
	ASSERT_EQUAL(fn_name, "Mouse", rows[1][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, 19.99, rows[1][1].Get<double>());
	RETURN_TEST(fn_name, 0);
}

int verify_inserted_orders() {
	const std::string fn_name = "verify_inserted_orders";
	TestMemoryDatabase db;
	db.Connect();
	auto expected_rows = db.get_orders();
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(fn_name, 2, rows.Count());
	ASSERT_EQUAL(fn_name, 1, rows[0][0].Get<int>());
	ASSERT_EQUAL(fn_name, 1, rows[0][1].Get<int>());
	ASSERT_EQUAL(fn_name, 1, rows[0][2].Get<int>());
	ASSERT_EQUAL(fn_name, 2, rows[1][0].Get<int>());
	ASSERT_EQUAL(fn_name, 2, rows[1][1].Get<int>());
	ASSERT_EQUAL(fn_name, 2, rows[1][2].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int verify_relationships() {
	const std::string fn_name = "verify_relationships";
	TestMemoryDatabase db;
	db.Connect();
	auto expected_rows = db.get_joined_data();
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	const auto& rows = expected_rows.value();
	ASSERT_EQUAL(fn_name, 2, rows.Count());
	ASSERT_EQUAL(fn_name, "Alice", rows[0][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, "Laptop", rows[0][1].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, 1, rows[0][2].Get<int>());
	ASSERT_EQUAL(fn_name, "Bob", rows[1][0].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, "Mouse", rows[1][1].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, 2, rows[1][2].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int query_test() {
	const std::string fn_name = "query_test";
	TestMemoryDatabase db;
	db.Connect();
	auto expected_rows = db.Query("SELECT COUNT(*) FROM users;");
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	ASSERT_EQUAL(fn_name, 2, expected_rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int empty_result_test() {
	const std::string fn_name = "empty_result_test";
	TestMemoryDatabase db;
	db.Connect();
	auto expected_rows = db.Query("SELECT * FROM users WHERE name = 'NonExistent';");
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	ASSERT_EQUAL(fn_name, 0, expected_rows.value().Count());
	RETURN_TEST(fn_name, 0);
}

int syntax_error_test() {
	const std::string fn_name = "syntax_error_test";
	TestMemoryDatabase db;
	db.Connect();
	auto res = db.Query("SELEC * FROM users;");
	ASSERT_FALSE(fn_name, res.has_value());
	RETURN_TEST(fn_name, 0);
}

int silent_syntax_error_preserves_connection() {
	const std::string fn_name = "silent_syntax_error_preserves_connection";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	ASSERT_FALSE(fn_name, db.SilentQuery("SELEC * FROM users;"));
	auto rows = db.Query("SELECT COUNT(*) FROM users;");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 2, rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int missing_required_bind_is_error() {
	const std::string fn_name = "missing_required_bind_is_error";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	auto result = db.ExecuteSTMT("insert_required");
	ASSERT_FALSE(fn_name, result.has_value());
	ASSERT_FALSE(fn_name, db.ExecuteSTMT("insert_required", "extra", "argument").has_value());
	ASSERT_TRUE(fn_name, db.ExecuteSTMT("insert_required", "valid").has_value());
	auto rows = db.Query("SELECT COUNT(*) FROM required_values;");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 1, rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int unsigned_bind_preserves_value() {
	const std::string fn_name = "unsigned_bind_preserves_value";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	const unsigned int value = std::numeric_limits<unsigned int>::max();
	ASSERT_TRUE(fn_name, db.ExecuteSTMT("insert_unsigned", value).has_value());
	auto rows = db.Query("SELECT CAST(value AS TEXT) FROM unsigned_values;");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, StormByte::Safe::String{std::to_string(value)}, rows.value()[0][0].Get<StormByte::Safe::String>());
	RETURN_TEST(fn_name, 0);
}

int constraint_violation_preserves_connection() {
	const std::string fn_name = "constraint_violation_preserves_connection";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	ASSERT_FALSE(fn_name, db.SilentQuery("INSERT INTO users (name, email) VALUES ('Mallory', 'alice@example.com');"));
	auto rows = db.Query("SELECT COUNT(*) FROM users;");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 2, rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int invalid_row_index_throws() {
	const std::string fn_name = "invalid_row_index_throws";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	auto rows = db.get_users();
	ASSERT_TRUE(fn_name, rows.has_value());
	bool threw = false;
	try {
		(void)rows.value()[0][99];
	} catch (const OutOfBounds&) {
		threw = true;
	}

	ASSERT_TRUE(fn_name, threw);
	RETURN_TEST(fn_name, 0);
}

int bool_test() {
	const std::string fn_name = "bool_test";
	TestMemoryDatabase db;
	db.Connect();
	auto expected_rows = db.Query("SELECT COUNT(*) > 0 FROM users;");
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	ASSERT_EQUAL(fn_name, true, static_cast<bool>(expected_rows.value()[0][0].Get<int>()));
	RETURN_TEST(fn_name, 0);
}

int scalar_backend_contract() {
	TestMemoryDatabase db;
	int result = verify_scalar_backend_contract(db, "scalar_backend_contract");
	TestMemoryDatabase binary_db;
	result += verify_binary_backend_contract(binary_db, "binary_backend_contract");
	return result;
}

int verify_blobs() {
	const std::string fn_name = "verify_blobs";
	TestMemoryDatabase db;
	db.Connect();
	StormByte::BinaryData data{std::byte{0}, std::byte{1}, std::byte{2}, std::byte{0xFF}};
	auto insert_res = db.ExecuteSTMT("insert_blob", data);
	ASSERT_TRUE(fn_name, insert_res.has_value());
	auto expected_rows = db.get_blob();
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	const auto& blob = expected_rows.value()[0][0].Get<StormByte::BinaryData>();
	ASSERT_EQUAL(fn_name, 4, static_cast<int>(blob.size()));
	ASSERT_EQUAL(fn_name, 0, static_cast<int>(static_cast<unsigned char>(blob[0])));
	ASSERT_EQUAL(fn_name, 255, static_cast<int>(static_cast<unsigned char>(blob[3])));
	RETURN_TEST(fn_name, 0);
}

int empty_blob_test() {
	const std::string fn_name = "empty_blob_test";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	StormByte::BinaryData empty;
	auto insert_res = db.ExecuteSTMT("insert_blob", empty);
	ASSERT_TRUE(fn_name, insert_res.has_value());
	auto rows = db.get_blob();
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 1, rows.value().Count());
	ASSERT_FALSE(fn_name, rows.value()[0][0].IsNull());
	ASSERT_EQUAL(fn_name, 0, rows.value()[0][0].Get<StormByte::BinaryData>().size());
	RETURN_TEST(fn_name, 0);
}

int null_value_test() {
	const std::string fn_name = "null_value_test";
	TestMemoryDatabase db;
	db.Connect();
	auto rows = db.ExecuteSTMT("select_nulls");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_TRUE(fn_name, rows.value()[0][0].IsNull());
	RETURN_TEST(fn_name, 0);
}

int bind_null_test() {
	const std::string fn_name = "bind_null_test";
	TestMemoryDatabase db;
	db.Connect();
	auto res = db.ExecuteSTMT("insert_null", nullptr);
	ASSERT_TRUE(fn_name, res.has_value());
	auto rows = db.Query("SELECT value FROM nulls ORDER BY id DESC LIMIT 1;");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 1, rows.value().Count());
	ASSERT_TRUE(fn_name, rows.value()[0][0].IsNull());
	RETURN_TEST(fn_name, 0);
}

int unknown_stmt_test() {
	const std::string fn_name = "unknown_stmt_test";
	TestMemoryDatabase db;
	db.Connect();
	auto res = db.ExecuteSTMT("non_existent_stmt");
	ASSERT_FALSE(fn_name, res.has_value());
	ASSERT_TRUE(fn_name, std::string{res.error()->what()}.find("not found") != std::string::npos);
	RETURN_TEST(fn_name, 0);
}

int name_access_test() {
	const std::string fn_name = "name_access_test";
	TestMemoryDatabase db;
	db.Connect();
	auto expected_rows = db.get_users();
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	ASSERT_EQUAL(fn_name, "Alice", expected_rows.value()[0]["name"].Get<StormByte::Safe::String>());
	ASSERT_EQUAL(fn_name, "alice@example.com", expected_rows.value()[0]["email"].Get<StormByte::Safe::String>());
	RETURN_TEST(fn_name, 0);
}

int name_access_missing_column() {
	const std::string fn_name = "name_access_missing_column";
	TestMemoryDatabase db;
	db.Connect();
	auto expected_rows = db.get_users();
	ASSERT_TRUE(fn_name, expected_rows.has_value());
	bool threw = false;
	try {
		(void)expected_rows.value()[0]["non_existent_column"];
	} catch (const ColumnNotFound&) {
		threw = true;
	}

	ASSERT_TRUE(fn_name, threw);
	RETURN_TEST(fn_name, 0);
}

int transaction_commit_test() {
	const std::string fn_name = "transaction_commit_test";
	TestMemoryDatabase db;
	db.Connect();
	{
		auto tx_result = db.BeginTransaction();
		ASSERT_TRUE(fn_name, tx_result.has_value());
		auto tx = std::move(*tx_result);
		ASSERT_TRUE(fn_name, tx.IsActive());
		db.SilentQuery("INSERT INTO users (name, email) VALUES ('Charlie', 'charlie@example.com');");
		tx.Commit();
		ASSERT_FALSE(fn_name, tx.IsActive());
		tx.Commit();
	}

	auto rows = db.Query("SELECT COUNT(*) FROM users;");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 3, rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int transaction_rollback_explicit() {
	const std::string fn_name = "transaction_rollback_explicit";
	TestMemoryDatabase db;
	db.Connect();
	{
		auto tx_result = db.BeginTransaction();
		ASSERT_TRUE(fn_name, tx_result.has_value());
		auto tx = std::move(*tx_result);
		db.SilentQuery("INSERT INTO users (name, email) VALUES ('David', 'david@example.com');");
		db.SilentQuery("INSERT INTO users (name, email) VALUES ('David Two', 'david2@example.com');");
		tx.Rollback();
		ASSERT_FALSE(fn_name, tx.IsActive());
		tx.Rollback();
	}

	auto rows = db.Query("SELECT COUNT(*) FROM users WHERE name = 'David';");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 0, rows.value()[0][0].Get<int>());
	rows = db.Query("SELECT COUNT(*) FROM users WHERE email = 'david2@example.com';");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 0, rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int transaction_rollback_after_statement_error() {
	const std::string fn_name = "transaction_rollback_after_statement_error";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	auto tx_result = db.BeginTransaction();
	ASSERT_TRUE(fn_name, tx_result.has_value());
	auto tx = std::move(*tx_result);
	ASSERT_TRUE(fn_name, db.SilentQuery("INSERT INTO users (name, email) VALUES ('Transient', 'transient@example.com');"));
	ASSERT_FALSE(fn_name, db.SilentQuery("INSERT INTO users (name, email) VALUES ('Conflict', 'alice@example.com');"));
	tx.Rollback();
	auto rows = db.Query("SELECT COUNT(*) FROM users WHERE email = 'transient@example.com';");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 0, rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int transaction_rollback_auto() {
	const std::string fn_name = "transaction_rollback_auto";
	TestMemoryDatabase db;
	db.Connect();
	{
		auto tx_result = db.BeginTransaction();
		ASSERT_TRUE(fn_name, tx_result.has_value());
		auto tx = std::move(*tx_result);
		db.SilentQuery("INSERT INTO users (name, email) VALUES ('Eve', 'eve@example.com');");
	}

	auto rows = db.Query("SELECT COUNT(*) FROM users WHERE name = 'Eve';");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 0, rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int isolation_default() {
	const std::string fn_name = "isolation_default";
	TestMemoryDatabase db;
	db.Connect();
	auto tx_result = db.BeginTransaction(IsolationLevel::Default);
	ASSERT_TRUE(fn_name, tx_result.has_value());
	auto tx = std::move(*tx_result);
	tx.Commit();
	RETURN_TEST(fn_name, 0);
}

int base_exception_becomes_transaction_error() {
	const std::string fn_name = "base_exception_becomes_transaction_error";
	ThrowingBaseExceptionDatabase db;
	auto result = db.BeginTransaction();
	ASSERT_FALSE(fn_name, result.has_value());
	ASSERT_TRUE(fn_name, result.error() != nullptr);
	ASSERT_TRUE(fn_name, std::string{result.error()->what()}.find("base begin failure") != std::string::npos);
	RETURN_TEST(fn_name, 0);
}

int isolation_serializable() {
	const std::string fn_name = "isolation_serializable";
	TestMemoryDatabase db;
	db.Connect();
	auto tx_result = db.BeginTransaction(IsolationLevel::Serializable);
	ASSERT_TRUE(fn_name, tx_result.has_value());
	auto tx = std::move(*tx_result);
	tx.Commit();
	RETURN_TEST(fn_name, 0);
}

int isolation_repeatable_read() {
	const std::string fn_name = "isolation_repeatable_read";
	TestMemoryDatabase db;
	db.Connect();
	auto tx_result = db.BeginTransaction(IsolationLevel::RepeatableRead);
	ASSERT_TRUE(fn_name, tx_result.has_value());
	auto tx = std::move(*tx_result);
	tx.Commit();
	RETURN_TEST(fn_name, 0);
}

int concurrent_multiple_connections() {
	const std::string fn_name = "concurrent_multiple_connections";
	constexpr int num_threads = 6;
	constexpr int inserts_per_thread = 40;
	constexpr int max_attempts = 80;
	constexpr int retry_ms = 15;
	std::atomic<int> failures{};
	const std::filesystem::path db_path = TemporaryDatabasePath();
	std::error_code ec;
	std::filesystem::remove(db_path, ec);
	{
		TestFileDatabase setup(db_path);
		ASSERT_TRUE(fn_name, setup.Connect());
		setup.SilentQuery("DELETE FROM concurrent;");
	}

	std::vector<std::thread> threads;
	threads.reserve(static_cast<std::size_t>(num_threads));
	for (int t = 0; t < num_threads; ++t) {
		threads.emplace_back([t, db_path, retry_ms, &failures]() {
			TestFileDatabase local_db(db_path);
			if (!local_db.Connect()) {
				++failures;
				return;
			}
			for (int i = 0; i < inserts_per_thread; ++i) {
				bool inserted = false;
				for (int attempt = 0; attempt < max_attempts; ++attempt) {
					auto res = local_db.ExecuteSTMT("insert_concurrent", t * 1000 + i);
					if (res.has_value()) {
						inserted = true;
						break;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(retry_ms));
				}
				if (!inserted)
					++failures;
			}
		});
	}

	for (auto& th : threads)
		th.join();
	ASSERT_EQUAL(fn_name, 0, failures.load());
	{
		TestFileDatabase check_db(db_path);
		ASSERT_TRUE(fn_name, check_db.Connect());
		auto rows = check_db.ExecuteSTMT("count_concurrent");
		ASSERT_TRUE(fn_name, rows.has_value());
		ASSERT_EQUAL(fn_name, num_threads * inserts_per_thread, rows.value()[0][0].Get<int>());
		check_db.Disconnect();
	}

	std::filesystem::remove(db_path, ec);
	RETURN_TEST(fn_name, 0);
}

int concurrent_shared_connection() {
	const std::string fn_name = "concurrent_shared_connection";
	constexpr int num_threads = 6;
	constexpr int inserts_per_thread = 40;
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	std::atomic<int> failures{};
	std::vector<std::thread> threads;
	threads.reserve(static_cast<std::size_t>(num_threads));
	for (int t = 0; t < num_threads; ++t) {
		threads.emplace_back([t, &db, &failures]() {
			for (int i = 0; i < inserts_per_thread; ++i) {
				if (!db.ExecuteSTMT("insert_concurrent", t * 1000 + i).has_value())
					++failures;
			}
		});
	}

	for (auto& th : threads)
		th.join();
	ASSERT_EQUAL(fn_name, 0, failures.load());
	auto rows = db.ExecuteSTMT("count_concurrent");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, num_threads * inserts_per_thread, rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int transaction_serializes_shared_connection() {
	const std::string fn_name = "transaction_serializes_shared_connection";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	auto tx_result = db.BeginTransaction();
	ASSERT_TRUE(fn_name, tx_result.has_value());
	auto tx = std::move(*tx_result);
	ASSERT_TRUE(fn_name, db.SilentQuery("INSERT INTO concurrent (value) VALUES (1);"));
	std::promise<void> started;
	auto started_signal = started.get_future();
	auto worker = std::async(std::launch::async, [&db, started = std::move(started)]() mutable {
		started.set_value();
		return db.SilentQuery("INSERT INTO concurrent (value) VALUES (2);");
	});
	started_signal.wait();
	ASSERT_TRUE(fn_name, worker.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout);
	tx.Rollback();
	ASSERT_TRUE(fn_name, worker.get());
	auto rows = db.Query("SELECT value FROM concurrent;");
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_EQUAL(fn_name, 1, rows.value().Count());
	ASSERT_EQUAL(fn_name, 2, rows.value()[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int connected_database_move() {
	const std::string fn_name = "connected_database_move";
	std::unique_ptr<TestMemoryDatabase> moved;
	{
		TestMemoryDatabase source;
		ASSERT_TRUE(fn_name, source.Connect());
		moved = std::make_unique<TestMemoryDatabase>(std::move(source));
	}

	ASSERT_TRUE(fn_name, moved->IsConnected());
	ASSERT_TRUE(fn_name, moved->Query("SELECT 1;").has_value());
	TestMemoryDatabase reassigned;
	{
		TestMemoryDatabase source;
		ASSERT_TRUE(fn_name, source.Connect());
		reassigned = std::move(source);
	}

	ASSERT_TRUE(fn_name, reassigned.IsConnected());
	ASSERT_TRUE(fn_name, reassigned.Query("SELECT 1;").has_value());
	RETURN_TEST(fn_name, 0);
}

int telemetry_tracks_sqlite_operations_and_survives_database() {
	const std::string fn_name = "telemetry_tracks_sqlite_operations_and_survives_database";
	StormByte::Safe::Shared<StormByte::Database::Telemetry> retained;
	{
		TestMemoryDatabase db;
		retained = db.Telemetry();
		ASSERT_TRUE(fn_name, retained != nullptr);
		ASSERT_TRUE(fn_name, dynamic_cast<StormByte::Database::SQLite::Telemetry*>(retained.get()) != nullptr);
		ASSERT_TRUE(fn_name, db.Connect());
		ASSERT_TRUE(fn_name, db.Query("SELECT 1;").has_value());
		ASSERT_FALSE(fn_name, db.Query("SELEC 1;").has_value());
		ASSERT_TRUE(fn_name, db.ExecuteSTMT("select_users").has_value());
		ASSERT_FALSE(fn_name, db.ExecuteSTMT("missing_telemetry_statement").has_value());
		ASSERT_FALSE(fn_name, db.SilentQuery("INSERT INTO users (name, email) VALUES ('Duplicate', 'alice@example.com');"));
		auto transaction = db.BeginTransaction();
		ASSERT_TRUE(fn_name, transaction.has_value());
		transaction->Rollback();
		auto committed_transaction = db.BeginTransaction();
		ASSERT_TRUE(fn_name, committed_transaction.has_value());
		committed_transaction->Commit();
		db.Disconnect();
	}

	const auto* telemetry = dynamic_cast<const StormByte::Database::SQLite::Telemetry*>(retained.get());
	ASSERT_TRUE(fn_name, telemetry != nullptr);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::Connect).Successes);
	ASSERT_EQUAL(fn_name, std::uint64_t{2}, telemetry->Metrics(StormByte::Database::Operation::Disconnect).Successes);
	const auto query = telemetry->Metrics(StormByte::Database::Operation::Query);
	ASSERT_EQUAL(fn_name, std::uint64_t{2}, query.Attempts);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, query.Successes);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, query.Failures);
	ASSERT_TRUE(fn_name, query.MinimumNanoseconds <= query.MeanNanoseconds());
	ASSERT_TRUE(fn_name, query.MeanNanoseconds() <= query.MaximumNanoseconds);
	ASSERT_EQUAL(fn_name, std::uint64_t{3}, telemetry->RowsReturned());
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::PreparedStatement).Failures);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::PreparedStatement).Successes);
	const auto prepare_metrics = telemetry->Metrics(StormByte::Database::Operation::PrepareStatement);
	ASSERT_TRUE(fn_name, prepare_metrics.Attempts > 0);
	ASSERT_EQUAL(fn_name, prepare_metrics.Attempts, prepare_metrics.Successes);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::SilentQuery).Failures);
	ASSERT_EQUAL(fn_name, std::uint64_t{2}, telemetry->Metrics(StormByte::Database::Operation::BeginTransaction).Successes);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::CommitTransaction).Successes);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->Metrics(StormByte::Database::Operation::RollbackTransaction).Successes);
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, telemetry->ConstraintErrors());
	ASSERT_TRUE(fn_name, static_cast<std::string>(*retained).find("SQLite{") != std::string::npos);
	RETURN_TEST(fn_name, 0);
}

int prepared_registry_preserves_unique_statements() {
	constexpr std::string_view fn_name = "prepared_registry_preserves_unique_statements";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	const std::string name(512, 's');
	db.RegisterStatement(name, "SELECT 17;");
	db.RegisterStatement(name, "SELECT 29;");
	const auto first = db.ExecuteSTMT(name);
	ASSERT_TRUE(fn_name, first.has_value());
	ASSERT_EQUAL(fn_name, 17, (*first)[0][0].Get<int>());
	ASSERT_EQUAL(fn_name, std::uint64_t{1}, db.Telemetry()->Metrics(StormByte::Database::Operation::PrepareStatement).Failures);
	db.Disconnect();
	ASSERT_TRUE(fn_name, db.Connect());
	ASSERT_TRUE(fn_name, !db.ExecuteSTMT(name));
	db.RegisterStatement(name, "SELECT 31;");
	const auto replaced = db.ExecuteSTMT(name);
	ASSERT_TRUE(fn_name, replaced.has_value());
	ASSERT_EQUAL(fn_name, 31, (*replaced)[0][0].Get<int>());
	RETURN_TEST(fn_name, 0);
}

int embedded_nul_contract() {
	constexpr std::string_view fn_name = "embedded_nul_contract";
	TestMemoryDatabase db;
	ASSERT_TRUE(fn_name, db.Connect());
	constexpr std::string_view text{"left\0right", 10};
	constexpr std::string_view name{"echo\0suffix", 11};
	db.RegisterStatement("echo", "SELECT 17;");
	db.RegisterStatement(name, "SELECT ?;");
	const auto rows = db.ExecuteSTMT(name, StormByte::Safe::String{text});
	ASSERT_TRUE(fn_name, rows.has_value());
	ASSERT_TRUE(fn_name, static_cast<std::string_view>((*rows)[0][0].Get<StormByte::Safe::String>()) == text);
	const auto prefix = db.ExecuteSTMT("echo");
	ASSERT_TRUE(fn_name, prefix.has_value());
	ASSERT_EQUAL(fn_name, 17, (*prefix)[0][0].Get<int>());
	const auto empty = db.ExecuteSTMT(name, std::string_view{});
	ASSERT_TRUE(fn_name, empty.has_value());
	ASSERT_FALSE(fn_name, (*empty)[0][0].IsNull());
	ASSERT_TRUE(fn_name, (*empty)[0][0].Get<StormByte::Safe::String>().empty());
	constexpr std::string_view invalid_sql{"SELECT 1;\0SELECT 2;", 19};
	ASSERT_FALSE(fn_name, db.Query(invalid_sql).has_value());
	ASSERT_FALSE(fn_name, db.SilentQuery(invalid_sql));
	db.RegisterStatement("invalid_nul", invalid_sql);
	ASSERT_FALSE(fn_name, db.ExecuteSTMT("invalid_nul").has_value());
	const std::string invalid_path{":memory:\0suffix", 15};
	TestFileDatabase file{std::filesystem::path{invalid_path}};
	ASSERT_FALSE(fn_name, file.Connect());
	ASSERT_TRUE(fn_name, db.Query("SELECT 1;").has_value());
	RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;
	result += embedded_nul_contract();
	result += not_connected_query();
	result += not_connected_silent();
	result += not_connected_execute();
	result += not_connected_transaction();
	result += base_exception_becomes_transaction_error();
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
	result += prepared_registry_preserves_unique_statements();
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
	result += concurrent_shared_connection();
	result += transaction_serializes_shared_connection();
	result += connected_database_move();
	result += telemetry_tracks_sqlite_operations_and_survives_database();
	if (result == 0) {
		std::cout << "All tests passed successfully.\n";
	} else {
		std::cout << result << " tests failed.\n";
	}

	return result;
}
