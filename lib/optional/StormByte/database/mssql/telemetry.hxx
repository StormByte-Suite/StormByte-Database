/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Database.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-StormByte-Commercial
 */

#pragma once

#include <StormByte/database/telemetry.hxx>

#include <atomic>

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
			class PreparedSTMT;

			/**
			 * @class Telemetry
			 * @brief SQL Server-specific counters layered on common database telemetry.
			 */
			class STORMBYTE_DATABASE_PUBLIC Telemetry final : public StormByte::Database::Telemetry {
			public:
				/** @brief Construct zeroed SQL Server counters. */
				Telemetry() noexcept;
				/** @brief Out-of-line virtual destructor for DLL-safe destruction. */
				~Telemetry() noexcept override;
				/** @brief SQL Server errors reported by DB-Library. */
				std::uint64_t Errors() const noexcept;
				/** @brief Flatten common and SQL Server-specific metrics. */
				operator StormByte::Safe::String() const override;
			private:
				friend class MSSQL;
				friend class PreparedSTMT;
				/** @brief Count one SQL Server error reported to the DB-Library message handler. */
				void RecordError() noexcept;
				void RecordEvent(BackendEvent event) noexcept override;
				std::atomic<std::uint64_t> m_errors{0}; ///< DB-Library SQL Server errors.
			};
		}
	}
}

/** @brief Conditional DLL safety requires compatible ABIs and live Base and Database modules. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::MSSQL::Telemetry);