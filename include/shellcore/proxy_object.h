/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _PROXY_OBJECT_H_
#define _PROXY_OBJECT_H_

#include "shellcore/types_common.h"
#include "types_cpp.h"

namespace shcore {

class SHCORE_PUBLIC Proxy_object : public shcore::Cpp_object_bridge
{
public:
  virtual std::string class_name() const { return "Proxy_object"; }

  Proxy_object(const boost::function<Value (const std::string&)> &delegate);

  virtual Value get_member(const std::string &prop) const;

  virtual bool operator == (const Object_bridge &other) const
  {
    return this == &other;
  }

private:
  boost::function<Value (const std::string&)> _delegate;
};

};

#endif
