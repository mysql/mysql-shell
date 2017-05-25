/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/shell_jscript.h"
#include "scripting/jscript_context.h"
#include "shellcore/base_session.h"
#include "modules/devapi/mod_mysqlx_session.h"

using namespace shcore;

Shell_javascript::Shell_javascript(Shell_core *shcore)
  : Shell_language(shcore) {
  _js = std::shared_ptr<JScript_context>(new JScript_context(shcore->registry(), shcore->get_delegate()));
}

void Shell_javascript::handle_input(std::string &code, Input_state &state,
    std::function<void(shcore::Value)> result_processor) {
  // Undefined to be returned in case of errors
  Value result;

  if ((*Shell_core_options::get())[SHCORE_INTERACTIVE].as_bool())
    result = _js->execute_interactive(code, state);
  else {
    try {
      result = _js->execute(code, _owner->get_input_source(), _owner->get_input_args());
    } catch (std::exception &exc) {
      _owner->print_error(exc.what());
    }
  }

  _last_handled = code;

  result_processor(result);
}

std::string Shell_javascript::prompt() {
  try {
    shcore::Value value = _js->execute("shell.customPrompt ? shell.customPrompt() : null", "shell.customPrompt");
    if (value && value.type == String)
      return value.as_string();
  } catch (std::exception &exc) {
    _owner->print_error(std::string("Exception in JS ps function: ") + exc.what());
  }

  std::string node_type = "mysql";
  std::shared_ptr<mysqlsh::ShellBaseSession> session = _owner->get_dev_session();

  if (session)
    node_type = session->get_node_type();

  return node_type + "-js> ";
}

void Shell_javascript::set_global(const std::string &name, const Value &value) {
  _js->set_global(name, value);
}

void Shell_javascript::abort() {
  /*
  // TODO: this way to gather the session is wrong in JS, because there sessions are typically created with getNodeSession

  Value session_wrapper = _owner->active_session();
  std::shared_ptr<mysqlsh::ShellBaseSession> session = session_wrapper.as_object<mysqlsh::ShellBaseSession>();
  // duplicate the connection
  std::shared_ptr<mysqlsh::mysqlx::BaseSession> kill_session = NULL;
  mysqlsh::mysqlx::NodeSession* node_session = dynamic_cast<mysqlsh::mysqlx::NodeSession*>(session.get());
  mysqlsh::mysqlx::XSession* x_session = dynamic_cast<mysqlsh::mysqlx::XSession*>(session.get());

  if (node_session != NULL)
  {
  kill_session.reset(new mysqlsh::mysqlx::NodeSession(*node_session));
  }
  else if (x_session != NULL)
  {
  kill_session.reset(new mysqlsh::mysqlx::XSession(*x_session));
  }
  else
  throw std::runtime_error("Unexpected session type");

  uint64_t connection_id = session->get_connection_id();
  if (connection_id != 0)
  {
  shcore::Argument_list a;
  a.push_back(shcore::Value(""));
  kill_session->connect(a);
  if (!kill_session)
  {
  throw std::runtime_error(boost::format().str());
  }
  std::string cmd = (boost::format("kill query %u") % connection_id).str();
  a.clear();
  kill_session->execute_sql(cmd, a);
  }*/
}
