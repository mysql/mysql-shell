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

#include "modules/util/dump/dump_manifest.h"

#include <chrono>
#include <ctime>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/oci/oci_bucket.h"
#include "mysqlshdk/libs/storage/backend/http.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_time.h"

#include "modules/util/dump/dump_errors.h"
#include "modules/util/load/load_errors.h"

static constexpr auto k_at_manifest_json = "@.manifest.json";
static constexpr auto k_par_prefix = "shell-dump-";
static constexpr auto k_dumping_suffix = ".dumping";
static constexpr auto k_at_done_json = "@.done.json";

namespace mysqlsh {
namespace dump {

FI_DEFINE(par_manifest, ([](const mysqlshdk::utils::FI::Args &args) {
            throw mysqlshdk::rest::Response_error(
                static_cast<mysqlshdk::rest::Response::Status_code>(
                    args.get_int("code")),
                args.get_string("msg"));
          }));

using mysqlshdk::storage::IDirectory;
using mysqlshdk::storage::Mode;

namespace {

class Dump_manifest_object
    : public mysqlshdk::storage::backend::object_storage::Object {
 public:
  Dump_manifest_object(const std::shared_ptr<Manifest_writer> &writer,
                       const Dump_manifest_write_config_ptr &config,
                       const std::string &name, const std::string &prefix);

  ~Dump_manifest_object() override = default;

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
   * Finishes the operation being done on the object.
   *
   * If the object was opened in WRITE or APPEND mode this will flush the object
   * data and complete the object.
   */
  void close() override;

  std::unique_ptr<mysqlshdk::storage::IDirectory> parent() const override;

 private:
  void create_par();
  std::string object_name() const;

  std::shared_ptr<Manifest_writer> m_writer;
  mysqlshdk::utils::nullable<size_t> m_size;
  std::unique_ptr<std::thread> m_par_thread;
  std::unique_ptr<mysqlshdk::oci::PAR> m_par;
  std::string m_par_thread_error;
  Dump_manifest_write_config_ptr m_config;
};

Dump_manifest_object::Dump_manifest_object(
    const std::shared_ptr<Manifest_writer> &writer,
    const Dump_manifest_write_config_ptr &config, const std::string &name,
    const std::string &prefix)
    : Object(config, name, prefix), m_writer(writer), m_config(config) {}

void Dump_manifest_object::open(mysqlshdk::storage::Mode mode) {
  // The PAR creation is done in parallel
  m_par_thread = std::make_unique<std::thread>(
      mysqlsh::spawn_scoped_thread([this]() { create_par(); }));

  Object::open(mode);
}

void Dump_manifest_object::close() {
  if (m_par_thread) {
    m_par_thread->join();
    m_par_thread.reset();

    // Getting the file size when a writer is open takes the value from the
    // writer rather than sending another request to the server
    if (m_par) {
      m_par->size = file_size();
    }
  }

  {
    if (m_par) {
      m_writer->add_par(std::move(*m_par));
    } else {
      try {
        this->remove();
      } catch (const mysqlshdk::rest::Response_error &error) {
        log_error("Failed removing object '%s' after PAR creation failed: %s",
                  Object::full_path().masked().c_str(), error.what());
      } catch (const mysqlshdk::rest::Connection_error &error) {
        log_error("Failed removing object '%s' after PAR creation failed: %s",
                  Object::full_path().masked().c_str(), error.what());
      }
    }
  }

  Object::close();

  // throw only after everything is cleaned up
  if (!m_par) {
    THROW_ERROR(SHERR_DUMP_MANIFEST_PAR_CREATION_FAILED, object_name().c_str(),
                m_par_thread_error.c_str());
  }
}

void Dump_manifest_object::create_par() {
  const auto name = object_name();

  try {
    FI_TRIGGER_TRAP(par_manifest,
                    mysqlshdk::utils::FI::Trigger_options({{"name", name}}));

    const auto bucket = m_config->oci_bucket();
    const auto par_name = k_par_prefix + name;

    auto par = bucket->create_pre_authenticated_request(
        mysqlshdk::oci::PAR_access_type::OBJECT_READ,
        m_config->par_expire_time(), par_name, name);
    m_par = std::make_unique<mysqlshdk::oci::PAR>(std::move(par));
  } catch (const mysqlshdk::rest::Response_error &error) {
    m_par_thread_error = error.what();
    log_error("Error creating PAR for object '%s': %s", name.c_str(),
              error.what());
  } catch (const mysqlshdk::rest::Connection_error &error) {
    m_par_thread_error = error.what();
    log_error("Error creating PAR for object '%s': %s", name.c_str(),
              error.what());
  }
}

std::string Dump_manifest_object::object_name() const {
  auto name = Object::full_path().real();

  if (shcore::str_endswith(name, k_dumping_suffix)) {
    name = name.substr(0, name.length() - std::strlen(k_dumping_suffix));
  }

  return name;
}

std::unique_ptr<mysqlshdk::storage::IDirectory> Dump_manifest_object::parent()
    const {
  // if the full path does not contain a backslash then the parent directory is
  // the root directory
  return std::make_unique<Dump_manifest_writer>(m_config, m_writer->base_name(),
                                                m_writer);
}

class Http_manifest_object : public mysqlshdk::storage::backend::Http_object {
 public:
  Http_manifest_object(const std::shared_ptr<Manifest_reader> &reader,
                       const Dump_manifest_read_config_ptr &config,
                       const std::string &full_name);

  ~Http_manifest_object() override = default;

  /**
   * Returns the total size of the object.
   */
  size_t file_size() const override;

  /**
   * Indicates whether the object already exists in the bucket or not.
   */
  bool exists() const override;

  std::unique_ptr<mysqlshdk::storage::IDirectory> parent() const override;

 private:
  std::shared_ptr<Manifest_reader> m_reader;
  Dump_manifest_read_config_ptr m_config;
  std::string m_full_name;
};

Http_manifest_object::Http_manifest_object(
    const std::shared_ptr<Manifest_reader> &reader,
    const Dump_manifest_read_config_ptr &config, const std::string &full_name)
    : Http_object(
          mysqlshdk::oci::anonymize_par(config->par().endpoint() +
                                        reader->get_object(full_name).name()),
          true),
      m_reader(reader),
      m_config(config),
      m_full_name(full_name) {}

size_t Http_manifest_object::file_size() const {
  return m_reader->get_object(m_full_name).size();
}

bool Http_manifest_object::exists() const {
  bool exists = m_reader->has_object(m_full_name);

  if (!exists && !m_reader->is_complete()) {
    m_reader->reload();
    exists = m_reader->has_object(m_full_name);
  }

  return exists;
}

std::unique_ptr<mysqlshdk::storage::IDirectory> Http_manifest_object::parent()
    const {
  // if the full path does not contain a backslash then the parent directory is
  // the root directory
  return std::make_unique<Dump_manifest_reader>(m_config, m_reader);
}

}  // namespace

bool Manifest_base::has_object(const std::string &name) const {
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_objects.find(name) != m_objects.end();
}

const IDirectory::File_info &Manifest_base::get_object(
    const std::string &name) {
  std::lock_guard<std::mutex> lock(m_mutex);
  const auto object = m_objects.find(name);

  if (object != m_objects.end()) {
    return object->second;
  } else {
    THROW_ERROR(SHERR_LOAD_MANIFEST_UNKNOWN_OBJECT, name.c_str());
  }
}

Manifest_writer::Manifest_writer(
    std::unique_ptr<mysqlshdk::storage::IFile> file_handle,
    const std::string &base_name, const std::string &par_expire_time)
    : Manifest_base(std::move(file_handle), base_name),
      m_par_expire_time(par_expire_time) {
  m_par_thread =
      std::make_unique<std::thread>(mysqlsh::spawn_scoped_thread([this]() {
        const auto five_seconds = std::chrono::milliseconds(5000);
        auto last_update = std::chrono::system_clock::now();
        auto wait_for = five_seconds;
        size_t last_count = 0;
        bool test = false;

        m_start_time = shcore::current_time_rfc3339();

        while (!is_complete()) {
          auto par = m_pars_queue.try_pop(wait_for.count());

          if (par.id == "DONE" && !test) {
            // TODO(rennox): If this is reached then it means the manifest
            // was not completed, so we can fall in the following scenarios:
            // - The PAR cache was completely flushed into the manifest in
            //   which case all the data to do cleanup is there.
            // - The cache contains PARs that were not yet flushed, so the
            //   data is not visible anywhere.
            //
            // If any cleanup is to be done, this is the place. OTOH the
            // pars are prefixed with shell-dump-<prefix> so they can be
            // identified.
            flush();
            break;
          } else {
            // If PAR is received, adds it to the cache
            cache_par(std::move(par));

            auto waited_for =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - last_update);

            // The manifest will be flushed
            // - When the first PAR arrives
            // - When close() is called
            // - Or if waited for five seconds and new PARs arrived in the
            //   mean time
            if (last_count == 0 ||
                (waited_for >= five_seconds &&
                 last_count < m_par_cache.size()) ||
                is_complete()) {
              last_update = std::chrono::system_clock::now();
              try {
                flush();
              } catch (const std::runtime_error &error) {
                m_par_thread_error = shcore::str_format(
                    "Error flushing dump manifest: %s", error.what());
                break;
              }
              last_count = m_par_cache.size();
              wait_for = five_seconds;
            } else {
              wait_for = five_seconds - waited_for;
            }
          }
        }
      }));
}

void Manifest_writer::cache_par(mysqlshdk::oci::PAR &&par) {
  if (!par.name.empty()) {
    if (shcore::str_endswith(par.name, k_at_done_json)) m_complete = true;
    m_par_cache.emplace_back(std::move(par));
  }
}

std::string Manifest_writer::serialize() const {
  shcore::JSON_dumper json(true);

  // Starts the manifest object
  json.start_object();
  json.append_string("contents");

  // Starts the contents array
  json.start_array();
  for (const auto &par : m_par_cache) {
    json.start_object();
    json.append_string("parId");
    json.append_string(par.id);
    json.append_string("parUrl");
    json.append_string(par.access_uri);
    json.append_string("objectName");
    json.append_string(par.object_name);
    json.append_string("objectSize");
    json.append_int(static_cast<int>(par.size));
    json.end_object();
  }
  // contents array
  json.end_array();
  auto last_update = shcore::current_time_rfc3339();
  json.append_string("expireTime");
  json.append_string(m_par_expire_time);
  json.append_string("lastUpdate");
  json.append_string(last_update);
  json.append_string("startTime");
  json.append_string(m_start_time);
  if (m_complete) {
    json.append_string("endTime");
    json.append_string(last_update);
  }

  // Ends the manifest object
  json.end_object();

  return json.str();
}

void Manifest_writer::flush() {
  // Avoids creating an empty manifest file
  if (m_par_cache.empty()) return;

  std::string data = serialize();

  try {
    m_file->open(mysqlshdk::storage::Mode::WRITE);
    m_file->write(data.c_str(), data.length());
    m_file->close();
  } catch (const std::runtime_error &error) {
    m_par_thread_error = error.what();
  }
}

/**
 * Ensures the par thread is properly finished and ensures the remaining cached
 * PARs are flushed into the manifest.
 */
void Manifest_writer::finalize() {
  if (m_par_thread) {
    mysqlshdk::oci::PAR done_guard;
    done_guard.id = "DONE";
    m_pars_queue.push(std::move(done_guard));
    m_par_thread->join();
    m_par_thread.reset();
  }
}

const IDirectory::File_info &Manifest_reader::get_object(
    const std::string &name) {
  if (!has_object(name) && !is_complete()) {
    reload();
  }

  return Manifest_base::get_object(name);
}

void Manifest_reader::unserialize(const std::string &data) {
  std::lock_guard<std::mutex> lock(m_mutex);

  auto manifest_map = shcore::Value::parse(data).as_map();

  {
    const auto expire_time = manifest_map->get_string("expireTime");

    if (std::chrono::system_clock::now() >=
        shcore::rfc3339_to_time_point(expire_time)) {
      THROW_ERROR(SHERR_LOAD_MANIFEST_EXPIRED_PARS, expire_time.c_str());
    }
  }

  m_objects.clear();

  const auto contents = manifest_map->get_array("contents");
  const auto done_path = base_name() + k_at_done_json;

  for (const auto &val : (*contents)) {
    const auto entry = val.as_map();
    auto object_name = entry->get_string("objectName");
    const auto done_found = object_name == done_path;

    m_objects.emplace(
        std::move(object_name),
        File_info{entry->get_string("parUrl"),
                  static_cast<size_t>(entry->get_int("objectSize"))});

    // When the @.done.json is found on the manifest, it's fully loaded
    if (done_found) m_complete = true;
  }
}

void Manifest_reader::reload() {
  // If the manifest is fully loaded, avoids rework
  if (is_complete()) return;

  m_file->open(mysqlshdk::storage::Mode::READ);
  size_t new_size = m_file->file_size();

  if (m_last_manifest_size != new_size) {
    std::string buffer;
    buffer.resize(new_size);
    m_last_manifest_size = new_size;

    m_file->read(&buffer[0], new_size);
    unserialize(buffer);
  }

  m_file->close();
}

std::unordered_set<IDirectory::File_info> Manifest_base::list_objects() {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::unordered_set<IDirectory::File_info> ret_val;
  for (const auto &entry : m_objects) {
    // The manifest also contains the bucket write PAR which has an empty
    // name, it should not be returned on the file list
    if (!entry.first.empty()) ret_val.emplace(entry.first, entry.second.size());
  }
  return ret_val;
}

Dump_manifest_writer::Dump_manifest_writer(
    const Dump_manifest_write_config_ptr &config, const std::string &name,
    const std::shared_ptr<Manifest_writer> &writer)
    : Directory(config, name), m_writer(writer), m_config(config) {
  if (!m_writer) {
    // This class is a Directory interface to handle dump and load operations
    // using Pre-authenticated requests (PARs), it works dual mode as follows:
    //
    // WRITE Mode: acts as a normal OCI Directory excepts that will
    // automatically create a PAR for each object created and a manifest file
    // containing the relation between files and PARs.
    m_writer = std::make_shared<Manifest_writer>(
        Directory::file(k_at_manifest_json), name, config->par_expire_time());
  }
}

Dump_manifest_writer::~Dump_manifest_writer() {
  try {
    m_writer->finalize();
  } catch (const std::runtime_error &error) {
    log_error("%s", error.what());
  }
}

std::unique_ptr<mysqlshdk::storage::IFile> Dump_manifest_writer::file(
    const std::string &name, const mysqlshdk::storage::File_options &) const {
  std::string prefix = m_name;

  if (!prefix.empty() && '/' != prefix.back()) {
    prefix += '/';
  }

  return std::make_unique<Dump_manifest_object>(m_writer, m_config, name,
                                                prefix);
}

void Dump_manifest_writer::close() {
  try {
    m_writer->finalize();
  } catch (const std::runtime_error &error) {
    log_error("%s", error.what());
  }
}

Dump_manifest_reader::Dump_manifest_reader(
    const Dump_manifest_read_config_ptr &config,
    const std::shared_ptr<Manifest_reader> &reader)
    : m_reader(reader), m_config(config) {
  if (!m_reader) {
    // This class is a Directory interface to handle dump and load operations
    // using Pre-authenticated requests (PARs), it works dual mode as follows:
    //
    // READ mode: Expects the name to be a PAR to be to a manifest file, the
    // directory will read data from the manifest and cause the returned files
    // to read their data using the associated PAR in the manifest.
    m_reader = std::make_shared<Manifest_reader>(
        std::make_unique<mysqlshdk::storage::backend::Http_object>(
            mysqlshdk::oci::anonymize_par(config->par().full_url), true),
        m_config->par().object_prefix);
  }
}

std::string Dump_manifest_reader::join_path(const std::string &a,
                                            const std::string &b) const {
  return a.empty() ? b : a + "/" + b;
}

std::unique_ptr<mysqlshdk::storage::IFile> Dump_manifest_reader::file(
    const std::string &name, const mysqlshdk::storage::File_options &) const {
  using mysqlshdk::oci::PAR_structure;
  using mysqlshdk::oci::PAR_type;
  // On read mode, the first file to be created is the handle for the manifest
  // file, if it has not been created then we skip the validation below for
  // additional files
  //
  // In READ mode, if a PAR is provided (i.e. a PAR to the progress file),
  // then the file is "created" and it is kept on a different registry (i.e.
  // not part of the manifest)
  PAR_structure data;

  if (parse_par(name, &data) != PAR_type::NONE) {
    if (data.region == m_config->par().region &&
        data.domain == m_config->par().domain &&
        data.ns_name == m_config->par().ns_name &&
        data.bucket == m_config->par().bucket &&
        data.object_prefix == m_config->par().object_prefix) {
      m_created_objects.emplace(std::move(data.object_name),
                                File_info{data.object_path(), 0});

      return std::make_unique<mysqlshdk::storage::backend::Http_object>(
          mysqlshdk::oci::anonymize_par(name), true);
    } else {
      THROW_ERROR(SHERR_LOAD_MANIFEST_PAR_MISMATCH, name.c_str());
    }
  } else {
    return std::make_unique<Http_manifest_object>(
        m_reader, m_config, m_config->par().object_prefix + name);
  }
}

std::unordered_set<File_info> Dump_manifest_reader::list_files(bool) const {
  if (!m_reader->is_complete()) {
    m_reader->reload();
  }

  // File list must exclude the PAR portion
  const auto prefix_length = m_config->par().object_prefix.length();
  std::unordered_set<IDirectory::File_info> files;

  for (const auto &object : m_reader->list_objects()) {
    files.emplace(object.name().substr(prefix_length), object.size());
  }

  return files;
}

}  // namespace dump
}  // namespace mysqlsh
