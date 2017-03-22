# Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

if(NOT WIN32)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/libedit")
endif()
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/mysqlshdk/libs"
                    "${CMAKE_SOURCE_DIR}/mysqlshdk/include"
                    "${CMAKE_SOURCE_DIR}/common"
                    "${CMAKE_SOURCE_DIR}"
                    "${CMAKE_SOURCE_DIR}/src"
                    "${CMAKE_SOURCE_DIR}/ext/rapidjson/include"
                    "${CMAKE_SOURCE_DIR}/src/mysqlsh/libedit")

SET(BOOST_SOURCE
"
#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/system/error_code.hpp>
int main()
{
 boost::system::error_code error;
 return error.value();
}
")


SET(BOOST_SYSTEM_COMPILES "BOOST_SYSTEM_COMPILES")

CHECK_CXX_SOURCE_COMPILES("${BOOST_SOURCE}" BOOST_SYSTEM_COMPILES)

if (BOOST_SYSTEM_COMPILES)
  set(BOOST_SOURCE_CODE "${PROJECT_SOURCE_DIR}/src/mysqlsh/boost_code.cc")
else()
  message(STATUS "Boost requires linkage")
  set(BOOST_LIBRARIES boost_system)
endif()

# disable optimization for boost error_code, otherwise we get link errors in Release builds
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET_SOURCE_FILES_PROPERTIES(${PROJECT_SOURCE_DIR}/src/shell/mysqlsh/boost_code.cc PROPERTIES COMPILE_FLAGS -O0)
endif ()


set(mysh_SRC
    mysqlsh/main.cc
    mysqlsh/get_password.cc
    mysqlsh/shell_cmdline_options.cc
    mysqlsh/cmdline_shell.cc
    interactive/interactive_dba_cluster.cc
    interactive/interactive_global_dba.cc
    interactive/interactive_global_schema.cc
    interactive/interactive_global_session.cc
    interactive/interactive_global_shell.cc
)

if(WIN32)
  SET(MYSH_FILE_TYPE "VFT_APP")
  SET(MYSH_PRODUCT_NAME "MySQL Shell")
  SET(MYSH_ORIGINAL_FILE_NAME "mysqlsh.exe")
  SET(MYSH_FILE_DESCRIPTION "MySQL Shell")
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/res/resource.rc.in   ${CMAKE_BINARY_DIR}/mysqlsh.rc @ONLY)
  set(mysh_SRC
      "${mysh_SRC}"
      "${CMAKE_BINARY_DIR}/mysqlsh.rc"
      )
endif()

add_executable(mysqlsh ${mysh_SRC} ${BOOST_SOURCE_CODE})
set_target_properties(mysqlsh PROPERTIES OUTPUT_NAME ${CMAKE_BINARY_DIR}/mysqlsh)

if(WIN32)
  if(NOT WINDOWS_RUNTIME_MD)
    add_dependencies(mysqlsh boost_code.obj)
  endif()
else()
  target_link_libraries(mysqlsh edit)
endif()

# Last, to make -lrt -lpthread last, v8 needs it
target_link_libraries(mysqlsh api_modules mysqlshdk-static ${GCOV_LDFLAGS})

install(TARGETS mysqlsh RUNTIME COMPONENT main DESTINATION ${INSTALL_BINDIR})

if(WIN32)
  remove_definitions(-DUNICODE)

  if(HAVE_PYTHON)
    install(DIRECTORY "${PYTHON_LIBRARY_PATH}" DESTINATION "lib/Python2.7" COMPONENT main)
    install(DIRECTORY "${PYTHON_DLLS_PATH}" DESTINATION "lib/Python2.7" COMPONENT main)

    set(PY_DLL ${PYTHON_LIBRARIES})
    string(REPLACE ".lib" ".dll" PY_DLL ${PY_DLL})
    get_filename_component(PY_FILE ${PY_DLL} NAME)
    get_filename_component(PY_DLL ${PY_DLL} DIRECTORY)
    get_filename_component(PY_DLL ${PY_DLL} DIRECTORY)
    install(FILES "${PY_DLL}/${PY_FILE}" DESTINATION bin COMPONENT main)
  endif()
endif()
