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

#include <StormByte/database/mariadb/prepared_stmt.hxx>
#include <StormByte/database/mariadb/result_fetch.hxx>
#include <StormByte/database/mariadb/telemetry.hxx>
#include <StormByte/database/engine_handles.hxx>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
using namespace StormByte::Database::MariaDB;
void PreparedSTMT::EnsureParamSize(StormByte::Safe::Vector<StormByte::Database::Value>& params, StormByte::Size index) {
	if (index >= StormByte::Size{params.size()})
		params.resize(static_cast<std::size_t>(index) + 1);
}

PreparedSTMT::PreparedSTMT(ConstructionKey, std::string_view name, std::string_view query,
		const StormByte::Safe::Shared<Logger::Log>& logger,
		const StormByte::Safe::Shared<StormByte::Database::Telemetry>& telemetry)
	: StormByte::Database::PreparedSTMT(name, query, logger, telemetry),
	  m_statement_handle(StormByte::Safe::Unique<StatementHandle>::MakePointer<StatementHandle>()),
	  m_params() {}

bool PreparedSTMT::Initialize() noexcept {
	MYSQL* cpp_conn = static_cast<MYSQL*>(m_statement_handle->m_native_connection);
	if (!cpp_conn)
		return false;
	MYSQL_STMT* stmt = mysql_stmt_init(cpp_conn);
	if (!stmt) {
		if (m_logger) {
			*m_logger << Logger::Level::Error << "MariaDB: mysql_stmt_init returned null for query: " << Query() << std::endl;
		}

		return false;
	}

	std::string prepq{Query()};
	while (!prepq.empty() && isspace(static_cast<unsigned char>(prepq.back()))) prepq.pop_back();
	if (!prepq.empty() && prepq.back() == ';') prepq.pop_back();
	if (prepq.size() > std::numeric_limits<unsigned long>::max()) {
		mysql_stmt_close(stmt);
		return false;
	}
	if (mysql_stmt_prepare(stmt, prepq.c_str(), static_cast<unsigned long>(prepq.size())) != 0) {
		if (m_logger) {
			*m_logger << Logger::Level::Error << "MariaDB: mysql_stmt_prepare error: "
					<< (mysql_stmt_error(stmt) ? mysql_stmt_error(stmt) : "<none>")
					<< " for query: " << Query() << std::endl;
		}

		mysql_stmt_close(stmt);
		return false;
	}

	m_statement_handle->m_native_statement = stmt;
	return true;
}

PreparedSTMT::PreparedSTMT(PreparedSTMT&& other) noexcept:
	StormByte::Database::PreparedSTMT(std::move(other)),
	m_statement_handle(std::move(other.m_statement_handle)), m_params(std::move(other.m_params)) {}

PreparedSTMT::~PreparedSTMT() noexcept {
	MYSQL_STMT* stmt = m_statement_handle ? static_cast<MYSQL_STMT*>(m_statement_handle->m_native_statement) : nullptr;
	if (stmt) {
		mysql_stmt_close(stmt);
		m_statement_handle->m_native_statement = nullptr;
	}
}

PreparedSTMT& PreparedSTMT::operator=(PreparedSTMT&& other) noexcept {
	if (this != &other) {
		MYSQL_STMT* stmt = m_statement_handle ? static_cast<MYSQL_STMT*>(m_statement_handle->m_native_statement) : nullptr;
		if (stmt)
			mysql_stmt_close(stmt);
		StormByte::Database::PreparedSTMT::operator=(std::move(other));
		m_statement_handle = std::move(other.m_statement_handle);
		m_params = std::move(other.m_params);
	}
	return *this;
}

void PreparedSTMT::Binder(StormByte::Size index, Value&& value) noexcept {
	EnsureParamSize(m_params, index);
	m_params[static_cast<std::size_t>(index)] = std::move(value);
}

void PreparedSTMT::Reset() noexcept {
	m_params.clear();
	MYSQL_STMT* stmt = m_statement_handle ? static_cast<MYSQL_STMT*>(m_statement_handle->m_native_statement) : nullptr;
	if (stmt) {
		mysql_stmt_reset(stmt);
	}
}

StormByte::Database::ExpectedRows PreparedSTMT::DoExecute() {
	if (!m_statement_handle || !m_statement_handle->m_native_connection || !m_statement_handle->m_native_statement) {
		return Unexpected<ExecuteError>("No DB connection or statement");
	}

	MYSQL_STMT* stmt = static_cast<MYSQL_STMT*>(m_statement_handle->m_native_statement);
	auto record_statement_error = [this, stmt]() {
		if (auto* mariadb_telemetry = dynamic_cast<Telemetry*>(m_telemetry.get()))
			mariadb_telemetry->RecordMariaDBError(mysql_stmt_errno(stmt));
	};
	if (m_params.size() != mysql_stmt_param_count(stmt))
		return Unexpected<ExecuteError>("Prepared statement parameter count mismatch");
	std::vector<MYSQL_BIND> bind_in;
	bind_in.resize(m_params.size());
	std::vector<int32_t> int_buf(m_params.size());
	std::vector<uint32_t> uint_buf(m_params.size());
	std::vector<int64_t> ll_buf(m_params.size());
	std::vector<uint64_t> ull_buf(m_params.size());
	std::vector<double> dbl_buf(m_params.size());
	std::vector<char> bool_buf(m_params.size());
	std::vector<std::string> str_buf(m_params.size());
	std::vector<StormByte::Safe::Binary> bin_buf(m_params.size());
	std::vector<char> empty_blob_buffer(m_params.size());
	std::vector<unsigned long> str_len(m_params.size());
	std::vector<my_bool> is_null(m_params.size());
	for (size_t i = 0; i < m_params.size(); ++i) {
		memset(&bind_in[i], 0, sizeof(MYSQL_BIND));
		const StormByte::Database::Value& p = m_params[i];
		is_null[i] = 0;
		if (p.IsNull()) {
			bind_in[i].buffer_type = MYSQL_TYPE_NULL;
			bind_in[i].is_null = &is_null[i];
			is_null[i] = 1;
			continue;
		}

		auto vt = p.Type();
		switch (vt) {
			case StormByte::Database::Value::Type::Integer: {
				int_buf[i] = p.Get<int>();
				bind_in[i].buffer_type = MYSQL_TYPE_LONG;
				bind_in[i].buffer = &int_buf[i];
				bind_in[i].is_null = &is_null[i];
				break;
			}

			case StormByte::Database::Value::Type::UnsignedInteger: {
				uint_buf[i] = p.Get<unsigned int>();
				bind_in[i].buffer_type = MYSQL_TYPE_LONG;
				bind_in[i].buffer = &uint_buf[i];
				bind_in[i].is_unsigned = 1;
				bind_in[i].is_null = &is_null[i];
				break;
			}

			case StormByte::Database::Value::Type::LongInteger: {
				ll_buf[i] = p.Get<long long int>();
				bind_in[i].buffer_type = MYSQL_TYPE_LONGLONG;
				bind_in[i].buffer = &ll_buf[i];
				bind_in[i].is_null = &is_null[i];
				break;
			}

			case StormByte::Database::Value::Type::UnsignedLongInteger: {
				ull_buf[i] = p.Get<unsigned long long int>();
				bind_in[i].buffer_type = MYSQL_TYPE_LONGLONG;
				bind_in[i].buffer = &ull_buf[i];
				bind_in[i].is_unsigned = 1;
				bind_in[i].is_null = &is_null[i];
				break;
			}

			case StormByte::Database::Value::Type::Double: {
				dbl_buf[i] = p.Get<double>();
				bind_in[i].buffer_type = MYSQL_TYPE_DOUBLE;
				bind_in[i].buffer = &dbl_buf[i];
				bind_in[i].is_null = &is_null[i];
				break;
			}

			case StormByte::Database::Value::Type::Boolean: {
				bool_buf[i] = p.Get<bool>() ? 1 : 0;
				bind_in[i].buffer_type = MYSQL_TYPE_TINY;
				bind_in[i].buffer = &bool_buf[i];
				bind_in[i].is_null = &is_null[i];
				break;
			}

			case StormByte::Database::Value::Type::Text: {
				{
					const auto text = p.Get<StormByte::Safe::String>();
					str_buf[i] = static_cast<std::string_view>(text);
				}
				if (str_buf[i].size() > std::numeric_limits<unsigned long>::max())
					return Unexpected<ExecuteError>("MariaDB bind text exceeds supported length");
				bind_in[i].buffer_type = MYSQL_TYPE_STRING;
				bind_in[i].buffer = const_cast<char*>(str_buf[i].data());
				bind_in[i].buffer_length = static_cast<unsigned long>(str_buf[i].size());
				str_len[i] = bind_in[i].buffer_length;
				bind_in[i].length = &str_len[i];
				bind_in[i].is_null = &is_null[i];
				break;
			}

			case StormByte::Database::Value::Type::Blob: {
				bin_buf[i] = p.Get<StormByte::Safe::Binary>();
				if (bin_buf[i].size() > StormByte::ByteSize{std::numeric_limits<unsigned long>::max()})
					return Unexpected<ExecuteError>("MariaDB bind blob exceeds supported length");
				bind_in[i].buffer_type = MYSQL_TYPE_BLOB;
				bind_in[i].buffer = bin_buf[i].empty()
					? static_cast<void*>(&empty_blob_buffer[i])
					: const_cast<std::byte*>(bin_buf[i].data());
				bind_in[i].buffer_length = static_cast<unsigned long>(bin_buf[i].size());
				str_len[i] = bind_in[i].buffer_length;
				bind_in[i].length = &str_len[i];
				bind_in[i].is_null = &is_null[i];
				break;
			}

			case StormByte::Database::Value::Type::Null:
			default: {
				bind_in[i].buffer_type = MYSQL_TYPE_NULL;
				bind_in[i].is_null = &is_null[i];
				is_null[i] = 1;
				break;
			}
		}
	}

	if (!bind_in.empty()) {
		if (mysql_stmt_bind_param(stmt, bind_in.data()) != 0) {
			record_statement_error();
			return Unexpected<ExecuteError>(mysql_stmt_error(stmt) ? mysql_stmt_error(stmt) : "Unknown MySQL stmt error");
		}
	}

	if (mysql_stmt_execute(stmt) != 0) {
		record_statement_error();
		return Unexpected<ExecuteError>(mysql_stmt_error(stmt) ? mysql_stmt_error(stmt) : "Unknown MySQL stmt error");
	}
	if (const unsigned int warnings = mysql_warning_count(static_cast<MYSQL*>(m_statement_handle->m_native_connection)); warnings > 0) {
		if (auto* mariadb_telemetry = dynamic_cast<Telemetry*>(m_telemetry.get()))
			mariadb_telemetry->RecordMariaDBWarnings(warnings);
	}

	MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
	if (!meta) {
		if (mysql_stmt_field_count(stmt) == 0) {
			return Rows();
		}

		return Unexpected<ExecuteError>(mysql_stmt_error(stmt) ? mysql_stmt_error(stmt) : "Unknown MySQL stmt error");
	}

	const unsigned int nfields = mysql_num_fields(meta);
	std::vector<MYSQL_BIND> bind_out(nfields);
	std::vector<unsigned long> out_len(nfields);
	std::vector<my_bool> out_is_null(nfields);
	std::vector<std::vector<char>> out_str(nfields);
	std::vector<int32_t> out_int(nfields);
	std::vector<uint32_t> out_uint(nfields);
	std::vector<int64_t> out_ll(nfields);
	std::vector<uint64_t> out_ull(nfields);
	std::vector<double> out_dbl(nfields);
	std::vector<char> out_bool(nfields);
	for (unsigned int i = 0; i < nfields; ++i) {
		memset(&bind_out[i], 0, sizeof(MYSQL_BIND));
		MYSQL_FIELD* f = mysql_fetch_field_direct(meta, i);
		unsigned long blen = (f && f->length) ? f->length : 1024;
		out_is_null[i] = 0;
		switch (f ? f->type : MYSQL_TYPE_STRING) {
			case MYSQL_TYPE_TINY:
				if (f && (f->flags & UNSIGNED_FLAG) == 0 && f->length == 1) {
					bind_out[i].buffer_type = MYSQL_TYPE_TINY;
					bind_out[i].buffer = &out_bool[i];
				} else {
					bind_out[i].buffer_type = MYSQL_TYPE_LONG;
					if (f && (f->flags & UNSIGNED_FLAG)) {
						bind_out[i].buffer = &out_uint[i];
					} else {
						bind_out[i].buffer = &out_int[i];
					}
				}

				bind_out[i].is_null = &out_is_null[i];
				bind_out[i].length = &out_len[i];
				break;
			case MYSQL_TYPE_SHORT:
			case MYSQL_TYPE_LONG:
				bind_out[i].buffer_type = MYSQL_TYPE_LONG;
				if (f && (f->flags & UNSIGNED_FLAG)) {
					bind_out[i].buffer = &out_uint[i];
				} else {
					bind_out[i].buffer = &out_int[i];
				}

				bind_out[i].is_null = &out_is_null[i];
				bind_out[i].length = &out_len[i];
				break;
			case MYSQL_TYPE_LONGLONG:
				bind_out[i].buffer_type = MYSQL_TYPE_LONGLONG;
				if (f && (f->flags & UNSIGNED_FLAG)) {
					bind_out[i].buffer = &out_ull[i];
				} else {
					bind_out[i].buffer = &out_ll[i];
				}

				bind_out[i].is_null = &out_is_null[i];
				bind_out[i].length = &out_len[i];
				break;
			case MYSQL_TYPE_FLOAT:
			case MYSQL_TYPE_DOUBLE:
				bind_out[i].buffer_type = MYSQL_TYPE_DOUBLE;
				bind_out[i].buffer = &out_dbl[i];
				bind_out[i].is_null = &out_is_null[i];
				bind_out[i].length = &out_len[i];
				break;
			case MYSQL_TYPE_BLOB:
			case MYSQL_TYPE_VAR_STRING:
			case MYSQL_TYPE_STRING:
			default:
				out_str[i].resize(blen + 1);
				bind_out[i].buffer_type = MYSQL_TYPE_STRING;
				bind_out[i].buffer = out_str[i].data();
				bind_out[i].buffer_length = static_cast<unsigned long>(out_str[i].size());
				bind_out[i].length = &out_len[i];
				bind_out[i].is_null = &out_is_null[i];
				break;
		}
	}

	if (mysql_stmt_bind_result(stmt, bind_out.data()) != 0) {
			record_statement_error();
		mysql_free_result(meta);
		return Unexpected<ExecuteError>(mysql_stmt_error(stmt) ? mysql_stmt_error(stmt) : "Unknown MySQL stmt error");
	}

	if (mysql_stmt_store_result(stmt) != 0) {
		record_statement_error();
		mysql_free_result(meta);
		return Unexpected<ExecuteError>(mysql_stmt_error(stmt) ? mysql_stmt_error(stmt) : "Unknown MySQL stmt error");
	}

	Rows rows;
	while (true) {
		int rc = mysql_stmt_fetch(stmt);
		if (rc == MYSQL_NO_DATA) break;
		if (rc != 0 && rc != MYSQL_DATA_TRUNCATED) {
			record_statement_error();
			mysql_free_result(meta);
			return Unexpected<ExecuteError>(mysql_stmt_error(stmt) ? mysql_stmt_error(stmt) : "Unknown MySQL stmt fetch error");
		}

		if (rc == MYSQL_DATA_TRUNCATED) {
			for (unsigned int ci = 0; ci < nfields; ++ci) {
				if (out_is_null[ci]) continue;
				if (out_len[ci] > bind_out[ci].buffer_length) {
					out_str[ci].resize(out_len[ci] + 1);
					bind_out[ci].buffer = out_str[ci].data();
					bind_out[ci].buffer_length = static_cast<unsigned long>(out_str[ci].size());
					if (mysql_stmt_fetch_column(stmt, &bind_out[ci], ci, 0) != 0) {
						record_statement_error();
						mysql_free_result(meta);
						return Unexpected<ExecuteError>(mysql_stmt_error(stmt) ? mysql_stmt_error(stmt) : "Unknown MySQL stmt fetch_column error");
					}
				}
			}
		}

		Row prow;
		for (unsigned int i = 0; i < nfields; ++i) {
			MYSQL_FIELD* f = mysql_fetch_field_direct(meta, i);
			const char* colName = f ? f->name : nullptr;
			const std::string_view column_name{colName ? colName : ""};
			if (out_is_null[i]) {
				prow.add(column_name, Value());
				continue;
			}

			switch (f ? f->type : MYSQL_TYPE_STRING) {
				case MYSQL_TYPE_TINY: {
					if (f && (f->flags & UNSIGNED_FLAG) == 0 && f->length == 1) {
						prow.add(column_name, static_cast<bool>(out_bool[i] != 0));
					} else if (f && (f->flags & UNSIGNED_FLAG)) {
						prow.add(column_name, static_cast<unsigned int>(out_uint[i]));
					} else {
						prow.add(column_name, static_cast<int>(out_int[i]));
					}

					break;
				}

				case MYSQL_TYPE_SHORT:
				case MYSQL_TYPE_LONG:
					if (f && (f->flags & UNSIGNED_FLAG))
						prow.add(column_name, static_cast<unsigned int>(out_uint[i]));
					else
						prow.add(column_name, static_cast<int>(out_int[i]));
					break;
				case MYSQL_TYPE_LONGLONG:
					if (f && (f->flags & UNSIGNED_FLAG)) {
						prow.add(column_name, static_cast<unsigned long long int>(out_ull[i]));
					} else {
						prow.add(column_name, static_cast<long long int>(out_ll[i]));
					}
					break;
				case MYSQL_TYPE_FLOAT:
				case MYSQL_TYPE_DOUBLE:
					prow.add(column_name, out_dbl[i]);
					break;
				case MYSQL_TYPE_BLOB: {
					unsigned long llen = out_len[i];
					// 63 = binary charset; otherwise treat as text (TEXT/VARCHAR)
					const bool is_binary = f && f->charsetnr == 63;
					if (is_binary) {
						StormByte::Safe::Binary blob{
							reinterpret_cast<const std::byte*>(out_str[i].data()),
							StormByte::ByteSize{llen}
						};
						prow.add(column_name, Value{std::move(blob)});
					} else {
						const char* text_data = out_str[i].data();
						if (!text_data)
							text_data = "";
						prow.add(column_name, std::string_view{text_data, static_cast<std::size_t>(llen)});
					}

					break;
				}

				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_STRING:
				default: {
					unsigned long llen = out_len[i];
					const char* text_data = out_str[i].data();
					if (!text_data)
						text_data = "";
					prow.add(column_name, std::string_view{text_data, static_cast<std::size_t>(llen)});
					break;
				}
			}
		}

		rows.add(std::move(prow));
	}

	mysql_free_result(meta);
	mysql_stmt_free_result(stmt);
	return rows;
}
