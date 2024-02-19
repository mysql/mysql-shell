# Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

function(add_shell_executable)
  # ARGV0 - name
  # ARGV1 - sources
  # ARGV2 - (optional) if true skips install
  message(STATUS "Adding executable ${ARGV0}")
  add_executable("${ARGV0}" ${ARGV1})
  set_target_properties("${ARGV0}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}")
  fix_target_output_directory("${ARGV0}" "${INSTALL_BINDIR}")

  if(BUNDLED_OPENSSL AND NOT WIN32)
    target_link_libraries("${ARGV0}"
      "-L${OPENSSL_DIRECTORY}"
    )
    if(NOT APPLE)
      target_link_libraries("${ARGV0}"
        "-Wl,-rpath-link,${OPENSSL_DIRECTORY}"
      )
    endif()
    if(NOT CRYPTO_DIRECTORY STREQUAL OPENSSL_DIRECTORY)
      target_link_libraries("${ARGV0}"
        "-L${CRYPTO_DIRECTORY}"
      )
      if(NOT APPLE)
        target_link_libraries("${ARGV0}"
          "-Wl,-rpath-link,${CRYPTO_DIRECTORY}"
        )
      endif()
    endif()
  endif()

  if(NOT ARGV2)
    message(STATUS "Marking executable ${ARGV0} as a run-time component")
    install(TARGETS "${ARGV0}" RUNTIME COMPONENT main DESTINATION "${INSTALL_BINDIR}")
  endif()

  if(APPLE)
    if(BUNDLED_OPENSSL)
      add_custom_command(TARGET "${ARGV0}" POST_BUILD
        COMMAND install_name_tool -change
                "${CRYPTO_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${CRYPTO_VERSION}"
                $<TARGET_FILE:${ARGV0}>
        COMMAND install_name_tool -change
                "${OPENSSL_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${OPENSSL_VERSION}"
                $<TARGET_FILE:${ARGV0}>
      )
    endif()
    if(BUNDLED_SHARED_PYTHON)
      get_filename_component(PYTHON_VERSION "${PYTHON_LIBRARIES}" NAME)
      add_custom_command(TARGET "${ARGV0}" POST_BUILD
        COMMAND install_name_tool -change
                "${PYTHON_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${PYTHON_VERSION}"
                $<TARGET_FILE:${ARGV0}>
      )
    endif()
    if(BUNDLED_SSH_DIR)
      get_target_property(_SSH_LIBRARY_LOCATION ssh LOCATION)
      get_filename_component(_SSH_LIBRARY_NAME ${_SSH_LIBRARY_LOCATION} NAME)
      add_custom_command(TARGET "${ARGV0}" POST_BUILD
        COMMAND install_name_tool -change
                "${_SSH_LIBRARY_NAME}" "@loader_path/../${INSTALL_LIBDIR}/${_SSH_LIBRARY_NAME}"
                $<TARGET_FILE:${ARGV0}>)
    endif()
    if(BUNDLED_ANTLR_DIR)
      add_custom_command(TARGET "${ARGV0}" POST_BUILD
        COMMAND install_name_tool -change
                "${ANTLR4_LIB_FILENAME}" "@loader_path/../${INSTALL_LIBDIR}/${ANTLR4_LIB_FILENAME}"
                $<TARGET_FILE:${ARGV0}>)
    endif()
  elseif(NOT WIN32)
  if(BUNDLED_OPENSSL OR BUNDLED_SHARED_PYTHON OR BUNDLED_SSH_DIR OR BUNDLED_KRB5_DIR OR BUNDLED_ANTLR_DIR OR BUNDLED_SASL_DIR)
    # newer versions of linker enable new dtags by default, causing -Wl,-rpath to create RUNPATH
    # entry instead of RPATH this results in loader loading system libraries (if they are available)
    # instead of bundled ones, this has special impact when bundled libraries are linked using a bundled
    # version of OpenSSL (which might be newer than the system one)
    set_property(TARGET "${ARGV0}" PROPERTY LINK_OPTIONS "-Wl,--disable-new-dtags")
    set_property(TARGET "${ARGV0}" PROPERTY INSTALL_RPATH "\$ORIGIN/../${INSTALL_LIBDIR}")
    set_property(TARGET "${ARGV0}" PROPERTY PROPERTY BUILD_WITH_INSTALL_RPATH TRUE)
  endif()
  endif()
  if(HAVE_V8)
    # strip the whole binary on 32bit Ubuntu 18.04, avoid OOM linker errors
    if(LINUX_UBUNTU_18_04 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
      MY_TARGET_LINK_OPTIONS("${ARGV0}" "LINKER:--strip-all")
    endif()
  endif()
endfunction()

function(write_rpath)
  set(options)
  set(oneValueArgs BINARY DESTINATION)
  set(multiValueArgs)
  cmake_parse_arguments(WRITE_RPATH "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  file(RELATIVE_PATH RELATIVE_PATH_TO_LIBDIR "${CONFIG_BINARY_DIR}/${WRITE_RPATH_DESTINATION}" "${CONFIG_BINARY_DIR}/${INSTALL_LIBDIR}")

  # if RELATIVE_PATH_TO_LIBDIR is not set, then binaries are copied to the INSTALL_LIBDIR, in which case rpath should be $ORIGIN
  # in all other cases we want to separate $ORIGIN with a slash
  if(RELATIVE_PATH_TO_LIBDIR)
    string(PREPEND RELATIVE_PATH_TO_LIBDIR "/")
  endif()

  execute_patchelf(--remove-rpath "${WRITE_RPATH_BINARY}")
  execute_patchelf(--set-rpath "\$ORIGIN${RELATIVE_PATH_TO_LIBDIR}" "${WRITE_RPATH_BINARY}")
endfunction()

function(install_bundled_binaries)
  set(options)
  set(oneValueArgs DESTINATION TARGET WRITE_RPATH)
  set(multiValueArgs BINARIES)
  cmake_parse_arguments(INSTALL_BUNDLED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED INSTALL_BUNDLED_WRITE_RPATH)
    # if not explicitly specified, we always add an RPATH
    set(INSTALL_BUNDLED_WRITE_RPATH TRUE)
  endif()

  if(WIN32 AND (INSTALL_BUNDLED_DESTINATION STREQUAL INSTALL_LIBDIR OR INSTALL_BUNDLED_DESTINATION MATCHES "^${INSTALL_LIBDIR}/.*"))
    # on Windows, if files should be installed in the 'lib' directory, we install them in the 'bin' directory instead
    set(INSTALL_BUNDLED_DESTINATION "${INSTALL_BINDIR}")
  endif()

  SET(NORMALIZED_BINARIES "")

  foreach(SOURCE_BINARY ${INSTALL_BUNDLED_BINARIES})
    FILE(TO_CMAKE_PATH "${SOURCE_BINARY}" SOURCE_BINARY)
    list(APPEND NORMALIZED_BINARIES "${SOURCE_BINARY}")
  endforeach()

  SET(INSTALL_BUNDLED_BINARIES ${NORMALIZED_BINARIES})

  INSTALL(PROGRAMS ${INSTALL_BUNDLED_BINARIES} DESTINATION "${INSTALL_BUNDLED_DESTINATION}" COMPONENT main)

  foreach(SOURCE_BINARY ${INSTALL_BUNDLED_BINARIES})
    if(NOT IS_SYMLINK "${SOURCE_BINARY}")
      message(STATUS "Bundling binary: ${SOURCE_BINARY}, installation directory: ${INSTALL_BUNDLED_DESTINATION}")
      if(LINUX)
        if(INSTALL_BUNDLED_WRITE_RPATH)
          write_rpath(BINARY "${SOURCE_BINARY}" DESTINATION "${INSTALL_BUNDLED_DESTINATION}" TARGET ${INSTALL_BUNDLED_TARGET})
        endif()
      elseif(APPLE)
        # if we bundle OpenSSL, then we want the bundled binary to use it instead of the system one
        if(BUNDLED_OPENSSL)
          SET_BUNDLED_OPEN_SSL(BINARIES "${SOURCE_BINARY}")
        endif()
      endif()
    endif()
  endforeach()

  # copy the binary to the build directory
  set(DESTINATION_BINARY_DIR "${CONFIG_BINARY_DIR}/${INSTALL_BUNDLED_DESTINATION}")
  add_custom_command(TARGET ${INSTALL_BUNDLED_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DESTINATION_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${INSTALL_BUNDLED_BINARIES} "${DESTINATION_BINARY_DIR}")
endfunction()

function(generate_rc_file)
  set(options)
  set(oneValueArgs NAME DESCRIPTION OUTPUT_DIR OUT_RC_FILE)
  set(multiValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(MYSH_ORIGINAL_FILE_NAME "${ARG_NAME}")
  get_filename_component(MYSH_INTERNAL_NAME "${ARG_NAME}" NAME_WLE)
  set(MYSH_FILE_DESCRIPTION "${ARG_DESCRIPTION}")

  get_filename_component(_ext "${ARG_NAME}" LAST_EXT)

  if(_ext STREQUAL ".exe")
    set(MYSH_FILE_TYPE "VFT_APP")
  elseif(_ext STREQUAL ".dll")
    set(MYSH_FILE_TYPE "VFT_DLL")
  else()
    message(FATAL_ERROR "Unsupported file type: ${_ext}")
  endif()

  if(NOT ARG_OUTPUT_DIR)
    set(ARG_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  set(_rc_file "${ARG_OUTPUT_DIR}/${MYSH_INTERNAL_NAME}.rc")
  configure_file("${PROJECT_SOURCE_DIR}/res/resource.rc.in" "${_rc_file}" @ONLY)
  set(${ARG_OUT_RC_FILE} "${_rc_file}" PARENT_SCOPE)
endfunction()
