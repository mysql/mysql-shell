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
#ifndef MYSQLSHDK_LIBS_OCI_OCI_OPTIONS_H_
#define MYSQLSHDK_LIBS_OCI_OCI_OPTIONS_H_

#include <string>
#include <unordered_map>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
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
  Oci_options() = default;
  Oci_options(const Oci_options &) = default;
  Oci_options(Oci_options &&) = default;

  Oci_options &operator=(const Oci_options &) = default;
  Oci_options &operator=(Oci_options &&) = default;

  enum Unpack_target {
    OBJECT_STORAGE,
    OBJECT_STORAGE_NO_PAR_OPTIONS,
    OBJECT_STORAGE_NO_PAR_SUPPORT
  };

  static std::string to_string(Unpack_target target);

  explicit operator bool() const {
    return !os_bucket_name.get_safe().empty() || !os_par.get_safe().empty();
  }

  void on_unpacked_options();
  void validate();
  void check_option_values();

  Unpack_target target = OBJECT_STORAGE;

  mysqlshdk::utils::nullable<std::string> os_bucket_name;
  mysqlshdk::utils::nullable<std::string> os_namespace;
  mysqlshdk::utils::nullable<std::string> config_path;
  mysqlshdk::utils::nullable<std::string> config_profile;
  mysqlshdk::utils::nullable<size_t> part_size;
  mysqlshdk::utils::nullable<bool> oci_par_manifest;
  mysqlshdk::utils::nullable<std::string> oci_par_expire_time;

  // Internal use options
  mysqlshdk::utils::nullable<std::string> oci_region;

  void set_par(const mysqlshdk::null_string &url);

  const mysqlshdk::null_string &get_par() const { return os_par; }

  const std::string &get_hash() const;

 private:
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
  void load_defaults();

  mysqlshdk::utils::nullable<std::string> os_par;
  std::vector<std::string> par_data;
  mutable std::string m_hash;
};

/**
 * Oci option pack that enables/disables allowed options based on the provided
 * Unpack_target
 */
template <Oci_options::Unpack_target T>
struct Oci_option_unpacker : public Oci_options {
  Oci_option_unpacker() { target = T; }

  static const shcore::Option_pack_def<Oci_option_unpacker> &options() {
    static shcore::Option_pack_def<Oci_option_unpacker> opts;

    if (T == Oci_options::Unpack_target::OBJECT_STORAGE) {
      static const auto with_par_opts =
          shcore::Option_pack_def<Oci_option_unpacker>()
              .optional(kOsNamespace,
                        &Oci_option_unpacker<T>::set_string_option)
              .optional(kOsBucketName,
                        &Oci_option_unpacker<T>::set_string_option)
              .optional(kOciConfigFile,
                        &Oci_option_unpacker<T>::set_string_option)
              .optional(kOciProfile, &Oci_option_unpacker<T>::set_string_option)
              .optional(kOciParExpireTime,
                        &Oci_option_unpacker<T>::set_string_option)
              .optional(kOciParManifest,
                        &Oci_option_unpacker<T>::set_par_manifest)
              .on_done(&Oci_option_unpacker<T>::on_unpacked_options);
      return with_par_opts;
    } else {
      static const auto no_par_opts =
          shcore::Option_pack_def<Oci_option_unpacker>()
              .optional(kOsNamespace,
                        &Oci_option_unpacker<T>::set_string_option)
              .optional(kOsBucketName,
                        &Oci_option_unpacker<T>::set_string_option)
              .optional(kOciConfigFile,
                        &Oci_option_unpacker<T>::set_string_option)
              .optional(kOciProfile, &Oci_option_unpacker<T>::set_string_option)
              .on_done(&Oci_option_unpacker<T>::on_unpacked_options);

      return no_par_opts;
    }
  }

 private:
  void set_string_option(const std::string &option, const std::string &value) {
    if (option == kOsNamespace)
      os_namespace = value;
    else if (option == kOsBucketName)
      os_bucket_name = value;
    else if (option == kOciConfigFile)
      config_path = value;
    else if (option == kOciProfile)
      config_profile = value;
    else if (option == kOciParExpireTime)
      oci_par_expire_time = value;
    else
      // This function should only be called with the options above.
      assert(false);
  }

  void set_par_manifest(const std::string & /*option*/, const bool &value) {
    oci_par_manifest = value;
  }
};

/**
 * Reviews in_options to pull from it any OCI option such as osBucketName and
 * osNamespace, returns true if the osNamespace option was found.
 **/
bool parse_oci_options(
    const std::string &path,
    const std::unordered_map<std::string, std::string> &in_options,
    Oci_options *out_options);
}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_OPTIONS_H_
