# Copyright (c) 2014, 2025, Oracle and/or its affiliates.
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

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")

INCLUDE_DIRECTORIES(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/mysqlshdk/libs
    ${CMAKE_SOURCE_DIR}/mysqlshdk/include
    ${CMAKE_SOURCE_DIR}/common/uuid/include
    ${MYSQL_INCLUDE_DIRS})

file(GLOB api_module_SOURCES
      "devapi/*.cc"
      "dynamic_*.cc"
      "util/binlog/binlog_dump_info.cc"
      "util/binlog/binlog_dumper.cc"
      "util/binlog/binlog_loader.cc"
      "util/binlog/dump_binlogs_options.cc"
      "util/binlog/load_binlogs_options.cc"
      "util/binlog/utils.cc"
      "util/common/common_options.cc"
      "util/common/storage_options.cc"
      "util/common/utils.cc"
      "util/common/dump/basenames.cc"
      "util/common/dump/checksums.cc"
      "util/common/dump/dump_info.cc"
      "util/common/dump/dump_version.cc"
      "util/common/dump/filtering_options.cc"
      "util/common/dump/resource_principals_info.cc"
      "util/common/dump/server_info.cc"
      "util/common/dump/utils.cc"
      "util/common/dump/vector_store_info.cc"
      "util/copy/copy_instance_options.cc"
      "util/copy/copy_operation.cc"
      "util/copy/copy_schemas_options.cc"
      "util/copy/copy_tables_options.cc"
      "util/dump/capability.cc"
      "util/dump/common_errors.cc"
      "util/dump/compatibility.cc"
      "util/dump/compatibility_issue.cc"
      "util/dump/compatibility_option.cc"
      "util/dump/console_with_progress.cc"
      "util/dump/ddl_dumper.cc"
      "util/dump/ddl_dumper_options.cc"
      "util/dump/decimal.cc"
      "util/dump/dump_instance_options.cc"
      "util/dump/dump_options.cc"
      "util/dump/dump_schemas_options.cc"
      "util/dump/dump_tables_options.cc"
      "util/dump/dump_writer.cc"
      "util/dump/dumper.cc"
      "util/dump/export_table.cc"
      "util/dump/export_table_options.cc"
      "util/dump/indexes.cc"
      "util/dump/instance_cache.cc"
      "util/dump/lakehouse_target_option.cc"
      "util/dump/progress_thread.cc"
      "util/dump/schema_dumper.cc"
      "util/dump/text_dump_writer.cc"
      "util/load/convert_vector_store.cc"
      "util/load/dump_loader.cc"
      "util/load/dump_reader.cc"
      "util/load/heatwave_load.cc"
      "util/load/lakehouse_source_option.cc"
      "util/load/load_dump_options.cc"
      "util/import_table/chunk_file.cc"
      "util/import_table/load_data.cc"
      "util/import_table/dialect.cc"
      "util/import_table/import_table_options.cc"
      "util/import_table/import_table.cc"
      "util/import_table/scanner.cc"
      "util/json_importer.cc"
      "util/mod_util.cc"
      "util/upgrade_check.cc"
      "mod_mysql.cc"
      "mod_mysql_*.cc"
      "mod_mysqlsh.cc"
      "mod_shell.cc"
      "mod_extensible_object.cc"
      "mod_os.cc"
      "mod_path.cc"
      "mod_shell.cc"
      "mod_shell_context.cc"
      "mod_shell_options.cc"
      "mod_shell_reports.cc"
      "mod_shell_result.cc"
      "mod_sys.cc"
      "mod_utils.cc"
      "mod_mysql_constants.cc"
      "reports/native_report.cc"
      "reports/query.cc"
      "reports/thread.cc"
      "reports/threads.cc"
      "reports/utils.cc"
      "util/upgrade_checker/common.cc"
      "util/upgrade_checker/custom_check.cc"
      "util/upgrade_checker/sql_upgrade_check.cc"
      "util/upgrade_checker/upgrade_check_config.cc"
      "util/upgrade_checker/upgrade_check_formatter.cc"
      "util/upgrade_checker/upgrade_check_options.cc"
      "util/upgrade_checker/upgrade_check.cc"
      "util/upgrade_checker/feature_life_cycle_check.cc"
      "util/upgrade_checker/upgrade_check_registry.cc"
      "util/upgrade_checker/upgrade_check_creators.cc"
      "util/upgrade_checker/upgrade_check_condition.cc"
      "util/upgrade_checker/sysvar_check.cc"
)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET_SOURCE_FILES_PROPERTIES(
        "mod_shell.cc"
        PROPERTIES COMPILE_FLAGS "-fno-var-tracking-assignments"
    )
ENDIF()

static_mysql_lib(NAME "mysql_binlog_event_standalone" PATH MYSQL_BINLOG_EVENT_LIB)

if(NOT BUILD_SOURCE_PACKAGE AND NOT EXISTS "${MYSQL_BINLOG_EVENT_LIB}")
    message(FATAL_ERROR "The binlog event library was not found, please compile it using: cmake --build '${MYSQL_BUILD_DIR}' --target mysql_binlog_event_standalone")
else()
    message(STATUS "MYSQL_BINLOG_EVENT_LIB: ${MYSQL_BINLOG_EVENT_LIB}")
endif()

set(MYSQL_BINLOG_LIBS "${MYSQL_BINLOG_EVENT_LIB}")

foreach(__lib "mysql_serialization")
    static_mysql_lib(NAME "${__lib}" PATH __path)
    list(APPEND MYSQL_BINLOG_LIBS "${__path}")
endforeach()

add_convenience_library(api_modules ${api_module_SOURCES} INCLUDE_DIRECTORIES "${MYSQL_SOURCE_DIR}/libs" COMPILE_DEFINITIONS STANDALONE_BINLOG)
target_link_libraries(api_modules utils adminapi_modules ${MYSQL_BINLOG_LIBS})

ADD_SUBDIRECTORY(adminapi)

ADD_STAN_TARGET(api_modules ${api_module_SOURCES})

# If defined, installs the mysqlbinlog binary into the shell bundle
IF(BUNDLED_MYSQLBINLOG)
  if(APPLE)
    # add rpath to allow executable to import bundled dylibs
    get_filename_component(_file_name "${BUNDLED_MYSQLBINLOG}" NAME)
    file(RELATIVE_PATH __relative_path "${CMAKE_BINARY_DIR}/${INSTALL_LIBEXECDIR}" "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}")
    set(EXTRA_COPY_COMMANDS COMMAND install_name_tool -add_rpath "@executable_path/${__relative_path}" "${CMAKE_BINARY_DIR}/${INSTALL_LIBEXECDIR}/${_file_name}")
  endif()

  install_bundled_binaries(
      DESCRIPTION "mysqlbinlog binary"
      BINARIES "${BUNDLED_MYSQLBINLOG}"
      DESTINATION "${INSTALL_LIBEXECDIR}"
      TARGET api_modules
      EXTRA_COPY_COMMANDS ${EXTRA_COPY_COMMANDS}
  )
ENDIF()
