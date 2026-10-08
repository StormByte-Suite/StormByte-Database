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

#include <StormByte/database/telemetry.hxx>
#include <StormByte/safe/atomic.hxx>

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
			Telemetry();
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
			StormByte::Safe::Atomic<std::uint64_t> m_corruption_errors{0}; ///< Corrupt/malformed result codes.
			StormByte::Safe::Atomic<std::uint64_t> m_busy_errors{0}; ///< Busy/locked results.
			StormByte::Safe::Atomic<std::uint64_t> m_constraint_errors{0}; ///< Constraint results.
			StormByte::Safe::Atomic<std::uint64_t> m_io_errors{0}; ///< I/O results.
	};
}

/** @brief Conditional DLL safety requires compatible ABIs and live Base and Database modules. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::SQLite::Telemetry);
