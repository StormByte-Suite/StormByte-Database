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

#include <StormByte/database/visibility.h>
#include <StormByte/exception.hxx>
#include <StormByte/safe/string.hxx>
#include <StormByte/size.hxx>

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
		 * @class Exception
		 * @brief Root exception for Database. `what()` is `StormByte.Database: message`.
		 *
		 * This type applies the `Database` segment. A further segment is an @ref Path. The body does not include the path. Literal `{}` are replaced by @ref StormByte::Safe::String::ReplaceToken. This does not call `std::format`. The destructor is defined in this module.
		 */
		class STORMBYTE_DATABASE_PUBLIC Exception: public StormByte::Exception {
			public:
				/**
				 * @struct Path
				 * @brief Additional path segments nested below `StormByte.Database`.
				 */
				struct Path {
					std::string_view segments; ///< Additional dotted path segments.

					/**
					 * @brief Wraps additional path segments.
					 * @param segments Dotted segments appended below `Database`.
					 */
					explicit constexpr Path(std::string_view segments) noexcept: segments(segments) {}
				};

				/**
				 * @brief Replace literal `{}` under `StormByte.Database`.
				 * @tparam Args Views, numbers, or types that convert to `std::string`. A number uses `std::to_string`.
				 * @param pattern Body with literal `{}` tokens. Not a `std::format` string.
				 * @param args Replacements. One per token, in order.
				 */
				template <typename... Args>
					requires (sizeof...(Args) >= 1)
				Exception(std::string_view pattern, Args&&... args)
					: StormByte::Exception(StormByte::Exception::Path{"Database"}, pattern, std::forward<Args>(args)...) {}

				/**
				 * @brief Copy a plain message under `StormByte.Database`.
				 * @param message Exception text. Not a format string.
				 */
				explicit Exception(std::string_view message);

				/**
				 * @brief Plain message under an additional Database path.
				 * @param path Additional segments nested below `StormByte.Database`.
				 * @param message Exception text. Not a format string.
				 */
				Exception(Path path, std::string_view message);

				/**
				 * @brief Replace literal `{}` under an additional Database path.
				 * @tparam Args Views, numbers, or types that convert to `std::string`. A number uses `std::to_string`.
				 * @param path Additional segments nested below `StormByte.Database`.
				 * @param pattern Body with literal `{}` tokens. Not a `std::format` string.
				 * @param args Replacements. One per token, in order.
				 */
				template <typename... Args>
					requires (sizeof...(Args) >= 1)
				Exception(Path path, std::string_view pattern, Args&&... args)
					: Exception(path, static_cast<std::string_view>(StormByte::Safe::String::ReplaceToken(pattern, std::forward<Args>(args)...))) {}

				/**
				 * @brief Copy constructor.
				 * @param other Exception to copy.
				 */
				Exception(const Exception& other) = default;

				/**
				 * @brief Move constructor.
				 * @param other Exception to move.
				 */
				Exception(Exception&& other) noexcept = default;

				/**
				 * @brief Destructor. Defined in this module so `catch` matches across a DLL.
				 */
				~Exception() noexcept override;

				/**
				 * @brief Copy assignment operator.
				 * @param other Exception to copy.
				 * @return This exception.
				 */
				Exception& operator=(const Exception& other) = default;

				/**
				 * @brief Move assignment operator.
				 * @param other Exception to move.
				 * @return This exception.
				 */
				Exception& operator=(Exception&& other) noexcept = default;

			protected:
				/**
				 * @brief Join a fixed prefix and a dynamic segment.
				 * @param prefix Fixed text, including a trailing dot when the segment follows.
				 * @param component Dynamic segment.
				 * @return Owned text. The view it converts to lives for the calling expression.
				 */
				static StormByte::Safe::String Segment(std::string_view prefix, std::string_view component) {
					StormByte::Safe::String path{prefix};
					path.append(component);
					return path;
				}
		};

		/**
		 * @class ConnectionError
		 * @brief Connection failed. `what()` is `StormByte.Database.Connection: message`.
		 */
		class STORMBYTE_DATABASE_PUBLIC ConnectionError final: public Exception {
			public:
				/**
				 * @brief Construct from a backend message.
				 * @param error Error text. Not a format string.
				 */
				explicit ConnectionError(std::string_view error);

				/**
				 * @brief Copy constructor.
				 * @param other Exception to copy.
				 */
				ConnectionError(const ConnectionError& other) = default;

				/**
				 * @brief Move constructor.
				 * @param other Exception to move.
				 */
				ConnectionError(ConnectionError&& other) noexcept = default;

				/**
				 * @brief Destructor. Defined in this module so `catch` matches across a DLL.
				 */
				~ConnectionError() noexcept override;

				/**
				 * @brief Copy assignment operator.
				 * @param other Exception to copy.
				 * @return This exception.
				 */
				ConnectionError& operator=(const ConnectionError& other) = default;

				/**
				 * @brief Move assignment operator.
				 * @param other Exception to move.
				 * @return This exception.
				 */
				ConnectionError& operator=(ConnectionError&& other) noexcept = default;
		};

		/**
		 * @class WrongValueType
		 * @brief Value accessed as the wrong type. `what()` is `StormByte.Database.WrongValueType: message`.
		 */
		class STORMBYTE_DATABASE_PUBLIC WrongValueType final: public Exception {
			public:
				/**
				 * @brief Construct from an error message.
				 * @param error Error text. Not a format string.
				 */
				explicit WrongValueType(std::string_view error);

				/**
				 * @brief Replace literal `{}` under `StormByte.Database.WrongValueType.<component>`.
				 * @tparam Args Views, numbers, or types that convert to `std::string`. A number uses `std::to_string`.
				 * @param component Context label appended after `WrongValueType`.
				 * @param pattern Body with literal `{}` tokens. Not a `std::format` string.
				 * @param args Replacements. One per token, in order.
				 */
				template <typename... Args>
					requires (sizeof...(Args) >= 1)
				WrongValueType(std::string_view component, std::string_view pattern, Args&&... args)
					: Exception(Path{Segment("WrongValueType.", component)}, pattern, std::forward<Args>(args)...) {}

				/**
				 * @brief Copy constructor.
				 * @param other Exception to copy.
				 */
				WrongValueType(const WrongValueType& other) = default;

				/**
				 * @brief Move constructor.
				 * @param other Exception to move.
				 */
				WrongValueType(WrongValueType&& other) noexcept = default;

				/**
				 * @brief Destructor. Defined in this module so `catch` matches across a DLL.
				 */
				~WrongValueType() noexcept override;

				/**
				 * @brief Copy assignment operator.
				 * @param other Exception to copy.
				 * @return This exception.
				 */
				WrongValueType& operator=(const WrongValueType& other) = default;

				/**
				 * @brief Move assignment operator.
				 * @param other Exception to move.
				 * @return This exception.
				 */
				WrongValueType& operator=(WrongValueType&& other) noexcept = default;
		};

		/**
		 * @class ColumnNotFound
		 * @brief Column name missing from a Row. `what()` is `StormByte.Database.ColumnNotFound: Column '<name>' not found`.
		 */
		class STORMBYTE_DATABASE_PUBLIC ColumnNotFound: public Exception {
			public:
				/**
				 * @brief Construct from the missing name.
				 * @param column Column name.
				 */
				explicit ColumnNotFound(std::string_view column);

				/**
				 * @brief Copy constructor.
				 * @param other Exception to copy.
				 */
				ColumnNotFound(const ColumnNotFound& other) = default;

				/**
				 * @brief Move constructor.
				 * @param other Exception to move.
				 */
				ColumnNotFound(ColumnNotFound&& other) noexcept = default;

				/**
				 * @brief Destructor. Defined in this module so `catch` matches across a DLL.
				 */
				~ColumnNotFound() noexcept override;

				/**
				 * @brief Copy assignment operator.
				 * @param other Exception to copy.
				 * @return This exception.
				 */
				ColumnNotFound& operator=(const ColumnNotFound& other) = default;

				/**
				 * @brief Move assignment operator.
				 * @param other Exception to move.
				 * @return This exception.
				 */
				ColumnNotFound& operator=(ColumnNotFound&& other) noexcept = default;
		};

		/**
		 * @class OutOfBounds
		 * @brief Column index out of range. `what()` is `StormByte.Database.OutOfBounds: Position <n> is out of bounds for size <n>`.
		 */
		class STORMBYTE_DATABASE_PUBLIC OutOfBounds: public Exception {
			public:
				/**
				 * @brief Construct from index and size.
				 * @param pos Requested index.
				 * @param size Container size.
				 */
				OutOfBounds(StormByte::Size pos, StormByte::Size size);

				/**
				 * @brief Copy constructor.
				 * @param other Exception to copy.
				 */
				OutOfBounds(const OutOfBounds& other) = default;

				/**
				 * @brief Move constructor.
				 * @param other Exception to move.
				 */
				OutOfBounds(OutOfBounds&& other) noexcept = default;

				/**
				 * @brief Destructor. Defined in this module so `catch` matches across a DLL.
				 */
				~OutOfBounds() noexcept override;

				/**
				 * @brief Copy assignment operator.
				 * @param other Exception to copy.
				 * @return This exception.
				 */
				OutOfBounds& operator=(const OutOfBounds& other) = default;

				/**
				 * @brief Move assignment operator.
				 * @param other Exception to move.
				 * @return This exception.
				 */
				OutOfBounds& operator=(OutOfBounds&& other) noexcept = default;
		};

		/**
		 * @class QueryException
		 * @brief Base for query errors. `what()` is `StormByte.Database.Query: message`.
		 */
		class STORMBYTE_DATABASE_PUBLIC QueryException: public Exception {
			public:
				/**
				 * @brief Construct from an error message.
				 * @param error Error text. Not a format string.
				 */
				explicit QueryException(std::string_view error);

				/**
				 * @brief Replace literal `{}` under `StormByte.Database.Query.<component>`.
				 * @tparam Args Views, numbers, or types that convert to `std::string`. A number uses `std::to_string`.
				 * @param component Subsystem name appended after `Query`.
				 * @param pattern Body with literal `{}` tokens. Not a `std::format` string.
				 * @param args Replacements. One per token, in order.
				 */
				template <typename... Args>
					requires (sizeof...(Args) >= 1)
				QueryException(std::string_view component, std::string_view pattern, Args&&... args)
					: Exception(Path{Segment("Query.", component)}, pattern, std::forward<Args>(args)...) {}

				/**
				 * @brief Copy constructor.
				 * @param other Exception to copy.
				 */
				QueryException(const QueryException& other) = default;

				/**
				 * @brief Move constructor.
				 * @param other Exception to move.
				 */
				QueryException(QueryException&& other) noexcept = default;

				/**
				 * @brief Destructor. Defined in this module so `catch` matches across a DLL.
				 */
				~QueryException() noexcept override;

				/**
				 * @brief Copy assignment operator.
				 * @param other Exception to copy.
				 * @return This exception.
				 */
				QueryException& operator=(const QueryException& other) = default;

				/**
				 * @brief Move assignment operator.
				 * @param other Exception to move.
				 * @return This exception.
				 */
				QueryException& operator=(QueryException&& other) noexcept = default;
		};

		/**
		 * @class UnknownSTMT
		 * @brief Prepared statement name is not registered. `what()` is `StormByte.Database.Query.PreparedSTMT: Statement '<name>' not found`.
		 */
		class STORMBYTE_DATABASE_PUBLIC UnknownSTMT: public QueryException {
			public:
				/**
				 * @brief Construct from the statement name.
				 * @param name Statement name.
				 */
				explicit UnknownSTMT(std::string_view name);

				/**
				 * @brief Copy constructor.
				 * @param other Exception to copy.
				 */
				UnknownSTMT(const UnknownSTMT& other) = default;

				/**
				 * @brief Move constructor.
				 * @param other Exception to move.
				 */
				UnknownSTMT(UnknownSTMT&& other) noexcept = default;

				/**
				 * @brief Destructor. Defined in this module so `catch` matches across a DLL.
				 */
				~UnknownSTMT() noexcept override;

				/**
				 * @brief Copy assignment operator.
				 * @param other Exception to copy.
				 * @return This exception.
				 */
				UnknownSTMT& operator=(const UnknownSTMT& other) = default;

				/**
				 * @brief Move assignment operator.
				 * @param other Exception to move.
				 * @return This exception.
				 */
				UnknownSTMT& operator=(UnknownSTMT&& other) noexcept = default;
		};

		/**
		 * @class ExecuteError
		 * @brief Query or statement execution failed. `what()` is `StormByte.Database.Query.Execute: Error executing query: <message>`.
		 */
		class STORMBYTE_DATABASE_PUBLIC ExecuteError: public QueryException {
			public:
				/**
				 * @brief Construct from a backend message.
				 * @param error Error text.
				 */
				explicit ExecuteError(std::string_view error);

				/**
				 * @brief Copy constructor.
				 * @param other Exception to copy.
				 */
				ExecuteError(const ExecuteError& other) = default;

				/**
				 * @brief Move constructor.
				 * @param other Exception to move.
				 */
				ExecuteError(ExecuteError&& other) noexcept = default;

				/**
				 * @brief Destructor. Defined in this module so `catch` matches across a DLL.
				 */
				~ExecuteError() noexcept override;

				/**
				 * @brief Copy assignment operator.
				 * @param other Exception to copy.
				 * @return This exception.
				 */
				ExecuteError& operator=(const ExecuteError& other) = default;

				/**
				 * @brief Move assignment operator.
				 * @param other Exception to move.
				 * @return This exception.
				 */
				ExecuteError& operator=(ExecuteError&& other) noexcept = default;
		};

		/**
		 * @class TransactionError
		 * @brief Beginning a database transaction failed. `what()` is `StormByte.Database.Transaction: Unable to begin transaction: <message>`.
		 */
		class STORMBYTE_DATABASE_PUBLIC TransactionError final: public Exception {
			public:
				/**
				 * @brief Construct from the failure reason.
				 * @param error Error text.
				 */
				explicit TransactionError(std::string_view error);

				/**
				 * @brief Copy constructor.
				 * @param other Exception to copy.
				 */
				TransactionError(const TransactionError& other) = default;

				/**
				 * @brief Move constructor.
				 * @param other Exception to move.
				 */
				TransactionError(TransactionError&& other) noexcept = default;

				/**
				 * @brief Destructor. Defined in this module so `catch` matches across a DLL.
				 */
				~TransactionError() noexcept override;

				/**
				 * @brief Copy assignment operator.
				 * @param other Exception to copy.
				 * @return This exception.
				 */
				TransactionError& operator=(const TransactionError& other) = default;

				/**
				 * @brief Move assignment operator.
				 * @param other Exception to move.
				 * @return This exception.
				 */
				TransactionError& operator=(TransactionError&& other) noexcept = default;
		};
	}
}

/** @brief Conditional DLL safety requires compatible ABIs and the Database provider module to remain loaded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Exception);

/** @brief Conditional DLL safety requires compatible ABIs and the Database provider module to remain loaded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::ConnectionError);

/** @brief Conditional DLL safety requires compatible ABIs and the Database provider module to remain loaded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::WrongValueType);

/** @brief Conditional DLL safety requires compatible ABIs and the Database provider module to remain loaded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::ColumnNotFound);

/** @brief Conditional DLL safety requires compatible ABIs and the Database provider module to remain loaded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::OutOfBounds);

/** @brief Conditional DLL safety requires compatible ABIs and the Database provider module to remain loaded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::QueryException);

/** @brief Conditional DLL safety requires compatible ABIs and the Database provider module to remain loaded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::UnknownSTMT);

/** @brief Conditional DLL safety requires compatible ABIs and the Database provider module to remain loaded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::ExecuteError);

/** @brief Conditional DLL safety requires compatible ABIs and the Database provider module to remain loaded. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::TransactionError);
