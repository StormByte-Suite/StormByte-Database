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
#include <StormByte/database/mssql/telemetry.hxx>
#include <StormByte/database/mssql/prepared_stmt.hxx>
#include <StormByte/safe/vector.hxx>

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
		 * @namespace StormByte::Database::MSSQL
		 * @brief Microsoft SQL Server backend using FreeTDS DB-Library.
		 */
		namespace MSSQL {
			/**
			 * @struct ConnectionHandle
			 * @brief Private opaque holder for DB-Library connection state.
			 */
			struct ConnectionHandle;
			/**
			 * @struct CallbackHandlers
			 * @brief Source-private adapter for native DB-Library callbacks.
			 */
			struct CallbackHandlers;

			/**
			 * @class MSSQL
			 * @brief Microsoft SQL Server backend.
			 *
			 * @note Prepared statements execute through sp_executesql RPC with typed parameters.
			 * @note The backend serializes each connection through Database's operation mutex.
			 */
			class STORMBYTE_DATABASE_PUBLIC MSSQL : public StormByte::Database::Database {
				public:
					/** @brief Deleted copy constructor. */
					MSSQL(const MSSQL&) = delete;
					/** @brief Move constructor. @param other Backend to move from. */
					MSSQL(MSSQL&& other) noexcept;
					/** @brief Deleted copy assignment. */
					MSSQL& operator=(const MSSQL&) = delete;
					/** @brief Move assignment. @param other Backend to move from. @return This backend. */
					MSSQL& operator=(MSSQL&& other) noexcept;
					/** @brief Disconnect and release the DB-Library process. */
					~MSSQL() noexcept override;

					/**
					 * @brief Execute a SQL batch and collect its first result set.
					 * @param query SQL text; embedded NUL is rejected.
					 * @return Collected rows or QueryException.
					 */
					ExpectedRows Query(std::string_view query) noexcept override;
					/** @brief Execute a SQL batch without returning rows. @param query SQL text. @return Success. */
					bool SilentQuery(std::string_view query) noexcept override;

				protected:
					/**
					 * @brief Construct from copied connection settings.
					 * @param host Server name or address.
					 * @param user SQL login; empty requests integrated authentication where supported.
					 * @param password SQL login password.
					 * @param database Initial database name.
					 * @param port TDS TCP port, normally 1433.
					 * @param logger Optional logger.
					 */
					MSSQL(std::string_view host, std::string_view user, std::string_view password,
						std::string_view database, int port, const StormByte::Safe::Shared<Logger::Log>& logger);

					/** @brief Execute an internal no-result query. @param query SQL text. @return Success. */
					bool DoSilentQuery(std::string_view query) noexcept override;

				private:
					friend class PreparedSTMT;
					friend struct CallbackHandlers;
					StormByte::Safe::String m_host; ///< Base-owned SQL Server host.
					StormByte::Safe::String m_user; ///< Base-owned SQL Server login.
					StormByte::Safe::String m_password; ///< Base-owned SQL Server password.
					StormByte::Safe::String m_database; ///< Base-owned initial database name.
					int m_port; ///< SQL Server TCP port.
					StormByte::Safe::Unique<ConnectionHandle> m_connection_handle; ///< Opaque owner for DB-Library state.
					StormByte::Safe::String m_last_error; ///< Base-owned most recent DB-Library callback error.

					/** @brief Open the DB-Library connection. @return Whether login succeeded. */
					bool DoConnect() noexcept override;
					/** @brief Close the DB-Library connection. */
					void DoDisconnect() noexcept override;
					/** @brief Clear logical statements before releasing DB-Library state. */
					void DoPreDisconnect() noexcept override;
					/** @brief Create a statement wrapper; preparation occurs when Execute is called. */
					StormByte::Safe::Unique<StormByte::Database::PreparedSTMT> CreatePreparedSTMT(
						std::string_view name, std::string_view query) noexcept override;
					/** @brief Begin a transaction at the requested isolation level. */
					void DoBeginTransaction(IsolationLevel level) override;

					/** @brief Execute SQL through sp_executesql with typed RPC parameters. */
					ExpectedRows ExecuteParameterized(std::string_view query, const StormByte::Safe::Vector<Value>& parameters);
			};
		}
	}
}

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules; derived facades must preserve Safe ownership. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::MSSQL::MSSQL);
