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
#include "mysqlshdk/libs/oci/oci_options.h"

#include <map>
#include <regex>
#include <vector>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/libs/oci/oci_rest_service.h"
#include "mysqlshdk/libs/oci/oci_setup.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/utils_time.h"

namespace mysqlshdk {
namespace oci {
const size_t KIB = 1024;
const size_t MIB = KIB * 1024;
const size_t DEFAULT_MULTIPART_PART_SIZE = MIB * 128;

using Status_code = Response::Status_code;

namespace {
void validate_config_profile(const std::string &config_path,
                             const std::string &profile) {
  mysqlshdk::oci::Oci_setup oci_setup(config_path);
  if (!oci_setup.has_profile(profile)) {
    throw std::runtime_error("The indicated OCI Profile does not exist.");
  }
}

bool parse_par(const std::string &par, std::vector<std::string> *data_ptr) {
  std::smatch results;
  if (std::regex_match(
          par, results,
          std::regex("^https:\\/\\/objectstorage\\.(.+)\\.oraclecloud\\.com\\/"
                     "p\\/.+\\/n\\/(.+)\\/b\\/"
                     "(.+)\\/o\\/.*$"))) {
    // Region
    data_ptr->push_back(results[1]);

    // Namespace
    data_ptr->push_back(results[2]);

    // Bucket
    data_ptr->push_back(results[3]);

    return true;
  }
  return false;
}
}  // namespace

std::mutex Oci_options::s_tenancy_name_mutex;
std::map<std::string, std::string> Oci_options::s_tenancy_names = {};

void Oci_options::load_defaults() {
  if (!par_data.empty()) {
    oci_region = par_data[0];
    os_namespace = par_data[1];
    os_bucket_name = par_data[2];
  } else {
    if (!operator bool()) {
      throw std::invalid_argument("The osBucketName option is missing.");
    }

    if (config_path.get_safe().empty()) {
      config_path = mysqlsh::current_shell_options()->get().oci_config_file;
    }

    if (config_profile.get_safe().empty()) {
      config_profile = mysqlsh::current_shell_options()->get().oci_profile;
    }

    if (!config_profile.get_safe().empty()) {
      validate_config_profile(*config_path, *config_profile);
    }

    if (os_namespace.get_safe().empty()) {
      Oci_rest_service object_storage(Oci_service::OBJECT_STORAGE, *this);
      std::lock_guard<std::mutex> lock(s_tenancy_name_mutex);
      if (s_tenancy_names.find(object_storage.get_tenancy_id()) ==
          s_tenancy_names.end()) {
        rest::String_response response;
        const auto &raw_data = response.buffer;
        auto request = Oci_request("/n/");

        object_storage.get(&request, &response);

        try {
          s_tenancy_names[object_storage.get_tenancy_id()] =
              shcore::Value::parse(raw_data.data(), raw_data.size())
                  .as_string();
        } catch (const shcore::Exception &error) {
          log_debug2("%s\n%.*s", error.what(),
                     static_cast<int>(raw_data.size()), raw_data.data());
          throw;
        }
      }

      os_namespace = s_tenancy_names.at(object_storage.get_tenancy_id());
    }
  }

  if (target == OBJECT_STORAGE) {
    if (oci_par_expire_time.is_null()) {
      // If not specified sets the expiration time to NOW + 1 Week
      oci_par_expire_time =
          shcore::future_time_rfc3339(std::chrono::hours(24 * 7));
    }
  }

  if (part_size.is_null()) {
    part_size = DEFAULT_MULTIPART_PART_SIZE;
  }
}

void Oci_options::on_unpacked_options() {
  if (os_bucket_name.get_safe().empty()) {
    if (!os_namespace.get_safe().empty()) {
      throw std::invalid_argument(
          "The option 'osNamespace' cannot be used when the value of "
          "'osBucketName' option is not set.");
    }

    if (!config_path.get_safe().empty()) {
      throw std::invalid_argument(
          "The option 'ociConfigFile' cannot be used when the value of "
          "'osBucketName' option is not set.");
    }

    if (!config_profile.get_safe().empty()) {
      throw std::invalid_argument(
          "The option 'ociProfile' cannot be used when the value of "
          "'osBucketName' option is not set.");
    }

    if (target == OBJECT_STORAGE && !oci_par_manifest.is_null()) {
      throw std::invalid_argument(
          "The option 'ociParManifest' cannot be used when the value of "
          "'osBucketName' option is not set.");
    }
  }

  if (target == OBJECT_STORAGE) {
    if (!oci_par_manifest.get_safe() &&
        !oci_par_expire_time.get_safe().empty()) {
      throw std::invalid_argument(
          "The option 'ociParExpireTime' cannot be used when the value "
          "of 'ociParManifest' option is not True.");
    }
  }
}

void Oci_options::check_option_values() {
  if (!os_bucket_name.get_safe().empty() || !par_data.empty()) {
    load_defaults();
  }
}

void Oci_options::set_par(const mysqlshdk::null_string &url) {
  assert(target != OBJECT_STORAGE_NO_PAR_SUPPORT);

  if (!url.get_safe().empty()) {
    if (parse_par(*url, &par_data)) {
      if (!os_namespace.is_null() && *os_namespace != par_data[1]) {
        throw std::invalid_argument(
            "The option 'osNamespace' doesn't match the namespace of the "
            "provided pre-authenticated request.");
      }
      if (!os_bucket_name.is_null() && *os_bucket_name != par_data[2]) {
        throw std::invalid_argument(
            "The option 'osBucketName' doesn't match the bucket name of "
            "the provided pre-authenticated request.");
      }
    }
  }

  os_par = url;
}

bool parse_oci_options(
    const std::string &path,
    const std::unordered_map<std::string, std::string> &in_options,
    Oci_options *out_options) {
  assert(out_options);

  const auto scheme = mysqlshdk::storage::utils::get_scheme(path);

  // The osBucketName only should be used when passing a raw path
  bool os_bucket_found = in_options.find(kOsBucketName) != in_options.end();

  if (!scheme.empty() && os_bucket_found) {
    throw std::runtime_error(
        "The option osBucketName can not be used when the path contains a "
        "scheme.'");
  }

  if (os_bucket_found) {
    out_options->os_bucket_name = in_options.at(mysqlshdk::oci::kOsBucketName);

    auto it = in_options.find(mysqlshdk::oci::kOsNamespace);

    if (it != in_options.end()) {
      out_options->os_namespace = it->second;
    }

    it = in_options.find(mysqlshdk::oci::kOciConfigFile);

    if (it != in_options.end()) {
      out_options->config_path = it->second;
    }

    it = in_options.find(mysqlshdk::oci::kOciProfile);

    if (it != in_options.end()) {
      out_options->config_profile = it->second;
    }

    out_options->check_option_values();
  }

  return os_bucket_found;
}

std::string Oci_options::to_string(Unpack_target target) {
  switch (target) {
    case OBJECT_STORAGE:
      return "OBJECT STORAGE";

    case OBJECT_STORAGE_NO_PAR_OPTIONS:
      return "OBJECT STORAGE NO PAR OPTIONS";

    case OBJECT_STORAGE_NO_PAR_SUPPORT:
      return "OBJECT STORAGE NO PAR SUPPORT";
  }

  throw std::logic_error("Should not happen");
}

const std::string &Oci_options::get_hash() const {
  if (m_hash.empty()) {
    m_hash.reserve(512);

    m_hash.append(to_string(target));
    m_hash.append(1, '-');
    m_hash.append(oci_region.get_safe());
    m_hash.append(1, '-');
    m_hash.append(os_namespace.get_safe());
    m_hash.append(1, '-');
    m_hash.append(os_bucket_name.get_safe());
    m_hash.append(1, '-');
    m_hash.append(config_path.get_safe());
    m_hash.append(1, '-');
    m_hash.append(config_profile.get_safe());
    m_hash.append(1, '-');
    m_hash.append(os_par.get_safe());
    m_hash.append(1, '-');
    m_hash.append(oci_par_expire_time.get_safe());
    m_hash.append(1, '-');
    m_hash.append(std::to_string(part_size.get_safe(0)));
    m_hash.append(1, '-');
    m_hash.append(std::to_string(oci_par_manifest.get_safe()));
  }

  return m_hash;
}

}  // namespace oci
}  // namespace mysqlshdk
