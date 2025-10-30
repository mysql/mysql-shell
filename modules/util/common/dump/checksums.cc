/*
 * Copyright (c) 2023, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/util/common/dump/checksums.h"

#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>
#include <rapidjson/stream.h>

#include <cassert>
#include <cinttypes>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/common/dump/server_info.h"

namespace mysqlsh {
namespace dump {
namespace common {

namespace {

const mysqlshdk::utils::Version k_current_version{1, 0, 0};

inline std::string quote(std::string_view object) {
  return shcore::quote_identifier(object);
}

inline std::string quote(std::string_view schema, std::string_view table) {
  return quote(schema) + "." + quote(table);
}

std::string to_string(Checksums::Algorithm algorithm) {
  switch (algorithm) {
    case Checksums::Algorithm::BIT_XOR:
      return "bit_xor";
  }

  throw std::logic_error("to_string(Algorithm)");
}

std::optional<Checksums::Algorithm> to_algorithm(std::string_view value) {
  if (shcore::str_caseeq(value, "bit_xor")) {
    return Checksums::Algorithm::BIT_XOR;
  } else {
    return {};
  }
}

std::string to_string(Checksums::Hash hash) {
  switch (hash) {
    case Checksums::Hash::SHA_224:
      return "sha224";
    case Checksums::Hash::SHA_256:
      return "sha256";
    case Checksums::Hash::SHA_384:
      return "sha384";
    case Checksums::Hash::SHA_512:
      return "sha512";
  }

  throw std::logic_error("to_string(Hash)");
}

std::optional<Checksums::Hash> to_hash(std::string_view value) {
  if (shcore::str_caseeq(value, "sha224")) {
    return Checksums::Hash::SHA_224;
  } else if (shcore::str_caseeq(value, "sha256")) {
    return Checksums::Hash::SHA_256;
  } else if (shcore::str_caseeq(value, "sha384")) {
    return Checksums::Hash::SHA_384;
  } else if (shcore::str_caseeq(value, "sha512")) {
    return Checksums::Hash::SHA_512;
  } else {
    return {};
  }
}

int bits(Checksums::Hash hash) {
  switch (hash) {
    case Checksums::Hash::SHA_224:
      return 224;
    case Checksums::Hash::SHA_256:
      return 256;
    case Checksums::Hash::SHA_384:
      return 384;
    case Checksums::Hash::SHA_512:
      return 512;
  }

  throw std::logic_error("bits(Hash)");
}

}  // namespace

Checksums::Checksum_data::Checksum_data(const Checksums *parent,
                                        const Table_info *info,
                                        std::string_view partition,
                                        int64_t chunk)
    : m_parent(parent), m_info(info), m_partition(partition), m_chunk(chunk) {
  update_id();
}

void Checksums::Checksum_data::compute(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const std::string &query_comment) {
  m_result = compute_checksum(session, query_comment);
}

std::pair<bool, Checksums::Checksum_result> Checksums::Checksum_data::validate(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const std::string &query_comment) const {
  auto result = compute_checksum(session, query_comment);
  const auto valid = result == m_result;

  if (!valid) {
    log_warning("Checksum mismatch (%s), expected: %s (%" PRIu64
                " rows), but got: %s (%" PRIu64 " rows)",
                m_id.c_str(), m_result.checksum.c_str(), m_result.count,
                result.checksum.c_str(), result.count);
  }

  return {valid, std::move(result)};
}

Checksums::Checksum_result Checksums::Checksum_data::compute_checksum(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const std::string &query_comment) const {
  log_debug(
      "Computing checksum of %s using boundary: %s, filter: %s", m_id.c_str(),
      m_boundary.length() ? m_boundary.c_str() : "NONE",
      m_info->query.where.length() ? m_info->query.where.c_str() : "NONE");

  // ensure that results for empty sets have the same length as other results -
  // use a single byte character set
  session->execute(
      "SET @saved_character_set_connection = @@character_set_connection");
  session->execute("SET @@character_set_connection = ascii");

  shcore::on_leave_scope restore_character_set{[&session]() {
    session->execute(
        "SET @@character_set_connection = @saved_character_set_connection");
  }};

  // set timezone to UTC to ensure consistent results
  session->execute("SET @saved_time_zone = @@time_zone");
  session->execute("SET @@time_zone = '+00:00'");

  shcore::on_leave_scope restore_time_zone{
      [&session]() { session->execute("SET @@time_zone = @saved_time_zone"); }};

  for (const auto &q : m_parent->m_extra_queries) {
    session->execute(q);
  }

  const auto q = query() + query_comment;
  Checksum_result r;

  if (const auto result = session->query(q)) {
    if (const auto row = result->fetch_one()) {
      r.count = row->get_uint(0);
      r.checksum = shcore::str_upper(row->get_string(1));
    }
  }

  if (r.checksum.empty()) {
    const auto msg = "Failed to compute checksum of " + m_id;
    log_error("%s", msg.c_str());
    throw std::runtime_error(msg);
  } else {
    log_debug("Checksum of %s: %s (%" PRIu64 " rows)", m_id.c_str(),
              r.checksum.c_str(), r.count);
    return r;
  }
}

std::string Checksums::Checksum_data::query() const {
  std::string q;
  q.reserve(256);

  q += "SELECT count(*),";
  q += m_info->query.select_expr;
  q += " FROM ";
  q += m_info->query.from;

  if (!m_partition.empty()) {
    q += " PARTITION (";
    q += quote(m_partition);
    q += ')';
  }

  if (!m_boundary.empty() || !m_info->query.where.empty()) {
    q += " WHERE ";

    if (!m_boundary.empty()) {
      q += m_boundary;
    }

    if (!m_boundary.empty() && !m_info->query.where.empty()) {
      q += " AND ";
    }

    if (!m_info->query.where.empty()) {
      q += m_info->query.where;
    }
  }

  if (!m_info->query.order_by.empty()) {
    q += " ORDER BY ";
    q += m_info->query.order_by;
  }

  return q;
}

void Checksums::Checksum_data::update_id() {
  m_id = "table ";
  m_id += m_info->query.from;

  if (!m_partition.empty()) {
    m_id += ", partition ";
    m_id += quote(m_partition);
  }

  m_id += ", chunk ";
  m_id += std::to_string(m_chunk);
}

void Checksums::configure(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  if (mysqlshdk::utils::Version() == m_version) {
    m_version = k_current_version;
  }

  m_generator_template =
      "sha2(concat_ws('#',{})," + std::to_string(bits(m_hash)) + ')';

  if (server_version(session).is_8_0) {
    m_select_expr_template = "hex(bit_xor(unhex({})))";
  } else {
    // versions older than 8.0 can only use 64bit unsigned integers in bit_xor()
    // we need to compute the hash in slices
    const std::vector<std::string> values = {"'s'"};
    const auto expected = session->query("select " + generator(values))
                              ->fetch_one_or_throw()
                              ->get_string(0);

    // four bits per character
    const auto characters = bits(m_hash) / 4;
    // 16 characters per slice
    constexpr int slice = 16;
    // round up
    const auto slices = (characters + slice - 1) / slice;

    std::string expr_template = "concat(";

    for (int i = 0, start = 1; i < slices; ++i, start += slice) {
      auto length = characters - start + 1;

      if (length > slice) {
        length = slice;
      }

      // get a slice of the hash, convert it from base16 to base10, convert to
      // an unsigned integer, apply bit_xor, convert to base16 string, left pad
      // if necessary
      expr_template += shcore::str_format(
          "lpad(hex(bit_xor(cast(conv(substring(@h{%d},%d,%d),16,10) as "
          "unsigned))),%d,'0'),",
          i, start, length, length);
    }

    // remove last comma
    expr_template.pop_back();
    expr_template += ')';

    const auto create_template = [&expr_template](int idx) {
      return shcore::str_subvars(
          expr_template,
          [idx](std::string_view key) {
            assert(!key.empty());
            return idx == key[0] - '0' ? ":={}" : "";
          },
          "{", "}");
    };

    // we reset the hash to help detect any errors; if user variable is not set,
    // first hash computed by a session may be invalid
    m_extra_queries.emplace_back("set @h:=''");

    // find out where we can safely place the assignment of user variable
    bool found = false;

    for (int i = 0; i < slices && !found; ++i) {
      m_select_expr_template = create_template(i);

      for (const auto &q : m_extra_queries) {
        session->execute(q);
      }

      const auto actual = session->query("select " + select_expr(values))
                              ->fetch_one_or_throw()
                              ->get_string(0);

      if (shcore::str_caseeq(expected, actual)) {
        found = true;
      }
    }

    if (!found) {
      throw std::runtime_error("Couldn't prepare the checksum query!");
    }
  }

  // initialize table data read from the file
  for (auto &schema : m_schemas) {
    for (auto &table : schema.second) {
      initialize(&table.second);
    }
  }
}

void Checksums::initialize_table(const std::string &schema,
                                 const std::string &table,
                                 const Instance_cache::Table *info,
                                 const Instance_cache::Index *index,
                                 const std::string &filter) {
  assert(!schema.empty());
  assert(!table.empty());
  assert(info);

  const auto s = m_schemas.try_emplace(schema).first;
  const auto t = s->second.try_emplace(table).first;
  auto &ti = t->second;

  ti.schema = s->first;
  ti.table = t->first;

  if (index) {
    ti.index_columns.reserve(index->columns().size());

    for (const auto &column : index->columns()) {
      ti.index_columns.emplace_back(column->name);
    }
  }

  ti.columns.reserve(info->columns.size());
  ti.null_columns.reserve(info->columns.size() - ti.index_columns.size());

  for (const auto &column : info->columns) {
    ti.columns.emplace_back(column->name);

    if (column->nullable) {
      ti.null_columns.emplace_back(column->name);
    }
  }

  initialize(&ti);
  ti.query.where = filter;
}

Checksums::Checksum_data *Checksums::prepare_checksum(
    const std::string &schema, const std::string &table,
    const std::string &partition, int64_t chunk, const std::string &boundary) {
  assert(!schema.empty());
  assert(!table.empty());

  const auto s = m_schemas.find(schema);
  assert(m_schemas.end() != s);
  const auto t = s->second.find(table);
  assert(s->second.end() != t);
  auto &ti = t->second;
  const auto p = ti.partitions.try_emplace(partition).first;

  auto &data =
      p->second.emplace(chunk, Checksum_data{this, &ti, p->first, chunk})
          .first->second;
  data.m_boundary = boundary;

  return &data;
}

const Checksums::Checksum_data *Checksums::find_checksum(
    const std::string &schema, const std::string &table,
    const std::string &partition, int64_t chunk) const {
  assert(!schema.empty());
  assert(!table.empty());

  const auto s = m_schemas.find(schema);

  if (m_schemas.end() == s) {
    return nullptr;
  }

  const auto t = s->second.find(table);

  if (s->second.end() == t) {
    return nullptr;
  }

  const auto p = t->second.partitions.find(partition);

  if (t->second.partitions.end() == p) {
    return nullptr;
  }

  const auto ch = p->second.find(chunk);

  if (p->second.end() == ch) {
    return nullptr;
  }

  return &ch->second;
}

std::unordered_set<const Checksums::Checksum_data *> Checksums::find_checksums(
    const std::string &schema, const std::string &table,
    const std::string &partition) const {
  assert(!schema.empty());
  assert(!table.empty());

  const auto s = m_schemas.find(schema);

  if (m_schemas.end() == s) {
    return {};
  }

  const auto t = s->second.find(table);

  if (s->second.end() == t) {
    return {};
  }

  const auto p = t->second.partitions.find(partition);

  if (t->second.partitions.end() == p) {
    return {};
  }

  std::unordered_set<const Checksum_data *> result;

  for (const auto &checksum : p->second) {
    result.emplace(&checksum.second);
  }

  return result;
}

void Checksums::serialize(
    std::unique_ptr<mysqlshdk::storage::IFile> file) const {
  shcore::JSON_dumper json{true};
  // State::START
  json.start_object();  // -> State::GLOBAL

  json.append("config");  // -> State::CONFIG_START
  json.start_object();    // -> State::CONFIG
  // -> State::STRING_CONVERSION -> State::CONFIG
  json.append("version", k_current_version.get_full());
  // -> State::STRING_CONVERSION -> State::CONFIG
  json.append("algorithm", to_string(m_algorithm));
  // -> State::STRING_CONVERSION -> State::CONFIG
  json.append("hash", to_string(m_hash));
  json.end_object();  // -> State::GLOBAL

  json.append("data");  // -> State::DATA_START
  json.start_object();  // -> State::SCHEMA_NAME

  for (const auto &schema : m_schemas) {
    // schema_name
    json.append(schema.first);  // -> State::SCHEMA_START
    json.start_object();        // -> State::TABLE_NAME

    for (const auto &table : schema.second) {
      // table_name
      json.append(table.first);  // -> State::TABLE_START
      json.start_object();       // -> State::TABLE

      if (!table.second.query.where.empty()) {
        // -> State::STRING -> State::TABLE
        json.append("extraFilter", table.second.query.where);
      }

      json.append("columns");  // -> State::COLUMNS_START
      json.start_array();      // -> State::ARRAY

      for (const auto &column : table.second.columns) {
        json.append(column);  // -> State::ARRAY
      }

      json.end_array();  // -> State::TABLE

      if (!table.second.index_columns.empty()) {
        json.append("indexColumns");  // -> State::INDEX_COLUMNS_START
        json.start_array();           // -> State::ARRAY

        for (const auto &column : table.second.index_columns) {
          json.append(column);  // -> State::ARRAY
        }

        json.end_array();  // -> State::TABLE
      }

      if (!table.second.null_columns.empty()) {
        json.append("nullColumns");  // -> State::NULL_COLUMNS_START
        json.start_array();          // -> State::ARRAY

        for (const auto &column : table.second.null_columns) {
          json.append(column);  // -> State::ARRAY
        }

        json.end_array();  // -> State::TABLE
      }

      json.append("partitions");  // -> State::PARTITIONS
      json.start_object();        // -> State::PARTITION_NAME

      for (const auto &partition : table.second.partitions) {
        // partition_name
        json.append(partition.first);  // -> State::PARTITION_START
        json.start_object();           // -> State::CHUNK_ID

        for (const auto &chunk : partition.second) {
          // chunk_id
          json.append(std::to_string(chunk.first));  // -> State::CHUNK_START
          json.start_object();                       // -> State::CHUNK

          // -> State::STRING -> State::CHUNK
          json.append("checksum", chunk.second.m_result.checksum);

          // -> State::COUNT -> State::CHUNK
          json.append("count", chunk.second.m_result.count);

          if (!chunk.second.m_boundary.empty()) {
            // -> State::STRING -> State::CHUNK
            json.append("boundary", chunk.second.m_boundary);
          }

          json.end_object();  // -> State::CHUNK_ID
        }

        json.end_object();  // -> State::PARTITION_NAME
      }

      json.end_object();  // -> State::TABLE
      json.end_object();  // -> State::TABLE_NAME
    }

    json.end_object();  // -> State::SCHEMA_NAME
  }

  json.end_object();  // -> State::GLOBAL
  json.end_object();  // -> State::END

  file->open(mysqlshdk::storage::Mode::WRITE);
  file->write(json.str().c_str(), json.str().length());
  file->close();
}

void Checksums::deserialize(std::unique_ptr<mysqlshdk::storage::IFile> file) {
  // deserialize() needs to be called before configure()
  assert(m_select_expr_template.empty() && m_generator_template.empty());

  struct Handler {
    using Ch = std::string::value_type;

    explicit Handler(Checksums *parent) : m_parent(parent) {}

    bool Null() {
      m_error = "Unexpected value: null";
      return false;
    }

    bool Bool(bool) {
      m_error = "Unexpected value: bool";
      return false;
    }

    bool Int(int) {
      m_error = "Unexpected value: int";
      return false;
    }

    bool Uint(unsigned) {
      m_error = "Unexpected value: unsigned int";
      return false;
    }

    bool Int64(int64_t) {
      m_error = "Unexpected value: int64";
      return false;
    }

    bool Uint64(uint64_t) {
      m_error = "Unexpected value: unsigned int64";
      return false;
    }

    bool Double(double) {
      m_error = "Unexpected value: double";
      return false;
    }

    bool RawNumber(const Ch *str, rapidjson::SizeType length, bool) {
      const std::string_view value{str, length};

      switch (m_state) {
        case State::COUNT:
          try {
            m_current_chunk->m_result.count =
                shcore::lexical_cast<uint64_t>(value);
            m_state = State::CHUNK;
            return true;
          } catch (const std::exception &e) {
            m_error = "Failed to parse row count '";
            m_error += value;
            m_error += "': ";
            m_error += e.what();
            return false;
          }

        default:
          m_error = "Unexpected value: raw number";
          return false;
      }
    }

    bool String(const Ch *str, rapidjson::SizeType length, bool) {
      const std::string_view value{str, length};

      switch (m_state) {
        case State::STRING:
          *m_current_string = std::string{value};
          m_state = m_return_to;
          return true;

        case State::STRING_CONVERSION:
          if (m_convert_string(value)) {
            m_state = m_return_to;
            return true;
          } else {
            m_error = m_string_conversion_error(value);
            return false;
          }

        case State::ARRAY:
          m_current_array->emplace_back(value);
          return true;

        default:
          m_error = "Unexpected value: string";
          return false;
      }
    }

    bool StartObject() {
      switch (m_state) {
        case State::START:
          m_state = State::GLOBAL;
          return true;

        case State::CONFIG_START:
          m_state = State::CONFIG;
          return true;

        case State::DATA_START:
          m_state = State::SCHEMA_NAME;
          return true;

        case State::SCHEMA_START:
          m_state = State::TABLE_NAME;
          return true;

        case State::TABLE_START:
          m_state = State::TABLE;
          return true;

        case State::PARTITIONS:
          m_state = State::PARTITION_NAME;
          return true;

        case State::PARTITION_START:
          m_state = State::CHUNK_ID;
          return true;

        case State::CHUNK_START:
          m_state = State::CHUNK;
          return true;

        default:
          m_error = "Unexpected start of an object";
          return false;
      }
    }

    bool Key(const Ch *str, rapidjson::SizeType length, bool) {
      const std::string_view key{str, length};

      switch (m_state) {
        case State::GLOBAL:
          if ("config" == key) {
            m_state = State::CONFIG_START;
            return true;
          } else if ("data" == key) {
            m_state = State::DATA_START;
            return true;
          } else {
            break;
          }

        case State::CONFIG:
          if ("version" == key) {
            return expect_string_conversion(
                [this](std::string_view value) {
                  try {
                    m_parent->m_version = mysqlshdk::utils::Version{value};
                    return true;
                  } catch (const std::exception &e) {
                    m_error = "Could not parse checksum format version '";
                    m_error += value;
                    m_error += "': ";
                    m_error += e.what();
                    return false;
                  }
                },
                [this](std::string_view) { return m_error; });
          } else if ("algorithm" == key) {
            return expect_string_conversion(
                [this](std::string_view value) {
                  if (const auto converted = to_algorithm(value);
                      converted.has_value()) {
                    m_parent->m_algorithm = *converted;
                    return true;
                  } else {
                    return false;
                  }
                },
                [](std::string_view value) {
                  std::string msg = "Unsupported algorithm type: ";
                  msg += value;
                  return msg;
                });
          } else if ("hash" == key) {
            return expect_string_conversion(
                [this](std::string_view value) {
                  if (const auto converted = to_hash(value);
                      converted.has_value()) {
                    m_parent->m_hash = *converted;
                    return true;
                  } else {
                    return false;
                  }
                },
                [](std::string_view value) {
                  std::string msg = "Unsupported hash type: ";
                  msg += value;
                  return msg;
                });
          } else {
            break;
          }

        case State::SCHEMA_NAME:
          m_current_schema =
              m_parent->m_schemas.try_emplace(std::string{key}).first;
          m_state = State::SCHEMA_START;
          return true;

        case State::TABLE_NAME: {
          const auto t =
              m_current_schema->second.try_emplace(std::string{key}).first;
          m_current_table = &t->second;
          m_current_table->schema = m_current_schema->first;
          m_current_table->table = t->first;
          m_current_table->query.from = quote(m_current_schema->first, key);
          m_state = State::TABLE_START;
          return true;
        }

        case State::TABLE:
          if ("extraFilter" == key) {
            return expect_string(&m_current_table->query.where);
          } else if ("columns" == key) {
            m_state = State::COLUMNS_START;
            return true;
          } else if ("indexColumns" == key) {
            m_state = State::INDEX_COLUMNS_START;
            return true;
          } else if ("nullColumns" == key) {
            m_state = State::NULL_COLUMNS_START;
            return true;
          } else if ("partitions" == key) {
            m_state = State::PARTITIONS;
            return true;
          } else {
            break;
          }

        case State::PARTITION_NAME:
          m_current_partition =
              m_current_table->partitions.try_emplace(std::string{key}).first;
          m_state = State::PARTITION_START;
          return true;

        case State::CHUNK_ID: {
          int64_t chunk;

          try {
            chunk = shcore::lexical_cast<int64_t>(key);
          } catch (const std::exception &e) {
            m_error = "Failed to parse chunk id '";
            m_error += key;
            m_error += "': ";
            m_error += e.what();
            return false;
          }

          m_current_chunk =
              &m_current_partition->second
                   .emplace(chunk,
                            Checksum_data{m_parent, m_current_table,
                                          m_current_partition->first, chunk})
                   .first->second;
          m_state = State::CHUNK_START;
          return true;
        }

        case State::CHUNK:
          if ("checksum" == key) {
            return expect_string(&m_current_chunk->m_result.checksum);
          } else if ("count" == key) {
            m_state = State::COUNT;
            return true;
          } else if ("boundary" == key) {
            return expect_string(&m_current_chunk->m_boundary);
          } else {
            break;
          }

        default:
          break;
      }

      m_error = "Unexpected key of an object: ";
      m_error += key;
      return false;
    }

    bool EndObject(rapidjson::SizeType) {
      switch (m_state) {
        case State::GLOBAL:
          m_state = State::END;
          return true;

        case State::CONFIG:
          m_state = State::GLOBAL;
          return true;

        case State::SCHEMA_NAME:
          m_state = State::GLOBAL;
          return true;

        case State::TABLE_NAME:
          m_state = State::SCHEMA_NAME;
          return true;

        case State::TABLE:
          m_state = State::TABLE_NAME;
          return true;

        case State::PARTITION_NAME:
          m_state = State::TABLE;
          return true;

        case State::CHUNK_ID:
          m_state = State::PARTITION_NAME;
          return true;

        case State::CHUNK:
          m_state = State::CHUNK_ID;
          return true;

        default:
          m_error = "Unexpected end of an object";
          return false;
      }
    }

    bool StartArray() {
      switch (m_state) {
        case State::COLUMNS_START:
          return expect_array(&m_current_table->columns, State::TABLE);

        case State::INDEX_COLUMNS_START:
          return expect_array(&m_current_table->index_columns, State::TABLE);

        case State::NULL_COLUMNS_START:
          return expect_array(&m_current_table->null_columns, State::TABLE);

        default:
          m_error = "Unexpected start of an array";
          return false;
      }
    }

    bool EndArray(rapidjson::SizeType) {
      switch (m_state) {
        case State::ARRAY:
          m_state = m_return_to;
          return true;

        default:
          m_error = "Unexpected end of an array";
          return false;
      }
    }

    inline const std::string &error() const noexcept { return m_error; }

   private:
    enum class State {
      START,
      GLOBAL,
      CONFIG_START,
      CONFIG,
      DATA_START,
      SCHEMA_NAME,
      SCHEMA_START,
      TABLE_NAME,
      TABLE_START,
      TABLE,
      STRING,
      STRING_CONVERSION,
      ARRAY,
      COLUMNS_START,
      INDEX_COLUMNS_START,
      NULL_COLUMNS_START,
      PARTITIONS,
      PARTITION_NAME,
      PARTITION_START,
      CHUNK_ID,
      CHUNK_START,
      CHUNK,
      COUNT,
      END,
    };

    bool expect_string(std::string *target) noexcept {
      m_current_string = target;
      m_return_to = m_state;
      m_state = State::STRING;
      return true;
    }

    bool expect_string_conversion(
        std::function<bool(std::string_view)> convert,
        std::function<std::string(std::string_view)> error) noexcept {
      m_convert_string = std::move(convert);
      m_string_conversion_error = std::move(error);
      m_return_to = m_state;
      m_state = State::STRING_CONVERSION;
      return true;
    }

    bool expect_array(std::vector<std::string> *target,
                      State return_to) noexcept {
      m_current_array = target;
      m_return_to = return_to;
      m_state = State::ARRAY;
      return true;
    }

    Checksums *m_parent;
    State m_state = State::START;
    std::string m_error;

    Schemas_t::iterator m_current_schema;
    Table_info *m_current_table;
    Partitions_t::iterator m_current_partition;
    std::string m_current_partition_name;
    Checksum_data *m_current_chunk;
    std::string *m_current_string;
    std::vector<std::string> *m_current_array;
    std::function<bool(std::string_view)> m_convert_string;
    std::function<std::string(std::string_view)> m_string_conversion_error;
    State m_return_to;
  };

  file->open(mysqlshdk::storage::Mode::READ);
  const auto json = mysqlshdk::storage::read_file(file.get());
  file->close();

  rapidjson::StringStream stream(json.c_str());

  Handler handler{this};
  rapidjson::Reader reader;

  if (!reader.Parse<rapidjson::ParseFlag::kParseNumbersAsStringsFlag>(
          stream, handler)) {
    const auto offset = reader.GetErrorOffset();
    throw std::runtime_error(shcore::str_format(
        "Failed to parse '%s': %s, at offset %zu, near: '%s'",
        file->full_path().masked().c_str(),
        handler.error().empty()
            ? rapidjson::GetParseError_En(reader.GetParseErrorCode())
            : handler.error().c_str(),
        offset, json.substr(offset >= 10 ? offset - 10 : 0, 20).c_str()));
  }

  if (mysqlshdk::utils::Version() == m_version ||
      m_version.get_major() > k_current_version.get_major() ||
      (m_version.get_major() == k_current_version.get_major() &&
       m_version.get_minor() > k_current_version.get_minor())) {
    throw std::runtime_error("Checksum format has version " +
                             m_version.get_full() +
                             " which is not supported by this version of MySQL "
                             "Shell. Please upgrade MySQL Shell to load it.");
  }
}

void Checksums::initialize(Table_info *info) const {
  std::vector<std::string> values;

  values.reserve(info->columns.size() + (info->null_columns.empty() ? 0 : 1));

  for (const auto &column : info->columns) {
    values.emplace_back("convert(" + quote(column) + " using binary)");
  }

  if (!info->null_columns.empty()) {
    // concat_ws() skips NULL values, if table has nullable columns, we add a
    // bit field to detect mismatches like (NULL, 'value') vs ('value', NULL)
    values.emplace_back("concat(" +
                        shcore::str_join(info->null_columns, ",",
                                         [](const auto &column) {
                                           return "isnull(" + quote(column) +
                                                  ")";
                                         }) +
                        ")");
  }

  info->query.select_expr = select_expr(values);
  info->query.from = quote(info->schema, info->table);
  info->query.order_by = shcore::str_join(
      info->index_columns, ",", [](const auto &c) { return quote(c); });
}

std::string Checksums::select_expr(
    const std::vector<std::string> &values) const {
  assert(!m_select_expr_template.empty());

  return shcore::str_subvars(
      m_select_expr_template,
      [this, &values](std::string_view) { return generator(values); }, "{",
      "}");
}

std::string Checksums::generator(const std::vector<std::string> &values) const {
  assert(!m_generator_template.empty());

  return shcore::str_subvars(
      m_generator_template,
      [&values](std::string_view) { return shcore::str_join(values, ","); },
      "{", "}");
}

void Checksums::rename_schema(const std::string &from, const std::string &to) {
  // rename the key
  auto node = m_schemas.extract(from);
  node.key() = to;
  auto s = m_schemas.insert(std::move(node)).position;

  // update the references
  for (auto &t : s->second) {
    auto &ti = t.second;
    ti.schema = s->first;
    ti.query.from = quote(ti.schema, ti.table);

    for (auto &p : ti.partitions) {
      for (auto &ch : p.second) {
        ch.second.update_id();
      }
    }
  }
}

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh
