# Copyright (c) 2022, Oracle and/or its affiliates.
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

SET(parser_GEN_SOURCE
    mysql/MySQLLexer.cpp
    mysql/MySQLParser.cpp
    mysql/MySQLParserBaseListener.cpp
    mysql/MySQLParserBaseVisitor.cpp
    mysql/MySQLParserListener.cpp
    mysql/MySQLParserVisitor.cpp
)

SET(parser_SOURCE
    base/symbol-info.cpp
    code-completion/CodeCompletionCore.cpp
    code-completion/mysql-code-completion.cpp
    code-completion/mysql_code_completion_api.cc
    code-completion/mysql_code_completion_context.cc
    server/sql_modes.cc
    MySQLBaseLexer.cpp
    MySQLBaseRecognizer.cpp
    MySQLRecognizerCommon.cpp
    parsers-common.cpp
    SymbolTable.cpp
    mysql_parser_utils.cc
    ${parser_GEN_SOURCE}
)

IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set_property(
    SOURCE ${parser_GEN_SOURCE}
    APPEND_STRING
    PROPERTY COMPILE_FLAGS "-Wno-unused-parameter -Wno-shadow"
  )
ENDIF()

IF(WIN32)
  add_definitions(-DPARSERS_EXPORTS)
  set_property(
    SOURCE ${parser_GEN_SOURCE}
    APPEND_STRING
    PROPERTY COMPILE_FLAGS "/bigobj"
  )
ENDIF()

# Disable debug symbols in 32 bit platforms
IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  ADD_CONVENIENCE_LIBRARY(parser ${parser_SOURCE}
    COMPILE_OPTIONS "-g0")
ELSE()
  ADD_CONVENIENCE_LIBRARY(parser ${parser_SOURCE})
ENDIF()

ADD_STAN_TARGET(parser ${parser_SOURCE})
