# Copyright (c) 2019, 2025, Oracle and/or its affiliates.
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

  if(NOT WIN32)
    # newer versions of linker enable new dtags by default, causing -Wl,-rpath to create RUNPATH
    # entry instead of RPATH this results in loader loading system libraries (if they are available)
    # instead of bundled ones, this has special impact when bundled libraries are linked using a bundled
    # version of OpenSSL (which might be newer than the system one)
    if(APPLE)
      set_property(TARGET "${ARGV0}" PROPERTY INSTALL_RPATH "@executable_path/../${INSTALL_LIBDIR}")
    else()
      set_property(TARGET "${ARGV0}" PROPERTY LINK_OPTIONS "-Wl,--disable-new-dtags")
      set_property(TARGET "${ARGV0}" PROPERTY INSTALL_RPATH "\$ORIGIN/../${INSTALL_LIBDIR}")
    endif()

    set_property(TARGET "${ARGV0}" PROPERTY BUILD_WITH_INSTALL_RPATH TRUE)
  endif()

  if(HAVE_JS)
    # strip the whole binary on 32bit Ubuntu 18.04, avoid OOM linker errors
    if(LINUX_UBUNTU_18_04 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
      MY_TARGET_LINK_OPTIONS("${ARGV0}" "LINKER:--strip-all")
    endif()
  endif()
endfunction()

if(APPLE)
  function(get_use_bundled_openssl_command)
    set(options)
    set(oneValueArgs PATTERN OUT_COMMAND)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set("${ARG_OUT_COMMAND}"
        ${CMAKE_COMMAND}
        -DCRYPTO_VERSION="${CRYPTO_VERSION}"
        -DOPENSSL_VERSION="${OPENSSL_VERSION}"
        -DINSTALL_LIBDIR="${INSTALL_LIBDIR}"
        -Dpattern="${ARG_PATTERN}"
        -P "${CMAKE_SOURCE_DIR}/cmake/apple_use_bundled_openssl.cmake"
        PARENT_SCOPE
    )
  endfunction()
elseif(NOT WIN32)
  function(get_patchelf_command)
    if(NOT PATCHELF_EXECUTABLE)
      message(FATAL_ERROR "Please install the patchelf(1) utility.")
    endif()

    set(options)
    set(oneValueArgs OUT_COMMAND)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set("${ARG_OUT_COMMAND}" ${PATCHELF_EXECUTABLE} ${PATCHELF_PAGE_SIZE_ARGS} ${ARG_ARGS} PARENT_SCOPE)
  endfunction()

  function(get_rpath_command)
    set(options)
    set(oneValueArgs BINARY DESTINATION OUT_COMMAND)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    file(RELATIVE_PATH RELATIVE_PATH_TO_LIBDIR "${CONFIG_BINARY_DIR}/${ARG_DESTINATION}" "${CONFIG_BINARY_DIR}/${INSTALL_LIBDIR}")

    # if RELATIVE_PATH_TO_LIBDIR is not set, then binaries are copied to the INSTALL_LIBDIR, in which case rpath should be $ORIGIN
    # in all other cases we want to separate $ORIGIN with a slash
    if(RELATIVE_PATH_TO_LIBDIR)
      string(PREPEND RELATIVE_PATH_TO_LIBDIR "/")
    endif()

    get_patchelf_command(ARGS --set-rpath "\\$$ORIGIN${RELATIVE_PATH_TO_LIBDIR}" "${ARG_BINARY}" OUT_COMMAND COMMAND)
    set("${ARG_OUT_COMMAND}" ${COMMAND} PARENT_SCOPE)
  endfunction()
endif()

function(get_binary_output_dir)
  set(options)
  set(oneValueArgs DESTINATION OUT_DESTINATION)
  set(multiValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(WIN32 AND CMAKE_VERSION VERSION_LESS "3.20.0")
    # on Windows, CONFIG_BINARY_DIR is using a generator expression, which is supported by OUTPUT of add_custom_command starting with 3.20
    set(DESTINATION_BINARY_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${ARG_DESTINATION}")
  else()
    set(DESTINATION_BINARY_DIR "${CONFIG_BINARY_DIR}/${ARG_DESTINATION}")
  endif()

  set("${ARG_OUT_DESTINATION}" "${DESTINATION_BINARY_DIR}" PARENT_SCOPE)
endfunction()

function(install_bundled_binaries)
  set(options USE_LIBDIR)
  set(oneValueArgs DESCRIPTION DESTINATION TARGET WRITE_RPATH)
  set(multiValueArgs BINARIES EXTRA_COPY_COMMANDS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  message(STATUS "Bundling ${ARG_DESCRIPTION}")

  if(NOT DEFINED ARG_WRITE_RPATH)
    if(APPLE)
      # if we bundle OpenSSL, then we want the bundled binary to use it instead of the system one
      set(ARG_WRITE_RPATH ${BUNDLED_OPENSSL})
    else()
      # if not explicitly specified, we always add an RPATH
      set(ARG_WRITE_RPATH TRUE)
    endif()
  endif()

  if(NOT ARG_USE_LIBDIR AND WIN32 AND (ARG_DESTINATION STREQUAL INSTALL_LIBDIR OR ARG_DESTINATION MATCHES "^${INSTALL_LIBDIR}/.*"))
    # on Windows, if files should be installed in the 'lib' directory, we install them in the 'bin' directory instead
    set(NEW_DESTINATION "${INSTALL_BINDIR}")
    # change destination directory, keep the suffix
    string(LENGTH "${INSTALL_LIBDIR}" _len)
    string(SUBSTRING "${ARG_DESTINATION}" ${_len} -1 _suffix)
    string(APPEND NEW_DESTINATION "${_suffix}")

    message(STATUS "  Changing destination from ${ARG_DESTINATION} to ${NEW_DESTINATION}")
    set(ARG_DESTINATION "${NEW_DESTINATION}")
  endif()

  SET(NORMALIZED_BINARIES "")

  foreach(SOURCE_BINARY ${ARG_BINARIES})
    file(TO_CMAKE_PATH "${SOURCE_BINARY}" SOURCE_BINARY)
    list(APPEND NORMALIZED_BINARIES "${SOURCE_BINARY}")
  endforeach()

  SET(ARG_BINARIES ${NORMALIZED_BINARIES})

  get_binary_output_dir(DESTINATION "${ARG_DESTINATION}" OUT_DESTINATION DESTINATION_BINARY_DIR)

  SET(COPIED_BINARIES "")

  foreach(SOURCE_BINARY ${ARG_BINARIES})
    get_filename_component(SOURCE_BINARY_NAME "${SOURCE_BINARY}" NAME)
    set(COPIED_BINARY "${DESTINATION_BINARY_DIR}/${SOURCE_BINARY_NAME}")
    set(COPY_TARGET "copy_${SOURCE_BINARY_NAME}")
    SET(COPY_COMMAND "")

    if(NOT IS_SYMLINK "${SOURCE_BINARY}")
      message(STATUS "  Bundling binary: ${SOURCE_BINARY}, installation directory: ${ARG_DESTINATION}")
      if(LINUX)
        if(ARG_WRITE_RPATH)
          get_rpath_command(BINARY "${COPIED_BINARY}" DESTINATION "${ARG_DESTINATION}" OUT_COMMAND RPATH_COMMAND)
          message(STATUS "    will execute: ${RPATH_COMMAND}")
          list(APPEND COPY_COMMAND COMMAND ${RPATH_COMMAND})
        endif()
      elseif(APPLE)
        # reset the ID to something predictable (file name)
        get_filename_component(_ext "${SOURCE_BINARY_NAME}" LAST_EXT)

        if(_ext STREQUAL ".so" OR _ext STREQUAL ".dylib")
          # if this is a shared library, modify the source and prepend @rpath, this will simplify linking
          execute_process(
              COMMAND install_name_tool -id "@rpath/${SOURCE_BINARY_NAME}" "${SOURCE_BINARY}"
              RESULT_VARIABLE COMMAND_RESULT)

          if(NOT "${COMMAND_RESULT}" STREQUAL "0")
            message(FATAL_ERROR "Failed to change ID of ${SOURCE_BINARY} to ${SOURCE_BINARY_NAME}.")
          endif()
        else()
          list(APPEND COPY_COMMAND COMMAND install_name_tool -id "${SOURCE_BINARY_NAME}" "${COPIED_BINARY}")
        endif()

        if(ARG_WRITE_RPATH)
          get_use_bundled_openssl_command(PATTERN "${COPIED_BINARY}" OUT_COMMAND COMMAND)
          list(APPEND COPY_COMMAND COMMAND ${COMMAND})
        endif()
      endif()
    endif()

    # create a dependency, so that files are copied only if source changes
    add_custom_command(OUTPUT "${COPIED_BINARY}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DESTINATION_BINARY_DIR}"
        COMMAND ${CMAKE_COMMAND} -Dsrc_file="${SOURCE_BINARY}" -Ddst_dir="${DESTINATION_BINARY_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/copy_file.cmake"
        ${COPY_COMMAND}
        ${ARG_EXTRA_COPY_COMMANDS}
        DEPENDS "${SOURCE_BINARY}"
        COMMENT "Copying ${SOURCE_BINARY} to ${DESTINATION_BINARY_DIR}..."
    )
    add_custom_target("${COPY_TARGET}"
        DEPENDS "${COPIED_BINARY}"
    )
    add_dependencies("${ARG_TARGET}" "${COPY_TARGET}")

    list(APPEND COPIED_BINARIES "${COPIED_BINARY}")
  endforeach()

  # we're installing copied binaries, so that source files remain intact (i.e. RPATH is written to the copy, not the original)
  INSTALL(PROGRAMS ${COPIED_BINARIES} DESTINATION "${ARG_DESTINATION}" COMPONENT main)
endfunction()

function(install_bundled_directory)
  set(options COPY_ONLY DEPS_ON_ALL)
  set(oneValueArgs DESCRIPTION DIRECTORY CONTENTS DESTINATION TARGET)
  set(multiValueArgs EXTRA_COPY_COMMANDS EXTRA_INSTALL_COMMANDS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(ARG_CONTENTS)
    set(ARG_DIRECTORY "${ARG_CONTENTS}")
    set(ARG_CONTENTS "contents of ")
  endif()

  file(TO_CMAKE_PATH "${ARG_DIRECTORY}" ARG_DIRECTORY)

  message(STATUS "Bundling ${ARG_DESCRIPTION}, ${ARG_CONTENTS}directory: ${ARG_DIRECTORY}, destination: ${ARG_DESTINATION}")

  get_binary_output_dir(DESTINATION "${ARG_DESTINATION}" OUT_DESTINATION DESTINATION_BINARY_DIR)
  get_filename_component(SRC_DIR_NAME "${ARG_DIRECTORY}" NAME)

  if(NOT ARG_CONTENTS)
    # cmake's copy_directory copies contents of a directory, when we're installing a directory, we need to append its name to the copy's destination
    string(APPEND DESTINATION_BINARY_DIR "/${SRC_DIR_NAME}")
  endif()

  if(ARG_DEPS_ON_ALL)
    # use all files in directories as dependencies
    file(GLOB_RECURSE COPY_DEPS LIST_DIRECTORIES false RELATIVE "${ARG_DIRECTORY}" "${ARG_DIRECTORY}/*")
  else()
    # use top level files as dependencies
    file(GLOB COPY_DEPS LIST_DIRECTORIES false RELATIVE "${ARG_DIRECTORY}" "${ARG_DIRECTORY}/*")
  endif()

  if(NOT COPY_DEPS)
    # no top level files, use the first one found recursively
    file(GLOB_RECURSE COPY_DEPS LIST_DIRECTORIES false RELATIVE "${ARG_DIRECTORY}" "${ARG_DIRECTORY}/*")

    if(NOT COPY_DEPS)
      message(FATAL_ERROR "Trying to bundle an empty directory: ${ARG_DIRECTORY}")
    endif()

    list(GET COPY_DEPS 0 COPY_DEPS)
  endif()

  set(COPIED_DEPS ${COPY_DEPS})

  list(TRANSFORM COPY_DEPS PREPEND "${ARG_DIRECTORY}/")
  list(TRANSFORM COPIED_DEPS PREPEND "${DESTINATION_BINARY_DIR}/")

  # create a dependency, so that directory is copied only if any of the top level files changes
  add_custom_command(OUTPUT ${COPIED_DEPS}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${DESTINATION_BINARY_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${ARG_DIRECTORY}" "${DESTINATION_BINARY_DIR}"
      ${ARG_EXTRA_COPY_COMMANDS}
      DEPENDS ${COPY_DEPS}
      COMMENT "Copying contents of directory ${ARG_DIRECTORY} to ${DESTINATION_BINARY_DIR}..."
  )

  get_filename_component(COPY_TARGET "${ARG_DESTINATION}" NAME)
  string(PREPEND COPY_TARGET "copy_dir_${SRC_DIR_NAME}_to_")

  add_custom_target("${COPY_TARGET}"
      DEPENDS ${COPIED_DEPS}
  )
  add_dependencies("${ARG_TARGET}" "${COPY_TARGET}")

  if(NOT ARG_COPY_ONLY)
    # install the copied directory
    if(ARG_CONTENTS)
      # we want to install contents of this directory, cmake will append its name to the destination, unless the path ends with a slash
      string(APPEND DESTINATION_BINARY_DIR "/")
    endif()

    install(DIRECTORY "${DESTINATION_BINARY_DIR}" DESTINATION "${ARG_DESTINATION}" USE_SOURCE_PERMISSIONS COMPONENT main ${ARG_EXTRA_INSTALL_COMMANDS})
  endif()
endfunction()

function(install_bundled_plugins)
  set(options)
  set(oneValueArgs DESCRIPTION DIRECTORY TARGET)
  set(multiValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  file(TO_CMAKE_PATH "${ARG_DIRECTORY}" ARG_DIRECTORY)

  message(STATUS "Bundling ${ARG_DESCRIPTION}s, directory: ${ARG_DIRECTORY}")

  file(GLOB plugins RELATIVE "${ARG_DIRECTORY}" "${ARG_DIRECTORY}/*")

  foreach(plugin ${plugins})
    if(NOT plugin STREQUAL "CMakeFiles" AND IS_DIRECTORY "${ARG_DIRECTORY}/${plugin}")
      install_bundled_directory(
          DESCRIPTION "${ARG_DESCRIPTION} (${plugin})"
          DIRECTORY "${ARG_DIRECTORY}/${plugin}"
          DESTINATION "${INSTALL_LIBDIR}/plugins"
          TARGET "${ARG_TARGET}"
          DEPS_ON_ALL
      )
    endif()
  endforeach()
endfunction()

function(install_bundled_files)
  set(options)
  set(oneValueArgs DESCRIPTION DESTINATION TARGET)
  set(multiValueArgs FILES)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  message(STATUS "Bundling ${ARG_DESCRIPTION} files, destination: ${ARG_DESTINATION}")

  get_binary_output_dir(DESTINATION "${ARG_DESTINATION}" OUT_DESTINATION DESTINATION_BINARY_DIR)

  set(SRC_FILES "")
  set(COPIED_FILES "")

  foreach(file ${ARG_FILES})
    file(TO_CMAKE_PATH "${file}" file)
    list(APPEND SRC_FILES "${file}")

    get_filename_component(file_name "${file}" NAME)
    list(APPEND COPIED_FILES "${DESTINATION_BINARY_DIR}/${file_name}")
    set(SRC_FILE_NAME "${file_name}")
  endforeach()

  # create a dependency
  add_custom_command(OUTPUT ${COPIED_FILES}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${DESTINATION_BINARY_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy ${SRC_FILES} "${DESTINATION_BINARY_DIR}"
      DEPENDS ${SRC_FILES}
      COMMENT "Copying ${ARG_DESCRIPTION} files to ${DESTINATION_BINARY_DIR}..."
  )

  get_filename_component(COPY_TARGET "${ARG_DESTINATION}" NAME)
  string(PREPEND COPY_TARGET "copy_file_${SRC_FILE_NAME}_to_")

  add_custom_target("${COPY_TARGET}"
      DEPENDS ${COPIED_FILES}
  )
  add_dependencies("${ARG_TARGET}" "${COPY_TARGET}")

  # install the copied files
  install(FILES ${COPIED_FILES} DESTINATION "${ARG_DESTINATION}" COMPONENT main)
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

function(static_mysql_lib)
  set(options)
  set(oneValueArgs NAME PATH)
  set(multiValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(WIN32)
    set(${ARG_PATH} "${MYSQL_BUILD_DIR}/archive_output_directory/${CMAKE_BUILD_TYPE}/${ARG_NAME}.lib" PARENT_SCOPE)
  else()
    set(${ARG_PATH} "${MYSQL_BUILD_DIR}/archive_output_directory/lib${ARG_NAME}.a" PARENT_SCOPE)
  endif()
endfunction()
