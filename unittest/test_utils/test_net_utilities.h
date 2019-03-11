/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef UNITTEST_TEST_UTILS_TEST_NET_UTILITIES_H_
#define UNITTEST_TEST_UTILS_TEST_NET_UTILITIES_H_

#include "mysqlshdk/libs/utils/utils_net.h"

#include <string>
#include <vector>

namespace tests {

/**
 * Network utilities used by the unit tests.
 */
class Test_net_utilities : public mysqlshdk::utils::Net {
 public:
  ~Test_net_utilities() override { remove(); }

  /**
   * Injects this instance of network utilities, replacing the default
   * behaviour.
   */
  void inject(const std::string &hostname, const shcore::Value &data,
              mysqlshdk::db::replay::Mode mode) {
    if (data)
      m_recorded_data = data.as_map();
    else
      m_recorded_data = shcore::make_dict();
    m_hostname = hostname;
    m_mode = mode;
    set(this);
  }

  shcore::Value get_recorded() const { return shcore::Value(m_recorded_data); }

  /**
   * Removes the injected instance, restoring the default behaviour.
   */
  void remove() {
    if (get() == this) set(nullptr);
  }

 protected:
  std::string m_hostname;
  mysqlshdk::db::replay::Mode m_mode;
  mutable shcore::Dictionary_t m_recorded_data;
  mutable std::map<std::string, size_t> m_recorded_data_index;

  /**
   * Allows to resolve the hostname stored by the shell test environment.
   */
  std::vector<std::string> resolve_hostname_ipv4_all_impl(
      const std::string &name) const override {
    std::vector<std::string> result;

    switch (m_mode) {
      case mysqlshdk::db::replay::Mode::Record:
        try {
          result = Net::resolve_hostname_ipv4_all_impl(name);
          add_recorded("resolve_hostname_ipv4_all:" + name, result);
        } catch (std::exception &) {
          add_recorded_exc("resolve_hostname_ipv4_all:" + name);
          throw;
        }
        break;
      case mysqlshdk::db::replay::Mode::Replay:
        result = get_recorded_strv("resolve_hostname_ipv4_all:" + name, true);
        break;
      case mysqlshdk::db::replay::Mode::Direct:
        result = Net::resolve_hostname_ipv4_all_impl(name);
        break;
    }
    return result;
  }

  std::vector<std::string> resolve_hostname_ipv6_all_impl(
      const std::string &name) const override {
    std::vector<std::string> result;

    switch (m_mode) {
      case mysqlshdk::db::replay::Mode::Record:
        try {
          result = Net::resolve_hostname_ipv6_all_impl(name);
          add_recorded("resolve_hostname_ipv6_all:" + name, result);
        } catch (std::exception &) {
          add_recorded_exc("resolve_hostname_ipv6_all:" + name);
          throw;
        }
        break;
      case mysqlshdk::db::replay::Mode::Replay:
        result = get_recorded_strv("resolve_hostname_ipv6_all:" + name, true);
        break;
      case mysqlshdk::db::replay::Mode::Direct:
        result = Net::resolve_hostname_ipv6_all_impl(name);
        break;
    }
    return result;
  }

  std::vector<std::string> resolve_hostname_ipv_any_all_impl(
      const std::string &name) const override {
    std::vector<std::string> result;

    switch (m_mode) {
      case mysqlshdk::db::replay::Mode::Record:
        try {
          result = Net::resolve_hostname_ipv_any_all_impl(name);
          add_recorded("resolve_hostname_ipv_any_all:" + name, result);
        } catch (std::exception &) {
          add_recorded_exc("resolve_hostname_ipv_any_all:" + name);
          throw;
        }
        break;
      case mysqlshdk::db::replay::Mode::Replay:
        result =
            get_recorded_strv("resolve_hostname_ipv_any_all:" + name, true);
        break;
      case mysqlshdk::db::replay::Mode::Direct:
        result = Net::resolve_hostname_ipv_any_all_impl(name);
        break;
    }
    return result;
  }

  bool is_local_address_impl(const std::string &address) const override {
    bool result = false;

    switch (m_mode) {
      case mysqlshdk::db::replay::Mode::Record:
        try {
          result = Net::is_local_address_impl(address);
          add_recorded("is_local_address:" + address, result);
        } catch (std::exception &) {
          add_recorded_exc("is_local_address:" + address);
          throw;
        }
        break;
      case mysqlshdk::db::replay::Mode::Replay:
        result = get_recorded_bool("is_local_address:" + address, true);
        break;
      case mysqlshdk::db::replay::Mode::Direct:
        result = Net::is_local_address_impl(address);
        break;
    }

    return result;
  }

  bool is_loopback_impl(const std::string &address) const override {
    bool result = false;

    switch (m_mode) {
      case mysqlshdk::db::replay::Mode::Record:
        try {
          result = Net::is_loopback_impl(address);
          add_recorded("is_loopback:" + address, result);
        } catch (std::exception &) {
          add_recorded_exc("is_loopback:" + address);
          throw;
        }
        break;
      case mysqlshdk::db::replay::Mode::Replay:
        result = get_recorded_bool("is_loopback:" + address, true);
        break;
      case mysqlshdk::db::replay::Mode::Direct:
        result = Net::is_loopback_impl(address);
        break;
    }

    return result;
  }

  std::string get_hostname_impl() const override { return m_hostname; }

  std::string get_fqdn_impl(const std::string &address) const
      noexcept override {
    const std::string key = "get_fqdn:" + address;
    switch (m_mode) {
      case mysqlshdk::db::replay::Mode::Record: {
        const std::string result = Net::get_fqdn(address);
        add_recorded(key, result);
        return result;
      }
      case mysqlshdk::db::replay::Mode::Replay:
        return get_recorded_string(key, true);
      case mysqlshdk::db::replay::Mode::Direct:
        return Net::get_fqdn(address);
    }
    return std::string{};
  }

  bool is_port_listening_impl(const std::string &address,
                              int port) const override {
    bool result = false;
    std::string key =
        "is_port_listening:" + address + "/" + std::to_string(port);

    switch (m_mode) {
      case mysqlshdk::db::replay::Mode::Record:
        try {
          result = Net::is_port_listening_impl(address, port);
          add_recorded(key, result);
        } catch (std::exception &) {
          add_recorded_exc(key);
          throw;
        }
        break;
      case mysqlshdk::db::replay::Mode::Replay:
        result = get_recorded_bool(key, false);
        break;
      case mysqlshdk::db::replay::Mode::Direct:
        result = Net::is_port_listening_impl(address, port);
        break;
    }

    return result;
  }

  bool get_recorded_bool(const std::string &key, bool is_static) const {
    if (!m_recorded_data->has_key(key))
      throw std::logic_error("Net_utils: Recorded data has no data for " + key);

    shcore::Array_t array = m_recorded_data->get_array(key);
    size_t idx = m_recorded_data_index[key];
    if (!array || idx >= array->size())
      throw std::logic_error(
          "Net_utils: Recorded data has not enough data for " + key);
    if (!is_static) m_recorded_data_index[key] = idx + 1;

    shcore::Value value = array->at(idx);
    if (value.type == shcore::Map) {
      if (value.as_map()->get_string("type") == "net_error")
        throw mysqlshdk::utils::net_error(value.as_map()->get_string("what"));
      else
        throw std::runtime_error(value.as_map()->get_string("what"));
    }
    return value.as_bool();
  }

  void add_recorded(const std::string &key, bool value) const {
    shcore::Array_t array;
    if (!m_recorded_data->has_key(key)) {
      array = shcore::make_array();
      m_recorded_data->set(key, shcore::Value(array));
    } else {
      array = m_recorded_data->get_array(key);
    }
    array->push_back(shcore::Value(value));
  }

  std::vector<std::string> get_recorded_strv(const std::string &key,
                                             bool is_static) const {
    if (!m_recorded_data->has_key(key))
      throw std::logic_error("Net_utils: Recorded data has no data for " + key);

    shcore::Array_t array = m_recorded_data->get_array(key);
    size_t idx = m_recorded_data_index[key];
    if (!array || idx >= array->size())
      throw std::logic_error(
          "Net_utils: Recorded data has not enough data for " + key);
    if (!is_static) m_recorded_data_index[key] = idx + 1;

    shcore::Value value = array->at(idx);
    if (value.type == shcore::Map) {
      if (value.as_map()->get_string("type") == "net_error")
        throw mysqlshdk::utils::net_error(value.as_map()->get_string("what"));
      else
        throw std::runtime_error(value.as_map()->get_string("what"));
    }

    std::vector<std::string> strv;
    shcore::Array_t a = value.as_array();
    for (size_t i = 0; i < a->size(); i++) {
      strv.push_back(a->at(i).as_string());
    }
    return strv;
  }

  std::string get_recorded_string(const std::string &key,
                                  bool is_static) const {
    if (!m_recorded_data->has_key(key))
      throw std::logic_error("Net_utils: Recorded data has no data for " + key);

    shcore::Array_t array = m_recorded_data->get_array(key);
    size_t idx = m_recorded_data_index[key];
    if (!array || idx >= array->size())
      throw std::logic_error(
          "Net_utils: Recorded data has not enough data for " + key);
    if (!is_static) m_recorded_data_index[key] = idx + 1;

    shcore::Value value = array->at(idx);
    if (value.type == shcore::Map) {
      if (value.as_map()->get_string("type") == "net_error")
        throw mysqlshdk::utils::net_error(value.as_map()->get_string("what"));
      else
        throw std::runtime_error(value.as_map()->get_string("what"));
    }

    return value.as_string();
  }

  void add_recorded_exc(const std::string &key) const {
    shcore::Array_t array;
    if (!m_recorded_data->has_key(key)) {
      array = shcore::make_array();
      m_recorded_data->set(key, shcore::Value(array));
    } else {
      array = m_recorded_data->get_array(key);
    }
    shcore::Dictionary_t dict = shcore::make_dict();
    try {
      throw;
    } catch (mysqlshdk::utils::net_error &e) {
      dict->set("type", shcore::Value("net_error"));
      dict->set("what", shcore::Value(e.what()));
    } catch (std::exception &e) {
      dict->set("type", shcore::Value("exception"));
      dict->set("what", shcore::Value(e.what()));
    }

    array->push_back(shcore::Value(dict));
  }

  void add_recorded(const std::string &key,
                    const std::vector<std::string> &value) const {
    shcore::Array_t array;
    if (!m_recorded_data->has_key(key)) {
      array = shcore::make_array();
      m_recorded_data->set(key, shcore::Value(array));
    } else {
      array = m_recorded_data->get_array(key);
    }

    shcore::Array_t strv = shcore::make_array();
    for (const auto &s : value) {
      strv->push_back(shcore::Value(s));
    }
    array->push_back(shcore::Value(strv));
  }

  void add_recorded(const std::string &key, const std::string &value) const {
    shcore::Array_t array;
    if (!m_recorded_data->has_key(key)) {
      array = shcore::make_array();
      m_recorded_data->set(key, shcore::Value(array));
    } else {
      array = m_recorded_data->get_array(key);
    }

    array->push_back(shcore::Value(value));
  }
};

}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_TEST_NET_UTILITIES_H_
