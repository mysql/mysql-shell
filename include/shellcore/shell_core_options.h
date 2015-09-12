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

#ifndef _SHELLCORE_OPTIONS_H_
#define _SHELLCORE_OPTIONS_H_

#include "shellcore/types_cpp.h"

#define SHCORE_OUTPUT_FORMAT "outputFormat"
#define SHCORE_INTERACTIVE "interactive"
#define SHCORE_SHOW_WARNINGS "showWarnings"
#define SHCORE_BATCH_CONTINUE_ON_ERROR "batchContinueOnError"

namespace shcore
{
  class SHCORE_PUBLIC  Shell_core_options :public shcore::Cpp_object_bridge
  {
  public:
    virtual ~Shell_core_options();

    // Retrieves the options directly, to be used from C++
    static Value::Map_type_ref get();

    // Exposes the object to JS/PY to allow custom validations on options
    static boost::shared_ptr<Shell_core_options> get_instance();

    virtual std::string class_name() const;
    virtual bool operator == (const Object_bridge &other) const;
    virtual std::vector<std::string> get_members() const;
    virtual Value get_member(const std::string &prop) const;
    virtual bool has_member(const std::string &prop) const;
    virtual void set_member(const std::string &prop, Value value);
    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;

  private:
    // Private constructor since this is a singleton
    Shell_core_options();

    // Options will be stored on a MAP
    Value::Map_type_ref _options;

    // The only available instance
    static boost::shared_ptr<Shell_core_options> _instance;
  };
};

#endif // _SHELLCORE_OPTIONS_H_
