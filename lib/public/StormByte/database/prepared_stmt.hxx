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

#include <StormByte/database/rows.hxx>
#include <StormByte/database/telemetry.hxx>
#include <StormByte/database/value.hxx>
#include <StormByte/logger/log.hxx>
#include <StormByte/size.hxx>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

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
		 * @class PreparedSTMT
		 * @brief Abstract prepared statement. Backends subclass this.
		 */
		class STORMBYTE_DATABASE_PUBLIC PreparedSTMT {
			public:
				/**
				 * @brief Copy name, query and logger.
				 * @param name Statement name.
				 * @param query SQL text.
				 * @param logger Logger instance.
				 */
				PreparedSTMT(std::string_view name, std::string_view query, const StormByte::Safe::Shared<Logger::Log>& logger);

				/**
				 * @brief Copy statement metadata and share connection telemetry.
				 * @param name Statement name.
				 * @param query SQL text.
				 * @param logger Logger instance.
				 * @param telemetry Connection telemetry handle.
				 */
				PreparedSTMT(std::string_view name, std::string_view query, const StormByte::Safe::Shared<Logger::Log>& logger,
					const StormByte::Safe::Shared<Telemetry>& telemetry);

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
				virtual ~PreparedSTMT() noexcept;

				/**
				 * @brief Copy assignment (deleted).
				 */
				PreparedSTMT &operator=(const PreparedSTMT &other) = delete;

				/**
				 * @brief Move assignment.
				 */
				PreparedSTMT &operator=(PreparedSTMT &&other) noexcept;

				/**
				 * @brief Bind arguments and execute.
				 * @tparam Args Argument types.
				 * @param args Positional bind values (0-based).
				 * @return Result rows or an error.
				 */
				template <typename... Args>
				ExpectedRows Execute(Args &&...args) {
					Reset();
					StormByte::Size idx{};
					(void)((Bind(idx++, std::forward<Args>(args))), ...);
					ExpectedRows result = DoExecute();
					Reset();
					return result;
				}

				/**
				 * @brief Statement name.
				 * @return Name.
				 */
				inline std::string_view Name() const noexcept {
					return m_name;
				}

				/**
				 * @brief SQL text.
				 * @return Query.
				 */
				inline std::string_view Query() const noexcept {
					return m_query;
				}

			protected:
				StormByte::Safe::Shared<Logger::Log> m_logger;	///< Shared logger, safe across the DLL boundary
				StormByte::Safe::Shared<Telemetry> m_telemetry; ///< Shared connection telemetry.

				/**
				 * @brief Record a backend event against the owning connection.
				 * @param event Backend event category.
				 */
				void RecordBackendEvent(BackendEvent event) noexcept {
					if (m_telemetry)
						m_telemetry->RecordEvent(event);
				}

				/**
				 * @brief Bind a value at @p index.
				 * @tparam T Value type.
				 * @param index Parameter index (0-based).
				 * @param value Value to bind.
				 */
				template <typename T>
				void Bind(StormByte::Size index, T &&value) noexcept {
					Binder(index, Value(std::forward<T>(value)));
				}

				/**
				 * @brief Bind SQL NULL at @p index.
				 * @param index Parameter index (0-based).
				 */
				void Bind(StormByte::Size index, std::nullptr_t) noexcept {
					Binder(index, Value());
				}

			private:
				StormByte::Safe::String m_name;	///< DLL-safe statement name
				StormByte::Safe::String m_query;	///< DLL-safe SQL text
				/**
				 * @brief Backend bind.
				 * @param index Parameter index (0-based).
				 * @param value Value to bind.
				 */
				virtual void Binder(StormByte::Size index, Value &&value) noexcept = 0;

				/**
				 * @brief Reset bindings / statement state.
				 */
				virtual void Reset() noexcept = 0;

				/**
				 * @brief Execute the prepared statement.
				 * @return Result rows or an error.
				 */
				virtual ExpectedRows DoExecute() = 0;
		};
	}
}

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules; derived statements must preserve Safe ownership. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::PreparedSTMT);
