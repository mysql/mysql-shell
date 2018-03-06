/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

// clang-format off
#include "scripting/python_utils.h"  // must be the 1st include
// clang-format on
#include "mysqlshdk/shellcore/provider_python.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <set>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "scripting/python_context.h"
#include "scripting/python_object_wrapper.h"

namespace shcore {
namespace completer {

static std::vector<std::string> k_builtin_keywords = {
    "and",   "as",     "assert", "break",  "class", "continue", "def",
    "del",   "elif",   "else",   "except", "exec",  "finally",  "for",
    "from",  "global", "if",     "import", "in",    "is",       "lambda",
    "not",   "or",     "pass",   "print",  "raise", "return",   "try",
    "while", "with",   "yield"};

/** Placeholders assume JS naming convention. This wrapper handles the
    conversion of Python vs JS naming styles. */
class Placeholder_wrapper : public Object {
 public:
  explicit Placeholder_wrapper(std::shared_ptr<Object> object)
      : real_object_(object) {
  }

  std::string get_type() const override {
    if (auto obj = real_object_.lock())
      return obj->get_type();
    return "";
  }

  bool is_member_callable(const std::string &name) const override {
    if (auto obj = real_object_.lock())
      return obj->is_member_callable(shcore::to_camel_case(name));
    return false;
  }

  std::shared_ptr<Object> get_member(const std::string &name) const override {
    // convert Python to JS convention
    if (auto obj = real_object_.lock())
      return std::shared_ptr<Object>(new Placeholder_wrapper(
          obj->get_member(shcore::to_camel_case(name))));
    return std::shared_ptr<Object>();
  }

  size_t add_completions(const std::string &prefix,
                         Completion_list *list) const override {
    size_t c = 0;
    if (auto obj = real_object_.lock()) {
      Completion_list tmp;
      // omit prefix to get the full member list
      obj->add_completions("", &tmp);
      for (auto n : tmp) {
        n = shcore::from_camel_case(n);
        if (shcore::str_beginswith(n, prefix)) {
          list->push_back(n);
          c++;
        }
      }
    }
    return c;
  }

 private:
  std::weak_ptr<Object> real_object_;
};

class PyObject_proxy : public Object {
 public:
  explicit PyObject_proxy(Provider_python *completer, PyObject *obj)
      : completer_(completer), pyobj_(obj) {
    Py_XINCREF(obj);
  }

  ~PyObject_proxy() {
    Py_XDECREF(pyobj_);
  }

  std::string get_type() const override {
    WillEnterPython lock;
    std::string t;

    std::shared_ptr<Object_bridge> bridged;
    if (shcore::unwrap(pyobj_, bridged) && bridged) {
      t = bridged->class_name();
    } else {
      PyObject *cls = PyObject_GetAttrString(pyobj_, "__class__");
      if (cls) {
        PyObject *name = PyObject_GetAttrString(cls, "__name__");
        if (name) {
          t = PyString_AsString(name);
          Py_XDECREF(name);
        }
        Py_XDECREF(cls);
      }
      PyErr_Clear();
    }
    return t;
  }

  bool is_member_callable(const std::string &name) const override {
    WillEnterPython lock;
    PyObject *mem = PyObject_GetAttrString(pyobj_, name.c_str());
    if (mem) {
      return PyCallable_Check(mem);
    }
    return false;
  }

  std::shared_ptr<Object> get_member(const std::string &name) const override {
    std::shared_ptr<Object> member;

    if (!wrapped_placeholder_) {
      WillEnterPython lock;
      PyObject *mem = PyObject_GetAttrString(pyobj_, name.c_str());
      if (mem) {
        if (PyCallable_Check(mem)) {
          // If the member is a method, we can't list their contents
          // to see the next completions, we need to list the contents of the
          // would be returned value, instead.
          // So instead of using a Proxy to the method, we
          // use a placeholder that knows the members the return type has
          wrapped_placeholder_.reset(new Placeholder_wrapper(
              completer_->object_registry()->lookup(get_type())));
        } else {
          member.reset(new PyObject_proxy(completer_, mem));
        }
        Py_XDECREF(mem);
      }
      PyErr_Clear();
    }
    if (!member && wrapped_placeholder_) {
      member = wrapped_placeholder_->get_member(name);
    }
    return member;
  }

  size_t add_completions(const std::string &prefix,
                         Completion_list *list) const override {
    size_t c = 0;
    WillEnterPython lock;
    // evaluate the string (which is expected to not have any side-effect)
    std::vector<std::pair<bool, std::string>> keys;
    shcore::Python_context::get_members_of(pyobj_, &keys);

    for (auto &i : keys) {
      if (shcore::str_beginswith(i.second, prefix)) {
        list->push_back(i.second + (i.first ? "()" : ""));
        ++c;
      }
    }
    return c;
  }

 private:
  Provider_python *completer_;
  PyObject *pyobj_;
  mutable std::shared_ptr<Object> wrapped_placeholder_;
};

Provider_python::Provider_python(std::shared_ptr<Object_registry> registry,
                                 std::shared_ptr<Python_context> context)
    : Provider_script(registry), context_(context) {
}

Completion_list Provider_python::complete_chain(const Chain &chain_a) {
  // handle globals/toplevel keywords
  Completion_list list;
  Chain chain(chain_a);
  if (chain.size() == 1) {
    std::vector<std::pair<bool, std::string>> globals(context_->list_globals());
    std::string prefix = chain.next().second;

    for (auto &i : globals) {
      if (shcore::str_beginswith(i.second, prefix)) {
        list.push_back(i.second + (i.first ? "()" : ""));
      }
    }
    for (auto &i : k_builtin_keywords) {
      if (shcore::str_beginswith(i, prefix)) {
        list.push_back(i);
      }
    }
  }
  Completion_list more(Provider_script::complete_chain(chain));
  std::copy(more.begin(), more.end(), std::back_inserter(list));
  return list;
}

std::shared_ptr<Object> Provider_python::lookup_global_object(
    const std::string &name) {
  WillEnterPython lock;
  PyObject *obj = context_->get_global_py(name);
  if (obj && obj != Py_None) {
    auto tmp = std::shared_ptr<Object>(new PyObject_proxy(this, obj));
    Py_XDECREF(obj);
    return tmp;
  }
  Py_XDECREF(obj);
  PyErr_Clear();
  return std::shared_ptr<Object>();
}

size_t span_python_string(const std::string &s, size_t p) {
  assert(!s.empty());
  size_t end = s.length();
  int q = s[p];
  bool esc = false;
  bool multiline = false;
  char multiline_q[3];
  assert(q == '"' || q == '\'');

  if (s.length() - p >= 3 &&
      ((strncmp(&s[p], "\"\"\"", 3) == 0) || strncmp(&s[p], "'''", 3) == 0)) {
    strncpy(multiline_q, &s[p], 3);
    multiline = true;
    p += 2;
  }
  // JS/Python like strings, with no double-quote escaping ('')
  while (++p < end) {
    if (!esc) {
      switch (s[p]) {
        case '\\':
          esc = true;
          break;

        case '"':
        case '\'':
          if (s[p] == q) {
            if (multiline) {
              if (s.length() - p >= 3 && strncmp(&s[p], multiline_q, 3) == 0) {
                p += 2;
              } else {
                break;
              }
            }
            return p;
          }
          break;
      }
    } else {
      esc = false;
    }
  }
  return p;
}

/** Parse the given string, and return the last Chain object in the string,
  considering a simplified scripting language syntax.

  completable:: identifier [whitespace*] ("." identifier | "." method_call)*
  method_call:: identifier "(" stuff ")"
  identifier:: ("_" | letters) ("_" | digits | letters)*
  stuff:: (anything | string | completable) stuff*
  */
Provider_script::Chain Provider_python::parse_until(const std::string &s,
                                                    size_t *pos, int close_char,
                                                    size_t *chain_start_pos) {
  size_t end = s.length();
  size_t &p = *pos;
  Provider_script::Chain chain;
  std::string identifier;

  while (p < end && !chain.invalid()) {
    switch (s[p]) {
      case '"':
      case '\'':
        p = span_python_string(s, p);
        break;

      case '{':
      case '[':
      case '(': {
        int closer = 0;
        switch (s[p]) {
          case '[':
            closer = ']';
            break;
          case '{':
            closer = '}';
            break;
          case '(':
            closer = ')';
            break;
        }
        ++p;
        size_t inner_pos = p;
        Provider_script::Chain inner(parse_until(s, &p, closer, &inner_pos));
        // if the inner block was not closed, then it's the last one
        if (p == end || inner.invalid()) {
          *chain_start_pos = inner_pos;
          return inner;
        }
        // otherwise, throw it away, consider it a method call/array/map
        // and continue
        if (!identifier.empty()) {
          if (closer == '}') {
            // this was a dict
            chain.clear();
          } else if (closer == ']' && identifier.empty()) {
            // this was an array
            chain.clear();
          } else if (closer == ')') {
            if (chain.add_method(identifier))
              *chain_start_pos = p - identifier.length();
          } else {
            if (chain.add_variable(identifier))
              *chain_start_pos = p - identifier.length();
          }
          identifier.clear();
        }
        break;
      }

      case '}':
      case ')':
      case ']':
        if (s[p] == close_char)
          return chain;
        chain.clear();
        identifier.clear();
        // unexpected closing thingy, probably bad syntax, but we don't care
        break;

      case '\n':  // a new line is a hard break on the previous part of a stmt
      case ':':   // or end of some other
      case ' ':   // or whitespace
      case '\t':
      case '\r':
        chain.clear();
        identifier.clear();
        break;

      case '#': {  // comment starting
        chain.clear();
        identifier.clear();
        // find end of line
        size_t nl = s.find('\n', p);
        if (nl == std::string::npos) {
          p = end;
        } else {
          p = nl;
        }
        break;
      }

      case '\\': {  // escape outside a string means line continuation..
                    // skip util end of line
        size_t nl = s.find('\n', p);
        if (nl == std::string::npos) {
          chain.clear();
          identifier.clear();
          p = end;
        } else {
          p = nl;
        }
        break;
      }

      case '.':
        if (!identifier.empty()) {
          if (chain.add_variable(identifier))
            *chain_start_pos = p - identifier.length();
        } else {
          // consecutive dots or dot at beginning = syntax error
          if (p == 0 || chain.empty() || s[p - 1] == '.') {
            chain.invalidate();
            return chain;
          }
        }
        identifier.clear();
        chain.add_dot();
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if (!identifier.empty()) {
          identifier.push_back(s[p]);
        } else {  // a number can't appear in a chain, outside an id or param
          chain.clear();
        }
        break;

      case '_':
        identifier.push_back(s[p]);
        break;

      default:
        // if we see any identifier-like char, we're in an identifier
        // anything else is a break
        if (isalpha(s[p])) {
          identifier.push_back(s[p]);
        } else {
          // garbage...
          chain.clear();
          identifier.clear();
        }
        break;
    }
    ++p;
  }
  if (chain.add_variable(identifier))
    *chain_start_pos = p - identifier.length();
  return chain;
}

}  // namespace completer
}  // namespace shcore
