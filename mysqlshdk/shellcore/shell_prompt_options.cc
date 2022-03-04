/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/shellcore/shell_prompt_options.h"

#include <map>
#include <unordered_set>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/utils/nullable_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace prompt {

namespace {
constexpr const char kPromptText[] = "text";
constexpr const char kPromptPassword[] = "password";
constexpr const char kPromptConfirm[] = "confirm";
constexpr const char kPromptSelect[] = "select";
constexpr const char kPromptFileSave[] = "fileSave";
constexpr const char kPromptFileOpen[] = "fileOpen";
constexpr const char kPromptDirectory[] = "directory";

}  // namespace

std::string to_string(Prompt_type type) {
  switch (type) {
    case Prompt_type::CONFIRM:
      return kPromptConfirm;
    case Prompt_type::DIRECTORY:
      return kPromptDirectory;
    case Prompt_type::FILEOPEN:
      return kPromptFileOpen;
    case Prompt_type::FILESAVE:
      return kPromptFileSave;
    case Prompt_type::PASSWORD:
      return kPromptPassword;
    case Prompt_type::SELECT:
      return kPromptSelect;
    case Prompt_type::TEXT:
      return kPromptText;
  }
  throw std::logic_error("Unhandled prompt type");
}

Prompt_type to_prompt_type(const std::string &type) {
  if (shcore::str_caseeq(type, kPromptConfirm)) {
    return Prompt_type::CONFIRM;
  } else if (shcore::str_caseeq(type, kPromptDirectory)) {
    return Prompt_type::DIRECTORY;
  } else if (shcore::str_caseeq(type, kPromptFileOpen)) {
    return Prompt_type::FILEOPEN;
  } else if (shcore::str_caseeq(type, kPromptFileSave)) {
    return Prompt_type::FILESAVE;
  } else if (shcore::str_caseeq(type, kPromptPassword)) {
    return Prompt_type::PASSWORD;
  } else if (shcore::str_caseeq(type, kPromptSelect)) {
    return Prompt_type::SELECT;
  } else if (shcore::str_caseeq(type, kPromptText)) {
    return Prompt_type::TEXT;
  } else {
    throw std::runtime_error("Invalid prompt type: '" + type + "'.");
  }
}

char process_label(const std::string &s, std::string *out_display,
                   std::string *out_clean_text) {
  out_display->clear();
  if (s.empty()) return 0;

  char letter = 0;
  char prev = 0;
  for (char c : s) {
    if (prev == '&') letter = c;
    if (c != '&') {
      if (prev == '&') {
        out_display->push_back('[');
        out_display->push_back(c);
        out_display->push_back(']');
      } else {
        out_display->push_back(c);
      }
      out_clean_text->push_back(c);
    }
    prev = c;
  }
  return letter;
}

bool Prompt_options::icomp(const std::string &lhs, const std::string &rhs) {
  return shcore::str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
}

Prompt_options::Prompt_options()
    : allow_custom(false), m_allowed_answers_map(Prompt_options::icomp) {}

const shcore::Option_pack_def<Prompt_options> &Prompt_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Prompt_options>()
          .optional(k_title, &Prompt_options::title)
          .optional(k_description, &Prompt_options::description)
          .optional(k_type, &Prompt_options::set_type)
          .optional(k_yes_label, &Prompt_options::yes_label)
          .optional(k_no_label, &Prompt_options::no_label)
          .optional(k_alt_label, &Prompt_options::alt_label)
          .optional(k_options, &Prompt_options::select_items)
          .optional(k_default_value, &Prompt_options::default_value)
          .on_done(&Prompt_options::on_unpacked_options);

  return opts;
}

void Prompt_options::set_type(const std::string &value) {
  try {
    type = to_prompt_type(value);
  } catch (const std::runtime_error &err) {
    throw std::invalid_argument(err.what());
  }
}

void Prompt_options::on_unpacked_options() {
  // Sets some default values to ease the rest of the logic
  if (type == Prompt_type::CONFIRM) {
    if (yes_label.empty()) {
      yes_label = "&Yes";
    }
    if (no_label.empty()) {
      no_label = "&No";
    }
  }

  if ((!yes_label.empty() || !no_label.empty() || !alt_label.empty()) &&
      type != Prompt_type::CONFIRM) {
    throw std::invalid_argument(
        "The 'yes', 'no' and 'alt' options only can be used on 'confirm' "
        "prompts.");
  }

  auto add_valid_answer = [this](const std::string &answer,
                                 const std::string &real_answer,
                                 const std::string &context) {
    if (m_allowed_answers_map.find(answer) != m_allowed_answers_map.end()) {
      throw std::invalid_argument(shcore::str_format(
          "'%s' %s is duplicated", answer.c_str(), context.c_str()));
    }

    m_allowed_answers_map[answer] = real_answer;
  };

  if (type == Prompt_type::SELECT) {
    if (select_items.empty()) {
      throw std::invalid_argument("The 'options' list can not be empty.");
    } else {
      for (size_t index = 0; index < select_items.size(); index++) {
        auto stripped = shcore::str_strip(select_items[index]);
        if (stripped.empty()) {
          throw std::invalid_argument(
              "The 'options' list can not contain empty or blank elements.");
        }

        add_valid_answer(stripped, stripped, "item");
        add_valid_answer(std::to_string(index + 1), stripped, "index");
      }
    }
  } else {
    if (!select_items.empty()) {
      throw std::invalid_argument(
          "The 'options' list only can be used on 'select' prompts.");
    }
  }

  if (type == Prompt_type::CONFIRM) {
    std::vector<std::string> labels = {yes_label, no_label};

    if (!alt_label.empty()) {
      labels.push_back(alt_label);
    }

    try {
      for (const auto &label : labels) {
        std::string display;
        std::string clean;
        char shortcut = process_label(label, &display, &clean);
        add_valid_answer(label, label, "label");

        if (clean != label) {
          add_valid_answer(clean, label, "value");
        }

        if (shortcut != 0) {
          add_valid_answer(std::string(&shortcut, 1), label, "shortcut");
        }
      }
    } catch (const std::invalid_argument &error) {
      throw std::invalid_argument(
          shcore::str_format("Labels, shortcuts and values in 'confirm' "
                             "prompts must be unique: %s",
                             error.what()));
    }
  }

  if (default_value) {
    try {
      if (type == Prompt_type::SELECT) {
        auto val = default_value.as_uint();

        if (val == 0 || val > select_items.size()) {
          throw std::invalid_argument(
              "The 'defaultValue' should be the 1 based index of the default "
              "option.");
        }
      } else {
        default_value.check_type(shcore::Value_type::String);
        const auto &val = default_value.as_string();

        if (type == Prompt_type::CONFIRM) {
          std::string real_answer;
          if (!is_valid_answer(val, &real_answer)) {
            std::vector<std::string> valid_answer_list;

            for (const auto &item : m_allowed_answers_map) {
              valid_answer_list.push_back(item.first);
            }

            throw std::invalid_argument(
                "Invalid 'defaultValue', allowed values include: " +
                shcore::str_join(valid_answer_list, ", "));
          }

          // Normalizes the default value to the real answer
          default_value = shcore::Value(real_answer);
        }
      }
    } catch (const shcore::Exception &err) {
      std::string error =
          shcore::str_replace(err.what(), "Invalid typecast",
                              "Invalid data type on 'defaultValue'");
      throw std::invalid_argument(error);
    }
  }
}

bool Prompt_options::is_valid_answer(const std::string &answer,
                                     std::string *real_answer) const {
  bool ret_val = false;
  std::string user_answer(answer);
  if (answer.empty()) {
    if (default_value) {
      user_answer = default_value.as_string();
    }
  }

  auto set_answer = [&ret_val, real_answer](const std::string &value) {
    ret_val = true;
    if (real_answer) {
      *real_answer = value;
    }
  };

  // Only handles the validation for SELECT/CONFIRM prompts, others should not
  // be calling this
  if (type == Prompt_type::SELECT) {
    try {
      int option = 0;
      option = std::stoi(user_answer);
      // The selection is an item number
      if (option > 0 && option <= static_cast<int>(select_items.size())) {
        set_answer(select_items[option - 1]);
      }
    } catch (...) {
      // NOOP - This means it was not an int
      if (allow_custom) {
        set_answer(user_answer);
      }
    }
  } else if (type == Prompt_type::CONFIRM) {
    if (!user_answer.empty()) {
      const auto answer_index = m_allowed_answers_map.find(user_answer);
      if (answer_index != m_allowed_answers_map.end()) {
        set_answer(answer_index->second);
      }
    }
  }

  return ret_val;
}

}  // namespace prompt
}  // namespace shcore
