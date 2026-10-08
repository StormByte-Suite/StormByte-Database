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
#include <StormByte/database/rows.hxx>
#include <StormByte/safe/optional.hxx>
#include <StormByte/safe/vector.hxx>

#include <string>

#if defined(STORMBYTE_TEST_SQLITE)
#include <StormByte/database/sqlite/sqlite3.hxx>
#endif

class ConsumerDatabase
#if defined(STORMBYTE_TEST_SQLITE)
	: public StormByte::Database::SQLite::SQLite3
#endif
{
#if defined(STORMBYTE_TEST_SQLITE)
	public:
		ConsumerDatabase() : SQLite3(StormByte::Safe::Shared<StormByte::Logger::Log>{}) {}
		ConsumerDatabase(const std::filesystem::path& path)
			: SQLite3(path, StormByte::Safe::Shared<StormByte::Logger::Log>{}) {}
		ConsumerDatabase(std::filesystem::path&& path)
			: SQLite3(std::move(path), StormByte::Safe::Shared<StormByte::Logger::Log>{}) {}
		void RegisterStatement(std::string_view name, std::string_view query) {
			DoPrepareSTMT(name, query);
		}
#endif
};

#if defined(STORMBYTE_TEST_SQLITE)
STORMBYTE_DECLARE_MAYBE_SAFE(ConsumerDatabase);
static_assert(StormByte::Type::MaybeSafe<StormByte::Database::SQLite::SQLite3>);
static_assert(StormByte::Type::SafeComponent<StormByte::Safe::Shared<ConsumerDatabase>>);
#endif

static_assert(StormByte::Type::MaybeSafe<StormByte::Database::Value>);
static_assert(StormByte::Type::SafeValue<StormByte::Database::Rows>);

int main() {
	StormByte::Database::Value value{42};
	if (value.Get<int>() != 42)
		return 1;
	StormByte::Safe::Optional<StormByte::Database::Value> stored{value};
	if (!stored || stored.value().Get<int>() != 42)
		return 8;

#if defined(STORMBYTE_TEST_SQLITE)
	ConsumerDatabase db;
	if (!db.Connect())
		return 2;
	const auto rows = db.Query("SELECT 42;");
	if (!rows || rows->Count() != 1 || rows->operator[](0)[0].Get<int>() != 42)
		return 3;
	const auto telemetry = db.Telemetry();
	if (!telemetry || telemetry->Metrics(StormByte::Database::Operation::Query).Successes != 1)
		return 4;
	const auto* sqlite_telemetry = dynamic_cast<const StormByte::Database::SQLite::Telemetry*>(telemetry.get());
	if (!sqlite_telemetry || static_cast<std::string>(*telemetry).find("SQLite{") == std::string::npos)
		return 5;
	if (sqlite_telemetry->Metrics(StormByte::Database::Operation::Query).Attempts != 1)
		return 6;
	auto transaction = db.BeginTransaction();
	if (!transaction)
		return 7;
	transaction->Rollback();
	StormByte::Safe::Vector<StormByte::Database::Rows> snapshots{*rows};
	const StormByte::Database::Rows snapshot = snapshots[0];
	if (snapshots.size() != 1 || snapshot[0][0].Get<int>() != 42)
		return 9;
	const std::filesystem::path borrowed_path{":memory:"};
	ConsumerDatabase borrowed{borrowed_path};
	if (!borrowed.Connect() || borrowed_path != ":memory:")
		return 10;
	ConsumerDatabase temporary{std::filesystem::path{":memory:"}};
	if (!temporary.Connect())
		return 11;
	constexpr std::string_view text{"left\0right", 10};
	constexpr std::string_view name{"echo\0suffix", 11};
	db.RegisterStatement(name, "SELECT ?;");
	const auto full_text = db.ExecuteSTMT(name, StormByte::Safe::String{text});
	if (!full_text || static_cast<std::string_view>((*full_text)[0][0].Get<StormByte::Safe::String>()) != text)
		return 12;
	StormByte::Safe::Vector<StormByte::Database::Rows> text_snapshots{*full_text};
	const StormByte::Database::Rows text_snapshot = text_snapshots[0];
	if (static_cast<std::string_view>(text_snapshot[0][0].Get<StormByte::Safe::String>()) != text)
		return 13;
#endif
	return 0;
}
