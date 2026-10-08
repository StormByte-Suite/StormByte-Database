# StormByte-Database

![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-lightgrey)
![C++26](https://img.shields.io/badge/C%2B%2B-26-00599C?logo=c%2B%2B&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.28+-064F8C?logo=cmake&logoColor=white)
![License: LGPL v3 or commercial](https://img.shields.io/badge/License-LGPL_v3_or_commercial-blue.svg)
[![CI](https://github.com/StormByte-Suite/StormByte-Database/actions/workflows/ci.yml/badge.svg)](https://github.com/StormByte-Suite/StormByte-Database/actions/workflows/ci.yml)
[![Sponsor](https://img.shields.io/badge/Sponsor-StormBytePP-ea4aaa?logo=githubsponsors)](https://github.com/sponsors/StormBytePP)

This repository is **StormByte Database**: the C++26 SQL layer of the StormByte suite.

It depends on StormByte-Logger 2.0.0 or newer, which supplies the bundled text and StormByte Base dependencies. Public headers live under `StormByte/database/`.

One API covers SQLite, PostgreSQL, MariaDB and Microsoft SQL Server (MSSQL). You do **not** construct those backends as generic objects. They are **base classes**: derive your schema, call the backend constructor, prepare statements and hook connect/disconnect there.

The suite is split on purpose. Base, Buffer, Config, Crypto, Logger, Multimedia, Network and System are **other repositories**. This repository does not implement them.

## What this module does

- **One connection type** — `StormByte::Database::Database` with Connect / Disconnect, Query / SilentQuery, named prepared statements and RAII transactions.
- **Inheritance first** — SQLite3, MariaDB, Postgres and MSSQL constructors are protected. Your application database is a subclass.
- **Values** — type-erased `Value` (NULL, integers, double, text, `Safe::Binary` blob, bool) with safe numeric `Get<T>()`.
- **Rows** — ordered columns, lookup by name (`ColumnNotFound` / `OutOfBounds`).
- **Prepared statements** — bind by position (0-based), `nullptr` is SQL NULL, `ExpectedRows` on execute.
- **Transactions** — `BeginTransaction(IsolationLevel)` returns `Expected<Transaction, TransactionError>`; failed starts are reported as a value, and an uncommitted transaction rolls back on destruction.
- **Telemetry** — `Telemetry()` returns a thread-safe, cumulative `StormByte::Safe::Shared` handle with operation counts, outcomes, rows and latency min/mean/max. Database telemetry extends Base telemetry and uses its named clocks; SQLite, PostgreSQL, MariaDB and MSSQL provide derived telemetry with backend-specific error counters. Retained handles remain readable after disconnect/destruction.
- **TLS** — `SslMode` for MariaDB and PostgreSQL. SQLite ignores it.
- **Concurrent access** — operations on one connection are serialized; separate connections can run concurrently. A transaction reserves its connection until commit or rollback and must remain on the thread that created it. Custom backend implementations use the protected `OperationGuard` in public operations that access connection state.

## The rest of the suite

| Module | Role | API |
| --- | --- | --- |
| [Base](https://github.com/StormByte-Suite/StormByte) | Exceptions, Expected, serialization, strings, UUID, concepts | [/StormByte](http://suite.stormbyte.org/StormByte) |
| [Buffer](https://github.com/StormByte-Suite/StormByte-Buffer) | FIFO, SharedFIFO, Ring, Producer/Consumer and multi-stage pipelines | [/StormByte-Buffer](http://suite.stormbyte.org/StormByte-Buffer) |
| [Config](https://github.com/StormByte-Suite/StormByte-Config) | Human-readable text and versioned binary documents (groups, lists, raw bytes) | [/StormByte-Config](http://suite.stormbyte.org/StormByte-Config) |
| [Crypto](https://github.com/StormByte-Suite/StormByte-Crypto) | Hash, compress, encrypt, sign and key agreement — Crypto++ never leaves the private tree | [/StormByte-Crypto](http://suite.stormbyte.org/StormByte-Crypto) |
| **Database** | This repository | [/StormByte-Database](http://suite.stormbyte.org/StormByte-Database) |
| [Logger](https://github.com/StormByte-Suite/StormByte-Logger) | Stream logger with levels, headers, human-readable sizes and redaction (`ThreadedLog`) | [/StormByte-Logger](http://suite.stormbyte.org/StormByte-Logger) |
| [Multimedia](https://github.com/StormByte-Suite/StormByte-Multimedia) | Decode, encode and containers without raw FFmpeg types; codecs enabled only if present | [/StormByte-Multimedia](http://suite.stormbyte.org/StormByte-Multimedia) |
| [Network](https://github.com/StormByte-Suite/StormByte-Network) | Framed packets, Client/Server, IPv4/IPv6 TCP and Buffer pipelines (compress/encrypt) | [/StormByte-Network](http://suite.stormbyte.org/StormByte-Network) |
| [System](https://github.com/StormByte-Suite/StormByte-System) | Processes, pipes and environment variables across Linux, Windows and macOS | [/StormByte-System](http://suite.stormbyte.org/StormByte-System) |

## Table of Contents

- [What this module does](#what-this-module-does)
- [The rest of the suite](#the-rest-of-the-suite)
- [Installation](#installation)
- [Backends](#backends)
  - [SQLite](#sqlite)
  - [PostgreSQL](#postgresql)
  - [MariaDB](#mariadb)
  - [Microsoft SQL Server (MSSQL)](#microsoft-sql-server-mssql)
- [Documentation](#documentation)
- [Usage](#usage)
  - [SQLite quickstart](#sqlite-quickstart)
  - [Queries and statements](#queries-and-statements)
  - [Values and rows](#values-and-rows)
  - [Transactions](#transactions)
- [Telemetry](#telemetry)
- [DLL Boundary Contract](#dll-boundary-contract)
- [Support](#support)
- [Contributing](#contributing)
- [License](#license)

## Installation

Needs a C++26 compiler, CMake 3.28 or newer, and StormByte-Logger 2.0.0 or newer. Logger supplies the bundled text and StormByte Base dependencies used by Database. The SQLite, PostgreSQL, MariaDB and MSSQL backends are independently optional. The default for each is `BUNDLED`; set each `WITH_SQLITE`, `WITH_POSTGRES`, `WITH_MARIADB` or `WITH_MSSQL` option to `OFF`, `SYSTEM` or `BUNDLED` to choose explicitly. `SYSTEM` discovers an installed client library and `BUNDLED` builds the pinned client dependency. Only enabled backends are compiled and their optional headers installed. The bundled MSSQL backend uses FreeTDS DB-Library under its LGPL license; FreeTDS utilities and ODBC/CT-Library targets are excluded.

`WITH_OPENSSL` accepts `SYSTEM` or `BUNDLED` (default); `OFF` is not supported. Windows always forces `BUNDLED`, regardless of the requested value. It selects TLS dependencies for bundled PostgreSQL, MariaDB and FreeTDS clients. `BUNDLED` builds their common pinned OpenSSL 3.5.9 dependency as static libraries and requires Perl and a make implementation, plus NASM on Windows. `SYSTEM` uses installed dynamic OpenSSL libraries and development files without requesting static archives; PostgreSQL uses Meson's normal OpenSSL discovery. Already-installed system database clients keep their own TLS dependencies. Changing dependency modes requires a clean build directory.

```sh
git clone --recurse-submodules https://github.com/StormByte-Suite/StormByte-Database.git
cd StormByte-Database
cmake -S . -B build
cmake --build build
```

For a SQLite-only build, make the selection explicit:

```sh
cmake -S . -B build-sqlite \
	-DWITH_SQLITE=BUNDLED \
	-DWITH_POSTGRES=OFF \
	-DWITH_MARIADB=OFF \
	-DWITH_MSSQL=OFF
cmake --build build-sqlite
```

Shared vs static follows CMake `BUILD_SHARED_LIBS` (default ON). `-DBUILD_SHARED_LIBS=OFF` builds a static archive. In static mode BuildMaster flattens private vendor dependencies into the consumer link closure; users do not need to repack vendor archives. The shared library keeps Database replaceable as its own DLL/shared object.

## Backends

The core `StormByte::Database::Database` API is built independently of the client backends. Enable only the client libraries this application needs. For every backend, `OFF` omits its implementation and optional headers, `SYSTEM` uses the installed client development files, and `BUNDLED` builds the repository's pinned client dependency.

### SQLite

Enable with `WITH_SQLITE=SYSTEM` or `WITH_SQLITE=BUNDLED` (`BUNDLED` is the default). SQLite is an embedded database, not a network server; a database is a file or `:memory:` connection. Its protected constructors take a native filesystem path or use the in-memory database; foreign-key enforcement is opt-in through `EnableForeignKeys()`.

### PostgreSQL

Enable with `WITH_POSTGRES=SYSTEM` or `WITH_POSTGRES=BUNDLED` (`BUNDLED` is the default). Applications derive from `StormByte::Database::Postgres::Postgres` and provide host, user, password and database settings. TLS policy is selected with `SslMode` before connecting. Prepared statement parameters are positional and sent separately from SQL.

### MariaDB

Enable with `WITH_MARIADB=SYSTEM` or `WITH_MARIADB=BUNDLED` (`BUNDLED` is the default). Applications derive from `StormByte::Database::MariaDB::MariaDB` and provide host, user, password, database and port. TLS policy is selected with `SslMode` before connecting. Prepared statement parameters are positional.

### Microsoft SQL Server (MSSQL)

Enable with `WITH_MSSQL=SYSTEM` or `WITH_MSSQL=BUNDLED` (`BUNDLED` is the default). The bundled client is FreeTDS DB-Library, not ODBC or CT-Library. Applications derive from `StormByte::Database::MSSQL::MSSQL`; logical prepared statements use `sp_executesql` RPC with typed positional parameters. Integration tests read `MSSQL_HOST`, `MSSQL_USER`, `MSSQL_PASSWORD`, `MSSQL_DATABASE` and `MSSQL_PORT`; they return CTest's configured skip code when `MSSQL_HOST` is unset.

## Documentation

- This README: build modes, backend selection, ownership and examples.
- Doxygen class reference: [http://suite.stormbyte.org/StormByte-Database/](http://suite.stormbyte.org/StormByte-Database/).

## Usage

Headers are `#include <StormByte/database/….hxx>`. Namespace root is `StormByte::Database`.
Moving a connected backend transfers ownership of its connection; the moved-from backend is disconnected.

### SQLite quickstart

```cpp
#include <StormByte/database/exception.hxx>
#include <StormByte/database/row.hxx>
#include <StormByte/database/rows.hxx>
#include <StormByte/database/sqlite/sqlite3.hxx>
#include <StormByte/database/transaction.hxx>
#include <StormByte/database/value.hxx>
#include <StormByte/logger/log.hxx>

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

class AppDatabase final : public StormByte::Database::SQLite::SQLite3 {
	public:
		AppDatabase(): SQLite3(std::filesystem::path{":memory:"}, StormByte::Safe::Shared<StormByte::Logger::Log>{}) {}

	private:
		void DoPostConnect() noexcept override {
			if (!DoSilentQuery("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);"))
				return;
			PrepareSTMT("insert_user", "INSERT INTO users (id, name) VALUES (?, ?);");
			PrepareSTMT("user_by_id", "SELECT id, name FROM users WHERE id = ?;");
		}
};

STORMBYTE_DECLARE_MAYBE_SAFE(AppDatabase);

int main() {
	AppDatabase database;
	if (!database.Connect())
		return 1;

	if (!database.SilentQuery("INSERT INTO users (id, name) VALUES (0, 'Lin');"))
		return 2;
	auto direct_query = database.Query("SELECT id, name FROM users WHERE id = 0;");
	if (!direct_query || direct_query.value().size() != 1)
		return 3;
	std::cout << "direct query: " << static_cast<std::string_view>(direct_query.value()[0]["name"].Get<StormByte::Safe::String>()) << '\n';

	auto inserted = database.ExecuteSTMT("insert_user", 1, std::string_view{"Ada"});
	if (!inserted) {
		std::cerr << inserted.error()->what() << '\n';
		return 4;
	}
	auto selected = database.ExecuteSTMT("user_by_id", 1);
	if (!selected || selected.value().size() != 1)
		return 5;
	const StormByte::Database::Rows& rows = selected.value();
	const int id = rows[0]["id"].Get<int>();
	const StormByte::Safe::String name = rows[0]["name"].Get<StormByte::Safe::String>();
	std::cout << "prepared result: " << id << ": " << static_cast<std::string_view>(name) << '\n';

	const StormByte::Database::Value null_value;
	const StormByte::Database::Value blob_value{StormByte::Safe::Binary{std::byte{0}, std::byte{0xFF}}};
	StormByte::Database::Row local_row;
	local_row.add("state", StormByte::Database::Value{std::string_view{"active"}});
	if (!null_value.IsNull() || blob_value.Type() != StormByte::Database::Value::Type::Blob || local_row.size() != 1)
		return 6;

	const auto failed_query = database.Query("SELECT * FROM missing_table;");
	if (failed_query || failed_query.error() == nullptr)
		return 7;
	std::cerr << "query error: " << failed_query.error()->what() << '\n';

	auto transaction_result = database.BeginTransaction(StormByte::Database::IsolationLevel::Serializable);
	if (!transaction_result) {
		std::cerr << transaction_result.error()->what() << '\n';
		return 8;
	}
	auto transaction = std::move(transaction_result.value());
	auto second_insert = database.ExecuteSTMT("insert_user", 2, std::string_view{"Grace"});
	if (!second_insert)
		return 9;
	transaction.Commit();

	{
		auto rollback_result = database.BeginTransaction();
		if (!rollback_result)
			return 10;
		auto rollback = std::move(rollback_result.value());
		if (!database.SilentQuery("INSERT INTO users (id, name) VALUES (3, 'Rolled back');"))
			return 11;
	}
	auto rollback_check = database.Query("SELECT COUNT(*) AS total FROM users WHERE id = 3;");
	if (!rollback_check || rollback_check.value()[0]["total"].Get<int>() != 0)
		return 12;

	const auto telemetry = database.Telemetry();
	const auto metrics = telemetry->Metrics(StormByte::Database::Operation::PreparedStatement);
	const std::string snapshot = static_cast<std::string>(*telemetry);
	std::cout << "prepared attempts=" << metrics.Attempts << '\n' << snapshot << '\n';
	return 0;
}
```

This is a complete in-memory program: save it as `sqlite_quickstart.cxx` in a CMake consumer linked to the Database and Logger targets, build as C++26, then run it. The derived class creates its schema and registers named statements in `DoPostConnect()`, which runs after each successful connection.

### Queries and statements

There are three execution paths:

- `Query(sql)` executes SQL and returns `ExpectedRows`. On success, read its ordered `Rows`; a successful command with no result set has an empty collection. On failure, the `Expected` contains a `QueryException`, commonly `ExecuteError`. Use it for `SELECT` and whenever result rows matter. SQL is a borrowed `std::string_view`, consumed during the call and not retained.
- `SilentQuery(sql)` executes a command when only success or failure matters, such as schema setup or a write with no returned rows. It returns `bool` and is `noexcept`; it returns neither rows nor a diagnostic object. Prefer `Query` when the caller needs results or an error value.
- `PrepareSTMT(name, sql)` registers a named positional statement on the connection. Register statements from `DoPostConnect()` so they are recreated after reconnecting. Execute them with `ExecuteSTMT(name, values...)`; values bind left to right starting at index zero, and `nullptr` binds SQL `NULL`. A missing name or execution/binding failure is returned through `ExpectedRows`. Bindings are reset around each execution.

Prepared statements keep SQL separate from values; do not concatenate user input into SQL. SQLite, MariaDB and MSSQL use `?` placeholders. PostgreSQL uses `$1`, `$2`, and so on. SQLite, PostgreSQL and MariaDB use their native prepared-statement APIs; MSSQL uses `sp_executesql` RPC with typed positional parameters. The shared API does not make SQL syntax portable, so write SQL for the selected backend. PostgreSQL text cannot contain embedded NUL; use `Safe::Binary` for arbitrary bytes. SQLite rejects embedded NUL in SQL and paths.

The following MSSQL program is complete. Save it in a CMake consumer linked to Database and Logger. Set `MSSQL_PASSWORD` and, when needed, `MSSQL_HOST`, `MSSQL_USER` and `MSSQL_DATABASE` in the environment before running it; it does not embed credentials in source.

```cpp
#include <StormByte/database/mssql/mssql.hxx>
#include <StormByte/logger/log.hxx>

#include <cstdlib>
#include <string_view>

namespace {
	std::string_view EnvironmentValue(const char* name, const char* fallback) {
		const char* value = std::getenv(name);
		return value ? std::string_view{value} : std::string_view{fallback};
	}
}

class AppMssqlDatabase final : public StormByte::Database::MSSQL::MSSQL {
	public:
		AppMssqlDatabase(std::string_view host, std::string_view user, std::string_view password, std::string_view database)
			: MSSQL(host, user, password, database, 1433, StormByte::Safe::Shared<StormByte::Logger::Log>{}) {}

	private:
		void DoPostConnect() noexcept override {
			PrepareSTMT("integer_echo", "SELECT CAST(? AS int) AS value;");
		}
};

STORMBYTE_DECLARE_MAYBE_SAFE(AppMssqlDatabase);

int main() {
	const std::string_view password = EnvironmentValue("MSSQL_PASSWORD", "");
	if (password.empty())
		return 1;
	AppMssqlDatabase database{
		EnvironmentValue("MSSQL_HOST", "sql.example.test"),
		EnvironmentValue("MSSQL_USER", "app_login"),
		password,
		EnvironmentValue("MSSQL_DATABASE", "app_database")
	};
	if (!database.Connect())
		return 2;
	auto result = database.ExecuteSTMT("integer_echo", 42);
	if (!result || result.value()[0]["value"].Get<int>() != 42)
		return 3;
	return 0;
}
```

### Values and rows

`Rows` preserves database column order. Access a column by index (`row[0]`) or by its result name (`row["name"]`); missing names and invalid indices throw `ColumnNotFound` and `OutOfBounds`. `Value` stores SQL `NULL`, integer and floating-point values, UTF-8 text in `Safe::String`, or arbitrary bytes in `Safe::Binary`. Retrieve a value with `Get<T>()`; numeric conversions are checked and throw `WrongValueType` if they would overflow or lose information. Empty text, an empty blob, and SQL `NULL` are distinct values.

Backend failures are returned as `ExpectedRows` errors rather than requiring exceptions for normal query control flow. Check the `Expected` before calling `value()` or indexing rows, and inspect `error()` for the `QueryException`. Checked `Value::Get<T>()` conversions and transaction commit failures use Database exceptions.

### Transactions

`BeginTransaction(level)` returns `Expected<Transaction, TransactionError>`: check it before taking the transaction. `Commit()` makes the work permanent; `Rollback()` cancels it. If neither is called, the transaction rolls back when its object is destroyed. It reserves the connection for its creating thread until it finishes, so do not move it to a worker thread or leave another thread waiting while the transaction owner waits on that worker. Isolation levels are mapped by each backend; `Default` asks the backend to use its normal transaction behavior.

### Telemetry

The SQLite quickstart prints the prepared-statement attempt count and a formatted telemetry snapshot. `Telemetry()` returns a retained `Safe::Shared` handle; backend-specific subclasses expose counters such as SQLite busy/constraint errors or MSSQL DB-Library errors. Metrics count attempts, outcomes, returned rows and durations; telemetry does not retain SQL text or bound values. Getters are thread-safe. A retained handle remains valid after disconnect and database destruction while its provider modules remain loaded.

### DLL Boundary Contract

Exported Database types declare `STORMBYTE_DECLARE_MAYBE_SAFE`: allocation ownership is preserved, but C++ consumers must use a compatible compiler, standard-library ABI and build configuration. Base, Logger, Database and any consumer module providing callbacks or derived objects must remain loaded while those objects exist.

Database facades keep their protected constructors and virtual hooks for application inheritance. A consumer derivative is not automatically classified as `MaybeSafe`; it must preserve the lifetime and ownership contract for its added state and declare the macro at global scope after its complete definition when used with Safe components. Copyable `Value`, `Row` and `Rows` can be used in Base Safe collections. Non-copyable facades should be retained through Safe owners.

Connection settings, PostgreSQL statement names and MSSQL callback diagnostics use Base-owned `Safe::String`, including private members of inheritable backends. `Value` stores its alternatives in `Safe::Variant`; `Row` and `Rows` use Base-owned `Safe::Vector` and `Safe::Map`, and prepared-statement values use `Safe::Vector`. Telemetry counters use `Safe::Atomic`. Backend-native connection and statement handles are confined to non-installed private holder definitions; public and optional headers do not include client headers or expose driver types. Temporary STL buffers required by client APIs are created and destroyed inside Database implementations. These ownership rules do not certify a general C++ ABI: derived-object layout, RTTI, exceptions and calling conventions must still agree. Consumer overrides must release their own resources in their provider module, and borrowed views and iterators remain subject to their owner's lifetime and invalidation rules.

Accessors use paired names: `Telemetry()` retrieves the handle, the protected `Telemetry(handle)` overload replaces it, and `SslMode()` / `SslMode(mode)` observe and configure TLS policy. `Value::Get<T>()` retains its name.

SQLite native-path constructors convert to owned UTF-8 inside the caller before entering Database, including for temporary paths. Telemetry uses independent Base clock samples, so measurements of the same operation may overlap or nest without external clock locks.

Length-bearing text values and column names preserve embedded NULs through copies and Safe collections. SQLite text bindings preserve them as well, but SQLite SQL and file paths reject embedded NULs rather than execute or open only the prefix. Network backends reject embedded NULs in connection settings before calling their client library. PostgreSQL rejects text bind parameters containing NUL because PostgreSQL text cannot represent them; use `StormByte::Safe::Binary` for arbitrary bytes. SQLite rejects text bindings exceeding its supported length instead of narrowing the length to `int`.

## Contributing

## Support

Questions and bugs: GitHub issues on this repository. Sponsorship: [github.com/sponsors/StormBytePP](https://github.com/sponsors/StormBytePP).

Issues only on this repository. Fork and open a pull request against `master`.

## License

Dual license: GNU Lesser General Public License v3.0 or later, or a commercial license from the copyright holder. See [LICENSE](LICENSE), [COPYING.LGPLv3](COPYING.LGPLv3) and <https://www.gnu.org/licenses/lgpl-3.0.html>. Third-party trees under `thirdparty/` keep their own licenses.
