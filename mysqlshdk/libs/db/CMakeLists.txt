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

set(db_SOURCE
    session.h
    result.h
    row.h
    column.cc
    charset.cc
    session_recorder.cc
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
)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  SET(mysqlsh_tests_SRC
      ${mysqlsh_tests_SRC}
      session_recorder.cc
  )
ENDIF()

add_convenience_library(db ${db_SOURCE})
ADD_STAN_TARGET(db ${db_SOURCE})
