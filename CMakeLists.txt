# Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

cmake_minimum_required (VERSION 2.8)

project (mysqlsh)

IF(CMAKE_VERSION VERSION_EQUAL "3.0.0" OR
   CMAKE_VERSION VERSION_GREATER "3.0.0")
  CMAKE_POLICY(SET CMP0026 OLD)
  CMAKE_POLICY(SET CMP0045 OLD)
  CMAKE_POLICY(SET CMP0042 OLD)

ENDIF()

set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/common/cmake
    ${CMAKE_MODULE_PATH}
)

###
### Initial configuration
###

INCLUDE(version.cmake)

###
### Detect Stuff
###

INCLUDE(install_macros)
INCLUDE(libutils)
INCLUDE(msvc)
INCLUDE(ssl)
INCLUDE(readline)
INCLUDE(compiler)
include(cmake/compile_flags.cmake)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
if(HAVE_PYTHON)
  FIND_PACKAGE(PythonLibs 2.6)
  # On windows stores the path to the python libraries since they will be bundled
  IF(WIN32)
    # PYTHON_PATH and PYTHON_FOLDER are used to properly copy the python libraries where needed
    # By default they take the value of PB2WORKDIR and PYTHON_SOURCENAME env vars
    IF (NOT PYTHON_PATH AND NOT PYTHON_FOLDER)
      SET(PYTHON_PATH "$ENV{PB2WORKDIR}")
      SET(PYTHON_FOLDER "$ENV{PYTHON_SOURCENAME}")
    ENDIF()

    IF (PYTHON_PATH AND PYTHON_FOLDER)
      SET(PYTHON_LIBRARY_PATH "${PYTHON_PATH}\\${PYTHON_FOLDER}\\Lib")
      STRING(REPLACE "\\" "\\\\" PYTHON_LIBRARY_PATH ${PYTHON_LIBRARY_PATH})
      STRING(REPLACE "Lib" "DLLs" PYTHON_DLLS_PATH ${PYTHON_LIBRARY_PATH})
    ENDIF()
  ENDIF()
endif()

INCLUDE(protobuf)
SETUP_PROTOBUF()

SET(HAVE_PROTOBUF "YES")              # Variable for CMake processing
add_definitions(-DX_PROTOCOL_ENABLED) # Preprocessor variable for generated projects

add_definitions(-DMYSQLXTEST_STANDALONE)

IF ( V8_INCLUDE_DIR AND V8_LIB_DIR )
  set(HAVE_V8 "YES")         # Variable for CMake processing
  add_definitions(-DHAVE_V8) # Preprocessor variable for generated projects
ELSE()
  message(WARNING "V8 is unavailable: building without JavaScript support.")
ENDIF()

IF (PYTHONLIBS_FOUND)
  set(HAVE_PYTHON "YES")         # Variable for CMake processing
  add_definitions(-DHAVE_PYTHON) # Preprocessor variable for generated projects
  message(STATUS "Python ${PYTHONLIBS_VERSION_STRING}")
  message(STATUS "PYTHON_LIRARIES: ${PYTHON_LIBRARIES}")

  IF(NOT(PYTHONLIBS_VERSION_STRING VERSION_LESS "3.0"))
    MESSAGE(FATAL_ERROR "Python support requires Python 2.6 or 2.7")
  ENDIF()
ELSE()
  message(WARNING "Python is unavailable: building without Python support.")
ENDIF()

# Sets default linking to static if not already defined
if(NOT DEFINED MYSQLCLIENT_STATIC_LINKING)
  set(MYSQLCLIENT_STATIC_LINKING TRUE)
endif()

find_package(MySQL REQUIRED)
add_definitions(-DHAVE_LIBMYSQLCLIENT)

find_package(Boost 1.42 REQUIRED)
find_package(Curses)

# Check whether boost::system can be compiled into the binary
include(CheckCXXSourceCompiles)
SET(CMAKE_REQUIRED_FLAGS "-DBOOST_ALL_NO_LIB")

 SET(CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS})

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
  set(BOOST_SOURCE_CODE "${PROJECT_SOURCE_DIR}/src/boost_code.cc")
else()
  message(STATUS "Boost requires linkage")
  set(BOOST_LIBRARIES boost_system)
endif()

# disable optimization for boost error_code, otherwise we get link errors in Release builds
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET_SOURCE_FILES_PROPERTIES(${PROJECT_SOURCE_DIR}/src/boost_code.cc PROPERTIES COMPILE_FLAGS -O0)
endif ()

##
## Installation location
##
SET(INSTALL_BINDIR "bin")
SET(INSTALL_LIBDIR "lib/mysqlsh")
SET(INSTALL_INCLUDEDIR "include/mysqlsh")
SET(INSTALL_SHAREDIR "share/mysqlsh")

##
## Configure style system detection (generation of config.h)
##
INCLUDE(configure.cmake)
CONFIGURE_FILE(config.h.cmake   ${CMAKE_BINARY_DIR}/include/mysh_config.h)

MACRO (MY_CHECK_CXX_COMPILER_FLAG FLAG RESULT)
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

# Must be before MYSQL_CHECK_SSL()
include_directories( ${CMAKE_BINARY_DIR}/include/ ${PROJECT_SOURCE_DIR}/include)

# Add bundled yassl/taocrypt or system openssl.
IF (WITH_SSL)
  MYSQL_CHECK_SSL()
ENDIF()

MYSQL_CHECK_EDITLINE()

MESSAGE(STATUS "SSL_LIBRARIES:    ${SSL_LIBRARIES}")
MESSAGE(STATUS "SSL_DEFINES:      ${SSL_DEFINES}")
MESSAGE(STATUS "SSL_INCLUDE_DIRS: ${SSL_INCLUDE_DIRS}")

###
### Handle User Options
###

if(NOT WINDOWS_RUNTIME_MD)
  add_definitions(-DNo_mysqlshtypes)
endif()

# Common warning flags for GCC, G++, Clang and Clang++
set(MY_WARNING_FLAGS "-Wall -Wextra -Wformat-security -Wvla")

# Common warning flags for GCC and Clang
set(MY_C_WARNING_FLAGS
    "${MY_WARNING_FLAGS} -Wwrite-strings -Wdeclaration-after-statement")

set(MY_CXX_WARNING_FLAGS "${MY_CXX_WARNING_FLAGS} -Werror")

if(NOT WIN32)
  # make some overzealous warnings into non errors
  set(MY_CXX_WARNING_FLAGS "${MY_CXX_WARNING_FLAGS} -Wno-error=unused-value,unused-variable")
  #  add_subdirectory(src/libedit)
endif()


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
                    "${CMAKE_SOURCE_DIR}"
                    "${CMAKE_SOURCE_DIR}/common"
                    "${CMAKE_SOURCE_DIR}/ext/rapidjson/include"
                   )

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

IF (HAVE_PYTHON)
  include_directories(${PYTHON_INCLUDE_DIRS})
ENDIF()

add_definitions(-DMYSH_VERSION="${MYSH_VERSION}")

IF(WIN32)
  add_definitions(-DMYSH_VERSION_WIN="${MYSH_VERSION_WIN}")
ENDIF()

IF(WITH_TESTS)
  ###
  ### Unit-test support
  ###
  include(testing)
  SETUP_TESTING()
  add_subdirectory(unittest)
  add_subdirectory(shell-tests)
  CONFIGURE_FILE(CTestCustom.cmake   ${CMAKE_BINARY_DIR}/CTestCustom.cmake)
ENDIF()

###
### Build Projects
###

add_subdirectory(python)

if(WINDOWS_RUNTIME_MD)
  #add_subdirectory(types)
  add_definitions(-DUSING_V8_SHARED)
  remove_definitions(-Dmysqlshtypes_EXPORTS)
endif()
add_subdirectory(mysqlxtest)
add_subdirectory(shellcore)

# mysqlsh cmdline util build
if(NOT WIN32)
  include_directories( "${PROJECT_SOURCE_DIR}/src/libedit" )
endif()

set(mysh_SRC
    "${PROJECT_SOURCE_DIR}/src/main.cc"
    "${PROJECT_SOURCE_DIR}/src/get_password.cc"
    "${PROJECT_SOURCE_DIR}/src/shell_cmdline_options.h"
    "${PROJECT_SOURCE_DIR}/src/shell_cmdline_options.cc"
    "${PROJECT_SOURCE_DIR}/src/cmdline_shell.h"
    "${PROJECT_SOURCE_DIR}/src/cmdline_shell.cc"
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

if(WIN32)
  if(NOT WINDOWS_RUNTIME_MD)
    add_dependencies(mysqlsh boost_code.obj)
  endif()
else()
  target_link_libraries(mysqlsh edit)
endif()

# Last, to make -lrt -lpthread last, v8 needs it
target_link_libraries(mysqlsh ${MYSQLSHCORE_LIBS} ${GCOV_LDFLAGS})

install(TARGETS mysqlsh RUNTIME COMPONENT main DESTINATION ${INSTALL_BINDIR})

if(WIN32)
  install(FILES "${CMAKE_SOURCE_DIR}/res/mysqlprovision.cmd" COMPONENT main DESTINATION ${INSTALL_BINDIR})
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()

  IF(CMAKE_BUILD_TYPE MATCHES Debug)
    add_custom_command(TARGET mysqlsh PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy
                        "${CMAKE_SOURCE_DIR}/res/mysqlprovision.cmd" $<TARGET_FILE_DIR:mysqlsh>)
  ENDIF()
endif()

add_definitions(-DBOOST_ALL_NO_LIB)

ADD_SUBDIRECTORY(doc)

include(packaging)
include(extra_install)

if(WIN32)
  remove_definitions(-DUNICODE)
  IF(PYTHON_LIBRARY_PATH)
    install(DIRECTORY "${PYTHON_LIBRARY_PATH}" DESTINATION "lib/Python2.7" COMPONENT main)
    install(DIRECTORY "${PYTHON_DLLS_PATH}" DESTINATION "lib/Python2.7" COMPONENT main)
  ENDIF()

  # Copies the mysqlshcore dll to the binary location to enable running the UTs
  if(WINDOWS_RUNTIME_MD)
    add_custom_command(TARGET mysqlsh PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy
                        $<TARGET_FILE:mysqlshcore> $<TARGET_FILE_DIR:mysqlsh>)
  endif()
endif()

# Installation instructions
if(WITH_DEV)
    if(HAVE_V8 AND WIN32)
       install(FILES ${V8_LIBS} DESTINATION lib COMPONENT dev)
    endif()

    install(DIRECTORY "${CMAKE_SOURCE_DIR}/utils/" DESTINATION "${INSTALL_INCLUDEDIR}/utils" COMPONENT dev FILES_MATCHING PATTERN "*.h" )
    install(FILES "${CMAKE_SOURCE_DIR}/common/logger/logger.h" DESTINATION "${INSTALL_INCLUDEDIR}/logger" COMPONENT dev)
endif()

if(WINDOWS_RUNTIME_MD AND HAVE_V8)
  # for MD runtime we also need DLL, not just lib, but dll is on parent directory.
  set(V8_DLL ${V8_LIBS})
  string(REPLACE ".lib" ".dll" V8_DLL ${V8_DLL})
  get_filename_component(V8_FILE ${V8_DLL} NAME)
  get_filename_component(V8_DLL ${V8_DLL} DIRECTORY)
  get_filename_component(V8_DLL ${V8_DLL} DIRECTORY)
  install(FILES "${V8_DLL}/${V8_FILE}" DESTINATION bin COMPONENT main)
  install(FILES "${V8_DLL}/${V8_FILE}" DESTINATION lib COMPONENT dev)

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    # Copies the dll to the binary location to enable debugging
    add_custom_command(TARGET mysqlsh PRE_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy
                        "${V8_DLL}/${V8_FILE}" $<TARGET_FILE_DIR:mysqlsh>/${V8_FILE})
  endif()
endif()

# This should be there for both MD and non MD windows builds
if (HAVE_PYTHON)
  if (WIN32)
    set(PY_DLL ${PYTHON_LIBRARIES})
    string(REPLACE ".lib" ".dll" PY_DLL ${PY_DLL})
    get_filename_component(PY_FILE ${PY_DLL} NAME)
    get_filename_component(PY_DLL ${PY_DLL} DIRECTORY)
    get_filename_component(PY_DLL ${PY_DLL} DIRECTORY)
    install(FILES "${PY_DLL}/${PY_FILE}" DESTINATION bin COMPONENT main)

    IF (WITH_DEV)
      install(FILES "${PY_DLL}/${PY_FILE}" DESTINATION lib COMPONENT dev)
    ENDIF()

    if(CMAKE_BUILD_TYPE MATCHES Debug)
      # Copies the dll to the binary location to enable debugging
      add_custom_command(TARGET mysqlsh PRE_BUILD
                          COMMAND ${CMAKE_COMMAND} -E copy
                          "${PY_DLL}/${PY_FILE}" $<TARGET_FILE_DIR:mysqlsh>/${PY_FILE})
    endif()
  endif()
endif()


# Generate mysqlsh_config script

SET(prefix "${CMAKE_INSTALL_PREFIX}")
SET(sysconfdir ${prefix})
SET(bindir ${prefix}/${INSTALL_BINDIR})
SET(datadir ${prefix}/${INSTALL_SHAREDIR})
SET(pkgdatadir ${prefix}/${INSTALL_SHAREDIR})
SET(libsubdir  ${INSTALL_LIBDIR})
SET(pkgincludedir ${prefix}/${INSTALL_INCLUDEDIR})
SET(pkglibdir ${prefix}/${INSTALL_LIBDIR})

FOREACH(LIB ${MYSQLSHCORE_LIBS})
  SET(MYSQLSHCORE_LIBLIST "${MYSQLSHCORE_LIBLIST} -l${LIB}")
ENDFOREACH()

#unused IF (WITH_DEV AND NOT WIN32)
IF(WITH_DEV AND FALSE)
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/mysqlsh_config.sh
        ${CMAKE_CURRENT_BINARY_DIR}/mysqlsh_config ESCAPE_QUOTES @ONLY)
    EXECUTE_PROCESS(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/mysqlsh_config)

   INSTALL_SCRIPT(
      ${CMAKE_CURRENT_BINARY_DIR}/mysqlsh_config
      DESTINATION ${INSTALL_BINDIR}
      COMPONENT dev
    )
ENDIF()
