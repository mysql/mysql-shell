/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/scripting/polyglot/polyglot_wrappers/types_polyglot.h"

#include "mysqlshdk/scripting/polyglot/utils/polyglot_api_clean.h"

#include "mysqlshdk/include/scripting/common.h"
#include "mysqlshdk/include/scripting/naming_style.h"
#include "mysqlshdk/include/scripting/object_factory.h"
#include "mysqlshdk/scripting/polyglot/languages/polyglot_language.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_error.h"

namespace shcore {
namespace polyglot {
Polyglot_object::Polyglot_object(const Polyglot_type_bridger *type_bridger,
                                 poly_thread thread, poly_context context,
                                 poly_value object,
                                 const std::string &class_name)
    : m_types{type_bridger},
      m_thread{thread},
      m_context{context},
      m_object{m_thread, object},
      m_class_name{class_name} {}

std::string Polyglot_object::class_name() const { return m_class_name; }

std::string &Polyglot_object::append_descr(std::string &s_out,
                                           int UNUSED(indent),
                                           int UNUSED(quote_strings)) const {
  return s_out.append("<" + class_name() + ">");
}

std::string &Polyglot_object::append_repr(std::string &s_out) const {
  return s_out;
}

std::vector<std::string> Polyglot_object::get_members() const {
  size_t size{0};

  if (const auto rc = poly_value_get_member_keys(
          m_thread, m_context, m_object.get(), &size, nullptr);
      rc != poly_ok) {
    throw Exception::scripting_error(Polyglot_error(m_thread, rc).format());
  }

  std::vector<poly_value> poly_keys;
  poly_keys.resize(size);

  if (const auto rc = poly_value_get_member_keys(
          m_thread, m_context, m_object.get(), &size, &poly_keys[0]);
      rc != poly_ok) {
    throw Exception::scripting_error(Polyglot_error(m_thread, rc).format());
  }

  std::vector<std::string> keys;
  for (auto key : poly_keys) {
    keys.push_back(to_string(m_thread, key));
  }

  return keys;
}

bool Polyglot_object::operator==(const Object_bridge &UNUSED(other)) const {
  return false;
}

bool Polyglot_object::operator!=(const Object_bridge &UNUSED(other)) const {
  return false;
}

Value Polyglot_object::get_property(const std::string &UNUSED(prop)) const {
  return Value();
}

void Polyglot_object::set_property(const std::string &UNUSED(prop),
                                   Value UNUSED(value)) {}

Value Polyglot_object::get_member(const std::string &prop) const {
  try {
    return m_types->poly_value_to_native_value(get_poly_member(prop));
  } catch (const Polyglot_error &error) {
    throw Exception::scripting_error(error.format());
  }
}

poly_value Polyglot_object::get_poly_member(const std::string &prop) const {
  poly_value member;
  throw_if_error(poly_value_get_member, m_thread, m_object.get(), prop.c_str(),
                 &member);

  return member;
}

bool Polyglot_object::has_member(const std::string &prop) const {
  bool found = false;
  if (auto rc =
          poly_value_has_member(m_thread, m_object.get(), prop.c_str(), &found);
      rc != poly_ok) {
    throw Exception::scripting_error(Polyglot_error(m_thread, rc).format());
  }
  return found;
}

void Polyglot_object::set_member(const std::string &prop, Value value) {
  set_poly_member(prop, m_types->native_value_to_poly_value(value));
}

void Polyglot_object::set_poly_member(const std::string &prop,
                                      poly_value value) {
  if (auto rc =
          poly_value_put_member(m_thread, m_object.get(), prop.c_str(), value);
      rc != poly_ok) {
    throw Exception::scripting_error(Polyglot_error(m_thread, rc).format());
  }
}

bool Polyglot_object::is_indexed() const { return false; }

Value Polyglot_object::get_member(size_t UNUSED(index)) const { return {}; }

void Polyglot_object::set_member(size_t UNUSED(index), Value UNUSED(value)) {}

size_t Polyglot_object::length() const { return 0; }

bool Polyglot_object::has_method(const std::string &UNUSED(name)) const {
  return false;
}

Value Polyglot_object::call(const std::string &name,
                            const Argument_list &args) {
  Value ret_val = {};
  try {
    const auto member = get_poly_member(name);

    bool executable = false;
    throw_if_error(poly_value_can_execute, m_thread, member, &executable);

    if (executable) {
      std::vector<poly_value> poly_args;
      poly_args.reserve(args.size());
      for (const auto &arg : args) {
        poly_args.push_back(m_types->native_value_to_poly_value(arg));
      }

      poly_value result;
      throw_if_error(poly_value_execute, m_thread, member, &poly_args[0],
                     poly_args.size(), &result);
      ret_val = m_types->poly_value_to_native_value(result);
    } else {
      throw Exception::attrib_error("Called member " + name +
                                    " is not a function");
    }
  } catch (const Polyglot_error &error) {
    throw Exception::scripting_error(error.format());
  }

  return ret_val;
}

bool Polyglot_object::remove_member(const std::string &name) {
  bool removed{false};

  if (const auto rc = poly_value_remove_member(m_thread, m_object.get(),
                                               name.c_str(), &removed);
      rc != poly_ok) {
    throw Exception::scripting_error(Polyglot_error(m_thread, rc).format());
  }

  return removed;
}

bool Polyglot_object::is_exception() const {
  bool is_exception = false;
  throw_if_error(poly_value_is_exception, m_thread, m_object.get(),
                 &is_exception);

  return is_exception;
}

void Polyglot_object::throw_exception() const {
  const auto rc = poly_value_throw_exception(m_thread, m_object.get());
  throw Polyglot_error(m_thread, rc);
}

Polyglot_function::Polyglot_function(std::weak_ptr<Polyglot_language> language,
                                     poly_value function)
    : m_language(std::move(language)) {
  const auto ctx = m_language.lock();
  if (!ctx) {
    throw std::logic_error(
        "Unable to wrap JavaScript function, context is gone!");
  }

  m_function = ctx->store(function);

  poly_value name;
  throw_if_error(poly_value_get_member, ctx->thread(), m_function, "name",
                 &name);

  m_name = ctx->to_string(name);
}
Polyglot_function::~Polyglot_function() {
  const auto ctx = m_language.lock();
  if (ctx) {
    ctx->erase(m_function);
  }
}

const std::vector<std::pair<std::string, Value_type>>
    &Polyglot_function::signature() const {
  // TODO:
  static std::vector<std::pair<std::string, Value_type>> tmp;
  return tmp;
}

Value_type Polyglot_function::return_type() const {
  // TODO:
  return Undefined;
}

bool Polyglot_function::operator==(const Function_base &UNUSED(other)) const {
  // TODO:
  return false;
}

bool Polyglot_function ::operator!=(const Function_base &UNUSED(other)) const {
  // TODO:
  return false;
}

Value Polyglot_function::invoke(const Argument_list &args) {
  Value ret_val = {};

  const auto ctx = m_language.lock();
  if (!ctx) {
    throw std::logic_error(
        "Unable to execute polyglot function, context is gone!");
  }

  shcore::Scoped_naming_style style(ctx->naming_style());

  std::vector<poly_value> poly_args;
  for (const auto &arg : args) {
    poly_args.push_back(ctx->convert(arg));
  }

  poly_value result;
  if (const auto rc = poly_value_execute(
          ctx->thread(), m_function, &poly_args[0], poly_args.size(), &result);
      rc != poly_ok) {
    throw Exception::scripting_error(
        Polyglot_error(ctx->thread(), rc).format());
  }

  ret_val = ctx->convert(result);

  return ret_val;
}
}  // namespace polyglot
}  // namespace shcore