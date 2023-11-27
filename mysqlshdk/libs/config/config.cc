/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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

#include <algorithm>
#include <cassert>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace config {

std::optional<bool> Config::get_bool(const std::string &name) const {
  return m_config_handlers.at(m_default_handler)->get_bool(name);
}

std::optional<std::string> Config::get_string(const std::string &name) const {
  return m_config_handlers.at(m_default_handler)->get_string(name);
}

std::optional<int64_t> Config::get_int(const std::string &name) const {
  return m_config_handlers.at(m_default_handler)->get_int(name);
}

void Config::set(const std::string &name, std::optional<bool> value,
                 const std::string &context) {
  for (const auto &config_handler : m_config_handlers) {
    config_handler.second->set(name, value, context);
  }
}

void Config::set(const std::string &name, std::optional<int64_t> value,
                 const std::string &context) {
  for (const auto &config_handler : m_config_handlers) {
    config_handler.second->set(name, value, context);
  }
}

void Config::set(const std::string &name,
                 const std::optional<std::string> &value,
                 const std::string &context) {
  for (const auto &config_handler : m_config_handlers) {
    config_handler.second->set(name, value, context);
  }
}

void Config::apply(bool skip_default_value_check) {
  for (const auto &config_handler : m_config_handlers) {
    log_debug("Apply config changes for %s.", config_handler.first.c_str());
    config_handler.second->apply(skip_default_value_check);
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

std::optional<bool> Config::get_bool(const std::string &name,
                                     const std::string &handler_name) const {
  return m_config_handlers.at(handler_name)->get_bool(name);
}

std::optional<std::string> Config::get_string(
    const std::string &name, const std::string &handler_name) const {
  return m_config_handlers.at(handler_name)->get_string(name);
}

std::optional<int64_t> Config::get_int(const std::string &name,
                                       const std::string &handler_name) const {
  return m_config_handlers.at(handler_name)->get_int(name);
}

void Config::set_for_handler(const std::string &name, std::optional<bool> value,
                             const std::string &handler_name,
                             const std::string &context) {
  m_config_handlers.at(handler_name)->set(name, value, context);
}

void Config::set_for_handler(const std::string &name,
                             std::optional<int64_t> value,
                             const std::string &handler_name,
                             const std::string &context) {
  m_config_handlers.at(handler_name)->set(name, value, context);
}

void Config::set_for_handler(const std::string &name,
                             const std::optional<std::string> &value,
                             const std::string &handler_name,
                             const std::string &context) {
  m_config_handlers.at(handler_name)->set(name, value, context);
}

IConfig_handler *Config::get_handler(const std::string &handler_name) const {
  return m_config_handlers.at(handler_name).get();
}

std::vector<std::string> Config::list_handler_names() const {
  std::vector<std::string> handler_names;
  handler_names.reserve(m_config_handlers.size());

  for (const auto &map_entry : m_config_handlers) {
    handler_names.push_back(map_entry.first);
  }
  return handler_names;
}

void set_indexable_option(mysqlshdk::config::Config *config,
                          const std::string &option_name,
                          const std::optional<std::string> &option_value,
                          const std::string &context) {
  assert(config);
  assert(option_value);

  // Option can be an index value, in this case convert it to
  // an integer otherwise an SQL error will occur when using this value
  // (because it will try to set an int as as string).
  if (std::all_of(option_value->cbegin(), option_value->cend(), ::isdigit)) {
    int64_t value = std::stoll(*option_value);
    config->set(option_name, std::optional<int64_t>(value), context);
  } else {
    config->set(option_name, option_value, context);
  }
}

}  // namespace config
}  // namespace mysqlshdk
