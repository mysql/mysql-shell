const default_cluster_guideline =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadReplica"
        }
    ],
    "name": "<replace_me>",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.1"
}

const expected_default_cluster_guideline_destinations = ["Primary", "Secondary", "ReadReplica"];
const expected_default_cluster_guideline_destinations_match = ["$.server.memberRole = PRIMARY", "$.server.memberRole = SECONDARY", "$.server.memberRole = READ_REPLICA"];

const expected_default_cluster_guideline_routes = ["rw", "ro"];
const expected_default_cluster_guideline_routes_match = ["$.session.targetPort = $.router.port.rw", "$.session.targetPort = $.router.port.ro"
];
const expected_default_cluster_guideline_routes_destinations = ["round-robin(Primary)", "round-robin(Secondary), round-robin(Primary)"];

const cluster_guideline_ro_targets_read_replicas =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadReplica"
        }
    ],
    "name": "<replace_me>",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "ReadReplica"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.1"
}

const expected_cluster_guideline_ro_targets_read_replicas_routes_destinations = ["round-robin(Primary)", "round-robin(ReadReplica), round-robin(Primary)"];

const cluster_guideline_ro_targets_all =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadReplica"
        }
    ],
    "name": "<replace_me>",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary",
                        "ReadReplica"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.1"
}

const expected_cluster_guideline_ro_targets_all_routes_destinations = ["round-robin(Primary)", "round-robin(Secondary, ReadReplica), round-robin(Primary)"];

const default_clusterset_guideline =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY AND ($.server.clusterRole = PRIMARY OR $.server.clusterRole = UNDEFINED)",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY AND ($.server.clusterRole = PRIMARY OR $.server.clusterRole = UNDEFINED)",
            "name": "PrimaryClusterSecondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA AND ($.server.clusterRole = PRIMARY OR $.server.clusterRole = UNDEFINED)",
            "name": "PrimaryClusterReadReplica"
        }
    ],
    "name": "<replace_me>",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "PrimaryClusterSecondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.1"
}

const expected_default_clusterset_guideline_destinations = ["Primary", "PrimaryClusterSecondary", "PrimaryClusterReadReplica"];
const expected_default_clusterset_guideline_destinations_match = ["$.server.memberRole = PRIMARY AND ($.server.clusterRole = PRIMARY OR $.server.clusterRole = UNDEFINED)","$.server.memberRole = SECONDARY AND ($.server.clusterRole = PRIMARY OR $.server.clusterRole = UNDEFINED)", "$.server.memberRole = READ_REPLICA AND ($.server.clusterRole = PRIMARY OR $.server.clusterRole = UNDEFINED)"];

const expected_default_clusterset_guideline_routes = ["rw", "ro"];
const expected_default_clusterset_guideline_routes_match = ["$.session.targetPort = $.router.port.rw", "$.session.targetPort = $.router.port.ro"];
const expected_default_clusterset_guideline_routes_destinations = ["round-robin(Primary)", "round-robin(PrimaryClusterSecondary), round-robin(Primary)"];

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
  "SupportedRoutingGuidelinesVersion": "1.1"
};

function group_fields(arr, field) {
    l = [];
    for (r in arr) {
        l.push(arr[r][field]);
    }
    return l;
}

function COMMON_RG_TESTS(rg, rg_name, expected_destinations, expected_destinations_match, expected_routes, expected_routes_match, expected_routes_destinations, expected_json, expected_show_output) {
    WIPE_OUTPUT();

    // rg.name()
    var name;
    EXPECT_NO_THROWS(function() { name = rg.getName(); });

    EXPECT_EQ(rg_name, name, ".name");
    EXPECT_EQ(rg_name, rg.name, ".rg.name");

    // rg.destinations()
    var default_destinations;
    EXPECT_NO_THROWS(function() { default_destinations = rg.destinations().fetchAll(); });

    EXPECT_JSON_EQ(expected_destinations, group_fields(default_destinations, 0), ".expected_destinations");
    EXPECT_JSON_EQ(expected_destinations_match, group_fields(   default_destinations, 1), ".expected_destinations_match");

    // rg.routes()
    var default_routes;
    EXPECT_NO_THROWS(function() { default_routes = rg.routes().fetchAll() });

    EXPECT_JSON_EQ(expected_routes, group_fields(default_routes, 0), ".expected_routes");
    EXPECT_JSON_EQ(expected_routes_match, group_fields(default_routes, 3), ".expected_routes_match");
    EXPECT_JSON_EQ(expected_routes_destinations, group_fields(default_routes, 4, ".expected_routes_destinations"));

    // rg.asJson()
    var json_defaults;
    EXPECT_NO_THROWS(function() { json_defaults = rg.asJson(); });

    // Inline replace the "name" field in expected_json with rg_name
    expected_json = JSON.parse(JSON.stringify(expected_json));
    expected_json.name = rg_name;

    EXPECT_JSON_EQ(expected_json, json_defaults, ".expected_json");

    // rg.show()
    EXPECT_NO_THROWS(function() { rg.show(); });

    EXPECT_OUTPUT_CONTAINS_MULTILINE(expected_show_output);
}
