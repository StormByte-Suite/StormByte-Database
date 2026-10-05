/*
 * SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-StormByte-Commercial
 */
#pragma once

#include <StormByte/database/telemetry.hxx>

#include <string_view>
#include <atomic>

namespace StormByte::Database::Postgres {
	class Postgres;
	class PreparedSTMT;

	/**
	 * @class Telemetry
	 * @brief PostgreSQL-specific counters layered on common database telemetry.
	 */
	class STORMBYTE_DATABASE_PUBLIC Telemetry : public StormByte::Database::Telemetry {
		public:
			/** @brief Construct zeroed PostgreSQL counters. */
			Telemetry() noexcept;
			/** @brief Out-of-line virtual destructor for DLL-safe destruction. */
			~Telemetry() noexcept override;
			/** @brief SQLSTATE serialization failures (40001). */
			std::uint64_t SerializationConflicts() const noexcept;
			/** @brief SQLSTATE deadlock detections (40P01). */
			std::uint64_t Deadlocks() const noexcept;
			/** @brief Connection exception SQLSTATEs (class 08). */
			std::uint64_t ConnectionErrors() const noexcept;
			/** @brief Flatten common and PostgreSQL-specific metrics. */
			operator StormByte::Safe::String() const override;
		private:
			friend class Postgres;
			friend class PreparedSTMT;
			void RecordSqlState(std::string_view sql_state) noexcept;
			void RecordEvent(BackendEvent event) noexcept override;
			std::atomic<std::uint64_t> m_serialization_conflicts{0}; ///< Serialization failures.
			std::atomic<std::uint64_t> m_deadlocks{0}; ///< Deadlock errors.
			std::atomic<std::uint64_t> m_connection_errors{0}; ///< Connection-class SQLSTATEs.
	};
}

/** @brief Conditional DLL safety requires compatible ABIs and live Base and Database modules. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Postgres::Telemetry);
