# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Summary]

StormByte Database is the C++26 SQL layer of the StormByte suite.

One API covers SQLite, PostgreSQL and MariaDB.
Backends are base classes: you derive your schema, prepare statements and hook connect there.
This repository is not Base, Buffer, Config, Crypto, Logger, Multimedia, Network or System.
It requires StormByte-Logger 2.0.0 or newer, which supplies the bundled text and StormByte Base dependencies.
The original StormByte-Database source is dual-licensed under LGPL v3.0-or-later or a commercial license.

If you landed here from a release link and have not read the tree:

- What this module is, how to build it, and short examples: [README.md](https://github.com/StormByte-Suite/StormByte-Database/blob/master/README.md)
- License: dual LGPL v3.0-or-later / commercial, [LICENSE](https://github.com/StormByte-Suite/StormByte-Database/blob/master/LICENSE) and [COPYING.LGPLv3](https://github.com/StormByte-Suite/StormByte-Database/blob/master/COPYING.LGPLv3)

## [Unreleased]
[Unreleased]: https://github.com/StormByte-Suite/StormByte-Database/compare/2.0.0...HEAD

## [2.0.0] - 2026-10-05

### Changed

- **DLL boundary** — Hardened provider-owned storage while preserving inheritable database facades:
	- Hid the prepared-statement registry behind an opaque `Safe::Unique` owner on Base's heap and migrated its entries to `Safe::Map<Safe::String, Safe::Shared<Safe::Unique<PreparedSTMT>>>`. Creator-module callbacks retain map nodes and allocator state; copyable handles keep each statement uniquely owned without changing inheritable factories returning `Safe::Unique`.
	- Column names and prepared-statement names and SQL text use `Safe::String`, so inline accessors no longer interpret another module's standard strings.
	- SQLite's protected native-path constructors convert to Base-owned UTF-8 in forced-inline caller adapters; rvalue paths no longer transfer caller STL allocations to the Database DLL, and Windows paths use SQLite's UTF-8 encoding.
	- Exported values, rows, transactions, database facades, prepared statements and telemetry declare `STORMBYTE_DECLARE_MAYBE_SAFE` with compatible-ABI and provider-lifetime requirements. Consumer derivatives remain responsible for their additional state and their own conditional declaration.
- **MSSQL backend** — Added an optional Microsoft SQL Server backend using the LGPL FreeTDS DB-Library client. Bundled builds compile only the static DB-Library and its required TDS support archives; logical prepared statements use `sp_executesql` RPC with typed, separately transmitted parameters.
- **StormByte Suite port** — Migrated first-party repository and documentation links to StormByte-Suite, removed the retired String repository from the suite listing and Doxygen tag references, and updated the Logger and BuildMaster submodule URLs.
- **Database API and connection behavior**
	- **Breaking**: `BeginTransaction` now returns `Expected<Transaction, TransactionError>` instead of throwing when transaction start fails. `Database` construction may also report allocation failure rather than terminating from a `noexcept` constructor; rebuild consumers against this API revision.
	- **Breaking**: The public API contract changed beyond the DLL boundary fix. Inputs and storage use StormByte 2.0 types (`StormByte::Safe::String`, `StormByte::BinaryData`, `StormByte::Size`, `StormByte::ByteSize`, `std::string_view`), backend logger ownership is `StormByte::Safe::Shared<Logger::Log>` instead of raw pointers, and statement/query factory signatures accept view-based names and SQL text rather than rvalue strings. Input views are consumed within Database and are not retained across the DLL boundary; owned text uses Base Safe types. The exported layout of `Database`, `PreparedSTMT`, `Row`, `Rows`, `Value`, `NamedValue` and backend result containers was tightened to enforce DLL-safe ownership and out-of-line heap operations; consumers must recompile and update code that relied on old string, logger, or STL-owning ABI assumptions.
	- **Breaking**: Added shared telemetry state to exported `Database` and `PreparedSTMT` objects; rebuild consumers against this ABI revision.
	- **Breaking**: `Value::Type::LongInteger` and `Value::Type::UnsignedLongInteger` now store `long long int` and `unsigned long long int`, so they are 64-bit on every platform; use `Get<long long int>()` / `Get<unsigned long long int>()`. `long int` and `unsigned long int` are still accepted when constructing values.
	- Serialized operations on each built-in connection. RAII transactions hold exclusive connection access through commit or rollback and must remain on their creating thread.
- **Telemetry** — Added thread-safe operation counts, success/failure totals, returned-row counts, latency aggregates, backend error/warning categories, retained `StormByte::Safe::Shared` snapshots, and `StormByte::Safe::String` / `std::string` flattening for SQLite, PostgreSQL and MariaDB. Database telemetry derives from `StormByte::Telemetry` and uses independent best-effort `Clock::Sample` measurements, removing obsolete per-category locks and cumulative-time subtraction. Same-category samples can overlap or nest, and clock snapshots use `GetValues()`. Concurrent readers see zero rather than an uninitialized minimum-latency sentinel while the first result is being published.
- **Build and distribution** — Ported the library to BuildMaster 2 HOST with shared/static selection. Static consumers receive flattened private vendor dependencies; vendor archives do not need repacking. Updated Doxygen configuration for StormByte Base and Logger 2.0 and the dual-license terms. Dependency tags and generated cross-links use verified HTTPS endpoints; preprocessing recognizes the Safe declaration and caller-inline macros. Doxygen generation was checked with warnings treated as errors without persisting that validation policy in CMake.
- **Robustness tests** — Expanded tests for numeric boundaries, Row/Rows value semantics, backend scalar and binary round-trips, transaction rollback, prepared-statement failures, registry duplicates and cleanup with long names, same-connection concurrency, overlapping telemetry samples, Safe classification, and an installed external consumer using Safe collections and inherited SQLite path constructors.

### Fixed

- **MSSQL backend** — Fixed result and statement handling across FreeTDS versions:
	- Zero-length text and binary values are returned as empty values instead of being padded by `dbconvert()`.
	- Connections enable `QUOTED_IDENTIFIER`, so double-quoted identifiers resolve as identifiers on older DB-Library releases.
	- NULL prepared-statement parameters are sent as untyped `NULL`, allowing them in any column type such as `VARBINARY(MAX)`.
	- SQL Server error messages are kept instead of DB-Library's generic notice, and `Telemetry::Errors()` now counts them.
- **Windows bundled OpenSSL consumers** — Nested builds now link `ws2_32` and `crypt32` alongside the static bundled OpenSSL:
	- PostgreSQL's Meson configure, so its OpenSSL function checks succeed.
	- MariaDB Connector/C plugin DLLs such as `sha256_password.dll`.
- **Bundled TLS backends** — Build one pinned OpenSSL 3.5.9 for bundled FreeTDS, MariaDB Connector C and PostgreSQL instead of relying on host OpenSSL libraries.
- **MSSQL behavior tests** — Query the fixture's temporary tables with SQL Server syntax, distinguish empty values from SQL NULL using DB-Library indicators, use the MAX binary RPC type, and set the text-size limit so large LOB values are not truncated.
- **Base 2.0 and backend diagnostics**
	- Migrated ownership and text types to Base Safe APIs and updated the public headers for the new Base include layout.
	- `BeginTransaction` now converts `StormByte::Exception` failures into `TransactionError` rather than falling through to the generic unknown-backend result.
	- Fixed misleading indentation in MariaDB long-long result handling so the switch branch is explicit and warning-free.
- **Value conversion and binding correctness**
	- Reject floating-point to integer conversions outside the destination range before casting, including values that round to the unsigned or signed upper bound.
	- Preserve zero-length SQLite and MariaDB BLOB bindings as empty BLOB values rather than SQL NULL.
	- Reject excess MariaDB prepared-statement parameters consistently and preserve SQLite long-integer bind errors so failed parameter binding is reported by statement execution.
- **Windows builds**
	- Avoid applying `dllimport` visibility to static Windows consumers; shared/export macros remain enabled only for shared builds.
	- Fixed clang-cl build failures in the SQLite, PostgreSQL and MariaDB prepared statements by fully qualifying the base `PreparedSTMT` so it is not resolved against the `Database` class.
	- Fixed undefined SQLite symbols when linking the Windows DLL: `sqlite3` and `sqlite3_stmt` are now forward-declared as `struct`, matching SQLite's own declarations and MSVC name mangling.
	- Windows Release builds no longer use `/fp:fast`, so NaN and infinity handling in value conversions follows IEEE semantics.
	- 64-bit integer results from SQLite, PostgreSQL and MariaDB are no longer truncated to the 32-bit Windows `long`, which turned values such as `UINT_MAX` negative and made reads throw.

[2.0.0]: https://github.com/StormByte-Suite/StormByte-Database/compare/1.1.0...2.0.0

## [1.1.0] - 2026-09-13

### Fixed

- **Backend connection and transaction handling**
	- Fixed system connector discovery with `WITH_SQLITE=SYSTEM`, `WITH_POSTGRES=SYSTEM` and `WITH_MARIADB=SYSTEM`.
	- Fixed ownership transfer when moving connected SQLite, PostgreSQL and MariaDB backends.
	- Fixed transactions silently continuing after failed `BEGIN` or `COMMIT` commands.
	- Fixed PostgreSQL connection handling for credentials containing quotes or backslashes.
- **Prepared statements and result handling**
	- Fixed PostgreSQL and MariaDB numeric result parsing so invalid values return query errors instead of silent zero values.
	- Fixed PostgreSQL prepared statements with multiple text or numeric parameters.
	- Fixed SQLite prepared statements truncating unsigned integer values.
	- Fixed MariaDB prepared statements interpreting unsigned integers as negative values.
- Fixed `Row` index access to throw the Database `OutOfBounds` exception.

### Changed

- Updated the minimum requirements to StormByte Base 1.1.0 and StormByte-Logger 1.1.0, including component-aware Database exceptions.
- Adopted StormByte Base type concepts for Database value conversions.

[1.1.0]: https://github.com/StormByte-Suite/StormByte-Database/compare/1.0.0...1.1.0

## [1.0.0] - 2026-09-05

Initial public release of StormByte Database.

### Added

- **Database** abstract connection: Connect / Disconnect, Query / SilentQuery, named prepared statements, isolation and RAII transactions
- **Inheritance-oriented backends** — SQLite3, MariaDB and Postgres with protected constructors
- **Value** — type-erased SQL cell (NULL, integers, double, text, blob, bool) with safe `Get<T>()`
- **NamedValue**, **Row**, **Rows** — column lookup by name or index
- **PreparedSTMT** — positional binds (0-based), `nullptr` is SQL NULL, `ExpectedRows` on execute
- **Transaction** — rolls back if neither Commit nor Rollback ran
- **SslMode** for MariaDB and PostgreSQL (SQLite ignores it)
- **IsolationLevel** mapped per backend
- Optional backends: `WITH_SQLITE` / `WITH_POSTGRES` / `WITH_MARIADB` as OFF, SYSTEM or BUNDLED
- Exception types: ConnectionError, WrongValueType, ColumnNotFound, OutOfBounds, QueryException, UnknownSTMT, ExecuteError

### Notes

- First stable release of StormByte Database.
- Not thread-safe: one connection per thread.
- Needs a C++26 compiler and CMake ≥ 3.28.
[1.0.0]: https://github.com/StormByte-Suite/StormByte-Database/releases/tag/1.0.0
