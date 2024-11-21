# Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

set(SCRIPTING_SOURCES
    common.cc
    naming_style.cc
    obj_date.cc
    object_factory.cc
    object_registry.cc
    proxy_object.cc
    types.cc
    types_cpp.cc
    shexcept.cc
    type_info/custom.cc)


if (HAVE_POLYGLOT)
    set(SCRIPTING_SOURCES
      ${SCRIPTING_SOURCES}
      polyglot/native_wrappers/polyglot_collectable.cc
      polyglot/native_wrappers/polyglot_iterator_wrapper.cc
      polyglot/native_wrappers/polyglot_object_wrapper.cc
      polyglot/native_wrappers/polyglot_function_wrapper.cc
      polyglot/native_wrappers/polyglot_array_wrapper.cc
      polyglot/native_wrappers/polyglot_map_wrapper.cc
      polyglot/native_wrappers/polyglot_file_system_wrapper.cc
      polyglot/native_wrappers/polyglot_seekable_channel_wrapper.cc
      polyglot/polyglot_wrappers/types_polyglot.cc
      polyglot/utils/polyglot_scope.cc
      polyglot/utils/polyglot_store.cc
      polyglot/utils/polyglot_error.cc
      polyglot/utils/polyglot_utils.cc
      polyglot/polyglot_type_conversion.cc
      polyglot/languages/polyglot_language.cc
      polyglot/languages/polyglot_common_context.cc
      polyglot/languages/polyglot_garbage_collector.cc
      polyglot/polyglot_context.cc
      polyglot/shell_polyglot_common_context.cc
      polyglot/shell_javascript.cc
      )
endif()

# Will generate the js_core_definitions file which will contain the
# core.js file in a variable.
if(HAVE_JS)
  # Copy the core.js file to another directory.
  # This ensures that it's treated as a dependency, allowing to reconfigure
  # the jscript_core_definitions.h header file whenever core.js is changed.
  configure_file("${CMAKE_SOURCE_DIR}/mysqlshdk/scripts/core.js"
                 "${CMAKE_BINARY_DIR}/scripting/core.js"
                 COPYONLY)

  set(SCRIPTING_SOURCES
    ${SCRIPTING_SOURCES}
    polyglot/languages/polyglot_javascript.cc
  )


  # Loads the core.js file
  FILE(READ "${CMAKE_BINARY_DIR}/scripting/core.js" JS_CORE_MODULE)

  # Strips the copyright notice from it
  string(FIND "${JS_CORE_MODULE}" "function" COPYRIGHT_END)
  string(SUBSTRING "${JS_CORE_MODULE}" 0 ${COPYRIGHT_END}+2 COPYRIGHT_TEXT)
  string(REPLACE "${COPYRIGHT_TEXT}" "" JS_CORE_MODULE_STRIPPED "${JS_CORE_MODULE}")

  # Creates the target file containing the code ready for processing
  configure_file("${CMAKE_SOURCE_DIR}/mysqlshdk/scripting/jscript_core_definitions.h.in"
                 "${CMAKE_BINARY_DIR}/scripting/jscript_core_definitions.h")

endif()

if(HAVE_PYTHON)
  include_directories("${PYTHON_INCLUDE_DIRS}")
  set(PYTHON_SCRIPTING_SOURCES
    types_python.cc
    python_array_wrapper.cc
    python_context.cc
    python_function_wrapper.cc
    python_map_wrapper.cc
    python_object_wrapper.cc
    python_type_conversion.cc
    python_utils.cc
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
