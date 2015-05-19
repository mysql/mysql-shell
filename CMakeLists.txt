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
SET(MYSH_PATCH 1)
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
INCLUDE(FindPythonLibs)

IF (WITH_PROTOBUF)
  SET(HAVE_PROTOBUF "YES")              # Variable for CMake processing
  add_definitions(-DX_PROTOCOL_ENABLED) # Preprocessor variable for generated projects
ENDIF()

IF ( V8_INCLUDE_DIR AND V8_LIB_DIR )
  set(HAVE_V8 "YES")         # Variable for CMake processing
  add_definitions(-DHAVE_V8) # Preprocessor variable for generated projects
ELSE()
  message(WARNING "V8 is unavailable: building without JavaScript support.")
ENDIF()

IF ( PYTHONLIBS_FOUND )
  set(HAVE_PYTHON "YES")         # Variable for CMake processing
  add_definitions(-DHAVE_PYTHON) # Preprocessor variable for generated projects
  message(STATUS "Python ${PYTHONLIBS_VERSION_STRING} found: ${PYTHON_LIBRARIES}")
ELSE()
  message(WARNING "Python is unavailable: building without Python support.")
ENDIF()

# Sets default linking to static if not already defined
if(NOT DEFINED MYSQLCLIENT_STATIC_LINKING)
  set(MYSQLCLIENT_STATIC_LINKING TRUE)
endif()

find_package(MySQL REQUIRED)
find_package(Boost 1.42 REQUIRED)
find_package(Curses)


# disable optimization for boost error_code, otherwise we get link errors in Release builds
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   SET_SOURCE_FILES_PROPERTIES(${PROJECT_SOURCE_DIR}/src/boost_code.cc PROPERTIES COMPILE_FLAGS -O0)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   SET_SOURCE_FILES_PROPERTIES(${PROJECT_SOURCE_DIR}/src/boost_code.cc PROPERTIES COMPILE_FLAGS -O0)
endif ()


##
## Configure style system detection (generation of config.h)
##
CHECK_INCLUDE_FILES (pwd.h HAVE_PWD_H)
if(WIN32)
else()
  CHECK_INCLUDE_FILES (sys/ioctl.h HAVE_SYS_IOCTL_H)
  CHECK_INCLUDE_FILES (termio.h HAVE_TERMIO_H)
  CHECK_INCLUDE_FILES (termios.h HAVE_TERMIOS_H)
  CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
endif()

CHECK_FUNCTION_EXISTS (getpass HAVE_GETPASS)

CONFIGURE_FILE(config.h.cmake   ${CMAKE_BINARY_DIR}/include/mysh_config.h)

include(CheckCXXSourceCompiles)

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

# Add bundled yassl/taocrypt or system openssl.
MYSQL_CHECK_SSL()


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


include_directories( ${CMAKE_BINARY_DIR}/include/ )


if(NOT WIN32)
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

IF (HAVE_PYTHON)
  include_directories(${PYTHON_INCLUDE_DIRS})
ENDIF()

include_directories("${PROJECT_SOURCE_DIR}/include"
                    "${MYSQL_INCLUDE_DIR}"
                    "${Boost_INCLUDE_DIRS}"
                    "${CMAKE_SOURCE_DIR}/utils"
                   )


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

IF(WITH_TESTS)
  ###
  ### Unit-test support
  ###
  include(testing)
  SETUP_TESTING()
  add_subdirectory(testing)
ENDIF()

# mysh cmdline util build
if(NOT WIN32)
  include_directories( "${PROJECT_SOURCE_DIR}/src/libedit" )
endif()				   

file(GLOB mysh_SRC
    "${PROJECT_SOURCE_DIR}/src/*.cc"
)

add_executable(mysqlx ${mysh_SRC})

if(WIN32)
  if(NOT WINDOWS_RUNTIME_MD)
    add_dependencies(mysqlx boost_code.obj)
  endif()
else()
  target_link_libraries(mysqlx edit)
endif()
target_link_libraries(mysqlx mysqlsh mysqlshmods)

target_link_libraries(mysqlx ${GCOV_LDFLAGS})
if ( HAVE_V8 )
  target_link_libraries(mysqlx ${V8_LINK_LIST})
endif()
if ( HAVE_PYTHON )
  target_link_libraries(mysqlx ${PYTHON_LIBRARIES})
endif()

# Last, to make -lrt -lpthread last, v8 needs it
target_link_libraries(mysqlx ${MYSQL_LIBRARIES})

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

include(packaging)
