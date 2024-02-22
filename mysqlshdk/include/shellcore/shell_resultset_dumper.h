/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
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
}  // namespace mysqlx

namespace mysql {
class ClassicResult;
}  // namespace mysql

enum class Print_flag {
  PRINT_0_AS_SPC = 1,
  PRINT_0_AS_ESC = 2,
  PRINT_CTRL = 4
};

using Print_flags =
    mysqlshdk::utils::Enum_set<Print_flag, Print_flag::PRINT_CTRL>;

std::tuple<size_t, size_t> get_utf8_sizes(const char *text, size_t length,
                                          Print_flags flags);

/**
 * Interface specifying operations used to output text produced by
 * Resultset_dumper_base class. Implementations may i.e. choose output type
 * (console, string, file), provide additional formatting, etc.
 */
class Resultset_printer {
 public:
  Resultset_printer() = default;
  virtual ~Resultset_printer() = default;

  /**
   * Outputs the given text.
   *
   * @param s - text to output
   */
  virtual void print(const std::string &s) = 0;

  /**
   * Outputs the given text and follows it with a newline character.
   *
   * @param s - text to output
   */
  virtual void println(const std::string &s) = 0;

  /**
   * Outputs the given text skipping any extra formatting.
   *
   * @param s - text to output
   */
  virtual void raw_print(const std::string &s) = 0;

  /**
   * Resets the printer
   */
  virtual void reset() {}

  /**
   * Returns the data on the printer
   */
  virtual std::string data() const { return ""; }
};

#define RESULTSET_DUMPER_FORMATS \
  "table, tabbed, vertical, json, ndjson, json/raw, json/array, json/pretty"
/**
 * Base dumper class which implements text-formatting logic for various output
 * formats. Has no public interface, making it essentially abstract, needs to
 * be configured by the derived class with a Resultset_printer instance.
 */
class Resultset_dumper_base {
 public:
  virtual ~Resultset_dumper_base() = default;

  static bool is_valid_format(const std::string &format) {
    return strstr(", " RESULTSET_DUMPER_FORMATS ", ",
                  (", " + format + ", ").c_str()) != nullptr;
  }

 protected:
  Resultset_dumper_base(mysqlshdk::db::IResult *target,
                        std::unique_ptr<Resultset_printer> printer,
                        const std::string &wrap_json, const std::string &format,
                        bool show_column_type_info = false);

  size_t dump_tabbed();
  size_t dump_table();
  size_t dump_vertical();
  size_t dump_documents(bool is_doc_result);
  virtual bool show_column_type_info() const { return m_show_column_type_info; }
  std::string format_json(const std::string &item_label, bool is_doc_result,
                          bool pretty, int *count);
  virtual size_t dump_json(const std::string &item_label, bool is_doc_result);
  void dump_warnings();
  std::string format_json_metadata(bool pretty);
  void dump_metadata();

  size_t format_vertical(bool has_header, bool align_right,
                         size_t min_label_width);

  mysqlshdk::db::IResult *m_result;
  std::string m_wrap_json;
  std::string m_format;
  bool m_cancelled = false;
  std::unique_ptr<Resultset_printer> m_printer;
  bool m_show_column_type_info;
};

/**
 * Dumps the provided result to a console, choosing the output format based on
 * the current Shell formatting options and interactive mode.
 */
class Resultset_dumper : public Resultset_dumper_base {
 public:
  Resultset_dumper(mysqlshdk::db::IResult *target, const std::string &wrap_json,
                   const std::string &format, bool show_warnings,
                   bool show_stats, bool show_column_type_info = false);
  explicit Resultset_dumper(mysqlshdk::db::IResult *target,
                            bool show_column_type_info = false);

  ~Resultset_dumper() override = default;

  virtual size_t dump(const std::string &item_label, bool is_query,
                      bool is_doc_result);

 protected:
  std::string get_affected_stats(const std::string &item_label);
  int get_warning_and_execution_time_stats(std::string *output_stats);

  bool m_show_warnings;
  bool m_show_stats;
};

/**
 * Dumps the provided result as needed by the GUI to render a proper GUI table
 *
 * NOTE: Eventually, this class will overwrite the following functions to met
 * the GUI requirements for result handling:
 * - dump()
 * - dump_metadata()
 * - dump_json()
 **/
class Gui_resultset_dumper : public Resultset_dumper {
 public:
  Gui_resultset_dumper(mysqlshdk::db::IResult *target,
                       const std::string &format = "");

 protected:
  size_t dump_json(const std::string &item_label, bool is_doc_result) override;
  bool show_column_type_info() const override;
};

/**
 * Dumps the provided result to a string, output format is selected by the user
 * of this class by calling one of the appropriate methods.
 */
class Resultset_writer : public Resultset_dumper_base {
 public:
  explicit Resultset_writer(mysqlshdk::db::IResult *target);
  ~Resultset_writer() override = default;

  std::string write_table();

  std::string write_vertical();

  std::string write_status();

  std::string writer_gui();

 protected:
  Resultset_writer(mysqlshdk::db::IResult *target,
                   std::unique_ptr<Resultset_printer> printer,
                   const std::string &wrap_json,
                   const std::string &result_format,
                   bool show_column_type_info = false);

  std::string write(const std::function<void()> &dump);
};

class Gui_resultset_writer : public Resultset_writer {
 public:
  explicit Gui_resultset_writer(mysqlshdk::db::IResult *target);
};

size_t dump_result(mysqlshdk::db::IResult *target,
                   const std::string &item_label, bool is_query = false,
                   bool is_doc_resule = false,
                   const std::optional<std::string> &wrap_json = {},
                   const std::optional<std::string> &opt_format = {},
                   const std::optional<bool> &show_warnings = {},
                   const std::optional<bool> &show_stats = {},
                   const std::optional<bool> &show_column_type_info = {});

}  // namespace mysqlsh
#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_RESULTSET_DUMPER_H_
