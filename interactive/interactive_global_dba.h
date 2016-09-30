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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef _INTERACTIVE_GLOBAL_DBA_H_
#define _INTERACTIVE_GLOBAL_DBA_H_

#include "shellcore/interactive_object_wrapper.h"

namespace shcore {
class SHCORE_PUBLIC Global_dba : public Interactive_object_wrapper {
public:
  Global_dba(Shell_core& shell_core) : Interactive_object_wrapper("dba", shell_core) { init(); }

  void init();
  //virtual void resolve() const;

  shcore::Value deploy_sandbox_instance(const shcore::Argument_list &args, const std::string& fname); // create and start
  shcore::Value stop_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value delete_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value kill_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value start_sandbox_instance(const shcore::Argument_list &args);

  shcore::Value create_cluster(const shcore::Argument_list &args);
  shcore::Value get_cluster(const shcore::Argument_list &args);
  shcore::Value drop_metadata_schema(const shcore::Argument_list &args);
  shcore::Value validate_instance(const shcore::Argument_list &args);
  shcore::Value prepare_instance(const shcore::Argument_list &args);

private:
  shcore::Argument_list check_instance_op_params(const shcore::Argument_list &args);
  shcore::Value perform_instance_operation(const shcore::Argument_list &args, const std::string &fname, const std::string& progressive, const std::string& past);
  void validate_session(const std::string &source) const;
};
}

#endif // _INTERACTIVE_GLOBAL_DBA_H_
