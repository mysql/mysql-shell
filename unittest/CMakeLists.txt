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

if(WITH_TESTS)
    IF(WITH_GTEST)
        ## Backwards compatibility with old scripts, remove once all
        ## builds are updated
        INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/gtest_legacy.cmake)
        SET(GMOCK_INCLUDE_DIRS ${GTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR})
        SET(GTEST_LIBRARIES gtest gmock)
    ELSE()
        INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/gtest.cmake)
    ENDIF()

    ADD_DEFINITIONS(-DWITH_GTEST=1)
    add_definitions(-DGTEST_LANG_CXX11=1 ${GMOCK_CFLAGS})
    add_definitions(-DMYSQLX_SOURCE_HOME="${CMAKE_SOURCE_DIR}")

    include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")

    include_directories(BEFORE
                ${CMAKE_SOURCE_DIR}/unittest
                ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/include
                ${CMAKE_SOURCE_DIR}/mysqlshdk/libs
                ${CMAKE_SOURCE_DIR}/mysqlshdk/include
                ${CMAKE_BINARY_DIR}/mysqlshdk/include
                ${CMAKE_SOURCE_DIR}/
                ${CMAKE_SOURCE_DIR}/src
                ${MYSQL_INCLUDE_DIRS}
                ${GMOCK_INCLUDE_DIRS})

    if(HAVE_V8)
      include_directories("${V8_INCLUDE_DIR}")
    endif()

    if(HAVE_PYTHON)
      include_directories("${PYTHON_INCLUDE_DIRS}")
    endif()

    add_subdirectory(mysql-secret-store-plaintext)
    add_subdirectory(sample-pager)

    file(GLOB mysqlsh_tests_SRC
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/shellcore/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/db/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/config/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/mysql/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/rest/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/storage/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/utils/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/textui/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/gr/group_replication_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/innodbcluster/*.cc"
        "${CMAKE_SOURCE_DIR}/unittest/*_t.cc"
        "${CMAKE_SOURCE_DIR}/unittest/test_utils/*.cc"
        "${CMAKE_SOURCE_DIR}/unittest/test_utils/mocks/mysqlshdk/libs/*/*.cc"
        "${CMAKE_SOURCE_DIR}/unittest/*/*_t.cc")

    set(mysqlsh_tests_SRC
        ${mysqlsh_tests_SRC}
        "${PROJECT_SOURCE_DIR}/unittest/modules/mysql_connection_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/mod_utils_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/mod_dba_common_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/mod_dba_replicaset_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/mod_dba_sql_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/mod_dba_preconditions_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/common/metadata_management_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/devapi/mod_mysqlx_collection_find_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/devapi/mod_mysqlx_table_select_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/shell_cmdline_regressions_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/shell_cli_operation_t.cc"
        "${CMAKE_SOURCE_DIR}/unittest/test_main.cc"
        "${CMAKE_SOURCE_DIR}/unittest/test_utils.cc"
        "${CMAKE_SOURCE_DIR}/unittest/shell_script_tester.cc"
    )
    if(NOT HAVE_PROTOBUF)
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/mod_mysqlx_t.cc")
    endif()

    if(NOT HAVE_V8)
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/value_js_bridging_t.cc")
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/shell_js_t.cc")
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/auto_script_js_t.cc")
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/shell_js_dba_t.cc")
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/shell_js_mysql_t.cc")
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/shell_js_mysqlx_t.cc")
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/test_js_dev_api_samples_t.cc")
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/modules/api_connections_t.cc")
    endif()

    if(NOT HAVE_PYTHON)
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/value_py_bridging_t.cc")
    endif()

    LIST(APPEND mysqlsh_tests_SRC
        ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/linenoise.cpp
        ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp
        ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/wcwidth.cpp)

    IF(CMAKE_COMPILER_IS_GNUCXX)
      SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp PROPERTIES COMPILE_FLAGS "-Wno-implicit-fallthrough")
    ENDIF()

    IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp PROPERTIES COMPILE_FLAGS "-Wno-keyword-macro")
    ENDIF()

    add_shell_executable(run_unit_tests "${mysqlsh_tests_SRC}" TRUE)

    add_dependencies(run_unit_tests
            shellfe
            api_modules
            mysqlshdk-static)

    target_link_libraries(run_unit_tests
            shellfe
            api_modules
            db
            mysqlshdk-static
            ${GTEST_LIBRARIES}
            ${MYSQLX_LIBRARIES}
            ${PROTOBUF_LIBRARY}
            ${LZ4_LIBRARY}
            ${SSL_LIBRARIES}
            ${SSL_LIBRARIES_DL}
            ${V8_LINK_LIST}
            ${PYTHON_LIBRARIES}
            ${MYSQL_EXTRA_LIBRARIES}
    )

    IF(WIN32)
      target_link_libraries(run_unit_tests Dbghelp.lib)
    ELSE()
      target_link_libraries(run_unit_tests pthread ${GCOV_LDFLAGS})
    ENDIF()

    # Copy sample prompt files to the test folder
    add_custom_command(TARGET run_unit_tests PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${CMAKE_SOURCE_DIR}/samples/prompt/
                        $<TARGET_FILE_DIR:run_unit_tests>/)

    if(HAVE_V8)
      target_link_libraries(run_unit_tests ${V8_LINK_LIST})
    endif()

    if(HAVE_PYTHON)
      target_link_libraries(run_unit_tests "${PYTHON_LIBRARIES}")
    endif()
else()
  message(WARNING "Skipping tests. To enable unit-tests use -DWITH_TESTS=1 -DWITH_GTEST=path")
endif()
