/*
 * Copyright (c) 2015 Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SHELL_RESULTSET_DUMPER_H_
#define _SHELL_RESULTSET_DUMPER_H_

#include <stdlib.h>
#include <iostream>
#include "cmdline_options.h"
#include "modules/base_resultset.h"

class ResultsetDumper
{
public:
  ResultsetDumper(boost::shared_ptr<mysh::BaseResultset>target);
  virtual void dump();

protected:
  boost::shared_ptr<mysh::BaseResultset>_resultset;

  void dump_json(const std::string& format, bool show_warnings);
  void dump_normal(bool interactive, const std::string& format, bool show_warnings);
  void dump_tabbed(shcore::Value::Array_type_ref records);
  void dump_table(shcore::Value::Array_type_ref records);
  void dump_warnings();
};
#endif
