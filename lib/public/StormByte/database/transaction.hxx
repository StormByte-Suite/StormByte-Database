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

#include <StormByte/database/visibility.h>
#include <StormByte/safe/pointers.hxx>

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
		class Database;

		/**
		 * @class Transaction
		 * @brief RAII transaction. Rolls back if neither Commit() nor Rollback() ran before destruction.
		 * @note Holds exclusive access to its connection and must remain on its creating thread.
		 */
		class STORMBYTE_DATABASE_PUBLIC Transaction {
			public:
				/**
				 * @brief Bind to a database connection.
				 * @param db Owning database.
				 */
				explicit Transaction(Database &db);

				/**
				 * @brief Copy constructor (deleted).
				 */
				Transaction(const Transaction &) = delete;

				/**
				 * @brief Copy assignment (deleted).
				 */
				Transaction &operator=(const Transaction &) = delete;

				/**
				 * @brief Move constructor.
				 * @param other Source transaction.
				 */
				Transaction(Transaction &&other) noexcept;

				/**
				 * @brief Move assignment.
				 * @param other Source transaction.
				 * @return *this.
				 */
				Transaction &operator=(Transaction &&other) noexcept;

				/**
				 * @brief Destructor. Rolls back if still active.
				 */
				~Transaction() noexcept;

				/**
				 * @brief Commit.
				 */
				void Commit();

				/**
				 * @brief Rollback.
				 */
				void Rollback();

				/**
				 * @brief Whether neither Commit nor Rollback has been called.
				 * @return true if still active.
				 */
				bool IsActive() const noexcept {
					return m_active;
				}

			private:
				/**
				 * @class ConnectionLock
				 * @brief Opaque RAII owner of the exclusive database connection lock.
				 */
				class ConnectionLock;
				Database *m_db; ///< Owning database (nullptr after move)
				bool m_active;	///< true until Commit / Rollback / destructor
				bool m_lock_held; ///< Whether this transaction currently owns a recursive lock

				/**
				 * @brief Release the transaction's connection lock.
				 */
				void ReleaseLock() noexcept;
		};
	}
}

/** @brief Conditional DLL safety requires a live Database and compatible ABI; the transaction stays on its creating thread. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Transaction);
