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


# Loads the metadata-model.sql file
file(READ "adminapi/common/metadata-model.sql" MD_MODEL_SQL)

# Strips the copyright + comments from it
string(FIND "${MD_MODEL_SQL}" "CREATE DATABASE" COPYRIGHT_END)
string(SUBSTRING "${MD_MODEL_SQL}" 0 ${COPYRIGHT_END}+2 COPYRIGHT_TEXT)
string(REPLACE "${COPYRIGHT_TEXT}" "" MD_MODEL_SQL_STRIPPED "${MD_MODEL_SQL}")

# Updates format to be a C++ multiline definition
string(REPLACE "\\" "\\\\" MD_MODEL_SQL_PREPARED "${MD_MODEL_SQL_STRIPPED}")
string(REPLACE "\n" "\\n\"\n\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_PREPARED}")

# Escape the " characters in "manual"
string(REPLACE "\"manual\"" "\\\"manual\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "ssh"
string(REPLACE "\"ssh\"" "\\\"ssh\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "254616cc-fb47-11e5-aac5"
string(REPLACE "\"254616cc-fb47-11e5-aac5\"" "\\\"254616cc-fb47-11e5-aac5\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysql://host.foo.com:3306"
string(REPLACE "\"mysql://host.foo.com:3306\"" "\\\"mysql://host.foo.com:3306\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysqlx://host.foo.com:33060"
string(REPLACE "\"mysqlx://host.foo.com:33060\"" "\\\"mysqlx://host.foo.com:33060\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysql://localhost:/tmp/mysql.sock"
string(REPLACE "\"mysql://localhost:/tmp/mysql.sock\"" "\\\"mysql://localhost:/tmp/mysql.sock\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysqlx://localhost:/tmp/mysqlx.sock"
string(REPLACE "\"mysqlx://localhost:/tmp/mysqlx.sock\"" "\\\"mysqlx://localhost:/tmp/mysqlx.sock\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysqlx://localhost:/tmp/mysqlx.sock"
string(REPLACE "\"mysqlXcom://host.foo.com:49213?channelName=<..>\"" "\\\"mysqlXcom://host.foo.com:49213?channelName=<..>\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")


# Creates the target file containing the code ready for processing
configure_file("${CMAKE_SOURCE_DIR}/modules/adminapi/common/metadata-model_definitions.h.in"
                  "${CMAKE_BINARY_DIR}/modules/adminapi/common/metadata-model_definitions.h")


