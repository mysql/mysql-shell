/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/base_session.h"
#include "scripting/object_factory.h"
#include "shellcore/shell_core.h"
#include "scripting/lang_base.h"
#include "scripting/common.h"
#include "scripting/proxy_object.h"
#include "shellcore/interrupt_handler.h"
#include "utils/debug.h"
#include "utils/utils_general.h"
#include "utils/utils_file.h"
#include "utils/utils_string.h"
#include "mysqlshdk/include/shellcore/utils_help.h"

using namespace mysqlsh;
using namespace shcore;

DEBUG_OBJ_ENABLE(ShellBaseSession);

ShellBaseSession::ShellBaseSession() : _tx_deep(0) {
  DEBUG_OBJ_ALLOC(ShellBaseSession);
}

ShellBaseSession::ShellBaseSession(const ShellBaseSession& s) :
_connection_options(s._connection_options), _tx_deep(s._tx_deep) {
  DEBUG_OBJ_ALLOC(ShellBaseSession);
}

ShellBaseSession::~ShellBaseSession() {
  DEBUG_OBJ_DEALLOC(ShellBaseSession);
}

std::string &ShellBaseSession::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const {
  if (!is_open())
    s_out.append("<" + class_name() + ":disconnected>");
  else
    s_out.append("<" + class_name() + ":" + uri() + ">");
  return s_out;
}

std::string &ShellBaseSession::append_repr(std::string &s_out) const {
  return append_descr(s_out, false);
}

void ShellBaseSession::append_json(shcore::JSON_dumper& dumper) const {
  dumper.start_object();

  dumper.append_string("class", class_name());
  dumper.append_bool("connected", is_open());

  if (is_open())
    dumper.append_string("uri", uri());

  dumper.end_object();
}

bool ShellBaseSession::operator == (const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

std::string ShellBaseSession::get_quoted_name(const std::string& name) {
  size_t index = 0;
  std::string quoted_name(name);

  while ((index = quoted_name.find("`", index)) != std::string::npos) {
    quoted_name.replace(index, 1, "``");
    index += 2;
  }

  quoted_name = "`" + quoted_name + "`";

  return quoted_name;
}

std::string ShellBaseSession::uri(
    mysqlshdk::db::uri::Tokens_mask format) const {
  return _connection_options.as_uri(format);
}

std::string ShellBaseSession::get_default_schema() {
  return _connection_options.has_schema() ?
         _connection_options.get_schema() :
         "";
}

void ShellBaseSession::reconnect() {
  connect(_connection_options);
}

shcore::Object_bridge_ref ShellBaseSession::get_schema(const std::string &name) {
  shcore::Object_bridge_ref ret_val;
  std::string type = "Schema";

  if (name.empty())
    throw Exception::runtime_error("Schema name must be specified");

  std::string found_name = db_object_exists(type, name, "");

  if (!found_name.empty()) {
    update_schema_cache(found_name, true);

    ret_val = (*_schemas)[found_name].as_object();
  } else {
    update_schema_cache(name, false);

    throw Exception::runtime_error("Unknown database '" + name + "'");
  }

  return ret_val;
}

void ShellBaseSession::begin_query() {
  if (_guard_active++ == 0) {
    // Install kill query as ^C handler
    Interrupts::push_handler([this]() {
      kill_query();
      return true;
    });
  }
}

void ShellBaseSession::end_query() {
  if (--_guard_active == 0) {
    Interrupts::pop_handler();
  }
}
