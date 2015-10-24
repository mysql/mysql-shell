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

#ifndef _SHELL_REGISTRY_H_
#define _SHELL_REGISTRY_H_

#include "shellcore/types_cpp.h"
#include "shellcore/server_registry.h"

namespace shcore
{
  class SHCORE_PUBLIC  StoredSessions :public shcore::Cpp_object_bridge
  {
  public:
    virtual ~StoredSessions();

    // Retrieves the options directly, to be used from C++
    static Value get();

    // Exposes the object to JS/PY to allow custom validations on options
    static boost::shared_ptr<StoredSessions> get_instance();

    // Methods for C++ interface
    bool add_connection(const std::string& name, const std::string& uri, bool overwrite = false);
    bool update_connection(const std::string& name, const std::string& uri);
    bool remove_connection(const std::string& name);
    Value::Map_type_ref connections(){ return _connections; }
  private:
    // Private constructor since this is a singleton
    StoredSessions();

    // Options will be stored on a MAP
    Value::Map_type_ref _connections;

    // The only available instance
    static boost::shared_ptr<StoredSessions> _instance;

    std::string get_options_string(const Value::Map_type_ref& connection);
    Value store_connection(const std::string& name, const Value::Map_type_ref& connection, bool update, bool overwrite);
    Value::Map_type_ref fill_connection(const std::string& uri);
    Value::Map_type_ref fill_connection(const Connection_options& options);

    virtual std::string class_name() const;
    virtual bool operator == (const Object_bridge &other) const;
    virtual std::vector<std::string> get_members() const;
    virtual Value get_member(const std::string &prop) const;
    virtual bool has_member(const std::string &prop) const;
    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
    virtual void append_json(shcore::JSON_dumper& dumper) const;

    Value add(const Argument_list &args);
    Value remove(const Argument_list &args);
    Value update(const Argument_list &args);

    void backup_passwords(Value::Map_type *pwd_backups) const;
    void restore_passwords(Value::Map_type *pwd_backups) const;
  };
};

#endif // _SHELLCORE_OPTIONS_H_
