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
- **Values** — type-erased `Value` (NULL, integers, double, text, blob, bool) with safe numeric `Get<T>()`.
- **Rows** — ordered columns, lookup by name (`ColumnNotFound` / `OutOfBounds`).
- **Prepared statements** — bind by position (0-based), `nullptr` is SQL NULL, `ExpectedRows` on execute.
- **Transactions** — `BeginTransaction(IsolationLevel)` returns `Expected<Transaction, TransactionError>`; failed starts are reported as a value, and an uncommitted transaction rolls back on destruction.
- **Telemetry** — `GetTelemetry()` returns a thread-safe, cumulative `StormByte::Safe::Shared` handle with operation counts, outcomes, rows and latency min/mean/max. Database telemetry extends Base telemetry and uses its named clocks; SQLite, PostgreSQL, MariaDB and MSSQL provide derived telemetry with backend-specific error counters. Retained handles remain readable after disconnect/destruction.
- **TLS** — `SslMode` for MariaDB and PostgreSQL. SQLite ignores it.
- **Concurrent access** — operations on one connection are serialized; separate connections can run concurrently. A transaction reserves its connection until commit or rollback and must remain on the thread that created it. Custom backend implementations must lock the shared connection mutex in public operations.

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
- [Documentation](#documentation)
- [Installation](#installation)
- [Usage](#usage)
  - [Derive your database](#derive-your-database)
  - [Values and rows](#values-and-rows)
  - [Queries and statements](#queries-and-statements)
  - [Transactions](#transactions)
- [Telemetry](#telemetry)
- [Support](#support)
- [Contributing](#contributing)
- [License](#license)

## Installation

Needs a C++26 compiler, CMake 3.28 or newer, and StormByte-Logger 2.0.0 or newer. Logger supplies the bundled text and StormByte Base dependencies used by Database. Enable the backends you want (`WITH_SQLITE`, `WITH_POSTGRES`, `WITH_MARIADB`, `WITH_MSSQL`: `OFF`, `SYSTEM` or `BUNDLED`); `SYSTEM` discovers installed client libraries and `BUNDLED` builds them. Bundled PostgreSQL, MariaDB and MSSQL builds share the pinned OpenSSL 3.5.9 source in this repository and do not use host OpenSSL libraries; these builds require Perl and a make implementation, plus NASM on Windows. The bundled MSSQL backend uses FreeTDS DB-Library under its LGPL license; FreeTDS utilities and ODBC/CT-Library targets are excluded.

```sh
git clone --recurse-submodules https://github.com/StormByte-Suite/StormByte-Database.git
cd StormByte-Database
cmake -S . -B build
cmake --build build
```

Shared vs static follows CMake `BUILD_SHARED_LIBS` (default ON). `-DBUILD_SHARED_LIBS=OFF` builds a static archive. In static mode BuildMaster flattens private vendor dependencies into the consumer link closure; users do not need to repack vendor archives. The shared library keeps Database replaceable as its own DLL/shared object.

## Documentation

- This README: build modes, backend selection, ownership and examples.
- Doxygen class reference: [http://suite.stormbyte.org/StormByte-Database/](http://suite.stormbyte.org/StormByte-Database/).

## Usage

Headers are `#include <StormByte/database/….hxx>`. Namespace root is `StormByte::Database`.
Moving a connected backend transfers ownership of its connection; the moved-from backend is disconnected.

### Derive your database

```cpp
#include <StormByte/database/sqlite/sqlite3.hxx>
#include <StormByte/logger/log.hxx>
#include <utility>

class AppDb : public StormByte::Database::SQLite::SQLite3 {
public:
	AppDb(StormByte::Safe::Shared<StormByte::Logger::Log> log)
		: SQLite3(std::filesystem::path{"app.db"}, log) {}

protected:
	void DoPostConnect() noexcept override {
		EnableForeignKeys();
		PrepareSTMT("user_by_id", "SELECT id, name FROM users WHERE id = ?");
	}
};

int main() {
	AppDb db(nullptr);
	if (!db.Connect())
		return 1;

	auto rows = db.Query("SELECT 1 AS n");
	if (!rows)
		return 1;
}
```

MariaDB / Postgres follow the same pattern: subclass, pass host / user / password / database (and port on MariaDB), optionally `SetSslMode` before `Connect()`. PostgreSQL connection parameters are passed separately, so credentials may contain quotes and backslashes.

MSSQL uses FreeTDS DB-Library. Its logical prepared statements execute through `sp_executesql` RPC with typed parameters; values are not interpolated into SQL text.

```cpp
#include <StormByte/database/mssql/mssql.hxx>

class AppDb : public StormByte::Database::MSSQL::MSSQL {
public:
	AppDb()
		: MSSQL("sql.example.test", "app_login", "secret", "app_database", 1433, nullptr) {}

protected:
	void DoPostConnect() noexcept override {
		PrepareSTMT("user_by_id", "SELECT id, name FROM dbo.users WHERE id = ?");
	}
};
```

### Values and rows

```cpp
#include <StormByte/database/value.hxx>

using namespace StormByte::Database;

Value n(42);
Value empty;          // SQL NULL
auto i = n.Get<int>();
if (auto row = /* from Query */) {
	const Value& name = (*row)[0]["name"];
}
```

Malformed or out-of-range numeric values returned by a backend are reported through `ExpectedRows` as query errors.

### Queries and statements

```cpp
auto result = db.ExecuteSTMT("user_by_id", 7);
if (!result)
	return 1;

for (const auto& row : *result) {
	auto id = row["id"].Get<int>();
}
```

`nullptr` binds SQL NULL. Missing statement names raise `UnknownSTMT` through `ExpectedRows`.

### Transactions

```cpp
#include <utility>

{
	auto tx_result = db.BeginTransaction(IsolationLevel::Serializable);
	if (!tx_result)
		return 1;
	auto tx = std::move(*tx_result);
	db.SilentQuery("INSERT INTO users(name) VALUES ('ada')");
	tx.Commit();
} // Rollback if Commit was not called
```

### Telemetry

```cpp
#include <StormByte/database/sqlite/sqlite3.hxx>

#include <iostream>
#include <string>

auto telemetry = db.GetTelemetry();
const auto query_metrics = telemetry->Metrics(StormByte::Database::Operation::Query);
std::cout << "queries=" << query_metrics.Attempts
		  << " failures=" << query_metrics.Failures
		  << " mean_ns=" << query_metrics.MeanNanoseconds() << '\n';

if (const auto* sqlite = dynamic_cast<const StormByte::Database::SQLite::Telemetry*>(telemetry.get()))
	std::cout << "sqlite_busy=" << sqlite->BusyErrors()
			  << " constraints=" << sqlite->ConstraintErrors() << '\n';

std::string snapshot = static_cast<std::string>(*telemetry);
```

Telemetry records operation attempts, successes/failures, total/minimum/mean/maximum latency, rows returned, and categorized backend events. It does not retain SQL text or bind values. Its getters are safe to call while operations run; snapshots may reflect updates that complete during the read. The `Shared` handle owns the same cumulative telemetry object and remains valid after the database is disconnected or destroyed.

### DLL Boundary Contract

Exported Database types declare `STORMBYTE_DECLARE_MAYBE_SAFE`: allocation ownership is preserved, but C++ consumers must use a compatible compiler, standard-library ABI and build configuration. Base, Logger, Database and any consumer module providing callbacks or derived objects must remain loaded while those objects exist.

Database facades keep their protected constructors and virtual hooks for application inheritance. A consumer derivative is not automatically classified as `MaybeSafe`; it must preserve the lifetime and ownership contract for its added state and declare the macro at global scope after its complete definition when used with Safe components. Copyable `Value`, `Row` and `Rows` can be used in Base Safe collections. Non-copyable facades should be retained through Safe owners.

SQLite native-path constructors convert to owned UTF-8 inside the caller before entering Database, including for temporary paths. Telemetry uses independent Base clock samples, so measurements of the same operation may overlap or nest without external clock locks.

## Contributing

## Support

Questions and bugs: GitHub issues on this repository. Sponsorship: [github.com/sponsors/StormBytePP](https://github.com/sponsors/StormBytePP).

Issues only on this repository. Fork and open a pull request against `master`.

## License

Dual license: GNU Lesser General Public License v3.0 or later, or a commercial license from the copyright holder. See [LICENSE](LICENSE), [COPYING.LGPLv3](COPYING.LGPLv3) and <https://www.gnu.org/licenses/lgpl-3.0.html>. Third-party trees under `thirdparty/` keep their own licenses.
