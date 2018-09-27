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
#include <tuple>
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/utils/enumset.h"

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

enum class Print_flag {
  PRINT_0_AS_SPC = 1,
  PRINT_0_AS_ESC = 2,
  PRINT_CTRL = 4
};

using Print_flags =
    mysqlshdk::utils::Enum_set<Print_flag, Print_flag::PRINT_CTRL>;

std::tuple<size_t, size_t> get_utf8_sizes(const char *text, size_t length,
                                          Print_flags flags);

class Resultset_dumper {
 public:
  Resultset_dumper(mysqlshdk::db::IResult *target, bool buffer_data);
  virtual ~Resultset_dumper() = default;
  virtual void dump(const std::string &item_label, bool is_query,
                    bool is_doc_result);

 protected:
  mysqlshdk::db::IResult *_rset;
  std::string _format;
  bool _show_warnings;
  bool _interactive;
  bool _buffer_data;
  bool _cancelled;

  std::string get_affected_stats(const std::string &item_label);
  int get_warning_and_execution_time_stats(std::string &output_stats);
  void dump_records(std::string &output_stats);
  size_t dump_tabbed();
  size_t dump_table();
  size_t dump_vertical();
  size_t dump_documents();
  size_t dump_json(const std::string &item_label, bool is_doc_result);
  void dump_warnings();
};

}  // namespace mysqlsh
#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_RESULTSET_DUMPER_H_
