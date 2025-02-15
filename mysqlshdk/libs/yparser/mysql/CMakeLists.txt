# Copyright (c) 2024, Oracle and/or its affiliates.
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

find_package(Python3 REQUIRED COMPONENTS Interpreter)

message(STATUS "Python3 interpreter (build only): ${Python3_EXECUTABLE}")

# disable bison warnings generated by 5.7 grammar
SET(BISON_FLAGS_WARNINGS "${BISON_FLAGS_WARNINGS},no-empty-rule,no-deprecated,no-other")

set(PLUGIN_INSTALL_DIR "${INSTALL_LIBDIR}/yparser")

function(create_parser_plugin)
    # parse arguments
    set(options)
    set(oneValueArgs PATH VERSION)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # constants
    set(STRIP_YACC "${CMAKE_CURRENT_SOURCE_DIR}/yacc/scripts/strip_yacc.py")
    set(COPY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/yacc/scripts/copy_file.py")

    set(INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ARG_VERSION}")
    set(OUTPUT_DIR "${INCLUDE_DIR}/sql")

    GET_MAJOR_MINOR("${ARG_VERSION}" VERSION_MAJOR_MINOR)
    set(PLUGIN_NAME "mysql_${VERSION_MAJOR_MINOR}")

    if(VERSION_MAJOR_MINOR EQUAL "5.7")
        set(PLUGIN_IS_5_7 "1")
    else()
        set(PLUGIN_IS_5_7 "0")
    endif()

    # create output directory
    file(MAKE_DIRECTORY "${OUTPUT_DIR}")

    # preprocess input YY files
    set(YACC_IN "${ARG_PATH}/sql_yacc.yy")
    set(YACC_OUT "${OUTPUT_DIR}/sql_yacc.yy")
    add_custom_command(
        OUTPUT "${YACC_OUT}"
        COMMAND "${Python3_EXECUTABLE}" ARGS "${STRIP_YACC}" "${YACC_IN}" "${YACC_OUT}" "mysqlsh_sql_parser_"
        DEPENDS "${STRIP_YACC}" "${YACC_IN}"
    )
    BISON_TARGET(
        sql_yacc_${ARG_VERSION}
        "${YACC_OUT}"
        "${OUTPUT_DIR}/sql_yacc.cc"
        COMPILE_FLAGS "--yacc ${BISON_FLAGS_WARNINGS} ${BISON_NO_LINE_OPT}"
        DEFINES_FILE "${OUTPUT_DIR}/sql_yacc.h"
    )

    set(HINTS_IN "${ARG_PATH}/sql_hints.yy")
    set(HINTS_OUT "${OUTPUT_DIR}/sql_hints.yy")
    add_custom_command(
        OUTPUT "${HINTS_OUT}"
        COMMAND "${Python3_EXECUTABLE}" ARGS "${STRIP_YACC}" "${HINTS_IN}" "${HINTS_OUT}" "mysqlsh_hint_parser_" "--strip-yyundef"
        DEPENDS "${STRIP_YACC}" "${HINTS_IN}"
    )
    BISON_TARGET(
        sql_hints_${ARG_VERSION}
        "${HINTS_OUT}"
        "${OUTPUT_DIR}/sql_hints.yy.cc"
        COMPILE_FLAGS "--yacc ${BISON_FLAGS_WARNINGS} ${BISON_NO_LINE_OPT}"
        DEFINES_FILE "${OUTPUT_DIR}/sql_hints.yy.h"
    )

    # holds source files from the server
    set(MYSQL_SOURCES
        "${OUTPUT_DIR}/sql_yacc.h"
        "${OUTPUT_DIR}/sql_yacc.cc"
        "${OUTPUT_DIR}/sql_hints.yy.h"
        "${OUTPUT_DIR}/sql_hints.yy.cc"
    )

    # BISON_TARGET (as opposed to add_custom_command()), does not mark produced files as generated
    set_property(
        SOURCE ${MYSQL_SOURCES}
        PROPERTY GENERATED 1
    )

    # lex.h is also needed by the plugin, but it's set to be a dependency only of the gen_lex_hash to avoid a cycle
    set(LEX_IN "${ARG_PATH}/lex.h")
    set(LEX_OUT "${OUTPUT_DIR}/lex.h")
    add_custom_command(
        OUTPUT "${LEX_OUT}"
        COMMAND "${Python3_EXECUTABLE}" ARGS "${COPY_FILE}" "${LEX_IN}" "${LEX_OUT}"
        DEPENDS "${COPY_FILE}" "${LEX_IN}"
    )

    # binary which generates lex_hash.h
    set(GEN_LEX_HASH_IN "${MYSQL_SOURCE_DIR}/sql/gen_lex_hash.cc")
    set(GEN_LEX_HASH_OUT "${OUTPUT_DIR}/gen_lex_hash.cc")
    add_custom_command(
        OUTPUT "${GEN_LEX_HASH_OUT}"
        COMMAND "${Python3_EXECUTABLE}" ARGS "${COPY_FILE}" "${GEN_LEX_HASH_IN}" "${GEN_LEX_HASH_OUT}"
        DEPENDS "${COPY_FILE}" ${MYSQL_SOURCES} "${LEX_OUT}" "${GEN_LEX_HASH_IN}"
    )

    set(GEN_LEX_HASH "gen_lex_hash_${VERSION_MAJOR_MINOR}")
    add_executable(${GEN_LEX_HASH} "${GEN_LEX_HASH_OUT}")
    target_include_directories(${GEN_LEX_HASH} BEFORE PRIVATE
        "${INCLUDE_DIR}"
    )

    # generate lex_hash.h
    set(LEX_HASH_OUT "${OUTPUT_DIR}/lex_hash.h")
    add_custom_command(
        OUTPUT "${LEX_HASH_OUT}"
        COMMAND "$<TARGET_FILE:${GEN_LEX_HASH}>" > "${LEX_HASH_OUT}"
        DEPENDS ${GEN_LEX_HASH}
    )
    list(APPEND MYSQL_SOURCES "${LEX_HASH_OUT}")

    # we need to copy the static source files to the output directory, because these need to be compiled per each version
    set(MYSQL_SOURCES_IN
        "${MYSQL_SOURCE_DIR}/sql/sql_lex_hash.cc"
        # Use lexer from Router, which is derived from server lexer with simpler deps
        "${MYSQL_SOURCE_DIR}/router/src/routing/src/sql_lexer.cc"
        "${MYSQL_SOURCE_DIR}/router/src/routing/src/sql_lexer_thd.cc"
    )
    foreach(SRC ${MYSQL_SOURCES_IN})
        get_filename_component(FILENAME "${SRC}" NAME)
        add_custom_command(
            OUTPUT "${OUTPUT_DIR}/${FILENAME}"
            COMMAND "${Python3_EXECUTABLE}" ARGS "${COPY_FILE}" "${SRC}" "${OUTPUT_DIR}/${FILENAME}"
            DEPENDS "${COPY_FILE}" "${SRC}"
        )

        # append copied files to the list of MySQL source files
        list(APPEND MYSQL_SOURCES "${OUTPUT_DIR}/${FILENAME}")
    endforeach()

    # don't report warnings as errors, disable known warnings
    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set_property(
            SOURCE ${MYSQL_SOURCES} "${GEN_LEX_HASH_OUT}"
            APPEND_STRING
            PROPERTY COMPILE_FLAGS "-Wno-error -Wno-format -Wno-format-y2k -Wno-format-security -Wno-format-pedantic -Wno-unused-but-set-variable -Wno-shadow -Wno-overflow -Wno-free-nonheap-object"
        )
    endif()

    # our source files, also need to be copied
    set(PLUGIN_SOURCES "")
    set(PLUGIN_SOURCES_IN
        "${CMAKE_CURRENT_SOURCE_DIR}/yacc/parser_interface.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/yacc/sql_lex_hints.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/parser.cc"
    )
    foreach(SRC ${PLUGIN_SOURCES_IN})
        get_filename_component(FILENAME "${SRC}" NAME)
        add_custom_command(
            OUTPUT "${OUTPUT_DIR}/${FILENAME}"
            COMMAND "${Python3_EXECUTABLE}" ARGS "${COPY_FILE}" "${SRC}" "${OUTPUT_DIR}/${FILENAME}"
            DEPENDS "${COPY_FILE}" "${SRC}"
        )

        # append copied files to the list of plugin source files
        list(APPEND PLUGIN_SOURCES "${OUTPUT_DIR}/${FILENAME}")
    endforeach()

    if(WIN32)
        generate_rc_file(NAME "${PLUGIN_NAME}.dll" DESCRIPTION "Parser of MySQL ${ARG_VERSION} grammar." OUTPUT_DIR "${OUTPUT_DIR}" OUT_RC_FILE RC_FILE)
        list(APPEND PLUGIN_SOURCES "${RC_FILE}")
    endif()

    # create the plugin
    add_library(${PLUGIN_NAME} MODULE ${MYSQL_SOURCES} ${PLUGIN_SOURCES})
    target_compile_definitions(${PLUGIN_NAME} PRIVATE
        "BUILDING_PARSER_PLUGIN"
        "PARSER_VERSION=\"${ARG_VERSION}\""
        "PARSER_IS_5_7=${PLUGIN_IS_5_7}"
        "USE_OPTIMIZER_HINTS_PARSER"
        "MYSQLCLIENT_IS_STATIC"
    )
    if(WIN32)
        # needed to correctly import symbols from the MySQL strings library (i.e. my_charset_utf8mb4_0900_ai_ci)
        target_compile_definitions(${PLUGIN_NAME} PRIVATE
            "MYSQL_LIBSTRINGS_DLL"
        )
    endif()
    target_include_directories(${PLUGIN_NAME} BEFORE PRIVATE
        "${INCLUDE_DIR}"
        "${MYSQL_SOURCE_DIR}/include"
        "${MYSQL_SOURCE_DIR}/libs"
        "${MYSQL_SOURCE_DIR}/router/src/routing/src"
    )

    target_link_libraries(${PLUGIN_NAME} mysql_deps)

    if(NOT WIN32)
        if(APPLE)
            set(PLUGIN_RPATH "@loader_path")
        else()
            set(PLUGIN_RPATH "\$ORIGIN")
        endif()

        set_target_properties(${PLUGIN_NAME} PROPERTIES BUILD_RPATH "${PLUGIN_RPATH}")
        set_target_properties(${PLUGIN_NAME} PROPERTIES INSTALL_RPATH "${PLUGIN_RPATH}")

        # report undefined symbols
        if(APPLE)
            # using '-undefined' with macOS 14 (clang 15) generates a warning, 'error' is the default value
            target_link_options(${PLUGIN_NAME} PRIVATE "-Wl,-undefined,error")
        else()
            target_link_options(${PLUGIN_NAME} PRIVATE "-Wl,--no-undefined")
        endif()
    endif()

    # hide all symbols except for exported ones
    set_target_properties(${PLUGIN_NAME} PROPERTIES C_VISIBILITY_PRESET hidden)
    set_target_properties(${PLUGIN_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
    set_target_properties(${PLUGIN_NAME} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

    # plugin's name is: mysql_MAJOR.MINOR.{so|dll}
    set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
    if(WIN32)
        set_target_properties(${PLUGIN_NAME} PROPERTIES SUFFIX ".dll")
    else()
        set_target_properties(${PLUGIN_NAME} PROPERTIES SUFFIX ".so")
    endif()

    # install the plugin
    install(TARGETS ${PLUGIN_NAME} LIBRARY COMPONENT main DESTINATION "${PLUGIN_INSTALL_DIR}")
    set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PLUGIN_INSTALL_DIR}")
    # needed by multi-configuration generators (i.e. Visual Studio)
    foreach(CONF ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${CONF}" CONF_UPPER)
        set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${CONF_UPPER} "${CMAKE_BINARY_DIR}/${CONF}/${PLUGIN_INSTALL_DIR}")
    endforeach()

    if(NOT WIN32 AND NOT APPLE)
        # test executable
        set(TEST_NAME "parser_test_${VERSION_MAJOR_MINOR}")
        # no need to copy the source file, it uses the public interface of the plugin
        add_executable(${TEST_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/parser_test.cc")
        add_dependencies(${TEST_NAME} ${PLUGIN_NAME})
        target_link_options(${TEST_NAME} PRIVATE "$<TARGET_FILE:${PLUGIN_NAME}>" "-Wl,--allow-shlib-undefined")
    endif()
endfunction()

# combine static libraries into a shared one, reducing the total size
if(WIN32)
    set(EXTRACT_SYMBOLS "${CMAKE_CURRENT_SOURCE_DIR}/yacc/scripts/extract_msvc_symbols.py")
    set(mysql_deps_DEF "${CMAKE_CURRENT_BINARY_DIR}/mysql_deps.def")
    set(mysql_deps_STATIC_LIBS
        "${MYSQL_BUILD_DIR}/archive_output_directory/${CMAKE_BUILD_TYPE}/mysys.lib"
        "${MYSQL_BUILD_DIR}/archive_output_directory/${CMAKE_BUILD_TYPE}/mytime.lib"
        "${MYSQL_BUILD_DIR}/archive_output_directory/${CMAKE_BUILD_TYPE}/strings.lib"
    )

    add_custom_command(
        OUTPUT "${mysql_deps_DEF}"
        COMMAND "${Python3_EXECUTABLE}" ARGS "${EXTRACT_SYMBOLS}" "--out" "${mysql_deps_DEF}" ${mysql_deps_STATIC_LIBS}
        DEPENDS "${EXTRACT_SYMBOLS}" ${mysql_deps_STATIC_LIBS}
    )

    generate_rc_file(NAME "mysql_deps.dll" DESCRIPTION "MySQL libraries - dependencies of parsers." OUT_RC_FILE RC_FILE)

    set(mysql_deps_SOURCES
        yacc/mysql_deps.cc
        "${mysql_deps_DEF}"
        "${RC_FILE}"
    )

    add_library(mysql_deps SHARED ${mysql_deps_SOURCES})

    target_include_directories(mysql_deps BEFORE PRIVATE
        "${MYSQL_SOURCE_DIR}"
    )

    target_link_libraries(mysql_deps PRIVATE ${mysql_deps_STATIC_LIBS})

    # install the library
    install(TARGETS mysql_deps RUNTIME COMPONENT main DESTINATION "${PLUGIN_INSTALL_DIR}")
    fix_target_output_directory(mysql_deps "${PLUGIN_INSTALL_DIR}")
else()
    set(mysql_deps_SOURCES
        yacc/mysql_deps.cc
    )

    add_library(mysql_deps SHARED ${mysql_deps_SOURCES})

    target_include_directories(mysql_deps BEFORE PRIVATE
        "${MYSQL_SOURCE_DIR}"
    )

    target_link_libraries(mysql_deps PRIVATE
        "${MYSQL_BUILD_DIR}/archive_output_directory/libmysys.a"
        "${MYSQL_BUILD_DIR}/archive_output_directory/libmytime.a"
        "${MYSQL_BUILD_DIR}/archive_output_directory/libstrings.a"
    )

    # specify some exports, so that the static libraries are not stripped
    set(mysql_deps_EXPORTS
        my_charset_latin1
        my_charset_utf8mb4_0900_ai_ci
        my_charset_utf8mb4_bin
        my_charset_utf8mb4_general_ci
    )

    if(APPLE)
        list(TRANSFORM mysql_deps_EXPORTS PREPEND "_")
    endif()

    foreach(SYMBOL ${mysql_deps_EXPORTS})
        target_link_options(mysql_deps PRIVATE "-Wl,-u,${SYMBOL}")
    endforeach()

    # install the library
    install(TARGETS mysql_deps LIBRARY COMPONENT main DESTINATION "${PLUGIN_INSTALL_DIR}")
    set_target_properties(mysql_deps PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PLUGIN_INSTALL_DIR}")
endif()

# build libraries for all bundled parsers
set(PARSERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/yacc")
file(GLOB DIR_CONTENTS LIST_DIRECTORIES true "${PARSERS_DIR}/*")
foreach(FULL_PATH ${DIR_CONTENTS})
    get_filename_component(VERSION "${FULL_PATH}" NAME)

    if(IS_DIRECTORY "${FULL_PATH}" AND VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
        create_parser_plugin(PATH "${FULL_PATH}" VERSION "${VERSION}")
    endif()
endforeach()

# parser for the current innovation release
create_parser_plugin(PATH "${MYSQL_SOURCE_DIR}/sql" VERSION "${MYSQL_VERSION}")
