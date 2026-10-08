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

#include <StormByte/database/transaction.hxx>
#include <StormByte/database/database.hxx>
using namespace StormByte::Database;

Transaction::Transaction(Database& db)
	: m_db(&db), m_active(true), m_lock_held(false) {
	m_db->LockOperation();
	m_lock_held = true;
}
Transaction::Transaction(Transaction&& other) noexcept
	: m_db(other.m_db), m_active(other.m_active), m_lock_held(std::exchange(other.m_lock_held, false)) {
	other.m_db = nullptr;
	other.m_active = false;
}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
	if (this != &other) {
		if (m_active && m_db)
			Rollback();
		m_db = other.m_db;
		m_active = other.m_active;
		m_lock_held = std::exchange(other.m_lock_held, false);
		other.m_db = nullptr;
		other.m_active = false;
	}

	return *this;
}

Transaction::~Transaction() noexcept {
	if (m_active && m_db)
		m_db->RollbackTransaction();
	ReleaseLock();
}

void Transaction::ReleaseLock() noexcept {
	if (m_lock_held && m_db) {
		m_db->UnlockOperation();
		m_lock_held = false;
	}
}

void Transaction::Commit() {
	if (!m_active || !m_db)
		return;
	m_db->CommitTransaction();
	m_active = false;
	ReleaseLock();
}

void Transaction::Rollback() {
	if (!m_active || !m_db)
		return;
	m_db->RollbackTransaction();
	m_active = false;
	ReleaseLock();
}
