# Copyright (c) 2014, 2020, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")

INCLUDE_DIRECTORIES(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/mysqlshdk/libs
    ${CMAKE_SOURCE_DIR}/mysqlshdk/include
    ${CMAKE_SOURCE_DIR}/common/uuid/include
    ${MYSQL_INCLUDE_DIRS})

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/generate_metadata_source.cmake)

file(GLOB api_module_SOURCES
      "devapi/*.cc"
      "dynamic_*.cc"
      "util/dump/compatibility.cc"
      "util/dump/compatibility_option.cc"
      "util/dump/console_with_progress.cc"
      "util/dump/ddl_dumper.cc"
      "util/dump/ddl_dumper_options.cc"
      "util/dump/default_dump_writer.cc"
      "util/dump/dump_instance.cc"
      "util/dump/dump_instance_options.cc"
      "util/dump/dump_options.cc"
      "util/dump/dump_schemas.cc"
      "util/dump/dump_schemas_options.cc"
      "util/dump/dump_utils.cc"
      "util/dump/dump_writer.cc"
      "util/dump/dumper.cc"
      "util/dump/schema_dumper.cc"
      "util/dump/text_dump_writer.cc"
      "util/import_table/chunk_file.cc"
      "util/import_table/load_data.cc"
      "util/import_table/dialect.cc"
      "util/import_table/import_table_options.cc"
      "util/import_table/import_table.cc"
      "util/json_importer.cc"
      "util/mod_util.cc"
      "util/upgrade_check.cc"
      "mod_mysql.cc"
      "mod_mysql_*.cc"
      "mod_shell.cc"
      "mod_extensible_object.cc"
      "mod_os.cc"
      "mod_path.cc"
      "mod_shell_options.cc"
      "mod_shell_reports.cc"
      "mod_sys.cc"
      "mod_utils.cc"
      "mod_mysql_constants.cc"
      "adminapi/dba_utils.cc"
      "adminapi/mod_dba.cc"
      "adminapi/mod_dba_replica_set.cc"
      "adminapi/mod_dba_cluster.cc"
      "adminapi/common/metadata_storage.cc"
      "adminapi/common/metadata_management_mysql.cc"
      "adminapi/common/metadata_backup_handler.cc"
      "adminapi/common/provisioning_interface.cc"
      "adminapi/common/common.cc"
      "adminapi/common/accounts.cc"
      "adminapi/common/health_enums.cc"
      "adminapi/common/group_replication_options.cc"
      "adminapi/common/clone_options.cc"
      "adminapi/common/clone_progress.cc"
      "adminapi/common/cluster_types.cc"
      "adminapi/common/sql.cc"
      "adminapi/common/validations.cc"
      "adminapi/common/preconditions.cc"
      "adminapi/common/provision.cc"
      "adminapi/common/instance_validations.cc"
      "adminapi/common/instance_pool.cc"
      "adminapi/common/async_utils.cc"
      "adminapi/common/global_topology.cc"
      "adminapi/common/async_topology.cc"
      "adminapi/common/global_topology_check.cc"
      "adminapi/common/gtid_validations.cc"
      "adminapi/common/star_global_topology_manager.cc"
      "adminapi/common/instance_monitoring.cc"
      "adminapi/common/instance_pool.cc"
      "adminapi/common/member_recovery_monitoring.cc"
      "adminapi/common/router.cc"
      "adminapi/common/setup_account.cc"
      "adminapi/dba/check_instance.cc"
      "adminapi/dba/configure_async_instance.cc"
      "adminapi/dba/configure_local_instance.cc"
      "adminapi/dba/configure_instance.cc"
      "adminapi/dba/create_cluster.cc"
      "adminapi/dba/restore_cluster.cc"
      "adminapi/dba/upgrade_metadata.cc"
      "adminapi/replica_set/replica_set_impl.cc"
      "adminapi/replica_set/replica_set_status.cc"
      "adminapi/cluster/base_cluster_impl.cc"
      "adminapi/cluster/cluster_impl.cc"
      "adminapi/cluster/cluster_describe.cc"
      "adminapi/cluster/cluster_status.cc"
      "adminapi/cluster/dissolve.cc"
      "adminapi/cluster/reset_recovery_accounts_password.cc"
      "adminapi/cluster/cluster_options.cc"
      "adminapi/cluster/cluster_set_option.cc"
      "adminapi/cluster/member_recovery.cc"
      "adminapi/cluster/replicaset/add_instance.cc"
      "adminapi/cluster/replicaset/replicaset_describe.cc"
      "adminapi/cluster/replicaset/replicaset_status.cc"
      "adminapi/cluster/replicaset/replicaset.cc"
      "adminapi/cluster/replicaset/remove_instance.cc"
      "adminapi/cluster/replicaset/rescan.cc"
      "adminapi/cluster/replicaset/topology_configuration_command.cc"
      "adminapi/cluster/replicaset/switch_to_single_primary_mode.cc"
      "adminapi/cluster/replicaset/switch_to_multi_primary_mode.cc"
      "adminapi/cluster/replicaset/set_primary_instance.cc"
      "adminapi/cluster/replicaset/replicaset_options.cc"
      "adminapi/cluster/replicaset/set_option.cc"
      "adminapi/cluster/replicaset/set_instance_option.cc"
      "adminapi/cluster/replicaset/check_instance_state.cc"
      "reports/native_report.cc"
      "reports/query.cc"
      "reports/thread.cc"
      "reports/threads.cc"
      "reports/utils.cc"
)

add_convenience_library(api_modules ${api_module_SOURCES})
target_link_libraries(api_modules utils)

ADD_STAN_TARGET(api_modules ${api_module_SOURCES})
