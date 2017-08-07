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

include_directories(BEFORE "${CMAKE_SOURCE_DIR}/mysqlshdk/libs"
                    "${CMAKE_SOURCE_DIR}/mysqlshdk/include"
                    "${CMAKE_SOURCE_DIR}"
                    "${CMAKE_SOURCE_DIR}/src"
                    "${CMAKE_SOURCE_DIR}/ext/linenoise-ng/include"
                    "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")

set(mysh_SRC
    mysqlsh/main.cc
    mysqlsh/get_password.cc
    mysqlsh/shell_cmdline_options.cc
    mysqlsh/cmdline_shell.cc
    mysqlsh/history.cc
    mysqlsh/mysql_shell.cc
    mysqlsh/prompt_renderer.cc
    mysqlsh/prompt_manager.cc
    interactive/interactive_dba_cluster.cc
    interactive/interactive_global_dba.cc
    interactive/interactive_global_schema.cc
    interactive/interactive_global_session.cc
    interactive/interactive_global_shell.cc
    ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/linenoise.cpp
    ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp
    ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/wcwidth.cpp
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

add_executable(mysqlsh ${mysh_SRC})
set_target_properties(mysqlsh PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

ADD_STAN_TARGET(mysqlsh ${mysh_SRC})

# Last, to make -lrt -lpthread last, v8 needs it
target_link_libraries(mysqlsh
    api_modules
    mysqlshdk-static
    ${GCOV_LDFLAGS})

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
    if (WITH_DEV)
      install(FILES "${PY_DLL}/${PY_FILE}" DESTINATION bin COMPONENT dev)
    endif()
  endif()
endif()
