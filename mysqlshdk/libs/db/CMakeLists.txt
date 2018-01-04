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

set(db_SOURCE
    session.h
    result.h
    row.h
    column.cc
    charset.cc
    uri_parser.cc
    uri_encoder.cc
    connection_options.cc
    ssl_options.cc
    utils_connection.cc
    row_copy.cc
    mysql/session.cc
    mysql/result.cc
    mysql/row.cc
    mysqlx/xsession.cc
    mysqlx/xresult.cc
    mysqlx/xrow.cc
    mysqlx/orderby_parser.cc
    mysqlx/tokenizer.cc
    mysqlx/expr_parser.cc
    mysqlx/proj_parser.cc
    replay/setup.cc
    replay/recorder.cc
    replay/replayer.cc
    replay/trace.cc
)


IF(CMAKE_BUILD_TYPE MATCHES Debug)
  SET(mysqlsh_tests_SRC
      ${mysqlsh_tests_SRC}
      session_recorder.cc
  )
ENDIF()

add_convenience_library(db ${db_SOURCE})
ADD_STAN_TARGET(db ${db_SOURCE})
