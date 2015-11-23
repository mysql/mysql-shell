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
      "shell_client.cc"
      "shell_client.h"
      "${CMAKE_SOURCE_DIR}/src/boost_code.cc"
)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/src/boost_code.cc PROPERTIES COMPILE_FLAGS -O0)
endif ()

if (WIN32 AND WINDOWS_RUNTIME_MD)
  add_library(shell_client SHARED ${lib_SRC})
else()
  add_library(shell_client ${lib_SRC})
endif()

add_definitions(-DSHELL_CLIENT_NATIVE_EXPORTS)
add_definitions(-DBOOST_ALL_NO_LIB)

#if(WIN32 AND NOT WINDOWS_RUNTIME_MD)
#  add_definitions(-DPy_NO_ENABLE_SHARED)
#endif()


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

  # ----------------------------------------------------------------------
  # Find V8, might be one or multiple libraries
  # ----------------------------------------------------------------------

IF (HAVE_V8)
  include_directories( ${V8_INCLUDE_DIR} )

  find_library(V8_LIB v8
               PATHS ${V8_LIB_DIR}
               NO_DEFAULT_PATH
  )
  set(V8_LIBS)
  if(V8_LIB)
    set(V8_LIBS ${V8_LIB})      # Single lib, just use it
    message(STATUS "v8 Library Found: \"${_libname}\" at ${V8_LIB}")
  else()
    # Could not find "libv8", in some distributions there are multiple "libv8_*"
    foreach(_libname v8_libplatform v8_base v8_libbase v8_snapshot)
      find_library(_v8_lib ${_libname}
                   PATHS ${V8_LIB_DIR}
                   NO_DEFAULT_PATH
      )
      if(NOT _v8_lib)
        message(FATAL_ERROR "Could not find the library \"v8\" or \"${_libname}\"")
      else()
        message(STATUS "v8 Library Found: \"${_libname}\" at ${_v8_lib}")
      endif()
      list(APPEND V8_LIBS ${_v8_lib})
      SET(_v8_lib _v8_lib-NOTFOUND)
    endforeach()
  endif()

  if(WIN32)
    set(V8_LINK_LIST ${V8_LIBS} winmm)
  else()	# Linux
    set(V8_LINK_LIST ${V8_LIBS})
  endif()
ENDIF()

include_directories(${MYSQL_INCLUDE_DIR}
                    "${Boost_INCLUDE_DIRS}"
                    "${CMAKE_SOURCE_DIR}"
                    "${CMAKE_SOURCE_DIR}/common"
                    "${CMAKE_SOURCE_DIR}/ext/rapidjson/include"
                   )
IF (HAVE_PYTHON)
  include_directories(${PYTHON_INCLUDE_DIRS})
ENDIF()

if(WIN32)
  if(NOT WINDOWS_RUNTIME_MD)
		CHANGE_MD_2_MT()
    add_dependencies(shell_client boost_code.obj)
  endif()
else()
  target_link_libraries(shell_client edit)
endif()

# Last, to make -lrt -lpthread last, v8 needs it
target_link_libraries(shell_client mysqlsh ${GCOV_LDFLAGS})


# Builds the managed library only if required
if(WIN32 AND WINDOWS_RUNTIME_MD)
	add_subdirectory(managed)
endif()

if (WITH_SAMPLES)
	add_subdirectory(sample)
endif()

install(FILES "shell_client.h" COMPONENT dev DESTINATION include)
install(TARGETS shell_client COMPONENT dev DESTINATION lib)
