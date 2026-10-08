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

#include <StormByte/database/prepared_stmt.hxx>
#include <StormByte/database/value.hxx>
#include <StormByte/safe/pointers.hxx>
#include <StormByte/size.hxx>

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
		 * @namespace StormByte::Database::SQLite
		 * @brief SQLite backend of the Database module.
		 */
		namespace SQLite {
			/**
			 * @struct StatementHandle
			 * @brief Private opaque holder for the native SQLite statement.
			 */
			struct StatementHandle;
			/**
			 * @class PreparedSTMT
			 * @brief SQLite prepared statement.
			 */
			class STORMBYTE_DATABASE_PUBLIC PreparedSTMT final : public StormByte::Database::PreparedSTMT {
					friend class SQLite3;

					struct STORMBYTE_DATABASE_PRIVATE ConstructionKey {
						private:
							ConstructionKey() noexcept = default;
							friend class SQLite3;
					};

				public:
					/**
					 * @brief Copy constructor (deleted).
					 */
					PreparedSTMT(const PreparedSTMT &other) = delete;

					/**
					 * @brief Move constructor.
					 */
					PreparedSTMT(PreparedSTMT &&other) noexcept;

					/**
					 * @brief Destructor.
					 */
					~PreparedSTMT() noexcept override;

					/**
					 * @brief Copy assignment (deleted).
					 */
					PreparedSTMT &operator=(const PreparedSTMT &other) = delete;

					/**
					 * @brief Move assignment.
					 */
					PreparedSTMT &operator=(PreparedSTMT &&other) noexcept;

					/**
					 * @brief Construct a SQLite statement through its backend factory.
					 * @param key Internal construction key.
					 * @param name Statement name.
					 * @param query SQL text.
					 * @param logger Non-owning logger observer.
					 * @param telemetry Shared operation telemetry.
					 */
					PreparedSTMT(ConstructionKey key, std::string_view name, std::string_view query,
						const StormByte::Safe::Shared<Logger::Log>& logger,
						const StormByte::Safe::Shared<StormByte::Database::Telemetry>& telemetry);

				private:
					StormByte::Safe::Unique<StatementHandle> m_statement_handle; ///< Opaque owner for the native SQLite statement.
					bool m_bind_error;	  ///< Whether binding failed or exceeded SQLite integer range

					/**
					 * @brief Copy statement name and SQL text.
					 * @param name Statement name.
					 * @param query SQL text.
					 * @param logger Logger instance.
					 */
					/**
					 * @brief Bind at @p index (0-based; SQLite is 1-based internally).
					 * @param index Parameter index.
					 * @param value Value to bind.
					 */
					void Binder(StormByte::Size index, Value &&value) noexcept override;

					/**
					 * @brief Step the statement and build Rows.
					 * @return Result rows or an error.
					 */
					ExpectedRows DoExecute() override;

					/**
					 * @brief Clear bindings and reset the statement.
					 */
					void Reset() noexcept override;
			};
		}	}}

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules; derived statements must preserve module-owned lifetime. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::SQLite::PreparedSTMT);
