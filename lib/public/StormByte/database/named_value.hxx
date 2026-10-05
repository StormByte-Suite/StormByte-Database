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

#include <StormByte/database/value.hxx>

#include <string>
#include <string_view>

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
		 * @class NamedValue
		 * @brief Value plus column name.
		 */
		class STORMBYTE_DATABASE_PUBLIC NamedValue : public Value {
			public:
				/**
				 * @brief Copy name and value.
				 * @param name Column name.
				 * @param value Value.
				 */
				NamedValue(std::string_view name, const Value &value);

				/**
				 * @brief Move name and value.
				 * @param name Column name.
				 * @param value Value.
				 */
				NamedValue(std::string_view name, Value &&value);

				/**
				 * @brief Copy constructor.
				 */
				NamedValue(const NamedValue &other);

				/**
				 * @brief Move constructor.
				 */
				NamedValue(NamedValue &&other) noexcept;

				/**
				 * @brief Destructor.
				 */
				~NamedValue() noexcept override;

				/**
				 * @brief Copy assignment.
				 */
				NamedValue &operator=(const NamedValue &other);

				/**
				 * @brief Move assignment.
				 */
				NamedValue &operator=(NamedValue &&other) noexcept;

				/**
				 * @brief Equality (name and value).
				 * @param other Other value.
				 * @return true if equal.
				 */
				inline bool operator==(const NamedValue &other) const noexcept {
					return m_name == other.m_name && Value::operator==(other);
				}

				/**
				 * @brief Inequality.
				 * @param other Other value.
				 * @return true if not equal.
				 */
				inline bool operator!=(const NamedValue &other) const noexcept {
					return !(*this == other);
				}

				/**
				 * @brief Column name.
				 * @return Name.
				 */
				inline std::string_view Name() const noexcept {
					return m_name;
				}

			private:
				StormByte::Safe::String m_name; ///< DLL-safe column name
		};
	}
}

/** @brief Conditional DLL safety requires a compatible ABI and live Database module; derivatives must preserve module-owned lifetime. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::NamedValue);
