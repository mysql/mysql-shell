/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _PROXY_OBJECT_H_
#define _PROXY_OBJECT_H_

#include "scripting/types_common.h"
#include "types_cpp.h"

namespace shcore {
class SHCORE_PUBLIC Proxy_object : public shcore::Cpp_object_bridge {
 public:
  virtual std::string class_name() const { return "Proxy_object"; }

  Proxy_object(const std::function<Value(const std::string &)> &delegate);

  virtual Value get_member(const std::string &prop) const;

  virtual bool operator==(const Object_bridge &other) const {
    return this == &other;
  }

 private:
  std::function<Value(const std::string &)> _delegate;
};
}  // namespace shcore

#endif
