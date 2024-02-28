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

#ifndef MODULES_ADMINAPI_COMMON_ROUTER_OPTIONS_H_
#define MODULES_ADMINAPI_COMMON_ROUTER_OPTIONS_H_

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "mysqlshdk/include/scripting/types.h"

namespace mysqlsh::dba {

/*  The Configuration Changes Schema has the following format:

  "ConfigurationChangesSchema": {
      "type": "object",
      "title": "MySQL Router configuration JSON schema",
      "$schema": "http://json-schema.org/draft-04/schema#",
      "properties": {
          "routing_rules": {
              "type": "object",
              "properties": {
                  "target_cluster": {
                    "type": "string"
                  },
                  "read_only_targets": {
                    "enum": [
                      "all",
                      "read_replicas",
                      "secondaries"
                    ],
                    "type": "string"
                  },
                  "stats_updates_frequency": {
                    "type": "number"
                  },
                  "use_replica_primary_as_rw": {
                    "type": "boolean"
                  },
                  "invalidated_cluster_policy": {
                    "enum": [
                      "accept_ro",
                      "drop_all"
                    ],
                    "type": "string"
                  },
                  "unreachable_quorum_allowed_traffic": {
                    "enum": [
                      "none",
                      "read",
                      "all"
                  ],
                  "type": "string"
                }
              },
              "additionalProperties": false
          }
      },
      "description": "JSON Schema for the Router configuration options that can
  be changed in the runtime. Shared by the Router in the metadata to announce
  which options it supports changing.", "additionalProperties": false
  }
*/

// Representation of the Configuration Changes Schema.
class Router_configuration_changes_schema final {
  struct Property;
  using Object = std::unordered_map<std::string, std::unique_ptr<Property>>;
  using Array = std::vector<std::unique_ptr<Property>>;
  using Enum = std::vector<std::string>;
  using Range = std::pair<double, double>;

 private:
  // Accepted types by MySQL Router configuration JSON schema
  enum class Json_type { STRING, NUMBER, INTEGER, BOOLEAN, OBJECT, ARRAY };

  // Representation of a Property of the schema
  struct Property {
    std::string name;                    // Property name, e.g. "routing_rules"
    Json_type type = Json_type::OBJECT;  // Property JSON type
    Enum accepted_values;                // List of accepted values
    Range number_range = {0.0, 0.0};     // Range of numeric values
    Object object;                       // Represents an object
    Array array;                         // Represents an array

    Property(std::string n, Json_type t, Enum values, Range range) noexcept
        : name(std::move(n)),
          type(t),
          accepted_values(std::move(values)),
          number_range(std::move(range)) {}

    Property() = delete;
    Property(const Property &) = delete;
    Property(Property &&) = default;
    Property &operator=(const Property &) = delete;
    Property &operator=(Property &&) = default;

    ~Property() = default;
  };

  Json_type to_type(std::string_view type);

  std::unordered_map<std::string, Array> properties;

  friend class Router_configuration_schema;
  friend class Admin_api_router_options_test;

 public:
  explicit Router_configuration_changes_schema(const shcore::Value &schema);

  Router_configuration_changes_schema() = default;
  Router_configuration_changes_schema(
      const Router_configuration_changes_schema &) = delete;
  Router_configuration_changes_schema(Router_configuration_changes_schema &&) =
      delete;
  Router_configuration_changes_schema &operator=(
      const Router_configuration_changes_schema &) = delete;
  Router_configuration_changes_schema &operator=(
      Router_configuration_changes_schema &&) = default;

  explicit operator bool() const { return !properties.empty(); }

  ~Router_configuration_changes_schema() = default;

 public:
  void add_property(const std::string &property_name, std::string name,
                    std::string_view str_type, Enum accepted_values = {},
                    Range number_range = {});

  std::vector<std::string> get_property_names() const;

 protected:
#ifdef FRIEND_TEST
  FRIEND_TEST(Admin_api_router_options_test,
              configuration_changes_get_properties);
  FRIEND_TEST(Admin_api_router_options_test,
              Router_configuration_changes_schema_from_value);
  FRIEND_TEST(Admin_api_router_options_test,
              Router_configuration_changes_schema_from_value_more);
  FRIEND_TEST(Admin_api_router_options_test,
              Router_configuration_changes_schema_from_value_errors);
#endif

  const std::unordered_map<std::string, Array> &get_properties() const {
    return properties;
  }
};

/*  The Configuration Schema has the following format:
{
    "connection_pool": {
        "idle_timeout": 5,
        "max_idle_server_connections": 0
    },
    "destination_status": {
        "error_quarantine_interval": 1,
        "error_quarantine_threshold": 1
    },
    "endpoints": {
        "bootstrap_ro": {
            "access_mode": "",
            "bind_address": "127.0.0.1",
            (...)
        },
        "bootstrap_rw": {
            "access_mode": "",
            "bind_address": "127.0.0.1",
            (...)
        (...)
        },
        "http_authentication_backends": {
            "default_auth_backend": {
                "backend": "default_auth_realm",
                "filename": ""
            }
        },
        "http_authentication_realm": {
            "backend": "default_auth_backend",
            "method": "basic",
            "name": "default_realm",
            "require": ""
        },
        "http_server": {
            "bind_address": "0.0.0.0",
            "port": 8081,
            "require_realm": "default_auth_realm",
            "ssl_cert": "",
 (...)
*/

// Representation of the Configuration Changes Schema.
class Router_configuration_schema final {
 private:
  struct Option;
  using Value =
      std::variant<std::monostate, std::string, int64_t, double, bool>;
  using Sub_option = std::unordered_map<std::string, std::unique_ptr<Option>>;

  struct Option {
    std::string name;       // Option name, e.g. "target_cluster"
    Value value;            // Option value, e.g. "primary"
    Sub_option sub_option;  // Represents a sub-option object

    explicit Option(std::string n) noexcept : name(std::move(n)){};

    Option(std::string n, std::string str_value) noexcept
        : name(std::move(n)), value(std::move(str_value)) {}
    Option(std::string n, int64_t int_value) noexcept
        : name(std::move(n)), value(int_value) {}
    Option(std::string n, double double_value) noexcept
        : name(std::move(n)), value(double_value) {}
    Option(std::string n, bool bool_value) noexcept
        : name(std::move(n)), value(bool_value) {}
    Option(std::string n, Value value_value) noexcept
        : name(std::move(n)), value(value_value) {}
    Option(const Option &other) noexcept
        : name(other.name), value(other.value) {
      // Copy sub-options
      for (const auto &pair : other.sub_option) {
        sub_option[pair.first] = std::make_unique<Option>(*pair.second);
      }
    }

    Option() = delete;
    Option(Option &&) = default;
    Option &operator=(const Option &) = default;
    Option &operator=(Option &&) = default;

    ~Option() = default;

    bool operator==(const Option &other) const {
      return name == other.name && value == other.value &&
             diff_sub_options(other);
    }

    bool operator!=(const Option &other) const { return !(*this == other); }

    // Check if option is different including sub-options
    bool diff_sub_options(const Option &target) const {
      if (sub_option.size() != target.sub_option.size()) {
        return false;
      }

      for (const auto &sub_option_current : sub_option) {
        const auto &it_sub_option_target =
            target.sub_option.find(sub_option_current.first);

        if (it_sub_option_target == target.sub_option.end() ||
            *(sub_option_current.second) != *(it_sub_option_target->second)) {
          return false;
        }
      }

      return true;
    }
  };

  std::unordered_map<std::string, std::vector<std::unique_ptr<Option>>> options;

 public:
  explicit Router_configuration_schema(const shcore::Value &schema);

  Router_configuration_schema() = default;
  Router_configuration_schema(const Router_configuration_schema &) = delete;
  Router_configuration_schema(Router_configuration_schema &&) = default;
  Router_configuration_schema &operator=(const Router_configuration_schema &) =
      delete;
  Router_configuration_schema &operator=(Router_configuration_schema &&) =
      default;

  ~Router_configuration_schema() = default;

 public:
  void add_option(const std::string &section_name, std::string name,
                  std::string str_value);

  void add_option(const std::string &section_name, Option option);

  Router_configuration_schema diff_configuration_schema(
      const Router_configuration_schema &target_schema) const;

  Router_configuration_schema filter_schema_by_changes(
      const Router_configuration_changes_schema &configurations) const;

  Router_configuration_schema filter_common_router_options() const;

  shcore::Value to_value();

 private:
  Option parse_option(std::string name, const shcore::Value &value);

  shcore::Value convert_option_to_value(const Option &option) const;

  const std::unordered_map<std::string, std::vector<std::unique_ptr<Option>>>
      &get_options() const {
    return options;
  }

#ifdef FRIEND_TEST
  FRIEND_TEST(Admin_api_router_options_test, add_option);
  FRIEND_TEST(Admin_api_router_options_test,
              Router_configuration_schema_from_value);
  FRIEND_TEST(Admin_api_router_options_test, filter_schema_by_changes);
  FRIEND_TEST(Admin_api_router_options_test, diff_schema);
  FRIEND_TEST(Admin_api_router_options_test, filter_common);
#endif
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_ROUTER_H_
