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

cmake_minimum_required (VERSION 2.8)

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
set(V8_INCLUDE_DIR /usr/local/include/v8)

set(MY_CXX_WARNING_FLAGS "${MY_CXX_WARNING_FLAGS} -Werror")

find_package(MYSQL REQUIRED)


link_directories( ${MY_GMOCK_LIB_DIR} )

add_subdirectory(src/libedit)


# needed when compiling in osx, for v8
add_definitions(-std=gnu++0x)

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
target_link_libraries(mysh -L/usr/local/lib/v8 -lv8_snapshot -licudata -lv8_libbase -lv8_base -licuuc -licui18n)


