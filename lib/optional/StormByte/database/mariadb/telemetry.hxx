/*
 * SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-StormByte-Commercial
 */
#pragma once

#include <StormByte/database/telemetry.hxx>

#include <atomic>

namespace StormByte::Database::MariaDB {
	class MariaDB;
	class PreparedSTMT;

	/**
	 * @class Telemetry
	 * @brief MariaDB-specific counters layered on common database telemetry.
	 */
	class STORMBYTE_DATABASE_PUBLIC Telemetry : public StormByte::Database::Telemetry {
		public:
			/** @brief Construct zeroed MariaDB counters. */
			Telemetry() noexcept;
			/** @brief Out-of-line virtual destructor for DLL-safe destruction. */
			~Telemetry() noexcept override;
			/** @brief MariaDB deadlock errors (1213). */
			std::uint64_t Deadlocks() const noexcept;
			/** @brief MariaDB lock wait timeout errors (1205). */
			std::uint64_t LockTimeouts() const noexcept;
			/** @brief MariaDB warnings reported after successful operations. */
			std::uint64_t Warnings() const noexcept;
			/** @brief Flatten common and MariaDB-specific metrics. */
			operator StormByte::Safe::String() const override;
		private:
			friend class MariaDB;
			friend class PreparedSTMT;
			void RecordMariaDBError(unsigned int error_code) noexcept;
			void RecordMariaDBWarnings(std::uint64_t count) noexcept;
			void RecordEvent(BackendEvent event) noexcept override;
			std::atomic<std::uint64_t> m_deadlocks{0}; ///< Deadlock error count.
			std::atomic<std::uint64_t> m_lock_timeouts{0}; ///< Lock timeout error count.
			std::atomic<std::uint64_t> m_warnings{0}; ///< Warning event count.
	};
}

/** @brief Conditional DLL safety requires compatible ABIs and live Base and Database modules. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::MariaDB::Telemetry);
