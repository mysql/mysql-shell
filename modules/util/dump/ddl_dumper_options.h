/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_DDL_DUMPER_OPTIONS_H_
#define MODULES_UTIL_DUMP_DDL_DUMPER_OPTIONS_H_

#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/aws/s3_bucket_options.h"
#include "mysqlshdk/libs/azure/blob_storage_options.h"

#include "modules/util/dump/dump_manifest_options.h"
#include "modules/util/dump/dump_options.h"

namespace mysqlsh {
namespace dump {

using Object_storage_options =
    mysqlshdk::storage::backend::object_storage::Object_storage_options;

class Ddl_dumper_options : public Dump_options {
 public:
  Ddl_dumper_options(const Ddl_dumper_options &) = default;
  Ddl_dumper_options(Ddl_dumper_options &&) = default;

  Ddl_dumper_options &operator=(const Ddl_dumper_options &) = default;
  Ddl_dumper_options &operator=(Ddl_dumper_options &&) = default;

  virtual ~Ddl_dumper_options() = default;

  static const shcore::Option_pack_def<Ddl_dumper_options> &options();

  bool split() const override { return m_split; }

  uint64_t bytes_per_chunk() const override { return m_bytes_per_chunk; }

  std::size_t threads() const override { return m_threads; }

  std::size_t worker_threads() const override { return m_worker_threads; }

  bool is_export_only() const override { return false; }

  bool use_single_file() const override { return false; }

  bool dump_ddl() const override { return !m_data_only; }

  bool dump_data() const override { return !m_ddl_only; }

  bool consistent_dump() const override { return m_consistent_dump; }

  bool skip_consistency_checks() const override {
    return m_skip_consistency_checks;
  }

  bool dump_triggers() const override { return m_dump_triggers; }

  bool use_timezone_utc() const override { return m_timezone_utc; }

  bool dump_binlog_info() const override { return true; }

  bool par_manifest() const override {
    return m_dump_manifest_options.par_manifest();
  }

  bool checksum() const override { return m_checksum; }

  void enable_mds_compatibility_checks();
  using Dump_options::set_target_version;
  void set_output_url(const std::string &url) override;

 protected:
  Ddl_dumper_options();

  void on_set_session(
      const std::shared_ptr<mysqlshdk::db::ISession> &) override {}

  void on_unpacked_options();

 private:
  void set_bytes_per_chunk(const std::string &value);
  void set_ocimds(bool value);
  void set_compatibility_options(const std::vector<std::string> &options);
  void set_target_version_str(const std::string &value);
  void set_dry_run(bool dry_run);
  void set_threads(uint64_t threads);
  const Object_storage_options *object_storage_options() const;

  Dump_manifest_options m_dump_manifest_options;
  // this should be in the Dump_options class, but storing it at the same level
  // as OCI options helps in handling both option groups at the same time
  mysqlshdk::aws::S3_bucket_options m_s3_bucket_options;
  mysqlshdk::azure::Blob_storage_options m_blob_storage_options;

  bool m_split = true;
  uint64_t m_bytes_per_chunk;

  // Number of threads requested by the user (or default)
  // At most this number of database connections will be used in the dump
  // operation.
  uint64_t m_threads = 4;

  // Internal number of threads (to be doubled in the case of prefix PAR dumps)
  uint64_t m_worker_threads = 4;

  bool m_dump_triggers = true;
  bool m_timezone_utc = true;
  bool m_ddl_only = false;
  bool m_data_only = false;
  bool m_consistent_dump = true;
  bool m_skip_consistency_checks = false;
  bool m_checksum = false;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DDL_DUMPER_OPTIONS_H_
