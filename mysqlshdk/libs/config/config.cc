/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/config/config.h"

#include <utility>

#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace config {

utils::nullable<bool> Config::get_bool(const std::string &name) const {
  return m_config_handlers.at(m_default_handler)->get_bool(name);
}

utils::nullable<std::string> Config::get_string(const std::string &name) const {
  return m_config_handlers.at(m_default_handler)->get_string(name);
}

utils::nullable<int64_t> Config::get_int(const std::string &name) const {
  return m_config_handlers.at(m_default_handler)->get_int(name);
}

void Config::set(const std::string &name, const utils::nullable<bool> &value) {
  for (const auto &config_handler : m_config_handlers) {
    config_handler.second->set(name, value);
  }
}

void Config::set(const std::string &name,
                 const utils::nullable<int64_t> &value) {
  for (const auto &config_handler : m_config_handlers) {
    config_handler.second->set(name, value);
  }
}

void Config::set(const std::string &name,
                 const utils::nullable<std::string> &value) {
  for (const auto &config_handler : m_config_handlers) {
    config_handler.second->set(name, value);
  }
}

void Config::apply() {
  for (const auto &config_handler : m_config_handlers) {
    log_debug("Apply config changes for %s.", config_handler.first.c_str());
    config_handler.second->apply();
  }
}

bool Config::has_handler(const std::string &handler_name) const {
  return m_config_handlers.find(handler_name) != m_config_handlers.end();
}

bool Config::add_handler(const std::string &handler_name,
                         std::unique_ptr<IConfig_handler> handler) {
  if (m_config_handlers.empty()) {
    m_default_handler = handler_name;
  }
  auto res = m_config_handlers.emplace(handler_name, std::move(handler));
  return res.second;
}

void Config::remove_handler(const std::string &handler_name) {
  if (m_config_handlers.find(handler_name) == m_config_handlers.end()) {
    throw std::out_of_range{"The specified configuration handler '" +
                            handler_name + "' does not exist."};
  }
  if (m_default_handler == handler_name) {
    throw std::runtime_error{
        "Cannot remove the default configuration handler '" + handler_name +
        "'."};
  }
  m_config_handlers.erase(handler_name);
}

void Config::clear_handlers() {
  m_config_handlers.clear();
  m_default_handler.clear();
}

void Config::set_default_handler(const std::string &handler_name) {
  if (m_config_handlers.find(handler_name) == m_config_handlers.end()) {
    throw std::out_of_range{"The specified configuration handler '" +
                            handler_name + "' does not exist."};
  }
  m_default_handler = handler_name;
}

utils::nullable<bool> Config::get_bool(const std::string &name,
                                       const std::string &handler_name) const {
  return m_config_handlers.at(handler_name)->get_bool(name);
}

utils::nullable<std::string> Config::get_string(
    const std::string &name, const std::string &handler_name) const {
  return m_config_handlers.at(handler_name)->get_string(name);
}

utils::nullable<int64_t> Config::get_int(
    const std::string &name, const std::string &handler_name) const {
  return m_config_handlers.at(handler_name)->get_int(name);
}

void Config::set(const std::string &name, const utils::nullable<bool> &value,
                 const std::string &handler_name) {
  m_config_handlers.at(handler_name)->set(name, value);
}

void Config::set(const std::string &name, const utils::nullable<int64_t> &value,
                 const std::string &handler_name) {
  m_config_handlers.at(handler_name)->set(name, value);
}

void Config::set(const std::string &name,
                 const utils::nullable<std::string> &value,
                 const std::string &handler_name) {
  m_config_handlers.at(handler_name)->set(name, value);
}

IConfig_handler *Config::get_handler(const std::string &handler_name) const {
  return m_config_handlers.at(handler_name).get();
}

std::vector<std::string> Config::list_handler_names() const {
  std::vector<std::string> handler_names;
  for (const auto &map_entry : m_config_handlers) {
    handler_names.push_back(map_entry.first);
  }
  return handler_names;
}
}  // namespace config
}  // namespace mysqlshdk
