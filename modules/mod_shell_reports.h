/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_MOD_SHELL_REPORTS_H_
#define MODULES_MOD_SHELL_REPORTS_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "modules/mod_extensible_object.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/include/shellcore/base_session.h"

namespace mysqlsh {

/*
 * Holds information about a report. Immutable type.
 */
class SHCORE_PUBLIC Report final {
 public:
  /*
   * Signature of a function used to format output from a report.
   */
  using Formatter = std::function<std::string(const shcore::Array_t &,
                                              const shcore::Dictionary_t &)>;

  /*
   * Signature of a native report.
   */
  using Native_report = std::function<shcore::Dictionary_t(
      const std::shared_ptr<ShellBaseSession> &, const shcore::Array_t &,
      const shcore::Dictionary_t &)>;

  /*
   * Data structure to hold argument count range.
   */
  using Argc = std::pair<uint32_t, uint32_t>;

  /*
   * Types of reports.
   */
  enum class Type {
    LIST,    //!< List type, output is displayed in tabbed form or vertically.
    REPORT,  //!< Free-form report, output is displayed using YAML.
    PRINT    //!< Output is printed by the report any additional output is
             //!< suppressed.
  };

  /*
   * Holds information about a report's option.
   */
  struct Option : public Parameter_definition {
    Option(const std::string &name,
           shcore::Value_type type = shcore::Value_type::String,
           bool required = false);

    std::string short_name;
  };

  using Options = std::vector<std::shared_ptr<Option>>;

  Report(const std::string &name, Type type,
         const shcore::Function_base_ref &function);

  Report(const std::string &name, Type type, const Native_report &function);

  /*
   * Provides the name of the Report.
   *
   * @returns name of the Report.
   */
  const std::string &name() const;

  /*
   * Provides the type of the Report.
   *
   * @returns type of the Report.
   */
  Type type() const;

  /*
   * Provides the function which invokes the Report.
   *
   * @returns function which invokes the Report.
   */
  const shcore::Function_base_ref &function() const;

  /*
   * Provides the brief description of the Report.
   *
   * @returns brief description of the Report.
   */
  const std::string &brief() const;

  void set_brief(const std::string &brief);

  /*
   * Provides the detailed description of the Report.
   *
   * @returns detailed description of the Report.
   */
  const std::vector<std::string> &details() const;

  void set_details(const std::vector<std::string> &details);

  void set_options(const Options &options);

  /*
   * Provides the expected number of free-form arguments.
   *
   * @returns the expected number of free-form arguments.
   */
  const Argc &argc() const;

  void set_argc(const Argc &argc);

  /*
   * Provides the formatter used to translate result of the Report into human
   * readable text.
   *
   * @returns the formatter used to translate result of the Report into human
   * readable text.
   */
  const Formatter &formatter() const;

  void set_formatter(const Formatter &formatter);

  /*
   * Checks if this report requires 'options' parameter.
   *
   * @returns true if this report requires 'options' parameter.
   */
  bool requires_options() const;

  /*
   * Checks if this report has options.
   *
   * @returns true if this report has options
   */
  bool has_options() const;

 private:
  friend class Shell_reports;

  static void validate_option(const Option &option);

  std::string m_name;
  Type m_type;
  shcore::Function_base_ref m_function;
  std::string m_brief;
  std::vector<std::string> m_details;
  Options m_options;
  Argc m_argc;
  Formatter m_formatter;
};

#if defined(DOXYGEN_JS) || defined(DOXYGEN_PY)
/**
 * \ingroup ShellAPI
 *
 * @brief $(REPORTS_BRIEF)
 *
 * $(REPORTS_DETAIL)
 */
#ifdef DOXYGEN_JS
/**
 * All user-defined reports registered using the shell.registerReport() method
 * are also available here.
 */
#elif DOXYGEN_PY
/**
 * All user-defined reports registered using the shell.register_report() method
 * are also available here.
 */
#endif
/**
 * $(REPORTS_DETAIL2)
 *
 * $(REPORTS_DETAIL3)
 * $(REPORTS_DETAIL4)
 * $(REPORTS_DETAIL5)
 * $(REPORTS_DETAIL6)
 *
 * $(REPORTS_DETAIL7)
 * $(REPORTS_DETAIL8)
 *
 * $(REPORTS_DETAIL9)
 */
class Reports {
 public:
  /**
   * Executes the given SQL statement. This is a 'list' type report.
   *
   * @param session Session object used to execute the given SQL statement.
   * @param argv List of strings constituting the SQL statement to be executed.
   *
   * @return Dictionary containing a single key named 'report' with value being
   *         a list of lists.
   */
  Dict query(Session session, List argv);
};
#endif

/*
 * A Shell module which allows to add, access and execute reports.
 */
class SHCORE_PUBLIC Shell_reports : public Extensible_object {
 public:
  Shell_reports(const std::string &name, const std::string &qualified_name);
  ~Shell_reports() override;

  // exported methods

  /*
   * Registers a new report.
   *
   * @param name - unique name of the report
   * @param type - type of the report
   * @param report - function to invoke when calling the report.
   * @param description - describes the new report.
   *
   * @throws shcore::Exception::argument_error - if a report with the same name
   * already exists
   */
  void register_report(const std::string &name, const std::string &type,
                       const shcore::Function_base_ref &report,
                       const shcore::Dictionary_t &description);

  /*
   * Registers a new report.
   *
   * @param report - a new report.
   *
   * @throws shcore::Exception::argument_error - if a report with the same name
   * already exists
   */
  void register_report(std::unique_ptr<Report> report);

  /*
   * Provides an unsorted list of names of all available reports.
   *
   * @returns list of all available reports.
   */
  std::vector<std::string> list_reports() const;

  /*
   * Calls the specified report and provides its output in text form.
   *
   * @param name - name of the report to be called.
   * @param session - Shell session object to be used by the report.
   * @param args - list of arguments to be parsed and passed to the report.
   *
   * @returns Output of the called report converted to a human-readable text.
   *
   * @throws shcore::Exception::argument_error - if session does not exist or is
   * closed
   * @throws shcore::Exception::argument_error - if specified report does not
   * exist
   * @throws shcore::Exception::runtime_error - if the executed report returns
   * the result of an unexpected type
   */
  std::string call_report(const std::string &name,
                          const std::shared_ptr<ShellBaseSession> &session,
                          const std::vector<std::string> &args);

 private:
  class Report_options;

  std::shared_ptr<Parameter_definition> start_parsing_parameter(
      const shcore::Dictionary_t &definition,
      shcore::Option_unpacker *unpacker) const override;

  std::unordered_map<std::string, std::unique_ptr<Report_options>> m_reports;
};

}  // namespace mysqlsh

#endif  // MODULES_MOD_SHELL_REPORTS_H_
