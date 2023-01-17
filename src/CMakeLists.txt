# Copyright (c) 2014, 2022, Oracle and/or its affiliates.
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

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")

include_directories(BEFORE "${CMAKE_SOURCE_DIR}/mysqlshdk/libs"
                    "${CMAKE_SOURCE_DIR}/mysqlshdk/include"
                    "${CMAKE_BINARY_DIR}/mysqlshdk/include"
                    "${CMAKE_SOURCE_DIR}"
                    "${CMAKE_SOURCE_DIR}/src"
                    "${CMAKE_SOURCE_DIR}/ext/linenoise-ng/include")

IF(HAVE_PYTHON)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})
ENDIF()

set(mysh_SRC
    mysqlsh/get_password.cc
    mysqlsh/cmdline_shell.cc
    mysqlsh/json_shell.cc
    mysqlsh/history.cc
    mysqlsh/mysql_shell.cc
    mysqlsh/prompt_renderer.cc
    mysqlsh/prompt_manager.cc
    mysqlsh/prompt_handler.cc
    mysqlsh/commands/command_edit.cc
    mysqlsh/commands/command_help.cc
    mysqlsh/commands/command_show.cc
    mysqlsh/commands/command_system.cc
    mysqlsh/commands/command_watch.cc
    ${CMAKE_SOURCE_DIR}/mysqlshdk/shellcore/interrupt_helper.cc
    ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/linenoise.cpp
    ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp
    ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/wcwidth.cpp
)

IF(CMAKE_COMPILER_IS_GNUCXX)
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp PROPERTIES COMPILE_FLAGS "-Wno-implicit-fallthrough")
ENDIF()

IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp PROPERTIES COMPILE_FLAGS "-Wno-keyword-macro")
ENDIF()

ADD_CONVENIENCE_LIBRARY(shellfe ${mysh_SRC})
ADD_STAN_TARGET(shellfe ${mysh_SRC})
target_link_libraries(shellfe ssh)

set(mysqlsh_SRC
  mysqlsh/main.cc
)

if(WIN32)
  # On Windows it is requred to include this source file on the list of sources
  # for the main project in order to:
  # - Define the icon to be used on the application.
  # - Define the version information to be shown while looking at the file
  # properties.
  SET(MYSH_FILE_TYPE "VFT_APP")
  SET(MYSH_PRODUCT_NAME "MySQL Shell")
  SET(MYSH_ORIGINAL_FILE_NAME "mysqlsh.exe")
  SET(MYSH_FILE_DESCRIPTION "MySQL Shell")
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/res/resource.rc.in   ${CMAKE_BINARY_DIR}/mysqlsh.rc @ONLY)
  set_source_files_properties("${SYSLOG_EVENT_RC}" PROPERTIES GENERATED TRUE)
  set(mysqlsh_SRC
      "${mysqlsh_SRC}"
      "${CMAKE_BINARY_DIR}/mysqlsh.rc"
      "${SYSLOG_EVENT_RC}"
      )
endif()

add_definitions(-DMYSH_COMMIT_ID="${MYSH_COMMIT_ID}")

add_shell_executable(mysqlsh "${mysqlsh_SRC}")
ADD_STAN_TARGET(mysqlsh mysqlsh/main.cc)

if(WIN32)
  add_dependencies(mysqlsh generate_syslog_event_rc)
endif()

set(mysqlsh_SRC
  ${mysqlsh_SRC}
  ${CMAKE_SOURCE_DIR}/unittest/test_utils/mod_testutils.cc
  mysqlsh/debug_shell.cc
)

# Last, to make -lrt -lpthread last, v8 needs it
target_link_libraries(mysqlsh
    shellfe
    api_modules
    mysqlshdk-static
    ssh
    ${GCOV_LDFLAGS}
    ${MYSQLX_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${MYSQL_EXTRA_LIBRARIES}
)

IF(WITH_TESTS)
  add_shell_executable(mysqlshrec "${mysqlsh_SRC}" TRUE)
  target_compile_definitions(mysqlshrec PRIVATE -DENABLE_SESSION_RECORDING)

  target_link_libraries(mysqlshrec
    shellfe
    api_modules
    mysqlshdk-static
    ${GCOV_LDFLAGS}
    ${MYSQLX_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${MYSQL_EXTRA_LIBRARIES}
  )
ENDIF()

### Bundling of OpenSSL libraries (if needed)
if(BUNDLED_OPENSSL)
  message(STATUS "Copying OpenSSL shared libraries")

  IF(WIN32)
    GET_FILENAME_COMPONENT(CRYPTO_NAME "${HAVE_CRYPTO_DLL}" NAME)
    GET_FILENAME_COMPONENT(OPENSSL_NAME "${HAVE_OPENSSL_DLL}" NAME)

    # Copy the OpenSSL libraries to the build dir (for running tests in build tree)
    ADD_CUSTOM_COMMAND(TARGET mysqlsh POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HAVE_CRYPTO_DLL}"
                  "$<TARGET_FILE_DIR:mysqlsh>/${CRYPTO_NAME}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HAVE_OPENSSL_DLL}"
                  "$<TARGET_FILE_DIR:mysqlsh>/${OPENSSL_NAME}")

    # Install/bundle the OpenSSL libraries
    install(PROGRAMS
      "${HAVE_CRYPTO_DLL}"
      "${HAVE_OPENSSL_DLL}"
      DESTINATION ${INSTALL_BINDIR}
      COMPONENT main
    )
  ELSE()
    # Copy the OpenSSL libraries to the build dir (for running tests in build tree)
    ADD_CUSTOM_COMMAND(TARGET mysqlsh POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CRYPTO_FULL_NAME}"
                  "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/${CRYPTO_VERSION}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENSSL_FULL_NAME}"
                  "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/${OPENSSL_VERSION}")

    # Install/bundle the OpenSSL libraries
    INSTALL(PROGRAMS
      "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/${CRYPTO_VERSION}"
      "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/${OPENSSL_VERSION}"
      DESTINATION ${INSTALL_LIBDIR}
      COMPONENT main
    )

    IF(APPLE)
      # Replace dependency "/Volumes/.../lib/libcrypto... of libssl
      EXECUTE_PROCESS(
        COMMAND otool -L "${OPENSSL_LIBRARY}"
        OUTPUT_VARIABLE OTOOL_OPENSSL_DEPS)
      STRING(REPLACE "\n" ";" DEPS_LIST ${OTOOL_OPENSSL_DEPS})
      FOREACH(LINE ${DEPS_LIST})
        STRING(REGEX MATCH "(/.*/lib/${CRYPTO_VERSION})" XXXXX ${LINE})
        IF(CMAKE_MATCH_1)
          SET(OPENSSL_DEPS "${CMAKE_MATCH_1}")
        ENDIF()
      ENDFOREACH()

      IF(OPENSSL_DEPS)
        # install_name_tool -change old new file
        ADD_CUSTOM_COMMAND(TARGET mysqlsh POST_BUILD
          COMMAND chmod +w "${CRYPTO_VERSION}" "${OPENSSL_VERSION}"
          COMMAND install_name_tool -change "${OPENSSL_DEPS}" "@loader_path/${CRYPTO_VERSION}" "${OPENSSL_VERSION}"
          WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}"
        )
      ENDIF()

      # include the soft links
      INSTALL(PROGRAMS
        "${CRYPTO_LIBRARY}"
        "${OPENSSL_LIBRARY}"
        DESTINATION ${INSTALL_LIBDIR}
        COMPONENT main
      )
    ENDIF()
  ENDIF()
endif()


### Bundling of V8 if needed
if(WITH_DEV)
    if(HAVE_V8 AND WIN32)
       install(PROGRAMS ${V8_LIBS} DESTINATION lib COMPONENT dev)
    endif()
endif()

### Bundling of Python runtime if needed
IF(HAVE_PYTHON)
  IF(PYTHON_DEPS)
    FILE(TO_CMAKE_PATH ${PYTHON_DEPS} PYTHON_DEPS_PATH)
    IF (APPLE)
      IF(BUNDLED_OPENSSL_DIR)
        FILE(GLOB_RECURSE OCI_OPEN_SSL_LIBRARIES ${PYTHON_DEPS_PATH}/cryptography/hazmat/bindings/_openssl*.so)
        SET_BUNDLED_OPEN_SSL(BINARIES ${OCI_OPEN_SSL_LIBRARIES})
      ENDIF()
    ENDIF()
  ENDIF()

  if(WIN32)
    remove_definitions(-DUNICODE)
    IF(PYTHON_LIBRARY_PATH)
      INSTALL(DIRECTORY "${PYTHON_LIBRARY_PATH}" DESTINATION "lib/Python${PYTHONLIBS_MAJOR_MINOR}" USE_SOURCE_PERMISSIONS COMPONENT main)
      INSTALL(DIRECTORY "${PYTHON_DLLS_PATH}" DESTINATION "lib/Python${PYTHONLIBS_MAJOR_MINOR}" USE_SOURCE_PERMISSIONS COMPONENT main)
      IF(PYTHON_DEPS)
        INSTALL(DIRECTORY "${PYTHON_DEPS_PATH}/" DESTINATION "lib/Python${PYTHONLIBS_MAJOR_MINOR}/Lib/site-packages" USE_SOURCE_PERMISSIONS COMPONENT main)
      ENDIF()
    ENDIF()

    # This should be there for both MD and non MD windows builds
    set(PY_ROOT ${PYTHON_LIBRARIES})
    get_filename_component(PY_ROOT ${PY_ROOT} DIRECTORY)
    get_filename_component(PY_ROOT ${PY_ROOT} DIRECTORY)

    file(GLOB PYTHON_DLLS "${PY_ROOT}/python*.dll")
    foreach(PY_DLL IN LISTS PYTHON_DLLS)
      install(PROGRAMS "${PY_DLL}" DESTINATION bin COMPONENT main)

      # Copies the dll to the binary location to enable debugging
      add_custom_command(TARGET mysqlsh PRE_BUILD
                          COMMAND ${CMAKE_COMMAND} -E copy
                          "${PY_DLL}" $<TARGET_FILE_DIR:mysqlsh>)
    endforeach()

                        # Copy the Python runtime
    add_custom_command(TARGET mysqlsh PRE_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_directory
                      "${PYTHON_LIBRARY_PATH}" $<TARGET_FILE_DIR:mysqlsh>/../lib/Python${PYTHONLIBS_MAJOR_MINOR}/Lib)
    add_custom_command(TARGET mysqlsh PRE_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_directory
                      "${PYTHON_DLLS_PATH}" $<TARGET_FILE_DIR:mysqlsh>/../lib/Python${PYTHONLIBS_MAJOR_MINOR}/DLLs)
    # Copy the Python dependencies
    IF(PYTHON_DEPS)
      add_custom_command(TARGET mysqlsh PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${PYTHON_DEPS_PATH}/"
                        $<TARGET_FILE_DIR:mysqlsh>/../lib/Python${PYTHONLIBS_MAJOR_MINOR}/Lib/site-packages)
    ENDIF()
    file(GLOB PYTHON_ADDITIONAL_DLLS
      "${PY_ROOT}/libffi.dll"
      "${PY_ROOT}/sqlite3.dll")

    foreach(PY_ADDITIONAL_DLL IN LISTS PYTHON_ADDITIONAL_DLLS)
      install(PROGRAMS "${PY_ADDITIONAL_DLL}" DESTINATION lib/Python${PYTHONLIBS_MAJOR_MINOR}/DLLs COMPONENT main)

      # Copies the dll to the binary location to enable debugging
      add_custom_command(TARGET mysqlsh PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PY_ADDITIONAL_DLL}" $<TARGET_FILE_DIR:mysqlsh>/../lib/Python${PYTHONLIBS_MAJOR_MINOR}/DLLs)
    endforeach()
  elseif(BUNDLED_PYTHON_DIR)
    GET_FILENAME_COMPONENT(PYTHON_INCLUDE_DIR_NAME "${PYTHON_INCLUDE_DIRS}" NAME)
    INSTALL(DIRECTORY "${BUNDLED_PYTHON_DIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}" DESTINATION "${INSTALL_LIBDIR}/lib" USE_SOURCE_PERMISSIONS COMPONENT main
              PATTERN "lib/python${PYTHONLIBS_MAJOR_MINOR}/test" EXCLUDE)
    INSTALL(FILES "${BUNDLED_PYTHON_DIR}/include/${PYTHON_INCLUDE_DIR_NAME}/pyconfig.h"
            DESTINATION "${INSTALL_LIBDIR}/include/${PYTHON_INCLUDE_DIR_NAME}" COMPONENT main)
    IF(PYTHON_DEPS)
      INSTALL(DIRECTORY "${PYTHON_DEPS_PATH}/"
        DESTINATION "${INSTALL_LIBDIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}/site-packages" USE_SOURCE_PERMISSIONS COMPONENT main)
    ENDIF()

    IF(APPLE)
      FILE(GLOB PYTHON_OPEN_SSL_LIBRARIES ${BUNDLED_PYTHON_DIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}/lib-dynload/_ssl*.so)
      SET_BUNDLED_OPEN_SSL(BINARIES ${PYTHON_OPEN_SSL_LIBRARIES})
    ENDIF()

    # Copy to build dir to allow testing
    add_custom_command(TARGET mysqlsh POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_directory
                      "${BUNDLED_PYTHON_DIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}"
                      "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}")
    IF(PYTHON_DEPS)
      add_custom_command(TARGET mysqlsh PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${PYTHON_DEPS_PATH}/"
                        "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}/site-packages")
    ENDIF()


    if(BUNDLED_SHARED_PYTHON)
      if(APPLE)
        set(_PYTHON_SHARED_LIB "${PYTHON_LIBRARIES}")
      else()
        file(GLOB _PYTHON_SHARED_LIB "${PYTHON_LIBRARIES}.*")
      endif()

      INSTALL(PROGRAMS
        "${_PYTHON_SHARED_LIB}"
        DESTINATION ${INSTALL_LIBDIR}
        COMPONENT main
      )

      if(STRIP_LIBCRYPT_DEPS)
        foreach(_PYTHON_LIB IN LISTS _PYTHON_SHARED_LIB)
          execute_patchelf(--remove-needed libcrypt.so.1 "${_PYTHON_LIB}")
        endforeach()
      endif()

      ADD_CUSTOM_COMMAND(TARGET mysqlsh POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${_PYTHON_SHARED_LIB}"
                  "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/.")
    endif()
  else()
    IF(PYTHON_DEPS)
      INSTALL(DIRECTORY ${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/lib
        DESTINATION "${INSTALL_LIBDIR}"
        USE_SOURCE_PERMISSIONS
        COMPONENT main)

      add_custom_command(TARGET mysqlsh PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${PYTHON_DEPS_PATH}"
                        "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}/site-packages")
    ENDIF()
  endif()
ENDIF(HAVE_PYTHON)

IF(BUNDLED_SSH_DIR)
  GET_TARGET_PROPERTY(_SSH_LIBRARY_LOCATION ssh LOCATION)
  GET_FILENAME_COMPONENT(_SSH_LIBRARY_DIR ${_SSH_LIBRARY_LOCATION} DIRECTORY)

  IF(WIN32)
    FILE(GLOB_RECURSE _SSH_LIBRARIES "${_SSH_LIBRARY_DIR}/*.dll")
  ELSEIF(APPLE)
    FILE(GLOB_RECURSE _SSH_LIBRARIES "${_SSH_LIBRARY_DIR}/*.dylib")
  ELSE()
    FILE(GLOB_RECURSE _SSH_LIBRARIES "${_SSH_LIBRARY_DIR}/*.so*")
  ENDIF()

  IF(NOT _SSH_LIBRARIES)
    MESSAGE(FATAL_ERROR "Could not find the bundled libssh libraries")
  ENDIF()

  install_bundled_binaries(BINARIES ${_SSH_LIBRARIES} DESTINATION "${INSTALL_LIBDIR}" TARGET mysqlsh)
ENDIF()

# krb5 is an indirect dependency for libssh, platforms that bundle libssh should may optionally
# require a custom built version of the krb5 library to be also bundled in the shell packages
# to prevent the shell from having different openssl dependencies (system one/bundled one)
IF(BUNDLED_KRB5_DIR)
  IF(WIN32)
    FILE(GLOB _KRB5_LIBRARIES "${BUNDLED_KRB5_DIR}/bin/*.dll")
  ELSE()
    FILE(GLOB _KRB5_LIBRARIES "${BUNDLED_KRB5_DIR}/lib/lib*")
  ENDIF()

  IF(NOT _KRB5_LIBRARIES)
    MESSAGE(FATAL_ERROR "Could not find the bundled krb5 libraries")
  ENDIF()

  install_bundled_binaries(BINARIES ${_KRB5_LIBRARIES} DESTINATION "${INSTALL_LIBDIR}" TARGET mysqlsh)
ENDIF()

IF(BUNDLED_SASL_DIR)
  IF(WIN32)
    FILE(GLOB _SASL_LIBRARIES "${BUNDLED_SASL_DIR}/lib/lib*.dll")
    FILE(GLOB _SASL_PLUGINS "${BUNDLED_SASL_DIR}/lib/sasl*.dll")
  ELSEIF(APPLE)
    FILE(GLOB _SASL_LIBRARIES "${BUNDLED_SASL_DIR}/lib/libsasl2*.dylib")
    # plugins have .so extension
    FILE(GLOB _SASL_PLUGINS "${BUNDLED_SASL_DIR}/lib/sasl2/lib*.so")
  ELSE()
    FILE(GLOB _SASL_LIBRARIES "${BUNDLED_SASL_DIR}/lib/libsasl2.so*")
    FILE(GLOB _SASL_PLUGINS "${BUNDLED_SASL_DIR}/lib/sasl2/lib*.so*")
  ENDIF()

  IF(NOT _SASL_LIBRARIES)
    MESSAGE(FATAL_ERROR "Could not find the bundled sasl libraries")
  ENDIF()

  install_bundled_binaries(BINARIES ${_SASL_LIBRARIES} DESTINATION "${INSTALL_LIBDIR}" TARGET mysqlsh)
  install_bundled_binaries(BINARIES ${_SASL_PLUGINS} DESTINATION "${INSTALL_LIBDIR}/sasl2" TARGET mysqlsh)
ENDIF()

IF(BUNDLED_ANTLR_DIR)
  if(WIN32)
    file(GLOB_RECURSE _ANTLR4_SHARED_LIB "${ANTLR4_LIB_BASE_DIRECTORY}/*.dll")
  elseif(APPLE)
    file(GLOB_RECURSE _ANTLR4_SHARED_LIB "${ANTLR4_LIB_BASE_DIRECTORY}/*.dylib")
  else()
    file(GLOB _ANTLR4_SHARED_LIB "${ANTLR4_LIBRARY}.*")
  endif()

  IF(WIN32)
    INSTALL(PROGRAMS
      "${_ANTLR4_SHARED_LIB}"
      DESTINATION ${INSTALL_BINDIR}
      COMPONENT main
    )

    # Copy the libraries to the build dir (for running tests in build tree)
    ADD_CUSTOM_COMMAND(TARGET mysqlsh POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_ANTLR4_SHARED_LIB}"
                  "$<TARGET_FILE_DIR:mysqlsh>/.")
  ELSE()
    foreach(CURRENT_ANTLR4_SHARED_LIB ${_ANTLR4_SHARED_LIB})
       INSTALL(PROGRAMS
        "${CURRENT_ANTLR4_SHARED_LIB}"
        DESTINATION ${INSTALL_LIBDIR}
        COMPONENT main
      )

      ADD_CUSTOM_COMMAND(TARGET mysqlsh POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CURRENT_ANTLR4_SHARED_LIB}"
                  "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/.")
    endforeach()
  ENDIF()
ENDIF()

# If defined, installs the client authentication plugins
IF(AUTH_CLIENT_PLUGINS)
  IF(AUTH_CLIENT_PLUGINS_DEPS)
    install_bundled_binaries(BINARIES ${AUTH_CLIENT_PLUGINS_DEPS} DESTINATION "${INSTALL_LIBDIR}" TARGET mysqlsh)
  ENDIF()

  install_bundled_binaries(BINARIES ${AUTH_CLIENT_PLUGINS} DESTINATION "${INSTALL_MYSQL_PLUGINS_DIR}" TARGET mysqlsh)
ELSE()
  MESSAGE(WARNING "Could not find Authentication Client Plugins!")
ENDIF()
