/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_UTIL_COMMON_DUMP_CHECKSUMS_H_
#define MODULES_UTIL_COMMON_DUMP_CHECKSUMS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/dump/instance_cache.h"

namespace mysqlsh {
namespace dump {
namespace common {

/**
 * Handles computation of checksum of table data.
 */
class Checksums {
 private:
  struct Table_info;

 public:
  enum class Algorithm {
    BIT_XOR,
  };

  enum class Hash {
    SHA_224,
    SHA_256,
    SHA_384,
    SHA_512,
  };

  /**
   * Holds result of a checksum operation.
   */
  struct Checksum_result {
    std::string checksum;
    uint64_t count;

    bool operator==(const Checksum_result &) const = default;
  };

  /**
   * Holds information about a checksum.
   */
  class Checksum_data {
   public:
    Checksum_data() = delete;

    Checksum_data(const Checksum_data &) = default;
    Checksum_data(Checksum_data &&) = default;

    Checksum_data &operator=(const Checksum_data &) = default;
    Checksum_data &operator=(Checksum_data &&) = default;

    /**
     * Computes checksum of table data.
     *
     * @param session - Session used to compute the checksum.
     * @param query_comment - additional comment to be included in query which
     *        computes the checksum
     */
    void compute(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                 const std::string &query_comment);

    /**
     * Validates checksum of table data.
     *
     * @param session - Session used to validate the checksum.
     * @param query_comment - additional comment to be included in query which
     *        validates the checksum
     *
     * @returns true if checksum is valid + result of checksum operation
     */
    std::pair<bool, Checksum_result> validate(
        const std::shared_ptr<mysqlshdk::db::ISession> &session,
        const std::string &query_comment) const;

    inline std::string_view schema() const noexcept { return m_info->schema; }

    inline std::string_view table() const noexcept { return m_info->table; }

    inline std::string_view partition() const noexcept { return m_partition; }

    inline int64_t chunk() const noexcept { return m_chunk; }

    inline const std::string &boundary() const noexcept { return m_boundary; }

    inline const Checksum_result &result() const noexcept { return m_result; }

   private:
    Checksum_data(const Checksums *parent, const Table_info *info,
                  std::string_view partition, int64_t chunk);

    Checksum_result compute_checksum(
        const std::shared_ptr<mysqlshdk::db::ISession> &session,
        const std::string &query_comment) const;

    std::string query() const;

    friend class Checksums;

    const Checksums *m_parent;
    const Table_info *m_info;
    std::string_view m_partition;
    int64_t m_chunk;

    std::string m_id;

    std::string m_boundary;
    Checksum_result m_result;
  };

  explicit Checksums(Hash hash = Hash::SHA_256,
                     Algorithm algorithm = Algorithm::BIT_XOR)
      : m_hash(hash), m_algorithm(algorithm) {}

  Checksums(const Checksums &) = default;
  Checksums(Checksums &&) = default;

  Checksums &operator=(const Checksums &) = default;
  Checksums &operator=(Checksums &&) = default;

  /**
   * Configures checksum for interaction with the given MySQL instance.
   */
  void configure(const std::shared_ptr<mysqlshdk::db::ISession> &session);

  /**
   * Initializes table for which checksum is going to be computed.
   *
   * @param schema Name of the schema.
   * @param table Name of the table.
   * @param info Information about the table.
   * @param index Index to be used to compute the checksum.
   * @param filter Additional filter to be used when querying data from this
   *        table.
   */
  void initialize_table(const std::string &schema, const std::string &table,
                        const Instance_cache::Table *info,
                        const Instance_cache::Index *index,
                        const std::string &filter);

  /**
   * Prepares for computation of checksum of the given table data.
   *
   * @param schema Name of the schema.
   * @param table Name of the table.
   * @param partition Name of the partition (can be empty).
   * @param chunk ID of the chunk.
   * @param boundary Data boundary (can be empty).
   *
   * @return Initialized checksum.
   */
  Checksum_data *prepare_checksum(const std::string &schema,
                                  const std::string &table,
                                  const std::string &partition, int64_t chunk,
                                  const std::string &boundary);

  /**
   * Fetches checksum of the given table data.
   *
   * @param schema Name of the schema.
   * @param table Name of the table.
   * @param partition Name of the partition (can be empty).
   * @param chunk ID of the chunk.
   *
   * @return Requested checksum or nullptr if it cannot be found.
   */
  const Checksum_data *find_checksum(const std::string &schema,
                                     const std::string &table,
                                     const std::string &partition,
                                     int64_t chunk) const;

  /**
   * Fetches all checksums of the given partition.
   *
   * @param schema Name of the schema.
   * @param table Name of the table.
   * @param partition Name of the partition (can be empty).
   *
   * @return Requested checksums.
   */
  std::unordered_set<const Checksum_data *> find_checksums(
      const std::string &schema, const std::string &table,
      const std::string &partition) const;

  /**
   * Stores checksum information in the given file.
   *
   * @param file Output file.
   */
  void serialize(std::unique_ptr<mysqlshdk::storage::IFile> file) const;

  /**
   * Reads checksum information from the given file.
   *
   * @param file Input file.
   */
  void deserialize(std::unique_ptr<mysqlshdk::storage::IFile> file);

 private:
  // schema -> table -> info
  using Schemas_t =
      std::unordered_map<std::string,
                         std::unordered_map<std::string, Table_info>>;

  // partition name -> chunk -> checksum
  using Partitions_t =
      std::unordered_map<std::string,
                         std::unordered_map<int64_t, Checksum_data>>;

  void initialize(Table_info *info) const;

  std::string select_expr(const std::vector<std::string> &values) const;

  std::string generator(const std::vector<std::string> &values) const;

  struct Table_info {
    std::string_view schema;
    std::string_view table;

    struct {
      std::string select_expr;
      std::string from;
      std::string where;
      std::string order_by;
    } query;

    std::vector<std::string> columns;
    std::vector<std::string> index_columns;
    std::vector<std::string> null_columns;

    Partitions_t partitions;
  };

  // read from the file
  Hash m_hash;
  Algorithm m_algorithm;
  Schemas_t m_schemas;
  mysqlshdk::utils::Version m_version;

  // computed
  std::string m_select_expr_template;
  std::string m_generator_template;
  std::vector<std::string> m_extra_queries;
};

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COMMON_DUMP_CHECKSUMS_H_
