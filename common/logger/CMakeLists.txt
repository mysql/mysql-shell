# Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#
# Sample cmakes (keep as reference)
#
# For Windows:
#  cd <ngcommon-root>/logger
#  mkdir MY_BUILD
#  cd MY_BUILD  
#  cmake -DBOOST_ROOT=C:\src\mysql-server\boost\boost_1_56_0 -DWITH_GTEST=C:\src\mysql-server\ngshell-deps\v8\testing\gtest\bld ..
# For Linux:
#  cmake -DBOOST_ROOT=/home/fernando/src/boost/boost_1_56_0/  -DWITH_GTEST=/home/fernando/src/gtest/gtest-1.7.0/MY_BUILD ..
#  cmake -DBOOST_ROOT=/home/fernando/src/boost/boost_1_56_0/  -DWITH_GTEST=/home/fernando/src/gtest/gtest-1.7.0/MY_BUILD -DCMAKE_BUILD_TYPE=Debug -DCMAKE_ECLIPSE_VERSION=4.4 -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -G "Eclipse CDT4 - Unix Makefiles" ..
#

cmake_minimum_required (VERSION 2.8)

project (logger)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../cmake)

include(msvc)

find_package(Boost)

# Common warning flags for GCC, G++, Clang and Clang++
set(MY_WARNING_FLAGS "-Wall -Wextra -Wformat-security -Wvla")

# Common warning flags for GCC and Clang
set(MY_C_WARNING_FLAGS
    "${MY_WARNING_FLAGS} -Wwrite-strings -Wdeclaration-after-statement")

include_directories("${Boost_INCLUDE_DIRS}")
include_directories("${PROJECT_SOURCE_DIR}")

#link_directories(${BOOST_LIB_DIR})

file(GLOB logger_SRC
	"${PROJECT_SOURCE_DIR}/logger.h"
	"${PROJECT_SOURCE_DIR}/logger.cc"
)

add_library(logger ${logger_SRC})

if(WITH_GTEST)

  include(testing)
  SETUP_TESTING()

  file(GLOB logger_tests_SRC
    "${PROJECT_SOURCE_DIR}/tests/*.cc"
  )

  add_definitions( -DPROCESS_LAUNCHER_TESTS_DIR="${PROJECT_SOURCE_DIR}/tests" )

  include_directories(${GTEST_INCLUDE_DIR})

  add_executable(logger.tests ${logger_tests_SRC})
  target_link_libraries(logger.tests logger)
  if (NOT WIN32)
    target_link_libraries(logger.tests pthread)
  endif()

  target_link_libraries(logger.tests gtest)


  #if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  #  target_link_libraries(logger.tests boost_system)
  #endif()

  target_link_libraries(logger.tests ${GMOCK_LIBRARIES})
endif()

# Force project be use MT, so they dont conflict a link time with clients of process_launcher library.
if(WIN32)
  CHANGE_MD_2_MT()
endif()

add_definitions(-DBOOST_ALL_NO_LIB )
