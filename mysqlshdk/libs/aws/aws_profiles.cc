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

#include "mysqlshdk/libs/aws/aws_profiles.h"

#include <utility>

namespace mysqlshdk {
namespace aws {

Profile::Profile(std::string name) : m_name(std::move(name)) {}

void Profile::set(std::string name, std::string value) {
  if ("aws_access_key_id" == name) {
    m_access_key_id = std::move(value);
  } else if ("aws_secret_access_key" == name) {
    m_secret_access_key = std::move(value);
  } else if ("aws_session_token" == name) {
    m_session_token = std::move(value);
  } else {
    m_settings.emplace(std::move(name), std::move(value));
  }
}

const std::string *Profile::get(const std::string &name) const {
  const auto setting = m_settings.find(name);
  return m_settings.end() == setting ? nullptr : &setting->second;
}

bool Profile::has(const std::string &name) const {
  return m_settings.end() != m_settings.find(name);
}

bool Profiles::exists(const std::string &name) const {
  return m_profiles.end() != m_profiles.find(name);
}

const Profile *Profiles::get(const std::string &name) const {
  const auto profile = m_profiles.find(name);
  return m_profiles.end() == profile ? nullptr : &profile->second;
}

Profile *Profiles::get_or_create(const std::string &name) {
  return &m_profiles.try_emplace(name, name).first->second;
}

}  // namespace aws
}  // namespace mysqlshdk
