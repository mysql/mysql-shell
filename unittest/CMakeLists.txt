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

if(WITH_TESTS)
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
                ${GTEST_INCLUDE_DIR}
                ${GMOCK_INCLUDE_DIR}
                ${PYTHON_INCLUDE_DIR}
                ${V8_INCLUDE_DIR})

    file(GLOB mysqlsh_tests_SRC
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/shellcore/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/db/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/mysql/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/utils/*.cc"
        "${PROJECT_SOURCE_DIR}/unittest/mysqlshdk/libs/gr/group_replication_t.cc"
        "${CMAKE_SOURCE_DIR}/unittest/*_t.cc"
        "${CMAKE_SOURCE_DIR}/unittest/test_utils/*.cc"
        "${CMAKE_SOURCE_DIR}/unittest/test_utils/mocks/mysqlshdk/libs/*/*.cc"
        "${CMAKE_SOURCE_DIR}/unittest/test_utils/mocks/modules/adminapi/*.h"
        "${CMAKE_SOURCE_DIR}/unittest/test_utils/mocks/modules/adminapi/*.cc"
        "${CMAKE_SOURCE_DIR}/unittest/*/*_t.cc")

    set(mysqlsh_tests_SRC
        ${mysqlsh_tests_SRC}
        "${PROJECT_SOURCE_DIR}/unittest/modules/mysql_connection_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/mod_utils_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/mod_dba_common_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/mod_dba_replicaset_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/mod_dba_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/mod_cluster_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/adminapi/mod_dba_sql_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/devapi/mod_mysqlx_collection_find_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/devapi/mod_mysqlx_table_select_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/shell_cmdline_regressions_t.cc"
        "${CMAKE_SOURCE_DIR}/unittest/test_main.cc"
        "${CMAKE_SOURCE_DIR}/unittest/test_utils.cc"
        "${CMAKE_SOURCE_DIR}/unittest/shell_script_tester.cc"

        # NOTE: these sources are required because Mysql_shell is not part of a library used by the test system
        # but is ont the application level. At some point we should put Mysql_shell available in a alibrary
        # to be integrable as Full Shell Functionality
        "${CMAKE_SOURCE_DIR}/src/mysqlsh/get_password.cc"
        "${CMAKE_SOURCE_DIR}/src/mysqlsh/mysql_shell.cc"
        "${CMAKE_SOURCE_DIR}/src/mysqlsh/prompt_renderer.cc"
        "${CMAKE_SOURCE_DIR}/src/mysqlsh/prompt_manager.cc"
        "${CMAKE_SOURCE_DIR}/src/mysqlsh/cmdline_shell.cc"
        "${CMAKE_SOURCE_DIR}/src/mysqlsh/history.cc"

        "${CMAKE_SOURCE_DIR}/src/interactive/interactive_global_dba.cc"
        "${CMAKE_SOURCE_DIR}/src/interactive/interactive_dba_cluster.cc"
        "${CMAKE_SOURCE_DIR}/src/interactive/interactive_global_shell.cc"
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

    add_subdirectory(mysqld_mock)
    LIST(APPEND mysqlsh_tests_SRC
        ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/linenoise.cpp
        ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp
        ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/wcwidth.cpp)

    add_executable(run_unit_tests ${mysqlsh_tests_SRC})
    add_dependencies(run_unit_tests
            api_modules
            mysqlshdk-static)

    target_link_libraries(run_unit_tests
            api_modules
            db
            mysqlshdk-static
            gtest
            gmock
            ${MYSQLX_LIBRARIES}
            ${PROTOBUF_LIBRARY}
            ${SSL_LIBRARIES}
            ${SSL_LIBRARIES_DL}
            ${V8_LINK_LIST}
            ${PYTHON_LIBRARIES}
            ${MYSQL_EXTRA_LIBRARIES}
    )

    # Copies the mysql_server_mock to the binary folder
    add_custom_command(TARGET run_unit_tests PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy
                        $<TARGET_FILE:mysql_server_mock> $<TARGET_FILE_DIR:run_unit_tests>/)
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

    # Sets the test environment so it is ready for execution out of the box
    # On windows mysqlprovision.zip, cmd and mysqlsh.exe need to be together
    IF(WIN32)
      # Copies the mysqlprovision.zip to the binary folder
      add_custom_command(TARGET run_unit_tests PRE_BUILD
                          COMMAND ${CMAKE_COMMAND} -E copy
                          "${CMAKE_BINARY_DIR}/mysqlprovision.zip" $<TARGET_FILE_DIR:run_unit_tests>/mysqlprovision.zip)

      # Copies the mysqlsh.exe to the binary folder
      add_custom_command(TARGET run_unit_tests PRE_BUILD
                          COMMAND ${CMAKE_COMMAND} -E copy
                          $<TARGET_FILE:mysqlsh> $<TARGET_FILE_DIR:run_unit_tests>/)

	  # Copies the mysqlshrec.exe to the binary folder
      add_custom_command(TARGET run_unit_tests PRE_BUILD
                          COMMAND ${CMAKE_COMMAND} -E copy
                          $<TARGET_FILE:mysqlshrec> $<TARGET_FILE_DIR:run_unit_tests>/)
    ENDIF()


    if ( HAVE_V8 )
      target_link_libraries(run_unit_tests ${V8_LINK_LIST})
    endif()

    if ( HAVE_PYTHON )
      target_link_libraries(run_unit_tests "${PYTHON_LIBRARIES}")

      if (WIN32)
        IF(NOT WITH_STATIC_MSVCRT)
          set(PY_DLL ${PYTHON_LIBRARIES})
          string(REPLACE ".lib" ".dll" PY_DLL ${PY_DLL})
          get_filename_component(PY_FILE ${PY_DLL} NAME)
          get_filename_component(PY_DLL ${PY_DLL} DIRECTORY)
          get_filename_component(PY_DLL ${PY_DLL} DIRECTORY)

          # Copies the dll to the binary location to enable debugging
          add_custom_command(TARGET run_unit_tests PRE_BUILD
                              COMMAND ${CMAKE_COMMAND} -E copy
                              "${PY_DLL}/${PY_FILE}" $<TARGET_FILE_DIR:run_unit_tests>/${PY_FILE})
        ENDIF()

        IF(PYTHON_PATH AND PYTHON_FOLDER)
          # This is to copy the modules into the build directory to make them
          # available for debugging purposes
          add_custom_command(TARGET run_unit_tests PRE_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_directory
                          "${PYTHON_LIBRARY_PATH}" $<TARGET_FILE_DIR:run_unit_tests>/Python2.7/Lib)

          # This is to copy the pyd modules into the build directory to make them
          # available for debugging purposes
          add_custom_command(TARGET run_unit_tests PRE_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_directory
                          "${PYTHON_DLLS_PATH}" $<TARGET_FILE_DIR:run_unit_tests>/Python2.7/DLLs)
        ENDIF()
      endif()
    endif()

    if(NOT WIN32)
      target_link_libraries(run_unit_tests pthread ${GCOV_LDFLAGS})
    endif()

    include(TestGroups.txt)
else()
  message(WARNING "Skipping tests. To enable unit-tests use -DWITH_TESTS=1 -DWITH_GTEST=path")
endif()
