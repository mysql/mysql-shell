# Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

project (mysh)

set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/common/cmake
    ${CMAKE_MODULE_PATH}
)

###
### Initial configuration
###

SET(MYSH_MAJOR 0)
SET(MYSH_MINOR 0)
SET(MYSH_PATCH 2)
SET(MYSH_LEVEL "-alpha") # Empty if GA

SET(MYSH_VERSION "${MYSH_MAJOR}.${MYSH_MINOR}.${MYSH_PATCH}${MYSH_LEVEL}")

###
### Detect Stuff
###

INCLUDE(libutils)
INCLUDE(msvc)
INCLUDE(ssl)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
if(HAVE_PYTHON)
  INCLUDE(FindPythonLibs)
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

IF ( PYTHONLIBS_FOUND )
  set(HAVE_PYTHON "YES")         # Variable for CMake processing
  add_definitions(-DHAVE_PYTHON) # Preprocessor variable for generated projects
  message(STATUS "Python ${PYTHONLIBS_VERSION_STRING}")
  message(STATUS "PYTHON_LIRARIES: ${PYTHON_LIBRARIES}")
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
CHECK_CXX_SOURCE_COMPILES(${PROJECT_SOURCE_DIR}/src/boost_code.cc BOOST_SYSTEM_COMPILES)

if (BOOST_SYSTEM_COMPILES)
  set(BOOST_SOURCE_CODE "${PROJECT_SOURCE_DIR}/boost_code.cc")
else()
  message(STATUS "Boost requires linkage")
  set(BOOST_LIBRARIES boost_system)
endif()

# disable optimization for boost error_code, otherwise we get link errors in Release builds
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET_SOURCE_FILES_PROPERTIES(${PROJECT_SOURCE_DIR}/src/boost_code.cc PROPERTIES COMPILE_FLAGS -O0)
endif ()


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
  add_subdirectory(src/libedit)
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

 install(FILES ${V8_LIBS} DESTINATION lib)
  if(WINDOWS_RUNTIME_MD)
    # for MD runtime we also need DLL, not just lib, but dll is on parent directory.
    set(V8_DLL ${V8_LIBS})
    string(REPLACE ".lib" ".dll" V8_DLL ${V8_DLL})
    get_filename_component(V8_FILE ${V8_DLL} NAME)
    get_filename_component(V8_DLL ${V8_DLL} DIRECTORY)
    get_filename_component(V8_DLL ${V8_DLL} DIRECTORY)
    install(FILES "${V8_DLL}/${V8_FILE}" DESTINATION lib)
  endif()

  if(APPLE)
    set(V8_LINK_LIST -mmacosx-version-min=10.7 ${V8_LIBS})
  elseif(WIN32)
    set(V8_LINK_LIST ${V8_LIBS} winmm)
  else()	# Linux
    set(V8_LINK_LIST ${V8_LIBS})
  endif()

  if (APPLE)
    # needed when compiling in osx, for v8
    # has to be after src/libedit
    add_definitions(-std=gnu++0x -mmacosx-version-min=10.7)
  endif()
ENDIF()

include_directories(${MYSQL_INCLUDE_DIR}
                    "${Boost_INCLUDE_DIRS}"
                    "${PYTHON_INCLUDE_DIRS}"
                    "${CMAKE_SOURCE_DIR}"
                    "${CMAKE_SOURCE_DIR}/common"
                    "${CMAKE_SOURCE_DIR}/ext/rapidjson/include"
                   )


IF(WITH_TESTS)
  ###
  ### Unit-test support
  ###
  include(testing)
  SETUP_TESTING()
  add_subdirectory(unittest)
ENDIF()

###
### Build Projects
###

if(WIN32)
  if(WINDOWS_RUNTIME_MD)
    add_definitions(-DUSING_V8_SHARED)
  endif()
endif()


if(WINDOWS_RUNTIME_MD)
  add_subdirectory(types)
  remove_definitions(-Dmysqlshtypes_EXPORTS)
endif()
add_subdirectory(mysqlxtest)
add_subdirectory(shellcore)
add_subdirectory(modules)

# mysh cmdline util build
if(NOT WIN32)
  include_directories( "${PROJECT_SOURCE_DIR}/src/libedit" )
endif()

set(mysh_SRC
    "${PROJECT_SOURCE_DIR}/src/main.cc"
    "${PROJECT_SOURCE_DIR}/src/get_password.cc"
)

add_executable(mysqlx ${mysh_SRC} ${BOOST_SOURCE_CODE})

if(WIN32)
  if(NOT WINDOWS_RUNTIME_MD)
    add_dependencies(mysqlx boost_code.obj)
  endif()
else()
  target_link_libraries(mysqlx edit)
endif()

# Last, to make -lrt -lpthread last, v8 needs it
target_link_libraries(mysqlx mysqlshmods mysqlsh mysqlxtest ${V8_LINK_LIST} ${PYTHON_LIBRARIES} ${BOOST_LIBRARIES} ${SSL_LIBRARIES} ${MYSQL_LIBRARIES} ${GCOV_LDFLAGS})

install(TARGETS mysqlx DESTINATION bin)
install(DIRECTORY scripting/modules DESTINATION lib)

# This is to copy the modules into the build directory to make them
# available for debugging purposes
add_custom_command(TARGET mysqlx PRE_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_directory
                      ${CMAKE_SOURCE_DIR}/scripting/modules $<TARGET_FILE_DIR:mysqlx>/modules)

if(WIN32)
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()
endif()

add_definitions(-DBOOST_ALL_NO_LIB )

ADD_SUBDIRECTORY(doc)


IF (WITH_SAMPLES)
  add_subdirectory(samples/native)
ENDIF()

include(packaging)

if(WIN32)
  remove_definitions(-DUNICODE)
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/utils/" DESTINATION "include/utils" FILES_MATCHING PATTERN "*.h" )
install(DIRECTORY "${CMAKE_SOURCE_DIR}/common/logger" DESTINATION "include/common" FILES_MATCHING PATTERN "*.h" )
install(DIRECTORY "${CMAKE_SOURCE_DIR}/ext/rapidjson/include/rapidjson" DESTINATION "include" FILES_MATCHING PATTERN "*.h" )
