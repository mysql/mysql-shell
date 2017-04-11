/*
* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/utils_help.h"
#include "utils/utils_general.h"
#include <vector>
#include <cctype>

namespace shcore {
Shell_help *Shell_help::_instance = nullptr;

Shell_help *Shell_help::get() {
  if (!_instance)
    _instance = new Shell_help();

  return _instance;
}

void Shell_help::add_help(const std::string& token, const std::string& data) {
  _help_data[token] = data;
}

std::string Shell_help::get_token(const std::string& token) {
  std::string ret_val;

  if (_help_data.find(token) != _help_data.end())
      ret_val = _help_data[token];

  return ret_val;
}

Help_register::Help_register(const std::string &token, const std::string &data) {
  shcore::Shell_help::get()->add_help(token, data);
};

std::vector<std::string> get_help_text(const std::string& token) {
  std::string real_token;
  for (auto c : token)
    real_token.append(1, std::toupper(c));

  int index = 0;
  std::string text = Shell_help::get()->get_token(real_token);

  std::vector<std::string> lines;
  while (!text.empty()) {
    lines.push_back(text);

    text = Shell_help::get()->get_token(real_token + std::to_string(++index));
  }

  return lines;
}

std::string get_function_help(shcore::NamingStyle style, const std::string& class_name, const std::string &bfname) {
  std::string ret_val;

  std::string fname = shcore::get_member_name(bfname, style);

  auto params = get_help_text(class_name + "_" + bfname + "_PARAM");

  if (!params.empty()) {
    std::vector<std::string> fpnames; // Parameter names as they will look in the signature
    std::vector<std::string> pnames;  // Parameter names for the WHERE section
    std::vector<std::string> pdescs;  // Parameter descriptions as they are defined

    for (auto paramdef : params) {
      // 7 is the length of: "\param " or "@param "
      size_t start_index = 7;
      auto pname = paramdef.substr(start_index, paramdef.find(" ", start_index) - start_index);
      pnames.push_back(pname);

      start_index += pname.size() + 1;
      auto desc = paramdef.substr(start_index);
      auto first_word = desc.substr(0, desc.find(" "));

      // Updates paramete names to reflect the optional attribute on the signature
      // Removed the optionsl word from the description
      if (first_word == "Optional") {
        if (fpnames.empty())
          fpnames.push_back("[" + pname + "]"); // First param, creates: [pname]
        else {
          fpnames[fpnames.size() - 1].append("[");
          fpnames.push_back(pname + "]"); // Non first param creates: pname[, pname]
        }
        desc = desc.substr(first_word.size() + 1); // Deletes the optional word
        desc[0] = std::toupper(desc[0]);
      } else
        fpnames.push_back(pname);

      pdescs.push_back(desc);
    }

    // Creates the syntax
    ret_val.append("SYNTAX\n\n  ");
    ret_val.append("<" + class_name + ">." + fname);
    ret_val.append("(" + shcore::join_strings(fpnames, ", ") + ")\n\n");

    // Describes the parameters
    ret_val.append("WHERE\n\n");

    size_t index;
    for (index = 0; index < params.size(); index++) {
      ret_val.append("  " + pnames[index] + ": ");

      size_t name_length = pnames[index].size() + 4;

      ret_val.append(shcore::format_text({pdescs[index]}, 80, name_length, true) + "\n");
    }

    ret_val.append("\n");
  } else {
    ret_val.append("SYNTAX\n\n  ");
    ret_val.append("<" + class_name + ">." + fname);
    ret_val.append("()\n\n");
  }

  // Describes the exceptions
  auto throws = get_help_text(class_name + "_" + bfname + "_THROWS");

  if (!throws.empty()) {
    std::vector<std::string> enames;  // Exception names for the THROWS section
    std::vector<std::string> edescs;  // Exception descriptions as they are defined

    for (auto exceptiondef : throws) {
      // 8 is the length of: "\throws " or "@throws "
      size_t start_index = 8;
      auto ename = exceptiondef.substr(start_index, exceptiondef.find(" ", start_index) - start_index);
      enames.push_back(ename);

      start_index += ename.size() + 1;
      auto desc = exceptiondef.substr(start_index);
      auto first_word = desc.substr(0, desc.find(" "));

      edescs.push_back(desc);
    }

    ret_val.append("EXCEPTIONS\n\n");

    size_t index;
    for (index = 0; index < throws.size(); index++) {
      ret_val.append("  " + enames[index] + ": ");

      size_t exception_length = enames[index].size() + 4;

      ret_val.append(shcore::format_text({edescs[index]}, 80, exception_length, true) + "\n");
    }

    ret_val.append("\n");
  }

  auto returns = get_help_text(class_name + "_" + bfname + "_RETURNS");
  if (!returns.empty()) {
    ret_val.append("RETURNS\n\n");
    // Removes the @returns tag
    returns[0] = returns[0].substr(8);
    ret_val.append(shcore::format_markup_text(returns, 80, 0) + "\n\n");
  }

  auto details = get_help_text(class_name + "_" + bfname + "_DETAIL");

  if (!details.empty()) {
    ret_val.append("DESCRIPTION\n\n");
    ret_val.append(shcore::format_markup_text(details, 80, 0));
  }

  ret_val += "\n";

  return ret_val;
};

std::string get_property_help(shcore::NamingStyle style, const std::string& class_name, const std::string &bpname) {
  std::string ret_val;

  std::string fname = shcore::get_member_name(bpname, style);

  auto details = get_help_text(class_name + "_" + bpname + "_DETAIL");

  if (!details.empty()) {
    ret_val.append("DESCRIPTION\n\n");
    ret_val.append(shcore::format_markup_text(details, 80, 0));
    ret_val += "\n";
  }

  return ret_val;
};

std::string get_chained_function_help(shcore::NamingStyle style, const std::string& class_name, const std::string &bfname) {
  std::string ret_val;

  auto chain_definition = shcore::get_help_text(class_name + "_" + bfname + "_CHAINED");

  ret_val.append("SYNTAX\n\n  ");
  auto cname = "<" + class_name + ">";
  ret_val.append(cname);

  // Gets the chain definition elements
  auto chained_functions = shcore::split_string(chain_definition[0], ".");

  // First element is the chain definition class
  std::string tgtcname = chained_functions[0];
  chained_functions.erase(chained_functions.begin());

  std::vector<std::string> full_syntax;
  std::vector<std::string> function_list;

  for (auto chained_function : chained_functions) {
    bool optional = false;
    bool child_optional = false;
    std::string formatted_child;
    std::string formatted_function;
    if (chained_function.find("[") == 0) {
      optional = true;
      chained_function = chained_function.substr(1, chained_function.length() - 2);
    }

    auto child_functions = shcore::split_string(chained_function, "->");

    // The first function is the parent
    chained_function = child_functions[0];

    function_list.push_back(chained_function);

    std::string child_function;
    // There's a function that is valid only on the context of a parent function
    // This section will format the child function
    if (child_functions.size() > 1) {
      child_function = child_functions[1];

      if (child_function.find("[") == 0) {
        child_optional = true;
        child_function = child_function.substr(1, child_function.length() - 2);
      }

      function_list.push_back(child_function);

      auto syntaxes = get_help_text(tgtcname + "_" + child_function + "_SYNTAX");

      auto style_child_fname = shcore::get_member_name(child_function, style);

      if (child_optional)
        formatted_child += "[";

      if (syntaxes.size() == 1) {
        if (child_function != style_child_fname)
          formatted_child += "." + shcore::replace_text(syntaxes[0], child_function, style_child_fname);
        else
          formatted_child += "." + syntaxes[0];
      } else
        formatted_child += "." + style_child_fname + "(...)";

      if (child_optional)
        formatted_child += "]";
    }

    // This section will format the parent function
    auto item_syntax = get_help_text(tgtcname + "_" + chained_function + "_SYNTAX");
    auto style_fname = shcore::get_member_name(chained_function, style);
    if (optional)
      formatted_function = "[";

    if (item_syntax.size() == 1) {
      if (chained_function != style_fname)
        formatted_function += "." + shcore::replace_text(item_syntax[0], chained_function, style_fname);
      else
        formatted_function += "." + item_syntax[0];
    } else
      formatted_function += "." + style_fname + "(...)";

    if (!formatted_child.empty())
      formatted_function += formatted_child;

    if (optional)
      formatted_function += "]";

    full_syntax.push_back(formatted_function);
  }

  ret_val += shcore::format_text(full_syntax, 80, cname.length() + 2, false) + "\n\n";

  ret_val.append("DESCRIPTION\n\n");

  auto details = shcore::get_help_text(class_name + "_" + bfname + "_DETAIL");
  if (details.empty())
    details = shcore::get_help_text(class_name + "_" + bfname + "_BRIEF");

  if (!details.empty())
    ret_val.append(shcore::format_markup_text(details, 80, 0));

  for (auto fname : function_list) {
    ret_val.append("\n\n");

    auto style_fname = shcore::get_member_name(fname, style);

    // On functions we continue with the rest of the documentation
    auto item_syntax = get_help_text(tgtcname + "_" + fname + "_SYNTAX");

    if (fname != style_fname) {
      for (size_t index = 0; index < item_syntax.size(); index++)
        item_syntax[index] = shcore::replace_text(item_syntax[index], fname, style_fname);
    }

    if (item_syntax.size() == 1)
      ret_val.append("  ." + item_syntax[0]);
    else
      ret_val.append("  ." + style_fname + "(...)");

    ret_val.append("\n\n");

    if (item_syntax.size() > 1)
      ret_val.append("    Variations\n\n      " + shcore::format_text(item_syntax, 80, 6, false) + "\n\n");

    auto idetails = get_help_text(tgtcname + "_" + fname + "_DETAIL");
    if (idetails.empty())
      idetails = get_help_text(tgtcname + "_" + fname + "_BRIEF");

    if (!idetails.empty())
      ret_val.append(shcore::format_markup_text(idetails, 80, 4));
  }

  ret_val.append("\n");

  return ret_val;
};
}
