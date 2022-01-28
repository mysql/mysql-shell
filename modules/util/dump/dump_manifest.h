/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_DUMP_MANIFEST_H_
#define MODULES_UTIL_DUMP_DUMP_MANIFEST_H_

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/storage/backend/object_storage.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

#include "modules/util/dump/dump_manifest_config.h"

namespace mysqlsh {
namespace dump {

using File_info = mysqlshdk::storage::IDirectory::File_info;

/**
 * Base class for Dump manifest handling
 */
class Manifest_base {
 public:
  Manifest_base(std::unique_ptr<mysqlshdk::storage::IFile> file_handle,
                const std::string &base_name)
      : m_file{std::move(file_handle)}, m_base_name(base_name) {}

  virtual bool has_object(const std::string &name) const;

  virtual const File_info &get_object(const std::string &name);

  std::unordered_set<File_info> list_objects();

  bool is_complete() const { return m_complete; }

  const std::string &base_name() const { return m_base_name; }

 protected:
  std::unique_ptr<mysqlshdk::storage::IFile> m_file;
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, File_info> m_objects;
  bool m_complete = false;

 private:
  std::string m_base_name;
};

/**
 * Handles the creation of the dump manifest
 */
class Manifest_writer final : public Manifest_base {
 public:
  Manifest_writer(std::unique_ptr<mysqlshdk::storage::IFile> file_handle,
                  const std::string &base_name,
                  const std::string &par_expire_time);

  void cache_par(mysqlshdk::oci::PAR &&par);

  void add_par(mysqlshdk::oci::PAR &&par) { m_pars_queue.push(std::move(par)); }

  std::string serialize() const;

  void flush();

  void finalize();

 private:
  std::string m_par_expire_time;
  std::vector<mysqlshdk::oci::PAR> m_par_cache;
  std::unique_ptr<std::thread> m_par_thread;
  std::string m_par_thread_error;
  std::string m_start_time;
  shcore::Synchronized_queue<mysqlshdk::oci::PAR> m_pars_queue;
};

/**
 * Handles reading the Dump manifest
 */
class Manifest_reader final : public Manifest_base {
 public:
  Manifest_reader(std::unique_ptr<mysqlshdk::storage::IFile> file_handle,
                  const std::string &base_name)
      : Manifest_base(std::move(file_handle), base_name) {}

  const File_info &get_object(const std::string &name) override;

  void unserialize(const std::string &data);

  void reload();

  bool exists() const { return m_file->exists(); }

  mysqlshdk::Masked_string full_path() const { return m_file->full_path(); }

 private:
  size_t m_last_manifest_size = 0;
};

/**
 * This class handles a dump manifest and for such reason it handles two
 * behaviors:
 *
 * - WRITE Mode: Used when creating the manifest i.e. in a dump operation
 * - READ Mode: Used when loading a dump using a manifest
 *
 * When in WRITE mode, the class behaves like a normal OCI directory
 *
 * - Bucket operations are authenticated using an OCI Profile
 * - Object creation is identical to when using the OCI Dictionary with the
 *   addition of the PAR creation for each created object.
 *
 * When in READ mode, the class emulates a directory by loading the files from a
 * dump manifest. For that reason:
 *
 * - The class should be initialized (name) using a manifest PAR
 * - Supports reading files contained on the manifest using the PARs contained
 *   inside
 * - Supports external file read/write operations as long as the files are
 *   created using a PAR with the required permissions.
 */
class Dump_manifest_writer
    : public mysqlshdk::storage::backend::object_storage::Directory {
 public:
  explicit Dump_manifest_writer(
      const Dump_manifest_write_config_ptr &config,
      const std::string &name = {},
      const std::shared_ptr<Manifest_writer> &writer = {});

  ~Dump_manifest_writer() override;

  /**
   * Creates a file handle either for a file contained on the manifest or
   * external file using a PAR as name.
   */
  std::unique_ptr<mysqlshdk::storage::IFile> file(
      const std::string &name,
      const mysqlshdk::storage::File_options &options = {}) const override;

  void close() override;

  bool is_local() const override { return false; }

 private:
  void finalize();

  std::shared_ptr<Manifest_writer> m_writer;
  Dump_manifest_write_config_ptr m_config;
};

class Dump_manifest_reader : public mysqlshdk::storage::IDirectory {
 public:
  Dump_manifest_reader(const Dump_manifest_read_config_ptr &config,
                       const std::shared_ptr<Manifest_reader> &reader = {});

  ~Dump_manifest_reader() override = default;

  void close() override {}

  bool is_local() const override { return false; }

  bool exists() const override { return m_reader->exists(); }

  mysqlshdk::Masked_string full_path() const override {
    return m_reader->full_path();
  }

  std::string join_path(const std::string &a,
                        const std::string &b) const override;

  std::unique_ptr<mysqlshdk::storage::IFile> file(
      const std::string &name,
      const mysqlshdk::storage::File_options &options = {}) const override;

  std::unordered_set<File_info> list_files(
      bool hidden_files = false) const override;

  void create() override {
    throw std::logic_error("Dump_manifest_reader::create() - not implemented");
  }

  std::unordered_set<File_info> filter_files(
      const std::string &) const override {
    throw std::logic_error(
        "Dump_manifest_reader::filter_files() - not implemented");
  }

 private:
  std::shared_ptr<Manifest_reader> m_reader;
  Dump_manifest_read_config_ptr m_config;
  mutable std::unordered_map<std::string, File_info> m_created_objects;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_MANIFEST_H_
