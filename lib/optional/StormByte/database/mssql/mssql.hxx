/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Database.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-StormByte-Commercial
 */

#pragma once

#include <StormByte/database/database.hxx>
#include <StormByte/database/mssql/telemetry.hxx>
#include <StormByte/database/mssql/prepared_stmt.hxx>

#include <string>
#include <string_view>
#include <vector>

struct tds_dblib_dbprocess;

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
					std::string m_host; ///< SQL Server host.
					std::string m_user; ///< SQL Server login.
					std::string m_password; ///< SQL Server password.
					std::string m_database; ///< Initial database name.
					int m_port; ///< SQL Server TCP port.
					struct tds_dblib_dbprocess* m_connection; ///< FreeTDS DBPROCESS handle.
					std::string m_last_error; ///< Most recent DB-Library callback error.

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

					/** @brief DB-Library error callback. */
					static int ErrorHandler(struct tds_dblib_dbprocess* process, int severity, int database_error,
						int operating_system_error, char* database_message, char* operating_system_message);
					/** @brief DB-Library informational message callback. */
					static int MessageHandler(struct tds_dblib_dbprocess* process, int message_number, int state,
						int severity, char* text, char* server, char* procedure, int line);
					/** @brief Execute SQL through sp_executesql with typed RPC parameters. */
					ExpectedRows ExecuteParameterized(std::string_view query, const std::vector<Value>& parameters);
			};
		}
	}
}

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules; derived facades must preserve Safe ownership. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::MSSQL::MSSQL);