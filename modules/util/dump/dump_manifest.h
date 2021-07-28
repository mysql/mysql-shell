/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/storage/backend/oci_object_storage.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlsh {
namespace dump {

using mysqlshdk::storage::backend::oci::Par_structure;
using mysqlshdk::storage::backend::oci::Par_type;
using File_info = mysqlshdk::storage::IDirectory::File_info;

enum class Manifest_mode { READ, WRITE };

/**
 * Base class for Dump manifest handling
 */
class Manifest_base {
 public:
  Manifest_base(Manifest_mode mode,
                std::unique_ptr<mysqlshdk::storage::IFile> file_handle)
      : m_mode(mode), m_file{std::move(file_handle)} {}
  virtual bool has_object(const std::string &name) const;
  bool is_complete() const { return m_complete; }
  virtual const File_info &get_object(const std::string &name);
  std::vector<File_info> list_objects();
  virtual const std::string &get_base_name() const = 0;
  Manifest_mode get_mode() const { return m_mode; }

 protected:
  Manifest_mode m_mode;
  std::unique_ptr<mysqlshdk::storage::IFile> m_file;
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, File_info> m_objects;
  bool m_complete = false;
};

/**
 * Handles the creation of the dump manifest
 */
class Manifest_writer final : public Manifest_base {
 public:
  Manifest_writer(const std::string &base_name,
                  const std::string &par_expire_time,
                  std::unique_ptr<mysqlshdk::storage::IFile> file_handle);
  void cache_par(const mysqlshdk::oci::PAR &par);
  void add_par(const mysqlshdk::oci::PAR &par) { m_pars_queue.push(par); }
  std::string serialize() const;
  void flush();
  void finalize();
  const std::string &get_base_name() const override { return m_base_name; }

 private:
  std::string m_base_name;
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
  Manifest_reader(const Par_structure &par_data,
                  std::unique_ptr<mysqlshdk::storage::IFile> file_handle)
      : Manifest_base(Manifest_mode::READ, std::move(file_handle)),
        m_meta(par_data) {}

  const std::string &get_base_name() const override {
    return m_meta.object_prefix;
  }

  const File_info &get_object(const std::string &name) override;
  void unserialize(const std::string &data);
  const std::string &get_object_prefix() const { return m_meta.object_prefix; }
  const std::string &url() const { return m_meta.full_url; }
  const std::string &region() const { return m_meta.region; }
  void reload();

 private:
  Par_structure m_meta;
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
class Dump_manifest : public mysqlshdk::storage::backend::oci::Directory {
 public:
  Dump_manifest(Manifest_mode mode, const mysqlshdk::oci::Oci_options &options,
                const std::shared_ptr<Manifest_base> &manifest = nullptr,
                const std::string &name = "");
  ~Dump_manifest() override;

  /**
   * Creates a file handle either for a file contained on the manifest or
   * external file using a PAR as name.
   */
  std::unique_ptr<mysqlshdk::storage::IFile> file(
      const std::string &name,
      const mysqlshdk::storage::File_options &options = {}) const override;

  void close() override;

  /**
   * Retrieves the list of files on the manifest.
   */
  std::vector<File_info> list_files(bool hidden_files = false) const override;

  bool is_local() const override { return false; }

  bool has_object(const std::string &name) const;

  const File_info &get_object(const std::string &name,
                              bool fetch_size = false) const;

 private:
  friend class Dump_manifest_object;

  // Members for WRITE mode
  void finalize();

  // Members for READ mode
  bool file_exists(const std::string &name) const;

  // Global Members
  std::shared_ptr<Manifest_base> m_manifest;
  mutable std::unordered_map<std::string, File_info> m_created_objects;
};

class Dump_manifest_object : public mysqlshdk::storage::backend::oci::Object {
 public:
  Dump_manifest_object(const std::shared_ptr<Manifest_base> &manifest,
                       const mysqlshdk::oci::Oci_options &options,
                       const std::string &name, const std::string &prefix = "");

  ~Dump_manifest_object() override = default;

  mysqlshdk::Masked_string full_path() const override;

  /**
   * Opens the object for data operations:
   *
   * @param mode indicates the mode in which the object will be used.
   *
   * Valid modes include:
   *
   * - WRITE: Used to create or overwrite an object.
   * - APPEND: used to continue writing the parts of a multipart object that has
   * not been completed.
   * - READ: Used to read the content of the object.
   */
  void open(mysqlshdk::storage::Mode mode) override;

  /**
   * Returns the size of the object.
   *
   * If the object was opened in READ mode it returns the total size of the
   * object, otherwize returns the number of bytes written so far.
   */
  size_t file_size() const override;

  /**
   * Indicates whether the object already exists in the bucket or not.
   */
  bool exists() const override;

  /**
   * Finishes the operation being done on the object.
   *
   * If the object was opened in WRITE or APPEND mode this will flush the object
   * data and complete the object.
   */
  void close() override;

  std::unique_ptr<mysqlshdk::storage::IDirectory> parent() const override;

 private:
  void create_par(const mysqlshdk::oci::Oci_options &options);
  std::string object_name() const;

  std::shared_ptr<Manifest_base> m_manifest;
  mysqlshdk::utils::nullable<size_t> m_size;
  std::unique_ptr<std::thread> m_par_thread;
  std::unique_ptr<mysqlshdk::oci::PAR> m_par;
  std::string m_par_thread_error;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_MANIFEST_H_
