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

cmake_minimum_required (VERSION 2.8)

project(sample_client)

set(CMAKE_INSTALL_PREFIX "${DEPLOY_DIR}")

file(GLOB app_SRC
      "${PROJECT_SOURCE_DIR}/app/simple_shell_client_app.cc"
      "${PROJECT_SOURCE_DIR}/app/boost_code.cc"
)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET_SOURCE_FILES_PROPERTIES(${PROJECT_SOURCE_DIR}/app/boost_code.cc PROPERTIES COMPILE_FLAGS -O0)
endif ()


add_executable(shell_client_app ${app_SRC})

if (WITH_SAMPLES)
  include_directories("${PROJECT_SOURCE_DIR}/include"
                      "${MYSQL_INCLUDE_DIR}"
                      "${Boost_INCLUDE_DIRS}"
                      "${CMAKE_SOURCE_DIR}/utils"
                      "${CMAKE_SOURCE_DIR}"
                     )
else()
  include_directories("${DEPLOY_DIR}/include")
endif()
include_directories(${BOOST_INCLUDE_DIR})
include_directories("${PROJECT_SOURCE_DIR}/lib")

add_subdirectory(lib)

if(WIN32)
  target_link_libraries(shell_client_app shell_client_lib ${PYTHON_LIBS})
else()
  target_link_libraries(shell_client_app
    shell_client_lib
    ${PYTHON_LIBS} ${PROTOBUF_LIBRARY}
  )
endif()

add_definitions(-DBOOST_ALL_NO_LIB)