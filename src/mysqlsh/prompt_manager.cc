/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlsh/prompt_manager.h"

#include <linenoise.h>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <vector>

#include "mysqlshdk/libs/textui/term_vt100.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

static const int k_min_prompt_space = 20;
static const int k_max_variable_recursion_depth = 32;

class Custom_variable_matches : public Prompt_manager::Custom_variable {
 public:
  Custom_variable_matches(const std::string &name, const std::string &value,
                          const std::string &pattern,
                          const std::string &true_value,
                          const std::string &false_value) {
    name_ = name;
    true_value_ = true_value;
    false_value_ = false_value;
    value_ = value;
    pattern_ = pattern;
  }

  virtual std::string evaluate(
      const std::function<std::string(const std::string &)> &apply_vars) {
    std::string pattern = apply_vars(pattern_);
    std::string value = apply_vars(value_);
    log_debug("Match '%s' vs '%s'", pattern.c_str(), value.c_str());
    if (shcore::match_glob(pattern, value))
      return apply_vars(true_value_);
    else
      return apply_vars(false_value_);
  }

 protected:
  std::string value_;
  std::string pattern_;
};

void Prompt_manager::Attributes::load(const shcore::Value::Map_type_ref &opts) {
  std::map<std::string, std::string> style_map;
  if (opts->has_key("fg")) {
    style_map["fg"] = opts->at("fg").descr();
  }
  if (opts->has_key("bg")) {
    style_map["bg"] = opts->at("bg").descr();
  }
  if (opts->has_key("bold"))
    style_map["bold"] = opts->at("bold").descr();
  if (opts->has_key("underline"))
    style_map["underline"] = opts->at("underline").descr();

  if (!style_map.empty()) {
    style.merge(mysqlshdk::textui::Style::parse(style_map));
  }

  if (opts->has_key("text")) {
    text = opts->get_string("text");
  }
  if (opts->has_key("min_width")) {
    min_width = opts->get_int("min_width");
    if (min_width < 0)
      throw std::invalid_argument("min_width value must be >= 0");
  }
  if (opts->has_key("padding")) {
    padding = opts->get_int("padding");
    if (padding < 0)
      throw std::invalid_argument("padding value must be >= 0");
  }
  if (opts->has_key("separator")) {
    sep.reset(new std::string(opts->get_string("separator")));
  }
  if (opts->has_key("shrink")) {
    std::string sh = opts->get_string("shrink");
    if (sh == "none") {
      shrink = Prompt_renderer::Shrinker_type::No_shrink;
    } else if (sh == "truncate_on_dot") {
      shrink = Prompt_renderer::Shrinker_type::Truncate_on_dot_from_right;
    } else if (sh == "ellipsize") {
      shrink = Prompt_renderer::Shrinker_type::Ellipsize_on_char;
    } else {
      throw std::invalid_argument("Invalid shrink type value " + sh);
    }
  }
}

Prompt_manager::Prompt_manager() : renderer_(k_min_prompt_space) {}

void Prompt_manager::set_theme(const shcore::Value &theme) {
  // set defaults
  renderer_.set_separator(" ", " ");

  shcore::Value::Map_type_ref old_theme(theme_);
  theme_ = theme.as_map();
  try {
    shcore::Value::Map_type_ref symbols(theme_->get_map("symbols"));
    if (symbols) {
      std::vector<std::string> sv(
          static_cast<int>(Prompt_renderer::Symbol::Last));
      if (symbols->has_key("ellipsis"))
        sv[static_cast<int>(Prompt_renderer::Symbol::Ellipsis)] =
            symbols->get_string("ellipsis");
      renderer_.set_symbols(sv);

      std::string sep, alt_sep;
      if (symbols->has_key("separator"))
        sep = symbols->get_string("separator");
      if (symbols->has_key("separator2"))
        alt_sep = symbols->get_string("separator2");
      renderer_.set_separator(sep, alt_sep);
    }
    shcore::Value::Map_type_ref prompt(theme_->get_map("prompt"));
    if (prompt) {
      std::string cont_text = prompt->get_string("cont_text");
      Attributes attr;
      attr.load(prompt);
      renderer_.set_prompt(attr.text, cont_text, attr.style);
    } else {
      renderer_.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
    }
    shcore::Value::Map_type_ref variables(theme_->get_map("variables"));
    if (variables) {
      load_variables(variables);
    }
  } catch (std::exception &e) {
    theme_ = old_theme;
    throw std::runtime_error(std::string("Error loading prompt theme: ") +
                             e.what());
  }
}

void Prompt_manager::load_variables(const shcore::Value::Map_type_ref &vars) {
  for (auto &var : *vars) {
    try {
      shcore::Value::Map_type_ref vardef(var.second.as_map());
      if (vardef) {
        std::string true_value;
        std::string false_value;
        if (vardef->has_key("if_true"))
          true_value = vardef->get_string("if_true");
        if (vardef->has_key("if_false"))
          false_value = vardef->get_string("if_false");

        if (vardef->has_key("match")) {
          auto match = vardef->get_map("match");
          if (!match->has_key("pattern"))
            throw std::logic_error("pattern missing from match rule");
          if (!match->has_key("value"))
            throw std::logic_error("value missing from match rule");
          custom_variables_[var.first].reset(new Custom_variable_matches(
              var.first, match->get_string("value"),
              match->get_string("pattern"), true_value, false_value));
        } else {
          throw std::logic_error("invalid match rule type");
        }
      }
    } catch (std::exception &e) {
      throw std::runtime_error("Error loading variable definition " +
                               var.first + ": " + e.what());
    }
  }
}

Prompt_manager::~Prompt_manager() {}

std::string Prompt_manager::do_apply_vars(
    const std::string &s, Prompt_manager::Variables_map *vars,
    Prompt_manager::Dynamic_variable_callback query_var,
    int recursion_depth) {
  std::string ret;
  std::string::size_type pos = 0, p;

  while ((p = s.find('%', pos)) != std::string::npos) {
    std::string::size_type end = s.find('%', p + 1);
    if (end == std::string::npos)
      break;

    ret.append(s.substr(pos, p - pos));

    std::string var = s.substr(p + 1, end - p - 1);
    auto it = vars->find(var);
    if (it != vars->end()) {
      ret.append(it->second);
    } else {
      std::string lvar = shcore::str_lower(var);
      if (var == "time") {
        ret.append(shcore::fmttime("%T"));
      } else if (var == "date") {
        ret.append(shcore::fmttime("%F"));
      } else if (shcore::str_beginswith(var, "env:")) {
        const char *v = getenv(var.substr(4).c_str());
        if (v) {
          ret.append(v);
        }
      } else if (shcore::str_beginswith(lvar, "sysvar:") && query_var) {
        std::string v =
            query_var(var.substr(7), Prompt_manager::Mysql_system_variable);
        if (var[0] != 'S')  // uppercase means no caching
          (*vars)[var.substr(7)] = v;
        ret.append(v);
      } else if (shcore::str_beginswith(lvar, "sessvar:") && query_var) {
        std::string v =
            query_var(var.substr(8), Prompt_manager::Mysql_session_variable);
        if (var[0] != 'S')  // uppercase means no caching
          (*vars)[var.substr(8)] = v;
        ret.append(v);
      } else if (shcore::str_beginswith(lvar, "status:") && query_var) {
        std::string v = query_var(var.substr(7), Prompt_manager::Mysql_status);
        if (var[0] != 'S')  // uppercase means no caching
          (*vars)[var.substr(7)] = v;
        ret.append(v);
      } else if (shcore::str_beginswith(lvar, "sessstatus:") && query_var) {
        std::string v =
            query_var(var.substr(11), Prompt_manager::Mysql_session_status);
        if (var[0] != 'S')  // uppercase means no caching
          (*vars)[var.substr(11)] = v;
        ret.append(v);
      } else if (custom_variables_.find(var) != custom_variables_.end()) {
        if (recursion_depth >= k_max_variable_recursion_depth) {
          return "<<Recursion detected during variable evaluation>>";
        } else {
          std::string v = custom_variables_[var]->evaluate(
              std::bind(&Prompt_manager::do_apply_vars, this,
                        std::placeholders::_1, vars, query_var,
                        recursion_depth+1));
          ret.append(v);
        }
      } else {
        ret.append(s.substr(p, end - p + 1));
      }
    }
    pos = end + 1;
  }
  ret.append(s.substr(pos));
  return ret;
}

/** Apply attributes from matching classes.
 */
void Prompt_manager::apply_classes(
    shcore::Value::Array_type_ref classes, Attributes *attributes,
    const std::function<std::string(const std::string &)> &apply_vars) {
  shcore::Value::Map_type_ref class_defs(theme_->get_map("classes"));

  if (class_defs) {
    for (auto &citer : *classes) {
      std::string class_name = apply_vars(citer.as_string());
      if (class_defs->has_key(class_name)) {
        try {
          shcore::Value::Map_type_ref class_def(
              class_defs->get_map(class_name));
          attributes->load(class_def);
          break;
        } catch (std::exception &e) {
          throw std::runtime_error("Error in class " + class_name +
                                   " of prompt theme: " + e.what());
        }
      }
    }
  }
}

void Prompt_manager::update(
    const std::function<std::string(const std::string &)> &apply_vars) {
  renderer_.clear();
  if (theme_) {
    if (theme_->has_key("segments") &&
        theme_->get_type("segments") == shcore::Array) {
      shcore::Value::Array_type_ref segments(theme_->get_array("segments"));
      for (auto seg = segments->begin(); seg != segments->end(); ++seg) {
        shcore::Value::Map_type_ref segment(seg->as_map());
        Attributes attribs;

        // load default attributes specified in the class
        int weight = segment->get_int("weight", 0);
        shcore::Value::Array_type_ref class_array(
            segment->get_array("classes"));
        if (class_array && !class_array->empty()) {
          apply_classes(class_array, &attribs, apply_vars);
        }

        // load attributes specified in the segment itself
        attribs.load(segment);

        // add the segment
        renderer_.add_segment(apply_vars(attribs.text), attribs.style, weight,
                              attribs.min_width, attribs.padding,
                              attribs.shrink, attribs.sep.get());
      }
    }
  } else {
    // Default simple style
    renderer_.set_separator("", "");
    renderer_.add_segment("mysql-");
    renderer_.add_segment(apply_vars("%mode%"));
    if (!apply_vars("%host%").empty()) {
      renderer_.add_segment(apply_vars(" [%schema%]"));
    }
    renderer_.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
  }
}

std::string Prompt_manager::get_prompt(Variables_map *vars,
                                       Dynamic_variable_callback query_var) {
  assert(vars != nullptr);
  try {
    update(std::bind(&Prompt_manager::do_apply_vars, this,
                     std::placeholders::_1, vars, query_var, 0));
    return renderer_.render();
  } catch (std::exception &e) {
    log_error("Error processing prompt: %s", e.what());
    return std::string("(Error in prompt theme: ") + e.what() + ")>";
  }
}

}  // namespace mysqlsh
