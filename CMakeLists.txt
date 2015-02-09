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

# Sample command lines
#  (Don't delete, my copy & paste)
# For Windows:
#  cmake -DV8_INCLUDE_DIR=C:\src\mysql-server\v8-git-v3.28.73\include -DV8_LIB_DIR=C:\src\mysql-server\MyLibs\v8\Debug -DWITH_BOOST=C:\src\mysql-server\boost\boost_1_56_0 -DGMOCK_DIR=C:\src\OpenSource\gmock-1.7.0 ..
# For Linux:
#  cmake -DWITH_BOOST=/home/fernando/src/boost_1_56_0/ -DV8_LIB_DIR=/home/fernando/src/v8/v8-git-v3.28.73/out/Debug/lib.target -DV8_INCLUDE_DIR=/home/fernando/src/v8/v8-git-v3.28.73/include .. 
#
#
#  cmake -DWITH_BOOST=/home/fernando/src/boost_1_56_0/ -DV8_LIB_DIR=/home/fernando/src/v8/v8-git-v3.28.73/out/Debug/lib.target -DV8_INCLUDE_DIR=/home/fernando/src/v8/v8-git-v3.28.73/include -DCMAKE_BUILD_TYPE=Debug -DCMAKE_ECLIPSE_VERSION=4.4 -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE --G "Eclipse CDT4 - Unix Makefiles" ..
#
#
#
cmake_minimum_required (VERSION 2.8)

project (mysh)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

###
### Detect Stuff
###

INCLUDE(libutils)
INCLUDE(boost)
include(msvc)

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckFunctionExists)


find_package(MySQL REQUIRED)


CHECK_INCLUDE_FILES (pwd.h HAVE_PWD_H)
if(WIN32)
else()
  CHECK_INCLUDE_FILES (sys/ioctl.h HAVE_SYS_IOCTL_H)
  CHECK_INCLUDE_FILES (termio.h HAVE_TERMIO_H)
  CHECK_INCLUDE_FILES (termios.h HAVE_TERMIOS_H)
  CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
endif()

CHECK_FUNCTION_EXISTS (getpass HAVE_GETPASS)

CONFIGURE_FILE(config.h.cmake   ${CMAKE_BINARY_DIR}/include/mysh_config.h)

###
### Handle User Options
###

# Common warning flags for GCC, G++, Clang and Clang++
set(MY_WARNING_FLAGS "-Wall -Wextra -Wformat-security -Wvla")

# Common warning flags for GCC and Clang
set(MY_C_WARNING_FLAGS
    "${MY_WARNING_FLAGS} -Wwrite-strings -Wdeclaration-after-statement")

set(MY_CXX_WARNING_FLAGS "${MY_CXX_WARNING_FLAGS} -Werror")

# enable gcov
if(ENABLE_GCOV)
    message("Enabling gcov")
    add_definitions(-fprofile-arcs -ftest-coverage)
endif()

add_definitions(-DSHCORE_EXPORT )

# GMock/GTest
if(GMOCK_DIR)
    message(STATUS "Determining gmock compile flags")
    set(GMOCK_CFLAGS "-I${GMOCK_DIR}/include -I${GMOCK_DIR}/gtest/include")
    find_library(GMOCK_LIB NAMES gmock PATHS ${GMOCK_DIR}/lib ${GMOCK_DIR}/lib/.libs)
    find_library(GTEST_LIB NAMES gtest PATHS ${GMOCK_DIR}/gtest/lib ${GMOCK_DIR}/gtest/lib/.libs)
    set(GMOCK_LIBRARIES ${GMOCK_LIB} ${GTEST_LIB})
    message(STATUS "gmock cflags: ${GMOCK_CFLAGS}")
    message(STATUS "gmock libs: ${GMOCK_LIBRARIES}")
else()
    message(STATUS "GMOCK_CONFIG_PATH not specified, will skip building tests")
endif()

###
### Build Projects
###

include_directories( ${CMAKE_BINARY_DIR}/include/ )
include_directories( ${V8_INCLUDE_DIR} )

link_directories( ${V8_LIB_DIR} )

if(NOT WIN32)
  add_subdirectory(src/libedit)
endif()

if (APPLE)
# needed when compiling in osx, for v8
# has to be after src/libedit
add_definitions(-std=gnu++0x -mmacosx-version-min=10.5)
endif()


add_subdirectory(shellcore)

# mysh cmdline util build
include_directories("${PROJECT_SOURCE_DIR}/include"
                    "${MYSQL_INCLUDE_DIRS}"
                    "${BOOST_INCLUDE_DIR}"
                   )
if(NOT WIN32)
  include_directories( "${PROJECT_SOURCE_DIR}/src/libedit" )
endif()				   

file(GLOB mysh_SRC
    "${PROJECT_SOURCE_DIR}/src/*.cc"
    "${PROJECT_SOURCE_DIR}/modules/*.cc"
    "${PROJECT_SOURCE_DIR}/modules/*.h"
)

add_executable(mysh ${mysh_SRC})

if(WIN32)
  target_link_libraries(mysh mysqlsh)
  add_dependencies(mysh boost_code.obj)
else()
  target_link_libraries(mysh mysqlsh readline)
endif()
target_link_libraries(mysh ${MYSQL_LIBRARIES})


if(APPLE)
  set(V8_LINK_LIST -mmacosx-version-min=10.5 -L${V8_LIB_DIR} -lv8_snapshot -licudata -lv8_libbase -lv8_base -licuuc -licui18n)
elseif(WIN32)
  set(V8_LINK_LIST v8_snapshot v8_libbase v8_base icuuc icui18n winmm)
else()	# Linux
  set(V8_LINK_LIST -L${V8_LIB_DIR} -lv8 -licuuc -licui18n)
endif()

target_link_libraries(mysh ${V8_LINK_LIST})

# enable_testing()

if(GMOCK_LIBRARIES)

  file(GLOB shcore_tests_SRC
    "${PROJECT_SOURCE_DIR}/modules/*.cc"
	"${PROJECT_SOURCE_DIR}/tests/*.cc"
	"${PROJECT_SOURCE_DIR}/src/boost_code.cc"
  )

  add_definitions(-DGTEST_LANG_CXX11=0 ${GMOCK_CFLAGS})

  add_executable( shcore.tests ${shcore_tests_SRC} )
  target_link_libraries( shcore.tests mysqlsh )

  if (NOT WIN32)
    target_link_libraries( shcore.tests pthread )
  endif()

  target_link_libraries(shcore.tests
        ${V8_LINK_LIST}
        ${GMOCK_LIBRARIES}
        ${MYSQL_LIBRARIES})

  if(ENABLE_GCOV)
    target_link_libraries(shcore.tests -fprofile-arcs -ftest-coverage)
  endif()

endif()


if(WIN32)
  CHANGE_MD_2_MT()
endif()

add_definitions(-DBOOST_ALL_NO_LIB )
