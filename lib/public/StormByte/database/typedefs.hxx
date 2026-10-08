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

#include <StormByte/database/exception.hxx>
#include <StormByte/expected.hxx>
#include <StormByte/safe/binary.hxx>
#include <StormByte/safe/string.hxx>
#include <StormByte/safe/variant.hxx>

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
		class Rows;

		/**
		 * @typedef ValuesVariant
		 * @brief Column alternatives stored on Base's heap. Safe::Monostate is SQL NULL.
		 */
		using ValuesVariant = StormByte::Safe::Variant<
			StormByte::Safe::Monostate,
			int,
			unsigned int,
			long long int,
			unsigned long long int,
			double,
			StormByte::Safe::String,
			bool,
			StormByte::Safe::Binary>;

		/**
		 * @typedef ExpectedRows
		 * @brief Query result: Rows or QueryException.
		 */
		using ExpectedRows = Expected<Rows, QueryException>;

		/**
		 * @enum SslMode
		 * @brief TLS policy for MariaDB / PostgreSQL. SQLite ignores it.
		 */
		enum class SslMode {
			Default, ///< Driver default
			Disable, ///< No TLS
			Prefer,	 ///< TLS if the server offers it
			Require	 ///< Fail if TLS cannot be used
		};

		/**
		 * @enum IsolationLevel
		 * @brief Isolation for BeginTransaction(). Mapping is backend-specific.
		 */
		enum class IsolationLevel {
			Default,		 ///< Backend default
			ReadUncommitted, ///< Dirty reads where supported
			ReadCommitted,	 ///< Committed data only
			RepeatableRead,	 ///< Stable reads in the transaction
			Serializable	 ///< Full serializability where supported
		};
	}
}
