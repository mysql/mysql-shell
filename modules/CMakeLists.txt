# Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

file(GLOB api_module_SOURCES
      "devapi/*.cc"
      "dynamic_*.cc"
      "util/common/dump/checksums.cc"
      "util/common/dump/filtering_options.cc"
      "util/common/dump/utils.cc"
      "util/copy/copy_instance_options.cc"
      "util/copy/copy_operation.cc"
      "util/copy/copy_schemas_options.cc"
      "util/copy/copy_tables_options.cc"
      "util/dump/capability.cc"
      "util/dump/common_errors.cc"
      "util/dump/compatibility.cc"
      "util/dump/compatibility_option.cc"
      "util/dump/console_with_progress.cc"
      "util/dump/ddl_dumper.cc"
      "util/dump/ddl_dumper_options.cc"
      "util/dump/decimal.cc"
      "util/dump/dialect_dump_writer.cc"
      "util/dump/dump_instance_options.cc"
      "util/dump/dump_manifest.cc"
      "util/dump/dump_manifest_config.cc"
      "util/dump/dump_manifest_options.cc"
      "util/dump/dump_options.cc"
      "util/dump/dump_schemas_options.cc"
      "util/dump/dump_tables_options.cc"
      "util/dump/dump_writer.cc"
      "util/dump/dumper.cc"
      "util/dump/export_table.cc"
      "util/dump/export_table_options.cc"
      "util/dump/indexes.cc"
      "util/dump/instance_cache.cc"
      "util/dump/progress_thread.cc"
      "util/dump/schema_dumper.cc"
      "util/dump/text_dump_writer.cc"
      "util/load/load_dump_options.cc"
      "util/load/dump_loader.cc"
      "util/load/dump_reader.cc"
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
      "util/upgrade_checker/upgrade_check_registry.cc"
      "util/upgrade_checker/upgrade_check_creators.cc"
      "util/upgrade_checker/upgrade_check_condition.cc"
)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET_SOURCE_FILES_PROPERTIES(
        "mod_shell.cc"
        PROPERTIES COMPILE_FLAGS "-fno-var-tracking-assignments"
    )
ENDIF()

add_convenience_library(api_modules ${api_module_SOURCES})
target_link_libraries(api_modules utils adminapi_modules)

ADD_SUBDIRECTORY(adminapi)

ADD_STAN_TARGET(api_modules ${api_module_SOURCES})
