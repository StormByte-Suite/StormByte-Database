/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Database.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-StormByte-Commercial
 */

#pragma once

#include <StormByte/database/prepared_stmt.hxx>
#include <StormByte/database/telemetry.hxx>

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
			class MSSQL;

			/**
			 * @class PreparedSTMT
			 * @brief A logical SQL statement executed through sp_executesql RPC.
			 */
			class STORMBYTE_DATABASE_PUBLIC PreparedSTMT final : public StormByte::Database::PreparedSTMT {
				friend class MSSQL;
				struct ConstructionKey {
					private:
						ConstructionKey() noexcept = default;
						friend class MSSQL;
				};
			public:
				/** @brief Deleted copy constructor. */
				PreparedSTMT(const PreparedSTMT&) = delete;
				/** @brief Move constructor. @param other Statement to move from. */
				PreparedSTMT(PreparedSTMT&& other) noexcept;
				/** @brief Release statement-owned state. */
				~PreparedSTMT() noexcept override;
				/** @brief Deleted copy assignment. */
				PreparedSTMT& operator=(const PreparedSTMT&) = delete;
				/** @brief Move assignment. @param other Statement to move from. @return This statement. */
				PreparedSTMT& operator=(PreparedSTMT&& other) noexcept;
				/**
				 * @brief Construct through MSSQL statement factory.
				 * @param key Factory-only key.
				 * @param name Statement name.
				 * @param query SQL text.
				 * @param connection DB-Library process.
				 * @param logger Optional logger.
				 * @param telemetry Connection telemetry.
				 */
				PreparedSTMT(ConstructionKey key, std::string_view name, std::string_view query,
					struct tds_dblib_dbprocess* connection,
					const StormByte::Safe::Shared<Logger::Log>& logger,
					const StormByte::Safe::Shared<StormByte::Database::Telemetry>& telemetry);

			private:
				struct tds_dblib_dbprocess* m_connection; ///< Owning connection, serialized by Database.
				std::vector<Value> m_parameters; ///< Bound parameter values for one execution.
				bool m_bind_error; ///< Whether a parameter index exceeded internal limits.

				/** @brief Store one bound parameter. @param index Zero-based parameter index. @param value Value to store. */
				void Binder(StormByte::Size index, Value&& value) noexcept override;
				/** @brief Clear parameter values after each execution. */
				void Reset() noexcept override;
				/** @brief Execute using sp_executesql and collect rows. */
				ExpectedRows DoExecute() override;
			};
		}
	}
}

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules; derived statements must preserve module-owned lifetime. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::MSSQL::PreparedSTMT);