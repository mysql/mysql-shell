/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/python_context.h"
#include "shellcore/shell_python.h"
#include "shellcore/python_utils.h"
#include "../modules/base_session.h"

using namespace shcore;

Shell_python::Shell_python(Shell_core *shcore)
  : Shell_language(shcore)
{
  _py = boost::shared_ptr<Python_context>(new Python_context(shcore->lang_delegate()));
}

Shell_python::~Shell_python()
{
  _py.reset();
}

std::string Shell_python::preprocess_input_line(const std::string &s)
{
  const char *p = s.c_str();
  while (*p && isblank(*p))
    ++p;
  if (*p == '#')
    return std::string();
  return s;
}

/*
* Helper function to ensure the exceptions generated on the mysqlx_connector
* are properly translated to the corresponding shcore::Exception type
*/

/*
 * Handle shell input on Python mode
 */
void Shell_python::handle_input(std::string &code, Interactive_input_state &state, boost::function<void(shcore::Value)> result_processor)
{
  Value result;

  if ((*Shell_core_options::get())[SHCORE_INTERACTIVE].as_bool())
  {
    WillEnterPython lock;
    result = _py->execute_interactive(code, state);
  }
  else
  {
    try
    {
      boost::system::error_code err;
      WillEnterPython lock;
      result = _py->execute(code, err, _owner->get_input_source());
      if (err)
      {
        _owner->print_error(err.message());
      }
    }
    catch (Exception &exc)
    {
      // This exception was already printed in PY
      // and the correct return_value of undefined is set
    }
  }
  _last_handled = code;

  // Only processes the result when full statements are executed
  if (state == Input_ok)
    result_processor(result);
}

/*
 * Shell prompt string
 */
std::string Shell_python::prompt()
{
  std::string ret_val = "mysql-py>";

  boost::system::error_code err;
  try
  {
    Interactive_input_state state = Input_ok;
    WillEnterPython lock;
    shcore::Value value = _py->execute_interactive("shell.ps() if 'ps' in dir(shell) else None", state);
    if (value && value.type == String)
      ret_val = value.as_string();
    else
    {
      Value session_wrapper = _owner->active_session();
      if (session_wrapper)
      {
        boost::shared_ptr<mysh::ShellBaseSession> session = session_wrapper.as_object<mysh::ShellBaseSession>();

        if (session)
        {
          shcore::Value st = session->get_capability("node_type");

          if (st)
            ret_val = st.as_string() + "-py>";
        }
      }
    }
  }
  catch (std::exception &exc)
  {
    _owner->print_error(std::string("Exception in PS ps function: ") + exc.what());
  }

  return ret_val;
}

/*
 * Set global variable
 */
void Shell_python::set_global(const std::string &name, const Value &value)
{
  _py->set_global(name, value);
}

void Shell_python::abort()
{
  // TODO:
}
