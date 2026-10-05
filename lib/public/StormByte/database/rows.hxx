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

#include <StormByte/database/row.hxx>
#include <StormByte/size.hxx>

#include <iterator>
#include <memory>

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
		 * @class Rows
		 * @brief Ordered collection of result rows.
		 */
		class STORMBYTE_DATABASE_PUBLIC Rows {
			public:
				using value_type = Row;                                                 ///< Row value type.
				using size_type = StormByte::Size;                                      ///< Row count and index type.
				using reference = Row&;                                                 ///< Mutable row reference.
				using const_reference = const Row&;                                     ///< Read-only row reference.
				using pointer = Row*;                                                   ///< Mutable row pointer.
				using const_pointer = const Row*;                                       ///< Read-only row pointer.
				using iterator = pointer;                                               ///< Mutable pointer iterator.
				using const_iterator = const_pointer;                                   ///< Read-only pointer iterator.
				using reverse_iterator = std::reverse_iterator<iterator>;               ///< Mutable reverse iterator.
				using const_reverse_iterator = std::reverse_iterator<const_iterator>;   ///< Read-only reverse iterator.

				/**
				 * @brief Default constructor.
				 */
				Rows() noexcept;

				/**
				 * @brief Copy constructor.
				 */
				Rows(const Rows &other);

				/**
				 * @brief Move constructor.
				 */
				Rows(Rows &&other) noexcept;

				/**
				 * @brief Destructor.
				 */
				~Rows() noexcept;

				/**
				 * @brief Copy assignment.
				 */
				Rows &operator=(const Rows &other);

				/**
				 * @brief Move assignment.
				 */
				Rows &operator=(Rows &&other) noexcept;

				/**
				 * @brief Equality of the ordered rows.
				 * @param other Rows to compare.
				 * @return Whether both collections contain equal rows.
				 */
				bool operator==(const Rows &other) const;

				/**
				 * @brief Inequality of the ordered rows.
				 * @param other Rows to compare.
				 * @return Whether the collections differ.
				 */
				bool operator!=(const Rows &other) const;

				/**
				 * @brief Mutable begin pointer.
				 * @return Pointer to the first row or null when empty.
				 */
				iterator begin() noexcept;

				/**
				 * @brief Read-only begin pointer.
				 * @return Pointer to the first row or null when empty.
				 */
				const_iterator begin() const noexcept;

				/**
				 * @brief Mutable end pointer.
				 * @return Pointer past the last row or null when empty.
				 */
				iterator end() noexcept;

				/**
				 * @brief Read-only end pointer.
				 * @return Pointer past the last row or null when empty.
				 */
				const_iterator end() const noexcept;

				/**
				 * @brief Read-only begin pointer.
				 * @return Pointer to the first row or null when empty.
				 */
				const_iterator cbegin() const noexcept;

				/**
				 * @brief Read-only end pointer.
				 * @return Pointer past the last row or null when empty.
				 */
				const_iterator cend() const noexcept;

				/**
				 * @brief Mutable reverse begin.
				 * @return Reverse iterator.
				 */
				reverse_iterator rbegin() noexcept;

				/**
				 * @brief Mutable reverse end.
				 * @return Reverse iterator.
				 */
				reverse_iterator rend() noexcept;

				/**
				 * @brief Read-only reverse begin.
				 * @return Reverse iterator.
				 */
				const_reverse_iterator rbegin() const noexcept;

				/**
				 * @brief Read-only reverse end.
				 * @return Reverse iterator.
				 */
				const_reverse_iterator rend() const noexcept;

				/**
				 * @brief Read-only reverse begin.
				 * @return Reverse iterator.
				 */
				const_reverse_iterator crbegin() const noexcept;

				/**
				 * @brief Read-only reverse end.
				 * @return Reverse iterator.
				 */
				const_reverse_iterator crend() const noexcept;

				/**
				 * @brief Row count.
				 * @return Number of rows.
				 */
				StormByte::Size size() const noexcept;

				/**
				 * @brief Whether the collection has no rows.
				 * @return True when empty.
				 */
				bool empty() const noexcept;

				/**
				 * @brief Access a row by zero-based index.
				 * @param index Row index.
				 * @return Mutable row reference.
				 * @throws OutOfBounds when @p index is outside this collection.
				 */
				Row &operator[](StormByte::Size index);

				/**
				 * @brief Access a row by zero-based index.
				 * @param index Row index.
				 * @return Read-only row reference.
				 * @throws OutOfBounds when @p index is outside this collection.
				 */
				const Row &operator[](StormByte::Size index) const;

				/**
				 * @brief Append a copy of a row.
				 * @param row Row to copy.
				 */
				void add(const Row &row);

				/**
				 * @brief Append a moved row.
				 * @param row Row to move.
				 */
				void add(Row &&row);

				/**
				 * @brief Search for an equal row.
				 * @param row Row to find.
				 * @return Whether the row exists in this collection.
				 */
				bool has_item(const Row &row) const;

				/**
				 * @brief Number of rows.
				 * @return Count.
				 */
				StormByte::Size Count() const noexcept;

			private:
				/**
				 * @class ResultSet
				 * @brief Private ordered row storage.
				 */
				struct ResultSet;

				std::unique_ptr<ResultSet> m_results;	///< Opaque storage owned by the Database DLL
		};
	}
}

/** @brief Conditional DLL safety requires a compatible ABI and live Database module; storage operations remain in Database. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Rows);
