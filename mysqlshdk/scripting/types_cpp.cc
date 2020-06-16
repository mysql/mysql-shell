/*
 * Copyright (c) 2014, 2020, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/scripting/types_cpp.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <limits>
#include <tuple>

#include "mysqlshdk/include/scripting/common.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_general.h"

#ifdef WIN32
#ifdef max
#undef max
#endif
#endif

// TODO(alfredo) - remove all the linear lookups on the member names
// TODO(alfredo) - clarify property mechanism

using namespace std::placeholders;
namespace shcore {

namespace {
std::tuple<bool, std::vector<std::string>> process_keyword_args(
    const shcore::Cpp_function::Raw_signature &cand,
    const shcore::Dictionary_t &kwds) {
  std::set<std::string> kwd_names;
  std::set<std::string> params;

  // First of all, we ensure all the provided keywords are valid parameters
  for (const auto &param : cand) {
    params.insert(param->name);
  }

  for (const auto &kwd : *kwds) {
    kwd_names.insert(kwd.first);
  }

  std::vector<std::string> diff;
  std::set_difference(kwd_names.begin(), kwd_names.end(), params.begin(),
                      params.end(), std::inserter(diff, diff.begin()));

  // There are keywords that are NOT parameter names.
  // If the last parameter is a map, and there's no keyword param for it these
  // options will be wrapped into a dictionary; otherwise they are considered
  // invalid named parameters.
  bool remaining_as_kwargs = !diff.empty() && !cand.empty() &&
                             cand.back()->type() == shcore::Value_type::Map &&
                             !kwds->has_key(cand.back()->name);

  return {remaining_as_kwargs, diff};
}
}  // namespace

Cpp_function::Raw_signature Cpp_function::gen_signature(
    const std::vector<std::pair<std::string, Value_type>> &args) {
  Raw_signature sig;
  for (auto &i : args) {
    Param_flag flag = Param_flag::Mandatory;
    std::string name = i.first;
    if (i.first[0] == '?') {
      name = i.first.substr(1);
      flag = Param_flag::Optional;
    }

    sig.push_back(std::make_shared<Parameter>(name, i.second, flag));
  }
  return sig;
}

std::tuple<bool, int, std::string> Cpp_function::match_signatures(
    const Raw_signature &cand, const std::vector<Value_type> &wanted,
    const shcore::Dictionary_t &kwds) {
  bool match = true;
  std::string error;
  bool have_object_params = false;
  size_t exact_matches = 0;

  // If keyword parameters are provided, function matching evaluation is done
  // reviewing parameter by parameter from the function signature
  if (kwds && !kwds->empty()) {
    std::vector<std::string> real_kwargs;
    bool support_kwargs = false;

    std::tie(support_kwargs, real_kwargs) = process_keyword_args(cand, kwds);

    if (!real_kwargs.empty() && !support_kwargs) {
      match = false;
      error = shcore::str_format(
          "Invalid keyword argument%s: %s", real_kwargs.size() == 1 ? "" : "s",
          shcore::str_join(real_kwargs, ", ", [](const std::string &item) {
            return "'" + item + "'";
          }).c_str());
    } else {
      size_t param_count = 0;
      // We make sure the data for each parameter is correctly defined
      for (const auto &param : cand) {
        // For each parameter ensures:
        // A positional parameter is given, or
        // A named parameter is given, or
        // Verify wether the parameter is optional
        // If a value was given ensures it is of a compatible type
        if (wanted.size() > param_count) {
          // A positional parameter was given, we must ensure:
          // - The given type is compatible with the expected one
          // - That NO keyword has been defined for the parameter
          if (param->valid_type(wanted[param_count])) {
            if (kwds->find(param->name) != kwds->end()) {
              match = false;
              error = shcore::str_format(
                  "Got multiple values for argument '%s'", param->name.c_str());
            } else if (param->type() == wanted[param_count]) {
              exact_matches++;
            }
            have_object_params =
                have_object_params || (wanted[param_count] == Object);
          } else {
            match = false;
            error = shcore::str_format("Argument #%zu is expected to be %s",
                                       param_count + 1,
                                       type_description(param->type()).c_str());
          }
        } else if (kwds->has_key(param->name)) {
          // A keyword parameter is given, we must ensure
          // - The given type is compatible with the expected one
          if (param->valid_type(kwds->get_type(param->name))) {
            if (param->type() == kwds->get_type(param->name)) {
              exact_matches++;
            }
            have_object_params =
                have_object_params || (kwds->at(param->name).type == Object);
          } else {
            match = false;
            error = shcore::str_format("Argument '%s' is expected to be %s",
                                       param->name.c_str(),
                                       type_description(param->type()).c_str());
          }
          // This is the last parameter, it's a map and there are dangling named
          // arguments, they will be used to construct the options dictionary
        } else if (param == cand.back() && support_kwargs) {
          exact_matches++;
        } else if (param->flag != Param_flag::Optional) {
          match = false;
          error = shcore::str_format("Missing value for argument '%s'",
                                     param->name.c_str());
        }
        param_count++;
        if (!match) break;
      }
    }
  } else {
    // this function follows mostly the same rules for finding viable
    // functions as c++ for function overload resolution
    size_t m = wanted.size();
    size_t c = cand.size();

    // more than 7 params not supported atm...
    // extend when more expose() variations added
    switch (static_cast<int>(cand.size()) - m) {
      case 7:  // 7 params extra, if the rest is optional, it's a match
        match = match && (cand[c - 7]->flag == Param_flag::Optional);
        // fallthrough
      case 6:  // 6 params extra, if the rest is optional, it's a match
        match = match && (cand[c - 6]->flag == Param_flag::Optional);
        // fallthrough
      case 5:  // 5 params extra, if the rest is optional, it's a match
        match = match && (cand[c - 5]->flag == Param_flag::Optional);
        // fallthrough
      case 4:  // 4 params extra, if the rest is optional, it's a match
        match = match && (cand[c - 4]->flag == Param_flag::Optional);
        // fallthrough
      case 3:  // 3 params extra, if the rest is optional, it's a match
        match = match && (cand[c - 3]->flag == Param_flag::Optional);
        // fallthrough
      case 2:  // 2 params extra, if the rest is optional, it's a match
        match = match && (cand[c - 2]->flag == Param_flag::Optional);
        // fallthrough
      case 1:  // 1 param extra, if the rest is optional, it's a match
        match = match && (cand[c - 1]->flag == Param_flag::Optional);
        // fallthrough
      case 0:  // # of params match
        break;
      default:
        match = false;
        break;
    }

    // In case of parameter count mistmatch, creates the appropiate error
    // message
    if (!match) {
      size_t max = cand.size();
      size_t min = 0;
      for (const auto &param : cand) {
        if (param->flag == Param_flag::Optional) {
          break;
        } else {
          min++;
        }
      }

      if (min == max)
        error = shcore::str_format(
            "Invalid number of arguments, expected %zu but got %zu", min, m);
      else
        error = shcore::str_format(
            "Invalid number of arguments, expected %zu to %zu but got %zu", min,
            max, m);
    } else {
      exact_matches = m;
      // check if all provided params are implicitly convertible to the ones
      // from the candidate
      switch (m) {
        case 7:  // 7 params to be considered
          if (!cand[6]->valid_type(wanted[6])) {
            match = false;
            error =
                shcore::str_format("Argument #7 is expected to be %s",
                                   type_description(cand[6]->type()).c_str());
          }
          exact_matches -= (wanted[6] != cand[6]->type());
          have_object_params = have_object_params || (wanted[6] == Object);
          // fallthrough
        case 6:  // 6 params to be considered
          if (!cand[5]->valid_type(wanted[5])) {
            match = false;
            error =
                shcore::str_format("Argument #6 is expected to be %s",
                                   type_description(cand[5]->type()).c_str());
          }
          exact_matches -= (wanted[5] != cand[5]->type());
          have_object_params = have_object_params || (wanted[5] == Object);
          // fallthrough
        case 5:  // 5 params to be considered
          if (!cand[4]->valid_type(wanted[4])) {
            match = false;
            error =
                shcore::str_format("Argument #5 is expected to be %s",
                                   type_description(cand[4]->type()).c_str());
          }
          exact_matches -= (wanted[4] != cand[4]->type());
          have_object_params = have_object_params || (wanted[4] == Object);
          // fallthrough
        case 4:  // 4 params to be considered
          if (!cand[3]->valid_type(wanted[3])) {
            match = false;
            error =
                shcore::str_format("Argument #4 is expected to be %s",
                                   type_description(cand[3]->type()).c_str());
          }
          exact_matches -= (wanted[3] != cand[3]->type());
          have_object_params = have_object_params || (wanted[3] == Object);
          // fallthrough
        case 3:  // 3 params to be considered
          if (!cand[2]->valid_type(wanted[2])) {
            match = false;
            error =
                shcore::str_format("Argument #3 is expected to be %s",
                                   type_description(cand[2]->type()).c_str());
          }
          exact_matches -= (wanted[2] != cand[2]->type());
          have_object_params = have_object_params || (wanted[2] == Object);
          // fallthrough
        case 2:  // 2 params to be considered
          if (!cand[1]->valid_type(wanted[1])) {
            match = false;
            error =
                shcore::str_format("Argument #2 is expected to be %s",
                                   type_description(cand[1]->type()).c_str());
          }
          exact_matches -= (wanted[1] != cand[1]->type());
          have_object_params = have_object_params || (wanted[1] == Object);
          // fallthrough
        case 1:  // 1 param to be considered
          if (!cand[0]->valid_type(wanted[0])) {
            match = false;
            error =
                shcore::str_format("Argument #1 is expected to be %s",
                                   type_description(cand[0]->type()).c_str());
          }
          exact_matches -= (wanted[0] != cand[0]->type());
          have_object_params = have_object_params || (wanted[0] == Object);
          // fallthrough
        case 0:  // 0 params to be considered = nothing to check
          break;
      }
    }
  }

  if (exact_matches == cand.size() && !have_object_params) {
    return std::make_tuple(match, std::numeric_limits<int>::max(), error);
  } else {
    return std::make_tuple(match, have_object_params ? 0 : exact_matches,
                           error);
  }
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

void Cpp_function::Metadata::set_name(const std::string &name_) {
  auto index = name_.find("|");
  if (index == std::string::npos) {
    name[LowerCamelCase] = get_member_name(name_, LowerCamelCase);
    name[LowerCaseUnderscores] = get_member_name(name_, LowerCaseUnderscores);
  } else {
    name[LowerCamelCase] = name_.substr(0, index);
    name[LowerCaseUnderscores] = name_.substr(index + 1);
  }
}

void Cpp_function::Metadata::set(
    const std::string &name_, Value_type rtype,
    const std::vector<std::pair<std::string, Value_type>> &ptypes) {
  set_name(name_);

  signature = Cpp_function::gen_signature(ptypes);

  param_types = ptypes;
  return_type = rtype;
}

void Cpp_function::Metadata::set(const std::string &name_, Value_type rtype,
                                 const Raw_signature &params) {
  set_name(name_);

  signature = params;

  for (const auto &param : signature) {
    std::string param_name = param->name;

    if (param->flag == Param_flag::Optional) param_name = "?" + param_name;

    param_types.push_back(std::make_pair(param_name, param->type()));
  }

  return_type = rtype;
}

Cpp_object_bridge::Cpp_object_bridge() {
  expose("help", &Cpp_object_bridge::help, "?item");
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

std::vector<std::string> Cpp_object_bridge::get_members() const {
  std::vector<std::string> members;

  for (const auto &prop : _properties)
    members.push_back(prop.name(current_naming_style()));

  for (const auto &func : _funcs) {
    members.push_back(func.second->name(current_naming_style()));
  }
  return members;
}

std::string Cpp_object_bridge::get_base_name(const std::string &member) const {
  std::string ret_val;
  auto func = lookup_function(member);
  if (func) {
    ret_val = func->name(NamingStyle::LowerCamelCase);
  } else {
    auto prop = std::find_if(_properties.begin(), _properties.end(),
                             [member](const Cpp_property_name &p) {
                               return p.name(current_naming_style()) == member;
                             });
    if (prop != _properties.end())
      ret_val = (*prop).name(NamingStyle::LowerCamelCase);
  }

  return ret_val;
}

std::string Cpp_object_bridge::get_function_name(const std::string &member,
                                                 bool fully_specified) const {
  auto m = lookup_function(member);
  std::string name;
  if (!m) {
    name = get_member_name(member, current_naming_style());
  } else {
    name = m->name(current_naming_style());
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

  return get_member_advanced(get_member_name(prop, current_naming_style()));
}

Value Cpp_object_bridge::get_member_advanced(const std::string &prop) const {
  Value ret_val;

  auto func = std::find_if(
      _funcs.begin(), _funcs.end(), [prop](const FunctionEntry &f) {
        return f.second->name(current_naming_style()) == prop;
      });

  if (func != _funcs.end()) {
    ret_val = Value(std::shared_ptr<Function_base>(func->second));
  } else {
    auto prop_index =
        std::find_if(_properties.begin(), _properties.end(),
                     [prop](const Cpp_property_name &p) {
                       return p.name(current_naming_style()) == prop;
                     });
    if (prop_index != _properties.end()) {
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

bool Cpp_object_bridge::has_member_advanced(const std::string &prop) const {
  if (lookup_function(prop)) return true;

  auto prop_index =
      std::find_if(_properties.begin(), _properties.end(),
                   [prop](const Cpp_property_name &p) {
                     return p.name(current_naming_style()) == prop;
                   });
  return (prop_index != _properties.end());
}

bool Cpp_object_bridge::has_member(const std::string &prop) const {
  {
    Scoped_naming_style lower(NamingStyle::LowerCamelCase);
    if (lookup_function(prop)) return true;
  }

  auto prop_index = std::find_if(
      _properties.begin(), _properties.end(),
      [prop](const Cpp_property_name &p) { return p.base_name() == prop; });
  return (prop_index != _properties.end());
}

void Cpp_object_bridge::set_member_advanced(const std::string &prop,
                                            Value value) {
  auto prop_index =
      std::find_if(_properties.begin(), _properties.end(),
                   [prop](const Cpp_property_name &p) {
                     return p.name(current_naming_style()) == prop;
                   });
  if (prop_index != _properties.end()) {
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

bool Cpp_object_bridge::has_method_advanced(const std::string &name) const {
  if (lookup_function(name)) return true;
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
                                       const shcore::Dictionary_t &kwargs) {
  auto func = lookup_function_overload(name, args, kwargs);
  if (func) {
    // At this point func is the best function valid with the given arguments
    // and keyword arguments.
    auto scope = get_function_name(name, true);

    auto signature = func->function_signature();

    // If all the arguments are already in place the function is called
    if (args.size() == signature.size()) {
      return call_function(scope, func, args);
    } else {
      // If missing arguments, they are taken from the keywords or the default
      // values
      Argument_list new_args;
      for (const auto &arg : args) {
        new_args.push_back(arg);
      }

      bool support_kwargs = false;
      std::vector<std::string> final_kwargs;

      if (kwargs && !kwargs->empty()) {
        std::tie(support_kwargs, final_kwargs) =
            process_keyword_args(signature, kwargs);
      }

      std::vector<size_t> skipped_optionals;
      for (size_t index = new_args.size(); index < signature.size(); index++) {
        if (kwargs && kwargs->has_key(signature.at(index)->name)) {
          new_args.push_back(kwargs->at(signature.at(index)->name));
        } else if (index == signature.size() - 1 && support_kwargs) {
          auto kwargs_dict = shcore::make_dict();
          for (const auto &key : final_kwargs) {
            kwargs_dict->set(key, kwargs->at(key));
          }
          new_args.push_back(shcore::Value(kwargs_dict));
        } else if (signature.at(index)->flag == Param_flag::Optional) {
          // An undefined Value is used as a trigger to use the defined
          // default for optional paraeters the parameter when the function is
          // called
          new_args.push_back(signature.at(index)->def_value);
        }
      }
      return call_function(scope, func, new_args);
    }
  } else {
    throw Exception::attrib_error("Invalid object function " + name);
  }
}

/**
 * Utility function to handle function calls using both legacy and new
 * export framework.
 *
 * On new framework, any error will be prepended with the given scope
 * which usually is <class>.<function>
 */
Value Cpp_object_bridge::call_function(
    const std::string &scope, const std::shared_ptr<Cpp_function> &func,
    const Argument_list &args) {
  if (func->is_legacy) {
    return func->invoke(args);
  } else {
    try {
      return func->invoke(args);
    } catch (const shcore::Exception &e) {
      throw shcore::Exception(e.type(), scope + ": " + e.what(), e.code());
    } catch (const shcore::Error &e) {
      throw shcore::Exception(scope + ": " + e.what(), e.code());
    } catch (const std::runtime_error &e) {
      throw shcore::Exception::runtime_error(scope + ": " + e.what());
    } catch (const std::logic_error &e) {
      throw shcore::Exception::logic_error(scope + ": " + e.what());
    } catch (...) {
      throw;
    }
  }
}

std::shared_ptr<Cpp_function> Cpp_object_bridge::lookup_function(
    const std::string &method) const {
  // NOTE this linear lookup is no good, but needed until the naming style
  // mechanism is improved
  std::multimap<std::string, std::shared_ptr<Cpp_function>>::const_iterator i;
  for (i = _funcs.begin(); i != _funcs.end(); ++i) {
    if (i->second->name(current_naming_style()) == method) break;
  }
  if (i == _funcs.end()) {
    return std::shared_ptr<Cpp_function>(nullptr);
  }
  // ignore the overloads and just return first match...
  return i->second;
}

std::shared_ptr<Cpp_function> Cpp_object_bridge::lookup_function_overload(
    const std::string &method, const shcore::Argument_list &args,
    const shcore::Dictionary_t &kwds) const {
  // NOTE this linear lookup is no good, but needed until the naming style
  // mechanism is improved
  std::multimap<std::string, std::shared_ptr<Cpp_function>>::const_iterator i;
  for (i = _funcs.begin(); i != _funcs.end(); ++i) {
    if (i->second->name(current_naming_style()) == method) break;
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
  int max_error_score = -1;
  std::string match_error;
  while (i != _funcs.end() &&
         i->second->name(current_naming_style()) == method) {
    if (i->second->is_legacy) return i->second;

    bool match;
    int score;
    std::string error;
    std::tie(match, score, error) = Cpp_function::match_signatures(
        i->second->function_signature(), arg_types, kwds);
    if (match) {
      candidates.push_back({score, i->second});
    } else {
      if (score > max_error_score) {
        max_error_score = score;
        match_error = error;
      }
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

  if (!match_error.empty()) {
    throw Exception::argument_error(get_function_name(method, true) + ": " +
                                    match_error);
  } else {
    throw Exception::argument_error("Call to " +
                                    get_function_name(method, true) +
                                    " has wrong parameter count/types.");
  }
}

Value Cpp_object_bridge::call(const std::string &name,
                              const Argument_list &args) {
  std::shared_ptr<Cpp_function> func;
  {
    Scoped_naming_style lower(LowerCamelCase);
    func = lookup_function_overload(name, args);
  }
  assert(func);
  auto scope = get_function_name(name, true);
  return call_function(scope, func, args);
}

std::string Cpp_object_bridge::help(const std::string &item) {
  IShell_core::Mode mode;

  if (current_naming_style() == NamingStyle::LowerCamelCase)
    mode = IShell_core::Mode::JavaScript;
  else
    mode = IShell_core::Mode::Python;

  Help_manager help;
  help.set_mode(mode);

  shcore::Topic_mask mask;
  std::string pattern = get_help_id();
  if (!item.empty()) {
    // This group represents the API topics that can be children of another
    // API topic
    pattern += "." + item;
    mask.set(shcore::Topic_type::FUNCTION);
    mask.set(shcore::Topic_type::PROPERTY);
    mask.set(shcore::Topic_type::OBJECT);
    mask.set(shcore::Topic_type::CLASS);
    mask.set(shcore::Topic_type::CONSTANTS);
  } else {
    // This group represents the API topics that can contain children
    mask.set(shcore::Topic_type::MODULE);
    mask.set(shcore::Topic_type::OBJECT);
    mask.set(shcore::Topic_type::GLOBAL_OBJECT);
    mask.set(shcore::Topic_type::CONSTANTS);
    mask.set(shcore::Topic_type::CLASS);
  }

  return help.get_help(pattern, mask);
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
        if (function_sig[i]->flag != Param_flag::Optional) break;
      if (i < function_sig.size()) continue;
    } else if (diff > 0) {
      size_t i = function_sig.size();
      for (; i < overload_sig.size(); i++)
        if (overload_sig[i]->flag != Param_flag::Optional) break;
      if (i < overload_sig.size()) continue;
    }
    size_t i = 0;
    size_t args_num = std::min(overload_sig.size(), function_sig.size());
    for (; i < args_num; i++)
      if (!overload_sig[i]->valid_type(function_sig[i]->type()) &&
          !function_sig[i]->valid_type(overload_sig[i]->type()))
        break;
    if (i == args_num)
      throw Exception::attrib_error(
          "Ambiguous overload detected for funtion: " + name);
  }
}

//-------

Cpp_function::Cpp_function(const Metadata *meta, const Function &func)
    : _func(func), _meta(meta) {}

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
  _meta_tmp.signature = Cpp_function::gen_signature(args);
  _meta = &_meta_tmp;
}

const std::string &Cpp_function::name() const {
  return _meta->name[current_naming_style()];
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
    const auto &sig = signature();
    auto s = _meta->signature.begin();
    const auto s_end = _meta->signature.end();
    int n = 0;

    if (args.size() > sig.size() ||
        (args.size() < sig.size() &&
         _meta->signature[args.size()]->flag != Param_flag::Optional)) {
      throw Exception::argument_error(
          name() + ": Invalid number of arguments, expected " +
          num_args_expected(sig) + " but got " + std::to_string(args.size()));
    }

    while (s != s_end) {
      if (a == args.end()) {
        if ((*s)->flag == Param_flag::Optional) {
          // param is optional... remaining expected params can only be
          // optional too
          break;
        }
      }

      // NOTE: Undefined Optional parameters will skip this validation as they
      // will be taking the default value
      if ((a->type != Value_type::Undefined ||
           (*s)->flag != Param_flag::Optional) &&
          !(*s)->valid_type(a->type)) {
        throw Exception::argument_error(
            name() + ": Argument #" + std::to_string(n + 1) +
            " is expected to be " + type_description((*s)->type()));
      }

      ++n;
      ++a;
      ++s;
    }
  }

  // Note: exceptions caught here should all be self-descriptive and be
  // enough for the caller to figure out what's wrong. Other specific
  // exception types should have been caught earlier, in the bridges
  try {
    return _func(args);
  } catch (const shcore::Exception &e) {
    // shcore::Exception can be thrown by bridges
    throw;
  } catch (const std::invalid_argument &e) {
    throw Exception::argument_error(e.what());
  } catch (const std::logic_error &e) {
    throw Exception::logic_error(e.what());
  } catch (const std::runtime_error &e) {
    throw Exception::runtime_error(e.what());
  } catch (const std::exception &e) {
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

std::string Parameter_context::str() const {
  std::vector<std::string> ctx_data;

  for (auto it = levels.begin(); it != levels.end(); it++) {
    if (it->position.is_null()) {
      ctx_data.push_back(it->name);
    } else {
      ctx_data.push_back(
          shcore::str_format("%s #%i", it->name.c_str(), *it->position));
    }
  }

  return (title.empty() ? "" : title + " ") + shcore::str_join(ctx_data, ", ");
}

bool Parameter_validator::valid_type(const Parameter &param,
                                     Value_type type) const {
  return is_compatible_type(type, param.type());
}

void Parameter_validator::validate(const Parameter &param, const Value &data,
                                   Parameter_context *context) const {
  try {
    // NOTE: Skipping validation for Undefined parameters that are optional as
    // they will use the default value
    if (param.flag == Param_flag::Optional &&
        data.type == Value_type::Undefined)
      return;

    if (!valid_type(param, data.type)) {
      throw std::invalid_argument("Invalid type");
    }

    // The call to check_type only verifies the kConvertible matrix there:
    // - A string is convertible to integer, bool and float
    // - A Null us convertible to object, array and dictionary
    // We do this validation to make sure the conversion is really valid
    if (data.type == shcore::String) {
      if (param.type() == shcore::Integer)
        data.as_int();
      else if (param.type() == shcore::Bool)
        data.as_bool();
      else if (param.type() == shcore::Float)
        data.as_double();
    }
  } catch (...) {
    auto error =
        shcore::str_format("%s is expected to be %s", context->str().c_str(),
                           shcore::type_description(param.type()).c_str());
    throw shcore::Exception::argument_error(error);
  }
}

void Object_validator::validate(const Parameter &param, const Value &data,
                                Parameter_context *context) const {
  // NOTE: Skipping validation for Undefined parameters that are optional as
  // they will use the default value
  if (param.flag == Param_flag::Optional && data.type == Value_type::Undefined)
    return;
  Parameter_validator::validate(param, data, context);

  if (!m_allowed.empty()) {
    const auto object = data.as_object();

    if (!object) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "%s is expected to be an object", context->str().c_str()));
    }

    if (std::find(std::begin(m_allowed), std::end(m_allowed),
                  object->class_name()) == std::end(m_allowed)) {
      auto allowed_str = shcore::str_join(m_allowed, ", ");
      std::string error;
      if (m_allowed.size() == 1) {
        error = shcore::str_format("%s is expected to be a '%s' object.",
                                   context->str().c_str(), allowed_str.c_str());
      } else {
        error = shcore::str_format("%s is expected to be one of '%s'.",
                                   context->str().c_str(), allowed_str.c_str());
      }
      throw shcore::Exception::argument_error(error);
    }
  }
}

void String_validator::validate(const Parameter &param, const Value &data,
                                Parameter_context *context) const {
  // NOTE: Skipping validation for Undefined parameters that are optional as
  // they will use the default value
  if (param.flag == Param_flag::Optional && data.type == Value_type::Undefined)
    return;
  Parameter_validator::validate(param, data, context);

  if (!m_allowed.empty()) {
    if (std::find(std::begin(m_allowed), std::end(m_allowed),
                  data.as_string()) == std::end(m_allowed)) {
      auto error = shcore::str_format(
          "%s only accepts the following values: %s.", context->str().c_str(),
          shcore::str_join(m_allowed, ", ").c_str());
      throw shcore::Exception::argument_error(error);
    }
  }
}

void Option_validator::validate(const Parameter &param, const Value &data,
                                Parameter_context *context) const {
  // NOTE: Skipping validation for Undefined parameters that are optional as
  // they will use the default value
  if (param.flag == Param_flag::Optional && data.type == Value_type::Undefined)
    return;
  Parameter_validator::validate(param, data, context);

  if (!m_allowed.empty()) {
    Option_unpacker unpacker(data.as_map());

    for (const auto &item : m_allowed) {
      shcore::Value value;
      if (item->flag == Param_flag::Mandatory) {
        unpacker.required(item->name.c_str(), &value);
      } else {
        unpacker.optional(item->name.c_str(), &value);
      }

      if (value) {
        context->levels.push_back({"option '" + item->name + "'", {}});
        item->validate(value, context);
        context->levels.pop_back();
      }
    }

    unpacker.end("at " + context->str());
  }
}

void Parameter::validate(const Value &data, Parameter_context *context) const {
  if (m_validator) {
    m_validator->validate(*this, data, context);
  } else {
    // If no validator was set, uses the default validator.
    Parameter_validator{}.validate(*this, data, context);
  }
}

bool Parameter::valid_type(Value_type type) const {
  if (m_validator) {
    return m_validator->valid_type(*this, type);
  } else {
    // If no validator was set, uses the default validator.
    return Parameter_validator{}.valid_type(*this, type);
  }
}

}  // namespace shcore
