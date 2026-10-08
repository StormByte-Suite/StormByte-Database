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

#include <StormByte/database/telemetry.hxx>

#include <algorithm>
#include <string_view>

using namespace StormByte::Database;

namespace {
	constexpr std::array<std::string_view, static_cast<std::size_t>(Operation::Count)> operation_names{
		"Connect", "Disconnect", "Query", "SilentQuery", "PrepareStatement",
		"PreparedStatement", "BeginTransaction", "CommitTransaction", "RollbackTransaction"
	};

	constexpr std::array<std::string_view, static_cast<std::size_t>(BackendEvent::Count)> event_names{
		"Warnings", "BusyErrors", "ConstraintErrors", "IoErrors", "ConnectionErrors",
		"SerializationConflicts", "Deadlocks", "LockTimeouts", "OtherBackendErrors"
	};

	void Append(std::string& output, const std::string_view name, const std::uint64_t value) {
		if (!output.empty())
			output.push_back(' ');
		output.append(name);
		output.push_back('=');
		output += std::to_string(value);
	}
}

Telemetry::Telemetry():
	StormByte::Telemetry(),
	m_rows_returned(0) {
	for (auto& event : m_events)
		event.store(std::uint64_t{0}, StormByte::Safe::MemoryOrder::Relaxed);
}

Telemetry::~Telemetry() noexcept = default;

Telemetry::OperationScope::OperationScope(StormByte::Safe::Shared<Telemetry> telemetry, const Operation operation) noexcept:
	m_telemetry(std::move(telemetry)), m_operation(operation),
	m_rows_returned(0), m_success(false), m_completed(false) {
	const auto index = static_cast<std::size_t>(operation);
	if (!m_telemetry || index >= operation_names.size())
		return;
	m_sample = m_telemetry->MeasureClock(operation_names[index]);
}

void Telemetry::OperationScope::Complete(const bool success, const std::uint64_t rows_returned) noexcept {
	if (m_completed)
		return;
	m_success = success;
	m_rows_returned = rows_returned;
	m_completed = true;
}

Telemetry::OperationScope::~OperationScope() noexcept {
	if (!m_telemetry)
		return;
	if (m_sample.Active()) {
		const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(m_sample.Stop());
		m_telemetry->RecordOperation(m_operation, m_success, elapsed, m_rows_returned);
	}
}

OperationMetrics Telemetry::Metrics(const Operation operation) const noexcept {
	const auto index = static_cast<std::size_t>(operation);
	if (index >= m_operations.size())
		return {};
	const StormByte::Clock& clock = StormByte::Telemetry::Clock(operation_names[index]);
	const auto values = clock.GetValues();
	const std::uint64_t attempts = values.Count;
	const auto total = std::chrono::duration_cast<std::chrono::nanoseconds>(values.Time);
	const Counter& counter = m_operations[index];
	const std::uint64_t minimum = counter.minimum_nanoseconds.load(StormByte::Safe::MemoryOrder::Acquire);
	const OperationMetrics metrics{
		attempts,
		counter.successes.load(StormByte::Safe::MemoryOrder::Acquire),
		counter.failures.load(StormByte::Safe::MemoryOrder::Acquire),
		static_cast<std::uint64_t>(total.count()),
		attempts == 0 || minimum == std::numeric_limits<std::uint64_t>::max() ? 0 : minimum,
		counter.maximum_nanoseconds.load(StormByte::Safe::MemoryOrder::Acquire)
	};
	return metrics;
}

std::uint64_t Telemetry::RowsReturned() const noexcept {
	return m_rows_returned.load(StormByte::Safe::MemoryOrder::Acquire);
}

std::uint64_t Telemetry::Events(const BackendEvent event) const noexcept {
	const auto index = static_cast<std::size_t>(event);
	return index < m_events.size() ? m_events[index].load(StormByte::Safe::MemoryOrder::Acquire) : 0;
}

Telemetry::operator StormByte::Safe::String() const {
	std::string output;
	for (std::size_t index{}; index < operation_names.size(); ++index) {
		const OperationMetrics metrics = Metrics(static_cast<Operation>(index));
		if (metrics.Attempts == 0)
			continue;
		if (!output.empty())
			output.push_back(' ');
		output.append(operation_names[index]);
		output += "{calls=";
		output += std::to_string(metrics.Attempts);
		output += ",ok=";
		output += std::to_string(metrics.Successes);
		output += ",failed=";
		output += std::to_string(metrics.Failures);
		output += ",mean_ns=";
		output += std::to_string(metrics.MeanNanoseconds());
		output += ",min_ns=";
		output += std::to_string(metrics.MinimumNanoseconds);
		output += ",max_ns=";
		output += std::to_string(metrics.MaximumNanoseconds);
		output.push_back('}');
	}
	Append(output, "RowsReturned", RowsReturned());
	for (std::size_t index{}; index < event_names.size(); ++index) {
		const std::uint64_t count = Events(static_cast<BackendEvent>(index));
		if (count > 0)
			Append(output, event_names[index], count);
	}
	return StormByte::Safe::String(std::string_view{output});
}

void Telemetry::RecordEvent(const BackendEvent event) noexcept {
	RecordEvents(event, 1);
}

void Telemetry::RecordEvents(const BackendEvent event, const std::uint64_t count) noexcept {
	const auto index = static_cast<std::size_t>(event);
	if (index < m_events.size())
		m_events[index].fetch_add(count, StormByte::Safe::MemoryOrder::Relaxed);
}

void Telemetry::RecordOperation(const Operation operation, const bool success,
		const std::chrono::nanoseconds elapsed, const std::uint64_t rows_returned) noexcept {
	const auto index = static_cast<std::size_t>(operation);
	if (index >= m_operations.size())
		return;
	const std::uint64_t duration = elapsed.count() > 0
		? static_cast<std::uint64_t>(elapsed.count())
		: 0;
	Counter& counter = m_operations[index];
	(success ? counter.successes : counter.failures).fetch_add(std::uint64_t{1}, StormByte::Safe::MemoryOrder::Relaxed);
	std::uint64_t minimum = counter.minimum_nanoseconds.load(StormByte::Safe::MemoryOrder::Relaxed);
	while (duration < minimum && !counter.minimum_nanoseconds.compare_exchange_weak(
		minimum, duration, StormByte::Safe::MemoryOrder::Relaxed, StormByte::Safe::MemoryOrder::Relaxed)) {}
	std::uint64_t maximum = counter.maximum_nanoseconds.load(StormByte::Safe::MemoryOrder::Relaxed);
	while (duration > maximum && !counter.maximum_nanoseconds.compare_exchange_weak(
		maximum, duration, StormByte::Safe::MemoryOrder::Relaxed, StormByte::Safe::MemoryOrder::Relaxed)) {}
	if (success && rows_returned > 0)
		m_rows_returned.fetch_add(rows_returned, StormByte::Safe::MemoryOrder::Relaxed);
}
