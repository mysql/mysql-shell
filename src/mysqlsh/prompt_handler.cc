/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "src/mysqlsh/prompt_handler.h"

#include <vector>

#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_json.h"

extern char *mysh_get_tty_password(const char *opt_message);

namespace mysqlsh {
namespace {
void print_prompt_description(const std::vector<std::string> &description) {
  // Prints each paragraph on a separate line if the description is provided
  auto console = mysqlsh::current_console();

  for (const auto &paragraph : description) {
    console->println(paragraph);
    console->println();
  }
}

inline bool use_json() {
  return mysqlsh::current_shell_options()->get().wrap_json != "off";
}

std::string json_prompt(const std::string &prompt,
                        const shcore::prompt::Prompt_options &options) {
  shcore::JSON_dumper dumper(
      mysqlsh::current_shell_options()->get().wrap_json == "json");

  dumper.start_object();
  dumper.append_string("prompt");
  dumper.append_string(prompt);

  dumper.append_string(shcore::prompt::k_type);
  dumper.append_string(shcore::prompt::to_string(options.type));

  if (!options.title.empty()) {
    dumper.append_string(shcore::prompt::k_title);
    dumper.append_string(options.title);
  }

  if (!options.description.empty()) {
    dumper.append_string(shcore::prompt::k_description);
    dumper.start_array();
    for (const auto &paragraph : options.description) {
      dumper.append_string(paragraph);
    }
    dumper.end_array();
  }

  if (options.default_value) {
    dumper.append_string(shcore::prompt::k_default_value);
    dumper.append_value(options.default_value);
  }

  if (options.type == shcore::prompt::Prompt_type::CONFIRM) {
    dumper.append_string(shcore::prompt::k_yes_label);
    dumper.append_string(options.yes_label);
    dumper.append_string(shcore::prompt::k_no_label);
    dumper.append_string(options.no_label);
    if (!options.alt_label.empty()) {
      dumper.append_string(shcore::prompt::k_alt_label);
      dumper.append_string(options.alt_label);
    }
  } else if (options.type == shcore::prompt::Prompt_type::SELECT) {
    dumper.append_string(shcore::prompt::k_options);
    dumper.start_array();
    for (const auto &item : options.select_items) {
      dumper.append_string(item);
    }
    dumper.end_array();
  }

  dumper.end_object();

  return dumper.str() + "\n";
}

}  // namespace

std::string render_text_prompt(const std::string &prompt,
                               const shcore::prompt::Prompt_options &options) {
  std::string prompt_text;

  if (use_json()) {
    prompt_text = json_prompt(prompt, options);
  } else {
    print_prompt_description(options.description);
    std::string more;
    if (!shcore::str_iendswith(prompt, ": ") &&
        !shcore::str_iendswith(prompt, ":")) {
      more = ": ";
    }

    prompt_text = mysqlshdk::textui::bold(prompt + more);
  }

  return prompt_text;
}

std::string render_confirm_prompt(const std::string &prompt,
                                  const shcore::prompt::Prompt_options &options,
                                  std::string *out_valid_answers) {
  std::string prompt_text;

  std::string clean_yes_text, clean_no_text, clean_alt_text;
  std::string yes_display_text, no_display_text, alt_display_text;
  std::string def;

  // Label pre-processing
  shcore::prompt::process_label(options.yes_label, &yes_display_text,
                                &clean_yes_text);
  shcore::prompt::process_label(options.no_label, &no_display_text,
                                &clean_no_text);
  if (!options.alt_label.empty()) {
    shcore::prompt::process_label(options.alt_label, &alt_display_text,
                                  &clean_alt_text);
  }

  if (options.default_value) {
    def = options.default_value.as_string();
  }

  std::string def_str;

  if (options.yes_label == "&Yes" && options.no_label == "&No" &&
      options.alt_label.empty()) {
    if (def == options.yes_label)
      def_str = "[Y/n]: ";
    else if (def == options.no_label)
      def_str = "[y/N]: ";
    else
      def_str = "[y/n]: ";
  } else {
    if (!yes_display_text.empty()) {
      def_str.append(yes_display_text).append("/");
    }
    if (!no_display_text.empty()) {
      def_str.append(no_display_text).append("/");
    }
    if (!alt_display_text.empty()) {
      def_str.append(alt_display_text).append("/");
    }

    def_str.pop_back();  // erase trailing /

    if (def == options.yes_label)
      def_str.append(" (default ").append(clean_yes_text).append("): ");
    else if (def == options.no_label)
      def_str.append(" (default ").append(clean_no_text).append("): ");
    else if (!def.empty() && def == options.alt_label)
      def_str.append(" (default ").append(clean_alt_text).append("): ");
  }

  if (use_json()) {
    prompt_text = json_prompt(prompt, options);
  } else {
    prompt_text = mysqlshdk::textui::bold(prompt + " " + def_str);
    print_prompt_description(options.description);
  }

  if (out_valid_answers) {
    *out_valid_answers = def_str;
  }

  return prompt_text;
}

std::string render_select_prompt(
    const std::string &prompt, const shcore::prompt::Prompt_options &options) {
  std::string prompt_text;
  if (use_json()) {
    prompt_text = json_prompt(prompt, options);
  } else {
    print_prompt_description(options.description);

    uint32_t default_option = 0;

    if (options.default_value) {
      default_option = options.default_value.as_uint();
    }

    prompt_text = prompt;
    if (default_option != 0) {
      prompt_text += " [" + std::to_string(default_option) + "]: ";
    } else if (!shcore::str_iendswith(prompt, ": ") && prompt.back() != ':') {
      prompt_text.append(": ");
    }

    prompt_text = mysqlshdk::textui::bold(prompt_text);

    auto console = mysqlsh::current_console();
    int index = 1;
    for (const auto &option : options.select_items)
      console->println(shcore::str_format("  %d) %s", index++, option.c_str()));

    console->println();
  }

  return prompt_text;
}

Prompt_handler::Prompt_handler(
    const std::function<shcore::Prompt_result(bool, const char *,
                                              std::string *)> &do_prompt_cb)
    : m_do_prompt_cb{do_prompt_cb} {}

shcore::Prompt_result Prompt_handler::handle_prompt(
    const std::string &prompt, const shcore::prompt::Prompt_options &options,
    std::string *response) {
  if (options.type == shcore::prompt::Prompt_type::CONFIRM) {
    return handle_confirm_prompt(prompt, options, response);
  } else if (options.type == shcore::prompt::Prompt_type::SELECT) {
    return handle_select_prompt(prompt, options, response);
  } else {
    return handle_text_prompt(prompt, options, response);
  }
}

shcore::Prompt_result Prompt_handler::handle_text_prompt(
    const std::string &prompt, const shcore::prompt::Prompt_options &options,
    std::string *response) {
  auto is_password = options.type == shcore::prompt::Prompt_type::PASSWORD;
  std::string prompt_text = render_text_prompt(prompt, options);

  auto ret_val = m_do_prompt_cb(is_password, prompt_text.c_str(), response);

  return ret_val;
}

shcore::Prompt_result Prompt_handler::handle_confirm_prompt(
    const std::string &prompt, const shcore::prompt::Prompt_options &options,
    std::string *response) {
  std::string valid_answers;
  std::string prompt_text =
      render_confirm_prompt(prompt, options, &valid_answers);

  shcore::Prompt_result res;
  while (true) {
    res = m_do_prompt_cb(false, prompt_text.c_str(), response);

    if (res == shcore::Prompt_result::Cancel)
      throw shcore::cancelled("Cancelled");
    // Since fixing the prompt we need to preserve original behavior of the
    // functionality, which was repeating prompt when user did press ctrl+d.
    if (res == shcore::Prompt_result::CTRL_D) continue;
    if (res == shcore::Prompt_result::Ok) {
      if (options.is_valid_answer(*response, response)) {
        break;
      } else {
        current_console()->print("Please pick an option out of " +
                                 valid_answers);
      }
    }
  }

  return res;
}

shcore::Prompt_result Prompt_handler::handle_select_prompt(
    const std::string &prompt, const shcore::prompt::Prompt_options &options,
    std::string *response) {
  std::string prompt_text = render_select_prompt(prompt, options);

  mysqlshdk::utils::nullable<std::string> good_answer;

  shcore::Dictionary_t prompt_options;
  while (true) {
    auto res = m_do_prompt_cb(false, prompt_text.c_str(), response);

    if (res == shcore::Prompt_result::Cancel)
      throw shcore::cancelled("Cancelled");
    // Since fixing the prompt we need to preserve original behavior of the
    // functionality, which was repeating prompt when user did press ctrl+d.
    if (res == shcore::Prompt_result::CTRL_D) continue;
    if (res == shcore::Prompt_result::Ok) {
      if (options.is_valid_answer(*response, response)) {
        break;
      } else {
        current_console()->print_warning("Invalid option selected.");
      }
    }
  }

  return shcore::Prompt_result::Ok;
}

}  // namespace mysqlsh