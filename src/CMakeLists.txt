# Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

include_directories(BEFORE "${CMAKE_SOURCE_DIR}/mysqlshdk/libs"
                    "${CMAKE_SOURCE_DIR}/mysqlshdk/include"
                    "${CMAKE_BINARY_DIR}/mysqlshdk/include"
                    "${CMAKE_SOURCE_DIR}"
                    "${CMAKE_SOURCE_DIR}/src"
                    "${CMAKE_SOURCE_DIR}/ext/linenoise-ng/include"
                    "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")

set(mysh_SRC
    mysqlsh/main.cc
    mysqlsh/get_password.cc
    mysqlsh/cmdline_shell.cc
    mysqlsh/history.cc
    mysqlsh/mysql_shell.cc
    mysqlsh/prompt_renderer.cc
    mysqlsh/prompt_manager.cc
    interactive/interactive_dba_cluster.cc
    interactive/interactive_global_dba.cc
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

add_executable(mysqlshrec ${mysh_SRC})
set_target_properties(mysqlshrec PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_compile_definitions(mysqlshrec PRIVATE -DENABLE_SESSION_RECORDING)

ADD_STAN_TARGET(mysqlsh ${mysh_SRC})

# Last, to make -lrt -lpthread last, v8 needs it
target_link_libraries(mysqlsh
    api_modules
    mysqlshdk-static
    ${GCOV_LDFLAGS}
    ${MYSQLX_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${MYSQL_EXTRA_LIBRARIES}
)

target_link_libraries(mysqlshrec
    api_modules
    mysqlshdk-static
    ${GCOV_LDFLAGS}
    ${MYSQLX_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${MYSQL_EXTRA_LIBRARIES}
)

install(TARGETS mysqlsh RUNTIME COMPONENT main DESTINATION ${INSTALL_BINDIR})

if(WIN32 OR APPLE)

  # In MySQL Server 8.0.4 and up, OpenSSL is linked dynamically and
  # preferably linked against the system installed OpenSSL. But there
  # is no standard system installed OpenSSL on macOS or Windows so we
  # bundle the library copying it from the server install we link
  # against.

  set(OPENSSL_TO_BUNDLE_DIR)

  if(WIN32)

    if(EXISTS "${MYSQL_BUILD_DIR}/runtime_output_directory/RelWithDebInfo/ssleay32.dll")
      set(OPENSSL_TO_BUNDLE_DIR "${MYSQL_BUILD_DIR}/runtime_output_directory/RelWithDebInfo")
    elseif(WITH_SSL AND EXISTS "${WITH_SSL}/lib/ssleay32.dll")
      set(OPENSSL_TO_BUNDLE_DIR "${WITH_SSL}/lib")
    endif()

    if(OPENSSL_TO_BUNDLE_DIR)
      message(STATUS "Copying OpenSSL shared libraries from ${OPENSSL_TO_BUNDLE_DIR}")
      install(FILES
        "${OPENSSL_TO_BUNDLE_DIR}/ssleay32.dll"
        "${OPENSSL_TO_BUNDLE_DIR}/libeay32.dll"
        DESTINATION ${INSTALL_BINDIR}
        COMPONENT main
      )
    endif()

  elseif(APPLE)

    if(EXISTS "${MYSQL_BUILD_DIR}/library_output_directory/libssl.dylib")
      set(OPENSSL_TO_BUNDLE_DIR "${MYSQL_BUILD_DIR}/library_output_directory")
    elseif(WITH_SSL AND EXISTS "${WITH_SSL}/lib/libssl.dylib")
      set(OPENSSL_TO_BUNDLE_DIR "${WITH_SSL}/lib")
    endif()

    if(OPENSSL_TO_BUNDLE_DIR)
      message(STATUS "Copying OpenSSL shared libraries from ${OPENSSL_TO_BUNDLE_DIR}")

      # Find the "version" from the file name, it is the same
      file(GLOB CRYPTO_VERSION  RELATIVE ${OPENSSL_TO_BUNDLE_DIR} "${OPENSSL_TO_BUNDLE_DIR}/libcrypto.*.*.*.dylib")
      file(GLOB OPENSSL_VERSION RELATIVE ${OPENSSL_TO_BUNDLE_DIR} "${OPENSSL_TO_BUNDLE_DIR}/libssl.*.*.*.dylib")

      # Install/bundle the OpenSSL libraries including the soft links
      install(FILES
        "${OPENSSL_TO_BUNDLE_DIR}/libcrypto.dylib"
        "${OPENSSL_TO_BUNDLE_DIR}/libssl.dylib"
        "${OPENSSL_TO_BUNDLE_DIR}/${CRYPTO_VERSION}"
        "${OPENSSL_TO_BUNDLE_DIR}/${OPENSSL_VERSION}"
        DESTINATION ${INSTALL_LIBDIR}
        COMPONENT main
      )

      # We will not pollute "/usr/lib/", OpenSSL libraries ends up in "/usr/lib/mysqlsh/"
      add_custom_command(TARGET mysqlsh POST_BUILD
        COMMAND install_name_tool -change
                "${CRYPTO_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${CRYPTO_VERSION}"
                $<TARGET_FILE:mysqlsh>
        COMMAND install_name_tool -change
                "${OPENSSL_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${OPENSSL_VERSION}"
                $<TARGET_FILE:mysqlsh>
      )

    endif()

  endif()

endif()


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
