# Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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
    include_directories(${CMAKE_SOURCE_DIR}/unittest/include 
                ${CMAKE_SOURCE_DIR}/mysqlxtest
                ${GTEST_INCLUDE_DIR} 
                ${CMAKE_SOURCE_DIR}/include
                ${MYSQL_INCLUDE_DIRS}
    )

    file(GLOB mysqlsh_tests_SRC
        "${PROJECT_SOURCE_DIR}/unittest/*.h"
        "${PROJECT_SOURCE_DIR}/unittest/*_t.cc"
        "${PROJECT_SOURCE_DIR}/unittest/test_main.cc"
        "${PROJECT_SOURCE_DIR}/src/boost_code.cc"
    )

    if (HAVE_PROTOBUF)
      INCLUDE(protobuf)
      SETUP_PROTOBUF()
      INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/mysqlxtest")
    else()
      list(REMOVE_ITEM mysqlsh_tests_SRC "${PROJECT_SOURCE_DIR}/unittest/mod_mysqlx_t.cc")
    endif()

    if ( NOT HAVE_V8 )
      list(REMOVE_ITEM mysqlsh_tests_SRC "${PROJECT_SOURCE_DIR}/unittest/value_js_bridging_t.cc")
      list(REMOVE_ITEM mysqlsh_tests_SRC "${PROJECT_SOURCE_DIR}/unittest/shell_js_t.cc")
    endif()

    if (NOT HAVE_PYTHON )
      list(REMOVE_ITEM mysqlsh_tests_SRC "${PROJECT_SOURCE_DIR}/unittest/value_py_bridging_t.cc")
    endif()

    add_executable(run_unit_tests ${mysqlsh_tests_SRC})
    add_dependencies(run_unit_tests mysqlshmods)
    if (HAVE_PROTOBUF)
      add_dependencies(run_unit_tests mysqlxtest)
    endif()
    target_link_libraries(run_unit_tests
            mysqlsh
            mysqlshmods
            gtest
            ${MYSQL_LIBRARIES}
            )

    if (HAVE_PROTOBUF)
      target_link_libraries(run_unit_tests
              ${PROTOBUF_LIBRARY}
              mysqlxtest
              )
    endif()

    if ( HAVE_V8 )
      target_link_libraries(run_unit_tests ${V8_LINK_LIST})

      add_custom_command(TARGET run_unit_tests PRE_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_directory
                            ${CMAKE_SOURCE_DIR}/unittest/modules $<TARGET_FILE_DIR:run_unit_tests>)
    endif()

    add_custom_command(TARGET run_unit_tests PRE_BUILD
                          COMMAND ${CMAKE_COMMAND} -E copy_directory
                          ${CMAKE_SOURCE_DIR}/unittest/data $<TARGET_FILE_DIR:run_unit_tests>)

    # This is to copy the mysqlx into the build directory to make them
    # available for running the unit tests
    add_custom_command(TARGET run_unit_tests PRE_BUILD
                          COMMAND ${CMAKE_COMMAND} -E copy_directory
                          ${CMAKE_SOURCE_DIR}/scripting/modules $<TARGET_FILE_DIR:run_unit_tests>/modules)

    if ( HAVE_PYTHON )
      target_link_libraries(run_unit_tests ${PYTHON_LIBRARIES})
    endif()

    if (NOT WIN32)
      target_link_libraries(run_unit_tests pthread ${GCOV_LDFLAGS})
    endif()

    include(TestGroups.txt)
else()
    message(WARNING "Skipping tests. To enable unit-tests use -DWITH_TESTS=1 -DWITH_GTEST=path")
endif()


add_definitions(-DBOOST_ALL_NO_LIB )

if(WIN32)
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()
endif()
