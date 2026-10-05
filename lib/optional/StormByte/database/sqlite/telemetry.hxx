/*
 * SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-StormByte-Commercial
 */
#pragma once

#include <StormByte/database/telemetry.hxx>

#include <atomic>

namespace StormByte::Database::SQLite {
	class SQLite3;
	class PreparedSTMT;
	/**
	 * @class Telemetry
	 * @brief SQLite-specific counters layered on common database telemetry.
	 */
	class STORMBYTE_DATABASE_PUBLIC Telemetry : public StormByte::Database::Telemetry {
		public:
			/** @brief Construct zeroed SQLite counters. */
			Telemetry() noexcept;
			/** @brief Out-of-line virtual destructor for DLL-safe destruction. */
			~Telemetry() noexcept override;
			/** @brief SQLite BUSY/LOCKED failures. */
			std::uint64_t BusyErrors() const noexcept;
			/** @brief SQLite constraint failures. */
			std::uint64_t ConstraintErrors() const noexcept;
			/** @brief SQLite I/O failures. */
			std::uint64_t IoErrors() const noexcept;
			/** @brief SQLite corrupt or malformed database failures. */
			std::uint64_t CorruptionErrors() const noexcept;
			/** @brief Flatten common and SQLite-specific metrics. */
			operator StormByte::Safe::String() const override;
		private:
			friend class SQLite3;
			friend class PreparedSTMT;
			void RecordSQLiteResult(int result_code) noexcept;
			void RecordEvent(BackendEvent event) noexcept override;
			std::atomic<std::uint64_t> m_corruption_errors{0}; ///< Corrupt/malformed result codes.
			std::atomic<std::uint64_t> m_busy_errors{0}; ///< Busy/locked results.
			std::atomic<std::uint64_t> m_constraint_errors{0}; ///< Constraint results.
			std::atomic<std::uint64_t> m_io_errors{0}; ///< I/O results.
	};
}

/** @brief Conditional DLL safety requires compatible ABIs and live Base and Database modules. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::SQLite::Telemetry);
