/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SHELL_SQL_H_
#define _SHELL_SQL_H_

#include "shellcore/shell_core.h"
#include "shellcore/ishell_core.h"
#include "shellcore/common.h"
#include <boost/system/error_code.hpp>
#include <stack>

namespace shcore
{
  class SHCORE_PUBLIC Shell_sql : public Shell_language
  {
  public:
    Shell_sql(IShell_core *owner);
    virtual ~Shell_sql() {};

    virtual void set_global(const std::string &, const Value &) {}

    virtual void handle_input(std::string &code, Interactive_input_state &state, boost::function<void(shcore::Value)> result_processor);

    virtual std::string prompt();

    virtual bool print_help(const std::string& topic);
    void print_exception(const shcore::Exception &e);
    virtual void abort();

  private:
    std::string _sql_cache;
    std::string _delimiter;
    std::stack<std::string> _parsing_context_stack;

    void cmd_process_file(const std::vector<std::string>& params);
  };
};

#endif
