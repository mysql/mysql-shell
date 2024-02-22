/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/router_options.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include "mysqlshdk/include/scripting/types.h"
#include "utils/utils_string.h"

namespace mysqlsh::dba {

// -- Router_configuration_changes_schema --

Router_configuration_changes_schema::Json_type
Router_configuration_changes_schema::to_type(std::string_view type) {
  if (shcore::str_caseeq("STRING", type)) {
    return Json_type::STRING;
  } else if (shcore::str_caseeq("NUMBER", type)) {
    return Json_type::NUMBER;
  } else if (shcore::str_caseeq("INTEGER", type)) {
    return Json_type::INTEGER;
  } else if (shcore::str_caseeq("BOOLEAN", type)) {
    return Json_type::BOOLEAN;
  } else if (shcore::str_caseeq("OBJECT", type)) {
    return Json_type::OBJECT;
  } else if (shcore::str_caseeq("ARRAY", type)) {
    return Json_type::ARRAY;
  } else {
    throw std::logic_error(shcore::str_format("Unsupported Json_type value: %s",
                                              std::string(type).c_str()));
  }
}

void Router_configuration_changes_schema::add_property(
    const std::string &property_name, std::string name,
    std::string_view str_type, std::vector<std::string> accepted_values,
    std::pair<double, double> number_range) {
  Json_type type = to_type(str_type);

  // Find or insert the property vector for the given property_name
  auto &property_vector = properties[property_name];

  // Emplace back a new Property into the vector
  property_vector.push_back(std::make_unique<Property>(
      std::move(name), std::move(type), std::move(accepted_values),
      std::move(number_range)));
}

std::vector<std::string>
Router_configuration_changes_schema::get_property_names() const {
  std::vector<std::string> ret;
  ret.reserve(properties.size());

  std::transform(properties.begin(), properties.end(), std::back_inserter(ret),
                 [](const auto &pair) { return pair.first; });

  return ret;
}

/*
 * Construct the Router_configuration_changes_schema object that represents the
 * shcore::Value schema passed by argument
 */
Router_configuration_changes_schema::Router_configuration_changes_schema(
    const shcore::Value &schema) {
  if (!schema) return;

  const auto configuration_map = schema.as_map();

  // Iterate the properties
  for (const auto &property : *configuration_map) {
    const std::string &property_name = property.first;

    // Get the list of sub-properties
    const auto properties_list = property.second.as_map();

    if (!properties_list->has_key("properties")) {
      throw std::logic_error("Unable to parse document: malformed JSON");
    }

    const auto option_list = properties_list->at("properties").as_map();

    // Iterate the options
    for (const auto &option : *option_list) {
      const std::string &name = option.first;
      const auto &option_map = option.second.as_map();

      if (!option_map->has_key("type")) {
        throw std::logic_error("Unable to parse document: malformed JSON");
      }

      const std::string &type = option_map->at("type").as_string();

      std::vector<std::string> accepted_values;
      std::pair<double, double> number_range;

      // Get the accepted values, either ENUM or RANGE
      if (option_map->has_key("enum")) {
        const auto &avs = option.second.as_map()->at("enum").as_array();
        accepted_values.reserve(avs->size());

        std::transform(
            avs->begin(), avs->end(), std::back_inserter(accepted_values),
            [](const shcore::Value &val) { return val.as_string(); });
      }

      if (option_map->has_key("minimum") && option_map->has_key("maximum")) {
        const auto &minimum = option_map->at("minimum").as_double();
        const auto &maximum = option_map->at("maximum").as_double();

        number_range = {minimum, maximum};
      }

      // After all data is collected, add the property to the object
      add_property(property_name, name, type, accepted_values, number_range);
    }
  }
}

// -- Router_configuration_schema --

/*
 * Convert options in shcore::Value format into the Router_configuration_schema
 * types
 */
Router_configuration_schema::Option Router_configuration_schema::parse_option(
    std::string name, const shcore::Value &value) {
  Option option(std::move(name));

  switch (value.get_type()) {
    case shcore::Undefined:
    case shcore::Function:
    case shcore::Binary:
    case shcore::Object:
      throw std::logic_error("Unable to parse document: malformed JSON");
    case shcore::Null:
      option.value = std::monostate{};
      break;
    case shcore::Bool:
      option.value = value.as_bool();
      break;
    case shcore::String:
      option.value = value.as_string();
      break;
    case shcore::Integer:
    case shcore::UInteger:
      option.value = value.as_int();
      break;
    case shcore::Float:
      option.value = value.as_double();
      break;
    case shcore::Array:
      // TODO(miguel): implement array handling
      break;
    case shcore::Map: {
      const auto inner_options = value.as_map();

      if (inner_options->empty()) {
        option.value = std::monostate{};
        break;
      }

      for (const auto &inner_option : *inner_options) {
        const std::string &inner_name = inner_option.first;
        const shcore::Value &inner_value = inner_option.second;

        auto inner_opt = parse_option(inner_name, inner_value);

        option.sub_option[inner_name] =
            std::make_unique<Option>(std::move(inner_opt));
      }
      break;
    }
  }

  return option;
}

void Router_configuration_schema::add_option(const std::string &category,
                                             std::string name,
                                             std::string str_value) {
  auto &options_map = options[category];

  // Create the new Option and emplace it in the options vector. If the vector
  // is empty it's started, otherwise, the Option is added there
  options_map.emplace_back(
      std::make_unique<Option>(std::move(name), std::move(str_value)));
}

void Router_configuration_schema::add_option(const std::string &category,
                                             Option option) {
  auto &options_map = options[category];

  // Create the new Option and emplace it in the options vector. If the vector
  // is empty it's started, otherwise, the Option is added there
  options_map.emplace_back(std::make_unique<Option>(std::move(option)));
}

/*
 * Construct the Router_configuration_schema object that represents the
 * shcore::Value schema passed by argument
 */
Router_configuration_schema::Router_configuration_schema(
    const shcore::Value &schema) {
  if (!schema) return;

  const auto options_map = schema.as_map();

  // Iterate the sections
  for (const auto &section : *options_map) {
    const std::string &section_name = section.first;
    const auto &options_list = section.second.as_map();

    // Iterate the options of each section
    for (const auto &option : *options_list) {
      const std::string &option_name = option.first;
      const shcore::Value &option_value = option.second;

      add_option(section_name, parse_option(option_name, option_value));
    }
  }
}

/*
 * Get the differences of the parent Configuration Schema in comparison with the
 * one passed by argument
 */
Router_configuration_schema
Router_configuration_schema::diff_configuration_schema(
    const Router_configuration_schema &target_schema) const {
  Router_configuration_schema diff_schema;

  // Iterate over the categories in the parent schema
  for (const auto &category : options) {
    const auto &category_name = category.first;

    const auto &it_target = target_schema.options.find(category_name);

    // If the category is not found in the target schema, include all options
    // This cover the scenario on which the router configuration has some
    // options that are not part of the defaults, for example
    // [metadata_cache::user]
    if (it_target == target_schema.options.end()) {
      auto &diff_category = diff_schema.options[category_name];
      for (const auto &option : category.second) {
        diff_category.push_back(std::make_unique<Option>(*option));
      }

      // Skip the rest, we're done with this category
      continue;
    }

    const auto &options_parent = category.second;
    const auto &options_target = it_target->second;

    std::vector<std::unique_ptr<Option>> diff_category_options;

    // Iterate over options in the parent schema
    for (const auto &option_current : options_parent) {
      auto it_option_target =
          std::find_if(options_target.begin(), options_target.end(),
                       [&option_current](const auto &opt) {
                         return opt->name == option_current->name;
                       });

      // If option not found in target schema, include in differences
      if (it_option_target == options_target.end()) {
        diff_category_options.emplace_back(
            std::make_unique<Option>(*option_current));

        // We're done, move to the next one
        continue;
      }

      // Look for differences in the options
      const auto &option_target = **it_option_target;

      Option diff_option(option_current->name);

      if (option_current->value != option_target.value) {
        diff_option.value = option_current->value;
      }

      // Iterate the sub-options to check for differences
      for (const auto &sub_option_current : option_current->sub_option) {
        auto it_sub_option_target =
            option_target.sub_option.find(sub_option_current.first);

        // If option was found and has a different value, include it in the
        // differences. If the option was not found, include it too
        if (it_sub_option_target == option_target.sub_option.end() ||
            sub_option_current.second->value !=
                it_sub_option_target->second->value) {
          // Set the name and the value in the aux diff_option object
          diff_option.sub_option[sub_option_current.first] =
              std::make_unique<Option>(sub_option_current.first);

          diff_option.sub_option[sub_option_current.first]->value =
              sub_option_current.second->value;
        }
      }

      // If diff_option is not empty, emplace that to the aux vector of Options
      if (!diff_option.sub_option.empty() || diff_option.value.index() != 0) {
        diff_category_options.emplace_back(
            std::make_unique<Option>(diff_option));
      }
    }

    // If the aux vector of option is not empty, move it to the final schema
    if (!diff_category_options.empty()) {
      diff_schema.options[category_name] = std::move(diff_category_options);
    }
  }

  return diff_schema;
}

/*
 * Filter out from the parent Configuration Schema all options that do not
 * belong to the Configuration ChangesSchema passed by argument
 */
Router_configuration_schema
Router_configuration_schema::filter_schema_by_changes(
    const Router_configuration_changes_schema &configurations) const {
  Router_configuration_schema result;

  const auto &target_properties = configurations.get_properties();

  // Iterate the categories of the parent schema
  for (const auto &category : options) {
    const std::string &category_name = category.first;

    const auto &changes_category_it = target_properties.find(category_name);

    // Not found, move to the next one
    if (changes_category_it == target_properties.end()) continue;

    std::vector<std::unique_ptr<Option>> filtered_options;

    const auto &cat_options = category.second;
    const auto &changes_properties = changes_category_it->second;

    // Iterate over the options of the categories
    for (const auto &option : cat_options) {
      const std::string &option_name = option->name;

      // keep the "tags" to ensure backward compatibility
      if (option_name == "tags") {
        filtered_options.push_back(std::make_unique<Option>(*option));
        continue;
      }

      // Check if the option is in the configuration changes schema
      auto changes_option_it =
          std::find_if(changes_properties.begin(), changes_properties.end(),
                       [&option_name](const auto &changes_option) {
                         return changes_option->name == option_name;
                       });

      // If found, add the to the auxiliary list
      if (changes_option_it != changes_properties.end()) {
        filtered_options.push_back(std::make_unique<Option>(*option));
      }
    }

    // Move the auxiliary list of options to the returning object
    if (!filtered_options.empty()) {
      result.options[category_name] = std::move(filtered_options);
    }
  }

  return result;
}

/*
 * Filter out from the parent Configuration Schema all options that exist in the
 * "common" category, i.e. have the same name and value
 */
Router_configuration_schema
Router_configuration_schema::filter_common_router_options() const {
  if (!options.contains("common")) return {};

  // Create a new schema object to store filtered options
  Router_configuration_schema filtered_schema;

  // Copy "common" options to the filtered schema since we must keep it
  const auto &common_options = options.at("common");

  filtered_schema.options["common"].reserve(common_options.size());

  for (const auto &option : common_options) {
    filtered_schema.options["common"].push_back(
        std::make_unique<Option>(*option));
  }

  // Iterate through each category and its options in the original schema
  for (const auto &[cat_name, cat_options] : options) {
    // Skip the "common" category
    if (cat_name == "common") continue;

    // Filter options
    std::vector<std::unique_ptr<Option>> filtered_cat_options;

    for (const auto &opt : cat_options) {
      // Check if the current option is present in the "common" options
      bool common_match =
          std::any_of(common_options.begin(), common_options.end(),
                      [&opt](const auto &common_opt) {
                        return opt->name == common_opt->name &&
                               opt->value == common_opt->value;
                      });

      // If the current option is present in "common", skip it
      if (common_match) continue;

      // If the option is not found, add it to the resulting object
      auto filtered_option = std::make_unique<Option>(*opt);
      filtered_option->sub_option.clear();  // Remove existing sub-options

      // Filter sub-options
      for (const auto &[sub_opt_name, sub_opt] : opt->sub_option) {
        bool sub_common_match = false;

        // Iterate over the sub-options too
        for (const auto &global_sub_opt : options.at("common")) {
          if (sub_opt_name == global_sub_opt->name &&
              sub_opt->value == global_sub_opt->value) {
            sub_common_match = true;
            break;
          }
        }

        // If the sub-option was not found we must add it
        if (!sub_common_match) {
          filtered_option->sub_option.emplace(
              sub_opt_name, std::make_unique<Option>(*sub_opt));
        }
      }

      // Add the option to the resulting object if it's not empty, except for
      // the "tags" that should be added even if empty
      if (filtered_option->name == "tags" ||
          !filtered_option->sub_option.empty() ||
          filtered_option->value.index() != 0) {
        filtered_cat_options.push_back(std::move(filtered_option));
      }
    }

    // Add the filtered options for the current category to the filtered schema
    if (!filtered_cat_options.empty()) {
      filtered_schema.options[cat_name] = std::move(filtered_cat_options);
    }
  }

  return filtered_schema;
}

/*
 * Convert an Option to a representation in shcore::Value
 */
shcore::Value Router_configuration_schema::convert_option_to_value(
    const Option &option) const {
  // If the option has sub-options, convert them to a shcore::Map
  if (!option.sub_option.empty()) {
    shcore::Dictionary_t inner_options = shcore::make_dict();

    for (const auto &inner_option : option.sub_option) {
      inner_options->emplace(inner_option.first,
                             convert_option_to_value(*inner_option.second));
    }

    return shcore::Value(std::move(inner_options));
  }

  // If the option does not have sub-options, set its value directly
  if (auto *str_value = std::get_if<std::string>(&option.value)) {
    return shcore::Value(*str_value);
  } else if (auto *int_value = std::get_if<int64_t>(&option.value)) {
    return shcore::Value(*int_value);
  } else if (auto *double_value = std::get_if<double>(&option.value)) {
    return shcore::Value(*double_value);
  } else if (auto *bool_value = std::get_if<bool>(&option.value)) {
    return shcore::Value(*bool_value);
  } else if (std::holds_alternative<std::monostate>(option.value)) {
    return shcore::Value(shcore::make_dict());
  }

  return {};
}

/*
 * Convert the object to a representation in shcore::Value
 */
shcore::Value Router_configuration_schema::to_value() {
  shcore::Dictionary_t result = shcore::make_dict();

  for (const auto &category : options) {
    const std::string &category_name = category.first;
    const auto &cat_options = category.second;

    shcore::Dictionary_t category_options = shcore::make_dict();

    for (const auto &option : cat_options) {
      category_options->emplace(option->name, convert_option_to_value(*option));
    }

    result->emplace(category_name, shcore::Value(std::move(category_options)));
  }

  return shcore::Value(std::move(result));
}

}  // namespace mysqlsh::dba
