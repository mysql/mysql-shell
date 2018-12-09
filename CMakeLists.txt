# Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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
### This can't be automated because we require generating
### packages to be released in January by the end of the year.
set(PACKAGE_YEAR 2019)
add_definitions(-DPACKAGE_YEAR="${PACKAGE_YEAR}")

###
### Initial configuration
###

INCLUDE(version.cmake)

###
### Detect Stuff
###

INCLUDE(install_macros)
INCLUDE(libutils)
INCLUDE(compiler)
INCLUDE(static_analysis)
include(cmake/compile_flags.cmake)
INCLUDE(msvc)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)

if(HAVE_PYTHON)
  # On windows stores the path to the python libraries since they will be bundled
  IF(WIN32)
    FIND_PACKAGE(PythonLibs 2.6)
    # PYTHON_PATH and PYTHON_FOLDER are used to properly copy the python libraries where needed
    # By default they take the value of PB2WORKDIR and PYTHON_SOURCENAME env vars
    IF(NOT PYTHON_PATH AND NOT PYTHON_FOLDER)
      SET(PYTHON_PATH "$ENV{PB2WORKDIR}")
      SET(PYTHON_FOLDER "$ENV{PYTHON_SOURCENAME}")
    ENDIF()

    IF(NOT PYTHON_PATH AND NOT PYTHON_FOLDER)
      GET_FILENAME_COMPONENT(PYTHON_PATH ${PYTHON_INCLUDE_DIR} DIRECTORY)
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
    SET(PYTHON_INCLUDE_DIR "${BUNDLED_PYTHON_DIR}/include/python2.7")
    SET(PYTHON_LIBRARIES "${BUNDLED_PYTHON_DIR}/lib/libpython2.7.a")
    SET(PYTHONLIBS_VERSION_STRING "2.7")
  ELSE()
    FIND_PACKAGE(PythonLibs 2.6)
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
  message(STATUS "PYTHON_INCLUDE_DIR: ${PYTHON_INCLUDE_DIR}")
  message(STATUS "PYTHON_LIRARIES: ${PYTHON_LIBRARIES}")

  IF(NOT(PYTHONLIBS_VERSION_STRING VERSION_LESS "3.0"))
    MESSAGE(FATAL_ERROR "Python support requires Python 2.6 or 2.7")
  ENDIF()

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
IF(WIN32 OR APPLE OR BUNDLED_OPENSSL_DIR)

  # In MySQL Server 8.0.4 and up, OpenSSL is linked dynamically and
  # preferably linked against the system installed OpenSSL. But there
  # is no standard system installed OpenSSL on macOS or Windows so we
  # bundle the library copying it from the server install we link
  # against.

  IF(WIN32)
    IF(EXISTS "${MYSQL_BUILD_DIR}/runtime_output_directory/RelWithDebInfo/ssleay32.dll")
      FILE(TO_CMAKE_PATH ${MYSQL_BUILD_DIR} MYSQL_BUILD_DIR)
      SET(OPENSSL_TO_BUNDLE_DIR "${MYSQL_BUILD_DIR}/runtime_output_directory/RelWithDebInfo")
    ELSEIF(WITH_SSL AND EXISTS "${WITH_SSL}/lib/ssleay32.dll")
      SET(OPENSSL_TO_BUNDLE_DIR "${WITH_SSL}/lib")
    ENDIF()
  ELSEIF(APPLE)
    IF(EXISTS "${MYSQL_BUILD_DIR}/library_output_directory/libssl.dylib")
      SET(OPENSSL_TO_BUNDLE_DIR "${MYSQL_BUILD_DIR}/library_output_directory")
    ELSEIF(WITH_SSL AND EXISTS "${WITH_SSL}/lib/libssl.dylib")
      SET(OPENSSL_TO_BUNDLE_DIR "${WITH_SSL}/lib")
    ENDIF()
    # Find the "version" from the file name, it is the same
    FILE(GLOB CRYPTO_VERSION  RELATIVE ${OPENSSL_TO_BUNDLE_DIR} "${OPENSSL_TO_BUNDLE_DIR}/libcrypto.*.*.*.dylib")
    FILE(GLOB OPENSSL_VERSION RELATIVE ${OPENSSL_TO_BUNDLE_DIR} "${OPENSSL_TO_BUNDLE_DIR}/libssl.*.*.*.dylib")
  ELSE()
    SET(OPENSSL_TO_BUNDLE_DIR "${BUNDLED_OPENSSL_DIR}/lib")
    # Find the "version" from the file name, it is the same
    FILE(GLOB CRYPTO_VERSION  RELATIVE ${OPENSSL_TO_BUNDLE_DIR} "${OPENSSL_TO_BUNDLE_DIR}/libcrypto.so.*.*.*")
    FILE(GLOB OPENSSL_VERSION RELATIVE ${OPENSSL_TO_BUNDLE_DIR} "${OPENSSL_TO_BUNDLE_DIR}/libssl.so.*.*.*")
  ENDIF()
ENDIF()

if (NOT BUILD_SOURCE_PACKAGE)
  # Needed for protobuf header paths, which are needed to use libmysqlxclient
  INCLUDE(protobuf)
  INCLUDE(FindMySQLx)
endif()

##
## Installation location
##
SET(INSTALL_BINDIR "bin")
SET(INSTALL_LIBDIR "lib/mysqlsh")
SET(INSTALL_INCLUDEDIR "include/mysqlsh")
SET(INSTALL_SHAREDIR "share/mysqlsh")

##
## Configure style system detection(generation of config.h)
##
INCLUDE(configure.cmake)

## The following definition is required to have memset_s
## available on the platforms that implement it
IF (HAVE_MEMSET_S)
  ADD_DEFINITIONS(-D__STDC_WANT_LIB_EXT1__=1)
ENDIF()

# This is used for the version_compile_machine variable.
SET(MYSQL_MACHINE_TYPE ${CMAKE_SYSTEM_PROCESSOR})
IF(CMAKE_SYSTEM_PROCESSOR MATCHES "i386" AND CMAKE_SIZEOF_VOID_P MATCHES 8)
  SET(MYSQL_MACHINE_TYPE "x86_64")
ENDIF()
CONFIGURE_FILE(config.h.cmake   ${CMAKE_BINARY_DIR}/mysqlshdk/include/mysh_config.h)

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

# Includes common to the whole project
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include/
            ${PROJECT_SOURCE_DIR}/include
            ${MYSQL_INCLUDE_DIRS}
            ${MYSQLX_INCLUDE_DIRS}
            ${PROTOBUF_INCLUDE_DIRS})

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
    if(NOT WIN32)
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

IF(WITH_TESTS)
  ###
  ### Unit-test support
  ###
  add_subdirectory(unittest)
  add_subdirectory(shell-tests)
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

###
### Optionally bundle Visual C++ Redistributable for Visual Studio
###

if(WIN32)
  option(BUNDLE_RUNTIME_LIBRARIES "Install Windows runtime libraries" OFF)
  if(BUNDLE_RUNTIME_LIBRARIES)
     INSTALL_VS_REDIST(main)
  endif()
endif()

###
### Packaging and other dependecy copying
###

INCLUDE(packaging)
