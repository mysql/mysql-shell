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
#ifndef MYSQLSHDK_LIBS_OCI_OCI_OPTIONS_H_
#define MYSQLSHDK_LIBS_OCI_OCI_OPTIONS_H_

#include <string>
#include <unordered_map>

#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/shellcore/wizard.h"

namespace mysqlshdk {
namespace oci {

constexpr const char kOsNamespace[] = "osNamespace";
constexpr const char kOsBucketName[] = "osBucketName";
constexpr const char kOciConfigFile[] = "ociConfigFile";
constexpr const char kOciProfile[] = "ociProfile";
constexpr const char kOciParManifest[] = "ociParManifest";
constexpr const char kOciParExpireTime[] = "ociParExpireTime";
// Internal Options, not exposed to the user
constexpr const char kOsPar[] = "osPar";
constexpr const char kOciRegion[] = "ociRegion";

enum class Oci_uri_type { FILE, DIRECTORY };

struct Oci_options {
  Oci_options(const Oci_options &) = default;
  Oci_options(Oci_options &&) = default;

  Oci_options &operator=(const Oci_options &) = default;
  Oci_options &operator=(Oci_options &&) = default;

  enum Unpack_target {
    OBJECT_STORAGE,
    OBJECT_STORAGE_NO_PAR_OPTIONS,
    OBJECT_STORAGE_NO_PAR_SUPPORT
  };

  Oci_options() : Oci_options(OBJECT_STORAGE) {}

  explicit Oci_options(Unpack_target t) : target(t) {}

  template <typename Unpacker>
  Unpacker &unpack(Unpacker *options) {
    do_unpack(options);
    return *options;
  }

  explicit operator bool() const {
    return !os_bucket_name.get_safe().empty() || !os_par.get_safe().empty();
  }

  void check_bucket_name_dependent_options();
  void check_option_values();

  Unpack_target target;

  mysqlshdk::utils::nullable<std::string> os_bucket_name;
  mysqlshdk::utils::nullable<std::string> os_namespace;
  mysqlshdk::utils::nullable<std::string> config_path;
  mysqlshdk::utils::nullable<std::string> config_profile;
  mysqlshdk::utils::nullable<size_t> part_size;
  mysqlshdk::utils::nullable<bool> oci_par_manifest;
  mysqlshdk::utils::nullable<std::string> oci_par_expire_time;

  // Internal use options
  mysqlshdk::utils::nullable<std::string> os_par;
  mysqlshdk::utils::nullable<std::string> oci_region;

 private:
  void do_unpack(shcore::Option_unpacker *unpacker);

  static std::mutex s_tenancy_name_mutex;
  static std::map<std::string, std::string> s_tenancy_names;

  /**
   * Read the Object_storage options from the default location:
   *
   * - Shell Option: oci.configFile
   * - Shell Option: oci.Profile
   *
   * @param instance target Instance object to read the GR options.
   */
  void load_defaults(const std::vector<std::string> &par_data);
};

bool parse_oci_options(
    Oci_uri_type type, const std::string &in_path,
    const std::unordered_map<std::string, std::string> &in_options,
    Oci_options *out_options, std::string *out_path);
}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_OPTIONS_H_
