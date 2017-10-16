# Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/common/uuid/include
    ${CMAKE_SOURCE_DIR}/ext/rapidjson/include
    ${PYTHON_INCLUDE_DIR}
    ${V8_INCLUDE_DIR})

set(SCRIPTING_SOURCES
    common.cc
    obj_date.cc
    object_factory.cc
    object_registry.cc
    proxy_object.cc
    types.cc
    types_cpp.cc
    shexcept.cc)

# Will generate the js_core_definitions file which will contain the
# core.js file in a variable.
if(HAVE_V8)
  # Loads the core.js file
  FILE(READ "${CMAKE_SOURCE_DIR}/mysqlshdk/scripts/core.js" JS_CORE_MODULE)

  # Strips the copyright notice from it
  string(FIND "${JS_CORE_MODULE}" "function" COPYRIGHT_END)
  string(SUBSTRING "${JS_CORE_MODULE}" 0 ${COPYRIGHT_END}+2 COPYRIGHT_TEXT)
  string(REPLACE "${COPYRIGHT_TEXT}" "" JS_CORE_MODULE_STRIPPED "${JS_CORE_MODULE}")

  # Updates format to be a C++ multiline definition
  string(REPLACE "\\" "\\\\" JS_CORE_MODULE_PREPARED "${JS_CORE_MODULE_STRIPPED}")
  string(REPLACE "\n" "\\n\"\n\"" JS_CORE_MODULE_UPDATED "${JS_CORE_MODULE_PREPARED}")

  # Creates the target file containing the code ready for processing
  configure_file("${CMAKE_SOURCE_DIR}/mysqlshdk/scripting/jscript_core_definitions.h.in"
                 "${CMAKE_BINARY_DIR}/scripting/jscript_core_definitions.h")

  set(SCRIPTING_SOURCES
    ${SCRIPTING_SOURCES}
    types_jscript.cc
    jscript_array_wrapper.cc
    jscript_context.cc
    jscript_function_wrapper.cc
    jscript_map_wrapper.cc
    jscript_object_wrapper.cc
    jscript_type_conversion.cc)
endif()

if(HAVE_PYTHON)
  set(PYTHON_SCRIPTING_SOURCES
    types_python.cc
    python_array_wrapper.cc
    python_context.cc
    python_function_wrapper.cc
    python_map_wrapper.cc
    python_object_wrapper.cc
    python_type_conversion.cc
  )
  set(SCRIPTING_SOURCES
    ${SCRIPTING_SOURCES}
    ${PYTHON_SCRIPTING_SOURCES}
  )

  # dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
  # https://www.python.org/dev/peps/pep-3123/
  # This should be removed when we switch to Python 3.x
  set_source_files_properties(${PYTHON_SCRIPTING_SOURCES} PROPERTIES COMPILE_FLAGS -fno-strict-aliasing)
endif()


add_convenience_library(scripting ${SCRIPTING_SOURCES})

ADD_STAN_TARGET(scripting ${SCRIPTING_SOURCES})
