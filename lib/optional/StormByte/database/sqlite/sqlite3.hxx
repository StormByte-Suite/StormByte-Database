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
#include <StormByte/database/sqlite/prepared_stmt.hxx>
#include <StormByte/database/sqlite/telemetry.hxx>

#include <filesystem>
#include <string_view>

/**
 * @namespace StormByte
 * @brief Root namespace of the StormByte C++ suite.
 */
namespace StormByte {
	/**
	 * @namespace StormByte::Database
	 * @brief Database module of the StormByte suite.
	 */
	namespace Database {
		/**
		 * @namespace StormByte::Database::SQLite
		 * @brief SQLite backend of the Database module.
		 */
		namespace SQLite {
			/**
			 * @struct ConnectionHandle
			 * @brief Private opaque holder for the native SQLite database.
			 */
			struct ConnectionHandle;

			/**
			 * @class SQLite3
			 * @brief SQLite3 backend.
			 *
				 * @note Built-in connection operations are serialized. Transactions must stay on their creating thread.
			 * @note Inheritance-oriented. Constructors are protected. Derive and call them from your constructor.
			 */
			class STORMBYTE_DATABASE_PUBLIC SQLite3 : public Database {
				public:
					/**
					 * @brief Copy constructor (deleted).
					 */
					SQLite3(const SQLite3 &db) = delete;

					/**
					 * @brief Move constructor that transfers the database connection.
					 * @param db Database to move from.
					 */
					SQLite3(SQLite3 &&db) noexcept;

					/**
					 * @brief Copy assignment (deleted).
					 */
					SQLite3 &operator=(const SQLite3 &db) = delete;

					/**
					 * @brief Move assignment that transfers the database connection.
					 * @param db Database to move from.
					 * @return Reference to this database.
					 */
					SQLite3 &operator=(SQLite3 &&db) noexcept;

					/**
					 * @brief Destructor.
					 */
					~SQLite3() noexcept override;

					/**
					 * @brief Execute a query that returns rows.
					 * @param query SQL text.
					 * @return Result rows or an error.
					 */
					ExpectedRows Query(std::string_view query) noexcept override;

					/**
					 * @brief Execute a query that does not return rows.
					 * @param query SQL text.
					 * @return true on success.
					 */
					bool SilentQuery(std::string_view query) noexcept override;

				protected:
					/**
					 * @brief In-memory database.
					 * @param logger Logger instance.
					 */
					SQLite3(const StormByte::Safe::Shared<Logger::Log>& logger);

					/**
					 * @brief File-backed database.
					 * @param dbfile Path to the database file.
					 * @param logger Logger instance.
					 */
					STORMBYTE_FORCE_INLINE SQLite3(const std::filesystem::path &dbfile, const StormByte::Safe::Shared<Logger::Log>& logger)
						: SQLite3(PathText(dbfile), logger, Utf8Path{}) {}

					/**
					 * @brief File-backed database (moved path and logger).
					 * @param dbfile Path to the database file.
					 * @param logger Logger instance.
					 */
					STORMBYTE_FORCE_INLINE SQLite3(std::filesystem::path &&dbfile, const StormByte::Safe::Shared<Logger::Log>& logger)
						: SQLite3(PathText(dbfile), logger, Utf8Path{}) {}

					/**
					 * @brief Enable foreign keys (off by default in SQLite).
					 */
					void EnableForeignKeys();

					/**
					 * @brief Internal silent query.
					 * @param query SQL text.
					 * @return true on success.
					 */
					bool DoSilentQuery(std::string_view query) noexcept override;

				private:
					/**
					 * @struct Utf8Path
					 * @brief Distinguishes the DLL-safe path constructor from caller-side adapters.
					 */
					struct Utf8Path {};

					/**
					 * @brief Store an owned UTF-8 database path inside Database.
					 * @param dbfile Base-owned UTF-8 path.
					 * @param logger Logger instance.
					 * @param tag Selects the UTF-8 constructor.
					 */
					SQLite3(const StormByte::Safe::String& dbfile, const StormByte::Safe::Shared<Logger::Log>& logger, Utf8Path tag);

					/**
					 * @brief Convert a native path without transferring caller-owned STL storage.
					 * @param path Native path interpreted in the caller's module.
					 * @return Base-owned UTF-8 text suitable for sqlite3_open.
					 */
					STORMBYTE_FORCE_INLINE static StormByte::Safe::String PathText(const std::filesystem::path& path) {
						const auto text = path.u8string();
						return StormByte::Safe::String(std::string_view{reinterpret_cast<const char*>(text.data()), text.size()});
					}

					StormByte::Safe::String m_database_file; ///< Base-owned UTF-8 database file path
					StormByte::Safe::Unique<ConnectionHandle> m_connection_handle; ///< Opaque owner for the native SQLite database.

					/**
					 * @brief Open the database and initialize SQLite if needed.
					 * @return true on success.
					 */
					bool DoConnect() noexcept override;

					/**
					 * @brief Clear prepared statements before close.
					 */
					void DoPreDisconnect() noexcept override;

					/**
					 * @brief Close the database handle.
					 */
					void DoDisconnect() noexcept override;

					/**
					 * @brief Decrement global SQLite init refcount / shutdown.
					 */
					void DoPostDisconnect() noexcept override;

					/**
					 * @brief Create a SQLite prepared statement.
					 * @param name Statement name.
					 * @param query SQL text.
					 * @return Prepared statement or nullptr.
					 */
					StormByte::Safe::Unique<StormByte::Database::PreparedSTMT> CreatePreparedSTMT(std::string_view name, std::string_view query) noexcept override;

					/**
					 * @brief Map IsolationLevel to BEGIN DEFERRED/IMMEDIATE/EXCLUSIVE.
					 * @param level Isolation level.
					 */
					void DoBeginTransaction(IsolationLevel level) override;
			};
		}	}}

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules; derived facades must preserve Safe ownership. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::SQLite::SQLite3);
