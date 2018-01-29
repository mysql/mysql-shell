/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/types_cpp.h"
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <limits>
#include <tuple>
#include "scripting/common.h"
#include "shellcore/utils_help.h"
#include "utils/utils_general.h"

#ifdef WIN32
#ifdef max
#undef max
#endif
#endif

// TODO(alfredo) - remove all the linear lookups on the member names
// TODO(alfredo) - clarify property mechanism

using namespace std::placeholders;
using namespace shcore;

Cpp_function::Raw_signature Cpp_function::gen_signature(
    const std::vector<std::pair<std::string, Value_type>> &args) {
  Raw_signature sig;
  for (auto &i : args) {
    sig.push_back({i.second, i.first[0] == '?' ? Optional : Mandatory});
  }
  return sig;
}

std::pair<bool, int> Cpp_function::match_signatures(
    const Raw_signature &cand, const std::vector<Value_type> &wanted) {
  // this function follows mostly the same rules for finding viable functions
  // as c++ for function overload resolution
  size_t m = wanted.size();
  size_t c = cand.size();
  bool match = true;

  switch (static_cast<int>(cand.size()) - m) {
    case 3:  // 3 params extra, if the rest is optional, it's a match
      match = match && (cand[c - 3].second == Optional);
    case 2:  // 2 params extra, if the rest is optional, it's a match
      match = match && (cand[c - 2].second == Optional);
    case 1:  // 1 param extra, if the rest is optional, it's a match
      match = match && (cand[c - 1].second == Optional);
    case 0:  // # of params match
      break;
    default:
      if (cand.size() < m) {
        return {false, 0};
      }
      assert(0);
      // more than 3 params not supported atm...
      // extend when more expose() variations added
      match = false;
      break;
  }
  if (match) {
    bool have_object_params = false;
    size_t exact_matches = m;
    // check if all provided params are implicitly convertible to the ones
    // from the candidate
    switch (m) {
      case 3:  // 3 params to be considered
        match = match && kTypeConvertible[static_cast<int>(wanted[2])]
                                         [static_cast<int>(cand[2].first)];
        exact_matches -= (wanted[2] != cand[2].first);
        have_object_params = have_object_params || (wanted[2] == Object);
      case 2:  // 2 params to be considered
        match = match && kTypeConvertible[static_cast<int>(wanted[1])]
                                         [static_cast<int>(cand[1].first)];
        exact_matches -= (wanted[1] != cand[1].first);
        have_object_params = have_object_params || (wanted[1] == Object);
      case 1:  // 1 param to be considered
        match = match && kTypeConvertible[static_cast<int>(wanted[0])]
                                         [static_cast<int>(cand[0].first)];
        exact_matches -= (wanted[0] != cand[0].first);
        have_object_params = have_object_params || (wanted[0] == Object);
      case 0:  // 0 params to be considered = nothing to check
        break;
      default:
        assert(0);  // more than 3 params not supported
        match = false;
        break;
    }
    if (exact_matches == m && !have_object_params) {
      return {match, std::numeric_limits<int>::max()};
    } else {
      return {match, have_object_params ? 0 : exact_matches};
    }
  }
  return {false, 0};
}

using FunctionEntry = std::pair<std::string, std::shared_ptr<Cpp_function>>;

std::map<std::string, Cpp_function::Metadata> Cpp_object_bridge::mdtable;

void Cpp_object_bridge::clear_metadata() { mdtable.clear(); }

Cpp_function::Metadata &Cpp_object_bridge::get_metadata(
    const std::string &name) {
  return mdtable[name];
}

void Cpp_object_bridge::set_metadata(
    Cpp_function::Metadata &meta, const std::string &name, Value_type rtype,
    const std::vector<std::pair<std::string, Value_type>> &ptypes) {
  bool found_optional = false;
  // validate optional parameter sanity
  // func(p1, p2=opt, p3=opt) is ok
  // func(p1=opt, p2=opt, p3) is not
  for (const auto &ptype : ptypes) {
    if (ptype.first[0] == '?')
      found_optional = true;
    else if (found_optional)
      throw std::logic_error(
          "optional parameters have to be at the end of param list");
  }
  meta.set(name, rtype, ptypes);
}

void Cpp_function::Metadata::set(
    const std::string &name_, Value_type rtype,
    const std::vector<std::pair<std::string, Value_type>> &ptypes) {
  auto index = name_.find("|");
  if (index == std::string::npos) {
    name[LowerCamelCase] = get_member_name(name_, LowerCamelCase);
    name[LowerCaseUnderscores] = get_member_name(name_, LowerCaseUnderscores);
  } else {
    name[LowerCamelCase] = name_.substr(0, index);
    name[LowerCaseUnderscores] = name_.substr(index + 1);
  }
  signature = Cpp_function::gen_signature(ptypes);

  param_types = ptypes;
  return_type = rtype;
}

Cpp_object_bridge::Cpp_object_bridge() : naming_style(LowerCamelCase) {
  add_varargs_method("help", std::bind(&Cpp_object_bridge::help, this, _1));
}

Cpp_object_bridge::~Cpp_object_bridge() {
  _funcs.clear();
  _properties.clear();
}

std::string &Cpp_object_bridge::append_descr(std::string &s_out, int,
                                             int) const {
  s_out.append("<" + class_name() + ">");
  return s_out;
}

std::string &Cpp_object_bridge::append_repr(std::string &s_out) const {
  return append_descr(s_out, 0, '"');
}

std::vector<std::string> Cpp_object_bridge::get_members_advanced(
    const NamingStyle &style) {
  ScopedStyle ss(this, style);

  return get_members();
}

std::vector<std::string> Cpp_object_bridge::get_members() const {
  std::vector<std::string> members;

  for (const auto &prop : _properties)
    members.push_back(prop.name(naming_style));

  for (const auto &func : _funcs) {
    members.push_back(func.second->name(naming_style));
  }
  return members;
}

std::string Cpp_object_bridge::get_base_name(const std::string &member) const {
  std::string ret_val;
  auto style = naming_style;
  auto func = lookup_function(member, style);
  if (func) {
    ret_val = func->name(NamingStyle::LowerCamelCase);
  } else {
    auto prop = std::find_if(_properties.begin(), _properties.end(),
                             [member, style](const Cpp_property_name &p) {
                               return p.name(style) == member;
                             });
    if (prop != _properties.end())
      ret_val = (*prop).name(NamingStyle::LowerCamelCase);
  }

  return ret_val;
}

std::string Cpp_object_bridge::get_function_name(const std::string &member,
                                                 bool fully_specified) const {
  auto m = lookup_function(member, naming_style);
  std::string name;
  if (!m) {
    name = get_member_name(member, naming_style);
  } else {
    name = m->name(naming_style);
  }
  if (fully_specified) {
    return class_name() + "." + name;
  } else {
    return name;
  }
}

shcore::Value Cpp_object_bridge::get_member_method(
    const shcore::Argument_list &args, const std::string &method,
    const std::string &prop) {
  args.ensure_count(0, get_function_name(method).c_str());

  return get_member_advanced(get_member_name(prop, naming_style), naming_style);
}

Value Cpp_object_bridge::get_member_advanced(const std::string &prop,
                                             const NamingStyle &style) const {
  Value ret_val;

  auto func = std::find_if(_funcs.begin(), _funcs.end(),
                           [prop, style](const FunctionEntry &f) {
                             return f.second->name(style) == prop;
                           });

  if (func != _funcs.end()) {
    ret_val = Value(std::shared_ptr<Function_base>(func->second));
  } else {
    auto prop_index = std::find_if(_properties.begin(), _properties.end(),
                                   [prop, style](const Cpp_property_name &p) {
                                     return p.name(style) == prop;
                                   });
    if (prop_index != _properties.end()) {
      ScopedStyle ss(this, style);
      ret_val = get_member((*prop_index).base_name());
    } else
      throw Exception::attrib_error("Invalid object member " + prop);
  }

  return ret_val;
}

Value Cpp_object_bridge::get_member(const std::string &prop) const {
  std::map<std::string, std::shared_ptr<Cpp_function>>::const_iterator i;
  if ((i = _funcs.find(prop)) != _funcs.end()) {
    return Value(std::shared_ptr<Function_base>(i->second));
  }
  throw Exception::attrib_error("Invalid object member " + prop);
}

bool Cpp_object_bridge::has_member_advanced(const std::string &prop,
                                            const NamingStyle &style) const {
  if (lookup_function(prop, style)) return true;

  auto prop_index = std::find_if(_properties.begin(), _properties.end(),
                                 [prop, style](const Cpp_property_name &p) {
                                   return p.name(style) == prop;
                                 });
  return (prop_index != _properties.end());
}

bool Cpp_object_bridge::has_member(const std::string &prop) const {
  if (lookup_function(prop, NamingStyle::LowerCamelCase)) return true;

  auto prop_index = std::find_if(
      _properties.begin(), _properties.end(),
      [prop](const Cpp_property_name &p) { return p.base_name() == prop; });
  return (prop_index != _properties.end());
}

void Cpp_object_bridge::set_member_advanced(const std::string &prop,
                                            Value value,
                                            const NamingStyle &style) {
  auto prop_index = std::find_if(_properties.begin(), _properties.end(),
                                 [prop, style](const Cpp_property_name &p) {
                                   return p.name(style) == prop;
                                 });
  if (prop_index != _properties.end()) {
    ScopedStyle ss(this, style);

    set_member((*prop_index).base_name(), value);
  } else {
    throw Exception::attrib_error("Can't set object member " + prop);
  }
}

void Cpp_object_bridge::set_member(const std::string &prop, Value) {
  throw Exception::attrib_error("Can't set object member " + prop);
}

bool Cpp_object_bridge::is_indexed() const { return false; }

Value Cpp_object_bridge::get_member(size_t) const {
  throw Exception::attrib_error("Can't access object members using an index");
}

void Cpp_object_bridge::set_member(size_t, Value) {
  throw Exception::attrib_error("Can't set object member using an index");
}

bool Cpp_object_bridge::has_method(const std::string &name) const {
  auto method_index = _funcs.find(name);

  return method_index != _funcs.end();
}

bool Cpp_object_bridge::has_method_advanced(const std::string &name,
                                            const NamingStyle &style) const {
  if (lookup_function(name, style)) return true;
  return false;
}

void Cpp_object_bridge::add_method_(
    const std::string &name, Cpp_function::Function func,
    std::vector<std::pair<std::string, Value_type>> *signature) {
  auto f = _funcs.find(name);
  if (f != _funcs.end()) {
#ifndef NDEBUG
    log_warning("Attempt to register a duplicate method: %s", name.c_str());
#endif
    // overloading not supported in old API, erase the previous one
    _funcs.erase(f);
  }

  auto function =
      std::shared_ptr<Cpp_function>(new Cpp_function(name, func, *signature));
  function->is_legacy = true;
  _funcs.emplace(name.substr(0, name.find("|")), function);
}

void Cpp_object_bridge::add_varargs_method(const std::string &name,
                                           Cpp_function::Function func) {
  auto f = _funcs.find(name);
  if (f != _funcs.end()) {
#ifndef NDEBUG
    log_warning("Attempt to register a duplicate method: %s", name.c_str());
#endif
    // overloading not supported in old API, erase the previous one
    _funcs.erase(f);
  }
  auto function =
      std::shared_ptr<Cpp_function>(new Cpp_function(name, func, true));
  function->is_legacy = true;
  _funcs.emplace(name.substr(0, name.find("|")), function);
}

void Cpp_object_bridge::add_constant(const std::string &name) {
  _properties.push_back(Cpp_property_name(name, true));
}

void Cpp_object_bridge::add_property(const std::string &name,
                                     const std::string &getter) {
  _properties.push_back(Cpp_property_name(name));
  if (!getter.empty())
    add_method(getter, std::bind(&Cpp_object_bridge::get_member_method, this,
                                 _1, getter, name));
}

void Cpp_object_bridge::delete_property(const std::string &name,
                                        const std::string &getter) {
  auto prop_index = std::find_if(
      _properties.begin(), _properties.end(),
      [name](const Cpp_property_name &p) { return p.base_name() == name; });
  if (prop_index != _properties.end()) {
    _properties.erase(prop_index);

    if (!getter.empty()) _funcs.erase(getter);
  }
}

Value Cpp_object_bridge::call_advanced(const std::string &name,
                                       const Argument_list &args,
                                       const NamingStyle &style) {
  auto func = lookup_function_overload(name, style, args);
  if (func) {
    ScopedStyle ss(this, style);
    return func->invoke(args);
  } else {
    throw Exception::attrib_error("Invalid object function " + name);
  }
}

std::shared_ptr<Cpp_function> Cpp_object_bridge::lookup_function(
    const std::string &method) const {
  return lookup_function(method, naming_style);
}

std::shared_ptr<Cpp_function> Cpp_object_bridge::lookup_function(
    const std::string &method, const NamingStyle &style) const {
  // NOTE this linear lookup is no good, but needed until the naming style
  // mechanism is improved
  std::multimap<std::string, std::shared_ptr<Cpp_function>>::const_iterator i;
  for (i = _funcs.begin(); i != _funcs.end(); ++i) {
    if (i->second->name(style) == method) break;
  }
  if (i == _funcs.end()) {
    return std::shared_ptr<Cpp_function>(nullptr);
  }
  // ignore the overloads and just return first match...
  return i->second;
}

std::shared_ptr<Cpp_function> Cpp_object_bridge::lookup_function_overload(
    const std::string &method, const NamingStyle &style,
    const shcore::Argument_list &args) const {
  // NOTE this linear lookup is no good, but needed until the naming style
  // mechanism is improved
  std::multimap<std::string, std::shared_ptr<Cpp_function>>::const_iterator i;
  for (i = _funcs.begin(); i != _funcs.end(); ++i) {
    if (i->second->name(style) == method) break;
  }
  if (i == _funcs.end()) {
    throw Exception::attrib_error("Invalid object function " + method);
  }

  std::vector<Value_type> arg_types;
  for (const auto &arg : args) {
    arg_types.push_back(arg.type);
  }
  // find best matching function taking param types into account
  // by using rules similar to those from C++
  // we don't bother trying to optimize much, since overloads are rare
  // and won't have more than 2 or 3 candidates

  std::vector<std::pair<int, std::shared_ptr<Cpp_function>>> candidates;
  while (i != _funcs.end() && i->second->name(style) == method) {
    if (i->second->is_legacy) return i->second;

    bool match;
    int score;
    std::tie(match, score) = Cpp_function::match_signatures(
        i->second->function_signature(), arg_types);
    if (match) {
      candidates.push_back({score, i->second});
    }
    ++i;
  }
  if (candidates.size() == 1) {
    return candidates[0].second;
  } else if (candidates.size() > 1) {
#ifndef NDEBUG
    for (const auto &cand : candidates) {
      log_info("Candidates: %s(%i)", cand.second->name().c_str(),
               static_cast<int>(cand.second->signature().size()));
    }
#endif
    // multiple matches, find best one by the closest implicit conversion
    std::sort(
        candidates.begin(), candidates.end(),
        [](const std::pair<int, std::shared_ptr<Cpp_function>> &a,
           const std::pair<int, std::shared_ptr<Cpp_function>> &b) -> bool {
          return a.first > b.first;
        });
    // TODO(alfredo) object matching not supported yet...
    throw Exception::attrib_error("Call to overloaded function " +
                                  get_function_name(method, true) +
                                  " has ambiguous candidates.");
  }
  throw Exception::attrib_error("Call to " + get_function_name(method, true) +
                                " has wrong parameter count/types.");
}

Value Cpp_object_bridge::call(const std::string &name,
                              const Argument_list &args) {
  auto func = lookup_function_overload(name, LowerCamelCase, args);
  assert(func);
  return func->invoke(args);
}

shcore::Value Cpp_object_bridge::help(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("help").c_str());

  std::string ret_val;
  std::string item;

  try {
    if (args.size() == 1) item = args.string_at(0);
  } catch (const Exception &e) {
    throw Exception::type_error(get_function_name("help") + ": " + e.what());
  }

  IShell_core::Mode mode;

  if (naming_style == NamingStyle::LowerCamelCase)
    mode = IShell_core::Mode::JavaScript;
  else
    mode = IShell_core::Mode::Python;

  Help_manager help;
  help.set_mode(mode);

  shcore::Topic_mask mask;
  std::string pattern = class_name();
  if (!item.empty()) {
    pattern += "." + item;
    mask.set(shcore::Topic_type::FUNCTION);
    mask.set(shcore::Topic_type::PROPERTY);
    mask.set(shcore::Topic_type::OBJECT);
    mask.set(shcore::Topic_type::CLASS);
  } else {
    mask.set(shcore::Topic_type::MODULE);  // i.e. mysql, mysqlx
    mask.set(shcore::Topic_type::OBJECT);  // i.e. Shell, Options
    mask.set(shcore::Topic_type::CLASS);   // i.e. Anything else
  }

  ret_val = help.get_help(pattern, mask);

  return shcore::Value(ret_val);
}

void Cpp_object_bridge::detect_overload_conflicts(
    const std::string &name, const Cpp_function::Metadata &md) {
  const auto &function_sig = md.signature;
  auto range = _funcs.equal_range(name);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second->is_legacy)
      throw Exception::attrib_error("Attempt to overload legacy function: " +
                                    name);

    const auto &overload_sig = it->second->function_signature();
    int diff = static_cast<int>(overload_sig.size()) -
               static_cast<int>(function_sig.size());
    if (diff < 0) {
      size_t i = overload_sig.size();
      for (; i < function_sig.size(); i++)
        if (function_sig[i].second != Cpp_function::Optional) break;
      if (i < function_sig.size()) continue;
    } else if (diff > 0) {
      size_t i = function_sig.size();
      for (; i < overload_sig.size(); i++)
        if (overload_sig[i].second != Cpp_function::Optional) break;
      if (i < overload_sig.size()) continue;
    }
    size_t i = 0;
    size_t args_num = std::min(overload_sig.size(), function_sig.size());
    for (; i < args_num; i++)
      if (!kTypeConvertible[static_cast<int>(overload_sig[i].first)]
                           [static_cast<int>(function_sig[i].first)] &&
          !kTypeConvertible[static_cast<int>(function_sig[i].first)]
                           [static_cast<int>(overload_sig[i].first)])
        break;
    if (i == args_num)
      throw Exception::attrib_error(
          "Ambiguous overload detected for funtion: " + name);
  }
}

void Cpp_object_bridge::set_naming_style(const NamingStyle &style) {
  naming_style = style;
}

std::shared_ptr<Cpp_object_bridge::ScopedStyle>
Cpp_object_bridge::set_scoped_naming_style(const NamingStyle &style) {
  std::shared_ptr<Cpp_object_bridge::ScopedStyle> ss(
      new Cpp_object_bridge::ScopedStyle(this, style));

  return ss;
}

//-------

Cpp_function::Cpp_function(const Metadata *meta, const Function &func)
    : _func(func), _meta(meta) {}

// TODO(alfredo) legacy, delme
Cpp_function::Cpp_function(const std::string &name_, const Function &func,
                           bool var_args)
    : _func(func) {
  // The | separator is used when specific names are given for a function
  // Otherwise the function name is retrieved based on the style
  auto index = name_.find("|");
  if (index == std::string::npos) {
    _meta_tmp.name[LowerCamelCase] = get_member_name(name_, LowerCamelCase);
    _meta_tmp.name[LowerCaseUnderscores] =
        get_member_name(name_, LowerCaseUnderscores);
  } else {
    _meta_tmp.name[LowerCamelCase] = name_.substr(0, index);
    _meta_tmp.name[LowerCaseUnderscores] = name_.substr(index + 1);
  }
  _meta_tmp.var_args = var_args;
  _meta = &_meta_tmp;
}

// TODO(alfredo) legacy, delme
Cpp_function::Cpp_function(
    const std::string &name_, const Function &func,
    const std::vector<std::pair<std::string, Value_type>> &args)
    : _func(func) {
  // The | separator is used when specific names are given for a function
  // Otherwise the function name is retrieved based on the style
  auto index = name_.find("|");
  if (index == std::string::npos) {
    _meta_tmp.name[LowerCamelCase] = get_member_name(name_, LowerCamelCase);
    _meta_tmp.name[LowerCaseUnderscores] =
        get_member_name(name_, LowerCaseUnderscores);
  } else {
    _meta_tmp.name[LowerCamelCase] = name_.substr(0, index);
    _meta_tmp.name[LowerCaseUnderscores] = name_.substr(index + 1);
  }
  _meta_tmp.param_types = args;
  _meta_tmp.var_args = false;
  _meta_tmp.signature = Cpp_function::gen_signature(args);
  _meta = &_meta_tmp;
}

const std::string &Cpp_function::name() const {
  return _meta->name[LowerCamelCase];
}

const std::string &Cpp_function::name(const NamingStyle &style) const {
  return _meta->name[style];
}

const std::vector<std::pair<std::string, Value_type>> &Cpp_function::signature()
    const {
  return _meta->param_types;
}

Value_type Cpp_function::return_type() const { return _meta->return_type; }

bool Cpp_function::operator==(const Function_base &UNUSED(other)) const {
  throw Exception::logic_error("Cannot compare function objects");
  return false;
}

static std::string num_args_expected(
    const std::vector<std::pair<std::string, Value_type>> &argt) {
  size_t min_args = argt.size();
  for (auto i = argt.rbegin(); i != argt.rend(); ++i) {
    if (i->first[0] == '?' || i->first[0] == '*') --min_args;
  }
  if (min_args != argt.size()) {
    return std::to_string(min_args) + " to " + std::to_string(argt.size());
  }
  return std::to_string(min_args);
}

Value Cpp_function::invoke(const Argument_list &args) {
  // Check that the list of arguments is correct
  if (!_meta->signature.empty() && !is_legacy) {
    auto a = args.begin();
    auto sig = signature();
    auto s = sig.begin();
    int n = 0;
    while (s != sig.end()) {
      if (a == args.end()) {
        if (s->first[0] == '?') {
          // param is optional... remaining expected params can only be optional
          // too
          break;
        } else {
          throw Exception::argument_error("Too few arguments for " + name() +
                                          "()" + ": " + num_args_expected(sig) +
                                          " expected, but got " +
                                          std::to_string(args.size()));
        }
      }
      try {
        a->check_type(s->second);
      } catch (...) {
        throw Exception::argument_error(
            "Argument " + s->first + " at pos " + std::to_string(n) + " for " +
            name() + "() has wrong type: expected " + type_name(s->second) +
            " but got " + type_name(a->type));
      }
      ++n;
      ++a;
      ++s;
    }
    if (a != args.end()) {
      throw Exception::argument_error("Too many arguments for " + name() +
                                      "()" + ": " + num_args_expected(sig) +
                                      " expected, but got " +
                                      std::to_string(args.size()));
    }
  }
  // Note: exceptions caught here should all be self-descriptive and be enough
  // for the caller to figure out what's wrong. Other specific exception
  // types should have been caught earlier, in the bridges
  try {
    return _func(args);
  } catch (shcore::Exception &e) {
    // shcore::Exception can be thrown by bridges
    throw;
  } catch (std::invalid_argument &e) {
    throw Exception::argument_error(e.what());
  } catch (std::logic_error &e) {
    throw Exception::logic_error(e.what());
  } catch (std::runtime_error &e) {
    throw Exception::runtime_error(e.what());
  } catch (std::exception &e) {
    throw Exception::logic_error(std::string("Uncaught exception: ") +
                                 e.what());
  }
}

std::shared_ptr<Function_base> Cpp_function::create(
    const std::string &name, const Function &func,
    const std::vector<std::pair<std::string, Value_type>> &args) {
  return std::shared_ptr<Function_base>(new Cpp_function(name, func, args));
}

Cpp_property_name::Cpp_property_name(const std::string &name, bool constant) {
  // The | separator is used when specific names are given for a function
  // Otherwise the function name is retrieved based on the style
  auto index = name.find("|");
  if (index == std::string::npos) {
    _name[LowerCamelCase] =
        get_member_name(name, constant ? Constants : LowerCamelCase);
    _name[LowerCaseUnderscores] =
        get_member_name(name, constant ? Constants : LowerCaseUnderscores);
  } else {
    _name[LowerCamelCase] = name.substr(0, index);
    _name[LowerCaseUnderscores] = name.substr(index + 1);
  }
}

std::string Cpp_property_name::name(const NamingStyle &style) const {
  return _name[style];
}

std::string Cpp_property_name::base_name() const {
  return _name[LowerCamelCase];
}
