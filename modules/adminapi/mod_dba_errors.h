/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MODULES_ADMINAPI_MOD_DBA_ERRORS_H_
#define MODULES_ADMINAPI_MOD_DBA_ERRORS_H_

#include "scripting/types.h"

// AdminAPI / InnoDB cluster error codes

#define SHERR_DBA_FIRST 0x10000

// Errors caused by bad user input / arguments
#define SHERR_DBA_INSTANCE_NOT_IN_CLUSTER 0x10000

// Errors casued by cluster state

// Errors caused by DB errors

// Errors caused by system errors

#define SHERR_DBA_LAST 0x20000

namespace mysqlsh {
namespace dba {
inline shcore::Exception make_error(int code, const std::string &summary,
                                    const std::string &description,
                                    const std::string &cause) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("AdminAPI");
  (*error)["message"] = Value(summary);
  (*error)["description"] = Value(description);
  (*error)["cause"] = Value(cause);
  (*error)["code"] = Value(code);
  return Exception(error);
}

inline shcore::Exception make_error(int code, const std::string &summary,
                                    const std::string &description,
                                    const std::string &cause,
                                    const std::exception_ptr &root_exception) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("AdminAPI");
  (*error)["message"] = Value(summary);
  (*error)["description"] = Value(description);
  (*error)["cause"] = Value(cause);
  (*error)["code"] = Value(code);
  return Exception(error, root_exception);
}
}
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_ERRORS_H_
