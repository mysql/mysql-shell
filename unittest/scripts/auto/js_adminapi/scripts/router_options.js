//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var cluster = scene.cluster

//@<> routerOptions() without any Router bootstrap
cluster.routerOptions();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {},
    "routers": {}
}`);

//@<> create routers
var cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

var router1 = "routerhost1::system";

session.runSql("UPDATE `mysql_innodb_cluster_metadata`.`clusters` SET router_options = '{\"tags\": {}, \"Configuration\": {\"8.4.0\": {\"Defaults\": {\"io\": {\"threads\": 0}, \"common\": {\"name\": \"system\", \"user\": \"\", \"read_timeout\": 30, \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"server_ssl_mode\": \"PREFERRED\", \"server_ssl_verify\": \"DISABLED\", \"max_total_connections\": 512, \"unknown_config_option\": \"error\", \"router_require_enforce\": true, \"max_idle_server_connections\": 64}, \"loggers\": {\"filelog\": {\"level\": \"info\", \"filename\": \"mysqlrouter.log\", \"destination\": \"\", \"timestamp_precision\": \"second\"}}, \"endpoints\": {\"bootstrap_ro\": {\"protocol\": \"classic\", \"bind_port\": 6447, \"access_mode\": \"\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=SECONDARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0, \"server_ssl_mode\": \"PREFERRED\", \"routing_strategy\": \"round-robin-with-fallback\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": false, \"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9, \"connection_sharing_delay\": 1.0}, \"bootstrap_rw\": {\"protocol\": \"classic\", \"bind_port\": 6446, \"access_mode\": \"\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=PRIMARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0, \"server_ssl_mode\": \"PREFERRED\", \"routing_strategy\": \"first-available\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": false, \"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9, \"connection_sharing_delay\": 1.0}, \"bootstrap_x_ro\": {\"protocol\": \"x\", \"bind_port\": 6449, \"access_mode\": \"\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=SECONDARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0, \"server_ssl_mode\": \"PREFERRED\", \"routing_strategy\": \"round-robin-with-fallback\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": false, \"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9, \"connection_sharing_delay\": 1.0}, \"bootstrap_x_rw\": {\"protocol\": \"x\", \"bind_port\": 6448, \"access_mode\": \"\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=PRIMARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0, \"server_ssl_mode\": \"PREFERRED\", \"routing_strategy\": \"first-available\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": false, \"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9, \"connection_sharing_delay\": 1.0}, \"bootstrap_rw_split\": {\"protocol\": \"classic\", \"bind_port\": 6450, \"access_mode\": \"auto\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=PRIMARY_AND_SECONDARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0,\"server_ssl_mode\": \"PREFERRED\", \"routing_strategy\": \"round-robin\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": true, \"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9,\"connection_sharing_delay\": 1.0}}, \"http_server\": {\"ssl\": true, \"port\": 8443, \"ssl_key\": \"\", \"ssl_cert\": \"\", \"ssl_cipher\": \"ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305\", \"ssl_curves\": \"\", \"bind_address\": \"0.0.0.0\", \"require_realm\": \"\", \"ssl_dh_params\": \"\", \"static_folder\":\"\"}, \"rest_configs\": {\"rest_api\": {\"require_realm\": \"\"}, \"rest_router\": {\"require_realm\": \"default_auth_realm\"}, \"rest_routing\": {\"require_realm\": \"default_auth_realm\"}, \"rest_metadata_cache\": {\"require_realm\": \"default_auth_realm\"}}, \"routing_rules\": {\"read_only_targets\": \"secondaries\", \"stats_updates_frequency\": -1, \"use_replica_primary_as_rw\": false, \"invalidated_cluster_policy\": \"drop_all\", \"unreachable_quorum_allowed_traffic\": \"none\"}, \"metadata_cache\": {\"ttl\": 0.5, \"read_timeout\": 30, \"auth_cache_ttl\": -1.0, \"connect_timeout\": 5, \"use_gr_notifications\": false, \"auth_cache_refresh_interval\": 2.0}, \"connection_pool\": {\"idle_timeout\": 5, \"max_idle_server_connections\": 64}, \"destination_status\": {\"error_quarantine_interval\": 1, \"error_quarantine_threshold\": 1}, \"http_authentication_realm\": {\"name\": \"default_realm\", \"method\": \"basic\", \"backend\": \"default_auth_backend\", \"require\": \"valid-user\"}, \"http_authentication_backends\": {\"default_auth_backend\": {\"backend\": \"metadata_cache\", \"filename\": \"\"}}}, \"ConfigurationChangesSchema\": {\"type\": \"object\", \"title\": \"MySQL Router configuration JSON schema\", \"$schema\": \"http://json-schema.org/draft-04/schema#\", \"properties\": {\"routing_rules\": {\"type\": \"object\", \"properties\": {\"target_cluster\": {\"type\": \"string\"},\"read_only_targets\": {\"enum\": [\"all\", \"read_replicas\", \"secondaries\"], \"type\": \"string\"}, \"stats_updates_frequency\": {\"type\": \"number\"}, \"use_replica_primary_as_rw\": {\"type\": \"boolean\"}, \"invalidated_cluster_policy\": {\"enum\": [\"accept_ro\", \"drop_all\"], \"type\": \"string\"}, \"unreachable_quorum_allowed_traffic\": {\"enum\": [\"none\", \"read\", \"all\"], \"type\": \"string\"}}, \"additionalProperties\": false}}, \"description\":\"JSON Schema for the Router configuration options that can be changed in the runtime. Shared by the Router in the metadata to announce which options it supports changing.\", \"additionalProperties\": false}}}, \"read_only_targets\":\"secondaries\"}' WHERE cluster_id=?", [cluster_id])

session.runSql("INSERT INTO `mysql_innodb_cluster_metadata`.`routers` VALUES (1,'system','MySQL Router','routerhost1','8.4.0',NULL,'{\"ROEndpoint\": \"6447\", \"RWEndpoint\": \"6446\", \"ROXEndpoint\": \"6449\", \"RWXEndpoint\": \"6448\", \"MetadataUser\": \"mysql_router1_mc1zc8dzj7yn\", \"Configuration\": {\"io\": {\"backend\": \"linux_epoll\", \"threads\": 0}, \"common\": {\"name\": \"system\", \"user\": \"\", \"read_timeout\": 30, \"client_ssl_key\": \"/home/miguel/work/testbase/router_test/data/router-key.pem\", \"client_ssl_cert\": \"/home/miguel/work/testbase/router_test/data/router-cert.pem\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"server_ssl_mode\": \"PREFERRED\", \"server_ssl_verify\": \"DISABLED\", \"max_total_connections\": 512, \"unknown_config_option\": \"error\", \"router_require_enforce\": true, \"max_idle_server_connections\": 64}, \"loggers\": {\"filelog\": {\"level\": \"info\", \"filename\": \"mysqlrouter.log\", \"destination\": \"\", \"timestamp_precision\": \"second\"}}, \"endpoints\": {\"bootstrap_ro\": {\"protocol\": \"classic\", \"bind_port\": 6447, \"access_mode\": \"\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=SECONDARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"/home/miguel/work/testbase/router_test/data/router-key.pem\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"/home/miguel/work/testbase/router_test/data/router-cert.pem\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0, \"server_ssl_mode\": \"PREFERRED\", \"routing_strategy\": \"round-robin-with-fallback\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": false,\"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9, \"connection_sharing_delay\": 1.0}, \"bootstrap_rw\": {\"protocol\": \"classic\", \"bind_port\": 6446, \"access_mode\": \"\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=PRIMARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"/home/miguel/work/testbase/router_test/data/router-key.pem\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"/home/miguel/work/testbase/router_test/data/router-cert.pem\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0, \"server_ssl_mode\": \"PREFERRED\",\"routing_strategy\": \"first-available\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": false, \"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9, \"connection_sharing_delay\": 1.0}, \"bootstrap_x_ro\": {\"protocol\": \"x\", \"bind_port\": 6449, \"access_mode\": \"\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=SECONDARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"/home/miguel/work/testbase/router_test/data/router-key.pem\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"/home/miguel/work/testbase/router_test/data/router-cert.pem\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0, \"server_ssl_mode\": \"PREFERRED\", \"routing_strategy\": \"round-robin-with-fallback\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": false, \"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9, \"connection_sharing_delay\": 1.0}, \"bootstrap_x_rw\": {\"protocol\": \"x\", \"bind_port\": 6448, \"access_mode\": \"\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=PRIMARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"/home/miguel/work/testbase/router_test/data/router-key.pem\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"/home/miguel/work/testbase/router_test/data/router-cert.pem\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0, \"server_ssl_mode\": \"PREFERRED\", \"routing_strategy\": \"first-available\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": false, \"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9, \"connection_sharing_delay\": 1.0}, \"bootstrap_rw_split\": {\"protocol\": \"classic\", \"bind_port\": 6450, \"access_mode\": \"auto\", \"bind_address\": \"0.0.0.0\", \"destinations\": \"metadata-cache://tst/?role=PRIMARY_AND_SECONDARY\", \"named_socket\": \"\", \"server_ssl_ca\": \"\", \"client_ssl_key\": \"/home/miguel/work/testbase/router_test/data/router-key.pem\", \"server_ssl_crl\": \"\", \"client_ssl_cert\": \"/home/miguel/work/testbase/router_test/data/router-cert.pem\", \"client_ssl_mode\": \"PREFERRED\", \"connect_timeout\": 5, \"max_connections\": 0,\"server_ssl_mode\": \"PREFERRED\", \"routing_strategy\": \"round-robin\", \"client_ssl_cipher\": \"\", \"client_ssl_curves\": \"\", \"net_buffer_length\": 16384, \"server_ssl_capath\": \"\", \"server_ssl_cipher\": \"\", \"server_ssl_curves\": \"\", \"server_ssl_verify\": \"DISABLED\", \"thread_stack_size\": 1024, \"connection_sharing\": true, \"max_connect_errors\": 100, \"server_ssl_crlpath\": \"\", \"client_ssl_dh_params\": \"\", \"client_connect_timeout\": 9,\"connection_sharing_delay\": 1.0}}, \"http_server\": {\"ssl\": true, \"port\": 8443, \"ssl_key\": \"/home/miguel/work/testbase/router_test/data/router-key.pem\", \"ssl_cert\": \"/home/miguel/work/testbase/router_test/data/router-cert.pem\", \"ssl_cipher\": \"ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305\", \"ssl_curves\": \"\", \"bind_address\": \"0.0.0.0\", \"require_realm\": \"\", \"ssl_dh_params\": \"\", \"static_folder\": \"\"}, \"rest_configs\": {\"rest_api\": {\"require_realm\": \"\"}, \"rest_router\": {\"require_realm\": \"default_auth_realm\"}, \"rest_routing\": {\"require_realm\": \"default_auth_realm\"}, \"rest_metadata_cache\": {\"require_realm\": \"default_auth_realm\"}}, \"routing_rules\": {\"read_only_targets\": \"secondaries\", \"stats_updates_frequency\": -1, \"use_replica_primary_as_rw\": false, \"invalidated_cluster_policy\": \"drop_all\", \"unreachable_quorum_allowed_traffic\": \"none\"}, \"metadata_cache\": {\"ttl\": 0.5, \"user\": \"mysql_router1_mc1zc8dzj7yn\", \"read_timeout\": 30, \"auth_cache_ttl\": -1.0, \"connect_timeout\": 5, \"use_gr_notifications\": false, \"auth_cache_refresh_interval\": 2.0}, \"connection_pool\": {\"idle_timeout\": 5, \"max_idle_server_connections\": 64}, \"destination_status\": {\"error_quarantine_interval\": 1, \"error_quarantine_threshold\": 1}, \"http_authentication_realm\": {\"name\": \"default_realm\", \"method\": \"basic\", \"backend\": \"default_auth_backend\", \"require\": \"valid-user\"}, \"http_authentication_backends\": {\"default_auth_backend\": {\"backend\": \"metadata_cache\", \"filename\": \"\"}}}, \"RWSplitEndpoint\": \"6450\", \"bootstrapTargetType\": \"cluster\"}', ?, NULL, NULL)", [cluster_id]);

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
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "max_idle_server_connections": 64,
            "max_total_connections": 512,
            "name": "system",
            "read_timeout": 30,
            "router_require_enforce": true,
            "server_ssl_mode": "PREFERRED",
            "server_ssl_verify": "DISABLED",
            "unknown_config_option": "error",
            "user": ""
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
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6447,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_rw": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6446,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_address": "0.0.0.0",
                "bind_port": 6450,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": true,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY_AND_SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_x_ro": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6449,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_x_rw": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6448,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
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
            "connect_timeout": 5,
            "read_timeout": 30,
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
                    "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                    "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem"
                },
                "http_server": {
                    "ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                    "ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem"
                },
                "io": {
                    "backend": "linux_epoll"
                },
                "metadata_cache": {
                    "user": "mysql_router1_mc1zc8dzj7yn"
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
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "max_idle_server_connections": 64,
            "max_total_connections": 512,
            "name": "system",
            "read_timeout": 30,
            "router_require_enforce": true,
            "server_ssl_mode": "PREFERRED",
            "server_ssl_verify": "DISABLED",
            "unknown_config_option": "error",
            "user": ""
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
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6447,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_rw": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6446,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_address": "0.0.0.0",
                "bind_port": 6450,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": true,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY_AND_SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_x_ro": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6449,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_x_rw": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6448,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
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
            "connect_timeout": 5,
            "read_timeout": 30,
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
                    "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                    "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                    "client_ssl_mode": "PREFERRED",
                    "connect_timeout": 5,
                    "max_idle_server_connections": 64,
                    "max_total_connections": 512,
                    "name": "system",
                    "read_timeout": 30,
                    "router_require_enforce": true,
                    "server_ssl_mode": "PREFERRED",
                    "server_ssl_verify": "DISABLED",
                    "unknown_config_option": "error",
                    "user": ""
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
                        "access_mode": "",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6447,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
                    },
                    "bootstrap_rw": {
                        "access_mode": "",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6446,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
                    },
                    "bootstrap_rw_split": {
                        "access_mode": "auto",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6450,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": true,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=PRIMARY_AND_SECONDARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
                    },
                    "bootstrap_x_ro": {
                        "access_mode": "",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6449,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
                    },
                    "bootstrap_x_rw": {
                        "access_mode": "",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6448,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
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
                    "ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                    "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
                    "ssl_curves": "",
                    "ssl_dh_params": "",
                    "ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
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
                    "connect_timeout": 5,
                    "read_timeout": 30,
                    "ttl": 0.5,
                    "use_gr_notifications": false,
                    "user": "mysql_router1_mc1zc8dzj7yn"
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
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "max_idle_server_connections": 64,
            "max_total_connections": 512,
            "name": "system",
            "read_timeout": 30,
            "router_require_enforce": true,
            "server_ssl_mode": "PREFERRED",
            "server_ssl_verify": "DISABLED",
            "unknown_config_option": "error",
            "user": ""
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
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6447,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_rw": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6446,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_address": "0.0.0.0",
                "bind_port": 6450,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": true,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY_AND_SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_x_ro": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6449,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_x_rw": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6448,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
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
            "connect_timeout": 5,
            "read_timeout": 30,
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
                    "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                    "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem"
                },
                "http_server": {
                    "ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                    "ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem"
                },
                "io": {
                    "backend": "linux_epoll"
                },
                "metadata_cache": {
                    "user": "mysql_router1_mc1zc8dzj7yn"
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
            "client_ssl_mode": "PREFERRED",
            "connect_timeout": 5,
            "max_idle_server_connections": 64,
            "max_total_connections": 512,
            "name": "system",
            "read_timeout": 30,
            "router_require_enforce": true,
            "server_ssl_mode": "PREFERRED",
            "server_ssl_verify": "DISABLED",
            "unknown_config_option": "error",
            "user": ""
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
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6447,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_rw": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6446,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_rw_split": {
                "access_mode": "auto",
                "bind_address": "0.0.0.0",
                "bind_port": 6450,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": true,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY_AND_SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_x_ro": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6449,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
            },
            "bootstrap_x_rw": {
                "access_mode": "",
                "bind_address": "0.0.0.0",
                "bind_port": 6448,
                "client_connect_timeout": 9,
                "client_ssl_cert": "",
                "client_ssl_cipher": "",
                "client_ssl_curves": "",
                "client_ssl_dh_params": "",
                "client_ssl_key": "",
                "client_ssl_mode": "PREFERRED",
                "connect_timeout": 5,
                "connection_sharing": false,
                "connection_sharing_delay": 1,
                "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                "server_ssl_mode": "PREFERRED",
                "server_ssl_verify": "DISABLED",
                "thread_stack_size": 1024
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
            "connect_timeout": 5,
            "read_timeout": 30,
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
                    "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                    "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                    "client_ssl_mode": "PREFERRED",
                    "connect_timeout": 5,
                    "max_idle_server_connections": 64,
                    "max_total_connections": 512,
                    "name": "system",
                    "read_timeout": 30,
                    "router_require_enforce": true,
                    "server_ssl_mode": "PREFERRED",
                    "server_ssl_verify": "DISABLED",
                    "unknown_config_option": "error",
                    "user": ""
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
                        "access_mode": "",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6447,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
                    },
                    "bootstrap_rw": {
                        "access_mode": "",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6446,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
                    },
                    "bootstrap_rw_split": {
                        "access_mode": "auto",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6450,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": true,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=PRIMARY_AND_SECONDARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
                    },
                    "bootstrap_x_ro": {
                        "access_mode": "",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6449,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=SECONDARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
                    },
                    "bootstrap_x_rw": {
                        "access_mode": "",
                        "bind_address": "0.0.0.0",
                        "bind_port": 6448,
                        "client_connect_timeout": 9,
                        "client_ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                        "client_ssl_cipher": "",
                        "client_ssl_curves": "",
                        "client_ssl_dh_params": "",
                        "client_ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
                        "client_ssl_mode": "PREFERRED",
                        "connect_timeout": 5,
                        "connection_sharing": false,
                        "connection_sharing_delay": 1,
                        "destinations": "metadata-cache://tst/?role=PRIMARY",
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
                        "server_ssl_mode": "PREFERRED",
                        "server_ssl_verify": "DISABLED",
                        "thread_stack_size": 1024
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
                    "ssl_cert": "/home/miguel/work/testbase/router_test/data/router-cert.pem",
                    "ssl_cipher": "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-CCM:ECDHE-ECDSA-AES128-CCM:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-CCM:DHE-RSA-AES256-CCM:DHE-RSA-CHACHA20-POLY1305",
                    "ssl_curves": "",
                    "ssl_dh_params": "",
                    "ssl_key": "/home/miguel/work/testbase/router_test/data/router-key.pem",
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
                    "connect_timeout": 5,
                    "read_timeout": 30,
                    "ttl": 0.5,
                    "use_gr_notifications": false,
                    "user": "mysql_router1_mc1zc8dzj7yn"
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
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = '{}'");

cluster.routerOptions();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "configuration": {},
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
            "hostname": "routerhost1",
            "lastCheckIn": null,
            "roPort": "6447",
            "roXPort": "6449",
            "routerErrors": [
                "WARNING: Unable to fetch all configuration options: Please ensure the Router account has the right privileges using <Cluster>.setupRouterAccount() and re-bootstrap Router."
            ],
            "rwPort": "6446",
            "rwSplitPort": "6450",
            "rwXPort": "6448",
            "version": "8.4.0"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2024-02-14 11:22:33",
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "rwPort": "mysql.sock",
            "rwSplitPort": null,
            "rwXPort": "mysqlx.sock",
            "version": "8.1.0"
        }
    }
}
`);

//@<> The same check must be done in clusterset.listRouters()
clusterset = cluster.createClusterSet("cs");
clusterset.listRouters();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "domainName": "cs",
    "routers": {
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": null,
            "roPort": "6447",
            "roXPort": "6449",
            "routerErrors": [
                "WARNING: Router must be bootstrapped again for the ClusterSet to be recognized.",
                "WARNING: Unable to fetch all configuration options: Please ensure the Router account has the right privileges using <Cluster>.setupRouterAccount() and re-bootstrap Router."
            ],
            "rwPort": "6446",
            "rwSplitPort": "6450",
            "rwXPort": "6448",
            "targetCluster": null,
            "version": "8.4.0"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2024-02-14 11:22:33",
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "routerErrors": [
                "WARNING: Router must be bootstrapped again for the ClusterSet to be recognized."
            ],
            "rwPort": "mysql.sock",
            "rwSplitPort": null,
            "rwXPort": "mysqlx.sock",
            "targetCluster": null,
            "version": "8.1.0"
        }
    }
}
`);


//@<> Cleanup
scene.destroy();
