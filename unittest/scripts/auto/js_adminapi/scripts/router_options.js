//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var cluster = scene.cluster

//@<> routerOptions() without any Router bootstrap
cluster.routerOptions();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {
        "routing_rules": {
            "read_only_targets": "secondaries",
            "tags": {}
        }
    },
    "routers": {}
}
`);

//@<> create routers
var cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

var router1 = "routerhost1::system";

const router_options_json =
{
  "tags": {},
  "Configuration": {
    "9.2.0": {
      "Defaults": {
        "io": {
          "threads": 0
        },
        "common": {
          "name": "system",
          "socket": "",
          "bind_address": "0.0.0.0",
          "read_timeout": 30,
          "server_ssl_ca": "",
          "server_ssl_crl": "",
          "client_ssl_mode": "PREFERRED",
          "connect_timeout": 5,
          "max_connections": 0,
          "server_ssl_mode": "PREFERRED",
          "client_ssl_cipher": "",
          "client_ssl_curves": "",
          "net_buffer_length": 16384,
          "server_ssl_capath": "",
          "server_ssl_cipher": "",
          "server_ssl_curves": "",
          "server_ssl_verify": "DISABLED",
          "thread_stack_size": 1024,
          "connection_sharing": false,
          "max_connect_errors": 100,
          "server_ssl_crlpath": "",
          "wait_for_my_writes": true,
          "client_ssl_dh_params": "",
          "max_total_connections": 512,
          "unknown_config_option": "error",
          "client_connect_timeout": 9,
          "connection_sharing_delay": 1.0,
          "wait_for_my_writes_timeout": 2,
          "max_idle_server_connections": 64
        },
        "loggers": {
          "filelog": {
            "level": "info",
            "filename": "mysqlrouter.log",
            "destination": "",
            "timestamp_precision": "second"
          }
        },
        "endpoints": {
          "bootstrap_ro": {
            "socket": "",
            "protocol": "classic",
            "bind_port": 6447,
            "bind_address": "0.0.0.0",
            "destinations": "metadata-cache://devCluster/?role=SECONDARY",
            "server_ssl_ca": "",
            "server_ssl_crl": "",
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "max_connections": 0,
            "server_ssl_mode": "PREFERRED",
            "routing_strategy": "round-robin-with-fallback",
            "client_ssl_cipher": "",
            "client_ssl_curves": "",
            "net_buffer_length": 16384,
            "server_ssl_capath": "",
            "server_ssl_cipher": "",
            "server_ssl_curves": "",
            "server_ssl_verify": "DISABLED",
            "thread_stack_size": 1024,
            "connection_sharing": false,
            "max_connect_errors": 100,
            "server_ssl_crlpath": "",
            "wait_for_my_writes": true,
            "client_ssl_dh_params": "",
            "client_connect_timeout": 9,
            "router_require_enforce": true,
            "connection_sharing_delay": 1.0,
            "wait_for_my_writes_timeout": 2
          },
          "bootstrap_rw": {
            "socket": "",
            "protocol": "classic",
            "bind_port": 6446,
            "bind_address": "0.0.0.0",
            "destinations": "metadata-cache://devCluster/?role=PRIMARY",
            "server_ssl_ca": "",
            "server_ssl_crl": "",
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "max_connections": 0,
            "server_ssl_mode": "PREFERRED",
            "routing_strategy": "first-available",
            "client_ssl_cipher": "",
            "client_ssl_curves": "",
            "net_buffer_length": 16384,
            "server_ssl_capath": "",
            "server_ssl_cipher": "",
            "server_ssl_curves": "",
            "server_ssl_verify": "DISABLED",
            "thread_stack_size": 1024,
            "connection_sharing": false,
            "max_connect_errors": 100,
            "server_ssl_crlpath": "",
            "wait_for_my_writes": true,
            "client_ssl_dh_params": "",
            "client_connect_timeout": 9,
            "router_require_enforce": true,
            "connection_sharing_delay": 1.0,
            "wait_for_my_writes_timeout": 2
          },
          "bootstrap_x_ro": {
            "socket": "",
            "protocol": "x",
            "bind_port": 6449,
            "bind_address": "0.0.0.0",
            "destinations": "metadata-cache://devCluster/?role=SECONDARY",
            "server_ssl_ca": "",
            "server_ssl_crl": "",
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "max_connections": 0,
            "server_ssl_mode": "PREFERRED",
            "routing_strategy": "round-robin-with-fallback",
            "client_ssl_cipher": "",
            "client_ssl_curves": "",
            "net_buffer_length": 16384,
            "server_ssl_capath": "",
            "server_ssl_cipher": "",
            "server_ssl_curves": "",
            "server_ssl_verify": "DISABLED",
            "thread_stack_size": 1024,
            "connection_sharing": false,
            "max_connect_errors": 100,
            "server_ssl_crlpath": "",
            "wait_for_my_writes": true,
            "client_ssl_dh_params": "",
            "client_connect_timeout": 9,
            "router_require_enforce": false,
            "connection_sharing_delay": 1.0,
            "wait_for_my_writes_timeout": 2
          },
          "bootstrap_x_rw": {
            "socket": "",
            "protocol": "x",
            "bind_port": 6448,
            "bind_address": "0.0.0.0",
            "destinations": "metadata-cache://devCluster/?role=PRIMARY",
            "server_ssl_ca": "",
            "server_ssl_crl": "",
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "max_connections": 0,
            "server_ssl_mode": "PREFERRED",
            "routing_strategy": "first-available",
            "client_ssl_cipher": "",
            "client_ssl_curves": "",
            "net_buffer_length": 16384,
            "server_ssl_capath": "",
            "server_ssl_cipher": "",
            "server_ssl_curves": "",
            "server_ssl_verify": "DISABLED",
            "thread_stack_size": 1024,
            "connection_sharing": false,
            "max_connect_errors": 100,
            "server_ssl_crlpath": "",
            "wait_for_my_writes": true,
            "client_ssl_dh_params": "",
            "client_connect_timeout": 9,
            "router_require_enforce": false,
            "connection_sharing_delay": 1.0,
            "wait_for_my_writes_timeout": 2
          },
          "bootstrap_rw_split": {
            "socket": "",
            "protocol": "classic",
            "bind_port": 6450,
            "access_mode": "auto",
            "bind_address": "0.0.0.0",
            "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
            "server_ssl_ca": "",
            "server_ssl_crl": "",
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "max_connections": 0,
            "server_ssl_mode": "PREFERRED",
            "routing_strategy": "round-robin",
            "client_ssl_cipher": "",
            "client_ssl_curves": "",
            "net_buffer_length": 16384,
            "server_ssl_capath": "",
            "server_ssl_cipher": "",
            "server_ssl_curves": "",
            "server_ssl_verify": "DISABLED",
            "thread_stack_size": 1024,
            "connection_sharing": true,
            "max_connect_errors": 100,
            "server_ssl_crlpath": "",
            "wait_for_my_writes": true,
            "client_ssl_dh_params": "",
            "client_connect_timeout": 9,
            "router_require_enforce": true,
            "connection_sharing_delay": 1.0,
            "wait_for_my_writes_timeout": 2
          }
        },
        "http_server": {
          "ssl": true,
          "port": 8443,
          "ssl_key": "",
          "ssl_cert": "",
          "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
          "ssl_curves": "",
          "bind_address": "0.0.0.0",
          "require_realm": "",
          "ssl_dh_params": "",
          "static_folder": ""
        },
        "rest_configs": {
          "rest_api": {
            "require_realm": ""
          },
          "rest_router": {
            "require_realm": "default_auth_realm"
          },
          "rest_routing": {
            "require_realm": "default_auth_realm"
          },
          "rest_metadata_cache": {
            "require_realm": "default_auth_realm"
          }
        },
        "routing_rules": {
          "read_only_targets": "secondaries",
          "stats_updates_frequency": -1,
          "use_replica_primary_as_rw": false,
          "invalidated_cluster_policy": "drop_all",
          "unreachable_quorum_allowed_traffic": "none"
        },
        "metadata_cache": {
          "ttl": 0.5,
          "ssl_ca": "",
          "ssl_crl": "",
          "ssl_mode": "PREFERRED",
          "ssl_capath": "",
          "ssl_cipher": "",
          "ssl_crlpath": "",
          "tls_version": "",
          "read_timeout": 30,
          "auth_cache_ttl": -1.0,
          "connect_timeout": 5,
          "thread_stack_size": 1024,
          "use_gr_notifications": false,
          "auth_cache_refresh_interval": 2.0,
          "close_connection_after_refresh": false
        },
        "connection_pool": {
          "idle_timeout": 5,
          "max_idle_server_connections": 64
        },
        "destination_status": {
          "error_quarantine_interval": 1,
          "error_quarantine_threshold": 1
        },
        "http_authentication_realm": {
          "name": "default_realm",
          "method": "basic",
          "backend": "default_auth_backend",
          "require": "valid-user"
        },
        "http_authentication_backends": {
          "default_auth_backend": {
            "backend": "metadata_cache",
            "filename": ""
          }
        }
      },
      "GuidelinesSchema": {
        "type": "object",
        "title": "MySQL Router routing guidelines engine document schema",
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "required": [
          "version",
          "destinations",
          "routes"
        ],
        "properties": {
          "name": {
            "type": "string",
            "description": "Name of the routing guidelines document"
          },
          "routes": {
            "type": "array",
            "items": {
              "type": "object",
              "minItems": 1,
              "required": [
                "name",
                "match",
                "destinations"
              ],
              "properties": {
                "name": {
                  "type": "string",
                  "description": "Name of the route"
                },
                "match": {
                  "type": "string",
                  "description": "Connection matching criteria"
                },
                "enabled": {
                  "type": "boolean"
                },
                "destinations": {
                  "type": "array",
                  "items": {
                    "type": "object",
                    "minItems": 1,
                    "required": [
                      "classes",
                      "strategy",
                      "priority"
                    ],
                    "properties": {
                      "classes": {
                        "type": "array",
                        "items": {
                          "type": "string",
                          "description": "Reference to 'name' entries in the 'destinations' section"
                        },
                        "description": "Destination group"
                      },
                      "priority": {
                        "type": "integer",
                        "minimum": 0,
                        "description": "Priority of the given group"
                      },
                      "strategy": {
                        "enum": [
                          "round-robin",
                          "first-available"
                        ],
                        "type": "string",
                        "description": "Routing strategy that will be used for this route"
                      }
                    },
                    "uniqueItems": true
                  },
                  "description": "Destination groups used for routing, by order of preference"
                },
                "connectionSharingAllowed": {
                  "type": "boolean"
                }
              },
              "uniqueItems": true
            },
            "description": "Routes entries that are binding destinations with connection matching criteria"
          },
          "version": {
            "type": "string",
            "description": "Version of the routing guidelines document"
          },
          "destinations": {
            "type": "array",
            "items": {
              "type": "object",
              "required": [
                "name",
                "match"
              ],
              "properties": {
                "name": {
                  "type": "string",
                  "description": "Unique name of the given destinations entry"
                },
                "match": {
                  "type": "string",
                  "description": "Matching criteria for destinations class"
                }
              }
            },
            "minItems": 1,
            "description": "Entries representing set of MySQL server instances",
            "uniqueItems": true
          }
        },
        "match_rules": {
          "keywords": {
            "type": "array",
            "items": {
              "enum": [
                "LIKE",
                "NULL",
                "NOT",
                "FALSE",
                "AND",
                "IN",
                "OR",
                "TRUE"
              ],
              "type": "string"
            }
          },
          "functions": {
            "type": "array",
            "items": {
              "enum": [
                "SQRT",
                "NUMBER",
                "IS_IPV4",
                "IS_IPV6",
                "REGEXP_LIKE",
                "SUBSTRING_INDEX",
                "STARTSWITH",
                "ENDSWITH",
                "CONTAINS",
                "RESOLVE_V4",
                "RESOLVE_V6",
                "CONCAT",
                "NETWORK"
              ],
              "type": "string"
            }
          },
          "variables": {
            "type": "array",
            "items": {
              "enum": [
                "router.localCluster",
                "router.hostname",
                "router.bindAddress",
                "router.port.ro",
                "router.port.rw",
                "router.port.rw_split",
                "router.routeName",
                "server.label",
                "server.address",
                "server.port",
                "server.uuid",
                "server.version",
                "server.clusterName",
                "server.clusterSetName",
                "server.isClusterInvalidated",
                "server.memberRole",
                "server.clusterRole",
                "session.targetIP",
                "session.targetPort",
                "session.sourceIP",
                "session.randomValue",
                "session.user",
                "session.schema"
              ],
              "type": "string"
            }
          }
        },
        "additionalProperties": false
      },
      "ConfigurationChangesSchema": {
        "type": "object",
        "title": "MySQL Router configuration JSON schema",
        "$schema": "http://json-schema.org/draft-04/schema#",
        "properties": {
          "routing_rules": {
            "type": "object",
            "properties": {
              "guideline": {
                "type": "string"
              },
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
        "description": "JSON Schema for the Router configuration options that can be changed in the runtime. Shared by the Router in the metadata to announce which options it supports changing.",
        "additionalProperties": false
      }
    }
  },
  "read_only_targets": "secondaries"
}

session.runSql(
  "UPDATE `mysql_innodb_cluster_metadata`.`clusters` SET router_options = ? WHERE cluster_id = ?",
  [JSON.stringify(router_options_json), cluster_id]
);

const router_configuration_json =
{
  "ROEndpoint": "6447",
  "RWEndpoint": "6446",
  "ROXEndpoint": "6449",
  "RWXEndpoint": "6448",
  "LocalCluster": "devCluster",
  "MetadataUser": "mysql_router1_lyrks5g",
  "Configuration": {
    "io": {
      "backend": "linux_epoll",
      "threads": 0
    },
    "common": {
      "name": "my_router",
      "socket": "",
      "bind_address": "0.0.0.0",
      "read_timeout": 30,
      "server_ssl_ca": "",
      "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
      "server_ssl_crl": "",
      "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
      "client_ssl_mode": "PREFERRED",
      "connect_timeout": 5,
      "max_connections": 0,
      "server_ssl_mode": "PREFERRED",
      "client_ssl_cipher": "",
      "client_ssl_curves": "",
      "net_buffer_length": 16384,
      "server_ssl_capath": "",
      "server_ssl_cipher": "",
      "server_ssl_curves": "",
      "server_ssl_verify": "DISABLED",
      "thread_stack_size": 1024,
      "connection_sharing": false,
      "max_connect_errors": 100,
      "server_ssl_crlpath": "",
      "wait_for_my_writes": true,
      "client_ssl_dh_params": "",
      "max_total_connections": 512,
      "unknown_config_option": "error",
      "client_connect_timeout": 9,
      "connection_sharing_delay": 1.0,
      "wait_for_my_writes_timeout": 2,
      "max_idle_server_connections": 64
    },
    "loggers": {
      "filelog": {
        "level": "info",
        "filename": "mysqlrouter.log",
        "destination": "",
        "timestamp_precision": "second"
      }
    },
    "endpoints": {
      "bootstrap_ro": {
        "socket": "",
        "protocol": "classic",
        "bind_port": 6447,
        "bind_address": "0.0.0.0",
        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
        "server_ssl_ca": "",
        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
        "server_ssl_crl": "",
        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
        "client_ssl_mode": "PREFERRED",
        "connect_timeout": 5,
        "max_connections": 0,
        "server_ssl_mode": "PREFERRED",
        "routing_strategy": "round-robin-with-fallback",
        "client_ssl_cipher": "",
        "client_ssl_curves": "",
        "net_buffer_length": 16384,
        "server_ssl_capath": "",
        "server_ssl_cipher": "",
        "server_ssl_curves": "",
        "server_ssl_verify": "DISABLED",
        "thread_stack_size": 1024,
        "connection_sharing": false,
        "max_connect_errors": 100,
        "server_ssl_crlpath": "",
        "wait_for_my_writes": true,
        "client_ssl_dh_params": "",
        "client_connect_timeout": 9,
        "router_require_enforce": true,
        "connection_sharing_delay": 1.0,
        "wait_for_my_writes_timeout": 2
      },
      "bootstrap_rw": {
        "socket": "",
        "protocol": "classic",
        "bind_port": 6446,
        "bind_address": "0.0.0.0",
        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
        "server_ssl_ca": "",
        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
        "server_ssl_crl": "",
        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
        "client_ssl_mode": "PREFERRED",
        "connect_timeout": 5,
        "max_connections": 0,
        "server_ssl_mode": "PREFERRED",
        "routing_strategy": "first-available",
        "client_ssl_cipher": "",
        "client_ssl_curves": "",
        "net_buffer_length": 16384,
        "server_ssl_capath": "",
        "server_ssl_cipher": "",
        "server_ssl_curves": "",
        "server_ssl_verify": "DISABLED",
        "thread_stack_size": 1024,
        "connection_sharing": false,
        "max_connect_errors": 100,
        "server_ssl_crlpath": "",
        "wait_for_my_writes": true,
        "client_ssl_dh_params": "",
        "client_connect_timeout": 9,
        "router_require_enforce": true,
        "connection_sharing_delay": 1.0,
        "wait_for_my_writes_timeout": 2
      },
      "bootstrap_x_ro": {
        "socket": "",
        "protocol": "x",
        "bind_port": 6449,
        "bind_address": "0.0.0.0",
        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
        "server_ssl_ca": "",
        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
        "server_ssl_crl": "",
        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
        "client_ssl_mode": "PREFERRED",
        "connect_timeout": 5,
        "max_connections": 0,
        "server_ssl_mode": "PREFERRED",
        "routing_strategy": "round-robin-with-fallback",
        "client_ssl_cipher": "",
        "client_ssl_curves": "",
        "net_buffer_length": 16384,
        "server_ssl_capath": "",
        "server_ssl_cipher": "",
        "server_ssl_curves": "",
        "server_ssl_verify": "DISABLED",
        "thread_stack_size": 1024,
        "connection_sharing": false,
        "max_connect_errors": 100,
        "server_ssl_crlpath": "",
        "wait_for_my_writes": true,
        "client_ssl_dh_params": "",
        "client_connect_timeout": 9,
        "router_require_enforce": false,
        "connection_sharing_delay": 1.0,
        "wait_for_my_writes_timeout": 2
      },
      "bootstrap_x_rw": {
        "socket": "",
        "protocol": "x",
        "bind_port": 6448,
        "bind_address": "0.0.0.0",
        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
        "server_ssl_ca": "",
        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
        "server_ssl_crl": "",
        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
        "client_ssl_mode": "PREFERRED",
        "connect_timeout": 5,
        "max_connections": 0,
        "server_ssl_mode": "PREFERRED",
        "routing_strategy": "first-available",
        "client_ssl_cipher": "",
        "client_ssl_curves": "",
        "net_buffer_length": 16384,
        "server_ssl_capath": "",
        "server_ssl_cipher": "",
        "server_ssl_curves": "",
        "server_ssl_verify": "DISABLED",
        "thread_stack_size": 1024,
        "connection_sharing": false,
        "max_connect_errors": 100,
        "server_ssl_crlpath": "",
        "wait_for_my_writes": true,
        "client_ssl_dh_params": "",
        "client_connect_timeout": 9,
        "router_require_enforce": false,
        "connection_sharing_delay": 1.0,
        "wait_for_my_writes_timeout": 2
      },
      "bootstrap_rw_split": {
        "socket": "",
        "protocol": "classic",
        "bind_port": 6450,
        "access_mode": "auto",
        "bind_address": "0.0.0.0",
        "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
        "server_ssl_ca": "",
        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
        "server_ssl_crl": "",
        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
        "client_ssl_mode": "PREFERRED",
        "connect_timeout": 5,
        "max_connections": 0,
        "server_ssl_mode": "PREFERRED",
        "routing_strategy": "round-robin",
        "client_ssl_cipher": "",
        "client_ssl_curves": "",
        "net_buffer_length": 16384,
        "server_ssl_capath": "",
        "server_ssl_cipher": "",
        "server_ssl_curves": "",
        "server_ssl_verify": "DISABLED",
        "thread_stack_size": 1024,
        "connection_sharing": true,
        "max_connect_errors": 100,
        "server_ssl_crlpath": "",
        "wait_for_my_writes": true,
        "client_ssl_dh_params": "",
        "client_connect_timeout": 9,
        "router_require_enforce": true,
        "connection_sharing_delay": 1.0,
        "wait_for_my_writes_timeout": 2
      }
    },
    "http_server": {
      "ssl": true,
      "port": 8443,
      "ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
      "ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
      "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
      "ssl_curves": "",
      "bind_address": "0.0.0.0",
      "require_realm": "",
      "ssl_dh_params": "",
      "static_folder": ""
    },
    "rest_configs": {
      "rest_api": {
        "require_realm": ""
      },
      "rest_router": {
        "require_realm": "default_auth_realm"
      },
      "rest_routing": {
        "require_realm": "default_auth_realm"
      },
      "rest_metadata_cache": {
        "require_realm": "default_auth_realm"
      }
    },
    "routing_rules": {
      "read_only_targets": "secondaries",
      "stats_updates_frequency": -1,
      "use_replica_primary_as_rw": false,
      "invalidated_cluster_policy": "drop_all",
      "unreachable_quorum_allowed_traffic": "none"
    },
    "metadata_cache": {
      "ttl": 0.5,
      "user": "mysql_router1_lyrks5g",
      "ssl_ca": "",
      "ssl_crl": "",
      "ssl_mode": "PREFERRED",
      "ssl_capath": "",
      "ssl_cipher": "",
      "ssl_crlpath": "",
      "tls_version": "",
      "read_timeout": 30,
      "auth_cache_ttl": -1.0,
      "connect_timeout": 5,
      "thread_stack_size": 1024,
      "use_gr_notifications": false,
      "auth_cache_refresh_interval": 2.0,
      "close_connection_after_refresh": false
    },
    "connection_pool": {
      "idle_timeout": 5,
      "max_idle_server_connections": 64
    },
    "destination_status": {
      "error_quarantine_interval": 1,
      "error_quarantine_threshold": 1
    },
    "http_authentication_realm": {
      "name": "default_realm",
      "method": "basic",
      "backend": "default_auth_backend",
      "require": "valid-user"
    },
    "http_authentication_backends": {
      "default_auth_backend": {
        "backend": "metadata_cache",
        "filename": ""
      }
    }
  },
  "RWSplitEndpoint": "6450",
  "bootstrapTargetType": "cluster",
  "SupportedRoutingGuidelinesVersion": "1.0"
};

session.runSql(
  "INSERT INTO `mysql_innodb_cluster_metadata`.`routers` VALUES (?, ?, ?, ?, ?, NULL, ?, ?, NULL, NULL)",
  [
    1, // ID
    "system", // Name
    "MySQL Router", // Description
    "routerhost1", // Hostname
    "9.2.0", // Version
    JSON.stringify(router_configuration_json), // Configuration
    cluster_id, // Cluster ID
  ]
);

var router2 = "routerhost2::system";

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.1.0', '2024-02-14 11:22:33', '{\"bootstrapTargetType\": \"cluster\", \"ROEndpoint\": \"mysqlro.sock\", \"RWEndpoint\": \"mysql.sock\", \"ROXEndpoint\": \"mysqlxro.sock\", \"RWXEndpoint\": \"mysqlx.sock\"}', ?, NULL, NULL)", [cluster_id]);

//@<> cluster.routerOptions(): defaults
cluster.routerOptions();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {
        "routing_rules": {
            "invalidated_cluster_policy": "drop_all",
            "read_only_targets": "secondaries",
            "stats_updates_frequency": -1,
            "tags": {},
            "unreachable_quorum_allowed_traffic": "none",
            "use_replica_primary_as_rw": false
        }
    },
    "routers": {
        "routerhost1::system": {
            "configuration": {}
        },
        "routerhost2::system": {
            "configuration": {}
        }
    }
}
`
);

//@<> cluster.routerOptions(): extended:1
cluster.routerOptions({extended:1});

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {
        "common": {
            "bind_address": "0.0.0.0",
            "client_connect_timeout": 9,
            "client_ssl_cipher": "",
            "client_ssl_curves": "",
            "client_ssl_dh_params": "",
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "connection_sharing": false,
            "connection_sharing_delay": 1,
            "max_connect_errors": 100,
            "max_connections": 0,
            "max_idle_server_connections": 64,
            "max_total_connections": 512,
            "name": "system",
            "net_buffer_length": 16384,
            "read_timeout": 30,
            "server_ssl_ca": "",
            "server_ssl_capath": "",
            "server_ssl_cipher": "",
            "server_ssl_crl": "",
            "server_ssl_crlpath": "",
            "server_ssl_curves": "",
            "server_ssl_mode": "PREFERRED",
            "server_ssl_verify": "DISABLED",
            "socket": "",
            "thread_stack_size": 1024,
            "unknown_config_option": "error",
            "wait_for_my_writes": true,
            "wait_for_my_writes_timeout": 2
        },
        "connection_pool": {
            "idle_timeout": 5
        },
        "destination_status": {
            "error_quarantine_interval": 1,
            "error_quarantine_threshold": 1
        },
        "endpoints": {
            "bootstrap_ro": {
                "bind_port": 6447,
                "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "round-robin-with-fallback"
            },
            "bootstrap_rw": {
                "bind_port": 6446,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "first-available"
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_port": 6450,
                "connection_sharing": true,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "round-robin"
            },
            "bootstrap_x_ro": {
                "bind_port": 6449,
                "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                "protocol": "x",
                "router_require_enforce": false,
                "routing_strategy": "round-robin-with-fallback"
            },
            "bootstrap_x_rw": {
                "bind_port": 6448,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                "protocol": "x",
                "router_require_enforce": false,
                "routing_strategy": "first-available"
            }
        },
        "http_authentication_backends": {
            "default_auth_backend": {
                "backend": "metadata_cache",
                "filename": ""
            }
        },
        "http_authentication_realm": {
            "backend": "default_auth_backend",
            "method": "basic",
            "name": "default_realm",
            "require": "valid-user"
        },
        "http_server": {
            "port": 8443,
            "require_realm": "",
            "ssl": true,
            "ssl_cert": "",
            "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
            "ssl_curves": "",
            "ssl_dh_params": "",
            "ssl_key": "",
            "static_folder": ""
        },
        "io": {
            "threads": 0
        },
        "loggers": {
            "filelog": {
                "destination": "",
                "filename": "mysqlrouter.log",
                "level": "info",
                "timestamp_precision": "second"
            }
        },
        "metadata_cache": {
            "auth_cache_refresh_interval": 2,
            "auth_cache_ttl": -1,
            "close_connection_after_refresh": false,
            "ssl_ca": "",
            "ssl_capath": "",
            "ssl_cipher": "",
            "ssl_crl": "",
            "ssl_crlpath": "",
            "ssl_mode": "PREFERRED",
            "tls_version": "",
            "ttl": 0.5,
            "use_gr_notifications": false
        },
        "rest_configs": {
            "rest_api": {
                "require_realm": ""
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
            "tags": {},
            "unreachable_quorum_allowed_traffic": "none",
            "use_replica_primary_as_rw": false
        }
    },
    "routers": {
        "routerhost1::system": {
            "configuration": {
                "common": {
                    "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "name": "my_router"
                },
                "http_server": {
                    "ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem"
                },
                "io": {
                    "backend": "linux_epoll"
                },
                "metadata_cache": {
                    "user": "mysql_router1_lyrks5g"
                }
            }
        },
        "routerhost2::system": {
            "configuration": {}
        }
    }
}
`
);

//@<> cluster.routerOptions(): extended:2
cluster.routerOptions({extended:2});

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {
        "common": {
            "bind_address": "0.0.0.0",
            "client_connect_timeout": 9,
            "client_ssl_cipher": "",
            "client_ssl_curves": "",
            "client_ssl_dh_params": "",
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "connection_sharing": false,
            "connection_sharing_delay": 1,
            "max_connect_errors": 100,
            "max_connections": 0,
            "max_idle_server_connections": 64,
            "max_total_connections": 512,
            "name": "system",
            "net_buffer_length": 16384,
            "read_timeout": 30,
            "server_ssl_ca": "",
            "server_ssl_capath": "",
            "server_ssl_cipher": "",
            "server_ssl_crl": "",
            "server_ssl_crlpath": "",
            "server_ssl_curves": "",
            "server_ssl_mode": "PREFERRED",
            "server_ssl_verify": "DISABLED",
            "socket": "",
            "thread_stack_size": 1024,
            "unknown_config_option": "error",
            "wait_for_my_writes": true,
            "wait_for_my_writes_timeout": 2
        },
        "connection_pool": {
            "idle_timeout": 5,
            "max_idle_server_connections": 64
        },
        "destination_status": {
            "error_quarantine_interval": 1,
            "error_quarantine_threshold": 1
        },
        "endpoints": {
            "bootstrap_ro": {
                "bind_address": "0.0.0.0",
                "bind_port": 6447,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            },
            "bootstrap_rw": {
                "bind_address": "0.0.0.0",
                "bind_port": 6446,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_address": "0.0.0.0",
                "bind_port": 6450,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": true,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "round-robin",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            },
            "bootstrap_x_ro": {
                "bind_address": "0.0.0.0",
                "bind_port": 6449,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "x",
                "router_require_enforce": false,
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            },
            "bootstrap_x_rw": {
                "bind_address": "0.0.0.0",
                "bind_port": 6448,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "x",
                "router_require_enforce": false,
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            }
        },
        "http_authentication_backends": {
            "default_auth_backend": {
                "backend": "metadata_cache",
                "filename": ""
            }
        },
        "http_authentication_realm": {
            "backend": "default_auth_backend",
            "method": "basic",
            "name": "default_realm",
            "require": "valid-user"
        },
        "http_server": {
            "bind_address": "0.0.0.0",
            "port": 8443,
            "require_realm": "",
            "ssl": true,
            "ssl_cert": "",
            "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
            "ssl_curves": "",
            "ssl_dh_params": "",
            "ssl_key": "",
            "static_folder": ""
        },
        "io": {
            "threads": 0
        },
        "loggers": {
            "filelog": {
                "destination": "",
                "filename": "mysqlrouter.log",
                "level": "info",
                "timestamp_precision": "second"
            }
        },
        "metadata_cache": {
            "auth_cache_refresh_interval": 2,
            "auth_cache_ttl": -1,
            "close_connection_after_refresh": false,
            "connect_timeout": 5,
            "read_timeout": 30,
            "ssl_ca": "",
            "ssl_capath": "",
            "ssl_cipher": "",
            "ssl_crl": "",
            "ssl_crlpath": "",
            "ssl_mode": "PREFERRED",
            "thread_stack_size": 1024,
            "tls_version": "",
            "ttl": 0.5,
            "use_gr_notifications": false
        },
        "rest_configs": {
            "rest_api": {
                "require_realm": ""
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
            "tags": {},
            "unreachable_quorum_allowed_traffic": "none",
            "use_replica_primary_as_rw": false
        }
    },
    "routers": {
        "routerhost1::system": {
            "configuration": {
                "common": {
                    "bind_address": "0.0.0.0",
                    "client_connect_timeout": 9,
                    "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "client_ssl_cipher": "",
                    "client_ssl_curves": "",
                    "client_ssl_dh_params": "",
                    "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "client_ssl_mode": "PREFERRED",
                    "connect_timeout": 5,
                    "connection_sharing": false,
                    "connection_sharing_delay": 1,
                    "max_connect_errors": 100,
                    "max_connections": 0,
                    "max_idle_server_connections": 64,
                    "max_total_connections": 512,
                    "name": "my_router",
                    "net_buffer_length": 16384,
                    "read_timeout": 30,
                    "server_ssl_ca": "",
                    "server_ssl_capath": "",
                    "server_ssl_cipher": "",
                    "server_ssl_crl": "",
                    "server_ssl_crlpath": "",
                    "server_ssl_curves": "",
                    "server_ssl_mode": "PREFERRED",
                    "server_ssl_verify": "DISABLED",
                    "socket": "",
                    "thread_stack_size": 1024,
                    "unknown_config_option": "error",
                    "wait_for_my_writes": true,
                    "wait_for_my_writes_timeout": 2
                },
                "connection_pool": {
                    "idle_timeout": 5,
                    "max_idle_server_connections": 64
                },
                "destination_status": {
                    "error_quarantine_interval": 1,
                    "error_quarantine_threshold": 1
                },
                "endpoints": {
                    "bootstrap_ro": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6447,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "round-robin-with-fallback",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_rw": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6446,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "first-available",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_rw_split": {
                        "access_mode": "auto",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6450,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": true,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "round-robin",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_x_ro": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6449,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "x",
                        "router_require_enforce": false,
                        "routing_strategy": "round-robin-with-fallback",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_x_rw": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6448,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "x",
                        "router_require_enforce": false,
                        "routing_strategy": "first-available",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    }
                },
                "http_authentication_backends": {
                    "default_auth_backend": {
                        "backend": "metadata_cache",
                        "filename": ""
                    }
                },
                "http_authentication_realm": {
                    "backend": "default_auth_backend",
                    "method": "basic",
                    "name": "default_realm",
                    "require": "valid-user"
                },
                "http_server": {
                    "bind_address": "0.0.0.0",
                    "port": 8443,
                    "require_realm": "",
                    "ssl": true,
                    "ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
                    "ssl_curves": "",
                    "ssl_dh_params": "",
                    "ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "static_folder": ""
                },
                "io": {
                    "backend": "linux_epoll",
                    "threads": 0
                },
                "loggers": {
                    "filelog": {
                        "destination": "",
                        "filename": "mysqlrouter.log",
                        "level": "info",
                        "timestamp_precision": "second"
                    }
                },
                "metadata_cache": {
                    "auth_cache_refresh_interval": 2,
                    "auth_cache_ttl": -1,
                    "close_connection_after_refresh": false,
                    "connect_timeout": 5,
                    "read_timeout": 30,
                    "ssl_ca": "",
                    "ssl_capath": "",
                    "ssl_cipher": "",
                    "ssl_crl": "",
                    "ssl_crlpath": "",
                    "ssl_mode": "PREFERRED",
                    "thread_stack_size": 1024,
                    "tls_version": "",
                    "ttl": 0.5,
                    "use_gr_notifications": false,
                    "user": "mysql_router1_lyrks5g"
                },
                "rest_configs": {
                    "rest_api": {
                        "require_realm": ""
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
                    "tags": {},
                    "unreachable_quorum_allowed_traffic": "none",
                    "use_replica_primary_as_rw": false
                }
            }
        },
        "routerhost2::system": {
            "configuration": {
                "routing_rules": {
                    "read_only_targets": "secondaries",
                    "tags": {}
                }
            }
        }
    }
}
`
);

//@<> cluster.routerOptions(): router: "domus::router_test" (should fail)
EXPECT_THROWS(function(){ cluster.routerOptions({router: "domus::router_test"}); }, "Router 'domus::router_test' is not registered in the cluster");


//@<> cluster.routerOptions(): router: "routerhost1::system"
cluster.routerOptions({router: "routerhost1::system"});

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {
        "routing_rules": {
            "invalidated_cluster_policy": "drop_all",
            "read_only_targets": "secondaries",
            "stats_updates_frequency": -1,
            "tags": {},
            "unreachable_quorum_allowed_traffic": "none",
            "use_replica_primary_as_rw": false
        }
    },
    "routers": {
        "routerhost1::system": {
            "configuration": {}
        }
    }
}
`);

//@<> cluster.routerOptions(): router: "routerhost1::system" + extended:1
cluster.routerOptions({router: "routerhost1::system", extended:1});

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {
        "common": {
            "bind_address": "0.0.0.0",
            "client_connect_timeout": 9,
            "client_ssl_cipher": "",
            "client_ssl_curves": "",
            "client_ssl_dh_params": "",
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "connection_sharing": false,
            "connection_sharing_delay": 1,
            "max_connect_errors": 100,
            "max_connections": 0,
            "max_idle_server_connections": 64,
            "max_total_connections": 512,
            "name": "system",
            "net_buffer_length": 16384,
            "read_timeout": 30,
            "server_ssl_ca": "",
            "server_ssl_capath": "",
            "server_ssl_cipher": "",
            "server_ssl_crl": "",
            "server_ssl_crlpath": "",
            "server_ssl_curves": "",
            "server_ssl_mode": "PREFERRED",
            "server_ssl_verify": "DISABLED",
            "socket": "",
            "thread_stack_size": 1024,
            "unknown_config_option": "error",
            "wait_for_my_writes": true,
            "wait_for_my_writes_timeout": 2
        },
        "connection_pool": {
            "idle_timeout": 5
        },
        "destination_status": {
            "error_quarantine_interval": 1,
            "error_quarantine_threshold": 1
        },
        "endpoints": {
            "bootstrap_ro": {
                "bind_port": 6447,
                "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "round-robin-with-fallback"
            },
            "bootstrap_rw": {
                "bind_port": 6446,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "first-available"
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_port": 6450,
                "connection_sharing": true,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "round-robin"
            },
            "bootstrap_x_ro": {
                "bind_port": 6449,
                "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                "protocol": "x",
                "router_require_enforce": false,
                "routing_strategy": "round-robin-with-fallback"
            },
            "bootstrap_x_rw": {
                "bind_port": 6448,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                "protocol": "x",
                "router_require_enforce": false,
                "routing_strategy": "first-available"
            }
        },
        "http_authentication_backends": {
            "default_auth_backend": {
                "backend": "metadata_cache",
                "filename": ""
            }
        },
        "http_authentication_realm": {
            "backend": "default_auth_backend",
            "method": "basic",
            "name": "default_realm",
            "require": "valid-user"
        },
        "http_server": {
            "port": 8443,
            "require_realm": "",
            "ssl": true,
            "ssl_cert": "",
            "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
            "ssl_curves": "",
            "ssl_dh_params": "",
            "ssl_key": "",
            "static_folder": ""
        },
        "io": {
            "threads": 0
        },
        "loggers": {
            "filelog": {
                "destination": "",
                "filename": "mysqlrouter.log",
                "level": "info",
                "timestamp_precision": "second"
            }
        },
        "metadata_cache": {
            "auth_cache_refresh_interval": 2,
            "auth_cache_ttl": -1,
            "close_connection_after_refresh": false,
            "ssl_ca": "",
            "ssl_capath": "",
            "ssl_cipher": "",
            "ssl_crl": "",
            "ssl_crlpath": "",
            "ssl_mode": "PREFERRED",
            "tls_version": "",
            "ttl": 0.5,
            "use_gr_notifications": false
        },
        "rest_configs": {
            "rest_api": {
                "require_realm": ""
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
            "tags": {},
            "unreachable_quorum_allowed_traffic": "none",
            "use_replica_primary_as_rw": false
        }
    },
    "routers": {
        "routerhost1::system": {
            "configuration": {
                "common": {
                    "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "name": "my_router"
                },
                "http_server": {
                    "ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem"
                },
                "io": {
                    "backend": "linux_epoll"
                },
                "metadata_cache": {
                    "user": "mysql_router1_lyrks5g"
                }
            }
        }
    }
}
`);

//@<> cluster.routerOptions(): router: "routerhost1::system" + extended:2
cluster.routerOptions({router: "routerhost1::system", extended:2});

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {
        "common": {
            "bind_address": "0.0.0.0",
            "client_connect_timeout": 9,
            "client_ssl_cipher": "",
            "client_ssl_curves": "",
            "client_ssl_dh_params": "",
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "connection_sharing": false,
            "connection_sharing_delay": 1,
            "max_connect_errors": 100,
            "max_connections": 0,
            "max_idle_server_connections": 64,
            "max_total_connections": 512,
            "name": "system",
            "net_buffer_length": 16384,
            "read_timeout": 30,
            "server_ssl_ca": "",
            "server_ssl_capath": "",
            "server_ssl_cipher": "",
            "server_ssl_crl": "",
            "server_ssl_crlpath": "",
            "server_ssl_curves": "",
            "server_ssl_mode": "PREFERRED",
            "server_ssl_verify": "DISABLED",
            "socket": "",
            "thread_stack_size": 1024,
            "unknown_config_option": "error",
            "wait_for_my_writes": true,
            "wait_for_my_writes_timeout": 2
        },
        "connection_pool": {
            "idle_timeout": 5,
            "max_idle_server_connections": 64
        },
        "destination_status": {
            "error_quarantine_interval": 1,
            "error_quarantine_threshold": 1
        },
        "endpoints": {
            "bootstrap_ro": {
                "bind_address": "0.0.0.0",
                "bind_port": 6447,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            },
            "bootstrap_rw": {
                "bind_address": "0.0.0.0",
                "bind_port": 6446,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_address": "0.0.0.0",
                "bind_port": 6450,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": true,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "classic",
                "router_require_enforce": true,
                "routing_strategy": "round-robin",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            },
            "bootstrap_x_ro": {
                "bind_address": "0.0.0.0",
                "bind_port": 6449,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "x",
                "router_require_enforce": false,
                "routing_strategy": "round-robin-with-fallback",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            },
            "bootstrap_x_rw": {
                "bind_address": "0.0.0.0",
                "bind_port": 6448,
                "client_connect_timeout": 9,
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                "max_connect_errors": 100,
                "max_connections": 0,
                "net_buffer_length": 16384,
                "protocol": "x",
                "router_require_enforce": false,
                "routing_strategy": "first-available",
                "server_ssl_ca": "",
                "server_ssl_capath": "",
                "server_ssl_cipher": "",
                "server_ssl_crl": "",
                "server_ssl_crlpath": "",
                "server_ssl_curves": "",
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "socket": "",
                "thread_stack_size": 1024,
                "wait_for_my_writes": true,
                "wait_for_my_writes_timeout": 2
            }
        },
        "http_authentication_backends": {
            "default_auth_backend": {
                "backend": "metadata_cache",
                "filename": ""
            }
        },
        "http_authentication_realm": {
            "backend": "default_auth_backend",
            "method": "basic",
            "name": "default_realm",
            "require": "valid-user"
        },
        "http_server": {
            "bind_address": "0.0.0.0",
            "port": 8443,
            "require_realm": "",
            "ssl": true,
            "ssl_cert": "",
            "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
            "ssl_curves": "",
            "ssl_dh_params": "",
            "ssl_key": "",
            "static_folder": ""
        },
        "io": {
            "threads": 0
        },
        "loggers": {
            "filelog": {
                "destination": "",
                "filename": "mysqlrouter.log",
                "level": "info",
                "timestamp_precision": "second"
            }
        },
        "metadata_cache": {
            "auth_cache_refresh_interval": 2,
            "auth_cache_ttl": -1,
            "close_connection_after_refresh": false,
            "connect_timeout": 5,
            "read_timeout": 30,
            "ssl_ca": "",
            "ssl_capath": "",
            "ssl_cipher": "",
            "ssl_crl": "",
            "ssl_crlpath": "",
            "ssl_mode": "PREFERRED",
            "thread_stack_size": 1024,
            "tls_version": "",
            "ttl": 0.5,
            "use_gr_notifications": false
        },
        "rest_configs": {
            "rest_api": {
                "require_realm": ""
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
            "tags": {},
            "unreachable_quorum_allowed_traffic": "none",
            "use_replica_primary_as_rw": false
        }
    },
    "routers": {
        "routerhost1::system": {
            "configuration": {
                "common": {
                    "bind_address": "0.0.0.0",
                    "client_connect_timeout": 9,
                    "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "client_ssl_cipher": "",
                    "client_ssl_curves": "",
                    "client_ssl_dh_params": "",
                    "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "client_ssl_mode": "PREFERRED",
                    "connect_timeout": 5,
                    "connection_sharing": false,
                    "connection_sharing_delay": 1,
                    "max_connect_errors": 100,
                    "max_connections": 0,
                    "max_idle_server_connections": 64,
                    "max_total_connections": 512,
                    "name": "my_router",
                    "net_buffer_length": 16384,
                    "read_timeout": 30,
                    "server_ssl_ca": "",
                    "server_ssl_capath": "",
                    "server_ssl_cipher": "",
                    "server_ssl_crl": "",
                    "server_ssl_crlpath": "",
                    "server_ssl_curves": "",
                    "server_ssl_mode": "PREFERRED",
                    "server_ssl_verify": "DISABLED",
                    "socket": "",
                    "thread_stack_size": 1024,
                    "unknown_config_option": "error",
                    "wait_for_my_writes": true,
                    "wait_for_my_writes_timeout": 2
                },
                "connection_pool": {
                    "idle_timeout": 5,
                    "max_idle_server_connections": 64
                },
                "destination_status": {
                    "error_quarantine_interval": 1,
                    "error_quarantine_threshold": 1
                },
                "endpoints": {
                    "bootstrap_ro": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6447,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "round-robin-with-fallback",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_rw": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6446,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "first-available",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_rw_split": {
                        "access_mode": "auto",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6450,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": true,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "round-robin",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_x_ro": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6449,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "x",
                        "router_require_enforce": false,
                        "routing_strategy": "round-robin-with-fallback",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_x_rw": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6448,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "x",
                        "router_require_enforce": false,
                        "routing_strategy": "first-available",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    }
                },
                "http_authentication_backends": {
                    "default_auth_backend": {
                        "backend": "metadata_cache",
                        "filename": ""
                    }
                },
                "http_authentication_realm": {
                    "backend": "default_auth_backend",
                    "method": "basic",
                    "name": "default_realm",
                    "require": "valid-user"
                },
                "http_server": {
                    "bind_address": "0.0.0.0",
                    "port": 8443,
                    "require_realm": "",
                    "ssl": true,
                    "ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
                    "ssl_curves": "",
                    "ssl_dh_params": "",
                    "ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "static_folder": ""
                },
                "io": {
                    "backend": "linux_epoll",
                    "threads": 0
                },
                "loggers": {
                    "filelog": {
                        "destination": "",
                        "filename": "mysqlrouter.log",
                        "level": "info",
                        "timestamp_precision": "second"
                    }
                },
                "metadata_cache": {
                    "auth_cache_refresh_interval": 2,
                    "auth_cache_ttl": -1,
                    "close_connection_after_refresh": false,
                    "connect_timeout": 5,
                    "read_timeout": 30,
                    "ssl_ca": "",
                    "ssl_capath": "",
                    "ssl_cipher": "",
                    "ssl_crl": "",
                    "ssl_crlpath": "",
                    "ssl_mode": "PREFERRED",
                    "thread_stack_size": 1024,
                    "tls_version": "",
                    "ttl": 0.5,
                    "use_gr_notifications": false,
                    "user": "mysql_router1_lyrks5g"
                },
                "rest_configs": {
                    "rest_api": {
                        "require_realm": ""
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
                    "tags": {},
                    "unreachable_quorum_allowed_traffic": "none",
                    "use_replica_primary_as_rw": false
                }
            }
        }
    }
}
`
);

/*
When Shell detects that a Router is running 8.4.0+ but the global dynamic options per version is not exposed in the corresponding topology table, a warning must be added to the corresponding Router dictionary in the output of routerOptions() indicating that Shell unable to fetch all configurations and the Router needs to be re-bootstrapped and/or the Router account needs to be upgraded.
*/

//@<> A warning must be included in the output when there's a router running 8.4.0+ but no global options are exposed in the topology table
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET version = '9.3.0' WHERE address = 'routerhost1'");

cluster.routerOptions();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {
        "routing_rules": {
            "read_only_targets": "secondaries",
            "tags": {}
        }
    },
    "routers": {
        "routerhost1::system": {
            "configuration": {},
            "routerErrors": [
                "WARNING: Unable to fetch all configuration options: Please ensure the Router account has the right privileges using <Cluster>.setupRouterAccount() and re-bootstrap Router."
            ]
        },
        "routerhost2::system": {
            "configuration": {}
        }
    }
}
`);

//@<> The same check must be done in cluster.listRouters()
cluster.listRouters();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "routers": {
        "routerhost1::system": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost1",
            "lastCheckIn": null,
            "localCluster": "devCluster",
            "roPort": "6447",
            "roXPort": "6449",
            "routerErrors": [
                "WARNING: Unable to fetch all configuration options: Please ensure the Router account has the right privileges using <Cluster>.setupRouterAccount() and re-bootstrap Router."
            ],
            "rwPort": "6446",
            "rwSplitPort": "6450",
            "rwXPort": "6448",
            "supportedRoutingGuidelinesVersion": "1.0",
            "version": "9.3.0"
        },
        "routerhost2::system": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost2",
            "lastCheckIn": "2024-02-14 11:22:33",
            "localCluster": null,
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "rwPort": "mysql.sock",
            "rwSplitPort": null,
            "rwXPort": "mysqlx.sock",
            "supportedRoutingGuidelinesVersion": null,
            "version": "8.1.0"
        }
    }
}
`);

session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET version = '9.2.0' WHERE address = 'routerhost1'");

//@<> The same check must be done in clusterset.listRouters()

// Without doing a re-bootstrap, the error is the same
clusterset = cluster.createClusterSet("cs");
clusterset.listRouters();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "domainName": "cs",
    "routers": {
        "routerhost1::system": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost1",
            "lastCheckIn": null,
            "localCluster": "devCluster",
            "roPort": "6447",
            "roXPort": "6449",
            "routerErrors": [
                "WARNING: Router must be bootstrapped again for the ClusterSet to be recognized.",
                "WARNING: Unable to fetch all configuration options: Please ensure the Router account has the right privileges using <Cluster>.setupRouterAccount() and re-bootstrap Router."
            ],
            "rwPort": "6446",
            "rwSplitPort": "6450",
            "rwXPort": "6448",
            "supportedRoutingGuidelinesVersion": "1.0",
            "targetCluster": null,
            "version": "9.2.0"
        },
        "routerhost2::system": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost2",
            "lastCheckIn": "2024-02-14 11:22:33",
            "localCluster": null,
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "routerErrors": [
                "WARNING: Router must be bootstrapped again for the ClusterSet to be recognized."
            ],
            "rwPort": "mysql.sock",
            "rwSplitPort": null,
            "rwXPort": "mysqlx.sock",
            "supportedRoutingGuidelinesVersion": null,
            "targetCluster": null,
            "version": "8.1.0"
        }
    }
}
`);

//@<> clusterset.routerOptions() should display the legacy routing options although routers haven't been re-bootstrapped yet
clusterset.routerOptions();
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "configuration": {
        "routing_rules": {
            "invalidated_cluster_policy": "drop_all",
            "read_only_targets": "secondaries",
            "tags": {},
            "target_cluster": "primary",
            "use_replica_primary_as_rw": false
        }
    },
    "domainName": "cs",
    "routers": {
        "routerhost1::system": {
            "configuration": {},
            "routerErrors": [
                "WARNING: Unable to fetch all configuration options: Please ensure the Router account has the right privileges using <Cluster>.setupRouterAccount() and re-bootstrap Router."
            ]
        },
        "routerhost2::system": {
            "configuration": {}
        }
    }
}
`);

WIPE_STDOUT()
clusterset.routerOptions({extended:1})
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "configuration": {
        "routing_rules": {
            "invalidated_cluster_policy": "drop_all",
            "read_only_targets": "secondaries",
            "tags": {},
            "target_cluster": "primary",
            "use_replica_primary_as_rw": false
        }
    },
    "domainName": "cs",
    "routers": {
        "routerhost1::system": {
            "configuration": {
                "common": {
                    "bind_address": "0.0.0.0",
                    "client_connect_timeout": 9,
                    "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "client_ssl_cipher": "",
                    "client_ssl_curves": "",
                    "client_ssl_dh_params": "",
                    "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "client_ssl_mode": "PREFERRED",
                    "connect_timeout": 5,
                    "connection_sharing": false,
                    "connection_sharing_delay": 1,
                    "max_connect_errors": 100,
                    "max_connections": 0,
                    "max_idle_server_connections": 64,
                    "max_total_connections": 512,
                    "name": "my_router",
                    "net_buffer_length": 16384,
                    "read_timeout": 30,
                    "server_ssl_ca": "",
                    "server_ssl_capath": "",
                    "server_ssl_cipher": "",
                    "server_ssl_crl": "",
                    "server_ssl_crlpath": "",
                    "server_ssl_curves": "",
                    "server_ssl_mode": "PREFERRED",
                    "server_ssl_verify": "DISABLED",
                    "socket": "",
                    "thread_stack_size": 1024,
                    "unknown_config_option": "error",
                    "wait_for_my_writes": true,
                    "wait_for_my_writes_timeout": 2
                },
                "connection_pool": {
                    "idle_timeout": 5
                },
                "destination_status": {
                    "error_quarantine_interval": 1,
                    "error_quarantine_threshold": 1
                },
                "endpoints": {
                    "bootstrap_ro": {
                        "bind_port": 6447,
                        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "round-robin-with-fallback"
                    },
                    "bootstrap_rw": {
                        "bind_port": 6446,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "first-available"
                    },
                    "bootstrap_rw_split": {
                        "access_mode": "auto",
                        "bind_port": 6450,
                        "connection_sharing": true,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "round-robin"
                    },
                    "bootstrap_x_ro": {
                        "bind_port": 6449,
                        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                        "protocol": "x",
                        "router_require_enforce": false,
                        "routing_strategy": "round-robin-with-fallback"
                    },
                    "bootstrap_x_rw": {
                        "bind_port": 6448,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                        "protocol": "x",
                        "router_require_enforce": false,
                        "routing_strategy": "first-available"
                    }
                },
                "http_authentication_backends": {
                    "default_auth_backend": {
                        "backend": "metadata_cache",
                        "filename": ""
                    }
                },
                "http_authentication_realm": {
                    "backend": "default_auth_backend",
                    "method": "basic",
                    "name": "default_realm",
                    "require": "valid-user"
                },
                "http_server": {
                    "port": 8443,
                    "require_realm": "",
                    "ssl": true,
                    "ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
                    "ssl_curves": "",
                    "ssl_dh_params": "",
                    "ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "static_folder": ""
                },
                "io": {
                    "backend": "linux_epoll",
                    "threads": 0
                },
                "loggers": {
                    "filelog": {
                        "destination": "",
                        "filename": "mysqlrouter.log",
                        "level": "info",
                        "timestamp_precision": "second"
                    }
                },
                "metadata_cache": {
                    "auth_cache_refresh_interval": 2,
                    "auth_cache_ttl": -1,
                    "close_connection_after_refresh": false,
                    "ssl_ca": "",
                    "ssl_capath": "",
                    "ssl_cipher": "",
                    "ssl_crl": "",
                    "ssl_crlpath": "",
                    "ssl_mode": "PREFERRED",
                    "tls_version": "",
                    "ttl": 0.5,
                    "use_gr_notifications": false,
                    "user": "mysql_router1_lyrks5g"
                },
                "rest_configs": {
                    "rest_api": {
                        "require_realm": ""
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
                    "stats_updates_frequency": -1,
                    "unreachable_quorum_allowed_traffic": "none"
                }
            },
            "routerErrors": [
                "WARNING: Unable to fetch all configuration options: Please ensure the Router account has the right privileges using <Cluster>.setupRouterAccount() and re-bootstrap Router."
            ]
        },
        "routerhost2::system": {
            "configuration": {}
        }
    }
}
`);

WIPE_STDOUT()
clusterset.routerOptions({extended:2})
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "configuration": {
        "routing_rules": {
            "invalidated_cluster_policy": "drop_all",
            "read_only_targets": "secondaries",
            "tags": {},
            "target_cluster": "primary",
            "use_replica_primary_as_rw": false
        }
    },
    "domainName": "cs",
    "routers": {
        "routerhost1::system": {
            "configuration": {
                "common": {
                    "bind_address": "0.0.0.0",
                    "client_connect_timeout": 9,
                    "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "client_ssl_cipher": "",
                    "client_ssl_curves": "",
                    "client_ssl_dh_params": "",
                    "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "client_ssl_mode": "PREFERRED",
                    "connect_timeout": 5,
                    "connection_sharing": false,
                    "connection_sharing_delay": 1,
                    "max_connect_errors": 100,
                    "max_connections": 0,
                    "max_idle_server_connections": 64,
                    "max_total_connections": 512,
                    "name": "my_router",
                    "net_buffer_length": 16384,
                    "read_timeout": 30,
                    "server_ssl_ca": "",
                    "server_ssl_capath": "",
                    "server_ssl_cipher": "",
                    "server_ssl_crl": "",
                    "server_ssl_crlpath": "",
                    "server_ssl_curves": "",
                    "server_ssl_mode": "PREFERRED",
                    "server_ssl_verify": "DISABLED",
                    "socket": "",
                    "thread_stack_size": 1024,
                    "unknown_config_option": "error",
                    "wait_for_my_writes": true,
                    "wait_for_my_writes_timeout": 2
                },
                "connection_pool": {
                    "idle_timeout": 5,
                    "max_idle_server_connections": 64
                },
                "destination_status": {
                    "error_quarantine_interval": 1,
                    "error_quarantine_threshold": 1
                },
                "endpoints": {
                    "bootstrap_ro": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6447,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "round-robin-with-fallback",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_rw": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6446,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "first-available",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_rw_split": {
                        "access_mode": "auto",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6450,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": true,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY_AND_SECONDARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "classic",
                        "router_require_enforce": true,
                        "routing_strategy": "round-robin",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_x_ro": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6449,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=SECONDARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "x",
                        "router_require_enforce": false,
                        "routing_strategy": "round-robin-with-fallback",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    },
                    "bootstrap_x_rw": {
                        "bind_address": "0.0.0.0",
                        "bind_port": 6448,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://devCluster/?role=PRIMARY",
                        "max_connect_errors": 100,
                        "max_connections": 0,
                        "net_buffer_length": 16384,
                        "protocol": "x",
                        "router_require_enforce": false,
                        "routing_strategy": "first-available",
                        "server_ssl_ca": "",
                        "server_ssl_capath": "",
                        "server_ssl_cipher": "",
                        "server_ssl_crl": "",
                        "server_ssl_crlpath": "",
                        "server_ssl_curves": "",
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "socket": "",
                        "thread_stack_size": 1024,
                        "wait_for_my_writes": true,
                        "wait_for_my_writes_timeout": 2
                    }
                },
                "http_authentication_backends": {
                    "default_auth_backend": {
                        "backend": "metadata_cache",
                        "filename": ""
                    }
                },
                "http_authentication_realm": {
                    "backend": "default_auth_backend",
                    "method": "basic",
                    "name": "default_realm",
                    "require": "valid-user"
                },
                "http_server": {
                    "bind_address": "0.0.0.0",
                    "port": 8443,
                    "require_realm": "",
                    "ssl": true,
                    "ssl_cert": "/home/miguel/work/testbase/ngshell/router_test/data/router-cert.pem",
                    "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
                    "ssl_curves": "",
                    "ssl_dh_params": "",
                    "ssl_key": "/home/miguel/work/testbase/ngshell/router_test/data/router-key.pem",
                    "static_folder": ""
                },
                "io": {
                    "backend": "linux_epoll",
                    "threads": 0
                },
                "loggers": {
                    "filelog": {
                        "destination": "",
                        "filename": "mysqlrouter.log",
                        "level": "info",
                        "timestamp_precision": "second"
                    }
                },
                "metadata_cache": {
                    "auth_cache_refresh_interval": 2,
                    "auth_cache_ttl": -1,
                    "close_connection_after_refresh": false,
                    "connect_timeout": 5,
                    "read_timeout": 30,
                    "ssl_ca": "",
                    "ssl_capath": "",
                    "ssl_cipher": "",
                    "ssl_crl": "",
                    "ssl_crlpath": "",
                    "ssl_mode": "PREFERRED",
                    "thread_stack_size": 1024,
                    "tls_version": "",
                    "ttl": 0.5,
                    "use_gr_notifications": false,
                    "user": "mysql_router1_lyrks5g"
                },
                "rest_configs": {
                    "rest_api": {
                        "require_realm": ""
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
                    "tags": {},
                    "target_cluster": "primary",
                    "unreachable_quorum_allowed_traffic": "none",
                    "use_replica_primary_as_rw": false
                }
            },
            "routerErrors": [
                "WARNING: Unable to fetch all configuration options: Please ensure the Router account has the right privileges using <Cluster>.setupRouterAccount() and re-bootstrap Router."
            ]
        },
        "routerhost2::system": {
            "configuration": {
                "routing_rules": {
                    "invalidated_cluster_policy": "drop_all",
                    "read_only_targets": "secondaries",
                    "tags": {},
                    "target_cluster": "primary",
                    "use_replica_primary_as_rw": false
                }
            }
        }
    }
}
`);

// Simulate a re-bootstrap
var clusterset_id = session.runSql("SELECT clusterset_id FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_id = ?", [cluster_id]).fetchOne()[0];

session.runSql(
  "UPDATE `mysql_innodb_cluster_metadata`.`clustersets` SET router_options = ? WHERE clusterset_id = ?",
  [JSON.stringify(router_options_json), clusterset_id]
);

session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET cluster_id=NULL where address = 'routerhost1'");
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET clusterset_id=? where address = 'routerhost1'", [clusterset_id]);
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET attributes = JSON_SET(attributes, '$.bootstrapTargetType', 'clusterset') WHERE address = 'routerhost1'");

session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET cluster_id=NULL where address = 'routerhost2'");
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET clusterset_id=? where address = 'routerhost2'", [clusterset_id]);
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET attributes = JSON_SET(attributes, '$.bootstrapTargetType', 'clusterset') WHERE address = 'routerhost2'");

//@<> .routerOptions() should automatically translate the target_cluster value from the group uuid to the cluster's name, likewise .listRouters() and .routingOption() do

// Set target_cluster to 'primary'
clusterset.setRoutingOption("target_cluster", "primary");

// extended:0
router_options = clusterset.routerOptions();
target_cluster = router_options["configuration"]["routing_rules"]["target_cluster"];
EXPECT_EQ("primary", target_cluster);

// extended:1
router_options = clusterset.routerOptions({extended:1});
target_cluster = router_options["configuration"]["routing_rules"]["target_cluster"];
EXPECT_EQ("primary", target_cluster);

// extended:1
router_options = clusterset.routerOptions({extended:2});
target_cluster = router_options["configuration"]["routing_rules"]["target_cluster"];
EXPECT_EQ("primary", target_cluster);

// Set to a specific cluster
clusterset.setRoutingOption("routerhost1::system", "target_cluster", "cluster");

// extended:0
router_options = clusterset.routerOptions();
target_cluster = router_options["routers"]["routerhost1::system"]["configuration"]["routing_rules"]["target_cluster"];
EXPECT_EQ("cluster", target_cluster);

// extended:1
router_options = clusterset.routerOptions({extended:1});
target_cluster = router_options["routers"]["routerhost1::system"]["configuration"]["routing_rules"]["target_cluster"];
EXPECT_EQ("cluster", target_cluster);

// extended:2
router_options = clusterset.routerOptions({extended:2});
target_cluster = router_options["routers"]["routerhost1::system"]["configuration"]["routing_rules"]["target_cluster"];
EXPECT_EQ("cluster", target_cluster);

//@<> .removeRouterMetadata() should clear the Defaults Configuration Document if needed
EXPECT_NO_THROWS(function() { cluster.removeRouterMetadata("routerhost2::system"); });

EXPECT_NE(null, session.runSql(`select router_options->'$.Configuration."9.2.0"' from mysql_innodb_cluster_metadata.clusters`).fetchOne());

EXPECT_NO_THROWS(function() { cluster.removeRouterMetadata("routerhost1::system"); });

EXPECT_EQ([null], session.runSql(`select router_options->'$.Configuration."9.2.0"' from mysql_innodb_cluster_metadata.clustersets`).fetchOne());

//@<> Cleanup
scene.destroy();
