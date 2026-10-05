/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Database.
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

Telemetry::Telemetry() noexcept:
	StormByte::Telemetry(),
	m_rows_returned(0) {
	for (auto& event : m_events)
		event.store(0, std::memory_order_relaxed);
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
	const std::uint64_t minimum = counter.minimum_nanoseconds.load(std::memory_order_acquire);
	const OperationMetrics metrics{
		attempts,
		counter.successes.load(std::memory_order_acquire),
		counter.failures.load(std::memory_order_acquire),
		static_cast<std::uint64_t>(total.count()),
		attempts == 0 || minimum == std::numeric_limits<std::uint64_t>::max() ? 0 : minimum,
		counter.maximum_nanoseconds.load(std::memory_order_acquire)
	};
	return metrics;
}

std::uint64_t Telemetry::RowsReturned() const noexcept {
	return m_rows_returned.load(std::memory_order_acquire);
}

std::uint64_t Telemetry::Events(const BackendEvent event) const noexcept {
	const auto index = static_cast<std::size_t>(event);
	return index < m_events.size() ? m_events[index].load(std::memory_order_acquire) : 0;
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
		m_events[index].fetch_add(count, std::memory_order_relaxed);
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
	(success ? counter.successes : counter.failures).fetch_add(1, std::memory_order_relaxed);
	std::uint64_t minimum = counter.minimum_nanoseconds.load(std::memory_order_relaxed);
	while (duration < minimum && !counter.minimum_nanoseconds.compare_exchange_weak(
		minimum, duration, std::memory_order_relaxed, std::memory_order_relaxed)) {}
	std::uint64_t maximum = counter.maximum_nanoseconds.load(std::memory_order_relaxed);
	while (duration > maximum && !counter.maximum_nanoseconds.compare_exchange_weak(
		maximum, duration, std::memory_order_relaxed, std::memory_order_relaxed)) {}
	if (success && rows_returned > 0)
		m_rows_returned.fetch_add(rows_returned, std::memory_order_relaxed);
}
