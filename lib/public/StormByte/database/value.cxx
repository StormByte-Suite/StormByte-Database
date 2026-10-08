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

#include <StormByte/database/value.hxx>

using namespace StormByte::Database;

Value::Value(std::string_view value):
	m_value(StormByte::Safe::String{value}), m_type(Type::Text) {}

Value::Value(const StormByte::Safe::Binary& value):
	m_value(value), m_type(Type::Blob) {}

Value::Value(StormByte::Safe::Binary&& value):
	m_value(std::move(value)), m_type(Type::Blob) {}

Value::Value(const Value& other):
	m_value(other.m_value), m_type(other.m_type) {}

Value::Value(Value&& other) noexcept:
	m_value(std::move(other.m_value)), m_type(other.m_type) {
	other.m_type = Type::Null;
}

Value& Value::operator=(const Value& other) {
	if (this != &other) {
		m_value = other.m_value;
		m_type = other.m_type;
	}
	return *this;
}

Value& Value::operator=(Value&& other) noexcept {
	if (this != &other) {
		m_value = std::move(other.m_value);
		m_type = other.m_type;
		other.m_type = Type::Null;
	}
	return *this;
}

Value::~Value() noexcept = default;
