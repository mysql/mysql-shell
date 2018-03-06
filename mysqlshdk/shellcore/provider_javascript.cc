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

#include "mysqlshdk/shellcore/provider_javascript.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <set>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "scripting/jscript_context.h"

namespace shcore {
namespace completer {

static std::vector<std::string> k_builtin_keywords = {
    "break",    "case",       "catch",  "class",    "const", "continue",
    "debugger", "default",    "delete", "do",       "else",  "export",
    "extends",  "finally",    "for",    "function", "if",    "import",
    "in",       "instanceof", "new",    "return",   "super", "switch",
    "this",     "throw",      "try",    "typeof",   "var",   "void",
    "while",    "with",       "yield"};

class JavaScript_proxy : public Object {
 public:
  explicit JavaScript_proxy(Provider_javascript *completer, const JSObject &obj,
                            const std::string &obj_class, bool callable)
      : completer_(completer), jsobj_(obj), jsobj_class_(obj_class) {
  }

  std::string get_type() const override {
    return jsobj_class_;
  }

  bool is_member_callable(const std::string &name) const override {
    if (wrapped_placeholder_) {
      return wrapped_placeholder_->is_member_callable(name);
    } else {
      bool callable;
      JSObject object;
      std::string object_type;

      std::tie(callable, object, object_type) =
          completer_->context_->get_member_of(&jsobj_, name);

      return callable;
    }
  }

  std::shared_ptr<Object> get_member(const std::string &name) const override {
    std::shared_ptr<Object> member;

    bool callable;
    JSObject object;
    std::string object_type;

    std::tie(callable, object, object_type) =
        completer_->context_->get_member_of(&jsobj_, name);
    if (!name.empty()) {
      if (callable || object_type.empty()) {
        // If the member is a method, we can't list their contents
        // to see the next completions, we need to list the contents of the
        // would be returned value, instead.
        // So instead of using a Proxy to the method, we
        // use a placeholder that knows the members the return type has
        wrapped_placeholder_ =
            completer_->object_registry()->lookup(get_type());
      } else {
        member.reset(
            new JavaScript_proxy(completer_, object, object_type, callable));
      }
    }
    if (!member && wrapped_placeholder_) {
      member = wrapped_placeholder_->get_member(name);
    }
    return member;
  }

  size_t add_completions(const std::string &prefix,
                         Completion_list *list) const override {
    size_t c = 0;

    // evaluate the string (which is expected to not have any side-effect)
    std::vector<std::pair<bool, std::string>> keys;

    keys = completer_->context_->get_members_of(&jsobj_);
    for (const auto &key : keys) {
      if (shcore::str_beginswith(key.second, prefix)) {
        list->push_back(key.second + (key.first ? "()" : ""));
        ++c;
      }
    }
    return c;
  }

 private:
  Provider_javascript *completer_;
  JSObject jsobj_;
  std::string jsobj_class_;

  mutable std::shared_ptr<Object> wrapped_placeholder_;
};

Provider_javascript::Provider_javascript(
    std::shared_ptr<Object_registry> registry,
    std::shared_ptr<JScript_context> context)
    : Provider_script(registry), context_(context) {
}

Completion_list Provider_javascript::complete_chain(const Chain &chain_a) {
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

std::shared_ptr<Object> Provider_javascript::lookup_global_object(
    const std::string &name) {
  JSObject obj;
  std::string obj_type;
  std::tie(obj, obj_type) = context_->get_global_js(name);
  if (!obj.IsEmpty()) {
    return std::shared_ptr<Object>(
        new JavaScript_proxy(this, obj, obj_type, obj_type.empty()));
  }
  return std::shared_ptr<Object>();
}

/** Parse the given string, and return the last Chain object in the string,
  considering a simplified scripting language syntax.

  completable:: identifier [whitespace*] ("." identifier | "." method_call)*
  method_call:: identifier "(" stuff ")"
  identifier:: ("_" | letters) ("_" | digits | letters)*
  stuff:: (anything | string | completable) stuff*
  */
Provider_script::Chain Provider_javascript::parse_until(
    const std::string &s, size_t *pos, int close_char,
    size_t *chain_start_pos) {
  size_t end = s.length();
  size_t &p = *pos;
  Provider_script::Chain chain;
  std::string identifier;

  while (p < end && !chain.invalid()) {
    switch (s[p]) {
      case '"':
        p = mysqlshdk::utils::span_quoted_string_dq(s, p);
        if (p == std::string::npos) {
          chain.invalidate();
          return chain;
        }
        --p;
        break;

      case '\'':
        p = mysqlshdk::utils::span_quoted_string_sq(s, p);
        if (p == std::string::npos) {
          chain.invalidate();
          return chain;
        }
        --p;
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
      case ' ':   // or whitespace
      case '\t':
      case '\r':
        chain.clear();
        identifier.clear();
        break;

      case '/':  // comment starting
        if (p + 1 < end) {
          if (s[p + 1] == '/') {
            // skip until EOL
            size_t nl = s.find('\n', p);
            if (nl == std::string::npos) {
              chain.clear();
              identifier.clear();
              p = end;
            } else {
              p = nl;
            }
          } else if (s[p + 1] == '*') {
            size_t eoc = mysqlshdk::utils::span_cstyle_comment(s, p);
            if (eoc == std::string::npos) {
              chain.invalidate();
              return chain;
            } else {
              p = eoc;
            }
          }
        }
        break;

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
