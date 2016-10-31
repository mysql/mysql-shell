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

#ifndef _MODULES_ADMINAPI_MOD_DBA_COMMON_
#define _MODULES_ADMINAPI_MOD_DBA_COMMON_

#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "modules/adminapi/mod_dba_provisioning_interface.h"

namespace mysqlsh {
namespace dba {

shcore::Value::Map_type_ref get_instance_options_map(const shcore::Argument_list &args, bool get_password_from_options = false);
void resolve_instance_credentials(const shcore::Value::Map_type_ref& options, shcore::Interpreter_delegate* delegate = nullptr);
std::string get_mysqlprovision_error_string(const shcore::Value::Array_type_ref& errors);

}
}
#endif