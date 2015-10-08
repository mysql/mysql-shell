/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SHELLCORE_PYTHON_H_
#define _SHELLCORE_PYTHON_H_

#include "shellcore/shell_core.h"

namespace shcore {
  class Python_context;

  class Shell_python : public Shell_language
  {
  public:
    Shell_python(Shell_core *shcore);
    virtual ~Shell_python();

    virtual void set_global(const std::string &name, const Value &value);

    virtual std::string preprocess_input_line(const std::string &s);
    virtual void handle_input(std::string &code, Interactive_input_state &state, boost::function<void(shcore::Value)> result_processor);

    virtual std::string prompt();
  private:
    boost::shared_ptr<Python_context> _py;
  };
};

#endif
