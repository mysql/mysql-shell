# Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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
    add_definitions(-DGTEST_LANG_CXX11=0 ${GMOCK_CFLAGS})
    add_definitions(-DMYSQLX_SOURCE_HOME="${CMAKE_SOURCE_DIR}")

    include_directories(BEFORE
                ${CMAKE_SOURCE_DIR}/unittest
                ${CMAKE_SOURCE_DIR}/ext/rapidjson/include
                ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/include
                ${CMAKE_SOURCE_DIR}/mysqlshdk/libs
                ${CMAKE_SOURCE_DIR}/mysqlshdk/include
                ${CMAKE_BINARY_DIR}/mysqlshdk/include
                ${CMAKE_SOURCE_DIR}/
                ${CMAKE_SOURCE_DIR}/src
                ${MYSQL_INCLUDE_DIRS}
                ${GMOCK_INCLUDE_DIRS}
                ${PYTHON_INCLUDE_DIR}
                ${V8_INCLUDE_DIR})

    file(GLOB mysqlsh_tests_SRC
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/shellcore/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/db/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/mysql/*.cc"
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
        "${PROJECT_SOURCE_DIR}/unittest/modules/devapi/mod_mysqlx_collection_find_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/devapi/mod_mysqlx_table_select_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/shell_cmdline_regressions_t.cc"
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
    endif()

    if(NOT HAVE_PYTHON)
      list(REMOVE_ITEM mysqlsh_tests_SRC "${CMAKE_SOURCE_DIR}/unittest/value_py_bridging_t.cc")
    endif()

    LIST(APPEND mysqlsh_tests_SRC
        ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/linenoise.cpp
        ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp
        ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/wcwidth.cpp)

    add_executable(run_unit_tests ${mysqlsh_tests_SRC})
    set_target_properties(run_unit_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${INSTALL_BINDIR})
    fix_target_output_directory(run_unit_tests "${INSTALL_BINDIR}")
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
                        COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_SOURCE_DIR}/samples/prompt/prompt_nocolor.json
                        $<TARGET_FILE_DIR:run_unit_tests>/)
    add_custom_command(TARGET run_unit_tests PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_SOURCE_DIR}/samples/prompt/prompt_16.json
                        $<TARGET_FILE_DIR:run_unit_tests>/)
    add_custom_command(TARGET run_unit_tests PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_SOURCE_DIR}/samples/prompt/prompt_256.json
                        $<TARGET_FILE_DIR:run_unit_tests>/)
    add_custom_command(TARGET run_unit_tests PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_SOURCE_DIR}/samples/prompt/prompt_256pl.json
                        $<TARGET_FILE_DIR:run_unit_tests>/)

    # Fix lookup path for OpenSSL dylibs
    IF(APPLE)
      # We will not pollute "/usr/lib/", OpenSSL libraries ends up in "/usr/lib/mysqlsh/"
      add_custom_command(TARGET run_unit_tests POST_BUILD
        COMMAND install_name_tool -change
                "${CRYPTO_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${CRYPTO_VERSION}"
                $<TARGET_FILE:run_unit_tests>
        COMMAND install_name_tool -change
                "${OPENSSL_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${OPENSSL_VERSION}"
                $<TARGET_FILE:run_unit_tests>
      )
    ENDIF()

    IF(OPENSSL_TO_BUNDLE_DIR AND NOT APPLE AND NOT WIN32)
       SET_PROPERTY(TARGET run_unit_tests PROPERTY INSTALL_RPATH "\$ORIGIN/../${INSTALL_LIBDIR}")
       SET_PROPERTY(TARGET run_unit_tests PROPERTY PROPERTY BUILD_WITH_INSTALL_RPATH TRUE)
    ENDIF()

    if(HAVE_V8)
      target_link_libraries(run_unit_tests ${V8_LINK_LIST})
    endif()

    if(HAVE_PYTHON)
      target_link_libraries(run_unit_tests "${PYTHON_LIBRARIES}")
    endif()
else()
  message(WARNING "Skipping tests. To enable unit-tests use -DWITH_TESTS=1 -DWITH_GTEST=path")
endif()
