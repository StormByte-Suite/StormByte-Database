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

#include <StormByte/database/prepared_stmt.hxx>
#include <StormByte/database/rows.hxx>
#include <StormByte/database/telemetry.hxx>
#include <StormByte/database/transaction.hxx>
#include <StormByte/database/typedefs.hxx>
#include <StormByte/logger/log.hxx>
#include <StormByte/safe/pointers.hxx>
#include <StormByte/size.hxx>

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
		 * @class Database
		 * @brief Abstract backend.
		 *
		 * @note Operations on one connection are serialized. A Transaction reserves its connection and must remain on its creating thread.
		 * @note Custom backends must use @ref OperationGuard in public operations that access backend state.
		 * @note Inheritance-oriented. Concrete backends expose protected constructors. Derive, call the backend constructor, override hooks if needed.
		 */
		class STORMBYTE_DATABASE_PUBLIC Database {
			private:
				/**
				 * @class OperationMutex
				 * @brief Private reentrant operation gate defined in the Database module.
				 */
				class OperationMutex;

			protected:
				/**
				 * @class OperationGuard
				 * @brief RAII ownership of the connection operation gate.
				 */
				class OperationGuard final {
					public:
						/**
						 * @brief Acquire the connection operation gate.
						 * @param database Database whose operations are serialized.
						 */
						explicit OperationGuard(const Database& database) noexcept;

						/**
						 * @brief Copying a lock guard is disabled.
						 * @param other Source guard.
						 */
						OperationGuard(const OperationGuard& other) = delete;

						/**
						 * @brief Release the connection operation gate.
						 */
						~OperationGuard() noexcept;

					private:
						const Database& m_database; ///< Database whose gate is held.
				};

			public:
				/**
				 * @brief Construct with an optional logger.
				 * @param logger Logger instance (may be null).
				 */
				Database(const StormByte::Safe::Shared<Logger::Log>& logger);

				/**
				 * @brief Copy constructor (deleted).
				 */
				Database(const Database &) = delete;

				/**
				 * @brief Move constructor.
				 */
				Database(Database &&other) noexcept;

				/**
				 * @brief Copy assignment (deleted).
				 */
				Database &operator=(const Database &) = delete;

				/**
				 * @brief Move assignment.
				 */
				Database &operator=(Database &&other) noexcept;

				/**
				 * @brief Destructor.
				 */
				virtual ~Database() noexcept;

				/**
				 * @brief Obtain the cumulative connection telemetry handle.
				 * @return Shared counters that remain valid after close or Database destruction.
				 */
				StormByte::Safe::Shared<class Telemetry> Telemetry() const noexcept;

				/**
				 * @brief Connect.
				 * @return true on success.
				 */
				bool Connect() noexcept;

				/**
				 * @brief Disconnect.
				 * @note Has no effect when the database is already disconnected.
				 */
				void Disconnect() noexcept;

				/**
				 * @brief Whether the connection is open.
				 * @return true if connected.
				 */
				bool IsConnected() const noexcept {
					OperationGuard lock{*this};
					return m_connected;
				}

				/**
				 * @brief TLS policy for the next Connect(). Ignored by SQLite.
				 * @param mode Desired SSL mode.
				 */
				void SslMode(StormByte::Database::SslMode mode) noexcept {
					OperationGuard lock{*this};
					m_ssl_mode = mode;
				}

				/**
				 * @brief Current TLS policy.
				 * @return Mode.
				 */
				StormByte::Database::SslMode SslMode() const noexcept {
					OperationGuard lock{*this};
					return m_ssl_mode;
				}

				/**
				 * @brief Execute a prepared statement by name.
				 * @tparam Args Bind argument types.
				 * @param name Prepared statement name.
				 * @param args Positional values (0-based).
				 * @return Result rows or an error.
				 */
				template <typename... Args>
				ExpectedRows ExecuteSTMT(std::string_view name, Args &&...args) {
					auto telemetry = TrackOperation(Operation::PreparedStatement);
					OperationGuard lock{*this};
					PreparedSTMT *statement = FindPreparedSTMT(name);
					if (!statement)
						return Unexpected<UnknownSTMT>(name);
					ExpectedRows result = statement->Execute(std::forward<Args>(args)...);
					telemetry.Complete(result.has_value(), result ? static_cast<std::uint64_t>(result->Count()) : std::uint64_t{0});
					return result;
				}

				/**
				 * @brief Execute a query that returns rows.
				 * @param query SQL text.
				 * @return Result rows or an error.
				 */
				virtual ExpectedRows Query(std::string_view query) = 0;

				/**
				 * @brief Execute a query that does not return rows.
				 * @param query SQL text.
				 * @return true on success.
				 */
				virtual bool SilentQuery(std::string_view query) noexcept = 0;

				/**
				 * @brief Begin a transaction.
				 * @param level Isolation (backend-specific mapping).
				 * @return RAII Transaction (rollback on destruction if not committed).
				 */
				Expected<Transaction, TransactionError> BeginTransaction(IsolationLevel level = IsolationLevel::Default);

				/**
				 * @brief Commit the current transaction.
				 */
				void CommitTransaction();

				/**
				 * @brief Roll back the current transaction.
				 */
				void RollbackTransaction();

			protected:
				friend class Transaction;
				StormByte::Safe::Shared<OperationMutex> m_operation_mutex; ///< Base-heap owner of the private reentrant operation gate.
				StormByte::Safe::Shared<class Telemetry> m_telemetry; ///< Shared cumulative counters for this connection.

				/**
				 * @brief Replace the telemetry implementation, normally in a concrete backend constructor.
				 * @param telemetry Backend-specific telemetry allocated on Base's heap.
				 */
				void Telemetry(StormByte::Safe::Shared<class Telemetry> telemetry) noexcept;

				/**
				 * @brief Record a categorized event reported by the active backend.
				 * @param event Backend event category.
				 */
				void RecordBackendEvent(BackendEvent event) noexcept {
					m_telemetry->RecordEvent(event);
				}

				/**
				 * @brief Start timing a Database operation without invoking user callbacks.
				 * @param operation Operation category.
				 * @return Scope that records failure unless Complete() reports otherwise.
				 */
				StormByte::Database::Telemetry::OperationScope TrackOperation(Operation operation) const noexcept {
					return StormByte::Database::Telemetry::OperationScope{m_telemetry, operation};
				}

				bool m_connected;																 ///< Connection state
				StormByte::Database::SslMode m_ssl_mode;											 ///< TLS policy for network backends

				StormByte::Safe::Shared<Logger::Log> m_logger;	///< Shared logger, safe across the DLL boundary

				/**
				 * @brief Destroy all registered statements inside the Database module.
				 */
				void ClearPreparedSTMTs() noexcept;

				/**
				 * @name Lifecycle hooks
				 * Called by Connect() / Disconnect(). Prefer DoSilentQuery() and DoPrepareSTMT() from overrides.
				 * @{
				 */

				/**
				 * @brief Pre-connect hook. Default no-op.
				 */
				virtual void DoPreConnect() noexcept {}

				/**
				 * @brief Backend connect.
				 * @return true on success.
				 */
				virtual bool DoConnect() noexcept = 0;

				/**
				 * @brief Post-connect hook. Default no-op.
				 */
				virtual void DoPostConnect() noexcept {}

				/**
				 * @brief Pre-disconnect hook. Default no-op.
				 */
				virtual void DoPreDisconnect() noexcept {}

				/**
				 * @brief Backend disconnect.
				 */
				virtual void DoDisconnect() noexcept = 0;

				/**
				 * @brief Post-disconnect hook. Default no-op.
				 */
				virtual void DoPostDisconnect() noexcept {}

				/** @} */

				/**
				 * @brief Create a backend prepared statement.
				 * @param name Statement name.
				 * @param query SQL text.
				 * @return Statement or nullptr on failure.
				 */
				virtual StormByte::Safe::Unique<PreparedSTMT> CreatePreparedSTMT(std::string_view name, std::string_view query) noexcept = 0;

				/**
				 * @brief Register a prepared statement under @p name.
				 * @param name Statement name.
				 * @param query SQL text.
				 */
				void PrepareSTMT(std::string_view name, std::string_view query) noexcept;

				/**
				 * @brief Same as PrepareSTMT; kept for hook symmetry.
				 * @param name Statement name.
				 * @param query SQL text.
				 */
				void DoPrepareSTMT(std::string_view name, std::string_view query) noexcept;

				/**
				 * @brief Backend BEGIN with isolation.
				 * @param level Isolation level.
				 */
				virtual void DoBeginTransaction(IsolationLevel level) = 0;

				/**
				 * @brief Backend silent query.
				 * @param query SQL text.
				 * @return true on success.
				 */
				virtual bool DoSilentQuery(std::string_view query) noexcept = 0;

			private:
				friend class Transaction;

				/**
				 * @brief Acquire the private connection operation gate.
				 */
				void LockOperation() const noexcept;

				/**
				 * @brief Release the private connection operation gate.
				 */
				void UnlockOperation() const noexcept;

				/**
				 * @struct PreparedStatements
				 * @brief Opaque prepared-statement registry defined in the Database DLL.
				 */
				struct PreparedStatements;

				StormByte::Safe::Unique<PreparedStatements> m_prepared_stmts; ///< Base-heap owner of the opaque registry

				/**
				 * @brief Find a prepared statement by name.
				 * @param name Statement name.
				 * @return Statement or nullptr when absent.
				 */
				PreparedSTMT *FindPreparedSTMT(std::string_view name);
		};
	}
}

/** @brief Conditional DLL safety requires compatible ABIs and live provider modules; derived facades must preserve Safe ownership. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Database);
