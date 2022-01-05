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

#include "modules/shell_prompt_options.h"

#include <map>
#include <unordered_set>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/utils/nullable_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace prompt {
namespace {
constexpr const char kTitle[] = "title";
constexpr const char kDescription[] = "description";
constexpr const char kType[] = "type";
constexpr const char kYesLabel[] = "yes";
constexpr const char kNoLabel[] = "no";
constexpr const char kAltLabel[] = "alt";
constexpr const char kOptions[] = "options";
constexpr const char kDefaultValue[] = "defaultValue";
}  // namespace

Prompt_options::Prompt_options() {}

const shcore::Option_pack_def<Prompt_options> &Prompt_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Prompt_options>()
          .optional(kTitle, &Prompt_options::title)
          .optional(kDescription, &Prompt_options::description)
          .optional(kType, &Prompt_options::set_type)
          .optional(kYesLabel, &Prompt_options::yes_label)
          .optional(kNoLabel, &Prompt_options::no_label)
          .optional(kAltLabel, &Prompt_options::alt_label)
          .optional(kOptions, &Prompt_options::select_items)
          .optional(kDefaultValue, &Prompt_options::default_value)
          .on_done(&Prompt_options::on_unpacked_options);

  return opts;
}

void Prompt_options::set_type(const std::string &value) {
  try {
    type = mysqlsh::to_prompt_type(value);
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

  if (type == Prompt_type::SELECT) {
    if (select_items.empty()) {
      throw std::invalid_argument("The 'options' list can not be empty.");
    } else {
      for (const auto &item : select_items) {
        auto stripped = shcore::str_strip(item);
        if (stripped.empty()) {
          throw std::invalid_argument(
              "The 'options' list can not contain empty or blank elements.");
        }
      }
    }
  } else {
    if (!select_items.empty()) {
      throw std::invalid_argument(
          "The 'options' list only can be used on 'select' prompts.");
    }
  }

  auto comparator = [](const std::string &lhs, const std::string &rhs) {
    return shcore::str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
  };

  std::map<std::string, std::string, decltype(comparator)> allowed_defaults(
      comparator);
  std::vector<std::string> default_list;
  if (type == Prompt_type::CONFIRM) {
    std::vector<std::string> labels = {yes_label, no_label};

    if (!alt_label.empty()) {
      labels.push_back(alt_label);
    }

    auto add_default = [&allowed_defaults, &default_list](
                           const std::string &def, const std::string &lbl,
                           const std::string &context) {
      if (allowed_defaults.find(def) != allowed_defaults.end()) {
        throw std::invalid_argument(shcore::str_format(
            "Labels, shortcuts and values in 'confirm' prompts must "
            "be unique: '%s' %s is duplicated",
            def.c_str(), context.c_str()));
      }

      allowed_defaults[def] = lbl;
      default_list.push_back(def);
    };

    for (const auto &label : labels) {
      std::string display;
      std::string clean;
      char shortcut = process_label(label, &display, &clean);
      add_default(label, label, "label");
      add_default(clean, label, "value");
      add_default(std::string(&shortcut, 1), label, "shortcut");
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
          const auto ad = allowed_defaults.find(val);
          if (ad == allowed_defaults.end()) {
            throw std::invalid_argument(
                "Invalid 'defaultValue', allowed values include: " +
                shcore::str_join(default_list, ", "));
          }

          // Normalizes the default value to the label for internal use
          default_value = shcore::Value(ad->second);
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
}  // namespace prompt
}  // namespace mysqlsh
