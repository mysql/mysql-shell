/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include <Windows.h>

#include <Msi.h>
#include <MsiQuery.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace {

constexpr auto k_all_users = "ALLUSERS";
constexpr auto k_all_users_old = "ALLUSERS_OLD";
constexpr auto k_upgrade_code = "UpgradeCode";

template <typename... Args>
[[nodiscard]] std::string format(const char *fmt, Args &&... args) {
  const auto size = snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);

  if (size < 0) {
    return fmt;
  }

  std::string result;
  result.resize(size);
  snprintf(result.data(), size + 1, fmt, std::forward<Args>(args)...);

  return result;
}

class Msi {
 public:
  explicit Msi(MSIHANDLE handle) : m_handle(handle) {}

  [[nodiscard]] std::string property(const char *name) const {
    std::string result;
    DWORD length = 0;

    // get length of the property
    auto rc = MsiGetPropertyA(m_handle, name, result.data(), &length);

    if (ERROR_MORE_DATA == rc) {
      result.resize(length);
      // add one for null character
      ++length;

      rc = MsiGetPropertyA(m_handle, name, result.data(), &length);
    }

    if (ERROR_SUCCESS != rc) {
      throw std::runtime_error(
          format("Failed to get property '%s': %u", name, rc));
    }

    result.resize(length);
    return result;
  }

  void set_property(const char *name, const char *value) {
    const auto rc = MsiSetPropertyA(m_handle, name, value);

    if (ERROR_SUCCESS != rc) {
      throw std::runtime_error(
          format("Failed to set property '%s' to '%s': %u", name, value, rc));
    }
  }

  inline void set_property(const char *name, const std::string &value) {
    set_property(name, value.c_str());
  }

  inline void clear_property(const char *name) { set_property(name, ""); }

  void log(const char *msg) const noexcept {
    PMSIHANDLE record = MsiCreateRecord(0);

    if (ERROR_SUCCESS != MsiRecordSetStringA(record, 0, msg)) {
      return;
    }

    MsiProcessMessage(m_handle, INSTALLMESSAGE_INFO, record);
  }

  inline void log(const std::string &msg) const noexcept { log(msg.c_str()); }

 private:
  MSIHANDLE m_handle;
};

[[nodiscard]] std::string product_property(const char *product_code,
                                           const char *name) {
  std::string result;
  DWORD length = 0;

  // get length of the property
  auto rc = MsiGetProductInfoA(product_code, name, result.data(), &length);

  if (ERROR_MORE_DATA == rc) {
    result.resize(length);
    // add one for null character
    ++length;

    rc = MsiGetProductInfoA(product_code, name, result.data(), &length);
  }

  if (ERROR_SUCCESS != rc) {
    throw std::runtime_error(
        format("Failed to get product property '%s': %u", name, rc));
  }

  result.resize(length);
  return result;
}

[[nodiscard]] inline std::string product_property(
    const std::string &product_code, const char *name) {
  return product_property(product_code.c_str(), name);
}

[[nodiscard]] inline bool is_per_machine(const std::string &p) {
  // ALLUSERS:
  //  - empty string - per-user installation
  //  - 1 - per-machine installation
  //  - 2 - per-machine or per-user, depending on user's privileges and version
  //        of Windows
  // AssignmentType:
  //  - 0 - per-user installation
  //  - 1 - per-machine installation
  return "1" == p;
}

}  // namespace

extern "C" __declspec(dllexport) UINT __stdcall SetAllUsers(MSIHANDLE handle) {
  Msi msi{handle};

  try {
    const auto all_users = msi.property(k_all_users);
    msi.log(format("%s has the value: '%s'", k_all_users, all_users.c_str()));

    const auto upgrade_code = msi.property(k_upgrade_code);
    msi.log(format("Searching for related products using upgrade code: '%s'",
                   upgrade_code.c_str()));

    std::string product_code;
    // GUID is 38 characters long (+ NULL character)
    product_code.resize(38);

    if (ERROR_SUCCESS != MsiEnumRelatedProductsA(upgrade_code.c_str(), 0, 0,
                                                 product_code.data())) {
      msi.log("No related products found");
      return ERROR_SUCCESS;
    }

    msi.log(format("Found related product: '%s'", product_code.c_str()));

    const auto assignment_type =
        product_property(product_code.c_str(), INSTALLPROPERTY_ASSIGNMENTTYPE);
    msi.log(format("%s has the value: '%s'", INSTALLPROPERTY_ASSIGNMENTTYPE,
                   assignment_type.c_str()));

    const auto installation_per_machine = is_per_machine(all_users);
    const auto installed_per_machine = is_per_machine(assignment_type);

    if (installation_per_machine == installed_per_machine) {
      msi.log(format("%s property was not altered", k_all_users));
    } else {
      const auto all_users_new = installed_per_machine ? "1" : "";
      msi.set_property(k_all_users_old, installation_per_machine ? "1" : "0");
      msi.set_property(k_all_users, all_users_new);
      msi.log(format("%s has been set to: '%s'", k_all_users, all_users_new));
    }

    return ERROR_SUCCESS;
  } catch (const std::exception &e) {
    msi.log(format("SetAllUsers() - fatal failure: %s", e.what()));
    return ERROR_INSTALL_FAILURE;
  }
}

extern "C" __declspec(dllexport) UINT
    __stdcall RevertAllUsers(MSIHANDLE handle) {
  Msi msi{handle};

  try {
    const auto all_users = msi.property(k_all_users);
    msi.log(format("%s has the value: '%s'", k_all_users, all_users.c_str()));

    const auto all_users_old = msi.property(k_all_users_old);
    msi.log(format("%s has the value: '%s'", k_all_users_old,
                   all_users_old.c_str()));

    if (all_users_old.empty()) {
      msi.log(format("%s property was not altered", k_all_users));
    } else {
      const auto all_users_new = is_per_machine(all_users_old) ? "1" : "";
      msi.set_property(k_all_users, all_users_new);
      msi.clear_property(k_all_users_old);
      msi.log(
          format("%s has been restored to: '%s'", k_all_users, all_users_new));
    }

    return ERROR_SUCCESS;
  } catch (const std::exception &e) {
    msi.log(format("RevertAllUsers() - fatal failure: %s", e.what()));
    return ERROR_INSTALL_FAILURE;
  }
}
