/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

// Interactive session access module for MySQL X sessions
// Exposed as "session" in the shell

#ifndef _MOD_MYSQLX_H_
#define _MOD_MYSQLX_H_

#include "shellcore/module_registry.h"

namespace mysh {
namespace mysqlx {
DECLARE_MODULE(Mysqlx, mysqlx);

DECLARE_FUNCTION(get_session);
DECLARE_FUNCTION(get_node_session);
DECLARE_FUNCTION(expr);
DECLARE_FUNCTION(date_value);

virtual shcore::Value get_member(const std::string &prop) const;

private:
  shcore::Object_bridge_ref _type;
  shcore::Object_bridge_ref _index_type;

  END_DECLARE_MODULE();
}
}

#endif
