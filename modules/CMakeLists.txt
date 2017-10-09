# Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

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
      "mod_mysql.cc"
      "mod_mysql_*.cc"
      "mod_shell.cc"
      "mod_sys.cc"
      "mod_utils.cc"
      "mysql_connection.cc"
      "interactive_object_wrapper.cc"
      "adminapi/password_hasher.cc"
      "adminapi/mod_dba.cc"
      "adminapi/mod_dba_cluster.cc"
      "adminapi/mod_dba_metadata_storage.cc"
      "adminapi/mod_dba_provisioning_interface.cc"
      "adminapi/mod_dba_replicaset.cc"
      "adminapi/mod_dba_common.cc"
      "adminapi/mod_dba_sql.cc")

add_convenience_library(api_modules ${api_module_SOURCES})
target_link_libraries(api_modules utils)

ADD_STAN_TARGET(api_modules ${api_module_SOURCES})
