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

# ----------------------------------------------------------------------
# Sample lib source
# ----------------------------------------------------------------------

file(GLOB lib_SRC
      "simple_shell_client.cc"
      "simple_shell_client.h"
      "boost_code.cc"
)

if (WIN32 AND WINDOWS_RUNTIME_MD)
  add_library(shell_client_lib SHARED ${lib_SRC})
else()
  add_library(shell_client_lib ${lib_SRC})
endif()

add_definitions(-DSHELL_CLIENT_NATIVE_EXPORTS)
add_definitions(-DBOOST_ALL_NO_LIB)

if(WIN32 AND NOT WINDOWS_RUNTIME_MD)
  add_definitions(-DPy_NO_ENABLE_SHARED)
endif()

# ----------------------------------------------------------------------
# Set up Boost the simplest way possible
# ----------------------------------------------------------------------

if(BOOST_INCLUDE_DIR)
  include_directories("${BOOST_INCLUDE_DIR}")
elseif(BOOST_ROOT)
  link_directories("${BOOST_ROOT}/include")
endif()

if(BOOST_LIB_DIR)
  link_directories("${BOOST_LIB_DIR}")
elseif(BOOST_ROOT)
  link_directories("${BOOST_ROOT}/lib")
endif()

if (WITH_SAMPLES)
  include_directories("${PROJECT_SOURCE_DIR}/include"
                      "${MYSQL_INCLUDE_DIR}"
                      "${Boost_INCLUDE_DIRS}"
                      "${CMAKE_SOURCE_DIR}/utils"
                      "${CMAKE_SOURCE_DIR}"
                     )

  target_link_libraries(shell_client_lib mysqlsh)

  target_link_libraries(shell_client_lib ${GCOV_LDFLAGS})
  if ( HAVE_V8 )
    include_directories( ${V8_INCLUDE_DIR} )
    target_link_libraries(shell_client_lib ${V8_LINK_LIST})
  endif()
  if ( HAVE_PYTHON )
    include_directories(${PYTHON_INCLUDE_DIRS})
    target_link_libraries(shell_client_lib ${PYTHON_LIBRARIES})
  endif()

  # Last, to make -lrt -lpthread last, v8 needs it
  target_link_libraries(shell_client_lib ${MYSQL_LIBRARIES})
else()
  include_directories("${DEPLOY_DIR}/include")

  # ----------------------------------------------------------------------
  # Find V8, might be one or multiple libraries
  # ----------------------------------------------------------------------

  find_library(lib_v8 "v8" "${DEPLOY_DIR}/lib"
               PATHS "${DEPLOY_DIR}/lib"
               NO_DEFAULT_PATH
  )
  if(NOT lib_v8)
    set(lib_v8 "")
    # Could not find "libv8", in some distributions there are multiple "libv8_*"
    foreach(_libname v8_libplatform v8_base v8_libbase v8_snapshot)
      find_library(_v8_lib_${_libname} ${_libname}
                   PATHS ${LIBS_DIR}
                   NO_DEFAULT_PATH
      )
      if(NOT _v8_lib_${_libname})
        message(FATAL_ERROR "Could not find the library \"v8\" or \"${_libname}\"")
      endif()
      list(APPEND lib_v8 ${_v8_lib_${_libname}})
    endforeach()
  endif()

  message(STATUS "v8 Libraries Found: ${lib_v8}")

  # If not found, try system path and assume just one library
  find_library(lib_v8 "v8")

  # ----------------------------------------------------------------------
  # Common libraries for Windows and Unix
  # ----------------------------------------------------------------------

  find_library(lib_mysqlsh "mysqlsh" "${DEPLOY_DIR}/lib")
  find_library(lib_mysqlxtest "mysqlxtest" "${DEPLOY_DIR}/lib")
  find_library(lib_mysqlxtest_pb "mysqlxtest_pb" "${DEPLOY_DIR}/lib")

  # ----------------------------------------------------------------------
  # Find Windows/Unix specific libraries and set libraries to link with
  # ----------------------------------------------------------------------

  if(WIN32)

    find_library(lib_mysqlshtypes "mysqlshtypes" "${DEPLOY_DIR}/lib")

    target_link_libraries(shell_client_lib
      ${lib_mysqlsh}
      ${lib_mysqlshtypes}
      ${lib_mysqlxtest}
      ${lib_mysqlxtest_pb}
      ${lib_v8}
      "${MYSQL_CLIENT_LIB}")
  else()

    target_link_libraries(shell_client_lib
      ${lib_mysqlsh}
      ${lib_mysqlxtest}
      ${lib_mysqlxtest_pb}
      ${lib_v8}
      "${MYSQL_CLIENT_LIB}")
  endif()

endif()

install(TARGETS shell_client_lib DESTINATION lib COMPONENT dev)
