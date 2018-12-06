/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

bool icomp::operator()(const std::string &lhs, const std::string &rhs) const {
  return str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
}

bool Help_topic::is_api() const {
  return m_type == Topic_type::MODULE || m_type == Topic_type::CLASS ||
         m_type == Topic_type::OBJECT || m_type == Topic_type::FUNCTION ||
         m_type == Topic_type::PROPERTY;
}

bool Help_topic::is_api_object() const {
  return m_type == Topic_type::MODULE || m_type == Topic_type::CLASS ||
         m_type == Topic_type::OBJECT;
}

bool Help_topic::is_member() const {
  return m_type == Topic_type::FUNCTION || m_type == Topic_type::PROPERTY;
}

bool Help_topic::is_enabled(IShell_core::Mode mode) const {
  return m_help->is_enabled(this, mode);
}

std::string Help_topic::get_name(IShell_core::Mode mode) const {
  // Member topics may have different names bsaed on the scripting
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
  while (category->m_parent &&
         category->m_parent->m_name != Help_registry::HELP_ROOT) {
    category = category->m_parent;
  }

  return category;
}

std::string Help_topic::get_id(bool fully_qualified,
                               IShell_core::Mode mode) const {
  std::string ret_val = m_id;

  if (is_api()) {
    // API Members require the last token to be formatted based on the current
    // shell mode
    if (is_member()) {
      ret_val = m_id.substr(0, m_id.rfind(HELP_API_SPLITTER) + 1);
      ret_val += get_name(mode);
    }
  } else {
    if (fully_qualified) {
      // Fully quialified excludes the first token "Contents"
      ret_val = m_id.substr(m_id.find(HELP_TOPIC_SPLITTER) + 1);
    } else {
      // Otherwise the name is returned
      ret_val = m_name;
    }
  }

  return ret_val;
}

const char Help_registry::HELP_ROOT[] = "Contents";
const char Help_registry::HELP_COMMANDS[] = "Shell Commands";
const char Help_registry::HELP_SQL[] = "SQL Syntax";

Help_registry::Help_registry()
    : m_help_data(Help_registry::icomp), m_keywords(Help_registry::icomp) {
  // The Contents category is registered right away since it is the root
  // Of the help system
  add_help_topic(HELP_ROOT, Topic_type::CATEGORY, "CONTENTS", "",
                 Mode_mask::all());

  // The SQL category is also registered right away, it doesn't have content
  // at the client side but will be used as parent for SQL topics
  add_help_topic(HELP_SQL, Topic_type::SQL, "SQL_CONTENTS", HELP_ROOT,
                 Mode_mask::all());
}

bool Help_registry::icomp(const std::string &lhs, const std::string &rhs) {
  return str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
}

Help_registry *Help_registry::get() {
  static Help_registry instance;
  return &instance;
}

void Help_registry::add_help(const std::string &token,
                             const std::string &data) {
  m_help_data[token] = data;
}

void Help_registry::add_help(const std::string &prefix, const std::string &tag,
                             const std::string &data) {
  auto full_prefix = prefix + "_" + tag;

  add_help(full_prefix, data);
}

void Help_registry::add_help(const std::string &prefix, const std::string &tag,
                             const std::vector<std::string> &data) {
  size_t sequence = 0;
  add_help(prefix, tag, &sequence, data);
}

void Help_registry::add_help(const std::string &prefix, const std::string &tag,
                             size_t *sequence,
                             const std::vector<std::string> &data) {
  assert(sequence);

  std::string index = (*sequence) ? std::to_string(*sequence) : "";

  for (auto &entry : data) {
    auto full_prefix = prefix + "_" + tag + index;
    add_help(full_prefix, entry);
    (*sequence)++;
    index = std::to_string(*sequence);
  }
}

Help_topic *Help_registry::add_help_topic(const std::string &name,
                                          Topic_type type,
                                          const std::string &tag,
                                          const std::string &parent_id,
                                          Mode_mask mode) {
  size_t topic_count = m_topics.size();
  m_topics[topic_count] = {name, name, type, tag, nullptr, {}, this, true};
  Help_topic *new_topic = &m_topics[topic_count];

  std::string splitter;

  if (!parent_id.empty()) {
    // Lookup the parent topic
    Help_topic *parent = nullptr;

    if (m_keywords.find(parent_id) != m_keywords.end()) {
      auto topics = &m_keywords[parent_id];

      for (const auto &topic : *topics) {
        // Only considers non members as the others can'
        if (!topic.first->is_member()) {
          if (parent == nullptr) {
            parent = topic.first;
          } else {
            std::string error = "Error registering topic '" + name +
                                "': parent topic is ambiguous: '" + parent_id +
                                "'";
            throw std::logic_error(error);
          }
        }
      }
    }

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

void Help_registry::add_help_class(const std::string &name,
                                   const std::string &parent,
                                   const std::string &upper_class) {
  Mode_mask mode(IShell_core::Mode::JavaScript);
  mode.set(IShell_core::Mode::Python);

  Help_topic *topic =
      add_help_topic(name, Topic_type::CLASS, name, parent, mode);

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
        type = "class";
        break;

      case Topic_type::OBJECT:
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
  if (!topic->is_member() &&
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
  // Patterns are associated to the mode where they are applicable
  // For that reason we need to create the following masks:
  // - all: for topics available on any shell mode, i.e. shell commands
  // - scripting: for topics that are valid both for Python and JavaScript
  //   i.e. modules and classes
  // - JavaScript and Python: for topics that are specific to either language,
  //   i.e. member names
  Mode_mask scripting(IShell_core::Mode::JavaScript);
  scripting.set(IShell_core::Mode::Python);
  Mode_mask javascript(IShell_core::Mode::JavaScript);
  Mode_mask python(IShell_core::Mode::Python);

  // The base keywords are the last component of the topic id, we need to
  // determine them as they will be combined with the rest of the nodes.
  std::vector<std::pair<std::string, Mode_mask>> base_keywords;

  if (topic->is_member()) {
    // Member names may differ at Python and JavaScript if that's the case
    // we add a base keyword for each valid on its own context
    auto names = shcore::str_split(topic->m_name, "|");
    if (names.size() == 1) names.push_back(shcore::from_camel_case(names[0]));

    if (names[0] != names[1]) {
      base_keywords.push_back({names[0], javascript});
      base_keywords.push_back({names[1], python});
    } else {
      base_keywords.push_back({topic->m_name, scripting});
    }
  } else {
    base_keywords.push_back({topic->m_name, mode});
  }

  // The ID is split in tokens
  std::string splitter =
      topic->is_api() ? HELP_API_SPLITTER : HELP_TOPIC_SPLITTER;
  std::string last_splitter =
      topic->is_command() ? HELP_CMD_SPLITTER : splitter;
  std::vector<std::string> tokens;

  size_t pos = topic->m_id.rfind(last_splitter);
  if (pos != std::string::npos) {
    tokens = str_split(topic->m_id.substr(0, pos), splitter);
  } else {
    tokens.push_back(topic->m_id);
  }
  std::reverse(tokens.begin(), tokens.end());

  // The name is at the base tokens, not needed here
  // tokens.erase(tokens.begin());

  // Registers the keywords
  for (auto &base_keyword : base_keywords) {
    std::string keyword = std::get<0>(base_keyword);
    Mode_mask context = std::get<1>(base_keyword);

    // Registers the base keyword
    register_keyword(keyword, context, topic);

    // Registers it's combination with rest of the tokens
    bool last = true;
    for (const auto &ptoken : tokens) {
      keyword = ptoken + (last ? last_splitter : splitter) + keyword;
      register_keyword(keyword, context, topic);
      last = true;
    }
  }

  // On Non API topics registers the help tag as a valid keyword if it is
  // different than the topic name
  if (!topic->is_api() &&
      !shcore::str_caseeq(topic->m_name, topic->m_help_tag)) {
    register_keyword(topic->m_help_tag, mode, topic);
  }
}

void Help_registry::register_keyword(const std::string &keyword,
                                     Mode_mask context, Help_topic *topic,
                                     bool case_sensitive) {
  if (!case_sensitive) {
    if (m_keywords.find(keyword) == m_keywords.end()) {
      m_keywords[keyword] = {};
    }

    auto &topics = m_keywords[keyword];

    if (topics.find(topic) == topics.end()) {
      topics[topic] = context;
    }
  } else {
    if (m_cs_keywords.find(keyword) == m_cs_keywords.end()) {
      m_cs_keywords[keyword] = {};
    }

    auto &topics = m_cs_keywords[keyword];

    if (topics.find(topic) == topics.end()) {
      topics[topic] = context;
    }
  }
}

std::string Help_registry::get_token(const std::string &token) {
  std::string ret_val;

  if (m_help_data.find(token) != m_help_data.end())
    ret_val = m_help_data[token];

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

  return ret_val;
}

std::vector<Help_topic *> Help_registry::search_topics(
    const std::string &pattern, IShell_core::Mode mode) {
  return search_topics(pattern, IShell_core::Mode_mask(mode), false);
}

Help_topic *Help_registry::get_topic(const std::string &id,
                                     bool allow_unexisting) {
  if (m_keywords.find(id) == m_keywords.end()) {
    if (!allow_unexisting)
      throw std::logic_error("Unable to find topic '" + id + "'");
  } else {
    auto &topics = m_keywords[id];

    if (topics.size() > 1)
      throw std::logic_error("Non unique topic found as '" + id + "'");

    return topics.begin()->first;
  }

  return nullptr;
}

Help_topic *Help_registry::get_class_parent(Help_topic *topic) {
  if (m_class_parents.find(topic) != m_class_parents.end())
    return m_class_parents.at(topic);

  return nullptr;
}

bool Help_registry::is_enabled(const Help_topic *topic,
                               IShell_core::Mode mode) const {
  bool ret_val = false;
  try {
    if (topic->is_enabled()) {
      auto topics = m_keywords.at(topic->get_id(true, mode));
      auto mask = topics.at(const_cast<Help_topic *>(topic));
      ret_val = mask.is_set(mode);
    }
  } catch (...) {
    log_error("Help_registry::is_enabled: using an unregistered topic '%s'.",
              topic->m_id.c_str());
  }

  return ret_val;
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
  return m_registry->search_topics(pattern, m_mode);
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
      // signature Removed the optionsl word from the description
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

void Help_manager::add_simple_function_help(
    const Help_topic &function, std::vector<std::string> *sections) {
  std::string name = function.get_base_name();
  Help_topic *parent = function.m_parent;

  // Starts the syntax
  std::string display_name = function.get_name(m_mode);
  std::string syntax =
      textui::bold(HELP_TITLE_SYNTAX) + HEADER_CONTENT_SEPARATOR;
  std::string fsyntax;

  if (parent->is_class())
    fsyntax += "<" + parent->m_name + ">." + textui::bold(display_name);
  else
    fsyntax += function.m_parent->m_name + HELP_API_SPLITTER +
               textui::bold(display_name);

  // Gets the function signature
  std::string signature = get_signature(function);

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

  if (signature != "()") {
    std::vector<std::pair<std::string, std::string>> pdata;
    auto params = resolve_help_text(*parent, name + "_PARAM");

    pdata = parse_function_parameters(params);

    if (!pdata.empty()) {
      // Describes the parameters
      std::string where = textui::bold("WHERE") + HEADER_CONTENT_SEPARATOR;
      std::vector<std::string> where_items;

      size_t index;
      for (index = 0; index < params.size(); index++) {
        // Padding includes two spaces at the beggining, colon and space at the
        // end like:"  name: "<description>.
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
    returns[0] = returns[0].substr(8);
    ret += format_help_text(&returns, MAX_HELP_WIDTH, SECTION_PADDING, true);
    sections->push_back(ret);
  }

  // Description
  add_member_section(HELP_TITLE_DESCRIPTION, name + "_DETAIL", *parent,
                     sections, SECTION_PADDING);

  // Exceptions
  add_member_section("EXCEPTIONS", name + "_THROWS", *parent, sections,
                     SECTION_PADDING);
}

std::string Help_manager::get_signature(const Help_topic &function) {
  std::string signature;
  Help_topic *parent = function.m_parent;
  std::string name = function.get_base_name();
  std::vector<std::string> signatures;

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
  std::string name = main_function.get_base_name();
  Help_topic *parent = main_function.m_parent;

  // Gets the chain definition already formatted for the active language
  auto chain_definition =
      get_help_text(parent->m_name + "_" + name + "_CHAINED");
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
      auto signatures = get_help_text(target_class + "_" + name + "_SIGNATURE");
      section.push_back("This function has the following overloads:");
      for (auto &real_signature : signatures) {
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
    const std::vector<Help_topic *> &topics, bool members, bool alias) {
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

    auto help_text = get_help_text(topic->m_help_tag + "_BRIEF");

    // Deprecation notices are added as part of the description on list format
    auto deprecated = get_help_text(topic->m_help_tag + "_DEPRECATED");
    for (const auto &text : deprecated) {
      help_text.push_back(text);
    }

    formatted.push_back(format_list_description(topic->m_name, &help_text,
                                                name_max_len, lpadding,
                                                alias_str, alias_max_len));
  }

  return shcore::str_join(formatted, "\n");
}

std::string Help_manager::format_member_list(
    const std::vector<Help_topic *> &topics, size_t lpadding) {
  std::vector<std::string> descriptions;
  for (auto member : topics) {
    std::string description(SECTION_PADDING, ' ');
    description += member->get_name(m_mode);

    // If it is a function we need to retrieve the signature
    if (member->is_function()) {
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

    std::string tag = member->m_help_tag + "_BRIEF";
    auto help_text = resolve_help_text(*member->m_parent, tag);

    // Deprecation notices are added as part of the description on list format
    tag = member->m_help_tag + "_DEPRECATED";
    auto deprecated = resolve_help_text(*member->m_parent, tag);
    for (const auto &text : deprecated) {
      help_text.push_back(text);
    }

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
                                    std::vector<std::string> *sections) {
  std::string dname = topic.get_name(m_mode);
  std::string formatted;
  std::vector<std::string> brief;

  if (topic.is_member())
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
      syntax += object.m_parent->m_name + HELP_API_SPLITTER +
                textui::bold(object.m_name);

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
  std::vector<Help_topic *> cat, prop, func, mod, cls, obj, others;
  for (auto child : object.m_childs) {
    if (child->is_enabled(m_mode)) {
      switch (child->m_type) {
        case Topic_type::MODULE:
          mod.push_back(child);
          break;
        case Topic_type::CLASS:
          cls.push_back(child);
          break;
        case Topic_type::OBJECT:
          obj.push_back(child);
          break;
        case Topic_type::FUNCTION:
          func.push_back(child);
          break;
        case Topic_type::PROPERTY:
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
  std::vector<std::string> details;
  details = resolve_help_text(parent, tag);
  add_section_data(title, &details, sections, padding);
}

void Help_manager::add_section(const std::string &title, const std::string &tag,
                               std::vector<std::string> *sections,
                               size_t padding, bool insert_blank_lines) {
  std::vector<std::string> details;
  details = get_help_text(tag);
  add_section_data(title, &details, sections, padding);
}

std::string Help_manager::format_function_help(const Help_topic &function) {
  std::string name = function.get_base_name();
  std::vector<std::string> sections;

  // Brief Description
  add_name_section(function, &sections);

  auto chain_definition =
      resolve_help_text(*function.m_parent, name + "_CHAINED");

  std::string additional_help;
  if (chain_definition.empty()) {
    add_simple_function_help(function, &sections);
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

  if (property.m_parent->is_class())
    syntax +=
        "<" + property.m_parent->m_name + ">." + textui::bold(display_name);
  else
    syntax += property.m_parent->m_name + HELP_API_SPLITTER +
              textui::bold(display_name);

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
                                        size_t padding) {
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
      section = mysqlshdk::textui::bold("EXAMPLE");
    else
      section = mysqlshdk::textui::bold("EXAMPLES");
    section += HEADER_CONTENT_SEPARATOR;
    section += shcore::str_join(examples, "\n\n");
    sections->push_back(section);
  }
}

std::map<std::string, std::string> Help_manager::preprocess_help(
    std::vector<std::string> *text_lines) {
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
                                           bool paragraph_per_line) {
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
                                   const Help_options &options) {
  switch (topic.m_type) {
    case Topic_type::CATEGORY:
    case Topic_type::TOPIC:
    case Topic_type::MODULE:
    case Topic_type::CLASS:
    case Topic_type::OBJECT:
      return format_object_help(topic, options);
    case Topic_type::FUNCTION:
      return format_function_help(topic);
    case Topic_type::PROPERTY:
      return format_property_help(topic);
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
