# Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

# Sample command lines
#  (Don't delete, my copy & paste)
# For Windows:
#   cmake -DMY_INCLUDE_DIR=C:\src\mysql-server\MyIncludes -DMY_GMOCK_LIB_DIR=C:\src\mysql-server\MyLibs -DMY_BOOST_LIB_DIR=C:\src\mysql-server\boost\boost_1_56_0\stage\lib -DMY_V8_LIB_DIR=C:\src\mysql-server\MyLibs\v8 .. 
#
# For Linux:
#  cmake -DMY_INCLUDE_DIR=/usr/local/include:/usr/include -DMY_BOOST_LIB_DIR=/usr/local/lib -DMY_V8_LIB_DIR=/home/fernando/src/v8/v8-git-mirror/out/Debug/lib.target -DV8_INCLUDE_DIR=/home/fernando/src/v8/v8-git-mirror .. 
#
#  cmake -DMY_INCLUDE_DIR=/usr/local/include:/usr/include -DMY_BOOST_LIB_DIR=/usr/local/lib -DMY_V8_LIB_DIR=/home/fernando/src/v8/v8-git-mirror/out/Debug/lib.target -DV8_INCLUDE_DIR=/home/fernando/src/v8/v8-git-mirror -DCMAKE_BUILD_TYPE=Debug -DCMAKE_ECLIPSE_VERSION=4.4 -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -DMYSQL_INCLUDE_DIRS=/usr/local/mysql/include  DBOOST_INCLUDE_DIR=/usr/local/include -G "Eclipse CDT4 - Unix Makefiles" ../ngshell-4/

#
#
cmake_minimum_required (VERSION 2.8)

if (APPLE)
# Deployment target must be set to 10.5, to match v8, otherwise we get link errors
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.5)
endif()

project (mysh)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

INCLUDE(libutils)
INCLUDE(boost)

# Common warning flags for GCC, G++, Clang and Clang++
set(MY_WARNING_FLAGS "-Wall -Wextra -Wformat-security -Wvla")

# Common warning flags for GCC and Clang
set(MY_C_WARNING_FLAGS
    "${MY_WARNING_FLAGS} -Wwrite-strings -Wdeclaration-after-statement")
	
	
# TODO add auto-detection/manual param taking for this
if( ${APPLE} )
  set(V8_INCLUDE_DIR /usr/local/include/v8)
else()
  # TODO: Other OSs specify it at the command line.
endif()

set(MY_CXX_WARNING_FLAGS "${MY_CXX_WARNING_FLAGS} -Werror")

if( ${APPLE} )
find_package(MYSQL REQUIRED)
else()
find_package(MySQL REQUIRED)
endif()

include_directories( ${MY_INCLUDE_DIR} )
include_directories( ${V8_INCLUDE_DIR} )
include_directories( ${MYSQL_INCLUDE_CMAKE_DIRS} )


link_directories( ${MY_GMOCK_LIB_DIR} )
link_directories( ${BOOST_INCLUDE_DIR} )
link_directories( ${MY_BOOST_LIB_DIR} )
link_directories( ${MY_V8_LIB_DIR} )

add_subdirectory(src/libedit)


if (APPLE)
# needed when compiling in osx, for v8
add_definitions(-std=gnu++0x)
endif()

add_subdirectory(shellcore)

# mysh cmdline util build
include_directories("${PROJECT_SOURCE_DIR}/include"
                    "${PROJECT_SOURCE_DIR}/src/libedit"
                    "${MYSQL_INCLUDE_DIRS}"
                    "${BOOST_INCLUDE_DIR}"
                   )

file(GLOB mysh_SRC
    "${PROJECT_SOURCE_DIR}/src/*.cc"
    "${PROJECT_SOURCE_DIR}/modules/*.cc"
    "${PROJECT_SOURCE_DIR}/modules/*.h"
)

add_executable(mysh ${mysh_SRC})
target_link_libraries(mysh mysqlsh readline)
target_link_libraries(mysh ${MYSQL_LIBRARIES})

if( ${APPLE} )
  target_link_libraries(mysh -L/usr/local/lib/v8 -lv8_snapshot -licudata -lv8_libbase -lv8_base -licuuc -licui18n)
elseif( ${WIN32} )
  # TODO: link in VC++
  target_link_libraries(mysh v8_snapshot v8_libbase v8_base icuuc icui18n)
else()	# Linux
  target_link_libraries(mysh -L${MY_V8_LIB_DIR} -lv8 -licuuc -licui18n)
endif()

# enable_testing()

file(GLOB shcore_tests_SRC
	"${PROJECT_SOURCE_DIR}/tests/*.cc"
)

add_executable( shcore.tests ${shcore_tests_SRC} )
target_link_libraries( shcore.tests mysqlsh )
target_link_libraries( shcore.tests gmock )
if (${WIN32})
  #target_link_libraries( shcore.tests gtestd )
  target_link_libraries( shcore.tests       	 libboost_filesystem-vc120-mt-gd-1_56 )
else()
  target_link_libraries( shcore.tests gtest )
  target_link_libraries( shcore.tests pthread )
  target_link_libraries( shcore.tests boost_filesystem )
  target_link_libraries( shcore.tests boost_system )
endif()

