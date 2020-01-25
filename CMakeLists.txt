# Copyright (c) 2014, 2020, Oracle and/or its affiliates. All rights reserved.
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

IF(UNIX)
  # CMake 3.4.0 is needed for Solaris due to CMake bug 15673.
  # Cannot use CMAKE_SYSTEM_NAME until after PROJECT(), use uname directly.
  FIND_PROGRAM(MY_UNAME uname /bin /usr/bin /usr/local/bin /sbin)
  IF(MY_UNAME)
    EXEC_PROGRAM(uname ARGS -s OUTPUT_VARIABLE MY_HOST_SYSTEM_NAME)
    IF(MY_HOST_SYSTEM_NAME MATCHES "SunOS")
      CMAKE_MINIMUM_REQUIRED(VERSION 3.4.0)
    ENDIF()
  ENDIF()
ENDIF()
# 2.8.12 as minimum unless higher is checked above.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

# On Linux el6/el7 the default gcc is too old, see if devtoolset is installed.
# Same with SLES 12, look for gcc 7 there.
# We need to look for gcc before calling PROJECT below.
OPTION(FORCE_UNSUPPORTED_COMPILER "Disable compiler version checks" OFF)
MARK_AS_ADVANCED(FORCE_UNSUPPORTED_COMPILER)

IF(CMAKE_HOST_UNIX AND NOT FORCE_UNSUPPORTED_COMPILER
    AND NOT CMAKE_C_COMPILER AND NOT CMAKE_CXX_COMPILER)
  # Cannot INCLUDE(CMakeDetermineSystem) prior to PROJECT initialization below.
  SET (ENV_CC "$ENV{CC}")
  SET (ENV_CXX "$ENV{CXX}")
  IF (ENV_CC STREQUAL "" AND ENV_CXX STREQUAL "")
    IF(MY_UNAME)
      EXEC_PROGRAM(uname ARGS -r OUTPUT_VARIABLE MY_HOST_SYSTEM_VERSION)
      IF(MY_HOST_SYSTEM_NAME MATCHES "Linux" AND
          MY_HOST_SYSTEM_VERSION MATCHES "\\.el[67](uek)?\\.")
        MESSAGE(STATUS
          "This is ${MY_HOST_SYSTEM_NAME} version ${MY_HOST_SYSTEM_VERSION}")
        MESSAGE(STATUS "We probably need some devtoolset compiler")

        FIND_PROGRAM(ALTERNATIVE_GCC gcc
          NO_DEFAULT_PATH
          PATHS "/opt/rh/devtoolset-7/root/usr/bin")
        FIND_PROGRAM(ALTERNATIVE_GPP g++
          NO_DEFAULT_PATH
          PATHS "/opt/rh/devtoolset-7/root/usr/bin")
        FIND_PROGRAM(ALTERNATIVE_ENABLE enable
          NO_DEFAULT_PATH
          PATHS "/opt/rh/devtoolset-7/")
        IF(ALTERNATIVE_GCC AND ALTERNATIVE_GPP)
          SET(CMAKE_C_COMPILER ${ALTERNATIVE_GCC})
          SET(CMAKE_CXX_COMPILER ${ALTERNATIVE_GPP})
          MESSAGE(STATUS "Using ${ALTERNATIVE_GCC}")
          MESSAGE(STATUS "Using ${ALTERNATIVE_GPP}")
        ELSE()
          MESSAGE(WARNING "Could not find devtoolset gcc")
        ENDIF()
      ELSEIF(MY_HOST_SYSTEM_NAME MATCHES "Linux" AND
          EXISTS "/etc/os-release")
        FILE(READ "/etc/os-release" MY_OS_RELEASE)
        IF (MY_OS_RELEASE MATCHES "SUSE Linux Enterprise Server 12")
          MESSAGE(STATUS "We need to look for a newer GCC on SLES 12")

          FIND_PROGRAM(ALTERNATIVE_GCC gcc-7
            NO_DEFAULT_PATH
            PATHS "/usr/bin")
          FIND_PROGRAM(ALTERNATIVE_GPP g++-7
            NO_DEFAULT_PATH
            PATHS "/usr/bin")
          IF (ALTERNATIVE_GCC AND ALTERNATIVE_GPP)
            SET(CMAKE_C_COMPILER ${ALTERNATIVE_GCC})
            SET(CMAKE_CXX_COMPILER ${ALTERNATIVE_GPP})
            MESSAGE(STATUS "Using ${ALTERNATIVE_GCC}")
            MESSAGE(STATUS "Using ${ALTERNATIVE_GPP}")
            # Use the new ABI so that std::string can be used with allocators
            # that are not default-constructible (e.g. Memroot_allocator)
            ADD_DEFINITIONS(-D_GLIBCXX_USE_CXX11_ABI=1)
          ELSE()
            MESSAGE(WARNING "Could not find newer gcc")
	  ENDIF()
	ENDIF()
      ENDIF()
    ENDIF()
  ENDIF()
ENDIF()


project(mysqlsh)

IF(CMAKE_VERSION VERSION_EQUAL "3.0.0" OR
   CMAKE_VERSION VERSION_GREATER "3.0.0")
  CMAKE_POLICY(SET CMP0026 OLD)
  CMAKE_POLICY(SET CMP0045 OLD)
  CMAKE_POLICY(SET CMP0042 OLD)
ENDIF()

set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_MODULE_PATH}
)

### Configuring package year
IF(NOT PACKAGE_YEAR)
  SET(PACKAGE_YEAR "2020")

  string(TIMESTAMP month_year "%Y/%m" UTC)
  # Changes done in december will probably be released
  # in january of year+1, so abort until someone confirms
  # the year (e.g. either by passing -DPACKAGE_YEAR or
  # SET(PACKAGE_YEAR ...) outside this IF())
  IF(month_year STREQUAL "${PACKAGE_YEAR}/12")
    MESSAGE(FATAL_ERROR "Package year is ${PACKAGE_YEAR}, please confirm correct value")
  ENDIF()
ENDIF()
add_definitions(-DPACKAGE_YEAR="${PACKAGE_YEAR}")

###
### Initial configuration
###

INCLUDE(version.cmake)

###
### Detect Stuff
###

INCLUDE(install_macros)
INCLUDE(exeutils)
INCLUDE(libutils)
INCLUDE(compiler)
INCLUDE(static_analysis)
include(cmake/compile_flags.cmake)
INCLUDE(msvc)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)

include(curl)
MYSQL_CHECK_CURL()

IF (WITH_OCI AND NOT HAVE_PYTHON)
  MESSAGE(FATAL_ERROR "WITH_OCI should not be used when python is not available.")
ENDIF()

IF(BUNDLED_OPENSSL_DIR)
  IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # this will force the MYSQL_CHECK_SSL() macro to use shared libraries
    SET(LINUX_STANDALONE 1)
  ENDIF()
  SET(WITH_SSL "${BUNDLED_OPENSSL_DIR}")
ELSE()
  SET(WITH_SSL "system")
ENDIF()

INCLUDE(ssl)
MYSQL_CHECK_SSL()

if(HAVE_PYTHON)
  IF(NOT USE_PYTHON_VERSION)
    SET(USE_PYTHON_VERSION "3.4.3")
  ENDIF()

  FUNCTION(GET_PYTHON_VERSION python_dir output_var)
    FILE(GLOB_RECURSE patch_level "${python_dir}/patchlevel.h")

    IF(patch_level AND EXISTS "${patch_level}")
      FILE(STRINGS "${patch_level}" python_version_str
           REGEX "^#define[ \t]+PY_VERSION[ \t]+\"[^\"]+\"")
      STRING(REGEX REPLACE "^#define[ \t]+PY_VERSION[ \t]+\"([^\"]+)\".*" "\\1"
                           python_version_str "${python_version_str}")
      SET(${output_var} "${python_version_str}" PARENT_SCOPE)
    ELSE()
      MESSAGE(FATAL_ERROR "Couldn't find patchlevel.h in ${python_dir}")
    ENDIF()
  ENDFUNCTION()

  FUNCTION(GET_MAJOR_MINOR python_ver output_var)
    STRING(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*" "\\1.\\2" python_ver "${python_ver}")
    SET(${output_var} ${python_ver} PARENT_SCOPE)
  ENDFUNCTION()

  # On windows stores the path to the python libraries since they will be bundled
  IF(WIN32)
    FIND_PACKAGE(PythonLibs "${USE_PYTHON_VERSION}" REQUIRED)
    # PYTHON_PATH and PYTHON_FOLDER are used to properly copy the python libraries where needed
    # By default they take the value of PB2WORKDIR and PYTHON_SOURCENAME env vars
    IF(NOT PYTHON_PATH AND NOT PYTHON_FOLDER)
      SET(PYTHON_PATH "$ENV{PB2WORKDIR}")
      SET(PYTHON_FOLDER "$ENV{PYTHON_SOURCENAME}")
    ENDIF()

    IF(NOT PYTHON_PATH AND NOT PYTHON_FOLDER)
      GET_FILENAME_COMPONENT(PYTHON_PATH ${PYTHON_INCLUDE_DIRS} DIRECTORY)
      GET_FILENAME_COMPONENT(PYTHON_FOLDER ${PYTHON_PATH} NAME)
      GET_FILENAME_COMPONENT(PYTHON_PATH ${PYTHON_PATH} DIRECTORY)
    ENDIF()

    IF(PYTHON_PATH AND PYTHON_FOLDER)
      SET(PYTHON_LIBRARY_PATH "${PYTHON_PATH}\\${PYTHON_FOLDER}\\Lib")
      STRING(REPLACE "\\" "\\\\" PYTHON_LIBRARY_PATH ${PYTHON_LIBRARY_PATH})
      STRING(REPLACE "Lib" "DLLs" PYTHON_DLLS_PATH ${PYTHON_LIBRARY_PATH})
    ENDIF()
  ELSEIF(BUNDLED_PYTHON_DIR)
    # If we're using a Python build for bundling, ensure we build against it too
    GET_PYTHON_VERSION("${BUNDLED_PYTHON_DIR}" PYTHONLIBS_VERSION_STRING)
    GET_MAJOR_MINOR("${PYTHONLIBS_VERSION_STRING}" PYTHONLIBS_MAJOR_MINOR)
    FIND_PATH(PYTHON_INCLUDE_DIRS
      NAMES
        Python.h
      HINTS
        "${BUNDLED_PYTHON_DIR}/include"
      PATH_SUFFIXES
        python${PYTHONLIBS_MAJOR_MINOR}mu
        python${PYTHONLIBS_MAJOR_MINOR}m
        python${PYTHONLIBS_MAJOR_MINOR}u
        python${PYTHONLIBS_MAJOR_MINOR}
      NO_DEFAULT_PATH
    )
    FIND_LIBRARY(PYTHON_LIBRARIES
      NAMES
        python${PYTHONLIBS_MAJOR_MINOR}mu
        python${PYTHONLIBS_MAJOR_MINOR}m
        python${PYTHONLIBS_MAJOR_MINOR}u
        python${PYTHONLIBS_MAJOR_MINOR}
      HINTS
        "${BUNDLED_PYTHON_DIR}/lib"
      NO_DEFAULT_PATH
    )
    IF(NOT PYTHON_LIBRARIES MATCHES ".*\\.a$")
      SET(BUNDLED_SHARED_PYTHON "YES")
    ENDIF()
  ELSE()
    FIND_PACKAGE(PythonLibs "${USE_PYTHON_VERSION}" REQUIRED)
  ENDIF()
endif()

IF(WIN32)
	# Speed up build process excluding unused header files
	# Also definitely resolves the trouble of using both
	# windows.h and winsock2.h
	ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)
  ADD_DEFINITIONS(-DNOMINMAX)
  # Silence warnings about functions that are perfectly fine for posix
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)

	# Sets the windows versions that should be supported
	ADD_DEFINITIONS(-DNTDDI_VERSION=0x06000100 -D_WIN32_WINNT=0x0600)
ELSE()
  # Non windows builds should build dev packages by default
  SET(WITH_DEV "1")
ENDIF()

IF(V8_INCLUDE_DIR AND V8_LIB_DIR)
  set(HAVE_V8 "YES")         # Variable for CMake processing
  add_definitions(-DHAVE_V8) # Preprocessor variable for generated projects
  add_definitions(-DV8_DEPRECATION_WARNINGS)
  add_definitions(-DV8_IMMINENT_DEPRECATION_WARNINGS)
ELSE()
  message(WARNING "V8 is unavailable: building without JavaScript support.")
ENDIF()

IF(PYTHONLIBS_FOUND OR BUNDLED_PYTHON_DIR)
  set(HAVE_PYTHON "YES")         # Variable for CMake processing
  IF(BUNDLED_PYTHON_DIR)
    add_definitions(-DHAVE_PYTHON=2) # 2 means bundled
  ELSE()
    add_definitions(-DHAVE_PYTHON=1) # 1 means normal (or windows)
  ENDIF()
  message(STATUS "Python ${PYTHONLIBS_VERSION_STRING}")
  message(STATUS "PYTHON_INCLUDE_DIRS: ${PYTHON_INCLUDE_DIRS}")
  message(STATUS "PYTHON_LIBRARIES: ${PYTHON_LIBRARIES}")

  IF(NOT PYTHONLIBS_VERSION_STRING)
    MESSAGE(FATAL_ERROR "Couldn't detect Python's version")
  ENDIF()

  # supported Python versions: ((v >= 2.6) && (v < 3.0)) || (v >= 3.4.3)
  IF(NOT((NOT(PYTHONLIBS_VERSION_STRING VERSION_LESS "2.6") AND (PYTHONLIBS_VERSION_STRING VERSION_LESS "3.0")) OR NOT(PYTHONLIBS_VERSION_STRING VERSION_LESS "3.4.3")))
    MESSAGE(FATAL_ERROR "Python support requires Python 2.6, 2.7, 3.4.3 or greater")
  ENDIF()

  IF(NOT(PYTHONLIBS_VERSION_STRING VERSION_LESS "3.0"))
    ADD_DEFINITIONS(-DIS_PY3K)
  ENDIF()

  GET_MAJOR_MINOR("${PYTHONLIBS_VERSION_STRING}" PYTHONLIBS_MAJOR_MINOR)
  message(STATUS "Python major.minor: ${PYTHONLIBS_MAJOR_MINOR}")

  # TODO: Temporary flag indicating python is static as well
  # Proper logic should be added to correctly configure this
  IF(WITH_STATIC_MSVCRT)
    SET(WITH_STATIC_PYTHON_LIB 1)
  ENDIF()

ELSE()
  message(WARNING "Python is unavailable: building without Python support.")
ENDIF()

# Sets default linking to static if not already defined
if(NOT DEFINED MYSQLCLIENT_STATIC_LINKING)
  set(MYSQLCLIENT_STATIC_LINKING TRUE)
endif()

### Bundling of OpenSSL libraries (if needed)
# macro MYSQL_CHECK_SSL_DLLS() adapted for Shell
IF (WITH_SSL_PATH AND (APPLE OR WIN32 OR LINUX_STANDALONE))
  SET(BUNDLED_OPENSSL 1)
  MESSAGE(STATUS "WITH_SSL_PATH ${WITH_SSL_PATH}")

  # In MySQL Server 8.0.4 and up, OpenSSL is linked dynamically and
  # preferably linked against the system installed OpenSSL. But there
  # is no standard system installed OpenSSL on macOS or Windows so we
  # bundle the library copying it from the server install we link
  # against.

  IF(WIN32)
    GET_FILENAME_COMPONENT(CRYPTO_NAME "${CRYPTO_LIBRARY}" NAME_WE)
    GET_FILENAME_COMPONENT(OPENSSL_NAME "${OPENSSL_LIBRARY}" NAME_WE)

    # Different naming scheme for the matching .dll as of SSL 1.1
    SET(SSL_MSVC_VERSION_SUFFIX)
    SET(SSL_MSVC_ARCH_SUFFIX)
    IF(OPENSSL_MINOR_VERSION VERSION_EQUAL 1)
      SET(SSL_MSVC_VERSION_SUFFIX "-1_1")
      SET(SSL_MSVC_ARCH_SUFFIX "-x64")
    ENDIF()

    # OpenSSL 1.1 Look for libcrypto-1_1-x64.dll or libcrypto-1_1.dll
    # OpenSSL 1.0 Look for libeay32.dll
    FIND_FILE(HAVE_CRYPTO_DLL
      NAMES
      "${CRYPTO_NAME}${SSL_MSVC_VERSION_SUFFIX}${SSL_MSVC_ARCH_SUFFIX}.dll"
      "${CRYPTO_NAME}${SSL_MSVC_VERSION_SUFFIX}.dll"
      PATHS "${WITH_SSL_PATH}/bin"
      NO_DEFAULT_PATH
      )
    FIND_FILE(HAVE_OPENSSL_DLL
      NAMES
      "${OPENSSL_NAME}${SSL_MSVC_VERSION_SUFFIX}${SSL_MSVC_ARCH_SUFFIX}.dll"
      "${OPENSSL_NAME}${SSL_MSVC_VERSION_SUFFIX}.dll"
      PATHS "${WITH_SSL_PATH}/bin"
      NO_DEFAULT_PATH
      )

    MESSAGE(STATUS "HAVE_CRYPTO_DLL ${HAVE_CRYPTO_DLL}")
    MESSAGE(STATUS "HAVE_OPENSSL_DLL ${HAVE_OPENSSL_DLL}")
  ELSE()
    EXECUTE_PROCESS(
      COMMAND readlink "${CRYPTO_LIBRARY}" OUTPUT_VARIABLE CRYPTO_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    EXECUTE_PROCESS(
      COMMAND readlink "${OPENSSL_LIBRARY}" OUTPUT_VARIABLE OPENSSL_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    MESSAGE(STATUS "CRYPTO_VERSION ${CRYPTO_VERSION}")
    MESSAGE(STATUS "OPENSSL_VERSION ${OPENSSL_VERSION}")

    GET_FILENAME_COMPONENT(CRYPTO_DIRECTORY "${CRYPTO_LIBRARY}" DIRECTORY)
    GET_FILENAME_COMPONENT(OPENSSL_DIRECTORY "${OPENSSL_LIBRARY}" DIRECTORY)

    MESSAGE(STATUS "CRYPTO_DIRECTORY ${CRYPTO_DIRECTORY}")
    MESSAGE(STATUS "OPENSSL_DIRECTORY ${OPENSSL_DIRECTORY}")

    SET(CRYPTO_FULL_NAME "${CRYPTO_DIRECTORY}/${CRYPTO_VERSION}")
    SET(OPENSSL_FULL_NAME "${OPENSSL_DIRECTORY}/${OPENSSL_VERSION}")

    MESSAGE(STATUS "CRYPTO_FULL_NAME ${CRYPTO_FULL_NAME}")
    MESSAGE(STATUS "OPENSSL_FULL_NAME ${OPENSSL_FULL_NAME}")
  ENDIF()
ENDIF()

if (NOT BUILD_SOURCE_PACKAGE)
  # Needed for protobuf header paths, which are needed to use libmysqlxclient
  INCLUDE(protobuf)
  INCLUDE(FindMySQLx)
  INCLUDE(lz4)
  # Add lz4 library (Dependency for the X Client Lib)
  MYSQL_CHECK_LZ4()
endif()

##
## Installation location
##
SET(INSTALL_BINDIR "bin")
SET(INSTALL_LIBDIR "lib/mysqlsh")
SET(INSTALL_INCLUDEDIR "include/mysqlsh")
SET(INSTALL_SHAREDIR "share/mysqlsh")
SET(INSTALL_DOCDIR "share/mysqlsh/Docs")

##
## Configure style system detection
##
INCLUDE(configure.cmake)

## The following definition is required to have memset_s
## available on the platforms that implement it
CHECK_FUNCTION_EXISTS (memset_s HAVE_MEMSET_S)
IF (HAVE_MEMSET_S)
  ADD_DEFINITIONS(-D__STDC_WANT_LIB_EXT1__=1)
ENDIF()
CONFIGURE_FILE(mysh_config.h.cmake   ${CMAKE_BINARY_DIR}/include/mysh_config.h)

MACRO(MY_CHECK_CXX_COMPILER_FLAG FLAG RESULT)
  SET(SAVE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${FLAG}")
  CHECK_CXX_SOURCE_COMPILES("int main(void) { return 0; }" ${RESULT}
    FAIL_REGEX "argument unused during compilation"
    FAIL_REGEX "unsupported .*option"
    FAIL_REGEX "unknown .*option"
    FAIL_REGEX "unrecognized .*option"
    FAIL_REGEX "ignoring unknown option"
    FAIL_REGEX "[Ww]arning: [Oo]ption"
   )
  SET(CMAKE_REQUIRED_FLAGS "${SAVE_CMAKE_REQUIRED_FLAGS}")
ENDMACRO()

# By default the shell uses Multi-threaded Dynamic RunTime
# Only sets Static if needed
IF(WIN32)
  IF(WITH_STATIC_MSVCRT)
    FOREACH(flag_var
          CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
       IF(${flag_var} MATCHES "/MD")
          STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
       ENDIF(${flag_var} MATCHES "/MD")
    ENDFOREACH(flag_var)
  ENDIF()
ENDIF()

# Set DBUG_OFF for non-debug project types.
FOREACH(BUILD_TYPE RELEASE RELWITHDEBINFO MINSIZEREL)
  FOREACH(LANG C CXX)
    SET(CMAKE_${LANG}_FLAGS_${BUILD_TYPE} "${CMAKE_${LANG}_FLAGS_${BUILD_TYPE}} -DDBUG_OFF ")
  ENDFOREACH()
ENDFOREACH()

IF(NOT CMAKE_BUILD_TYPE
    AND NOT CMAKE_GENERATOR MATCHES "Visual Studio"
    AND NOT CMAKE_GENERATOR MATCHES "Xcode")
    # This is the case of no CMAKE_BUILD_TYPE choosen, typical for VS and Xcode
    # or if custom C flags are set. In VS and Xcode for non-Debug configurations
    # DBUG_OFF is already correctly set.
    ADD_DEFINITIONS(-DDBUG_OFF)
ENDIF()

# Includes common to the whole project
INCLUDE_DIRECTORIES(
            ${CMAKE_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/ext
            ${MYSQL_INCLUDE_DIRS}
            ${MYSQLX_INCLUDE_DIRS}
            ${PROTOBUF_INCLUDE_DIRS})

IF(HAVE_PYTHON)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})
ENDIF()

###
### Handle User Options
###

if(HAVE_V8)
  find_library(V8_LIB v8_monolith
               PATHS ${V8_LIB_DIR}
               NO_DEFAULT_PATH
 )
  set(V8_LIBS)
  if(V8_LIB)
    set(V8_LIBS ${V8_LIB})      # Single lib, just use it
    message(STATUS "v8 Library Found: \"v8_monolith\" at ${V8_LIB}")
  else()
    # Could not find monolith library, try separate libs
    if(WIN32)
      # on Windows libraries could have .dll.lib extensions
      SET(ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
      SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".dll.lib")
    endif()
    foreach(_libname v8 v8_libbase v8_libplatform)
      find_library(_v8_lib ${_libname}
                   PATHS ${V8_LIB_DIR}
                   NO_DEFAULT_PATH
     )
      if(NOT _v8_lib)
        message(FATAL_ERROR "Could not find the library \"v8_monolith\" or \"${_libname}\" in ${V8_LIB_DIR}")
      else()
        message(STATUS "v8 Library Found: \"${_libname}\" at ${_v8_lib}")
      endif()
      list(APPEND V8_LIBS ${_v8_lib})
      UNSET(_v8_lib CACHE)
    endforeach()
    if(WIN32)
      # restore original extensions
      SET(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
    if(NOT WIN32 AND NOT APPLE)
      # newer versions of linker enable new dtags by default, causing -Wl,-rpath to create RUNPATH entry instead of RPATH
      # this results in loader being unable to find v8_libbase, as it's a transitive dependency of v8 and v8_libplatform
      # disable new dtags on debug builds, so resultant executable is usable without LD_LIBRARY_PATH
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -Wl,--disable-new-dtags")
    endif()
  endif()

  if(WIN32)
    set(V8_LINK_LIST ${V8_LIBS} winmm dbghelp shlwapi)
  else()  # Linux
    set(V8_LINK_LIST ${V8_LIBS})
  endif()
endif()

add_definitions(-DMYSH_VERSION="${MYSH_VERSION}" -DMYSH_BUILD_ID="${MYSH_BUILD_ID}" -DEXTRA_NAME_SUFFIX="${EXTRA_NAME_SUFFIX}")
if(WIN32)
  add_definitions(-DMYSH_VERSION_WIN="${MYSH_VERSION_WIN}")
endif()

##
## Prepares the OCI SDK when needed
###
IF(WITH_OCI)
  FILE(TO_CMAKE_PATH ${WITH_OCI} OCI_SDK)
  ADD_CUSTOM_COMMAND(OUTPUT oci_sdk_bundle
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${OCI_SDK}/ ${CMAKE_BINARY_DIR}/${INSTALL_SHAREDIR}/oci_sdk
      COMMENT "Bundling OCI SDK from ${WITH_OCI}"
  )
  ADD_CUSTOM_TARGET(oci_sdk ALL DEPENDS oci_sdk_bundle)

  INSTALL(DIRECTORY ${CMAKE_BINARY_DIR}/${INSTALL_SHAREDIR}/oci_sdk/ COMPONENT main DESTINATION ${INSTALL_SHAREDIR}/oci_sdk)
  ADD_DEFINITIONS(-DWITH_OCI)

  IF(BUNDLED_OPENSSL_DIR AND APPLE)
    FILE(GLOB_RECURSE OCI_OPEN_SSL_LIB ${OCI_SDK}/cryptography*.egg/cryptography/hazmat/bindings/_openssl*.so)
    EXECUTE_PROCESS(COMMAND install_name_tool -change
            "${CRYPTO_VERSION}" "@executable_path/../${INSTALL_LIBDIR}/${CRYPTO_VERSION}"
            ${OCI_OPEN_SSL_LIB})
    EXECUTE_PROCESS(COMMAND install_name_tool -change
            "${OPENSSL_VERSION}" "@executable_path/../${INSTALL_LIBDIR}/${OPENSSL_VERSION}"
            ${OCI_OPEN_SSL_LIB})
    EXECUTE_PROCESS(COMMAND otool -L ${OCI_OPEN_SSL_LIB})
  ENDIF()
ENDIF()


IF(WITH_TESTS)
  ###
  ### Unit-test support
  ###
  add_subdirectory(unittest)
ENDIF()

###
### Build Projects
###

ADD_SUBDIRECTORY(python)
ADD_SUBDIRECTORY(mysqlshdk)
ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(res)
ADD_SUBDIRECTORY(src)
add_subdirectory(samples)
add_subdirectory(mysql-secret-store)

CHECK_STAN_OPTIONS()

IF(EXISTS ${CMAKE_SOURCE_DIR}/internal/CMakeLists.txt)
  ADD_SUBDIRECTORY(internal)
ENDIF()

###
### Optionally bundle Visual C++ Redistributable for Visual Studio
###

if(WIN32)
  option(BUNDLE_RUNTIME_LIBRARIES "Install Windows runtime libraries" OFF)
  if(BUNDLE_RUNTIME_LIBRARIES)
     INSTALL_VS_REDIST(main)
  endif()
endif()

#
# Create the INFO_SRC and INFO_BIN files
# =========================
#
# Will set GIT_EXECUTABLE and GIT_FOUND
FIND_PACKAGE(Git)

CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/cmake/info_macros.cmake.in
    ${CMAKE_BINARY_DIR}/info_macros.cmake @ONLY)

INCLUDE(info_src)
INSTALL(FILES ${CMAKE_BINARY_DIR}/Docs/INFO_SRC COMPONENT main DESTINATION ${INSTALL_DOCDIR})

IF (NOT BUILD_SOURCE_PACKAGE)
  INCLUDE(info_bin)
  INSTALL(FILES ${CMAKE_BINARY_DIR}/Docs/INFO_BIN COMPONENT main DESTINATION ${INSTALL_DOCDIR})
ENDIF()


###
### Packaging and other dependecy copying
###

INCLUDE(packaging)
