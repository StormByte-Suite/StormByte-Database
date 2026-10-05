/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Database.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-StormByte-Commercial
 */

#pragma once

#include <StormByte/database/visibility.h>
#include <StormByte/safe/pointers.hxx>
#include <StormByte/safe/string.hxx>
#include <StormByte/telemetry.hxx>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

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
		 * @enum Operation
		 * @brief Database operation categories measured by telemetry.
		 */
		enum class Operation : std::uint8_t {
			Connect,             ///< Connection attempts.
			Disconnect,          ///< Disconnect calls.
			Query,               ///< Queries that return rows.
			SilentQuery,         ///< Queries that return only success or failure.
			PrepareStatement,    ///< Prepared-statement registration attempts.
			PreparedStatement,   ///< Prepared-statement execution attempts.
			BeginTransaction,    ///< Transaction begin attempts.
			CommitTransaction,   ///< Transaction commit attempts.
			RollbackTransaction, ///< Explicit and automatic rollback attempts.
			Count                ///< Number of operation categories.
		};

		/**
		 * @enum BackendEvent
		 * @brief Shared error and warning categories for backend-specific telemetry.
		 */
		enum class BackendEvent : std::uint8_t {
			Warning,       ///< Non-fatal warning reported by a backend.
			Busy,          ///< Busy, locked, or lock-timeout condition.
			Constraint,    ///< Constraint violation.
			Io,            ///< Storage or transport I/O failure.
			Connection,    ///< Connection or protocol failure.
			Serialization, ///< Transaction serialization conflict.
			Deadlock,      ///< Deadlock victim or equivalent conflict.
			LockTimeout,   ///< Lock wait timed out.
			Other,         ///< Backend failure without a narrower category.
			Count          ///< Number of backend event categories.
		};

		/**
		 * @struct OperationMetrics
		 * @brief Immutable view of cumulative metrics for one operation category.
		 */
		struct OperationMetrics {
			std::uint64_t Attempts{};         ///< Number of recorded attempts.
			std::uint64_t Successes{};        ///< Number of successful attempts.
			std::uint64_t Failures{};         ///< Number of failed attempts.
			std::uint64_t TotalNanoseconds{}; ///< Sum of measured durations.
			std::uint64_t MinimumNanoseconds{}; ///< Minimum measured duration, or zero when empty.
			std::uint64_t MaximumNanoseconds{}; ///< Maximum measured duration.

			/**
			 * @brief Mean duration across attempts.
			 * @return Mean nanoseconds, or zero when no attempt was recorded.
			 */
			std::uint64_t MeanNanoseconds() const noexcept {
				return Attempts == 0 ? 0 : TotalNanoseconds / Attempts;
			}
		};

		/**
		 * @class Telemetry
		 * @brief Thread-safe cumulative telemetry for one database connection.
		 *
		 * The owning Database exposes a @ref StormByte::Safe::Shared handle. Callers may
		 * retain that handle after disconnect or Database destruction. Counters
		 * are snapshots and do not expose SQL text or bound values.
		 */
		class STORMBYTE_DATABASE_PUBLIC Telemetry : public StormByte::Telemetry {
			public:
				/**
				 * @class OperationScope
				 * @brief Records one operation and its elapsed time on destruction.
			 */
				class STORMBYTE_DATABASE_PUBLIC OperationScope {
					public:
						/**
						 * @brief Start measuring one operation.
					 * @param telemetry Telemetry object that receives the measurement.
					 * @param operation Operation category.
					 */
					OperationScope(StormByte::Safe::Shared<Telemetry> telemetry, Operation operation) noexcept;

					/**
					 * @brief Copy constructor is deleted.
					 */
					OperationScope(const OperationScope&) = delete;

					/**
					 * @brief Move constructor is deleted.
					 */
					OperationScope(OperationScope&&) = delete;

					/**
					 * @brief Record the result and optional returned row count.
					 * @param success Whether the operation succeeded.
					 * @param rows_returned Rows returned by this operation.
				 */
					void Complete(bool success, std::uint64_t rows_returned = 0) noexcept;

					/**
					 * @brief Record the attempt when the scope ends.
				 */
					~OperationScope() noexcept;

				private:
					StormByte::Safe::Shared<Telemetry> m_telemetry; ///< Keeps the measured telemetry object alive.
					Operation m_operation; ///< Operation category.
					StormByte::Clock::Sample m_sample; ///< Independent Base clock measurement for this operation.
					std::uint64_t m_rows_returned; ///< Rows reported by Complete().
					bool m_success; ///< Result reported by Complete().
					bool m_completed; ///< Whether Complete() has been called.
				};

				/**
				 * @brief Virtual destructor, defined out-of-line for the DLL boundary.
				 */
				virtual ~Telemetry() noexcept;

				/**
				 * @brief Construct zeroed counters.
				 */
				Telemetry() noexcept;

				/**
				 * @brief Obtain a thread-safe metrics snapshot for an operation.
				 * @param operation Operation category.
				 * @return Attempts, outcomes, and latency aggregates.
				 */
				OperationMetrics Metrics(Operation operation) const noexcept;

				/**
				 * @brief Total rows successfully returned by Query and prepared statements.
				 * @return Cumulative row count.
				 */
				std::uint64_t RowsReturned() const noexcept;

				/**
				 * @brief Number of backend events in a shared category.
				 * @param event Backend event category.
				 * @return Cumulative event count.
				 */
				std::uint64_t Events(BackendEvent event) const noexcept;

				/**
				 * @brief Flatten counters into an owned StormByte string.
			 * @return Human-readable telemetry snapshot.
			 */
				virtual operator StormByte::Safe::String() const override;

				/**
				 * @brief Flatten counters into a caller-owned standard string.
				 * @return Human-readable telemetry snapshot.
				 */
				STORMBYTE_FORCE_INLINE operator std::string() const {
						return static_cast<std::string>(static_cast<StormByte::Safe::String>(*this));
				}

			protected:
				Telemetry(const Telemetry&) = delete; ///< Copying counters is disabled.
				Telemetry(Telemetry&&) = delete; ///< Moving counters is disabled.
				Telemetry& operator=(const Telemetry&) = delete; ///< Copy assignment is disabled.
				Telemetry& operator=(Telemetry&&) = delete; ///< Move assignment is disabled.

				/**
				 * @brief Record a categorized backend event.
			 * @param event Event category.
			 */
				virtual void RecordEvent(BackendEvent event) noexcept;

				/**
				 * @brief Add a known number of occurrences for one backend event.
				 * @param event Event category.
				 * @param count Number of occurrences.
				 */
				void RecordEvents(BackendEvent event, std::uint64_t count) noexcept;

			private:
				friend class Database;
				friend class PreparedSTMT;
				friend class OperationScope;

				/**
				 * @struct Counter
				 * @brief Atomic aggregates for one operation category.
				 */
				struct Counter {
					std::atomic<std::uint64_t> successes{0}; ///< Success count.
					std::atomic<std::uint64_t> failures{0}; ///< Failure count.
					std::atomic<std::uint64_t> minimum_nanoseconds{std::numeric_limits<std::uint64_t>::max()}; ///< Minimum duration.
					std::atomic<std::uint64_t> maximum_nanoseconds{0}; ///< Maximum duration.
				};

				/**
				 * @brief Add one completed operation measurement.
				 * @param operation Operation category.
				 * @param success Whether it succeeded.
				 * @param elapsed Measured duration.
				 * @param rows_returned Successful result rows.
			 */
				void RecordOperation(Operation operation, bool success, std::chrono::nanoseconds elapsed, std::uint64_t rows_returned) noexcept;

				std::array<Counter, static_cast<std::size_t>(Operation::Count)> m_operations; ///< Per-operation aggregates.
				std::array<std::atomic<std::uint64_t>, static_cast<std::size_t>(BackendEvent::Count)> m_events; ///< Backend event counters.
				std::atomic<std::uint64_t> m_rows_returned; ///< Successful result rows.
		};
	}
}

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules; derived telemetry must preserve Safe ownership. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Telemetry);

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules until the measurement is recorded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Telemetry::OperationScope);
