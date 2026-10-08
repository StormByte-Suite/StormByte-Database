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

#include <StormByte/database/rows.hxx>

#include <algorithm>

using namespace StormByte::Database;

Rows::Rows() noexcept = default;

Rows::Rows(const Rows& other):
	m_results(other.m_results) {}

Rows::Rows(Rows&& other) noexcept = default;

Rows::~Rows() noexcept = default;

Rows& Rows::operator=(const Rows& other) {
	if (this != &other)
		m_results = other.m_results;
	return *this;
}

Rows& Rows::operator=(Rows&& other) noexcept = default;

bool Rows::operator==(const Rows& other) const {
	return m_results == other.m_results;
}

bool Rows::operator!=(const Rows& other) const {
	return !(*this == other);
}

Rows::iterator Rows::begin() noexcept {
	return m_results.empty() ? nullptr : m_results.data();
}

Rows::const_iterator Rows::begin() const noexcept {
	return m_results.empty() ? nullptr : m_results.data();
}

Rows::iterator Rows::end() noexcept {
	return m_results.empty() ? nullptr : m_results.data() + static_cast<std::size_t>(m_results.size());
}

Rows::const_iterator Rows::end() const noexcept {
	return m_results.empty() ? nullptr : m_results.data() + static_cast<std::size_t>(m_results.size());
}

Rows::const_iterator Rows::cbegin() const noexcept {
	return begin();
}

Rows::const_iterator Rows::cend() const noexcept {
	return end();
}

Rows::reverse_iterator Rows::rbegin() noexcept {
	return reverse_iterator(end());
}

Rows::reverse_iterator Rows::rend() noexcept {
	return reverse_iterator(begin());
}

Rows::const_reverse_iterator Rows::rbegin() const noexcept {
	return const_reverse_iterator(end());
}

Rows::const_reverse_iterator Rows::rend() const noexcept {
	return const_reverse_iterator(begin());
}

Rows::const_reverse_iterator Rows::crbegin() const noexcept {
	return rbegin();
}

Rows::const_reverse_iterator Rows::crend() const noexcept {
	return rend();
}

StormByte::Size Rows::size() const noexcept {
	return StormByte::Size{m_results.size()};
}

bool Rows::empty() const noexcept {
	return m_results.empty();
}

Row& Rows::operator[](StormByte::Size index) {
	if (index >= size())
		throw OutOfBounds(index, size());
	return m_results[static_cast<std::size_t>(index)];
}

const Row& Rows::operator[](StormByte::Size index) const {
	if (index >= size())
		throw OutOfBounds(index, size());
	return m_results[static_cast<std::size_t>(index)];
}

void Rows::add(const Row& row) {
	m_results.emplace_back(row);
}

void Rows::add(Row&& row) {
	m_results.emplace_back(std::move(row));
}

bool Rows::has_item(const Row& row) const {
	return std::find(m_results.begin(), m_results.end(), row) != m_results.end();
}

StormByte::Size Rows::Count() const noexcept {
	return size();
}
