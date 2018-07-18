/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_RESULTSET_DUMPER_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_RESULTSET_DUMPER_H_

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <string>
#include "modules/devapi/base_resultset.h"
#include "scripting/lang_base.h"

namespace mysqlsh {
namespace mysqlx {
class SqlResult;
class RowResult;
class Result;
class DocResult;
};  // namespace mysqlx

namespace mysql {
class ClassicResult;
};
}  // namespace mysqlsh
class ResultsetDumper {
 public:
  ResultsetDumper(std::shared_ptr<mysqlsh::ShellBaseResult> target,
                  shcore::Interpreter_delegate *output_handler,
                  bool buffer_data);
  virtual ~ResultsetDumper() = default;
  virtual void dump();

 protected:
  shcore::Interpreter_delegate *_output_handler;
  std::shared_ptr<mysqlsh::ShellBaseResult> _resultset;
  std::string _format;
  bool _show_warnings;
  bool _interactive;
  bool _buffer_data;
  bool _cancelled;

  void dump_json();
  void dump_normal();
  void dump_normal(std::shared_ptr<mysqlsh::mysql::ClassicResult> result);
  void dump_normal(std::shared_ptr<mysqlsh::mysqlx::SqlResult> result);
  void dump_normal(std::shared_ptr<mysqlsh::mysqlx::RowResult> result);
  void dump_normal(std::shared_ptr<mysqlsh::mysqlx::DocResult> result);
  void dump_normal(std::shared_ptr<mysqlsh::mysqlx::Result> result);

  std::string get_affected_stats(const std::string &legend);
  int get_warning_and_execution_time_stats(std::string &output_stats);
  void dump_records(std::string &output_stats);
  size_t dump_tabbed(shcore::Value::Array_type_ref records);
  size_t dump_table(shcore::Value::Array_type_ref records);
  size_t dump_vertical(shcore::Value::Array_type_ref records);
  void dump_warnings(bool classic = false);
};
#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_RESULTSET_DUMPER_H_
