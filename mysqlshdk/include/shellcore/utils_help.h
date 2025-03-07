/*
 * Copyright (c) 2016, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_UTILS_HELP_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_UTILS_HELP_H_

#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/include/shellcore/ishell_core.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_cli_mapper.h"
#include "scripting/common.h"

namespace shcore {

enum class Topic_type {
  CATEGORY = 1,
  TOPIC,
  SQL,
  COMMAND,
  MODULE,
  CLASS,
  OBJECT,
  GLOBAL_OBJECT,
  CONSTANTS,
  FUNCTION,
  PROPERTY,
};

enum class Topic_id_mode {
  FULL = 1,           // Generate IDs considering all parent topics
  EXCLUDE_ROOT,       // Generate IDs removing the ROOT category
  EXCLUDE_CATEGORIES  // Generate IDs removing categories
};

enum class Keyword_location {
  GLOBAL_CTX = 1,  // Keyword should be created on the global context,
  LOCAL_CTX,       // Keyword should be created on the thread local context,
  BOTH_CTX  // Keyword should be created on both, local and global context
};

typedef mysqlshdk::utils::Enum_set<Topic_type, Topic_type::PROPERTY> Topic_mask;

class Help_registry;
struct Help_topic;

struct Help_topic_compare {
  bool operator()(const Help_topic *const &lhs,
                  const Help_topic *const &rhs) const;
};

struct Help_topic_id_compare {
  bool operator()(const Help_topic *const &lhs,
                  const Help_topic *const &rhs) const;
};

struct icomp {
  bool operator()(const std::string &lhs, const std::string &rhs) const;
};

typedef std::set<Help_topic *, Help_topic_compare> Help_topic_refs;

struct Help_topic {
  std::string m_id;
  std::string m_name;
  Topic_type m_type;
  // Tag that dentifies the entry point on the help data
  std::string m_help_tag;
  Help_topic *m_parent;
  Help_topic_refs m_childs;
  bool m_enabled;
  std::map<std::string, std::string> m_keywords;

  const Help_topic *get_category() const;

  bool is_module() const { return m_type == Topic_type::MODULE; }
  bool is_class() const { return m_type == Topic_type::CLASS; }
  bool is_object() const {
    return m_type == Topic_type::OBJECT ||
           m_type == Topic_type::GLOBAL_OBJECT ||
           m_type == Topic_type::CONSTANTS;
  }
  bool is_function() const { return m_type == Topic_type::FUNCTION; }
  bool is_property() const { return m_type == Topic_type::PROPERTY; }
  bool is_command() const { return m_type == Topic_type::COMMAND; }
  bool is_category() const { return m_type == Topic_type::CATEGORY; }
  bool is_topic() const { return m_type == Topic_type::TOPIC; }
  bool is_sql() const { return m_type == Topic_type::SQL; }

  bool is_api() const;
  bool is_api_object() const;
  bool is_member() const;

  void set_enabled(bool enabled) { m_enabled = enabled; }
  bool is_enabled() const { return m_enabled; }
  bool is_enabled(IShell_core::Mode mode) const;
  std::string get_name(IShell_core::Mode mode) const;
  std::string get_base_name() const;
  const std::string &get_id() const noexcept { return m_id; }
  std::string get_id(IShell_core::Mode mode,
                     Topic_id_mode id_mode = Topic_id_mode::FULL) const;

  std::string get_keyword(const std::string &keyword) const;

  /**
   * Fills the tokens array with the name of every single topic name
   * starting from the topmost relevant topic.
   *
   * The name is filled following naming convention (if applicable).
   *
   * The topmost relevant topic is defined by the full flag:
   * - If true, the topmost relevant topic is the root topic (Contents).
   * - If false the topmost relevant topic is:
   *   - For API topics: the first API topic in the hierarchy.
   *     (i.e. Contents/<API Root Topic> get removed).
   *   - For Non API topics: the topic that follows the root topic.
   *     (i.e. Contents gets removed).
   */
  void get_id_tokens(IShell_core::Mode mode, std::vector<std::string> *tokens,
                     Topic_id_mode id_mode = Topic_id_mode::FULL) const;
};

/**
 * Singleton holding the help data which consist of two components
 * - The help information: which is the actual text associated to each
 *   registered topic.
 * - The help tree: which contains all the registered topics and the relations
 *   between them and their associations with the help data.
 */
class Help_registry {
  // The Keyword Registry holds the relations between a keyword, a topic as well
  // as when it is valid. The following requirements are satisfied by this:
  // - Keywords are case insensitive.
  // - Multiple topics may me assigned to the same keyword
  // - The Keyword/Topic relation can be enabled for 1 or more modes
  typedef std::map<
      std::string,
      std::map<Help_topic *, IShell_core::Mode_mask, Help_topic_id_compare>,
      shcore::Case_insensitive_comparator>
      Keyword_registry;

  typedef std::map<std::string, std::map<Help_topic *, IShell_core::Mode_mask,
                                         Help_topic_id_compare>>
      Keyword_case_sensitive_registry;

  typedef std::map<std::string, std::string,
                   shcore::Case_insensitive_comparator>
      Data_registry;

 public:
  static const char HELP_ROOT[];
  static const char HELP_COMMANDS[];
  static const char HELP_SQL[];

  struct Example {
    std::string code;
    std::string description;
  };

  virtual ~Help_registry();

  // Access to the singleton
  static Help_registry *get();

  // Retrieves the help text associated to a specific token
  std::string get_token(const std::string &help) const;

  /**
   * Helper function to register multiple help entries for a topic at once.
   * @param prefix The token prefix under which the text entry will be
   * associated
   * @param data The help data
   * @param auto_brief If true, treats the 1st paragraph as the brief text.
   * @param nosuffix If true, don't append the section specific suffix to the
   * prefix when generating tag names.
   * @param is_shell_command If true, parsing will also support defining the
   * syntax tag wth @syntax and examples with @example
   */
  void add_split_help(const std::string &prefix, const std::string &data,
                      bool auto_brief, bool nosuffix,
                      bool is_shell_command = false);

  /**
   * Helper function to register a single help entry.
   * @param token The token under which the text entry will be associated
   * @param data The help data
   */
  void add_help(const std::string &token, std::string data,
                Keyword_location loc = Keyword_location::GLOBAL_CTX);

  /**
   * Helper function to register a sigle help entry using a prefix and a tag.
   *
   * @param prefix The prefix of the token to be used to register the data
   * @param tag The suffix of the token to be used to register the data.
   * @param data The help data
   *
   * A token will be created as <prefix>_<tag> and the data will be registered
   * using this token.
   */
  void add_help(std::string_view prefix, std::string_view tag, std::string data,
                Keyword_location loc = Keyword_location::GLOBAL_CTX);

  /**
   * Helper function to register a series of data elements under the same tag.
   * This function allows registering data for the same tag in separate
   * groups.
   *
   * @param prefix The prefix of the token to be used to register the data
   * @param tag The suffix of the token to be used to register the data.
   * @param sequence An in/out parameter holding the number of entries
   *                 registered with the same tag.
   * @param data An array with the help entries to be registered using the
   *             same tag.
   *
   * For each entry in data, a token will be created as
   * <prefix>_<tag><sequence> and the data will be registered using this
   * token.
   *
   * The sequence will be increased after each entry is registered.
   */
  void add_help(const std::string &prefix, const std::string &tag,
                size_t *sequence, const std::vector<std::string> &data,
                Keyword_location loc = Keyword_location::GLOBAL_CTX);

  /**
   * Helper function to register a series of data elements under the same tag.
   * This function allows registering data for the same tag in a single group.
   *
   * @param prefix The prefix of the token to be used to register the data
   * @param tag The suffix of the token to be used to register the data.
   * @param data An array with the help entries to be registered using the
   *             same tag.
   * @param bool Indicate if the help should be registered per thread context.
   *
   * This function initializes a sequence in 0 and calls the function above.
   */
  void add_help(const std::string &prefix, const std::string &tag,
                const std::vector<std::string> &data,
                Keyword_location loc = Keyword_location::GLOBAL_CTX);

  void add_help(const std::string &prefix, const std::vector<Example> &data,
                Keyword_location loc = Keyword_location::GLOBAL_CTX);

  void add_help(const std::string &prefix, size_t *sequence,
                const std::vector<Example> &data,
                Keyword_location loc = Keyword_location::GLOBAL_CTX);

  // Registers a new topic and it's associated keywords
  std::vector<Help_topic *> add_help_topic(
      const std::string &name, Topic_type type, const std::string &tag,
      const std::string &parent, IShell_core::Mode_mask mode,
      Keyword_location loc = Keyword_location::GLOBAL_CTX,
      bool exact_id_match = false);

  void remove_topic(Help_topic *topic, Keyword_location loc);

  // Registers class inheritance
  void add_help_class(
      const std::string &name, const std::string &parent,
      const std::string &upper_class = "",
      IShell_core::Mode_mask mode = IShell_core::all_scripting_modes(),
      std::map<std::string, std::string> keywords = {});
  void inherit_members(Help_topic *parent, Help_topic *child);
  void inherit_member(Help_topic *parent, Help_topic *member);

  // Searches the help topics associated to a keyword matching the specified
  // pattern if they are enabled for the indicated mode
  std::vector<const Help_topic *> search_topics(const std::string &pattern,
                                                IShell_core::Mode mode);
  std::vector<const Help_topic *> search_topics(const std::string &pattern,
                                                IShell_core::Mode_mask mode,
                                                bool case_sensitive);

  std::vector<std::string> search_topic_names(const std::string &pattern,
                                              IShell_core::Mode mode);

  Help_topic *get_topic(const std::string &id, bool allow_unexisting = false,
                        const Topic_mask &type = Topic_mask::any(),
                        bool exact_id_match = false) const;

  Help_topic *get_class_parent(Help_topic *topic) const;

  bool is_enabled(const Help_topic *topic, IShell_core::Mode mode) const;

  void register_keyword(const std::string &keyword, IShell_core::Mode mode,
                        Help_topic *topic, bool case_sensitive = false);
  void register_keyword(const std::string &keyword, IShell_core::Mode_mask mode,
                        Help_topic *topic, bool case_sensitive = false);

  const std::vector<Help_topic *> get_help_topics(Topic_type type) const {
    auto topics = m_topics_by_type.at(type);
    try {
      auto ret = get_thread_context_help()->m_topics_by_type.at(type);
      topics.insert(topics.end(), ret.begin(), ret.end());
    } catch (const std::out_of_range &) {
      // We skip this out_of_range as thread_context_help may not have
      // registered this type of help, we don't care about this.
    }

    return topics;
  }

  Help_registry *get_thread_context_help() const;

  std::unique_lock<std::recursive_mutex> ensure_lock();

  std::unique_lock<std::recursive_mutex> ensure_lock_threaded();

 private:
  // Options will be stored on a MAP
  Data_registry m_help_data;

  // Holds all the registered topics
  std::deque<Help_topic> m_topics;

  std::map<Topic_type, std::vector<Help_topic *>> m_topics_by_type;

  // List of orphan topics
  std::map<std::string, std::vector<Help_topic *>> m_orphans;

  std::map<std::string, std::vector<Help_topic *>> m_class_childs;

  std::map<Help_topic *, Help_topic *> m_class_parents;

  // Keyword registry: 1 - *
  Keyword_registry m_keywords;

  Keyword_case_sensitive_registry m_cs_keywords;

  std::recursive_mutex m_mutex;

  std::recursive_mutex m_th_help;

  bool m_threaded;

  std::vector<Help_registry *> m_threaded_help;

  /**
   * This is set on each threaded help instance so each one knows it shouldn't
   * try to deregister from the global_ctx if it's already gone.
   */
  bool m_global_removed;

  /**
   * This is used in the global_ctx so the first time it's calling add_topic it
   * will also create the local_ctx  while trying to find existing topic. This
   * can result in order creation fiasco which this var is protecting.
   */
  bool m_c_tor;

  Topic_mask get_parent_topic_types();

  // Private constructor since this is a singleton
  explicit Help_registry(bool threaded = false);

  // Helper functions for add_help_topic
  void register_topic(Help_topic *topic, bool new_topic,
                      IShell_core::Mode_mask mode);
  void register_keywords(Help_topic *topic, IShell_core::Mode_mask mode);

  Help_topic *get_topic(const Help_registry *registry, const std::string &id,
                        bool allow_unexisting = false,
                        const Topic_mask &type = Topic_mask::any(),
                        bool exact_id_match = false) const;

  // Registers a new topic and it's associated keywords
  Help_topic *create_help_topic(const std::string &name, Topic_type type,
                                const std::string &tag,
                                const std::string &parent_id,
                                Help_topic *parent, IShell_core::Mode_mask mode,
                                bool do_register = true);
  void remove_topic(Help_topic *topic);

  bool is_enabled(const Keyword_registry &registry, const Help_topic *topic,
                  IShell_core::Mode mode) const;
};

/**
 * Helper structure to statically register help data.
 */
struct Help_register {
  Help_register(const std::string &token, std::string data) {
    shcore::Help_registry::get()->add_help(token, std::move(data));
  }
};

/**
 * Helper structure to statically register help data after splitting into
 * the tradditional broken down format from Help_register.
 *
 * Note: Might be possible to skip the splitting/re-joining and register
 * the full text directly.
 */
struct Help_register_split {
  Help_register_split(const std::string &prefix, const std::string &data,
                      bool auto_brief, bool nosuffix,
                      bool is_shell_command = false) {
    shcore::Help_registry::get()->add_split_help(prefix, data, auto_brief,
                                                 nosuffix, is_shell_command);
  }
};

struct Help_register_topic_text {
  Help_register_topic_text(const std::string &prefix, const std::string &data,
                           bool auto_brief) {
    // Adds _DETAIL# entries for the whole thing
    shcore::Help_registry::get()->add_split_help(prefix, data, auto_brief,
                                                 false);
    // Add top-level reference to the 1st _DETAIL entry
    shcore::Help_registry::get()->add_help(prefix, "${" + prefix + "_DETAIL}");
  }
};

enum class Help_mode { ALL, SCRIPTING, JAVASCRIPT, PYTHON, SQL };
/**
 * Helper structure to statically register help topics
 */
struct Help_topic_register {
  Help_topic_register(const std::string &name, Topic_type type,
                      const std::string &tag, const std::string &parent,
                      Help_mode mode) {
    IShell_core::Mode_mask mask;
    using Mode = IShell_core::Mode;

    switch (mode) {
      case Help_mode::ALL:
        mask = IShell_core::Mode_mask::all();
        break;
      case Help_mode::SCRIPTING:
        mask = IShell_core::Mode_mask(Mode::JavaScript).set(Mode::Python);
        break;
      case Help_mode::JAVASCRIPT:
        mask = IShell_core::Mode_mask(Mode::JavaScript);
        break;
      case Help_mode::PYTHON:
        mask = IShell_core::Mode_mask(Mode::Python);
        break;
      case Help_mode::SQL:
        mask = IShell_core::Mode_mask(Mode::SQL);
        break;
    }
    Help_registry::get()->add_help_topic(name, type, tag, parent, mask);
  }
};

/**
 * Helper structure to statically register help classes
 */
struct Help_class_register {
  Help_class_register(const std::string &child, const std::string &parent,
                      const std::string &upper_class,
                      Help_mode mode = Help_mode::SCRIPTING,
                      const std::map<std::string, std::string> &keywords = {});
};
enum class Help_option {
  Name,
  Syntax,
  Brief,
  Detail,
  Categories,
  Modules,
  Objects,
  Classes,
  Functions,
  Properties,
  Constants,
  Childs,
  Closing,
  Example,
};

typedef mysqlshdk::utils::Enum_set<Help_option, Help_option::Example>
    Help_options;

/**
 * Enables querying the Help_registry to retrieve formatted help data
 *
 * The help lookup/formatting is performed using the active mode.
 */
class Help_manager {
 public:
  Help_manager();

  static constexpr size_t MAX_HELP_WIDTH = 80;

  // Define the mode for which the help is being retrieved
  void set_mode(IShell_core::Mode mode) { m_mode = mode; }
  IShell_core::Mode get_mode() { return m_mode; }

  // Searches for topics matching the specified pattern
  std::vector<const Help_topic *> search_topics(const std::string &pattern);

  /**
   * Retrieves the help text for a topic uniquely identified by the given
   * topic id.
   * Throws logic error if:
   * - No topics are found with the given topic id.
   * - Multiple topics are found with the given topic id.
   */
  std::string get_help(const std::string &topic_id,
                       Topic_mask type = Topic_mask(),
                       const std::string &options = "");
  std::string get_help(const std::string &topic_id, Topic_mask type,
                       const Help_options &options);

  /**
   * Retrieves the help text associated to the given topic.
   */
  std::string get_help(const Help_topic &topic,
                       const Help_options &options = Help_options::any(),
                       cli::Shell_cli_mapper *cli = nullptr);

  void add_childs_section(const Help_topic &parent,
                          const std::vector<const Help_topic *> &childs,
                          std::vector<std::string> *sections, size_t padding,
                          bool members, const std::string &tag,
                          const std::string &default_title, bool alias = false);

  void add_section(const Help_topic &topic, const std::string &title,
                   const std::string &tag, std::vector<std::string> *sections,
                   size_t padding, bool insert_blank_lines = true);

  void add_examples_section(const Help_topic &topic, const std::string &tag,
                            std::vector<std::string> *sections, size_t padding,
                            const std::string &single_title = "EXAMPLE",
                            const std::string &multi_title = "EXAMPLES");

  /**
   * Takes a list of member topics and produces a formatted list as follows:
   *
   *      MemberName1
   *            Member1 brief description
   *
   *      MemberName2
   *            Member2 brief description
   *      ...
   *      MemberNameN
   *            MemberN brief description
   *
   * Naming honors the active language naming convention
   */
  std::string format_member_list(
      const std::vector<const Help_topic *> &topics, size_t lpadding = 0,
      bool do_signatures = true,
      std::function<std::string(const Help_topic *)> format_name_cb = nullptr);

 private:
  // Holds the active mode to be used for the help handling
  IShell_core::Mode m_mode = IShell_core::Mode::None;
  std::map<std::string, Help_option, icomp> m_option_vals;
  Help_registry *m_registry;

  std::string format_object_help(const Help_topic &object,
                                 const Help_options &options);
  std::string format_function_help(const Help_topic &function,
                                   cli::Shell_cli_mapper *cli = nullptr);
  std::string format_property_help(const Help_topic &property);
  std::string format_command_help(const Help_topic &property,
                                  const Help_options &options);

  /**
   * Parses and updates the text_lines to update function names so they
   * matches the required naming convention.
   *
   * These function names must named in camelCase format and enclosed between
   * <<< and >>>
   *
   */
  std::map<std::string, std::string> preprocess_help(
      const Help_topic &topic, std::vector<std::string> *text_lines) const;
  std::string format_help_text(const Help_topic &topic,
                               std::vector<std::string> *lines, size_t width,
                               size_t left_padding,
                               bool paragraph_per_line) const;

  /**
   * Gets all the help data associated to the given token
   */
  std::vector<std::string> get_help_text(const std::string &token);

  /**
   * Searches in the class hierarchy to get the help data associated to the
   * given suffix using a bottom up approach.
   *
   * This enables documentation inheritance.
   */
  std::vector<std::string> resolve_help_text(const Help_topic &object,
                                             const std::string &suffix,
                                             bool legacy = false);
  /**
   * Parses a parameter definition list and returns the corresponding
   * signature as well as a vector of parameters and descriptions.
   */
  std::vector<std::pair<std::string, std::string>> parse_function_parameters(
      const std::vector<std::string> &parameters,
      std::string *signature = nullptr);

  void add_name_section(const Help_topic &topic,
                        std::vector<std::string> *sections,
                        cli::Shell_cli_mapper *cli = nullptr);

  /**
   * The following functions format the help data using a specific format for
   * each topic type
   */
  void add_section_data(const Help_topic &topic, const std::string &title,
                        std::vector<std::string> *details,
                        std::vector<std::string> *sections, size_t padding,
                        bool insert_blank_lines = true);

  void add_member_section(const std::string &title, const std::string &tag,
                          const Help_topic &member,
                          std::vector<std::string> *sections, size_t padding);

  void add_cli_options_section(
      const std::string &tag, const Help_topic &parent,
      const std::vector<std::pair<std::string, std::string>> &pdata,
      std::vector<std::string> *sections, size_t padding,
      cli::Shell_cli_mapper *cli);

  /**
   * Inserts a new section with the content of a function definition
   */
  void add_simple_function_help(const Help_topic &function,
                                std::vector<std::string> *sections,
                                cli::Shell_cli_mapper *cli = nullptr);

  /**
   * Formats help for a function that supports chaining
   */
  void add_chained_function_help(const Help_topic &function,
                                 std::vector<std::string> *sections);

  std::string get_signature(const Help_topic &function,
                            cli::Shell_cli_mapper *cli = nullptr);

  /**
   * Given a list of topic references, retrieves the max length either for
   * - The topic names
   * - Member names (honoring naming convention)
   * - Topic aliases (command alias)
   */
  size_t get_max_topic_length(const std::vector<const Help_topic *> &topics,
                              bool members, bool alias = false) const;

  /**
   * Takes a list of topics and produces a formatted list as follows:
   *
   * - TopicName1 Topic1 brief description
   * - TopicName2 Topic2 brief description
   *   ...
   * - TopicNameN TopicN brief description
   *
   * The alias parameter is used to also consider the topic aliases
   * i.e. in shell command topics as:
   *
   * - CommandName1 (\a)     Command1 brief description
   * - CommandName2 (\b, \c) Command1 brief description
   *   ...
   * - CommandNameN (\z)     CommandZ brief description
   */
  std::string format_topic_list(const std::vector<const Help_topic *> &topics,
                                size_t lpadding = 0, bool alias = false);
  std::string format_list_description(const Help_topic &topic,
                                      const std::string &name,
                                      std::vector<std::string> *help_text,
                                      size_t name_max_len, size_t lpadding = 0,
                                      const std::string &alias = "",
                                      size_t alas_max_len = 0);

  std::vector<std::string> get_member_brief(const Help_topic *member);
  std::vector<std::string> get_topic_brief(const Help_topic *member);
};

}  // namespace shcore

#define REGISTER_HELP(x, y) shcore::Help_register x(#x, y)

#define REGISTER_HELP_CLASS_MODE(name, parent, mode) \
  shcore::Help_class_register class_##parent##name(#name, #parent, "", mode)

#define REGISTER_HELP_CLASS_MODE_KW(name, parent, mode, keywords)         \
  shcore::Help_class_register class_kw_##parent##name(#name, #parent, "", \
                                                      mode, keywords)

#define REGISTER_HELP_CLASS(name, parent) \
  REGISTER_HELP_CLASS_MODE(name, parent, shcore::Help_mode::SCRIPTING)

#define REGISTER_HELP_CLASS_KW(name, parent, keywords)                    \
  REGISTER_HELP_CLASS_MODE_KW(name, parent, shcore::Help_mode::SCRIPTING, \
                              keywords)

#define REGISTER_HELP_SUB_CLASS(name, parent, upper) \
  shcore::Help_class_register class_##parent##name(#name, #parent, #upper)

#define REGISTER_HELP_TOPIC(name, type, tag, parent, mode) \
  shcore::Help_topic_register topic_##type##parent##tag(   \
      #name, shcore::Topic_type::type, #tag, #parent, shcore::Help_mode::mode)

#define REGISTER_HELP_TOPIC_NOTAG(name, type, parent, mode) \
  REGISTER_HELP_TOPIC(name, type, name, parent, mode)

#define REGISTER_HELP_OBJECT_MODE(name, parent, mode) \
  REGISTER_HELP_TOPIC_NOTAG(name, OBJECT, parent, mode)

#define REGISTER_HELP_OBJECT(name, parent) \
  REGISTER_HELP_OBJECT_MODE(name, parent, SCRIPTING)

#define REGISTER_HELP_CONSTANTS(name, parent) \
  REGISTER_HELP_TOPIC_NOTAG(name, CONSTANTS, parent, SCRIPTING)

#define REGISTER_HELP_GLOBAL_OBJECT_MODE(name, parent, mode) \
  REGISTER_HELP_TOPIC_NOTAG(name, GLOBAL_OBJECT, parent, mode)

#define REGISTER_HELP_GLOBAL_OBJECT(name, parent) \
  REGISTER_HELP_GLOBAL_OBJECT_MODE(name, parent, SCRIPTING)

#define REGISTER_HELP_MODULE(name, parent) \
  REGISTER_HELP_TOPIC_NOTAG(name, MODULE, parent, SCRIPTING)

#define REGISTER_HELP_FUNCTION_MODE(name, parent, mode) \
  REGISTER_HELP_TOPIC_NOTAG(name, FUNCTION, parent, mode)

#define REGISTER_HELP_FUNCTION(name, parent) \
  REGISTER_HELP_FUNCTION_MODE(name, parent, SCRIPTING)

#define REGISTER_HELP_PROPERTY_MODE(name, parent, mode) \
  REGISTER_HELP_TOPIC_NOTAG(name, PROPERTY, parent, mode)

#define REGISTER_HELP_PROPERTY(name, parent) \
  REGISTER_HELP_PROPERTY_MODE(name, parent, SCRIPTING)

#define REGISTER_HELP_CLASS_TEXT(x, y) \
  shcore::Help_register_topic_text x(#x, y, true)

#define REGISTER_HELP_TOPIC_TEXT(x, y) \
  shcore::Help_register_topic_text x(#x, y, false)

#define REGISTER_HELP_TOPIC_WITH_BRIEF_TEXT(x, y) \
  shcore::Help_register_topic_text x(#x, y, true)

#define REGISTER_HELP_FUNCTION_TEXT(x, y) \
  shcore::Help_register_split x##function(#x, y, true, false)

#define REGISTER_HELP_PROPERTY_TEXT(x, y) \
  shcore::Help_register_split x##property(#x, y, true, false)

#define REGISTER_HELP_COMMAND_TEXT(x, y) \
  shcore::Help_register_split x##command(#x, y, true, false, true)

#define REGISTER_HELP_DETAIL_TEXT(x, y) \
  shcore::Help_register_split x##detail(#x, y, false, true)

#define REGISTER_HELP_SHARED_TEXT(name, text) \
  inline constexpr const char *name = text

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_UTILS_HELP_H_
