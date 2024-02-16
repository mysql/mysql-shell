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

#include "my_inttypes.h"
#include "scripting/types.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <stdexcept>
#include "modules/adminapi/common/router_options.h"

namespace mysqlsh::dba {

class Admin_api_router_options_test : public ::testing::Test {
 public:
  Admin_api_router_options_test() {
    m_changes_schema_simple = shcore::Value::parse(R"##({
    'routing_rules': {
      'type':'object',
      'properties': {
        'target_cluster': {
          'type': 'string'
        },
        'read_only_targets': {
         'enum': [
           'all',
           'read_replicas',
           'secondaries'
          ],
          'type': 'string'
        },
        'stats_updates_frequency':{
          'type': 'number'
        },
        'use_replica_primary_as_rw':{
          'type': 'boolean'
        },
        'invalidated_cluster_policy':{
          'enum': [
            'accept_ro',
            'drop_all'
          ],
          'type': 'string'
        },
        'unreachable_quorum_allowed_traffic' : {
          'enum': [
            'none',
            'read',
            'all'
          ],
          'type' : 'string'
        }
      }
    }
    })##");

    m_changes_schema_extended = shcore::Value::parse(R"##({
        "routing_rules": {
            "type": "object",
            "properties": {
                "target_cluster": {
                    "type": "string"
                },
                "use_replica_primary_as_rw": {
                    "type": "boolean"
                },
                "stats_updates_frequency": {
                    "type": "number"
                },
                "read_only_targets": {
                    "type": "string",
                    "enum": [
                        "all",
                        "read_replicas",
                        "secondaries"
                    ]
                },
                "unreachable_quorum_allowed_traffic": {
                    "type": "string",
                    "enum": [
                        "none",
                        "read",
                        "all"
                    ]
                },
                "invalidated_cluster_policy": {
                    "type": "string",
                    "enum": [
                        "accept_ro",
                        "drop_all"
                    ]
                }
            },
            "additionalProperties": false
        },
        "metadata_cache": {
            "type": "object",
            "properties": {
                "ttl": {
                    "type": "number",
                    "minimum": 0,
                    "maximum": 42949672950
                },
                "connect_timeout": {
                    "type": "integer",
                    "minimum": 1,
                    "maximum": 65535
                },
                "read_timeout": {
                    "type": "integer",
                    "minimum": 1,
                    "maximum": 65535
                }
            },
            "additionalProperties": false
        },
        "destination_status": {
            "type": "object",
            "properties": {
                "error_quarantine_threshold": {
                    "type": "integer",
                    "minimum": 1,
                    "maximum": 65535
                },
                "error_quarantine_interval": {
                    "type": "integer",
                    "minimum": 1,
                    "maximum": 3600
                }
            },
            "additionalProperties": false
        },
        "connection_pool": {
            "type": "object",
            "properties": {
                "max_idle_server_connections": {
                    "type": "integer",
                    "minimum": 0,
                    "maximum": 4294967296
                },
                "idle_timeout": {
                    "type": "integer",
                    "minimum": 0,
                    "maximum": 4294967296
                }
            },
            "additionalProperties": false
        }
    })##");

    m_configuration_schema = shcore::Value::parse(R"##({
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
                "bind_port": 6447,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "classic",
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_rw": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6446,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "classic",
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_address": "127.0.0.1",
                "bind_port": 6450,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=PRIMARY_AND_SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "classic",
                "routing_strategy": "round-robin",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_x_ro": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6449,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "x",
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_x_rw": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6448,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "x",
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            }
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
            "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
            "ssl_curves": "",
            "ssl_dh_params": "",
            "ssl_key": "",
            "static_folder": "",
            "with_ssl": 0
        },
        "io": {
            "backend": "linux_epoll",
            "threads": 0
        },
        "loggers": {
            "filelog": {
                "destination": "",
                "filename": "mysqlrouter.log",
                "level": "warning",
                "timestamp_precision": "second"
            }
        },
        "metadata_cache": {
            "auth_cache_refresh_interval": 2,
            "auth_cache_ttl": -1,
            "cluster_type": "gr",
            "connect_timeout": 5,
            "read_timeout": 30,
            "ttl": 0.5,
            "use_gr_notifications": false
        },
        "rest_configs": {
            "rest_api": {
                "require_realm": "default_auth_realm"
            },
            "rest_metadata_cache": {
                "require_realm": "default_auth_realm"
            },
            "rest_router": {
                "require_realm": "default_auth_realm"
            },
            "rest_routing": {
                "require_realm": "default_auth_realm"
            }
        },
        "routing_rules": {
            "invalidated_cluster_policy": "drop_all",
            "read_only_targets": "secondaries",
            "stats_updates_frequency": -1,
            "tags": "{}",
            "unreachable_quorum_allowed_traffic": "none",
            "use_replica_primary_as_rw": false
        }
    })##");
  }

 protected:
  shcore::Value m_changes_schema_simple;
  shcore::Value m_changes_schema_extended;
  shcore::Value m_configuration_schema;
};

TEST_F(Admin_api_router_options_test, configuration_changes_get_properties) {
  Router_configuration_changes_schema changes_schema;

  changes_schema.add_property("category1", "name1", "string",
                              {"value1", "value2"});

  // Retrieve property names for the specified category
  auto property_names = changes_schema.get_property_names();

  ASSERT_EQ(property_names.size(), 1);
  ASSERT_EQ(property_names[0], "category1");

  // Retrieve properties for the specified category
  const auto &properties_list = changes_schema.get_properties();
  const auto &properties = properties_list.at("category1");

  ASSERT_EQ(properties.size(), 1);
  ASSERT_EQ(properties[0]->name, "name1");
  ASSERT_EQ(properties[0]->type,
            Router_configuration_changes_schema::Json_type::STRING);
  ASSERT_EQ(properties[0]->accepted_values.size(), 2);
  ASSERT_EQ(properties[0]->accepted_values[0], "value1");
  ASSERT_EQ(properties[0]->accepted_values[1], "value2");
}

TEST_F(Admin_api_router_options_test, get_property_names) {
  // Create an instance of Router_configuration_changes_schema for testing
  Router_configuration_changes_schema schema;

  // Add some properties for testing
  schema.add_property("category1", "property1", "string");
  schema.add_property("category1", "property2", "number");
  schema.add_property("category2", "property3", "boolean");

  // Get the property names
  std::vector<std::string> property_names = schema.get_property_names();

  // Assert the expected property names
  ASSERT_EQ(property_names.size(), 2);
  EXPECT_TRUE(std::find(property_names.begin(), property_names.end(),
                        "category1") != property_names.end());
  EXPECT_TRUE(std::find(property_names.begin(), property_names.end(),
                        "category2") != property_names.end());
}

TEST_F(Admin_api_router_options_test,
       Router_configuration_changes_schema_from_value) {
  Router_configuration_changes_schema changes_schema;

  EXPECT_NO_THROW(changes_schema = Router_configuration_changes_schema{
                      m_changes_schema_simple});

  // Get the property names
  std::vector<std::string> property_names = changes_schema.get_property_names();

  ASSERT_EQ(property_names.size(), 1);
  EXPECT_TRUE(std::find(property_names.begin(), property_names.end(),
                        "routing_rules") != property_names.end());

  const auto &properties_list = changes_schema.get_properties();
  ASSERT_EQ(properties_list.size(), 1);

  const auto &routing_rules = properties_list.at("routing_rules");
  ASSERT_EQ(routing_rules.size(), 6);

  ASSERT_EQ(routing_rules[0]->name, "invalidated_cluster_policy");
  ASSERT_EQ(routing_rules[0]->type,
            Router_configuration_changes_schema::Json_type::STRING);
  ASSERT_EQ(routing_rules[0]->accepted_values.size(), 2);
  ASSERT_EQ(routing_rules[0]->accepted_values[0], "accept_ro");
  ASSERT_EQ(routing_rules[0]->accepted_values[1], "drop_all");

  ASSERT_EQ(routing_rules[1]->name, "read_only_targets");
  ASSERT_EQ(routing_rules[1]->type,
            Router_configuration_changes_schema::Json_type::STRING);
  ASSERT_EQ(routing_rules[1]->accepted_values.size(), 3);
  ASSERT_EQ(routing_rules[1]->accepted_values[0], "all");
  ASSERT_EQ(routing_rules[1]->accepted_values[1], "read_replicas");
  ASSERT_EQ(routing_rules[1]->accepted_values[2], "secondaries");

  ASSERT_EQ(routing_rules[2]->name, "stats_updates_frequency");
  ASSERT_EQ(routing_rules[2]->type,
            Router_configuration_changes_schema::Json_type::NUMBER);
  ASSERT_EQ(routing_rules[2]->accepted_values.size(), 0);
  ASSERT_TRUE(routing_rules[2]->accepted_values.empty());

  ASSERT_EQ(routing_rules[3]->name, "target_cluster");
  ASSERT_EQ(routing_rules[3]->type,
            Router_configuration_changes_schema::Json_type::STRING);
  ASSERT_EQ(routing_rules[3]->accepted_values.size(), 0);
  ASSERT_TRUE(routing_rules[3]->accepted_values.empty());

  ASSERT_EQ(routing_rules[4]->name, "unreachable_quorum_allowed_traffic");
  ASSERT_EQ(routing_rules[4]->type,
            Router_configuration_changes_schema::Json_type::STRING);
  ASSERT_EQ(routing_rules[4]->accepted_values.size(), 3);
  ASSERT_EQ(routing_rules[4]->accepted_values[0], "none");
  ASSERT_EQ(routing_rules[4]->accepted_values[1], "read");
  ASSERT_EQ(routing_rules[4]->accepted_values[2], "all");

  ASSERT_EQ(routing_rules[5]->name, "use_replica_primary_as_rw");
  ASSERT_EQ(routing_rules[5]->type,
            Router_configuration_changes_schema::Json_type::BOOLEAN);
  ASSERT_EQ(routing_rules[5]->accepted_values.size(), 0);
  ASSERT_TRUE(routing_rules[5]->accepted_values.empty());
}

TEST_F(Admin_api_router_options_test,
       Router_configuration_changes_schema_from_value_more) {
  Router_configuration_changes_schema changes_schema;

  EXPECT_NO_THROW(changes_schema = Router_configuration_changes_schema{
                      m_changes_schema_extended});

  // Get the property names
  std::vector<std::string> property_names = changes_schema.get_property_names();

  ASSERT_EQ(property_names.size(), 4);
  EXPECT_TRUE(std::find(property_names.begin(), property_names.end(),
                        "routing_rules") != property_names.end());
  EXPECT_TRUE(std::find(property_names.begin(), property_names.end(),
                        "metadata_cache") != property_names.end());
  EXPECT_TRUE(std::find(property_names.begin(), property_names.end(),
                        "destination_status") != property_names.end());
  EXPECT_TRUE(std::find(property_names.begin(), property_names.end(),
                        "connection_pool") != property_names.end());

  const auto &properties_list = changes_schema.get_properties();
  ASSERT_EQ(properties_list.size(), 4);

  const auto &metadata_cache = properties_list.at("metadata_cache");
  ASSERT_EQ(metadata_cache.size(), 3);

  ASSERT_EQ(metadata_cache[0]->name, "connect_timeout");
  ASSERT_EQ(metadata_cache[0]->type,
            Router_configuration_changes_schema::Json_type::INTEGER);
  ASSERT_EQ(metadata_cache[0]->accepted_values.size(), 0);
  ASSERT_EQ(metadata_cache[0]->number_range.first, 1);
  ASSERT_EQ(metadata_cache[0]->number_range.second, 65535);
  ASSERT_TRUE(metadata_cache[0]->accepted_values.empty());

  ASSERT_EQ(metadata_cache[1]->name, "read_timeout");
  ASSERT_EQ(metadata_cache[1]->type,
            Router_configuration_changes_schema::Json_type::INTEGER);
  ASSERT_EQ(metadata_cache[1]->accepted_values.size(), 0);
  ASSERT_EQ(metadata_cache[1]->number_range.first, 1);
  ASSERT_EQ(metadata_cache[1]->number_range.second, 65535);
  ASSERT_TRUE(metadata_cache[1]->accepted_values.empty());

  ASSERT_EQ(metadata_cache[2]->name, "ttl");
  ASSERT_EQ(metadata_cache[2]->type,
            Router_configuration_changes_schema::Json_type::NUMBER);
  ASSERT_EQ(metadata_cache[2]->accepted_values.size(), 0);
  ASSERT_EQ(metadata_cache[2]->number_range.first, 0);
  ASSERT_EQ(metadata_cache[2]->number_range.second, 42949672950);
  ASSERT_TRUE(metadata_cache[2]->accepted_values.empty());
}

TEST_F(Admin_api_router_options_test,
       Router_configuration_changes_schema_from_value_errors) {
  shcore::Value schema;
  Router_configuration_changes_schema changes_schema;

  schema = shcore::Value::parse(R"##({
    'routing_rules': {
      'type':'object',
      'properties': {
        'target_cluster': {
          'type': 'foo'
        },
        'read_only_targets': {
         'enum': [
           'all',
           'read_replicas',
           'secondaries'
          ],
          'type': 'foo'
        }
      }
    }
    })##");

  EXPECT_THROW_MSG(Router_configuration_changes_schema{schema},
                   std::logic_error, "Unsupported Json_type value: foo");

  schema = shcore::Value::parse(R"##({
    'routing_rules': {
      'type':'object',
      'foobar': {
        'target_cluster': {
          'type': 'string'
        },
        'read_only_targets': {
         'foo': [
           'all',
           'read_replicas',
           'secondaries'
          ],
          'no_type': 'null'
        }
      }
    }
    })##");

  EXPECT_THROW_MSG(Router_configuration_changes_schema{schema},
                   std::logic_error,
                   "Unable to parse document: malformed JSON");
}

TEST_F(Admin_api_router_options_test, add_option) {
  Router_configuration_document schema;

  // Add an option with a category, name, and string value
  std::string category = "routing_rules";
  std::string name = "target_cluster";
  std::string str_value = "primary";
  schema.add_option(category, name, str_value);

  const auto &options_list = schema.get_options();

  // Check if the added option exists and has the expected values
  ASSERT_TRUE(options_list.find(category) != options_list.end());

  const auto &category_options = options_list.at(category);

  ASSERT_FALSE(category_options.empty());
  ASSERT_EQ(category_options.size(), 1);
  ASSERT_EQ(category_options[0]->name, name);
  ASSERT_EQ(std::get<std::string>(category_options[0]->value), str_value);

  // Create an Option object
  Router_configuration_document::Option option{"ttl", 0.5};

  // Add the option with a category
  std::string category2 = "metadata_cache";
  schema.add_option(category2, option);

  // Check if the added option exists and has the expected values
  const auto &options_list_reloaded = schema.get_options();

  ASSERT_TRUE(options_list_reloaded.find(category2) !=
              options_list_reloaded.end());

  auto &new_category_options = options_list_reloaded.at(category2);

  ASSERT_FALSE(new_category_options.empty());
  ASSERT_EQ(new_category_options.size(), 1);
  ASSERT_EQ(new_category_options[0]->name, option.name);
  ASSERT_EQ(new_category_options[0]->value, option.value);
}

TEST_F(Admin_api_router_options_test, Router_configuration_schema_from_value) {
  Router_configuration_document configuration_schema;

  EXPECT_NO_THROW(configuration_schema =
                      Router_configuration_document{m_configuration_schema});

  // Get the options
  const auto &options_list = configuration_schema.get_options();

  ASSERT_EQ(options_list.size(), 11);

  EXPECT_TRUE(options_list.find("routing_rules") != options_list.end());

  const auto &opts = options_list.at("routing_rules");

  ASSERT_FALSE(opts.empty());
  ASSERT_EQ(opts.size(), 6);

  ASSERT_EQ(opts[0]->name, "invalidated_cluster_policy");
  ASSERT_EQ(std::get<std::string>(opts[0]->value), "drop_all");
  ASSERT_EQ(opts[1]->name, "read_only_targets");
  ASSERT_EQ(std::get<std::string>(opts[1]->value), "secondaries");
  ASSERT_EQ(opts[2]->name, "stats_updates_frequency");
  ASSERT_EQ(std::get<int64_t>(opts[2]->value), -1);
  ASSERT_EQ(opts[3]->name, "tags");
  ASSERT_EQ(std::get<std::string>(opts[3]->value), "{}");
  ASSERT_EQ(opts[4]->name, "unreachable_quorum_allowed_traffic");
  ASSERT_EQ(std::get<std::string>(opts[4]->value), "none");
  ASSERT_EQ(opts[5]->name, "use_replica_primary_as_rw");
  ASSERT_EQ(std::get<bool>(opts[5]->value), false);

  const auto &new_opts = options_list.at("metadata_cache");

  ASSERT_FALSE(new_opts.empty());
  ASSERT_EQ(new_opts.size(), 7);

  ASSERT_EQ(new_opts[0]->name, "auth_cache_refresh_interval");
  ASSERT_EQ(std::get<int64_t>(new_opts[0]->value), 2);
  ASSERT_EQ(new_opts[1]->name, "auth_cache_ttl");
  ASSERT_EQ(std::get<int64_t>(new_opts[1]->value), -1);
  ASSERT_EQ(new_opts[2]->name, "cluster_type");
  ASSERT_EQ(std::get<std::string>(new_opts[2]->value), "gr");
  ASSERT_EQ(new_opts[3]->name, "connect_timeout");
  ASSERT_EQ(std::get<int64_t>(new_opts[3]->value), 5);
  ASSERT_EQ(new_opts[4]->name, "read_timeout");
  ASSERT_EQ(std::get<int64_t>(new_opts[4]->value), 30);
  ASSERT_EQ(new_opts[5]->name, "ttl");
  ASSERT_EQ(std::get<double>(new_opts[5]->value), 0.5);
  ASSERT_EQ(new_opts[6]->name, "use_gr_notifications");
  ASSERT_EQ(std::get<bool>(new_opts[6]->value), false);
}

TEST_F(Admin_api_router_options_test, filter_schema_by_changes) {
  Router_configuration_changes_schema changes_schema;
  Router_configuration_document configuration_schema;

  EXPECT_NO_THROW(changes_schema = Router_configuration_changes_schema{
                      m_changes_schema_simple});

  EXPECT_NO_THROW(configuration_schema =
                      Router_configuration_document{m_configuration_schema});

  // Filter out options that only exist in the configuration schema
  Router_configuration_document filtered_schema;

  EXPECT_NO_THROW(
      filtered_schema =
          configuration_schema.filter_schema_by_changes(changes_schema));

  // Get the options
  const auto &options_list = filtered_schema.get_options();

  // There can be only one (heh)
  ASSERT_EQ(options_list.size(), 1);

  EXPECT_TRUE(options_list.find("routing_rules") != options_list.end());

  // Test with a larger changes schema
  EXPECT_NO_THROW(changes_schema = Router_configuration_changes_schema{
                      m_changes_schema_extended});

  EXPECT_NO_THROW(
      filtered_schema =
          configuration_schema.filter_schema_by_changes(changes_schema));

  // Get the options
  const auto &options_list_filtered = filtered_schema.get_options();

  // Should be 4
  ASSERT_EQ(options_list_filtered.size(), 4);

  EXPECT_TRUE(options_list_filtered.find("routing_rules") !=
              options_list_filtered.end());
  EXPECT_TRUE(options_list_filtered.find("metadata_cache") !=
              options_list_filtered.end());
  EXPECT_TRUE(options_list_filtered.find("destination_status") !=
              options_list_filtered.end());
  EXPECT_TRUE(options_list_filtered.find("connection_pool") !=
              options_list_filtered.end());
}

TEST_F(Admin_api_router_options_test, diff_schema) {
  Router_configuration_document configuration_schema;
  Router_configuration_document configuration_schema_different;
  shcore::Value schema_different;

  schema_different = shcore::Value::parse(R"##({
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
                "bind_port": 6447,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "classic",
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "/home/miguel/some/path/file.ca",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_rw": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6446,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "classic",
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_address": "127.0.0.1",
                "bind_port": 6450,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=PRIMARY_AND_SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 17896,
                "protocol": "classic",
                "routing_strategy": "round-robin",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_x_ro": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6449,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "x",
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_x_rw": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6448,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "x",
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            }
        },
        "http_authentication_backends": {
            "default_auth_backend": {
                "backend": "default_auth_realm",
                "filename": "/home/miguel/some_filename"
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
            "port": 9081,
            "require_realm": "default_auth_realm",
            "ssl_cert": "",
            "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
            "ssl_curves": "",
            "ssl_dh_params": "",
            "ssl_key": "",
            "static_folder": "",
            "with_ssl": 0
        },
        "io": {
            "backend": "linux_epoll",
            "threads": 16
        },
        "loggers": {
            "filelog": {
                "destination": "",
                "filename": "mysqlrouter.log",
                "level": "error",
                "timestamp_precision": "second"
            }
        },
        "metadata_cache": {
            "auth_cache_refresh_interval": 2,
            "auth_cache_ttl": -1,
            "cluster_type": "gr",
            "connect_timeout": 5,
            "read_timeout": 30,
            "ttl": 0.5,
            "use_gr_notifications": false,
            "user": "miguel"
        },
        "rest_configs": {
            "rest_api": {
                "require_realm": "default_auth_realm"
            },
            "rest_metadata_cache": {
                "require_realm": "default_auth_realm"
            },
            "rest_router": {
                "require_realm": "default_auth_realm"
            },
            "rest_routing": {
                "require_realm": "default_auth_realm"
            }
        },
        "routing_rules": {
            "invalidated_cluster_policy": "drop_all",
            "read_only_targets": "all",
            "stats_updates_frequency": -1,
            "tags": "{}",
            "unreachable_quorum_allowed_traffic": "none",
            "use_replica_primary_as_rw": true
        }
    })##");

  EXPECT_NO_THROW(configuration_schema =
                      Router_configuration_document{m_configuration_schema});

  EXPECT_NO_THROW(configuration_schema_different =
                      Router_configuration_document{schema_different});

  // Diff out options that only exist in the configuration schema
  Router_configuration_document diff_schema;

  EXPECT_NO_THROW(diff_schema =
                      configuration_schema_different.diff_configuration_schema(
                          configuration_schema));

  /*
  The diffs are:

    "endpoints": {
      "bootstrap_ro": {
        "server_ssl_ca": "/home/miguel/some/path/file.ca"
      },
      bootstrap_rw_split": {
        "net_buffer_length": 17896
      }
    },
    "http_authentication_backends": {
        "default_auth_backend": {
            "filename": "/home/miguel/some_filename"
        }
    },
    "http_server": {
        "port": 9081
    },
    "io": {
        "threads": 16
    },
    "loggers": {
        "filelog": {
            "level": "error"
        }
    },
    "routing_rules": {
        "read_only_targets": "all",
        "use_replica_primary_as_rw": true
    },
    "metadata_cache": {
        "user": "miguel"
    }
    }
   */

  // Get the options
  const auto &options_list = diff_schema.get_options();

  // Assert the size of the options list
  ASSERT_EQ(options_list.size(), 7);

  // Assert the presence of specific options
  {
    EXPECT_NE(options_list.find("endpoints"), options_list.end());
    EXPECT_NE(options_list.find("http_authentication_backends"),
              options_list.end());
    EXPECT_NE(options_list.find("http_server"), options_list.end());
    EXPECT_NE(options_list.find("io"), options_list.end());
    EXPECT_NE(options_list.find("loggers"), options_list.end());
    EXPECT_NE(options_list.find("routing_rules"), options_list.end());
    EXPECT_NE(options_list.find("metadata_cache"), options_list.end());
  }

  // Assert specific differences within "endpoints"
  {
    const auto &endpoints_diff = options_list.at("endpoints");
    ASSERT_EQ(endpoints_diff.size(), 2);

    auto bootstrap_ro_diff = std::find_if(
        endpoints_diff.begin(), endpoints_diff.end(),
        [](const auto &opt) { return opt->name == "bootstrap_ro"; });
    ASSERT_NE(bootstrap_ro_diff, endpoints_diff.end());

    const auto &sub_opt = (*bootstrap_ro_diff)->sub_option;

    ASSERT_EQ(sub_opt.size(), 1);

    const auto &it = sub_opt.find("server_ssl_ca");

    ASSERT_TRUE(it != sub_opt.end());
    ASSERT_EQ(std::get<std::string>(it->second.get()->value),
              "/home/miguel/some/path/file.ca");

    auto bootstrap_rw_split_diff = std::find_if(
        endpoints_diff.begin(), endpoints_diff.end(),
        [](const auto &opt) { return opt->name == "bootstrap_rw_split"; });
    ASSERT_NE(bootstrap_rw_split_diff, endpoints_diff.end());

    const auto &sub_opt2 = (*bootstrap_rw_split_diff)->sub_option;

    ASSERT_EQ(sub_opt2.size(), 1);

    const auto &it2 = sub_opt2.find("net_buffer_length");

    ASSERT_TRUE(it2 != sub_opt2.end());
    ASSERT_EQ(std::get<int64>(it2->second.get()->value), 17896);
  }

  // Assert specific differences within "http_authentication_backends"
  {
    const auto &http_authentication_backends_diff =
        options_list.at("http_authentication_backends");
    ASSERT_EQ(http_authentication_backends_diff.size(), 1);

    auto default_auth_backend_diff = std::find_if(
        http_authentication_backends_diff.begin(),
        http_authentication_backends_diff.end(),
        [](const auto &opt) { return opt->name == "default_auth_backend"; });
    ASSERT_NE(default_auth_backend_diff,
              http_authentication_backends_diff.end());

    const auto &sub_opt = (*default_auth_backend_diff)->sub_option;

    ASSERT_EQ(sub_opt.size(), 1);

    const auto &it = sub_opt.find("filename");

    ASSERT_TRUE(it != sub_opt.end());
    ASSERT_EQ(std::get<std::string>(it->second->value),
              "/home/miguel/some_filename");
  }

  // Assert specific differences within "http_server"
  {
    const auto &http_server_diff = options_list.at("http_server");
    ASSERT_EQ(http_server_diff.size(), 1);

    ASSERT_EQ(http_server_diff[0]->name, "port");
    ASSERT_EQ(std::get<std::int64_t>(http_server_diff[0]->value), 9081);
  }

  // Assert specific differences within "io"
  {
    const auto &io_diff = options_list.at("io");
    ASSERT_EQ(io_diff.size(), 1);

    ASSERT_EQ(io_diff[0]->name, "threads");
    ASSERT_EQ(std::get<std::int64_t>(io_diff[0]->value), 16);
  }

  // Assert specific differences within "loggers"
  {
    const auto &loggers_diff = options_list.at("loggers");
    ASSERT_EQ(loggers_diff.size(), 1);

    auto filelog_diff =
        std::find_if(loggers_diff.begin(), loggers_diff.end(),
                     [](const auto &opt) { return opt->name == "filelog"; });
    ASSERT_NE(filelog_diff, loggers_diff.end());

    const auto &sub_opt = (*filelog_diff)->sub_option;

    ASSERT_EQ(sub_opt.size(), 1);

    const auto &it = sub_opt.find("level");

    ASSERT_TRUE(it != sub_opt.end());
    ASSERT_EQ(std::get<std::string>(it->second->value), "error");
  }

  // Assert specific differences within "routing_rules"
  {
    const auto &routing_rules_diff = options_list.at("routing_rules");
    ASSERT_EQ(routing_rules_diff.size(), 2);

    ASSERT_EQ(routing_rules_diff[0]->name, "read_only_targets");
    ASSERT_EQ(std::get<std::string>(routing_rules_diff[0]->value), "all");
    ASSERT_EQ(routing_rules_diff[1]->name, "use_replica_primary_as_rw");
    ASSERT_EQ(std::get<bool>(routing_rules_diff[1]->value), true);
  }

  // Assert specific differences within "metadata_cache". The difference here is
  // a special case, the option exists on the parent but not the target_schema,
  // i.e. the router options include it but the defaults no. The expected
  // outcome is that the difference is included in the result
  {
    const auto &metadata_cache_diff = options_list.at("metadata_cache");
    ASSERT_EQ(metadata_cache_diff.size(), 1);

    ASSERT_EQ(metadata_cache_diff[0]->name, "user");
    ASSERT_EQ(std::get<std::string>(metadata_cache_diff[0]->value), "miguel");
  }
}

TEST_F(Admin_api_router_options_test, filter_common) {
  Router_configuration_document configuration_schema_with_common;
  shcore::Value schema_with_common;

  schema_with_common = shcore::Value::parse(R"##({
        "connection_pool": {
            "idle_timeout": 5,
            "max_idle_server_connections": 0
        },
        "common": {
            "client_ssl_cert": "",
            "max_connect_errors": 100,
            "use_gr_notifications": false,
            "require_realm": "default_auth_realm"
        },
        "destination_status": {
            "error_quarantine_interval": 1,
            "error_quarantine_threshold": 1
        },
        "endpoints": {
            "bootstrap_ro": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6447,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "classic",
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "/home/miguel/some/path/file.ca",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_rw": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6446,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "classic",
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_address": "127.0.0.1",
                "bind_port": 6450,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=PRIMARY_AND_SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 17896,
                "protocol": "classic",
                "routing_strategy": "round-robin",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_x_ro": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6449,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "x",
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            },
            "bootstrap_x_rw": {
                "access_mode": "",
                "bind_address": "127.0.0.1",
                "bind_port": 6448,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1000,
                "destinations": "metadata-cache://test/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "named_socket": "",
                "net_buffer_length": 16384,
                "protocol": "x",
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "as_client",
                "server_ssl_verify": "as_client",
                "thread_stack_size": 1024
            }
        },
        "http_authentication_backends": {
            "default_auth_backend": {
                "backend": "default_auth_realm",
                "filename": "/home/miguel/some_filename"
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
            "port": 9081,
            "require_realm": "default_auth_realm",
            "ssl_cert": "",
            "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
            "ssl_curves": "",
            "ssl_dh_params": "",
            "ssl_key": "",
            "static_folder": "",
            "with_ssl": 0
        },
        "io": {
            "backend": "linux_epoll",
            "threads": 16
        },
        "loggers": {
            "filelog": {
                "destination": "",
                "filename": "mysqlrouter.log",
                "level": "error",
                "timestamp_precision": "second"
            }
        },
        "metadata_cache": {
            "auth_cache_refresh_interval": 2,
            "auth_cache_ttl": -1,
            "cluster_type": "gr",
            "connect_timeout": 5,
            "read_timeout": 30,
            "ttl": 0.5,
            "use_gr_notifications": false,
            "user": "miguel"
        },
        "rest_configs": {
            "rest_api": {
                "require_realm": "default_auth_realm"
            },
            "rest_metadata_cache": {
                "require_realm": "default_auth_realm"
            },
            "rest_router": {
                "require_realm": "default_auth_realm"
            },
            "rest_routing": {
                "require_realm": "default_auth_realm"
            }
        },
        "routing_rules": {
            "invalidated_cluster_policy": "drop_all",
            "read_only_targets": "all",
            "stats_updates_frequency": -1,
            "tags": "{}",
            "unreachable_quorum_allowed_traffic": "none",
            "use_replica_primary_as_rw": true
        }
    })##");

  EXPECT_NO_THROW(configuration_schema_with_common =
                      Router_configuration_document{schema_with_common});

  // Filter out the global common options
  Router_configuration_document filtered_schema;

  EXPECT_NO_THROW(
      filtered_schema =
          configuration_schema_with_common.filter_common_router_options());

  /*
  What must be filtered out is:

    - endpoints.client_ssl_cert
    - endpoints.max_connect_errors
    - metadata_cache.use_gr_notifications
    - rest_configs.*.require_realm
*/
  const auto &options_list = filtered_schema.get_options();

  // Assert the size of the options list
  ASSERT_EQ(options_list.size(), 11);

  // Assert the presence of specific options
  {
    EXPECT_NE(options_list.find("connection_pool"), options_list.end());
    EXPECT_NE(options_list.find("common"), options_list.end());
    EXPECT_NE(options_list.find("destination_status"), options_list.end());
    EXPECT_NE(options_list.find("endpoints"), options_list.end());
    EXPECT_NE(options_list.find("io"), options_list.end());
    EXPECT_NE(options_list.find("http_authentication_realm"),
              options_list.end());
    EXPECT_NE(options_list.find("http_server"), options_list.end());
    EXPECT_NE(options_list.find("io"), options_list.end());
    EXPECT_NE(options_list.find("loggers"), options_list.end());
    EXPECT_NE(options_list.find("metadata_cache"), options_list.end());
    // "rest_configs" should be completely gone
    EXPECT_EQ(options_list.find("rest_configs"), options_list.end());
    EXPECT_NE(options_list.find("routing_rules"), options_list.end());
  }

  // Assert specific differences within "endpoints"
  // "client_ssl_cert" and "max_connect_errors" should be gone
  {
    const auto &endpoints_diff = options_list.at("endpoints");
    ASSERT_EQ(endpoints_diff.size(), 5);

    auto bootstrap_ro_diff = std::find_if(
        endpoints_diff.begin(), endpoints_diff.end(),
        [](const auto &opt) { return opt->name == "bootstrap_ro"; });
    ASSERT_NE(bootstrap_ro_diff, endpoints_diff.end());

    const auto &sub_opt = (*bootstrap_ro_diff)->sub_option;

    // Total was 29, but 2 must have been removed
    ASSERT_EQ(sub_opt.size(), 27);

    ASSERT_TRUE(sub_opt.find("client_ssl_cert") == sub_opt.end());
    ASSERT_TRUE(sub_opt.find("max_connect_errors") == sub_opt.end());

    auto bootstrap_rw_diff = std::find_if(
        endpoints_diff.begin(), endpoints_diff.end(),
        [](const auto &opt) { return opt->name == "bootstrap_rw"; });
    ASSERT_NE(bootstrap_rw_diff, endpoints_diff.end());

    const auto &sub_opt2 = (*bootstrap_rw_diff)->sub_option;

    ASSERT_EQ(sub_opt2.size(), 27);

    ASSERT_TRUE(sub_opt2.find("client_ssl_cert") == sub_opt2.end());
    ASSERT_TRUE(sub_opt2.find("max_connect_errors") == sub_opt2.end());

    auto bootstrap_rw_split_diff = std::find_if(
        endpoints_diff.begin(), endpoints_diff.end(),
        [](const auto &opt) { return opt->name == "bootstrap_rw_split"; });
    ASSERT_NE(bootstrap_rw_split_diff, endpoints_diff.end());

    const auto &sub_opt3 = (*bootstrap_rw_split_diff)->sub_option;

    ASSERT_EQ(sub_opt3.size(), 27);

    ASSERT_TRUE(sub_opt3.find("client_ssl_cert") == sub_opt3.end());
    ASSERT_TRUE(sub_opt3.find("max_connect_errors") == sub_opt3.end());

    auto bootstrap_x_ro_diff = std::find_if(
        endpoints_diff.begin(), endpoints_diff.end(),
        [](const auto &opt) { return opt->name == "bootstrap_x_ro"; });
    ASSERT_NE(bootstrap_x_ro_diff, endpoints_diff.end());

    const auto &sub_opt4 = (*bootstrap_x_ro_diff)->sub_option;

    ASSERT_EQ(sub_opt4.size(), 27);

    ASSERT_TRUE(sub_opt4.find("client_ssl_cert") == sub_opt4.end());
    ASSERT_TRUE(sub_opt4.find("max_connect_errors") == sub_opt4.end());

    auto bootstrap_x_rw_diff = std::find_if(
        endpoints_diff.begin(), endpoints_diff.end(),
        [](const auto &opt) { return opt->name == "bootstrap_x_ro"; });
    ASSERT_NE(bootstrap_x_rw_diff, endpoints_diff.end());

    const auto &sub_opt5 = (*bootstrap_x_rw_diff)->sub_option;

    ASSERT_EQ(sub_opt5.size(), 27);

    ASSERT_TRUE(sub_opt5.find("client_ssl_cert") == sub_opt5.end());
    ASSERT_TRUE(sub_opt5.find("max_connect_errors") == sub_opt5.end());
  }

  // Assert specific differences within "metadata_cache"
  // "use_gr_notifications" should be gone
  {
    const auto &metadata_cache_diff = options_list.at("metadata_cache");
    ASSERT_EQ(metadata_cache_diff.size(), 7);

    auto use_gr_notifications = std::find_if(
        metadata_cache_diff.begin(), metadata_cache_diff.end(),
        [](const auto &opt) { return opt->name == "use_gr_notifications"; });

    ASSERT_EQ(use_gr_notifications, metadata_cache_diff.end());
  }
}

}  // namespace mysqlsh::dba
