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

#include "modules/util/dump/dump_manifest.h"

#include <ctime>
#include <utility>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_time.h"

static constexpr auto k_at_manifest_json = "@.manifest.json";
static constexpr auto k_par_prefix = "shell-dump-";
static constexpr auto k_dumping_suffix = ".dumping";
static constexpr auto k_at_done_json = "@.done.json";

namespace mysqlsh {
namespace dump {

namespace {
std::shared_ptr<Manifest_reader> manifest_reader(
    const std::shared_ptr<Manifest_base> &manifest) {
  if (manifest->get_mode() == Manifest_mode::READ) {
    auto reader = std::dynamic_pointer_cast<Manifest_reader>(manifest);
    if (reader) return reader;
  }
  throw std::logic_error("Invalid manifest reader object");
}

std::shared_ptr<Manifest_writer> manifest_writer(
    const std::shared_ptr<Manifest_base> &manifest) {
  if (manifest->get_mode() == Manifest_mode::WRITE) {
    auto writer = std::dynamic_pointer_cast<Manifest_writer>(manifest);
    if (writer) return writer;
  }
  throw std::logic_error("Invalid manifest writer object");
}
}  // namespace

FI_DEFINE(par_manifest, ([](const mysqlshdk::utils::FI::Args &args) {
            throw mysqlshdk::oci::Response_error(
                static_cast<mysqlshdk::oci::Response::Status_code>(
                    args.get_int("code")),
                args.get_string("msg"));
          }));

using mysqlshdk::storage::IDirectory;
using mysqlshdk::storage::Mode;

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
    throw std::logic_error(
        shcore::str_format("Unknown object in manifest: %s", name.c_str()));
  }
}

Manifest_writer::Manifest_writer(
    const std::string &base_name, const std::string &par_expire_time,
    std::unique_ptr<mysqlshdk::storage::IFile> file_handle)
    : Manifest_base(Manifest_mode::WRITE, std::move(file_handle)),
      m_base_name(base_name),
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
            break;
          } else {
            // If PAR is received, adds it to the cache
            cache_par(par);

            auto waited_for =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - last_update);

            // The manifest will be flushed
            // - When the first PAR arrives
            // - When close() is called
            // - Or if waited for five seconds and new PARs arrived in the
            //   mean time
            if (last_count == 0 || (waited_for >= five_seconds &&
                                    last_count < m_par_cache.size())) {
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

void Manifest_writer::cache_par(const mysqlshdk::oci::PAR &par) {
  if (!par.name.empty()) {
    if (shcore::str_endswith(par.name, k_at_done_json)) m_complete = true;
    m_par_cache.emplace_back(par);
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
    flush();
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

  m_objects.clear();

  auto contents = manifest_map->get_array("contents");

  std::string done_path = m_meta.object_prefix.empty()
                              ? k_at_done_json
                              : m_meta.object_prefix + "/" + k_at_done_json;

  for (const auto &val : (*contents)) {
    auto entry = val.as_map();
    auto object_name = entry->get_string("objectName");
    m_objects[object_name] = {
        entry->get_string("parUrl"),
        static_cast<size_t>(entry->get_int("objectSize"))};

    // When the @.done.json is found on the manifest, it's fully loaded
    if (object_name == done_path) m_complete = true;
  }
}

void Manifest_reader::reload() {
  // If the manifest is fully loaded, avoids rework
  if (is_complete()) return;

  size_t new_size = m_file->file_size();

  if (m_last_manifest_size != new_size) {
    std::string buffer;
    buffer.resize(new_size);
    m_last_manifest_size = new_size;

    m_file->open(mysqlshdk::storage::Mode::READ);
    m_file->read(&buffer[0], new_size);
    unserialize(buffer);
  }
}

std::vector<IDirectory::File_info> Manifest_base::list_objects() {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<IDirectory::File_info> ret_val;
  for (const auto &entry : m_objects) {
    // The manifest also contains the bucket write PAR which has an empty
    // name, it should not be returned on the file list
    if (!entry.first.empty())
      ret_val.push_back({entry.first, entry.second.size});
  }
  return ret_val;
}

Dump_manifest::Dump_manifest(Manifest_mode mode,
                             const mysqlshdk::oci::Oci_options &options,
                             const std::shared_ptr<Manifest_base> &manifest,
                             const std::string &name)
    : Directory(options, name), m_manifest(manifest) {
  if (m_manifest == nullptr) {
    using mysqlshdk::storage::backend::oci::parse_par;

    // This class is a Directory interface to handle dump and load operations
    // using Pre-authenticated requests (PARs), it works dual mode as follows:
    //
    // READ Mode: Expects the name to be a PAR to be to a manifest file, the
    // directory will read data from the manifest and cause the returned files
    // to read their data using the associated PAR in the manifest.
    //
    // WRITE Mode: acts as a normal OCI Directory excepts that will
    // automatically create a PAR for each object created and a manifest file
    // containing the relation between files and PARs.
    if (mode == Manifest_mode::READ) {
      Par_structure data;
      Par_type par_type = parse_par(name, &data);

      if (par_type == Par_type::MANIFEST) {
        // The directory name (removes the trailing / from the prefix)
        m_name = "";
        if (!data.object_prefix.empty()) {
          m_name = data.object_prefix.substr(0, data.object_prefix.size() - 1);
        }
        m_manifest = std::make_shared<Manifest_reader>(
            data,
            std::make_unique<mysqlshdk::storage::backend::oci::Object>(name));
      } else {
        // Any other name is not supported.
        throw std::runtime_error(
            shcore::str_format("Invalid manifest PAR: %s", name.c_str()));
      }
    } else {
      m_manifest = std::make_shared<Manifest_writer>(
          name, *options.oci_par_expire_time,
          Directory::file(k_at_manifest_json));
    }
  }
}

Dump_manifest::~Dump_manifest() {
  try {
    if (m_manifest->get_mode() == Manifest_mode::WRITE) {
      manifest_writer(m_manifest)->finalize();
    }
  } catch (const std::runtime_error &error) {
    log_error("%s", error.what());
  }
}

std::unique_ptr<mysqlshdk::storage::IFile> Dump_manifest::file(
    const std::string &name, const mysqlshdk::storage::File_options &) const {
  std::string object_name(name);

  std::string prefix;
  if (!m_name.empty()) prefix = m_name + "/";

  if (m_manifest->get_mode() == Manifest_mode::READ) {
    // On read mode, the first file to be created is the handle for the manifest
    // file, if it has not been created then we skip the validation below for
    // additional files
    auto reader = manifest_reader(m_manifest);

    // In READ mode, if a PAR is provided (i.e. a PAR to the progress file),
    // then the file is "created" and it is kept on a different registry (i.e.
    // not part of the manifest)
    Par_structure data;
    Par_type par_type = parse_par(name, &data);
    if (par_type != Par_type::NONE) {
      if (data.region == reader->region() &&
          data.ns_name == *m_bucket->get_options().os_namespace &&
          data.bucket == *m_bucket->get_options().os_bucket_name &&
          data.object_prefix == prefix) {
        object_name = data.object_name;
        m_created_objects[object_name] = {data.get_object_path(), 0};
        // prefix = "";
        return std::make_unique<mysqlshdk::storage::backend::oci::Object>(name);
      } else {
        throw std::runtime_error(shcore::str_format(
            "The provided PAR must be a file on the dump location: '%s'",
            name.c_str()));
      }
    }
  }

  return std::make_unique<Dump_manifest_object>(
      m_manifest, m_bucket->get_options(), object_name, prefix);
}

const IDirectory::File_info &Dump_manifest::get_object(const std::string &name,
                                                       bool fetch_size) const {
  auto created_object = m_created_objects.find(name);

  if (created_object != m_created_objects.end()) {
    auto &info = created_object->second;

    if (fetch_size) {
      info.size = m_bucket->head_object(info.name);
    }

    return info;
  }

  // Returns the object if exists on the manifest
  return m_manifest->get_object(name);
}

bool Dump_manifest::has_object(const std::string &name) const {
  auto created_object = m_created_objects.find(name);

  return created_object != m_created_objects.end() ||
         m_manifest->has_object(name);
}

/**
 * Returns true if the file with the given name exists either on the manifest
 * or on the files created with this class and actually exists on the bucket.
 * (i.e. a progress file for a resuming load)
 */
bool Dump_manifest::file_exists(const std::string &name) const {
  try {
    bool exists = m_manifest->has_object(name);
    if (!exists && m_manifest->get_mode() == Manifest_mode::READ &&
        !m_manifest->is_complete()) {
      manifest_reader(m_manifest)->reload();
      exists = m_manifest->has_object(name);
    }
    return exists;
  } catch (const mysqlshdk::rest::Response_error &error) {
    if (error.code() == mysqlshdk::rest::Response::Status_code::NOT_FOUND) {
      return false;
    } else {
      throw;
    }
  }
}

void Dump_manifest::close() {
  try {
    if (m_manifest->get_mode() == Manifest_mode::WRITE) {
      manifest_writer(m_manifest)->finalize();
    }
  } catch (const std::runtime_error &error) {
    log_error("%s", error.what());
  }
}

std::vector<IDirectory::File_info> Dump_manifest::list_files(
    bool hidden_files) const {
  if (m_manifest->get_mode() == Manifest_mode::READ) {
    if (!m_manifest->is_complete()) {
      manifest_reader(m_manifest)->reload();
    }

    auto objects = m_manifest->list_objects();

    // File list must exclude the PAR portion
    size_t prefix_length = m_name.empty() ? 0 : m_name.size() + 1;
    std::vector<IDirectory::File_info> files;

    for (const auto &object : objects) {
      files.push_back({object.name.substr(prefix_length), object.size});
    }
    return files;
  } else {
    return Directory::list_files(hidden_files);
  }
}

Dump_manifest_object::Dump_manifest_object(
    const std::shared_ptr<Manifest_base> &manifest,
    const mysqlshdk::oci::Oci_options &options, const std::string &name,
    const std::string &prefix)
    : Object(options, name, prefix), m_manifest(manifest) {}

mysqlshdk::Masked_string Dump_manifest_object::full_path() const {
  auto full_path = Object::full_path();

  if (m_manifest->get_mode() == Manifest_mode::READ) {
    return mysqlshdk::oci::anonymize_par(
        manifest_reader(m_manifest)->get_object(full_path.real()).name);
  } else {
    return full_path;
  }
}

void Dump_manifest_object::open(mysqlshdk::storage::Mode mode) {
  if (m_manifest->get_mode() == Manifest_mode::WRITE) {
    // The PAR creation is done in parallel
    auto options = m_bucket->get_options();
    m_par_thread = std::make_unique<std::thread>(mysqlsh::spawn_scoped_thread(
        [this, options]() { create_par(options); }));
  }

  Object::open(mode);
}

size_t Dump_manifest_object::file_size() const {
  if (m_manifest->get_mode() == Manifest_mode::READ) {
    return manifest_reader(m_manifest)
        ->get_object(Object::full_path().real())
        .size;
  } else {
    return Object::file_size();
  }
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

  if (m_manifest->get_mode() == Manifest_mode::WRITE) {
    auto writer = manifest_writer(m_manifest);

    if (m_par) {
      writer->add_par(*m_par.get());
    } else {
      try {
        this->remove();
      } catch (const mysqlshdk::oci::Response_error &error) {
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
  if (m_manifest->get_mode() == Manifest_mode::WRITE && !m_par) {
    throw std::runtime_error(
        shcore::str_format("Failed creating PAR for object '%s': %s",
                           object_name().c_str(), m_par_thread_error.c_str()));
  }
}

bool Dump_manifest_object::exists() const {
  if (m_manifest->get_mode() == Manifest_mode::READ) {
    bool exists = m_manifest->has_object(m_name);
    if (!exists && !m_manifest->is_complete()) {
      manifest_reader(m_manifest)->reload();
      exists = m_manifest->has_object(m_name);
    }

    return exists;
  } else {
    return Object::exists();
  }
}

void Dump_manifest_object::create_par(
    const mysqlshdk::oci::Oci_options &options) {
  const auto name = object_name();

  try {
    FI_TRIGGER_TRAP(par_manifest,
                    mysqlshdk::utils::FI::Trigger_options({{"name", name}}));

    mysqlshdk::oci::Bucket bucket(options);

    std::string expire_time = *bucket.get_options().oci_par_expire_time;

    std::string par_name(k_par_prefix);
    par_name += name;

    auto par = bucket.create_pre_authenticated_request(
        mysqlshdk::oci::PAR_access_type::OBJECT_READ, expire_time.c_str(),
        par_name, name);

    m_par = std::make_unique<mysqlshdk::oci::PAR>(par);
  } catch (const mysqlshdk::oci::Response_error &error) {
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
    name = shcore::str_replace(name, k_dumping_suffix, "");
  }

  return name;
}

std::unique_ptr<mysqlshdk::storage::IDirectory> Dump_manifest_object::parent()
    const {
  // if the full path does not contain a backslash then the parent directory is
  // the root directory
  return std::make_unique<Dump_manifest>(m_manifest->get_mode(),
                                         m_bucket->get_options(), m_manifest,
                                         m_manifest->get_base_name());
}

}  // namespace dump
}  // namespace mysqlsh
