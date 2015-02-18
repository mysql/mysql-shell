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
    add_definitions(-DGTEST_LANG_CXX11=0 ${GMOCK_CFLAGS})
    include_directories(${CMAKE_SOURCE_DIR}/testing/include 
                ${GTEST_INCLUDE_DIR} 
                ${CMAKE_SOURCE_DIR}/include
                ${MYSQL_INCLUDE_DIRS}
    )


    file(GLOB mysqlsh_tests_SRC
        "${PROJECT_SOURCE_DIR}/testing/*_t.cc"
        "${PROJECT_SOURCE_DIR}/testing/test_main.cc"
        "${PROJECT_SOURCE_DIR}/src/boost_code.cc"
    )
    add_executable(run_unit_tests ${mysqlsh_tests_SRC})
    target_link_libraries(run_unit_tests
            mysqlsh
            mysqlshmods
            gtest
            ${PROTOBUF_LIBRARY}
            ${V8_LINK_LIST}
            ${MYSQL_LIBRARIES}
            )

    if (NOT WIN32)
        target_link_libraries( run_unit_tests pthread )
#        target_link_libraries( run_unit_tests boost_filesystem )
        target_link_libraries( runt.tests, ${GCOV_LDFLAGS})
    endif()

    include(TestGroups.txt)
else()
    message(WARNING "Skipping tests. To enable unit-tests use -DWITH_TESTS=1 -DWITH_GTEST=path")
endif()



add_definitions(-DBOOST_ALL_NO_LIB )

if(WIN32)
  CHANGE_MD_2_MT()
endif()
