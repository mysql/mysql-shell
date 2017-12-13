/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
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
