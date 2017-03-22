/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "modules/base_session.h"


#ifndef _MODULES_MOD_SHELL_H_
#define _MODULES_MOD_SHELL_H_

namespace mysqlsh {
/**
  * $(SHELL_BRIEF)
  */
  class SHCORE_PUBLIC Shell : public shcore::Cpp_object_bridge {
  public:
    Shell(shcore::IShell_core* owner);
    virtual ~Shell();

    virtual std::string class_name() const { return "Shell"; };
    virtual bool operator == (const Object_bridge &other) const;

    virtual void set_member(const std::string &prop, shcore::Value value);
    virtual shcore::Value get_member(const std::string &prop) const;

    shcore::Value parse_uri(const shcore::Argument_list &args);
    shcore::Value prompt(const shcore::Argument_list &args);
    shcore::Value connect(const shcore::Argument_list &args);

    shcore::Value set_current_schema(const shcore::Argument_list &args);
    shcore::Value set_session(const shcore::Argument_list &args);
    shcore::Value get_session(const shcore::Argument_list &args);
    shcore::Value reconnect(const shcore::Argument_list &args);

    #if DOXYGEN_JS
    Dictionary options;
    Callback customPrompt;
    Dictionary parseUri(String uri);
    String prompt(String message, Dictionary options);
    Undefined connect(ConnectionData connectionData, String password);
    #elif DOXYGEN_PY
    dict options;
    Callback custom_prompt;
    dict parse_uri(str uri);
    str prompt(str message, dict options);
    None connect(ConnectionData connectionData, str password);
    #endif

    std::shared_ptr<mysqlsh::ShellDevelopmentSession> connect_dev_session(const shcore::Argument_list &args, mysqlsh::SessionType session_type);
    std::shared_ptr<mysqlsh::ShellDevelopmentSession> set_dev_session(const std::shared_ptr<mysqlsh::ShellDevelopmentSession>& session);
    std::shared_ptr<mysqlsh::ShellDevelopmentSession> get_dev_session();

  protected:
    void init();

    shcore::Value _custom_prompt[2];

    shcore::IShell_core *_shell_core;

    std::shared_ptr<ShellDevelopmentSession> _global_dev_session;
  };
}

#endif
