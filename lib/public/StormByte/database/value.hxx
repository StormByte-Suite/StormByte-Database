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

#include <StormByte/database/exception.hxx>
#include <StormByte/database/typedefs.hxx>
#include <StormByte/database/visibility.h>
#include <StormByte/safe/binary.hxx>
#include <StormByte/safe/variant.hxx>
#include <StormByte/type_traits.hxx>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>

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
		 * @class Value
		 * @brief Type-erased SQL value (NULL, integers, double, text, blob, bool).
		 */
		class STORMBYTE_DATABASE_PUBLIC Value {
			public:
				/**
				 * @enum Type
				 * @brief Discriminator for the stored alternative.
				 */
				enum class Type : unsigned short {
					Null = 0,			 ///< SQL NULL
					Integer,			 ///< int
					UnsignedInteger,	 ///< unsigned int
					LongInteger,		 ///< long long int (64-bit on every platform)
					UnsignedLongInteger, ///< unsigned long long int (64-bit on every platform)
					Double,				 ///< double
					Text,				 ///< StormByte::Safe::String
					Blob,				 ///< StormByte::Safe::Binary
					Boolean				 ///< bool
				};

				/**
				 * @name Construction
				 * @{
				 */
				/**
				 * @brief Default constructor. Stores SQL NULL.
				 */
				Value() : m_value(StormByte::Safe::Monostate{}), m_type(Type::Null) {}

				/**
				 * @brief From a signed int.
				 * @param value Stored value.
				 */
				Value(int value) : m_value(value), m_type(Type::Integer) {}

				/**
				 * @brief From an unsigned int.
				 * @param value Stored value.
				 */
				Value(unsigned int value) : m_value(value), m_type(Type::UnsignedInteger) {}

				/**
				 * @brief From a signed long int, stored as long long int.
				 * @param value Stored value.
				 */
				Value(long int value) : m_value(static_cast<long long int>(value)), m_type(Type::LongInteger) {}

				/**
				 * @brief From an unsigned long int, stored as unsigned long long int.
				 * @param value Stored value.
				 */
				Value(unsigned long int value) : m_value(static_cast<unsigned long long int>(value)), m_type(Type::UnsignedLongInteger) {}

				/**
				 * @brief From a signed long long int.
				 * @param value Stored value.
				 */
				Value(long long int value) : m_value(value), m_type(Type::LongInteger) {}

				/**
				 * @brief From an unsigned long long int.
				 * @param value Stored value.
				 */
				Value(unsigned long long int value) : m_value(value), m_type(Type::UnsignedLongInteger) {}

				/**
				 * @brief From a double.
				 * @param value Stored value.
				 */
				Value(double value) : m_value(value), m_type(Type::Double) {}

				/**
				 * @brief Copy UTF-8 text into a String-owned value.
				 * @param value Text to store.
				 */
				Value(std::string_view value);

				/**
				 * @brief Copy a blob into Base-owned storage.
				 * @param value Stored bytes.
				 */
				Value(const StormByte::Safe::Binary &value);

				/**
				 * @brief Move a blob into this value.
				 * @param value Stored bytes.
				 */
				Value(StormByte::Safe::Binary &&value);

				/**
				 * @brief From a bool.
				 * @param value Stored value.
				 */
				template <typename T>
					requires StormByte::Type::SameAs<std::remove_cvref_t<T>, bool>
					Value(T &&value) : m_value(value), m_type(Type::Boolean) {}
				/** @} */

				/**
				 * @brief Copy constructor.
				 */
				Value(const Value &other);

				/**
				 * @brief Move constructor.
				 */
				Value(Value &&other) noexcept;

				/**
				 * @brief Copy assignment.
				 */
				Value &operator=(const Value &other);

				/**
				 * @brief Move assignment.
				 */
				Value &operator=(Value &&other) noexcept;

				/**
				 * @brief Equality of the stored alternatives.
				 * @param other Other value.
				 * @return true if equal.
				 */
				inline bool operator==(const Value &other) const {
					if (m_type == Type::Null && other.m_type == Type::Null)
						return true;
					return m_value == other.m_value;
				}

				/**
				 * @brief Inequality.
				 * @param other Other value.
				 * @return true if not equal.
				 */
				inline bool operator!=(const Value &other) const {
					return !(*this == other);
				}

				/**
				 * @brief Destructor.
				 */
				virtual ~Value() noexcept;

				/**
				 * @brief Stored value as @p T, with safe numeric conversions.
				 * @tparam T Requested type (must be a ValuesVariant alternative).
				 * @return Converted value.
				 * @throws WrongValueType on mismatch or unsafe conversion.
				 */
				template <typename T>
					requires (ValuesVariant::AlternativeIndex<std::decay_t<T>>() != ValuesVariant::npos)
				std::decay_t<T> Get() const {
						if (m_value.valueless_by_exception())
							throw WrongValueType("Requested type does not match stored type (null).");
					using To = std::decay_t<T>;
						return StormByte::Safe::visit([](auto &&val) -> To {
						using From = std::decay_t<decltype(val)>;
							if constexpr (StormByte::Type::SameAs<From, StormByte::Safe::Monostate>) {
							throw WrongValueType("Requested type does not match stored type (null).");
						} else if constexpr (StormByte::Type::SameAs<From, To>) {
							return val;
						} else if constexpr (StormByte::Type::Arithmetic<From> && StormByte::Type::Arithmetic<To>) {
							return convert_numeric<To, From>(val);
						} else {
							throw WrongValueType("Requested type does not match stored type.");
						}
					},
									  m_value);
				}

				/**
				 * @brief Discriminator of the stored alternative.
				 * @return Type.
				 */
				inline Type Type() const noexcept {
					return m_type;
				}

				/**
				 * @brief Whether the value is SQL NULL.
				 * @return true if NULL.
				 */
				inline bool IsNull() const noexcept {
					return m_type == Type::Null;
				}

			private:
				/**
				 * @brief Safe numeric conversion between arithmetic types.
				 * @tparam To Destination type.
				 * @tparam From Source type.
				 * @param val Source value.
				 * @return Converted value.
				 * @throws WrongValueType on overflow, sign loss or a non-integral float.
				 */
				template <typename To, typename From>
					requires(StormByte::Type::Arithmetic<To> && StormByte::Type::Arithmetic<From>)
				static To convert_numeric(const From &val) {
					if constexpr (StormByte::Type::Integral<From> && StormByte::Type::Integral<To>) {
						if constexpr (StormByte::Type::Signed<From>) {
							std::intmax_t from = static_cast<std::intmax_t>(val);
							if constexpr (StormByte::Type::Signed<To>) {
								if (from < static_cast<std::intmax_t>(std::numeric_limits<To>::lowest()) || from > static_cast<std::intmax_t>(std::numeric_limits<To>::max()))
									throw WrongValueType("Integer conversion would overflow/narrow.");
								return static_cast<To>(from);
							} else {
								if (from < 0)
									throw WrongValueType("Negative value cannot be converted to unsigned.");
								if (static_cast<std::uintmax_t>(from) > static_cast<std::uintmax_t>(std::numeric_limits<To>::max()))
									throw WrongValueType("Integer conversion would overflow/narrow.");
								return static_cast<To>(from);
							}
						} else {
							std::uintmax_t from = static_cast<std::uintmax_t>(val);
							if constexpr (StormByte::Type::Signed<To>) {
								if (from > static_cast<std::uintmax_t>(std::numeric_limits<To>::max()))
									throw WrongValueType("Integer conversion would overflow/narrow.");
								return static_cast<To>(from);
							} else {
								if (from > static_cast<std::uintmax_t>(std::numeric_limits<To>::max()))
									throw WrongValueType("Integer conversion would overflow/narrow.");
								return static_cast<To>(from);
							}
						}
					} else if constexpr (StormByte::Type::Integral<From> && StormByte::Type::FloatingPoint<To>) {
						return static_cast<To>(val);
					} else if constexpr (StormByte::Type::FloatingPoint<From> && StormByte::Type::Integral<To>) {
						long double d = static_cast<long double>(val);
						if (!std::isfinite(d))
							throw WrongValueType("Non-finite floating conversion to integer.");
						if (std::trunc(d) != d)
							throw WrongValueType("Floating value has fractional part; would lose data.");
						const long double upper_bound = std::ldexp(1.0L, std::numeric_limits<To>::digits);
						if constexpr (StormByte::Type::Signed<To>) {
							if (d < -upper_bound || d >= upper_bound)
								throw WrongValueType("Floating to integer conversion would overflow/narrow.");
						} else {
							if (d < 0)
								throw WrongValueType("Negative value cannot be converted to unsigned.");
							if (d >= upper_bound)
								throw WrongValueType("Floating to integer conversion would overflow/narrow.");
						}
						return static_cast<To>(d);
					} else if constexpr (StormByte::Type::FloatingPoint<From> && StormByte::Type::FloatingPoint<To>) {
						return static_cast<To>(val);
					} else {
						throw WrongValueType("Unsupported numeric conversion.");
					}
				}

				ValuesVariant m_value; ///< Base-owned alternative storage.
				enum Type m_type;	   ///< SQL value discriminator.
		};
	}
}

/** @brief Conditional DLL safety requires a compatible ABI and live Database module; derivatives must preserve module-owned lifetime. */
STORMBYTE_DECLARE_MAYBE_SAFE(StormByte::Database::Value);
