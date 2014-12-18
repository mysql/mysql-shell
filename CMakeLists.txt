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
	
	

set(MY_CXX_WARNING_FLAGS "${MY_CXX_WARNING_FLAGS} -Werror")



link_directories( ${MY_GMOCK_LIB_DIR} )
link_directories( ${MY_BOOST_LIB_DIR} )

add_subdirectory(shellcore)
add_subdirectory(src/libedit)

# mysh cmdline util build

include_directories("${PROJECT_SOURCE_DIR}/include" /usr/local/include
                    "${PROJECT_SOURCE_DIR}/src/libedit"
                   )

file(GLOB mysh_SRC
	"${PROJECT_SOURCE_DIR}/src/*.cc"
)

add_executable(mysh ${mysh_SRC})
target_link_libraries(mysh mysqlsh readline)
target_link_libraries(mysh -L/usr/local/lib/v8 -lv8_base) # TODO v8 check


