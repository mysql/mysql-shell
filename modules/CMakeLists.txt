# Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

INCLUDE_DIRECTORIES(BEFORE
     ${CMAKE_CURRENT_SOURCE_DIR}
	 ${CMAKE_BINARY_DIR}
	 ${CMAKE_SOURCE_DIR}
     ${CMAKE_SOURCE_DIR}/mysqlshdk/libs
     ${CMAKE_SOURCE_DIR}/mysqlshdk/include
	 ${CMAKE_SOURCE_DIR}/ext/rapidjson/include
	 ${CMAKE_SOURCE_DIR}/common/uuid/include
	 ${MYSQL_INCLUDE_DIRS})

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/generate_metadata_source.cmake)


file(GLOB api_module_SOURCES
      "devapi/*.cc"
      "dynamic_*.cc"
      "util/*.cc"
      "mod_mysql.cc"
      "mod_mysql_*.cc"
      "mod_shell.cc"
      "mod_extensible_object.cc"
      "mod_shell_options.cc"
      "mod_shell_reports.cc"
      "mod_sys.cc"
      "mod_utils.cc"
      "mod_mysql_constants.cc"
      "interactive_object_wrapper.cc"
      "adminapi/mod_dba.cc"
      "adminapi/mod_dba_cluster.cc"
      "adminapi/common/metadata_storage.cc"
      "adminapi/common/metadata_management_mysql.cc"
      "adminapi/common/provisioning_interface.cc"
      "adminapi/common/common.cc"
      "adminapi/common/group_replication_options.cc"
      "adminapi/common/sql.cc"
      "adminapi/common/validations.cc"
      "adminapi/common/preconditions.cc"
      "adminapi/common/provision.cc"
      "adminapi/common/instance_validations.cc"
      "adminapi/dba/check_instance.cc"
      "adminapi/dba/configure_local_instance.cc"
      "adminapi/dba/configure_instance.cc"
      "adminapi/cluster/cluster_describe.cc"
      "adminapi/cluster/cluster_status.cc"
      "adminapi/cluster/dissolve.cc"
      "adminapi/cluster/cluster_options.cc"
      "adminapi/cluster/cluster_set_option.cc"
      "adminapi/replicaset/replicaset_describe.cc"
      "adminapi/replicaset/replicaset_status.cc"
      "adminapi/replicaset/replicaset.cc"
      "adminapi/replicaset/remove_instance.cc"
      "adminapi/replicaset/rescan.cc"
      "adminapi/replicaset/topology_configuration_command.cc"
      "adminapi/replicaset/switch_to_single_primary_mode.cc"
      "adminapi/replicaset/switch_to_multi_primary_mode.cc"
      "adminapi/replicaset/set_primary_instance.cc"
      "adminapi/replicaset/replicaset_options.cc"
      "adminapi/replicaset/set_option.cc"
      "adminapi/replicaset/set_instance_option.cc"
      "adminapi/replicaset/check_instance_state.cc")

add_convenience_library(api_modules ${api_module_SOURCES})
target_link_libraries(api_modules utils)

ADD_STAN_TARGET(api_modules ${api_module_SOURCES})
