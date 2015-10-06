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

namespace mysh
{
  namespace mysqlx
  {
    class SqlResult;
    class RowResult;
    class Result;
    class DocResult;
  };

  namespace mysql
  {
    class ClassicResult;
  };
}
class ResultsetDumper
{
public:
  ResultsetDumper(boost::shared_ptr<mysh::ShellBaseResult>target);
  virtual void dump();

protected:
  boost::shared_ptr<mysh::ShellBaseResult>_resultset;
  std::string _format;
  bool _show_warnings;
  bool _interactive;

  void dump_json();
  void dump_normal();
  void dump_normal(boost::shared_ptr<mysh::mysql::ClassicResult> result);
  void dump_normal(boost::shared_ptr<mysh::mysqlx::SqlResult> result);
  void dump_normal(boost::shared_ptr<mysh::mysqlx::RowResult> result);
  void dump_normal(boost::shared_ptr<mysh::mysqlx::Result> result);

  std::string get_affected_stats(const std::string& member, const std::string &legend);
  int get_warning_and_execution_time_stats(std::string& output_stats);
  void dump_records(std::string& output_stats);
  void dump_tabbed(shcore::Value::Array_type_ref records);
  void dump_table(shcore::Value::Array_type_ref records);
  void dump_warnings();
};
#endif
