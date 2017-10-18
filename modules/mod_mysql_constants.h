/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

// Interactive Expression access module
// Exposed as "Expression" in the shell

#ifndef MODULES_MOD_MYSQL_CONSTANTS_H_
#define MODULES_MOD_MYSQL_CONSTANTS_H_

#include <memory>
#include <string>
#include "modules/mod_common.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

namespace mysqlsh {
namespace mysql {
/**
* \ingroup mysql
* Constants to represent data types on Column objects
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
  virtual std::string class_name() const { return "mysql.Type"; }
  virtual bool operator==(const Object_bridge &other) const {
    return this == &other;
  }

  virtual shcore::Value get_member(const std::string &prop) const;

  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);
};

}  // namespace mysql
}  // namespace mysqlsh

#endif  // MODULES_MOD_MYSQL_CONSTANTS_H_
