/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_JSCRIPT_ITERATOR_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_JSCRIPT_ITERATOR_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/include_v8.h"
#include "mysqlshdk/include/scripting/types_cpp.h"

namespace shcore {

template <typename T>
inline shcore::Value get_value(const typename T::const_iterator &ci) {
  return *ci;
}

template <>
inline shcore::Value get_value<Value::Map_type>(
    const typename Value::Map_type::const_iterator &ci) {
  const auto v = shcore::make_array();

  v->emplace_back(ci->first);
  v->emplace_back(ci->second);

  return Value{v};
}

template <typename T>
class JScript_iterator : public shcore::Cpp_object_bridge {
 public:
  explicit JScript_iterator(const std::shared_ptr<T> &data)
      : m_data(data), m_size(m_data->size()), m_next(m_data->begin()) {
    expose("next", &JScript_iterator::next);
  }

  std::string class_name() const override { return "Native_iterator"; }

 private:
  shcore::Dictionary_t next() {
    const auto dict = shcore::make_dict();
    dict->emplace("done", !has_next());
    dict->emplace("value", get_next());
    return dict;
  }

  void validate() {
    if (m_data && m_size != m_data->size()) {
      // invalid size makes sure that this state is remembered
      m_size = static_cast<size_t>(-1);
      throw std::logic_error("size changed during iteration!");
    }
  }

  bool has_next() const { return m_data && m_next != m_data->end(); }

  shcore::Value get_next() {
    validate();

    if (has_next()) {
      const auto v = get_value<T>(m_next);
      m_next = std::next(m_next);

      if (!has_next()) {
        m_data.reset();
      }

      return v;
    } else {
      return {};
    }
  }

  std::shared_ptr<T> m_data;
  size_t m_size;
  typename T::const_iterator m_next;
};

// T has to be Collectable
template <typename T>
void add_iterator(v8::Local<v8::ObjectTemplate> templ, v8::Isolate *isolate) {
  auto f = v8::FunctionTemplate::New(
      isolate, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
        v8::Local<v8::Object> obj = info.Holder();
        const auto collectable =
            static_cast<T *>(obj->GetAlignedPointerFromInternalField(1));
        const auto &data = collectable->data();
        const auto context = collectable->context();

        info.GetReturnValue().Set(context->shcore_value_to_v8_value(
            Value(std::make_shared<JScript_iterator<typename T::Type>>(data))));
      });
  f->RemovePrototype();

  templ->Set(v8::Symbol::GetIterator(isolate), f,
             static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontEnum |
                                                v8::DontDelete));
}

};  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_JSCRIPT_ITERATOR_H_
