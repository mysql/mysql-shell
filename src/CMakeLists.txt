# Copyright (c) 2014, 2025, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
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
    mysqlsh/commands/command_query_attributes.cc
    ${CMAKE_SOURCE_DIR}/mysqlshdk/shellcore/interrupt_helper.cc
    ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/linenoise.cpp
    ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/ConvertUTF.cpp
    ${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/wcwidth.cpp
)

IF(CMAKE_COMPILER_IS_GNUCXX)
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/ext/linenoise-ng/src/linenoise.cpp PROPERTIES COMPILE_FLAGS "-Wno-stringop-overflow")
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
  generate_rc_file(NAME "mysqlsh.exe" DESCRIPTION "Advanced client and code editor for MySQL." OUT_RC_FILE RC_FILE)
  set(mysqlsh_SRC
      "${mysqlsh_SRC}"
      "${RC_FILE}"
  )

  # syslog event
  set_source_files_properties("${SYSLOG_EVENT_RC}" PROPERTIES GENERATED TRUE)
  set(mysqlsh_SRC
      "${mysqlsh_SRC}"
      "${SYSLOG_EVENT_RC}"
  )
endif()

add_definitions(-DMYSH_COMMIT_ID="${MYSH_COMMIT_ID}")
add_definitions(-DMYSH_PRODUCT_ID="${MYSH_PRODUCT_ID}")

add_shell_executable(mysqlsh "${mysqlsh_SRC}")
ADD_STAN_TARGET(mysqlsh mysqlsh/main.cc)

if(WIN32)
  add_dependencies(mysqlsh generate_syslog_event_files)
endif()

set(mysqlsh_SRC
  ${mysqlsh_SRC}
  ${CMAKE_SOURCE_DIR}/unittest/test_utils/mod_testutils.cc
  mysqlsh/debug_shell.cc
)

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
  IF(NOT HAVE_PYTHON)
    message(FATAL_ERROR "Building test support (i.e.: mysqlshrec) requires Python support enabled.")
  ENDIF()

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
  IF(WIN32)
    SET(OPENSSL_LIBS "${HAVE_CRYPTO_DLL}" "${HAVE_OPENSSL_DLL}")
  ELSE()
    SET(OPENSSL_LIBS "${CRYPTO_FULL_NAME}" "${OPENSSL_FULL_NAME}" "${CRYPTO_LIBRARY}" "${OPENSSL_LIBRARY}")
  ENDIF()

  install_bundled_binaries(
      DESCRIPTION "OpenSSL libraries"
      BINARIES ${OPENSSL_LIBS}
      DESTINATION "${INSTALL_LIBDIR}"
      TARGET mysqlsh
  )

  IF(OPENSSL_MODULES_DIR)
    install_bundled_binaries(
        DESCRIPTION "OpenSSL modules"
        BINARIES ${OPENSSL_MODULES_LIST}
        DESTINATION "${INSTALL_LIBDIR}/ossl-modules"
        USE_LIBDIR
        TARGET mysqlsh
    )
  ENDIF()
endif()


### Bundling of Python runtime if needed
IF(HAVE_PYTHON)
  if(WIN32)
    remove_definitions(-DUNICODE)

    install_bundled_binaries(
        DESCRIPTION "Python executable"
        BINARIES "${PYTHON_EXECUTABLE}"
        DESTINATION "lib/Python${PYTHONLIBS_MAJOR_MINOR}"
        TARGET mysqlsh
    )

    file(GLOB PYTHON_DLLS "${PYTHON_ROOT}/python*.dll")
    install_bundled_binaries(
        DESCRIPTION "Python dlls"
        BINARIES ${PYTHON_DLLS}
        DESTINATION "bin"
        TARGET mysqlsh
    )

    # GLOB is used here, because some of these files may not exist
    file(GLOB PYTHON_ADDITIONAL_DLLS
        "${PYTHON_ROOT}/libffi.dll"
        "${PYTHON_ROOT}/sqlite3.dll"
        "${PYTHON_ROOT}/psqlite3.dll"
    )
    install_bundled_binaries(
        DESCRIPTION "additional Python dlls"
        BINARIES ${PYTHON_ADDITIONAL_DLLS}
        DESTINATION "lib/Python${PYTHONLIBS_MAJOR_MINOR}/DLLs"
        TARGET mysqlsh
    )

    # Python runtime
    install_bundled_directory(
        DESCRIPTION "Python library"
        DIRECTORY "${PYTHON_LIBRARY_PATH}"
        DESTINATION "lib/Python${PYTHONLIBS_MAJOR_MINOR}"
        TARGET mysqlsh
    )
    install_bundled_directory(
        DESCRIPTION "Python library dlls"
        DIRECTORY "${PYTHON_DLLS_PATH}"
        DESTINATION "lib/Python${PYTHONLIBS_MAJOR_MINOR}"
        TARGET mysqlsh
    )

    # Python dependencies
    IF(PYTHON_DEPS)
      # no need to install this, this is a subdirectory of the Python library directory
      install_bundled_directory(
          DESCRIPTION "extra Python dependencies"
          COPY_ONLY
          CONTENTS "${PYTHON_DEPS}"
          DESTINATION "lib/Python${PYTHONLIBS_MAJOR_MINOR}/Lib/site-packages"
          TARGET mysqlsh
      )
    ENDIF()
  elseif(BUNDLED_PYTHON_DIR)
    set(WRITE_RPATH TRUE)
    set(EXTRA_COPY_COMMANDS "")

    if(BUNDLED_SHARED_PYTHON)
      if(APPLE)
        set(_PYTHON_SHARED_LIB "${PYTHON_LIBRARIES}")

        # add rpath to allow python executable to import dylibs
        set(WRITE_RPATH FALSE)
        set(EXTRA_COPY_COMMANDS COMMAND install_name_tool -add_rpath "@executable_path/.." "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/bin/python${PYTHONLIBS_MAJOR_MINOR}")
      else()
        file(GLOB _PYTHON_SHARED_LIB "${PYTHON_LIBRARIES}.*")

        if(STRIP_LIBCRYPT_DEPS)
          get_filename_component(_PYTHON_SHARED_LIB_NAME "${_PYTHON_SHARED_LIB}" NAME)
          get_patchelf_command(ARGS --remove-needed libcrypt.so.1 "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/${_PYTHON_SHARED_LIB_NAME}" OUT_COMMAND COMMAND)
          set(LIB_EXTRA_COPY_COMMANDS COMMAND ${COMMAND})
        endif()
      endif()

      install_bundled_binaries(
          DESCRIPTION "Python shared library"
          BINARIES "${_PYTHON_SHARED_LIB}"
          DESTINATION "${INSTALL_LIBDIR}"
          TARGET mysqlsh
          WRITE_RPATH FALSE
          EXTRA_COPY_COMMANDS ${LIB_EXTRA_COPY_COMMANDS}
      )
    endif()

    install_bundled_binaries(
        DESCRIPTION "Python executable"
        BINARIES "${PYTHON_EXECUTABLE}"
        DESTINATION "${INSTALL_LIBDIR}/bin"
        TARGET mysqlsh
        WRITE_RPATH ${WRITE_RPATH}
        EXTRA_COPY_COMMANDS ${EXTRA_COPY_COMMANDS}
    )

    # Python headers
    install(DIRECTORY "${BUNDLED_PYTHON_DIR}/include/python${PYTHONLIBS_MAJOR_MINOR}" DESTINATION "${INSTALL_LIBDIR}/include" COMPONENT main)

    # Python runtime
    set(EXTRA_COPY_COMMANDS "")

    if(APPLE)
      get_use_bundled_openssl_command(PATTERN "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}/lib-dynload/_[sh]*.so" OUT_COMMAND EXTRA_COPY_COMMANDS)
      list(INSERT EXTRA_COPY_COMMANDS 0 COMMAND)
    endif()

    install_bundled_directory(
        DESCRIPTION "Python library"
        DIRECTORY "${BUNDLED_PYTHON_DIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}"
        DESTINATION "${INSTALL_LIBDIR}/lib"
        TARGET mysqlsh
        EXTRA_COPY_COMMANDS ${EXTRA_COPY_COMMANDS}
        EXTRA_INSTALL_COMMANDS PATTERN "lib/python${PYTHONLIBS_MAJOR_MINOR}/test" EXCLUDE
    )

    # Python dependencies
    IF(PYTHON_DEPS)
      set(EXTRA_COPY_COMMANDS "")

      if(APPLE)
        get_use_bundled_openssl_command(PATTERN "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}/site-packages/*.so" OUT_COMMAND EXTRA_COPY_COMMANDS)
        list(INSERT EXTRA_COPY_COMMANDS 0 COMMAND)
     endif()

      # no need to install this, this is a subdirectory of the Python library directory
      install_bundled_directory(
          DESCRIPTION "extra Python dependencies"
          COPY_ONLY
          CONTENTS "${PYTHON_DEPS}"
          DESTINATION "${INSTALL_LIBDIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}/site-packages"
          TARGET mysqlsh
          EXTRA_COPY_COMMANDS ${EXTRA_COPY_COMMANDS}
      )
    ENDIF()
  else()
    # Python dependencies
    IF(PYTHON_DEPS)
      install_bundled_directory(
          DESCRIPTION "extra Python dependencies"
          CONTENTS "${PYTHON_DEPS}"
          DESTINATION "${INSTALL_LIBDIR}/lib/python${PYTHONLIBS_MAJOR_MINOR}/site-packages"
          TARGET mysqlsh
      )
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

  install_bundled_binaries(
      DESCRIPTION "SSH libraries"
      BINARIES ${_SSH_LIBRARIES}
      DESTINATION "${INSTALL_LIBDIR}"
      TARGET mysqlsh
  )
ENDIF()

# krb5 is an indirect dependency for libssh, platforms that bundle libssh should may optionally
# require a custom built version of the krb5 library to be also bundled in the shell packages
# to prevent the shell from having different openssl dependencies (system one/bundled one)
IF(BUNDLED_KRB5_DIR)
  IF(WIN32)
    SET(_KRB5_LIB_DIR "${BUNDLED_KRB5_DIR}/bin")
    SET(_KRB5_BUNDLED_LIBS
      "krb5_64.dll"
      "k5sprt64.dll"
      "comerr64.dll"
      "xpprof64.dll"
      "krbcc64.dll"
      "gssapi64.dll"
    )
  ELSE()
    SET(_KRB5_LIB_DIR "${BUNDLED_KRB5_DIR}/lib")
    SET(_KRB5_BUNDLED_LIBS
      "libkrb5.*"
      "libcom_err.*"
      "libgssapi_krb5.*"
      "libk5crypto.*"
      "libkrb5support.*"
    )
  ENDIF()

  SET(_KRB5_LIBRARIES "")

  FOREACH(_pattern ${_KRB5_BUNDLED_LIBS})
    FILE(GLOB _libs "${_KRB5_LIB_DIR}/${_pattern}")

    IF(NOT _libs)
      MESSAGE(FATAL_ERROR "Could not find the bundled krb5 library: ${_pattern}")
    ENDIF()

    LIST(APPEND _KRB5_LIBRARIES ${_libs})
  ENDFOREACH()

  install_bundled_binaries(
      DESCRIPTION "Kerberos libraries"
      BINARIES ${_KRB5_LIBRARIES}
      DESTINATION "${INSTALL_LIBDIR}"
      TARGET mysqlsh
  )
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

  install_bundled_binaries(
      DESCRIPTION "SASL libraries"
      BINARIES ${_SASL_LIBRARIES}
      DESTINATION "${INSTALL_LIBDIR}"
      TARGET mysqlsh
  )
  install_bundled_binaries(
      DESCRIPTION "SASL plugins"
      BINARIES ${_SASL_PLUGINS}
      DESTINATION "${INSTALL_LIBDIR}/sasl2"
      TARGET mysqlsh
  )
ENDIF()

IF(BUNDLED_ANTLR_DIR)
  if(WIN32)
    file(GLOB_RECURSE _ANTLR4_SHARED_LIB "${ANTLR4_LIB_BASE_DIRECTORY}/*.dll")
  elseif(APPLE)
    file(GLOB_RECURSE _ANTLR4_SHARED_LIB "${ANTLR4_LIB_BASE_DIRECTORY}/*.dylib")
  else()
    file(GLOB _ANTLR4_SHARED_LIB "${ANTLR4_LIBRARY}.*")
  endif()

  install_bundled_binaries(
      DESCRIPTION "ANTLR libraries"
      BINARIES ${_ANTLR4_SHARED_LIB}
      DESTINATION "${INSTALL_LIBDIR}"
      TARGET mysqlsh
  )
ENDIF()

# If defined, installs the client authentication plugins
IF(AUTH_CLIENT_PLUGINS)
  IF(AUTH_CLIENT_PLUGINS_DEPS)
    install_bundled_binaries(
        DESCRIPTION "MySQL authentication plugins - dependencies"
        BINARIES ${AUTH_CLIENT_PLUGINS_DEPS}
        DESTINATION "${INSTALL_LIBDIR}"
        TARGET mysqlsh
    )
  ENDIF()

  install_bundled_binaries(
      DESCRIPTION "MySQL authentication plugins"
      BINARIES ${AUTH_CLIENT_PLUGINS}
      DESTINATION "${INSTALL_MYSQL_PLUGINS_DIR}"
      TARGET mysqlsh
  )
ELSE()
  MESSAGE(WARNING "Could not find Authentication Client Plugins!")
ENDIF()

IF(BUNDLED_PROTOBUF_LIBRARY)
  install_bundled_binaries(
      DESCRIPTION "Protobuf library"
      BINARIES ${BUNDLED_PROTOBUF_LIBRARY}
      DESTINATION "${INSTALL_LIBDIR}"
      TARGET mysqlsh
  )

  IF(BUNDLED_ABSEIL_LIBRARIES)
    install_bundled_binaries(
        DESCRIPTION "abseil libraries"
        BINARIES ${BUNDLED_ABSEIL_LIBRARIES}
        DESTINATION "${INSTALL_LIBDIR}"
        TARGET mysqlsh
    )
  ENDIF()
ENDIF()

IF (JIT_EXECUTOR_LIB)
  SET(INSTALL_JIT_EXECUTOR_LIBRARY "${JIT_EXECUTOR_LIBRARY}")
  IF (WIN32)
    GET_FILENAME_COMPONENT(LIB_JIT_EXECUTOR_NAME_WE "${JIT_EXECUTOR_LIBRARY}" NAME_WE)
    GET_FILENAME_COMPONENT(LIB_JIT_EXECUTOR_DIR "${JIT_EXECUTOR_LIBRARY}" DIRECTORY)
    SET(INSTALL_JIT_EXECUTOR_LIBRARY "${LIB_JIT_EXECUTOR_DIR}/${LIB_JIT_EXECUTOR_NAME_WE}.dll")
  ENDIF()
  install_bundled_binaries(
      DESCRIPTION "Jit Executor library"
      BINARIES "${INSTALL_JIT_EXECUTOR_LIBRARY}"
      DESTINATION "${INSTALL_LIBDIR}"
      TARGET mysqlsh
      WRITE_RPATH FALSE
  )
ENDIF()
