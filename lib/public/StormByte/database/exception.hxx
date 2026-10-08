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

#include <format>
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
		 * @brief Base exception for the Database module.
		 */
		class STORMBYTE_DATABASE_PUBLIC Exception : public StormByte::Exception {
			public:
				/**
				 * @brief Construct with an unformatted message.
				 * @param message Message text.
				 */
				explicit Exception(std::string_view message);

				/**
				 * @brief Construct with a formatted message.
				 * @tparam Args Format argument types.
				 * @param fmt Format string.
				 * @param args Format arguments.
				 */
				template <typename... Args>
				Exception(std::format_string<Args...> fmt, Args &&...args) : StormByte::Exception(StormByte::Exception::Path{"Database"}, fmt, std::forward<Args>(args)...) {}

				/**
				 * @brief Destructor.
				 */
				virtual ~Exception() noexcept override;

			protected:
				/**
				 * @brief Construct with a qualified database path and formatted message.
				 * @tparam Args Format argument types.
				 * @param path Qualified component path.
				 * @param fmt Format string.
				 * @param args Format arguments.
				 */
				template <typename... Args>
				Exception(StormByte::Exception::Path path, std::format_string<Args...> fmt, Args &&...args) : StormByte::Exception(path, fmt, std::forward<Args>(args)...) {}

				/**
				 * @brief Build an owned component path for an exception.
				 * @param prefix Fixed Database exception prefix.
				 * @param component Dynamic exception component.
				 * @return Base-owned path text.
				 */
				static StormByte::Safe::String ComponentPath(std::string_view prefix, std::string_view component);
		};

		/**
		 * @class ConnectionError
		 * @brief Connection failed.
		 */
		class STORMBYTE_DATABASE_PUBLIC ConnectionError final : public Exception {
			public:
				/**
				 * @brief Construct from a backend message.
				 * @param error Error text.
				 */
				ConnectionError(std::string_view error);

				/**
				 * @brief Destructor.
				 */
				~ConnectionError() noexcept override;
		};

		/**
		 * @class WrongValueType
		 * @brief Value accessed as the wrong type.
		 */
		class STORMBYTE_DATABASE_PUBLIC WrongValueType final : public Exception {
			public:
				/**
				 * @brief Construct from an error message.
				 * @param error Error text.
				 */
				WrongValueType(std::string_view error);

				/**
				 * @brief Destructor.
				 */
				~WrongValueType() noexcept override;

				/**
				 * @brief Construct with a format string.
				 * @tparam Args Format argument types.
				 * @param component Context label.
				 * @param fmt Format string.
				 * @param args Format arguments.
				 */
				template <typename... Args>
				WrongValueType(std::string_view component, std::format_string<Args...> fmt, Args &&...args) : Exception(StormByte::Exception::Path{ComponentPath("Database.WrongValueType.", component)}, fmt, std::forward<Args>(args)...) {}
		};

		/**
		 * @class ColumnNotFound
		 * @brief Column name missing from a Row.
		 */
		class STORMBYTE_DATABASE_PUBLIC ColumnNotFound : public Exception {
			public:
				/**
				 * @brief Construct from the missing name.
				 * @param column Column name.
				 */
				ColumnNotFound(std::string_view column);

				/**
				 * @brief Destructor.
				 */
				~ColumnNotFound() noexcept override;
		};

		/**
		 * @class OutOfBounds
		 * @brief Column index out of range.
		 */
		class STORMBYTE_DATABASE_PUBLIC OutOfBounds : public Exception {
			public:
				/**
				 * @brief Construct from index and size.
				 * @param pos Requested index.
				 * @param size Container size.
				 */
				OutOfBounds(StormByte::Size pos, StormByte::Size size);

				/**
				 * @brief Destructor.
				 */
				~OutOfBounds() noexcept override;
		};

		/**
		 * @class QueryException
		 * @brief Base for query errors.
		 */
		class STORMBYTE_DATABASE_PUBLIC QueryException : public Exception {
			public:
				/**
				 * @brief Construct from an error message.
				 * @param error Error text.
				 */
				QueryException(std::string_view error);

				/**
				 * @brief Destructor.
				 */
				~QueryException() noexcept override;

				/**
				 * @brief Construct with a query subsystem prefix.
				 * @tparam Args Format argument types.
				 * @param component Subsystem name.
				 * @param fmt Format string.
				 * @param args Format arguments.
				 */
				template <typename... Args>
				QueryException(std::string_view component, std::format_string<Args...> fmt, Args &&...args) : Exception(StormByte::Exception::Path{ComponentPath("Database.Query.", component)}, fmt, std::forward<Args>(args)...) {}
		};

		/**
		 * @class UnknownSTMT
		 * @brief Prepared statement name is not registered.
		 */
		class STORMBYTE_DATABASE_PUBLIC UnknownSTMT : public QueryException {
			public:
				/**
				 * @brief Construct from the statement name.
				 * @param name Statement name.
				 */
				UnknownSTMT(std::string_view name);

				/**
				 * @brief Destructor.
				 */
				~UnknownSTMT() noexcept override;
		};

		/**
		 * @class ExecuteError
		 * @brief Query or statement execution failed.
		 */
		class STORMBYTE_DATABASE_PUBLIC ExecuteError : public QueryException {
			public:
				/**
				 * @brief Construct from a backend message.
				 * @param error Error text.
				 */
				ExecuteError(std::string_view error);

				/**
				 * @brief Destructor.
				 */
				~ExecuteError() noexcept override;
		};

		/**
		 * @class TransactionError
		 * @brief Beginning a database transaction failed.
		 */
		class STORMBYTE_DATABASE_PUBLIC TransactionError final : public Exception {
			public:
				/**
				 * @brief Construct from the failure reason.
				 * @param error Error text.
				 */
				TransactionError(std::string_view error);

				/**
				 * @brief Destructor.
				 */
				~TransactionError() noexcept override;
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
