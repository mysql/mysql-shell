/*
 * Copyright (c) 2016, 2021, Oracle and/or its affiliates.
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

#include "shellcore/utils_help.h"
#include <cctype>
#include <regex>
#include <vector>
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

#define HELP_OPTIONAL_STR "Optional"
#define HELP_IGNORE_STR "IGNORE"

#define MEMBER_TOPICS true
#define NO_MEMBER_TOPICS false

// Topic Splitter Constants
#define HELP_API_SPLITTER "."
#define HELP_CMD_SPLITTER " "
#define HELP_TOPIC_SPLITTER "/"

// Formatting constants
#define NO_PADDING 0
#define SECTION_PADDING 6
#define ITEM_DESC_PADDING 6
#define HEADER_CONTENT_SEPARATOR "\n"

#define HELP_TITLE_DESCRIPTION "DESCRIPTION"
#define HELP_TITLE_SYNTAX "SYNTAX"

// Tag to indicate a specific text line must NOT be formatted
#define HELP_NO_FORMAT "#NOFORMAT:"

namespace textui = mysqlshdk::textui;

namespace shcore {
namespace {
/**
 * This regular expression is to parse the help data for the options to be used
 * in the CLI help. Formatting of option help comes in the following format:
 *
 * @li [<b>]<option>[/b>]: [<type>|<allowed_values>[(default[:] <default>)] - ]
 *<description>
 *
 * This regular expression identifies the different tokens in order to:
 * - Remove documented default from CLI help (it is taken from API metadata)
 * - Produce a CLI formatted description as:
 *   [<allowed_values>.] <description>.[ Default: <default>.]
 **/
const std::regex k_cli_option_help_regexp(
    "^@li\\s(<b>)?([a-z|A-Z|_][a-z|A-Z|_|0-9]+)(<\\/"
    "b>)?(:\\s|\\s-\\s)((([a-z|A-Z|\\s]+)(\\s\\([default|required]*(.*)\\))?"
    "\\s-\\s)?(.*))$");

std::map<std::string, std::string> parse_cli_option_data(
    const std::vector<std::string> &data) {
  std::map<std::string, std::string> options;

  for (const auto &entry : data) {
    std::smatch results;
    if (std::regex_match(entry, results, k_cli_option_help_regexp)) {
      // Result 6 indicates type description was found
      if (results[6].matched) {
        std::string full_details;
        auto type_tokens = shcore::str_split(results[7], " ");
        // Type description found, should be ignored.
        if (type_tokens.size() == 1 ||
            shcore::str_caseeq(type_tokens[0], "array") ||
            shcore::str_caseeq(type_tokens[0], "list")) {
          // NO-OP (Let here for clarity)
        } else {
          // Allowed values found, they are the first part of the description.
          full_details = results[7];
          full_details.append(". ");
        }

        if (results[8].matched && results[8] == "(required)") {
          full_details.append("Required. ");
        }

        // Appends the description.
        std::string details = results[10];
        if (!details.empty()) details[0] = std::toupper(details[0]);

        // Appends the default/required data.
        full_details.append(details);
        if (results[9].matched) {
          full_details.append(" Default: ");
          std::string def_val = results[9];
          if (def_val[0] == ':') def_val = def_val.substr(1);
          def_val = shcore::str_strip(def_val);
          full_details.append(def_val + ".");
        }
        options[results[2]] = full_details;
      } else {
        // Appends the description.
        std::string details = results[5];
        if (!details.empty()) details[0] = std::toupper(details[0]);
        options[results[2]] = details;
      }
    }
  }
  return options;
}

std::string get_cli_param_type(shcore::Value_type type) {
  switch (type) {
    case shcore::Value_type::Bool:
      return "bool";
    case shcore::Value_type::Float:
      return "float";
    case shcore::Value_type::Integer:
      return "int";
    case shcore::Value_type::UInteger:
      return "uint";
    case shcore::Value_type::String:
      return "str";
    case shcore::Value_type::Array:
      return "list";
    default:
      return "";
  }
}

std::string format_cli_option(const std::shared_ptr<Parameter> &option) {
  std::string option_detail;
  if (!option->short_name.empty())
    option_detail = "-" + option->short_name + ", ";
  option_detail.append("--" + option->name);

  if (option->type() == shcore::Array) {
    auto validator = option->validator<List_validator>();
    if (validator->element_type() == shcore::Undefined) {
      option_detail += "[:<type>]=<value>";
    } else {
      option_detail += "=<" + get_cli_param_type(validator->element_type()) +
                       " " + get_cli_param_type(option->type()) + ">";
    }
  } else if (option->type() == shcore::Map) {
    option_detail += "=<key>[:<type>]=<value>";
  } else if (option->type() == shcore::Undefined) {
    option_detail += "[:<type>]=<value>";
  } else {
    option_detail += "=<" + get_cli_param_type(option->type()) + ">";
  }

  return option_detail;
}

std::string strip_param_type(const std::string &input) {
  std::string ret_val{input};

  auto pos = input.find(" - ");
  if (pos != std::string::npos) {
    auto type = input.substr(0, pos);

    std::string allowed_types =
        "DictionaryBoolIntegerUIntegerFloatStringObjectArrayMapFunctionUndefine"
        "dNull";

    if (allowed_types.find(type) != std::string::npos) {
      ret_val = ret_val.substr(pos + 3);
      ret_val[0] = std::toupper(ret_val[0]);
    }
  }
  return ret_val;
}

}  // namespace
using Mode_mask = shcore::IShell_core::Mode_mask;

bool Help_topic_compare::operator()(Help_topic *const &lhs,
                                    Help_topic *const &rhs) const {
  if (!lhs) {
    return true;
  } else if (!rhs) {
    return false;
  } else {
    int ret_val = str_casecmp(lhs->m_name.c_str(), rhs->m_name.c_str());

    // If case insensitive are equal, does case sensitive comparison
    return ret_val == 0 ? (lhs->m_name < rhs->m_name) : ret_val < 0;
  }
}

bool Help_topic_id_compare::operator()(Help_topic *const &lhs,
                                       Help_topic *const &rhs) const {
  if (!lhs) {
    return true;
  } else if (!rhs) {
    return false;
  } else {
    int ret_val = str_casecmp(lhs->m_id.c_str(), rhs->m_id.c_str());

    // If case insensitive are equal, does case sensitive comparison
    return ret_val == 0 ? (lhs->m_id < rhs->m_id) : ret_val < 0;
  }
}

bool icomp::operator()(const std::string &lhs, const std::string &rhs) const {
  return str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
}

bool Help_topic::is_api() const {
  return m_type == Topic_type::MODULE || m_type == Topic_type::CLASS ||
         m_type == Topic_type::CONSTANTS || m_type == Topic_type::OBJECT ||
         m_type == Topic_type::GLOBAL_OBJECT ||
         m_type == Topic_type::FUNCTION || m_type == Topic_type::PROPERTY;
}

bool Help_topic::is_api_object() const {
  return m_type == Topic_type::MODULE || m_type == Topic_type::CLASS ||
         m_type == Topic_type::CONSTANTS || m_type == Topic_type::OBJECT ||
         m_type == Topic_type::GLOBAL_OBJECT;
}

bool Help_topic::is_member() const {
  return m_type == Topic_type::FUNCTION || m_type == Topic_type::PROPERTY ||
         m_type == Topic_type::OBJECT;
}

bool Help_topic::is_enabled(IShell_core::Mode mode) const {
  return Help_registry::get()->is_enabled(this, mode);
}

std::string Help_topic::get_name(IShell_core::Mode mode) const {
  // Member topics may have different names based on the scripting
  // mode
  if (is_member()) {
    auto names = shcore::str_split(m_name, "|");
    if (mode == IShell_core::Mode::Python) {
      if (names.size() == 2)
        return names[1];
      else
        return shcore::from_camel_case(m_name);
    } else {
      return names[0];
    }
  }

  return m_name;
}

std::string Help_topic::get_base_name() const {
  // Member topics may have different names bsaed on the scripting
  // mode
  if (is_member()) {
    auto names = shcore::str_split(m_name, "|");
    return names[0];
  }

  return m_name;
}

Help_topic *Help_topic::get_category() {
  Help_topic *category = this;
  while (category->m_parent != nullptr &&
         category->m_parent->m_name != Help_registry::HELP_ROOT) {
    category = category->m_parent;
  }

  return category;
}

void Help_topic::get_id_tokens(IShell_core::Mode mode,
                               std::vector<std::string> *tokens,
                               Topic_id_mode id_mode) const {
  assert(tokens);

  if (m_parent != nullptr) {
    // With id_mode = FULL, every single parent is considered
    if (id_mode == Topic_id_mode::FULL) {
      m_parent->get_id_tokens(mode, tokens, id_mode);
    } else {
      // When not in FULL mode, ids for API topics only consider the API
      // Hierarchy, meaning both ROOT and Category parents are ignored
      if (is_api()) {
        if (m_parent->is_api()) {
          m_parent->get_id_tokens(mode, tokens, id_mode);
        }
      } else {
        // When not in FULL mode, non API topics automatically exclude ROOT
        // But categories will be included if mode_id != EXCLUDE_CATEGORIES
        if (m_parent->m_name != Help_registry::HELP_ROOT &&
            id_mode != Topic_id_mode::EXCLUDE_CATEGORIES) {
          m_parent->get_id_tokens(mode, tokens, id_mode);
        }
      }
    }
  }

  tokens->push_back(get_name(mode));
}  // namespace shcore

std::string Help_topic::get_id(IShell_core::Mode mode,
                               Topic_id_mode id_mode) const {
  std::string splitter = is_api() ? HELP_API_SPLITTER : HELP_TOPIC_SPLITTER;
  std::string last_splitter = is_command() ? HELP_CMD_SPLITTER : splitter;

  std::vector<std::string> id_tokens;
  get_id_tokens(mode, &id_tokens, id_mode);

  std::string this_name = id_tokens.back();
  id_tokens.pop_back();

  std::string ret_val = shcore::str_join(id_tokens, splitter);

  if (!ret_val.empty()) ret_val += last_splitter;

  ret_val += this_name;

  return ret_val;
}  // namespace shcore

const char Help_registry::HELP_ROOT[] = "Contents";
const char Help_registry::HELP_COMMANDS[] = "Shell Commands";
const char Help_registry::HELP_SQL[] = "SQL Syntax";

Topic_mask Help_registry::get_parent_topic_types() {
  static const Topic_mask kParentTopicTypes{Topic_mask(Topic_type::OBJECT)
                                                .set(Topic_type::GLOBAL_OBJECT)
                                                .set(Topic_type::CONSTANTS)
                                                .set(Topic_type::CATEGORY)
                                                .set(Topic_type::TOPIC)
                                                .set(Topic_type::SQL)
                                                .set(Topic_type::COMMAND)
                                                .set(Topic_type::MODULE)
                                                .set(Topic_type::CLASS)};

  return kParentTopicTypes;
}

Help_registry::Help_registry(bool threaded)
    : m_help_data(Help_registry::icomp),
      m_keywords(Help_registry::icomp),
      m_threaded(threaded),
      m_global_removed(false) {
  m_c_tor = true;
  shcore::Scoped_callback leave([this]() { m_c_tor = false; });

  if (!threaded) {
    // The Contents category is registered right away since it is the root
    // Of the help system
    add_help_topic(HELP_ROOT, Topic_type::CATEGORY, "CONTENTS", "",
                   Mode_mask::all());

    // The SQL category is also registered right away, it doesn't have content
    // at the client side but will be used as parent for SQL topics
    add_help_topic(HELP_SQL, Topic_type::SQL, "SQL_CONTENTS", HELP_ROOT,
                   Mode_mask::all());
  } else {
    auto lock = get()->ensure_lock_threaded();
    get()->m_threaded_help.push_back(this);
  }
}

Help_registry::~Help_registry() {
  // The cleanup needs to be done only and only from the thread scope so we get
  // rid of dangling pointers.
  if (m_threaded) {
    if (!m_global_removed) {
      for (auto &it : m_topics) {
        remove_topic(&it, Keyword_location::GLOBAL_CTX);
      }

      auto lock = get()->ensure_lock_threaded();
      get()->m_threaded_help.erase(
          std::remove_if(get()->m_threaded_help.begin(),
                         get()->m_threaded_help.end(),
                         [this](auto &it) { return it == this; }),
          get()->m_threaded_help.end());
    }
  } else {  // we need to go through the m_threaded_help and do a cleanup cause
            // it seems that sometimes global_ctx is removed before a local_ctx
    auto lock = ensure_lock_threaded();
    for (const auto &it : m_threaded_help) {
      it->m_global_removed = true;
      for (auto &sit : it->m_topics) {
        remove_topic(&sit);
      }
    }
  }
}

bool Help_registry::icomp(const std::string &lhs, const std::string &rhs) {
  return str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
}

Help_registry *Help_registry::get() {
  static Help_registry instance;
  return &instance;
}

void Help_registry::add_split_help(const std::string &prefix,
                                   const std::string &data, bool auto_brief,
                                   bool nosuffix) {
  std::map<std::string, int> current_index;

  auto token = [prefix, &current_index](const std::string &suffix) {
    int index = current_index[suffix];
    current_index[suffix] = index + 1;

    if (index == 0)
      return shcore::str_format("%s%s%s", prefix.c_str(),
                                !suffix.empty() ? "_" : "", suffix.c_str());
    else
      return shcore::str_format("%s%s%s%i", prefix.c_str(),
                                !suffix.empty() ? "_" : "", suffix.c_str(),
                                index);
  };

  // Split the help text into multiple entries of the same prefix topic and
  // then register them one by one.

  std::vector<std::string> lines = shcore::str_split(data, "\n");

  // make sure the last line is an empty line
  if ("" != lines.back()) {
    lines.emplace_back("");
  }

  auto line_iter = lines.cbegin();
  const auto lines_end = lines.cend();

  auto get_line = [&lines_end, &line_iter](bool *eos, bool strip) {
    if (line_iter == lines_end) {
      *eos = true;
      return std::string();
    } else {
      if (strip)
        return shcore::str_rstrip(*line_iter++);
      else
        return *line_iter++;
    }
  };

  auto unget_line = [&lines, &line_iter]() {
    if (line_iter > lines.cbegin()) --line_iter;
  };

  auto get_para = [&get_line, &unget_line](bool *eos) {
    std::string line;
    std::string para;
    bool rstrip = true;

    line = get_line(eos, rstrip);
    while (!*eos && line.empty()) line = get_line(eos, rstrip);

    para = line;
    // now keep adding lines to the paragraph until one of:
    // empty line, @ at bol, $ at bol
    if (shcore::str_beginswith(para, "@code")) {
      while (!*eos && line != "@endcode") {
        line = get_line(eos, rstrip);

        if (line != "@endcode") para.append("\n");

        para.append(line);
      }
    } else {
      line = get_line(eos, rstrip);
      while (!*eos && !line.empty()) {
        if (line[0] == '@' || line[0] == '$' ||
            (line.size() > 1 && line[0] == '-' && line[1] == '#'))
          break;
        para.append(" ").append(line);
        line = get_line(eos, rstrip);
      }
      unget_line();
    }

    return para;
  };

  bool eos = false;
  std::string para;
  if (auto_brief) add_help(token("BRIEF"), get_para(&eos));

  // params, return and deprecation warning
  para = get_para(&eos);
  while (!eos && !para.empty()) {
    if (shcore::str_beginswith(para, "@param")) {
      add_help(token("PARAM"), para);
    } else if (shcore::str_beginswith(para, "@return")) {
      add_help(token("RETURNS"), para);
    } else if (shcore::str_beginswith(para, "@attention") &&
               para.find("will be removed") != std::string::npos) {
      add_help(token("DEPRECATED"), para);
      break;
    } else {
      break;
    }
    para = get_para(&eos);
  }

  // main body
  while (!eos && !para.empty()) {
    if (shcore::str_beginswith(para, "@throw")) {
      break;
    }
    if (nosuffix)
      add_help(token(""), para);
    else
      add_help(token("DETAIL"), para);
    para = get_para(&eos);
  }

  // everything after the 1st @throw assumed to be more throws
  while (!eos && !para.empty()) {
    if (shcore::str_beginswith(para, "@throw"))
      add_help(token("THROWS"), para.substr(para.find_first_of(" \t") + 1));
    else
      add_help(token("THROWS"), para);
    para = get_para(&eos);
  }
}

void Help_registry::add_help(const std::string &token, const std::string &data,
                             Keyword_location loc) {
  auto lock = ensure_lock();
  if (loc == Keyword_location::LOCAL_CTX) {
    get_thread_context_help()->m_help_data[token] = data;
  } else if (loc == Keyword_location::GLOBAL_CTX) {
    m_help_data[token] = data;
  } else {
    get_thread_context_help()->m_help_data[token] = data;
    m_help_data[token] = data;
  }
}

void Help_registry::add_help(const std::string &prefix, const std::string &tag,
                             const std::string &data, Keyword_location loc) {
  auto full_prefix = prefix + "_" + tag;

  add_help(full_prefix, data, loc);
}

void Help_registry::add_help(const std::string &prefix, const std::string &tag,
                             const std::vector<std::string> &data,
                             Keyword_location loc) {
  size_t sequence = 0;
  add_help(prefix, tag, &sequence, data, loc);
}

void Help_registry::add_help(const std::string &prefix, const std::string &tag,
                             size_t *sequence,
                             const std::vector<std::string> &data,
                             Keyword_location loc) {
  assert(sequence);

  std::string index = (*sequence) ? std::to_string(*sequence) : "";

  for (auto &entry : data) {
    auto full_prefix = prefix + "_" + tag + index;
    add_help(full_prefix, entry, loc);
    (*sequence)++;
    index = std::to_string(*sequence);
  }
}

void Help_registry::add_help(const std::string &prefix,
                             const std::vector<Example> &data,
                             Keyword_location loc) {
  size_t sequence = 0;
  add_help(prefix, &sequence, data, loc);
}

void Help_registry::add_help(const std::string &prefix, size_t *sequence,
                             const std::vector<Example> &data,
                             Keyword_location loc) {
  assert(sequence);

  auto index = (*sequence) ? std::to_string(*sequence) : "";

  for (const auto &entry : data) {
    const auto full_prefix = prefix + "_EXAMPLE" + index;
    add_help(full_prefix, entry.code, loc);
    add_help(full_prefix + "_DESC", entry.description, loc);

    (*sequence)++;
    index = std::to_string(*sequence);
  }
}

std::vector<Help_topic *> Help_registry::add_help_topic(
    const std::string &name, Topic_type type, const std::string &tag,
    const std::string &parent_id, Mode_mask mode, Keyword_location loc) {
  auto parent = get_topic(parent_id, true, get_parent_topic_types());

  if (loc == Keyword_location::LOCAL_CTX) {
    return {get_thread_context_help()->add_help_topic(name, type, tag,
                                                      parent_id, parent, mode)};
  } else if (loc == Keyword_location::GLOBAL_CTX) {
    return {add_help_topic(name, type, tag, parent_id, parent, mode)};
  } else {
    std::vector<Help_topic *> ret;
    ret.push_back(get_thread_context_help()->add_help_topic(
        name, type, tag, parent_id, parent, mode));
    ret.push_back(add_help_topic(name, type, tag, parent_id, parent, mode));
    return ret;
  }
}

Help_topic *Help_registry::add_help_topic(const std::string &name,
                                          Topic_type type,
                                          const std::string &tag,
                                          const std::string &parent_id,
                                          Help_topic *parent, Mode_mask mode) {
  auto lock = ensure_lock();
  Help_topic *new_topic = &(*m_topics.insert(
      m_topics.end(), {name, name, type, tag, nullptr, {}, true}));
  m_topics_by_type[type].push_back(new_topic);

  std::string splitter;

  if (!parent_id.empty()) {
    if (parent) {
      new_topic->m_parent = parent;

      register_topic(new_topic, true, mode);
    } else {
      // Adds the topic to the orphans so it is properly registered when the
      // parent chain is made available
      if (m_orphans.find(parent_id) == m_orphans.end())
        m_orphans[parent_id] = {};

      m_orphans[parent_id].push_back(new_topic);
    }
  } else {
    register_topic(new_topic, true, mode);
  }

  return new_topic;
}

void Help_registry::remove_topic(Help_topic *topic, Keyword_location loc) {
  if (loc == Keyword_location::LOCAL_CTX) {
    get_thread_context_help()->remove_topic(topic);
  } else if (loc == Keyword_location::GLOBAL_CTX) {
    // The explicit call to the global ctx is needed because we're calling this
    // from the d-tor of the thread context.
    // it won't be possible to do this without it.
    get()->remove_topic(topic);
  } else {
    get_thread_context_help()->remove_topic(topic);
    get()->remove_topic(topic);
  }
}

void Help_registry::remove_topic(Help_topic *topic) {
  auto lock = ensure_lock();
  for (auto it = m_cs_keywords.begin(); it != m_cs_keywords.end();) {
    it->second.erase(topic);
    if (it->second.empty()) {
      it = m_cs_keywords.erase(it);
    } else {
      it = std::next(it);
    }
  }

  for (auto it = m_keywords.begin(); it != m_keywords.end();) {
    it->second.erase(topic);
    if (it->second.empty()) {
      it = m_keywords.erase(it);
    } else {
      it = std::next(it);
    }
  }

  if (topic->m_parent != nullptr) {
    topic->m_parent->m_childs.erase(topic);
    topic->m_parent = nullptr;
  }

  auto it = std::find_if(m_topics.begin(), m_topics.end(),
                         [topic](const Help_topic &t) { return &t == topic; });

  if (it != m_topics.end()) {
    m_topics.erase(it);
  }
}

bool Help_registry::is_enabled(const Keyword_registry &registry,
                               const Help_topic *topic,
                               IShell_core::Mode mode) const {
  bool ret_val = false;
  if (topic->is_enabled()) {
    auto it = registry.find(topic->get_id(mode));
    if (it != registry.end()) {
      auto mask_it = it->second.find(const_cast<Help_topic *>(topic));
      if (mask_it != it->second.end()) {
        ret_val = mask_it->second.is_set(mode);
      }
    }
  }
  return ret_val;
}

std::unique_lock<std::recursive_mutex> Help_registry::ensure_lock() {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  return lock;
}

std::unique_lock<std::recursive_mutex> Help_registry::ensure_lock_threaded() {
  std::unique_lock<std::recursive_mutex> lock(m_th_help);
  return lock;
}

void Help_registry::add_help_class(const std::string &name,
                                   const std::string &parent,
                                   const std::string &upper_class,
                                   IShell_core::Mode_mask mode) {
  auto lock = ensure_lock();
  Help_topic *topic =
      add_help_topic(name, Topic_type::CLASS, name, parent, mode).back();

  // A parent class has been specified
  if (!upper_class.empty()) {
    Help_topic *ptopic = get_topic(upper_class, true);

    // If the parent class corresponding topic already exists
    // adds the relation, otherwise adds the child to the corresponding
    // orphan list
    if (ptopic) {
      inherit_members(ptopic, topic);
    } else {
      m_class_childs[upper_class].push_back(topic);
    }
  }
}

void Help_registry::inherit_members(Help_topic *parent, Help_topic *child) {
  // Creates the parent relationship
  m_class_parents[child] = parent;

  // Propagates inherited members to the child
  auto lock = ensure_lock();
  for (auto member : parent->m_childs) {
    inherit_member(child, member);
  }
}

void Help_registry::inherit_member(Help_topic *parent, Help_topic *member) {
  // If the member already exists on the target parent then is not added again
  // std::string new_id = parent->m_id + HELP_API_SPLITTER + member->m_name;
  // if (m_keywords.find(new_id) == m_keywords.end()) {
  if (member->m_name != "help") {
    Mode_mask mode(IShell_core::Mode::JavaScript);
    mode.set(IShell_core::Mode::Python);
    add_help_topic(member->m_name, member->m_type, member->m_help_tag,
                   parent->m_id, mode);

    // If the parent is not orphan the inheritance continues
    // Otherwise it will be triggered when this object is constructed
    if (m_orphans.find(parent->m_name) == m_orphans.end()) {
      std::string upper_class = parent->m_name;
      if (m_class_childs.find(upper_class) != m_class_childs.end()) {
        for (auto child : m_class_childs[upper_class]) {
          inherit_member(child, member);
        }
      }
    }
  }
}

void Help_registry::register_topic(Help_topic *topic, bool new_topic,
                                   Mode_mask mode) {
  auto lock = ensure_lock();
  if (topic->m_parent) {
    if (topic->is_api()) {
      if (topic->m_parent->is_api()) {
        topic->m_id = topic->m_parent->m_id + HELP_API_SPLITTER + topic->m_name;
      }
    } else {
      if (topic->is_command())
        topic->m_id = topic->m_parent->m_id + HELP_CMD_SPLITTER + topic->m_name;
      else
        topic->m_id =
            topic->m_parent->m_id + HELP_TOPIC_SPLITTER + topic->m_name;
    }

    topic->m_parent->m_childs.insert(topic);
  }

  register_keywords(topic, mode);

  // The help function is 'injected' into every object, we need to do the same
  if (topic->is_api_object()) {
    // for it's help data
    std::string help_tag = shcore::str_upper(topic->m_name) + "_HELP";
    std::string type;

    switch (topic->m_type) {
      case Topic_type::MODULE:
        type = "module";
        break;

      case Topic_type::CLASS:
      case Topic_type::CONSTANTS:
        type = "class";
        break;

      case Topic_type::OBJECT:
      case Topic_type::GLOBAL_OBJECT:
        type = "object";
        break;

      default:
        throw std::logic_error("Unexpected topic type");
    }

    add_help(help_tag + "_BRIEF",
             "Provides help about this " + type + " and it's members");

    add_help(help_tag + "_PARAM",
             "@param member Optional If specified, provides detailed "
             "information on the given member.");

    add_help_topic("help", Topic_type::FUNCTION, "help", topic->m_id, mode);
  }

  // If there are orphans associated to the new topic, they are properly
  // registered
  if (!topic->is_function() && !topic->is_property() &&
      m_orphans.find(topic->m_help_tag) != m_orphans.end()) {
    auto orphans = m_orphans[topic->m_help_tag];
    m_orphans.erase(topic->m_help_tag);

    for (auto orphan : orphans) {
      orphan->m_parent = topic;
      register_topic(orphan, false, mode);
    }
  }

  if (topic->is_api()) {
    // A new object inherits all it's content to the subclasses if any
    if (topic->is_api_object()) {
      if (m_class_childs.find(topic->m_name) != m_class_childs.end()) {
        for (auto subclass : m_class_childs[topic->m_name]) {
          inherit_members(topic, subclass);
        }
      }
    } else if (new_topic) {
      // A new member is added to subclasses if any
      if (m_class_childs.find(topic->m_parent->m_name) !=
          m_class_childs.end()) {
        for (auto subclass : m_class_childs[topic->m_parent->m_name]) {
          inherit_member(subclass, topic);
        }
      }
    }
  }
}

/**
 * Associates the token to all the keywords that will identify it.
 *
 * The keywords are created by splitting the topic id and adding a new keyword
 * with from each token and until the end.
 *
 * i.e.
 *
 * For an ID as: mysqlx.Session.getSchema
 *
 * The following keywords will be added:
 * - getSchema
 * - Session.getSchema
 * - mysqlx.Session.getSchema
 *
 * Each keyword is valid depending on the active shell mode, for example on API
 * topics, the ones above are valid only for JavaScript. In addition the
 * following would be registered for Python:
 * - get_schema
 * - Session.get_schema
 * - mysqlx.Session.get_schema
 *
 * Non API topics register keywords available on any mode.
 */
void Help_registry::register_keywords(Help_topic *topic, Mode_mask mode) {
  std::vector<IShell_core::Mode> modes = {IShell_core::Mode::JavaScript,
                                          IShell_core::Mode::Python,
                                          IShell_core::Mode::SQL};

  std::string splitter =
      topic->is_api() ? HELP_API_SPLITTER : HELP_TOPIC_SPLITTER;
  std::string last_splitter =
      topic->is_command() ? HELP_CMD_SPLITTER : splitter;

  for (auto shell_mode : modes) {
    if (mode.is_set(shell_mode)) {
      std::string topic_name = topic->get_name(shell_mode);
      std::vector<std::string> id_tokens;

      if (topic->m_parent)
        topic->m_parent->get_id_tokens(shell_mode, &id_tokens);

      while (!id_tokens.empty()) {
        std::string keyword = shcore::str_join(id_tokens, splitter);
        keyword += last_splitter + topic_name;
        register_keyword(keyword, shell_mode, topic);
        id_tokens.erase(id_tokens.begin());
      }

      register_keyword(topic_name, shell_mode, topic);

      if (!topic->is_api() &&
          !shcore::str_caseeq(topic_name, topic->m_help_tag)) {
        register_keyword(topic->m_help_tag, shell_mode, topic);
      }
    }
  }
}

void Help_registry::register_keyword(const std::string &keyword,
                                     IShell_core::Mode_mask mode,
                                     Help_topic *topic, bool case_sensitive) {
  if (mode.is_set(IShell_core::Mode::Python))
    register_keyword(keyword, IShell_core::Mode::Python, topic, case_sensitive);
  if (mode.is_set(IShell_core::Mode::JavaScript))
    register_keyword(keyword, IShell_core::Mode::JavaScript, topic,
                     case_sensitive);
  if (mode.is_set(IShell_core::Mode::SQL))
    register_keyword(keyword, IShell_core::Mode::SQL, topic, case_sensitive);
}

Help_registry *Help_registry::get_thread_context_help() const {
  thread_local static Help_registry instance_thread(true);
  return &instance_thread;
}

void Help_registry::register_keyword(const std::string &keyword,
                                     IShell_core::Mode mode, Help_topic *topic,
                                     bool case_sensitive) {
  auto lock = ensure_lock();
  if (!case_sensitive) {
    // The keywords topics will be created here if it doesn't exist.
    auto &topics = m_keywords[keyword];

    if (topics.find(topic) == topics.end())
      topics[topic] = Mode_mask(mode);
    else
      topics[topic].set(mode);
  } else {
    // The cs_keywords topics will be created here if it doesn't exist.
    auto &topics = m_cs_keywords[keyword];

    if (topics.find(topic) == topics.end())
      topics[topic] = Mode_mask(mode);
    else
      topics[topic].set(mode);
  }
}

std::string Help_registry::get_token(const std::string &token) {
  std::string ret_val;

  try {
    ret_val = m_help_data.at(token);
  } catch (const std::out_of_range &) {
    try {
      ret_val = get_thread_context_help()->m_help_data.at(token);
    } catch (const std::out_of_range &) {
      // noop
    }
  }

  return ret_val;
}

bool find_wildcard(const std::string &pattern, const std::string &wildcard) {
  bool found = false;
  size_t pos = 0;

  while (!found && pos != std::string::npos) {
    pos = pattern.find(wildcard, pos);

    if (pos != std::string::npos) {
      found = (pos == 0 || pattern[pos - 1] != '\\');

      if (!found) pos++;
    }
  }

  return found;
}

bool is_pattern(const std::string &pattern) {
  return find_wildcard(pattern, "*") || find_wildcard(pattern, "?");
}

template <class Iterable>
std::vector<Help_topic *> get_topics(const Iterable &topic_map,
                                     const std::string &pattern,
                                     IShell_core::Mode_mask mode) {
  std::vector<Help_topic *> ret_val;

  if (is_pattern(pattern)) {
    // First we look in the case sensitive registry
    for (auto &entry : topic_map) {
      // Verifies if the registered keyword matches the given pattern
      if (match_glob(pattern, entry.first)) {
        // Verifies the associated topics to see which are enabled for
        // the active mode
        for (auto &topic : entry.second) {
          if (topic.second.matches_any(mode)) {
            ret_val.push_back(topic.first);
          }
        }
      }
    }
  } else if (topic_map.find(pattern) != topic_map.end()) {
    auto topics = topic_map.at(pattern);
    for (auto &topic : topics) {
      if (topic.second.matches_any(mode)) {
        ret_val.push_back(topic.first);
      }
    }
  }

  return ret_val;
}

std::vector<Help_topic *> Help_registry::search_topics(
    const std::string &pattern, IShell_core::Mode_mask mode,
    bool case_sensitive) {
  // First searches on the case sensitive topics
  std::vector<Help_topic *> ret_val = get_topics(m_cs_keywords, pattern, mode);

  // If not restricted to case sensitive, searches in the case insensitive
  if (ret_val.empty() && !case_sensitive) {
    ret_val = get_topics(m_keywords, pattern, mode);
  }

  // We need to also look for the pattern in the thread context help,
  {
    std::vector<Help_topic *> thread_result =
        get_topics(get_thread_context_help()->m_cs_keywords, pattern, mode);
    ret_val.insert(ret_val.end(), thread_result.begin(), thread_result.end());
  }

  if (ret_val.empty() && !case_sensitive) {
    std::vector<Help_topic *> thread_result =
        get_topics(get_thread_context_help()->m_keywords, pattern, mode);
    ret_val.insert(ret_val.end(), thread_result.begin(), thread_result.end());
  }

  return ret_val;
}

std::vector<Help_topic *> Help_registry::search_topics(
    const std::string &pattern, IShell_core::Mode mode) {
  return search_topics(pattern, IShell_core::Mode_mask(mode), false);
}

Help_topic *Help_registry::get_topic(const std::string &id,
                                     bool allow_unexisting,
                                     const Topic_mask &type) const {
  auto topics = get_topic(this, id, true, type);
  if (topics == nullptr && !m_c_tor) {
    topics = get_topic(get_thread_context_help(), id, allow_unexisting, type);
  }

  if (!allow_unexisting && topics == nullptr) {
    throw std::logic_error("Unable to find topic '" + id + "'");
  }

  return topics;
}

Help_topic *Help_registry::get_topic(const Help_registry *registry,
                                     const std::string &id,
                                     bool allow_unexisting,
                                     const Topic_mask &type) const {
  auto it = registry->m_keywords.find(id);
  if (it != registry->m_keywords.end()) {
    auto &topics = it->second;
    if (type == Topic_mask::any()) {
      if (topics.size() > 1)
        throw std::logic_error("Non unique topic found as '" + id + "'");
      return topics.begin()->first;
    } else {
      Help_topic *found_topic = nullptr;
      int topic_count = 0;
      for (const auto &topic : topics) {
        if (type.is_set(topic.first->m_type)) {
          found_topic = topic.first;
          topic_count++;
          if (topic_count > 1)
            throw std::logic_error("Non unique topic found as '" + id + "'");
        }
      }
      return found_topic;
    }
  } else {
    if (!allow_unexisting)
      throw std::logic_error("Unable to find topic '" + id + "'");
  }

  return nullptr;
}

Help_topic *Help_registry::get_class_parent(Help_topic *topic) const {
  if (m_class_parents.find(topic) != m_class_parents.end())
    return m_class_parents.at(topic);

  if (get_thread_context_help()->m_class_parents.find(topic) !=
      get_thread_context_help()->m_class_parents.end())
    return get_thread_context_help()->m_class_parents.at(topic);
  return nullptr;
}

bool Help_registry::is_enabled(const Help_topic *topic,
                               IShell_core::Mode mode) const {
  auto ret_val = is_enabled(m_keywords, topic, mode);
  if (ret_val == false) {
    ret_val = is_enabled(get_thread_context_help()->m_keywords, topic, mode);
  }

  if (ret_val == false) {
    log_error("Help_registry::is_enabled: using an unregistered topic '%s'.",
              topic->m_id.c_str());
  }
  return ret_val;
}

Help_class_register::Help_class_register(const std::string &child,
                                         const std::string &parent,
                                         const std::string &upper_class,
                                         Help_mode mode) {
  IShell_core::Mode_mask mask;
  using Mode = IShell_core::Mode;
  if (mode == Help_mode::SCRIPTING) {
    mask = IShell_core::Mode_mask(Mode::JavaScript).set(Mode::Python);
  } else if (mode == Help_mode::JAVASCRIPT) {
    mask = IShell_core::Mode_mask(Mode::JavaScript);
  } else if (mode == Help_mode::PYTHON) {
    mask = IShell_core::Mode_mask(Mode::Python);
  } else {
    throw std::logic_error("Invalid mode for a scripting class.");
  }

  Help_registry::get()->add_help_class(child, parent, upper_class, mask);
}

Help_manager::Help_manager() {
  m_option_vals["brief"] = Help_option::Brief;
  m_option_vals["detail"] = Help_option::Detail;
  m_option_vals["categories"] = Help_option::Categories;
  m_option_vals["modules"] = Help_option::Modules;
  m_option_vals["objects"] = Help_option::Objects;
  m_option_vals["classes"] = Help_option::Classes;
  m_option_vals["functions"] = Help_option::Functions;
  m_option_vals["properties"] = Help_option::Properties;
  m_option_vals["childs"] = Help_option::Childs;
  m_option_vals["closing"] = Help_option::Closing;
  m_option_vals["example"] = Help_option::Example;

  m_registry = Help_registry::get();
}

std::vector<Help_topic *> Help_manager::search_topics(
    const std::string &pattern) {
  return Help_registry::get()->search_topics(pattern, m_mode);
}

std::vector<std::string> Help_manager::resolve_help_text(
    const Help_topic &object, const std::string &suffix) {
  std::vector<std::string> help_text;

  auto object_ptr = &object;

  while (object_ptr && help_text.empty()) {
    help_text = get_help_text(object_ptr->m_name + "_" + suffix);
    object_ptr =
        m_registry->get_class_parent(const_cast<Help_topic *>(object_ptr));
  }

  return help_text;
}

std::vector<std::string> Help_manager::get_help_text(const std::string &token) {
  int index = 0;

  std::string text = m_registry->get_token(token);

  std::vector<std::string> lines;

  while (!text.empty()) {
    if (shcore::str_beginswith(text.c_str(), "${") &&
        shcore::str_endswith(text.c_str(), "}")) {
      auto tmp = get_help_text(text.substr(2, text.size() - 3));
      lines.insert(lines.end(), tmp.begin(), tmp.end());
    } else if (shcore::str_beginswith(text.c_str(), "%{") &&
               shcore::str_endswith(text.c_str(), "}")) {
      std::string target = text.substr(2, text.size() - 3);
      std::string options;
      size_t pos = target.find(":");

      if (pos != std::string::npos) {
        options = target.substr(pos + 1);
        target = target.substr(0, pos);
      }

      lines.push_back(HELP_NO_FORMAT +
                      get_help(target, Topic_mask::any(), options));
    } else {
      lines.push_back(text);
    }

    text = m_registry->get_token(token + std::to_string(++index));
  }

  return lines;
}

std::vector<std::pair<std::string, std::string>>
Help_manager::parse_function_parameters(
    const std::vector<std::string> &parameters, std::string *signature) {
  std::vector<std::pair<std::string, std::string>> pdata;

  if (!parameters.empty()) {
    std::vector<std::string>
        fpnames;  // Parameter names as they will look in the signature
    std::vector<std::string>
        pdescs;  // Parameter descriptions as they are defined

    for (const auto &paramdef : parameters) {
      // 7 is the length of: "\param " or "@param "
      size_t start_index = 7;
      auto pname = paramdef.substr(
          start_index, paramdef.find(" ", start_index) - start_index);

      start_index += pname.size() + 1;
      std::string desc;
      std::string first_word;

      if (paramdef.size() > start_index) {
        desc = paramdef.substr(start_index);
        first_word = desc.substr(0, desc.find(" "));
      }

      // Updates parameter names to reflect the optional attribute on the
      // signature Removed the options word from the description
      if (shcore::str_caseeq(first_word, HELP_OPTIONAL_STR)) {
        if (fpnames.empty()) {
          fpnames.push_back("[" + pname + "]");  // First creates: [pname]
        } else {
          fpnames[fpnames.size() - 1].append("[");
          fpnames.push_back(pname + "]");  // Rest creates: pname[, pname]
        }
        desc = desc.substr(first_word.size() + 1);  // Deletes the optional word
        desc[0] = std::toupper(desc[0]);
      } else {
        fpnames.push_back(pname);
      }

      pdata.push_back({pname, desc});
    }

    // Continues the syntax
    if (signature) *signature = "(" + shcore::str_join(fpnames, ", ") + ")";
  } else {
    if (signature) *signature = "()";
  }

  return pdata;
}

void Help_manager::add_simple_function_help(const Help_topic &function,
                                            std::vector<std::string> *sections,
                                            cli::Shell_cli_mapper *cli) {
  std::string name = function.get_base_name();
  Help_topic *parent = function.m_parent;

  // Starts the syntax
  std::string display_name = function.get_name(m_mode);
  std::string syntax =
      textui::bold(HELP_TITLE_SYNTAX) + HEADER_CONTENT_SEPARATOR;

  std::string fsyntax;

  if (cli) {
    fsyntax = shcore::str_join(cli->get_object_chain(), " ") + " " +
              textui::bold(shcore::from_camel_case_to_dashes(name));
  } else {
    if (parent->is_api()) {
      if (parent->is_class()) {
        fsyntax += "<" + parent->m_name + ">." + textui::bold(display_name);
      } else {
        std::string parent_name = function.m_parent->get_id(
            m_mode, Topic_id_mode::EXCLUDE_CATEGORIES);
        fsyntax += parent_name + HELP_API_SPLITTER + textui::bold(display_name);
      }
    } else {
      fsyntax += textui::bold(display_name);
    }
  }
  // Gets the function signature
  std::string signature = get_signature(function, cli);

  // Ellipsis indicates the function is overloaded
  std::vector<std::string> signatures;
  if (signature == "(...)") {
    signatures = resolve_help_text(*parent, name + "_SIGNATURE");
    for (auto &item : signatures) {
      item = fsyntax + item;
    }
  } else {
    signatures.push_back(fsyntax + signature);
  }

  syntax += textui::format_markup_text(signatures, MAX_HELP_WIDTH,
                                       SECTION_PADDING, false);
  sections->push_back(syntax);

  std::vector<std::pair<std::string, std::string>> pdata;
  if (signature != "()" || (cli && !signature.empty())) {
    auto params = resolve_help_text(*parent, name + "_PARAM");

    pdata = parse_function_parameters(params);

    if (!pdata.empty()) {
      // Describes the parameters
      std::string where = textui::bold("WHERE") + HEADER_CONTENT_SEPARATOR;
      std::vector<std::string> where_items;

      size_t index;
      for (index = 0; index < params.size(); index++) {
        // for CLI only anonymous parameters get explained on the WHERE section,
        // the rest (the options) will be explained on the OPTIONS section
        if (!cli || !cli->has_argument_options(pdata[index].first) ||
            cli->metadata()->param_codes[index] == 'C') {
          // Padding includes two spaces at the beggining, colon and space at
          // the end like:"  name: "<description>.
          size_t desc_padding = SECTION_PADDING;
          desc_padding += pdata[index].first.size() + 2;
          std::vector<std::string> data = {pdata[index].second};

          std::string desc =
              format_help_text(&data, MAX_HELP_WIDTH, desc_padding, true);

          if (!desc.empty()) {
            desc.replace(SECTION_PADDING, pdata[index].first.size() + 1,
                         pdata[index].first + ":");

            where_items.push_back(desc);
          }
        }
      }

      if (!where_items.empty()) {
        where += shcore::str_join(where_items, "\n");
        sections->push_back(where);
      }
    }
  }

  auto returns = resolve_help_text(*function.m_parent, name + "_RETURNS");
  if (!returns.empty()) {
    std::string ret = textui::bold("RETURNS") + HEADER_CONTENT_SEPARATOR;
    // Removes the @returns tag
    returns[0] = returns[0].substr(returns[0].find_first_of(" \t") + 1);
    ret += format_help_text(&returns, MAX_HELP_WIDTH, SECTION_PADDING, true);
    sections->push_back(ret);
  }

  if (cli) {
    add_cli_options_section(name + "_DETAIL", *parent, pdata, sections,
                            SECTION_PADDING, cli);
  } else {
    // Description
    add_member_section(HELP_TITLE_DESCRIPTION, name + "_DETAIL", *parent,
                       sections, SECTION_PADDING);

    // Exceptions
    add_member_section("EXCEPTIONS", name + "_THROWS", *parent, sections,
                       SECTION_PADDING);

    add_examples_section(parent->m_name + "_" + name + "_EXAMPLE", sections,
                         SECTION_PADDING);
  }
}

std::string Help_manager::get_signature(const Help_topic &function,
                                        cli::Shell_cli_mapper *cli) {
  std::string signature;
  Help_topic *parent = function.m_parent;
  std::string name = function.get_base_name();
  std::vector<std::string> signatures;

  if (cli) {
    for (size_t index = 0; index < cli->metadata()->param_codes.size();
         index++) {
      std::string param_signature;

      const auto &param = cli->metadata()->signature[index];

      if (cli->metadata()->param_codes[index] == 'C' ||
          cli->metadata()->param_codes[index] == 'D' ||
          !cli->has_argument_options(param->name)) {
        param_signature = "<" + param->name + ">";
      } else if (cli->metadata()->param_codes[index] == 'A') {
        param_signature = "--" + param->name;
        auto validator = param->validator<List_validator>();
        if (validator->element_type() == shcore::Value_type::Undefined) {
          param_signature.append("[:<type>]=<value>...");
        } else {
          param_signature.append("=<" + get_cli_param_type(param->type()) +
                                 " list>...");
        }
      } else {
        param_signature = "--" + param->name;
        if (param->type() == shcore::Value_type::Undefined) {
          param_signature.append("[:<type>]=<value>");
        } else {
          param_signature.append("=<" + get_cli_param_type(param->type()) +
                                 ">");
        }
      }

      if (param->flag == shcore::Param_flag::Optional) {
        signature.append(" [" + param_signature + "]");
      } else {
        signature.append(" " + param_signature + "");
      }
    }
  } else {
    signatures = resolve_help_text(*parent, name + "_SIGNATURE");

    if (signatures.empty()) {
      std::vector<std::string> params;
      // No signatures, we create it using the defined parameters
      params = resolve_help_text(*parent, name + "_PARAM");

      parse_function_parameters(params, &signature);
    } else if (signatures.size() > 1) {
      signature = "(...)";
    } else {
      signature = signatures[0];
    }
  }

  return signature;
}

/**
 * Chain function processing for help generation.
 *
 * Format is simple:
 *
 * fname1.fname2.fname3.fname4
 *
 * Syntax for optional functions is between brackets, i.e.:
 *
 * fname1.[fname2]
 *
 * Syntax for dependent functions uses '->' instead of '.', i.e.:
 *
 * limit->[offset]
 *
 * NOTE: Current chains are simple so nothing more elaborated is required.
 */
void Help_manager::add_chained_function_help(
    const Help_topic &main_function, std::vector<std::string> *sections) {
  Help_topic *parent = main_function.m_parent;

  // Gets the chain definition already formatted for the active language
  auto chain_definition = get_help_text(
      parent->m_name + "_" + main_function.get_base_name() + "_CHAINED");
  format_help_text(&chain_definition, MAX_HELP_WIDTH, 0, false);

  // Gets the main chain definition elements
  auto functions = shcore::split_string(chain_definition[0], HELP_API_SPLITTER);

  // First element is the chain definition class
  std::string target_class = functions[0];
  functions.erase(functions.begin());

  std::map<std::string, std::string> signatures;

  std::vector<std::string> full_syntax;
  std::vector<Help_topic *> function_topics;

  auto resolve = [this, &signatures, &function_topics](const std::string &id) {
    // Searches the topic corresponding to the given id
    auto topics = m_registry->search_topics(id, m_mode);
    assert(topics.size() == 1);
    Help_topic *function = *topics.begin();
    function_topics.push_back(function);

    // Now retrieves the display signature
    signatures[function->get_name(m_mode)] = get_signature(*function);
  };

  for (auto mfunction : functions) {
    bool optional = false;
    bool child_optional = false;

    if (mfunction[0] == '[') {
      optional = true;
      mfunction = mfunction.substr(1, mfunction.length() - 2);
    }

    // Verifies dependent functions
    auto dependents = shcore::split_string(mfunction, "->");

    // Gets topic and display signature
    resolve(target_class + HELP_API_SPLITTER + dependents[0]);

    mfunction = dependents[0] + signatures[dependents[0]];

    // Removes the parent function
    dependents.erase(dependents.begin());

    // There's a function that is valid only on the context of a parent function
    // This section will format the child function
    if (!dependents.empty()) {
      std::string dependent = dependents[0];

      if (dependent[0] == '[') {
        child_optional = true;
        dependent = dependent.substr(1, dependent.length() - 2);
      }

      // Gets topic and display signature
      resolve(target_class + HELP_API_SPLITTER + dependent);

      dependent += signatures[dependent];

      if (child_optional) {
        mfunction += "[." + dependent + "]";
      } else {
        mfunction += "." + dependent;
      }
    }

    if (optional)
      full_syntax.push_back("[." + mfunction + "]");
    else
      full_syntax.push_back("." + mfunction);
  }

  // Adds the full syntax section
  std::string syntax;
  size_t padding = SECTION_PADDING + parent->m_name.length();
  syntax =
      textui::format_markup_text(full_syntax, MAX_HELP_WIDTH, padding, false);
  syntax.replace(SECTION_PADDING, parent->m_name.length(), parent->m_name);
  sections->push_back(textui::bold("SYNTAX") + HEADER_CONTENT_SEPARATOR +
                      syntax);

  // The main description is taken from the parent class
  std::string tag =
      parent->m_name + "_" + function_topics[0]->m_name + "_DETAIL";
  add_section(HELP_TITLE_DESCRIPTION, tag, sections, SECTION_PADDING);

  // Padding for the chained function descriptions
  padding = SECTION_PADDING + ITEM_DESC_PADDING;

  // function_topics.erase(function_topics.begin());

  // Now adds a light version of the rest of the chained functions
  while (!function_topics.empty()) {
    std::string name = function_topics[0]->get_base_name();
    std::string dname = function_topics[0]->get_name(m_mode);
    std::string signature = signatures[dname];
    std::string space(SECTION_PADDING, ' ');
    std::vector<std::string> section;

    std::string title = space + textui::bold(dname + signature);

    if (signature == "(...)") {
      section.push_back("This function has the following overloads:");
      for (auto &real_signature :
           get_help_text(target_class + "_" + name + "_SIGNATURE")) {
        section.push_back("@li " + dname + real_signature);
      }

      add_section_data(title, &section, sections, padding);

      // Since title has been added, we clean it up for the section below
      title = "";
    }

    auto details = m_registry->get_token(target_class + "_" + name + "_DETAIL");

    if (details.empty())
      add_section(title, target_class + "_" + name + "_BRIEF", sections,
                  padding);
    else
      add_section(title, target_class + "_" + name + "_DETAIL", sections,
                  padding);

    auto single_title = space + "Example";
    auto multi_title = space + "Examples";
    add_examples_section(target_class + "_" + name + "_EXAMPLE", sections,
                         padding, single_title, multi_title);

    function_topics.erase(function_topics.begin());
  }
}

std::string Help_manager::format_list_description(
    const std::string &name, std::vector<std::string> *help_text,
    size_t name_max_len, size_t lpadding, const std::string &alias,
    size_t alias_max_len) {
  std::string desc;

  // Adds the extra espace before the descriptions begin
  // and the three spaces for the " - " before the property names
  size_t dpadding = lpadding + name_max_len + 4;

  // Adds alias max length + a space
  if (alias_max_len) dpadding += alias_max_len + 1;

  if (help_text && !help_text->empty()) {
    desc = format_help_text(help_text, MAX_HELP_WIDTH, dpadding, true);

    if (!alias.empty())
      desc.replace(dpadding - alias_max_len - 1, alias.size(), alias);

    desc.replace(lpadding, name.size() + 3, " - " + name);
  } else {
    if (!alias.empty()) {
      desc = std::string(name_max_len, ' ');
      desc.replace(lpadding, name.size() + 3, " - " + name);
      desc += " " + alias;
    } else {
      desc = " - " + name;
    }
  }

  return desc;
}

size_t Help_manager::get_max_topic_length(
    const std::vector<Help_topic *> &topics, bool members, bool alias) const {
  size_t ret_val = 0;

  if (members) {
    // On members uses the corresponding name for the style
    for (auto topic : topics) {
      size_t new_len = topic->get_name(m_mode).length();
      if (new_len > ret_val) ret_val = new_len;
    }
  } else {
    // On topics it may use either the name or the alias
    for (auto topic : topics) {
      size_t new_len;
      if (alias)
        new_len = m_registry->get_token(topic->m_help_tag + "_ALIAS").length();
      else
        new_len = topic->m_name.length();

      if (new_len > ret_val) ret_val = new_len;
    }
  }

  return ret_val;
}

std::vector<std::string> Help_manager::get_topic_brief(Help_topic *topic) {
  auto help_text = get_help_text(topic->m_help_tag + "_BRIEF");

  // Deprecation notices are added as part of the description on list format
  auto deprecated = get_help_text(topic->m_help_tag + "_DEPRECATED");
  for (const auto &text : deprecated) {
    help_text.push_back(text);
  }
  return help_text;
}

std::string Help_manager::format_topic_list(
    const std::vector<Help_topic *> &topics, size_t lpadding, bool alias) {
  std::vector<std::string> formatted;

  // Gets the required padding for the descriptions
  size_t name_max_len = get_max_topic_length(topics, false);
  size_t alias_max_len = 0;

  if (alias) alias_max_len = get_max_topic_length(topics, false, true);

  for (auto topic : topics) {
    std::string alias_str;
    if (alias) alias_str = m_registry->get_token(topic->m_help_tag + "_ALIAS");

    auto help_text = get_topic_brief(topic);

    formatted.push_back(format_list_description(topic->m_name, &help_text,
                                                name_max_len, lpadding,
                                                alias_str, alias_max_len));
  }

  return shcore::str_join(formatted, "\n");
}

std::vector<std::string> Help_manager::get_member_brief(Help_topic *member) {
  std::string tag = member->m_help_tag + "_BRIEF";
  auto help_text = resolve_help_text(*member->m_parent, tag);

  // Deprecation notices are added as part of the description on list format
  tag = member->m_help_tag + "_DEPRECATED";
  auto deprecated = resolve_help_text(*member->m_parent, tag);
  for (const auto &text : deprecated) {
    help_text.push_back(text);
  }

  return help_text;
}

std::string Help_manager::format_member_list(
    const std::vector<Help_topic *> &topics, size_t lpadding,
    bool do_signatures,
    std::function<std::string(Help_topic *)> format_name_cb) {
  std::vector<std::string> descriptions;
  for (auto member : topics) {
    std::string description(format_name_cb ? 0 : SECTION_PADDING, ' ');
    description +=
        format_name_cb ? format_name_cb(member) : member->get_name(m_mode);

    // If it is a function we need to retrieve the signature
    if (do_signatures && member->is_function()) {
      std::string name = member->get_base_name();
      auto chain_definition =
          resolve_help_text(*member->m_parent, name + "_CHAINED");

      if (chain_definition.empty()) {
        description += get_signature(*member);
      } else {
        // When the member is a chained function, the signature needs to be
        // function at the operation handler
        auto tokens = shcore::str_split(chain_definition[0], HELP_API_SPLITTER);
        auto op_name = tokens[0] + HELP_API_SPLITTER + name;
        auto operations = m_registry->search_topics(op_name, m_mode);
        auto operation = *operations.begin();
        assert(operations.size() == 1);
        description += get_signature(*operation);
      }
    }

    description += "\n";

    // Members could now be objects, briefs need to be retrieved
    // accordingly
    std::vector<std::string> help_text;
    if (member->is_object() || member->is_class())
      help_text = get_topic_brief(member);
    else
      help_text = get_member_brief(member);

    if (!help_text.empty()) {
      description += format_help_text(&help_text, MAX_HELP_WIDTH,
                                      lpadding + ITEM_DESC_PADDING, true);
    }

    descriptions.push_back(description);
  }

  return shcore::str_join(descriptions, "\n\n");
}

void Help_manager::add_childs_section(const std::vector<Help_topic *> &childs,
                                      std::vector<std::string> *sections,
                                      size_t lpadding, bool members,
                                      const std::string &tag,
                                      const std::string &default_title,
                                      bool alias) {
  if (!childs.empty()) {
    // Child list can be configured to be ignored
    if (m_registry->get_token(tag + "_DESC") != HELP_IGNORE_STR) {
      std::string title = m_registry->get_token(tag + "_TITLE");
      if (title.empty()) title = default_title;

      size_t count = sections->size();
      add_section("", tag + "_DESC", sections, lpadding);

      std::string section_data;
      if (count == sections->size()) {
        section_data = textui::bold(title);
        section_data += HEADER_CONTENT_SEPARATOR;
      }

      if (members)
        section_data += format_member_list(childs, SECTION_PADDING);
      else
        section_data += format_topic_list(childs, NO_PADDING, alias);

      sections->push_back(section_data);

      add_section("", tag + "_CLOSING_DESC", sections, lpadding);
    }
  }
}

void Help_manager::add_name_section(const Help_topic &topic,
                                    std::vector<std::string> *sections,
                                    cli::Shell_cli_mapper *cli) {
  std::string dname;
  if (cli) {
    dname = shcore::from_camel_case_to_dashes(topic.get_base_name());
  } else {
    dname = topic.get_name(m_mode);
  }

  std::string formatted;
  std::vector<std::string> brief;

  if (topic.is_member() && !topic.is_object())
    brief = resolve_help_text(*topic.m_parent, topic.m_help_tag + "_BRIEF");
  else
    brief = get_help_text(topic.m_help_tag + "_BRIEF");

  if (!brief.empty()) {
    formatted = format_help_text(&brief, MAX_HELP_WIDTH,
                                 dname.size() + 3 + SECTION_PADDING, true);

    formatted[dname.size() + 1 + SECTION_PADDING] = '-';
    formatted.replace(SECTION_PADDING, dname.size(), textui::bold(dname));
  } else {
    formatted = std::string(SECTION_PADDING, ' ');
    formatted += textui::bold(dname);
  }

  sections->push_back(textui::bold("NAME") + HEADER_CONTENT_SEPARATOR +
                      formatted);
}

std::string Help_manager::format_object_help(const Help_topic &object,
                                             const Help_options &options) {
  std::string tag = object.m_help_tag;
  size_t lpadding = object.is_api() ? SECTION_PADDING : NO_PADDING;

  std::vector<std::string> sections;
  std::string description_title;

  // The name section is not for all the objects
  if (object.is_api()) {
    if (options.is_set(Help_option::Name)) add_name_section(object, &sections);

    description_title = HELP_TITLE_DESCRIPTION;
  }

  if (options.is_set(Help_option::Syntax)) {
    // This is an object instance, so it's a property, we follow
    // Properties way for formatting
    if (object.is_object() && object.m_parent &&
        (object.m_parent->is_object() || object.m_parent->is_module())) {
      std::string syntax =
          textui::bold(HELP_TITLE_SYNTAX) + HEADER_CONTENT_SEPARATOR;

      syntax += std::string(SECTION_PADDING, ' ');

      std::string parent_name =
          object.m_parent->get_id(m_mode, Topic_id_mode::EXCLUDE_CATEGORIES);

      std::string display_name = object.m_name;
      if (object.is_member()) display_name = object.get_name(m_mode);

      syntax += parent_name + HELP_API_SPLITTER + textui::bold(display_name);

      sections.push_back(syntax);
    }
  }

  if (options.is_set(Help_option::Detail)) {
    size_t count = sections.size();

    if (object.is_topic()) {
      add_section("", tag, &sections, lpadding);
    } else {
      add_section(description_title, tag + "_DETAIL", &sections, lpadding);

      if (count == sections.size())
        add_section(description_title, tag + "_BRIEF", &sections, lpadding);
    }
  }

  // Classifies the child topics
  std::vector<Help_topic *> cat, prop, func, mod, cls, obj, consts, others;
  {
    auto lock = Help_registry::get()->ensure_lock();
    for (auto child : object.m_childs) {
      if (child->is_enabled(m_mode)) {
        switch (child->m_type) {
          case Topic_type::MODULE:
            mod.push_back(child);
            break;
          case Topic_type::CLASS:
            cls.push_back(child);
            break;
          case Topic_type::GLOBAL_OBJECT:
            obj.push_back(child);
            break;
          case Topic_type::CONSTANTS:
            consts.push_back(child);
            break;
          case Topic_type::FUNCTION:
            func.push_back(child);
            break;
          case Topic_type::PROPERTY:
          case Topic_type::OBJECT:
            prop.push_back(child);
            break;
          case Topic_type::CATEGORY:
          case Topic_type::SQL:
            cat.push_back(child);
            break;
          case Topic_type::TOPIC:
          case Topic_type::COMMAND:
            others.push_back(child);
            break;
        }
      }
    }
  }

  if (options.is_set(Help_option::Constants)) {
    add_childs_section(consts, &sections, lpadding, NO_MEMBER_TOPICS,
                       tag + "_CONSTANTS", "CONSTANTS");
  }

  if (options.is_set(Help_option::Properties)) {
    add_childs_section(prop, &sections, lpadding, MEMBER_TOPICS,
                       tag + "_PROPERTIES", "PROPERTIES");
  }

  if (options.is_set(Help_option::Objects)) {
    add_childs_section(obj, &sections, lpadding, NO_MEMBER_TOPICS,
                       tag + "_OBJECTS", "OBJECTS");
  }

  if (options.is_set(Help_option::Functions)) {
    add_childs_section(func, &sections, lpadding, MEMBER_TOPICS,
                       tag + "_FUNCTIONS", "FUNCTIONS");
  }

  if (options.is_set(Help_option::Classes)) {
    add_childs_section(cls, &sections, lpadding, NO_MEMBER_TOPICS,
                       tag + "_CLASSES", "CLASSES");
  }

  if (options.is_set(Help_option::Modules)) {
    add_childs_section(mod, &sections, lpadding, NO_MEMBER_TOPICS,
                       tag + "_MODULES", "MODULES");
  }

  if (options.is_set(Help_option::Categories)) {
    add_childs_section(cat, &sections, lpadding, NO_MEMBER_TOPICS,
                       tag + "_CATEGORIES", "RELATED CATEGORIES");
  }

  if (options.is_set(Help_option::Childs)) {
    add_childs_section(others, &sections, lpadding, NO_MEMBER_TOPICS,
                       tag + "_CHILDS", "RELATED TOPICS",
                       object.m_name == shcore::Help_registry::HELP_COMMANDS);
  }

  if (options.is_set(Help_option::Closing))
    add_section("", tag + "_CLOSING", &sections, lpadding);

  if (options.is_set(Help_option::Example))
    add_examples_section(tag + "_EXAMPLE", &sections, lpadding);

  return shcore::str_join(sections, "\n\n");
}

void Help_manager::add_section_data(const std::string &title,
                                    std::vector<std::string> *details,
                                    std::vector<std::string> *sections,
                                    size_t padding, bool insert_blank_lines) {
  if (!details->empty()) {
    std::string section;

    if (!title.empty())
      section += (mysqlshdk::textui::bold(title) + HEADER_CONTENT_SEPARATOR);

    section.append(
        format_help_text(details, MAX_HELP_WIDTH, padding, insert_blank_lines));

    sections->push_back(section);
  }
}

void Help_manager::add_member_section(const std::string &title,
                                      const std::string &tag,
                                      const Help_topic &parent,
                                      std::vector<std::string> *sections,
                                      size_t padding) {
  auto details = resolve_help_text(parent, tag);
  add_section_data(title, &details, sections, padding);
}

void Help_manager::add_cli_options_section(
    const std::string &tag, const Help_topic &parent,
    const std::vector<std::pair<std::string, std::string>> &pdata,
    std::vector<std::string> *sections, size_t padding,
    cli::Shell_cli_mapper *cli) {
  if (!cli->options().empty()) {
    auto option_data = parse_cli_option_data(resolve_help_text(parent, tag));
    std::vector<std::string> options_details;

    for (size_t index = 0; index < cli->metadata()->signature.size(); index++) {
      const auto &param = cli->metadata()->signature[index];
      const auto &param_code = cli->metadata()->param_codes[index];

      // Only parameters with named options
      if (cli->has_argument_options(param->name)) {
        if (param_code == 'C') {
          // NO-OP: Connection options are not being included to avoid making it
          // spammy.
        } else if (param_code == 'D') {
          auto validator = param->validator<Option_validator>();
          const auto &allowed_list = validator->allowed();
          for (const auto &allowed : allowed_list) {
            // Only options that are enabled for CLI
            if (allowed->cmd_line_enabled) {
              std::string option_detail = format_cli_option(allowed);

              auto param_data = option_data.find(allowed->name);
              if (param_data != option_data.end()) {
                std::vector<std::string> brief{param_data->second};
                option_detail += "\n";
                option_detail += format_help_text(
                    &brief, MAX_HELP_WIDTH, padding + ITEM_DESC_PADDING, true);
              }
              options_details.push_back(option_detail);
            }
          }
        } else {
          std::string option_detail = format_cli_option(param);

          for (const auto &param_data : pdata) {
            if (param_data.first == param->name) {
              if (!param_data.second.empty()) {
                std::vector<std::string> brief{param_data.second};

                // The option type is not needed on the description as it is
                // already displayed in the syntax.
                brief[0] = strip_param_type(brief[0]);
                option_detail += "\n";
                option_detail += format_help_text(
                    &brief, MAX_HELP_WIDTH, padding + ITEM_DESC_PADDING, true);
              }
              break;
            }
          }
          options_details.push_back(option_detail);
        }
      }
    }

    if (!options_details.empty()) {
      std::string section = mysqlshdk::textui::bold("OPTIONS");
      section += HEADER_CONTENT_SEPARATOR;
      section += shcore::str_join(options_details, "\n\n");
      sections->push_back(section);
    }
  }
}

void Help_manager::add_section(const std::string &title, const std::string &tag,
                               std::vector<std::string> *sections,
                               size_t padding, bool insert_blank_lines) {
  std::vector<std::string> details;
  details = get_help_text(tag);
  add_section_data(title, &details, sections, padding, insert_blank_lines);
}

std::string Help_manager::format_function_help(const Help_topic &function,
                                               cli::Shell_cli_mapper *cli) {
  std::string name = function.get_base_name();
  std::vector<std::string> sections;

  // Brief Description
  add_name_section(function, &sections, cli);

  auto chain_definition =
      resolve_help_text(*function.m_parent, name + "_CHAINED");

  std::string additional_help;
  if (chain_definition.empty()) {
    add_simple_function_help(function, &sections, cli);
  } else {
    add_chained_function_help(function, &sections);
  }

  return shcore::str_join(sections, "\n\n");
}

std::string Help_manager::format_property_help(const Help_topic &property) {
  std::string name = property.m_name;
  Help_topic *parent = property.m_parent;
  std::vector<std::string> sections;

  add_name_section(property, &sections);

  // Syntax
  std::string display_name = property.get_name(m_mode);
  std::string syntax =
      textui::bold(HELP_TITLE_SYNTAX) + HEADER_CONTENT_SEPARATOR;
  syntax += std::string(SECTION_PADDING, ' ');

  if (property.m_parent->is_class()) {
    syntax +=
        "<" + property.m_parent->m_name + ">." + textui::bold(display_name);
  } else {
    std::string parent_name =
        property.m_parent->get_id(m_mode, Topic_id_mode::EXCLUDE_CATEGORIES);

    syntax += parent_name + HELP_API_SPLITTER + textui::bold(display_name);
  }

  sections.push_back(syntax);

  // Details
  add_member_section(HELP_TITLE_DESCRIPTION, name + "_DETAIL", *parent,
                     &sections, SECTION_PADDING);

  return shcore::str_join(sections, "\n\n");
}

std::string Help_manager::format_command_help(const Help_topic &command,
                                              const Help_options &options) {
  std::vector<std::string> sections;
  std::string tag = command.m_help_tag;

  if (options.is_set(Help_option::Name)) {
    std::string description = "<b>" + command.m_name + "</b> - " +
                              m_registry->get_token(tag + "_BRIEF");

    std::string intro = mysqlshdk::textui::bold("NAME") + "\n";

    intro += textui::format_markup_text({description}, MAX_HELP_WIDTH,
                                        SECTION_PADDING, true);

    sections.push_back(intro);
  }

  if (options.is_set(Help_option::Syntax))
    add_section(HELP_TITLE_SYNTAX, tag + "_SYNTAX", &sections, SECTION_PADDING,
                false);

  if (options.is_set(Help_option::Detail))
    add_section(HELP_TITLE_DESCRIPTION, tag + "_DETAIL", &sections,
                SECTION_PADDING);

  if (options.is_set(Help_option::Example))
    add_examples_section(tag + "_EXAMPLE", &sections, SECTION_PADDING);

  return shcore::str_join(sections, "\n\n");
}

void Help_manager::add_examples_section(const std::string &tag,
                                        std::vector<std::string> *sections,
                                        size_t padding,
                                        const std::string &single_title,
                                        const std::string &multi_title) {
  std::string current_tag(tag);
  std::string example = m_registry->get_token(current_tag);

  if (!example.empty()) {
    int index = 0;

    std::vector<std::string> examples;
    while (!example.empty()) {
      std::string formatted;
      std::vector<std::string> data = {example};
      formatted = format_help_text(&data, MAX_HELP_WIDTH, padding, true);

      auto desc = get_help_text(current_tag + "_DESC");
      if (!desc.empty()) {
        formatted += "\n";
        formatted += format_help_text(&desc, MAX_HELP_WIDTH,
                                      padding + ITEM_DESC_PADDING, true);
      }

      examples.push_back(formatted);
      current_tag = tag + std::to_string(++index);

      example = m_registry->get_token(current_tag);
    }

    std::string section;

    if (examples.size() == 1)
      section = mysqlshdk::textui::bold(single_title);
    else
      section = mysqlshdk::textui::bold(multi_title);
    section += HEADER_CONTENT_SEPARATOR;
    section += shcore::str_join(examples, "\n\n");
    sections->push_back(section);
  }
}

std::map<std::string, std::string> Help_manager::preprocess_help(
    std::vector<std::string> *text_lines) const {
  std::map<std::string, std::string> no_format_data;
  for (auto &text : *text_lines) {
    if (shcore::str_beginswith(text.c_str(), HELP_NO_FORMAT)) {
      std::string id = HELP_NO_FORMAT;
      std::string data = text.substr(id.size());
      id += "[" + std::to_string(no_format_data.size()) + "]";
      no_format_data[id] = data;
      text = id;
    } else {
      size_t start;
      size_t end;

      start = text.find("<<<");
      while (start != std::string::npos) {
        end = text.find(">>>", start);
        if (end == std::string::npos)
          throw std::logic_error("Unterminated <<< in documentation");

        std::string member_name = text.substr(start + 3, end - start - 3);

        if (m_mode == IShell_core::Mode::Python)
          member_name = shcore::from_camel_case(member_name);

        text.replace(start, end - start + 3, member_name);

        start = text.find("<<<");
      }
    }
  }

  return no_format_data;
}

std::string Help_manager::format_help_text(std::vector<std::string> *lines,
                                           size_t width, size_t left_padding,
                                           bool paragraph_per_line) const {
  std::map<std::string, std::string> no_formats;
  no_formats = preprocess_help(lines);
  std::string formatted = textui::format_markup_text(
      *lines, width, left_padding, paragraph_per_line);

  for (auto item : no_formats) {
    formatted = shcore::str_replace(formatted, item.first, item.second);
  }

  return formatted;
}

std::string Help_manager::get_help(const Help_topic &topic,
                                   const Help_options &options,
                                   cli::Shell_cli_mapper *cli) {
  switch (topic.m_type) {
    case Topic_type::CATEGORY:
    case Topic_type::TOPIC:
    case Topic_type::MODULE:
    case Topic_type::CLASS:
    case Topic_type::OBJECT:
    case Topic_type::CONSTANTS:
    case Topic_type::GLOBAL_OBJECT: {
      return format_object_help(topic, options);
    }
    case Topic_type::FUNCTION: {
      return format_function_help(topic, cli);
    }
    case Topic_type::PROPERTY: {
      return format_property_help(topic);
    }
    case Topic_type::COMMAND:
      return format_command_help(topic, options);
    case Topic_type::SQL:
      throw std::logic_error("Unable to get help for topic...");
  }

  return "";
}

std::string Help_manager::get_help(const std::string &topic_id, Topic_mask type,
                                   const std::string &options) {
  Help_options hoptions;

  if (options.empty()) {
    hoptions = Help_options::all();
  } else {
    std::vector<std::string> tokens = shcore::str_split(options, ",");
    for (auto &token : tokens) {
      if (m_option_vals.find(token) != m_option_vals.end()) {
        hoptions.set(m_option_vals[token]);
      } else {
        throw std::logic_error("get_help: invalid option '" + token + "'");
      }
    }
  }

  return get_help(topic_id, type, hoptions);
}

std::string Help_manager::get_help(const std::string &topic_id, Topic_mask type,
                                   const Help_options &options) {
  auto topics = search_topics(topic_id);

  if (!type.empty()) {
    for (size_t index = 0; index < topics.size(); index++) {
      if (!type.is_set(topics[index]->m_type)) {
        topics.erase(topics.begin() + index);
        index--;
      }
    }
  }

  if (topics.empty())
    throw std::logic_error("Unable to find help for '" + topic_id + "'");
  else if (topics.size() > 1)
    throw std::logic_error("Multiple matches found for topic '" + topic_id +
                           "'");

  return get_help(**topics.begin(), options);
}
}  // namespace shcore
