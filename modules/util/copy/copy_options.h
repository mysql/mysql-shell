/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_COPY_COPY_OPTIONS_H_
#define MODULES_UTIL_COPY_COPY_OPTIONS_H_

#include <limits>
#include <type_traits>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"

#include "modules/util/dump/ddl_dumper_options.h"
#include "modules/util/load/load_dump_options.h"

namespace mysqlsh {
namespace copy {

template <class T, std::enable_if_t<
                       std::is_base_of_v<dump::Ddl_dumper_options, T>, int> = 0>
class Copy_options {
 public:
  Copy_options(const Copy_options &) = default;
  Copy_options(Copy_options &&) = default;

  Copy_options &operator=(const Copy_options &) = default;
  Copy_options &operator=(Copy_options &&) = default;

  virtual ~Copy_options() = default;

  static const shcore::Option_pack_def<Copy_options> &options() {
    static const auto opts =
        shcore::Option_pack_def<Copy_options>()
            .template ignore<mysqlshdk::oci::Oci_bucket_options>()
            .template ignore<mysqlshdk::aws::S3_bucket_options>()
            .template ignore<mysqlshdk::azure::Blob_storage_options>()
            .template ignore<import_table::Dialect>()
            .ignore({"backgroundThreads", "characterSet", "compression",
                     "createInvisiblePKs", "loadData", "loadDdl", "loadUsers",
                     "ocimds", "progressFile", "resetProgress", "showMetadata",
                     "targetVersion", "waitDumpTimeout"})
            .include(&Copy_options::m_dump_options)
            .include(&Copy_options::m_load_options)
            .on_done(&Copy_options::on_unpacked_options);

    return opts;
  }

  T *dump_options() { return &m_dump_options; }
  Load_dump_options *load_options() { return &m_load_options; }

 protected:
  Copy_options() {
    on_unpacked_options();

    // data is not compressed
    m_dump_options.set_compression(mysqlshdk::storage::Compression::NONE);
    // we don't use index files, data is streamed directly from writer to reader
    m_dump_options.disable_index_files();
    // do not use the .dumping extension
    m_dump_options.dont_rename_data_files();

    // we're going to show the metadata manually
    m_load_options.set_show_metadata(false);
    // wait indefinitely
    m_load_options.set_dump_wait_timeout_ms(
        std::numeric_limits<uint64_t>::max());
    // disable the progress file
    m_load_options.set_progress_file({});
    // loader can divide data into sub-chunks on the fly, as it always receives
    // buffer filled with full rows
    m_load_options.enable_fast_sub_chunking();
  }

  void on_log_options(const char *msg) const {
    log_info("Copy options: %s", msg);
  }

 private:
  void on_unpacked_options() {
    // dumper in the dry run mode writes all the files, because loader needs
    // these files to simulate the load
    if (m_load_options.dry_run()) {
      m_dump_options.set_dry_run_mode(dump::Dry_run::WRITE_EMPTY_DATA_FILES);
    }

    // always disable load progress, to make output more readable
    m_load_options.set_show_progress(false);

    // make sure we don't use too many background threads
    m_load_options.set_background_threads_count(m_load_options.threads_count());
    m_load_options.set_load_data(m_dump_options.dump_data());
    m_load_options.set_load_ddl(m_dump_options.dump_ddl());
    m_load_options.set_load_users(m_dump_options.dump_users());
  }

  T m_dump_options;
  Load_dump_options m_load_options;
};

}  // namespace copy
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COPY_COPY_OPTIONS_H_
