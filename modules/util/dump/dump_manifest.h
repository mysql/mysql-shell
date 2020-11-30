/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/libs/storage/backend/oci_object_storage.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlsh {
namespace dump {
struct Par_structure {
  std::string region;
  std::string ns_name;
  std::string bucket;
  std::string object_url;
  std::string object_prefix;
  std::string object_name;
};

// Parses full object PAR, including REST end point.
bool parse_full_object_par(const std::string &url, Par_structure *data);

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
  enum class Mode { READ, WRITE };
  Dump_manifest(Mode mode, const mysqlshdk::oci::Oci_options &options,
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

  Mode mode() const { return m_mode; }

  bool is_local() const override { return false; }

 private:
  friend class Dump_manifest_object;

  // Members for WRITE mode
  void add_par(const mysqlshdk::oci::PAR &par) { m_pars_queue.push(par); }
  void flush_manifest();
  void finalize();

  // Members for READ mode
  File_info get_object(const std::string &name, bool fetch_size = false) const;
  bool has_object(const std::string &name, bool fetch_if_needed = false) const;
  void reload_manifest() const;
  File_info list_file(const std::string &object_name);
  bool file_exists(const std::string &name) const;
  size_t file_size(const std::string &name) const;

  // Global Members
  Mode m_mode;
  mutable bool m_full_manifest = false;

  // Attributes for WRITE mode
  shcore::Synchronized_queue<mysqlshdk::oci::PAR> m_pars_queue;
  std::vector<mysqlshdk::oci::PAR> m_par_cache;
  std::unique_ptr<std::thread> m_par_thread;
  std::string m_par_thread_error;
  std::string m_start_time;

  // Attributes for READ mode
  std::string m_manifest_url;
  std::string m_region;
  mutable std::mutex m_manifest_mutex;
  mutable std::map<std::string, File_info> m_manifest_objects;
  mutable std::map<std::string, File_info> m_created_objects;
};

class Dump_manifest_object : public mysqlshdk::storage::backend::oci::Object {
 public:
  Dump_manifest_object(Dump_manifest *manifest,
                       const mysqlshdk::oci::Oci_options &options,
                       const std::string &name, const std::string &prefix = "");

  ~Dump_manifest_object() override = default;

  std::string full_path() const override;

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

 private:
  void create_par(const mysqlshdk::oci::Oci_options &options);
  std::string object_name() const;

  Dump_manifest *m_manifest;
  mysqlshdk::utils::nullable<size_t> m_size;
  std::unique_ptr<std::thread> m_par_thread;
  std::unique_ptr<mysqlshdk::oci::PAR> m_par;
  std::string m_par_thread_error;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_MANIFEST_H_
