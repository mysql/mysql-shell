# Copyright (c) 2014, 2024, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

file(GLOB adminapi_module_SOURCES
      "dba_utils.cc"
      "base_cluster.cc"
      "mod_dba.cc"
      "mod_dba_replica_set.cc"
      "mod_dba_cluster.cc"
      "mod_dba_cluster_set.cc"
      "common/api_options.cc"
      "common/connectivity_check.cc"
      "common/metadata_storage.cc"
      "common/metadata_management_mysql.cc"
      "common/metadata_backup_handler.cc"
      "common/provisioning_interface.cc"
      "common/common.cc"
      "common/common_status.cc"
      "common/accounts.cc"
      "common/health_enums.cc"
      "common/group_replication_options.cc"
      "common/clone_options.cc"
      "common/parallel_applier_options.cc"
      "common/clone_progress.cc"
      "common/cluster_types.cc"
      "common/sql.cc"
      "common/validations.cc"
      "common/preconditions.cc"
      "common/provision.cc"
      "common/instance_validations.cc"
      "common/instance_pool.cc"
      "common/async_utils.cc"
      "common/global_topology.cc"
      "common/async_topology.cc"
      "common/global_topology_check.cc"
      "common/gtid_validations.cc"
      "common/star_global_topology_manager.cc"
      "common/instance_monitoring.cc"
      "common/instance_pool.cc"
      "common/member_recovery_monitoring.cc"
      "common/router.cc"
      "common/server_features.cc"
      "common/setup_account.cc"
      "common/base_cluster_impl.cc"
      "common/undo.cc"
      "dba/check_instance.cc"
      "dba/configure_async_instance.cc"
      "dba/configure_instance.cc"
      "dba/create_cluster.cc"
      "dba/restore_cluster.cc"
      "dba/reboot_cluster_from_complete_outage.cc"
      "dba/upgrade_metadata.cc"
      "dba/api_options.cc"
      "cluster_set/api_options.cc"
      "cluster_set/cluster_set_impl.cc"
      "cluster_set/create_replica_cluster.cc"
      "cluster_set/status.cc"
      "replica_set/api_options.cc"
      "replica_set/describe.cc"
      "replica_set/dissolve.cc"
      "replica_set/replica_set_impl.cc"
      "replica_set/rescan.cc"
      "replica_set/status.cc"
      "cluster/add_instance.cc"
      "cluster/add_replica_instance.cc"
      "cluster/rejoin_instance.cc"
      "cluster/rejoin_replica_instance.cc"
      "cluster/cluster_join.cc"
      "cluster/create_cluster_set.cc"
      "cluster/remove_instance.cc"
      "cluster/remove_replica_instance.cc"
      "cluster/cluster_impl.cc"
      "cluster/describe.cc"
      "cluster/status.cc"
      "cluster/dissolve.cc"
      "cluster/reset_recovery_accounts_password.cc"
      "cluster/options.cc"
      "cluster/set_option.cc"
      "cluster/member_recovery.cc"
      "cluster/topology_configuration_command.cc"
      "cluster/switch_to_single_primary_mode.cc"
      "cluster/switch_to_multi_primary_mode.cc"
      "cluster/set_primary_instance.cc"
      "cluster/set_option.cc"
      "cluster/rescan.cc"
      "cluster/set_instance_option.cc"
      "cluster/api_options.cc"
)

# install metadata scripts
FILE(GLOB METADATA_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/common/metadata/*.sql)

add_convenience_library(adminapi_modules ${adminapi_module_SOURCES})
target_link_libraries(adminapi_modules utils mysql)

ADD_CUSTOM_COMMAND(OUTPUT metadata_scripts
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CONFIG_BINARY_DIR}/${INSTALL_SHAREDIR}/adminapi-metadata
    DEPENDS ${METADATA_SCRIPTS}
    COMMAND ${CMAKE_COMMAND} -E copy ${METADATA_SCRIPTS} ${CONFIG_BINARY_DIR}/${INSTALL_SHAREDIR}/adminapi-metadata/
)

ADD_CUSTOM_TARGET(module_data ALL DEPENDS metadata_scripts)
INSTALL(DIRECTORY ${CONFIG_BINARY_DIR}/${INSTALL_SHAREDIR}/adminapi-metadata COMPONENT main DESTINATION ${INSTALL_SHAREDIR})
