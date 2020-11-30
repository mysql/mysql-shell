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

#include "modules/util/dump/dump_manifest.h"

#include <ctime>
#include <regex>
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

const std::regex k_full_par_parser(
    "^https:\\/\\/objectstorage\\.(.+)\\.oraclecloud\\.com(\\/"
    "p\\/.+\\/n\\/(.+)\\/b\\/(.*)\\/o\\/((.*)\\/)?(.+))$");

FI_DEFINE(par_manifest, ([](const mysqlshdk::utils::FI::Args &args) {
            throw mysqlshdk::oci::Response_error(
                static_cast<mysqlshdk::oci::Response::Status_code>(
                    args.get_int("code")),
                args.get_string("msg"));
          }));

bool parse_full_object_par(const std::string &url, Par_structure *data) {
  std::smatch results;
  if (std::regex_match(url, results, k_full_par_parser)) {
    data->region = results[1];
    data->object_url = results[2];
    data->ns_name = results[3];
    data->bucket = results[4];
    data->object_prefix = results[6];
    data->object_name = results[7];

    return true;
  }
  return false;
}

using mysqlshdk::storage::IDirectory;
using mysqlshdk::storage::Mode;

Dump_manifest::Dump_manifest(Mode mode,
                             const mysqlshdk::oci::Oci_options &options,
                             const std::string &name)
    : Directory(options, name), m_mode(mode) {
  // This class acts in dual mode and the type is selected by the name provided
  // when opening the directory: If the name is a PAR, then it opens in READ
  // mode, otherwise it opens in WRITE mode.
  //
  // READ Mode: Expects the PAR to be to a manifest file, the directory will
  // read data from the manifest and cause the returned files to read their data
  // using the associated PAR in the manifest.
  //
  // WRITE Mode: acts as a normal OCI Directory excepts that will automatically
  // create a PAR for each object created and a manifest file containing the
  // relation between files and PARs.
  if (m_mode == Mode::READ) {
    Par_structure data;
    if (parse_full_object_par(name, &data) &&
        data.object_name == k_at_manifest_json) {
      m_region = data.region;
      m_manifest_url = data.object_url;

      // The directory name
      m_name = data.object_prefix;
    } else {
      // Any other name is not supported.
      throw std::runtime_error(
          shcore::str_format("Invalid manifest PAR: %s", name.c_str()));
    }
  } else {
    m_par_thread =
        std::make_unique<std::thread>(mysqlsh::spawn_scoped_thread([this]() {
          const auto five_seconds = std::chrono::milliseconds(5000);
          auto last_update = std::chrono::system_clock::now();
          auto wait_for = five_seconds;
          size_t last_count = 0;
          bool test = false;

          m_start_time = shcore::current_time_rfc3339();

          while (!m_full_manifest) {
            auto par = m_pars_queue.try_pop(wait_for.count());

            if (par.id == "DONE" && !test) {
              // TODO(rennox): If this is reached then it means the manifest was
              // not completed, so we can fall in the following scenarios:
              // - The PAR cache was completely flushed into the manifest in
              //   which case all the data to do cleanup is there.
              // - The cache contains PARs that were not yet flushed, so the
              //   data is not visible anywhere.
              //
              // If any cleanup is to be done, this is the place. OTOH the pars
              // are prefixed with shell-dump-<prefix> so they can be
              // identified.
              break;
            } else {
              // If PAR is received, adds it to the cache
              if (!par.name.empty()) {
                if (shcore::str_endswith(par.name, k_at_done_json))
                  m_full_manifest = true;
                m_par_cache.emplace_back(std::move(par));
              }

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
                  flush_manifest();
                } catch (const std::runtime_error &error) {
                  m_par_thread_error = error.what();
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
}

Dump_manifest::~Dump_manifest() {
  try {
    finalize();
  } catch (const std::runtime_error &error) {
    log_error("%s", error.what());
  }
}

std::unique_ptr<mysqlshdk::storage::IFile> Dump_manifest::file(
    const std::string &name, const mysqlshdk::storage::File_options &) const {
  std::string object_name(name);

  std::string prefix;
  if (!m_name.empty()) prefix = m_name + "/";

  if (m_mode == Mode::READ) {
    // In READ mode, if a PAR is provided (i.e. a PAR to the progress file),
    // then the file is "created" and it is kept on a different registry (i.e.
    // not part of the manifest)
    Par_structure data;
    if (parse_full_object_par(name, &data)) {
      if (data.region == m_region &&
          data.ns_name == *m_bucket->get_options().os_namespace &&
          data.bucket == *m_bucket->get_options().os_bucket_name &&
          data.object_prefix == m_name) {
        object_name = data.object_name;
        m_created_objects[object_name] = {data.object_url, 0};
        prefix = "";
      } else {
        throw std::runtime_error(shcore::str_format(
            "The provided PAR must be a file on the dump location: '%s'",
            name.c_str()));
      }
    }
  }

  return std::make_unique<Dump_manifest_object>(
      const_cast<Dump_manifest *>(this), m_bucket->get_options(), object_name,
      prefix);
}

void Dump_manifest::flush_manifest() {
  // Avoids creating an empty manifest file
  if (m_par_cache.empty()) return;

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
  json.append_string(*m_bucket->get_options().oci_par_expire_time);
  json.append_string("lastUpdate");
  json.append_string(last_update);
  json.append_string("startTime");
  json.append_string(m_start_time);
  if (m_full_manifest) {
    json.append_string("endTime");
    json.append_string(last_update);
  }

  // Ends the manifest object
  json.end_object();

  const std::string &data = json.str();

  try {
    auto file = Directory::file(k_at_manifest_json);
    file->open(mysqlshdk::storage::Mode::WRITE);
    file->write(data.c_str(), data.length());
    file->close();
  } catch (const std::runtime_error &error) {
    m_par_thread_error = error.what();
  }
}

IDirectory::File_info Dump_manifest::get_object(const std::string &name,
                                                bool fetch_size) const {
  // This method is ONLY available on READ mode
  assert(m_mode == Mode::READ);

  {
    auto created_object = m_created_objects.find(name);

    if (created_object != m_created_objects.end()) {
      auto &info = created_object->second;

      if (fetch_size) {
        info.size = m_bucket->head_object(info.name);
      }

      return info;
    }
  }

  {
    std::lock_guard<std::mutex> lock(m_manifest_mutex);
    const auto manifest_object = m_manifest_objects.find(name);

    if (manifest_object != m_manifest_objects.end()) {
      return manifest_object->second;
    }
  }

  return {};
}

bool Dump_manifest::has_object(const std::string &name,
                               bool fetch_if_needed) const {
  return !get_object(name, fetch_if_needed).name.empty();
}

/**
 * Returns true if the file with the given name exists either on the manifest
 * or on the files created with this class and actually exists on the bucket.
 * (i.e. a progress file for a resuming load)
 */
bool Dump_manifest::file_exists(const std::string &name) const {
  try {
    return has_object(name, true);
  } catch (const mysqlshdk::rest::Response_error &error) {
    if (error.code() == mysqlshdk::rest::Response::Status_code::NOT_FOUND) {
      return false;
    } else {
      throw;
    }
  }
}

size_t Dump_manifest::file_size(const std::string &name) const {
  const auto info = get_object(name, true);

  if (!info.name.empty()) {
    return info.size;
  }

  // This must never happen
  throw std::logic_error(
      shcore::str_format("Unknown object on size request : %s", name.c_str()));
}

IDirectory::File_info Dump_manifest::list_file(const std::string &object_name) {
  if (!has_object(object_name)) {
    reload_manifest();
  }

  return get_object(object_name);
}

void Dump_manifest::reload_manifest() const {
  // This function is ONLY available on READ mode
  assert(m_mode == Mode::READ);

  // If the manifest is fully loaded, avoids rework
  if (m_full_manifest) return;

  std::lock_guard<std::mutex> lock(m_manifest_mutex);

  mysqlshdk::rest::String_buffer manifest_data;
  auto manifest_size = m_bucket->get_object(m_manifest_url, &manifest_data);

  m_manifest_objects.clear();

  if (manifest_size) {
    auto manifest_map = shcore::Value::parse(manifest_data.data()).as_map();
    auto contents = manifest_map->get_array("contents");

    for (const auto &val : (*contents)) {
      auto entry = val.as_map();
      auto object_name = entry->get_string("objectName");
      m_manifest_objects[object_name] = {
          entry->get_string("parUrl"),
          static_cast<size_t>(entry->get_int("objectSize"))};

      // When the @.done.json is found on the manifest, it's fully loaded
      if (object_name == join_path(m_name, k_at_done_json))
        m_full_manifest = true;
    }
  }
}

void Dump_manifest::close() {
  try {
    finalize();
  } catch (const std::runtime_error &error) {
    log_error("%s", error.what());
  }
}

/**
 * Ensures the par thread is properly finished and ensures the remaining cached
 * PARs are flushed into the manifest.
 */
void Dump_manifest::finalize() {
  if (m_par_thread) {
    mysqlshdk::oci::PAR done_guard;
    done_guard.id = "DONE";
    m_pars_queue.push(std::move(done_guard));
    m_par_thread->join();
    m_par_thread.reset();
    flush_manifest();
  }
}

std::vector<IDirectory::File_info> Dump_manifest::list_files(
    bool hidden_files) const {
  if (m_mode == Mode::WRITE) {
    return Directory::list_files(hidden_files);
  } else {
    reload_manifest();

    std::vector<IDirectory::File_info> ret_val;

    std::lock_guard<std::mutex> lock(m_manifest_mutex);
    for (const auto &entry : m_manifest_objects) {
      std::string object_name(entry.first);
      if (!m_name.empty()) {
        object_name = object_name.substr(m_name.size() + 1);
      }
      // The manifest also contains the bucket write PAR which has an empty
      // name, it should not be returned on the file list
      if (!entry.first.empty())
        ret_val.push_back({std::move(object_name), entry.second.size});
    }
    return ret_val;
  }
}

Dump_manifest_object::Dump_manifest_object(
    Dump_manifest *manifest, const mysqlshdk::oci::Oci_options &options,
    const std::string &name, const std::string &prefix)
    : Object(options, name, prefix), m_manifest(manifest) {}

std::string Dump_manifest_object::full_path() const {
  if (m_manifest->mode() == Dump_manifest::Mode::READ) {
    return m_manifest->list_file(Object::full_path()).name;
  } else {
    return Object::full_path();
  }
}

void Dump_manifest_object::open(mysqlshdk::storage::Mode mode) {
  if (m_manifest->mode() == Dump_manifest::Mode::WRITE &&
      mode == mysqlshdk::storage::Mode::WRITE) {
    // The PAR creation is done in parallel
    auto options = m_bucket->get_options();
    m_par_thread = std::make_unique<std::thread>(mysqlsh::spawn_scoped_thread(
        [this, options]() { create_par(options); }));
  }

  Object::open(mode);
}

size_t Dump_manifest_object::file_size() const {
  if (m_manifest->mode() == Dump_manifest::Mode::READ) {
    return m_manifest->file_size(Object::full_path());
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

  if (m_manifest->mode() == Dump_manifest::Mode::WRITE) {
    if (m_par) {
      m_manifest->add_par(*m_par.get());
    } else {
      try {
        this->remove();
      } catch (const mysqlshdk::oci::Response_error &error) {
        m_par_thread_error = error.what();
        log_error("Failed removing object '%s' after PAR creation failed: %s",
                  Object::full_path().c_str(), error.what());
      } catch (const mysqlshdk::rest::Connection_error &error) {
        m_par_thread_error = error.what();
        log_error("Failed removing object '%s' after PAR creation failed: %s",
                  Object::full_path().c_str(), error.what());
      }
    }
  }

  Object::close();

  // throw only after everything is cleaned up
  if (m_manifest->mode() == Dump_manifest::Mode::WRITE && !m_par) {
    throw std::runtime_error(
        shcore::str_format("Failed creating PAR for object '%s': %s",
                           object_name().c_str(), m_par_thread_error.c_str()));
  }
}

bool Dump_manifest_object::exists() const {
  if (m_manifest->mode() == Dump_manifest::Mode::READ) {
    return m_manifest->file_exists(m_name);
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
  auto name = Object::full_path();
  if (shcore::str_endswith(name, k_dumping_suffix)) {
    name = shcore::str_replace(name, k_dumping_suffix, "");
  }

  return name;
}

}  // namespace dump
}  // namespace mysqlsh
