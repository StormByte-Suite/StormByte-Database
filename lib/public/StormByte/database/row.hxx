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

#include <StormByte/database/named_value.hxx>
#include <StormByte/size.hxx>
#include <StormByte/type_traits.hxx>

#include <iterator>
#include <memory>
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
		 * @class Row
		 * @brief One result row: ordered NamedValues with lookup by column name.
		 */
		class STORMBYTE_DATABASE_PUBLIC Row {
			public:
				using value_type = NamedValue;                                          ///< Column value type.
				using size_type = StormByte::Size;                                      ///< Column count and index type.
				using reference = NamedValue&;                                          ///< Mutable column reference.
				using const_reference = const NamedValue&;                              ///< Read-only column reference.
				using pointer = NamedValue*;                                            ///< Mutable column pointer.
				using const_pointer = const NamedValue*;                                ///< Read-only column pointer.
				using iterator = pointer;                                               ///< Mutable pointer iterator.
				using const_iterator = const_pointer;                                   ///< Read-only pointer iterator.
				using reverse_iterator = std::reverse_iterator<iterator>;               ///< Mutable reverse iterator.
				using const_reverse_iterator = std::reverse_iterator<const_iterator>;   ///< Read-only reverse iterator.

				/**
				 * @brief Default constructor.
				 */
				Row() noexcept;

				/**
				 * @brief Copy constructor.
				 * @param other Source row.
				 */
				Row(const Row &other);

				/**
				 * @brief Move constructor.
				 */
				Row(Row &&other) noexcept;

				/**
				 * @brief Destructor.
				 */
				~Row() noexcept;

				/**
				 * @brief Copy assignment.
				 * @param other Source row.
				 * @return *this.
				 */
				Row &operator=(const Row &other);

				/**
				 * @brief Move assignment.
				 */
				Row &operator=(Row &&other) noexcept;

				/**
				 * @brief Equality of the ordered columns.
				 * @param other Row to compare.
				 * @return Whether both rows have equal named values.
				 */
				bool operator==(const Row &other) const;

				/**
				 * @brief Inequality of the ordered columns.
				 * @param other Row to compare.
				 * @return Whether the rows differ.
				 */
				bool operator!=(const Row &other) const;

				/**
				 * @brief Mutable begin pointer.
				 * @return Pointer to the first column or null when empty.
				 */
				iterator begin() noexcept;

				/**
				 * @brief Read-only begin pointer.
				 * @return Pointer to the first column or null when empty.
				 */
				const_iterator begin() const noexcept;

				/**
				 * @brief Mutable end pointer.
				 * @return Pointer past the last column or null when empty.
				 */
				iterator end() noexcept;

				/**
				 * @brief Read-only end pointer.
				 * @return Pointer past the last column or null when empty.
				 */
				const_iterator end() const noexcept;

				/**
				 * @brief Read-only begin pointer.
				 * @return Pointer to the first column or null when empty.
				 */
				const_iterator cbegin() const noexcept;

				/**
				 * @brief Read-only end pointer.
				 * @return Pointer past the last column or null when empty.
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
				 * @brief Number of columns.
				 * @return Column count.
				 */
				StormByte::Size size() const noexcept;

				/**
				 * @brief Whether the row has no columns.
				 * @return True when empty.
				 */
				bool empty() const noexcept;

				/**
				 * @brief Access by column name (const lvalue).
				 * @param columnName Column name.
				 * @return Reference to the value.
				 * @throws ColumnNotFound if the name is absent.
				 */
				const Value &operator[](std::string_view columnName) const &;

				/**
				 * @brief Access by column name (lvalue).
				 * @param columnName Column name.
				 * @return Reference to the value.
				 * @throws ColumnNotFound if the name is absent.
				 */
				Value &operator[](std::string_view columnName) &;

				/**
				 * @brief Access by column name (rvalue).
				 * @param columnName Column name.
				 * @return Value (moved).
				 * @throws ColumnNotFound if the name is absent.
				 */
				Value operator[](std::string_view columnName) &&;

				/**
				 * @brief Access a column by zero-based index (const lvalue).
				 * @param index Column index.
				 * @return Const reference to the named value.
				 * @throws OutOfBounds if @p index is outside this row.
				 */
				const NamedValue &operator[](StormByte::Size index) const &;

				/**
				 * @brief Access a column by integral zero-based index (const lvalue).
				 * @tparam T Integral index type.
				 * @param index Column index.
				 * @return Const reference to the named value.
				 */
				template <StormByte::Type::Integral T>
				const NamedValue &operator[](T index) const & {
					return (*this)[StormByte::Size{index}];
				}

				/**
				 * @brief Access a column by zero-based index (lvalue).
				 * @param index Column index.
				 * @return Reference to the named value.
				 * @throws OutOfBounds if @p index is outside this row.
				 */
				NamedValue &operator[](StormByte::Size index) &;

				/**
				 * @brief Access a column by integral zero-based index (lvalue).
				 * @tparam T Integral index type.
				 * @param index Column index.
				 * @return Mutable reference to the named value.
				 */
				template <StormByte::Type::Integral T>
				NamedValue &operator[](T index) & {
					return (*this)[StormByte::Size{index}];
				}

				/**
				 * @brief Access a column by zero-based index (rvalue).
				 * @param index Column index.
				 * @return Named value moved from this row.
				 * @throws OutOfBounds if @p index is outside this row.
				 */
				NamedValue operator[](StormByte::Size index) &&;

				/**
				 * @brief Access a column by integral zero-based index (rvalue).
				 * @tparam T Integral index type.
				 * @param index Column index.
				 * @return Named value moved from this row.
				 */
				template <StormByte::Type::Integral T>
				NamedValue operator[](T index) && {
					return std::move(*this)[StormByte::Size{index}];
				}

				/**
				 * @brief Append a named column.
				 * @param columnName Column name.
				 * @param value Value to store.
				 */
				void add(std::string_view columnName, Value &&value);

				/**
				 * @brief Append a named value.
				 * @param value Named value to store.
				 */
				void add(NamedValue value);

				/**
				 * @brief Search for an equal named value.
				 * @param value Value to find.
				 * @return Whether the value exists in this row.
				 */
				bool has_item(const NamedValue &value) const;

				/**
				 * @brief Number of columns.
				 * @return Count.
				 */
				StormByte::Size Count() const noexcept;

			private:
				/**
				 * @class Columns
				 * @brief Private row storage and column-name index.
				 */
				struct Columns;

				std::unique_ptr<Columns> m_columns;	///< Opaque storage owned by the Database DLL

				/**
				 * @brief Build the name index if missing.
				 */
				void BuildNameIndex() const;
		};
	}
}

/** @brief Conditional DLL safety requires a compatible ABI and live Database module; storage operations remain in Database. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Row);
