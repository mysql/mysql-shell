# Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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
    # disable optimization for boost error_code, otherwise we get link errors in Release builds
    if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      SET_SOURCE_FILES_PROPERTIES(${PROJECT_SOURCE_DIR}/src/boost_code.cc PROPERTIES COMPILE_FLAGS -O0)
    endif ()

    add_definitions(-DGTEST_LANG_CXX11=0 ${GMOCK_CFLAGS})
    add_definitions(-DMYSQLX_SOURCE_HOME="${CMAKE_SOURCE_DIR}")

    include_directories(${CMAKE_SOURCE_DIR}/unittest/include
                ${CMAKE_SOURCE_DIR}/mysqlxtest
                ${CMAKE_SOURCE_DIR}/mysqlxtest/common
                ${CMAKE_SOURCE_DIR}/ext/rapidjson/include
                ${GTEST_INCLUDE_DIR}
                ${CMAKE_SOURCE_DIR}/include
                ${CMAKE_SOURCE_DIR}/modules
                ${MYSQL_INCLUDE_DIRS}
    )

    file(GLOB mysqlsh_tests_SRC
        "${PROJECT_SOURCE_DIR}/unittest/*.h"
        "${PROJECT_SOURCE_DIR}/unittest/*_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/mysql_connection_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/modules/mod_utils_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/test_main.cc"
        "${PROJECT_SOURCE_DIR}/unittest/test_utils.cc"
        "${PROJECT_SOURCE_DIR}/unittest/shell_script_tester.cc"
        "${PROJECT_SOURCE_DIR}/src/boost_code.cc"
        "${PROJECT_SOURCE_DIR}/src/get_password.cc"
        "${PROJECT_SOURCE_DIR}/src/shell_resultset_dumper.cc"
        "${PROJECT_SOURCE_DIR}/src/interactive_shell.cc"
        "${PROJECT_SOURCE_DIR}/src/shell_cmdline_options.cc"
    )

    if (HAVE_PROTOBUF)
      INCLUDE(protobuf)
      SETUP_PROTOBUF()
      INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/mysqlxtest")
    else()
      list(REMOVE_ITEM mysqlsh_tests_SRC "${PROJECT_SOURCE_DIR}/unittest/mod_mysqlx_t.cc")
    endif()

    if (NOT WIN32)
      include_directories( "${PROJECT_SOURCE_DIR}/src/libedit" )
    endif()

    if ( NOT HAVE_V8 )
      list(REMOVE_ITEM mysqlsh_tests_SRC "${PROJECT_SOURCE_DIR}/unittest/value_js_bridging_t.cc")
      list(REMOVE_ITEM mysqlsh_tests_SRC "${PROJECT_SOURCE_DIR}/unittest/shell_js_t.cc")
    endif()

    if (NOT HAVE_PYTHON )
      list(REMOVE_ITEM mysqlsh_tests_SRC "${PROJECT_SOURCE_DIR}/unittest/value_py_bridging_t.cc")
    endif()

    add_subdirectory(test_utils)

    add_executable(run_unit_tests ${mysqlsh_tests_SRC})
    add_dependencies(run_unit_tests mysqlshcore)
    add_dependencies(run_unit_tests mysqlxtest)
    target_link_libraries(run_unit_tests
            mysqlshcore
            mysqlxtest
            gtest
            test_utils
            ${MYSQL_LIBRARIES}
            ${PROTOBUF_LIBRARY}
            ${SSL_LIBRARIES}
            ${SSL_LIBRARIES_DL}
    )

    # Copies the mysqlshcore dll to the binary location to enable running the UTs
    if(WINDOWS_RUNTIME_MD)
      add_custom_command(TARGET run_unit_tests PRE_BUILD
                          COMMAND ${CMAKE_COMMAND} -E copy
                          $<TARGET_FILE:mysqlshcore> $<TARGET_FILE_DIR:run_unit_tests>)
    endif()


    if ( HAVE_V8 )
      target_link_libraries(run_unit_tests ${V8_LINK_LIST})

      if(WINDOWS_RUNTIME_MD)
        # for MD runtime we also need DLL, not just lib, but dll is on parent directory.
        set(V8_DLL ${V8_LIBS})
        string(REPLACE ".lib" ".dll" V8_DLL ${V8_DLL})
        get_filename_component(V8_FILE ${V8_DLL} NAME)
        get_filename_component(V8_DLL ${V8_DLL} DIRECTORY)
        get_filename_component(V8_DLL ${V8_DLL} DIRECTORY)
        install(FILES "${V8_DLL}/${V8_FILE}" DESTINATION bin COMPONENT main)

        if (WITH_DEV)
          install(FILES "${V8_DLL}/${V8_FILE}" DESTINATION lib COMPONENT dev)
        endif()

        # Copies the dll to the binary location to enable debugging
        add_custom_command(TARGET run_unit_tests PRE_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy
                            "${V8_DLL}/${V8_FILE}" $<TARGET_FILE_DIR:run_unit_tests>/${V8_FILE})
      endif()
    endif()

    if ( HAVE_PYTHON )
      target_link_libraries(run_unit_tests "${PYTHON_LIBRARIES}")

      if (WIN32)
        if(CMAKE_BUILD_TYPE MATCHES Debug OR WINDOWS_RUNTIME_MD)
          set(PY_DLL ${PYTHON_LIBRARIES})
          string(REPLACE ".lib" ".dll" PY_DLL ${PY_DLL})
          get_filename_component(PY_FILE ${PY_DLL} NAME)
          get_filename_component(PY_DLL ${PY_DLL} DIRECTORY)
          get_filename_component(PY_DLL ${PY_DLL} DIRECTORY)

          # Copies the dll to the binary location to enable debugging
          add_custom_command(TARGET run_unit_tests PRE_BUILD
                              COMMAND ${CMAKE_COMMAND} -E copy
                              "${PY_DLL}/${PY_FILE}" $<TARGET_FILE_DIR:run_unit_tests>/${PY_FILE})
        endif()
      endif()
    endif()

    if (NOT WIN32)
      target_link_libraries(run_unit_tests pthread edit ${GCOV_LDFLAGS})
    endif()

    include(TestGroups.txt)
else()
    message(WARNING "Skipping tests. To enable unit-tests use -DWITH_TESTS=1 -DWITH_GTEST=path")
endif()


add_definitions(-DBOOST_ALL_NO_LIB)

if(WIN32)
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()
endif()


ADD_EXECUTABLE(shexpr shexpr.cc)
TARGET_LINK_LIBRARIES(shexpr
            ${MYSQLSHCORE_LIBS}
            mysqlxtest
            gtest
            ${SSL_LIBRARIES}
            ${MYSQL_LIBRARIES}
            ${PROTOBUF_LIBRARY}
            ${GCOV_LDFLAGS})
