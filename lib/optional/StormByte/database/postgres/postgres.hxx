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
#include <StormByte/database/postgres/prepared_stmt.hxx>
#include <StormByte/database/postgres/telemetry.hxx>

#include <memory>
#include <string>
#include <string_view>

struct pg_conn;

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
		 * @namespace StormByte::Database::Postgres
		 * @brief PostgreSQL backend of the Database module.
		 */
		namespace Postgres {
			/**
			 * @class Postgres
			 * @brief PostgreSQL backend.
			 *
				 * @note Built-in connection operations are serialized. Transactions must stay on their creating thread.
			 * @note Inheritance-oriented. Constructors are protected. SetSslMode() before Connect() if needed.
			 */
			class STORMBYTE_DATABASE_PUBLIC Postgres : public Database {
				public:
					/**
					 * @brief Copy constructor (deleted).
					 */
					Postgres(const Postgres &db) = delete;

					/**
					 * @brief Move constructor that transfers the database connection.
					 * @param db Database to move from.
					 */
					Postgres(Postgres &&db) noexcept;

					/**
					 * @brief Copy assignment (deleted).
					 */
					Postgres &operator=(const Postgres &db) = delete;

					/**
					 * @brief Move assignment that transfers the database connection.
					 * @param db Database to move from.
					 * @return Reference to this database.
					 */
					Postgres &operator=(Postgres &&db) noexcept;

					/**
					 * @brief Destructor.
					 */
					~Postgres() noexcept override;

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
					 * @brief Copy connection parameters into the backend.
					 * @param host Host name or address.
					 * @param user User name.
					 * @param password Password.
					 * @param db_name Database name.
					 * @param logger Logger instance.
					 */
					Postgres(std::string_view host, std::string_view user, std::string_view password,
									 std::string_view db_name, const StormByte::Safe::Shared<Logger::Log>& logger);

					/**
					 * @brief Internal silent query.
					 * @param query SQL text.
					 * @return true on success.
					 */
					bool DoSilentQuery(std::string_view query) noexcept override;

				private:
					std::string m_host;		///< Host
					std::string m_user;		///< User
					std::string m_password; ///< Password
					std::string m_dbname;	///< Database name
					struct pg_conn *m_conn; ///< Connection handle

					/**
					 * @brief Connect via PQconnectdb.
					 * @return true on success.
					 */
					bool DoConnect() noexcept override;

					/**
					 * @brief Clear prepared statements.
					 */
					void DoPreDisconnect() noexcept override;

					/**
					 * @brief Close the connection.
					 */
					void DoDisconnect() noexcept override;

					/**
					 * @brief Create a PostgreSQL prepared statement (PQprepare).
					 * @param name Statement name.
					 * @param query SQL text.
					 * @return Prepared statement or nullptr.
					 */
					StormByte::Safe::Unique<StormByte::Database::PreparedSTMT> CreatePreparedSTMT(std::string_view name, std::string_view query) noexcept override;

					/**
					 * @brief BEGIN with isolation.
					 * @param level Isolation level.
					 */
					void DoBeginTransaction(IsolationLevel level) override;
			};
		}	}}

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules; derived facades must preserve Safe ownership. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Postgres::Postgres);
