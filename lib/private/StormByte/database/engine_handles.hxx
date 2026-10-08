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
		 * @namespace StormByte::Database::MariaDB
		 * @brief MariaDB backend of the Database module.
		 */
		namespace MariaDB {
			/**
			 * @struct ConnectionHandle
			 * @brief Private opaque owner for a native MariaDB connection handle.
			 */
			struct ConnectionHandle {
				void* m_native_connection{}; ///< Driver connection handle, stored opaquely.
			};

			/**
			 * @struct StatementHandle
			 * @brief Private opaque connection and statement handles for MariaDB.
			 */
			struct StatementHandle {
				void* m_native_connection{}; ///< Borrowed driver connection handle.
				void* m_native_statement{}; ///< Driver statement handle.
			};
		}

		/**
		 * @namespace StormByte::Database::MSSQL
		 * @brief Microsoft SQL Server backend using FreeTDS DB-Library.
		 */
		namespace MSSQL {
			/**
			 * @struct StatementHandle
			 * @brief Private borrowed DB-Library process handle for a statement.
			 */
			struct StatementHandle {
				void* m_native_connection{}; ///< Borrowed DB-Library process handle, stored opaquely.
			};

			/**
			 * @struct ConnectionHandle
			 * @brief Private opaque DB-Library process and callback state.
			 */
			struct ConnectionHandle {
				void* m_native_connection{}; ///< DB-Library process handle, stored opaquely.
			};
		}

		/**
		 * @namespace StormByte::Database::Postgres
		 * @brief PostgreSQL backend of the Database module.
		 */
		namespace Postgres {
			/**
			 * @struct ConnectionHandle
			 * @brief Private opaque owner for a native PostgreSQL connection handle.
			 */
			struct ConnectionHandle {
				void* m_native_connection{}; ///< PostgreSQL connection handle, stored opaquely.
			};

			/**
			 * @struct StatementHandle
			 * @brief Private borrowed PostgreSQL connection handle for a statement.
			 */
			struct StatementHandle {
				void* m_native_connection{}; ///< Borrowed PostgreSQL connection handle, stored opaquely.
			};
		}

		/**
		 * @namespace StormByte::Database::SQLite
		 * @brief SQLite backend of the Database module.
		 */
		namespace SQLite {
			/**
			 * @struct ConnectionHandle
			 * @brief Private opaque owner for a native SQLite database handle.
			 */
			struct ConnectionHandle {
				void* m_native_connection{}; ///< SQLite database handle, stored opaquely.
			};

			/**
			 * @struct StatementHandle
			 * @brief Private opaque owner for a native SQLite statement handle.
			 */
			struct StatementHandle {
				void* m_native_statement{}; ///< SQLite statement handle, stored opaquely.
			};
		}
	}
}
