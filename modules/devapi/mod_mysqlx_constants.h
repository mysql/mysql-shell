/*
 * Copyright (c) 2016, 2022, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

// Interactive Expression access module
// Exposed as "Expression" in the shell

#ifndef MODULES_DEVAPI_MOD_MYSQLX_CONSTANTS_H_
#define MODULES_DEVAPI_MOD_MYSQLX_CONSTANTS_H_

#include <memory>
#include <string>
#include "modules/mod_common.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

namespace mysqlsh {
namespace mysqlx {
/**
 * \ingroup mysqlx
 * Constants to represent data types of Column objects
 *
 * Supported Data Types
 *
 *  - BIT
 *  - TINYINT
 *  - SMALLINT
 *  - MEDIUMINT
 *  - INT
 *  - BIGINT
 *  - FLOAT
 *  - DECIMAL
 *  - DOUBLE
 *  - JSON
 *  - STRING
 *  - BYTES
 *  - TIME
 *  - DATE
 *  - DATETIME
 *  - TIMESTAMP
 *  - SET
 *  - ENUM
 *  - GEOMETRY
 */
class SHCORE_PUBLIC Type : public shcore::Cpp_object_bridge {
 public:
  Type();
  // Virtual methods from object bridge
  virtual std::string class_name() const { return "mysqlx.Type"; }
  virtual bool operator==(const Object_bridge &other) const {
    return this == &other;
  }

  virtual shcore::Value get_member(const std::string &prop) const;

  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);
};

/**
 * \ingroup mysqlx
 * Constants to represent lock contention types
 *
 * Supported Lock Contention types for row locking on table.select() and
 * collection.find() operations.
 *
 *  - DEFAULT
 *  - NOWAIT
 *  - SKIP_LOCKED
 */
class SHCORE_PUBLIC LockContention : public shcore::Cpp_object_bridge {
 public:
  LockContention();
  // Virtual methods from object bridge
  virtual std::string class_name() const { return "mysqlx.LockContention"; }
  virtual bool operator==(const Object_bridge &other) const {
    return this == &other;
  }

  virtual shcore::Value get_member(const std::string &prop) const;

  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);
};

}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_CONSTANTS_H_
