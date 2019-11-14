/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/shellcore/wizard.h"

#include <algorithm>

#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace wizard {

Wizard::Wizard_step *Wizard::add_step(const std::string &id, Prompt_type type) {
  if (has_step(id))
    throw std::runtime_error("Step '" + id + "' already exists.");

  switch (type) {
    case Prompt_type::PROMPT:
      m_steps.emplace(id, std::make_unique<Wizard_prompt>(id));
      break;
    case Prompt_type::CONFIRM:
      m_steps.emplace(id, std::make_unique<Wizard_confirm>(id));
      break;
    case Prompt_type::SELECTION:
      m_steps.emplace(id, std::make_unique<Wizard_select>(id));
      break;
  }

  return m_steps[id].get();
}

void Wizard::Wizard_prompt::verify() { ensure_link(K_NEXT); }

void Wizard::Wizard_confirm::verify() {
  if (!has_link(K_NEXT)) {
    if (!has_link(K_YES)) {
      throw std::logic_error(
          "Missing link, either 'K_YES' or 'K_NEXT' must be defined for step "
          "'" +
          id + "'.");
    } else if (!has_link(K_NO)) {
      throw std::logic_error(
          "Missing link, either 'K_NO' or 'K_NEXT' must be defined for step '" +
          id + "'.");
    }
  }
  if (!has_link(K_YES) || !has_link(K_NO)) ensure_link(K_NEXT);
}

void Wizard::Wizard_select::verify() {
  if (custom) ensure_link(K_NEXT);

  if (item_list.empty())
    throw std::logic_error("No items defined for select step '" + id + "'.");

  if (def_option > item_list.size())
    throw std::logic_error("Default option for select step '" + id +
                           "' is not valid.");

  size_t count = 0;
  for (const auto &item : item_list) {
    if (has_link(item)) count++;
  }

  if (count < item_list.size() && !has_link(K_NEXT))
    throw std::logic_error(
        "Missing link, either 'K_NEXT' or a link for every option must be "
        "defined for step '" +
        id + "'.");
}

std::string Wizard::Wizard_prompt::execute() {
  std::string response;

  auto console = mysqlsh::current_console();
  if (password)
    console->prompt_password(text, &response, validate_cb);
  else
    console->prompt(text, &response, validate_cb);

  return response;
}

std::string Wizard::Wizard_confirm::execute() {
  std::string answer;
  mysqlsh::Prompt_answer result =
      mysqlsh::current_console()->confirm(text, def_answer);

  switch (result) {
    case mysqlsh::Prompt_answer::YES:
      answer = K_YES;
      break;
    case mysqlsh::Prompt_answer::NO:
      answer = K_NO;
      break;
    case mysqlsh::Prompt_answer::NONE:
    case mysqlsh::Prompt_answer::ALT:
      // TODO(rennox): Nothing to do, wizard only allows YES/NO
      break;
  }

  return answer;
}

std::string Wizard::Wizard_select::execute() {
  std::string answer;
  mysqlsh::current_console()->select(text, &answer, item_list, def_option,
                                     custom, validate_cb);
  return answer;
}

Wizard::Wizard_prompt &Wizard::add_prompt(const std::string &id) {
  return *reinterpret_cast<Wizard_prompt *>(add_step(id, Prompt_type::PROMPT));
}

Wizard::Wizard_confirm &Wizard::add_confirm(const std::string &id) {
  return *reinterpret_cast<Wizard_confirm *>(
      add_step(id, Prompt_type::CONFIRM));
}

Wizard::Wizard_select &Wizard::add_select(const std::string &id) {
  return *reinterpret_cast<Wizard_select *>(
      add_step(id, Prompt_type::SELECTION));
}

void Wizard::relink(const std::string &step, const std::string &trigger,
                    const std::string &target) {
  if (m_steps.find(step) != m_steps.end()) {
    if (!m_steps[step]->has_link(trigger))
      throw std::logic_error("Link '" + trigger + "' for step '" + step +
                             "'is not defined.");
    else
      m_steps[step]->links[trigger] = target;
  } else {
    throw std::logic_error("Step '" + step + "' does not exist.");
  }
}

bool Wizard::has_step(const std::string &id) {
  return m_steps.find(id) != m_steps.end();
}

bool Wizard::execute(const std::string &id) {
  std::string current_id = id;

  // The first thing done is a verification of the wizard
  // link consistency (to avoid running a wizard that will fail anyway)
  verify(id);

  auto console = mysqlsh::current_console();

  while (!m_cancelled && current_id != K_DONE) {
    auto prompt = m_steps[current_id].get();

    if (!prompt->details.empty()) {
      for (const auto &data : prompt->details) {
        console->println(data);
      }
    }

    try {
      m_data[current_id] = prompt->execute();

      prompt->on_leave(this);

      if (prompt->has_link(m_data[current_id]))
        current_id = prompt->links[m_data[current_id]];
      else
        current_id = prompt->links[K_NEXT];
    } catch (const shcore::cancelled &) {
      m_cancelled = true;
    }
  }

  return !m_cancelled;
}

void Wizard::reset() {
  m_cancelled = false;
  m_data.clear();
}

void Wizard::verify(const std::string &id, std::vector<std::string> *data) {
  if (id != K_DONE) {
    if (!has_step(id)) throw std::logic_error("Unexisting step '" + id + "'");

    data->push_back(id);
    std::string token = id + "->";
    if (std::find_if(std::begin(*data), std::end(*data),
                     [&token](const std::string &item) {
                       return shcore::str_beginswith(item, token);
                     }) != std::end(*data)) {
      throw std::logic_error("Cycle found in wizard: " +
                             shcore::str_join(*data, ", "));
    }

    auto step = m_steps[id].get();

    step->verify();

    // Adds the link definition for the cicle verification
    for (const auto &link : step->links) {
      if (link.second != K_DONE) {
        data->back() += "->" + link.first;
        verify(link.second, data);
      }
    }

    data->pop_back();
  }
}

void Wizard::verify(const std::string &id) {
  std::vector<std::string> done;

  verify(id, &done);
}

}  // namespace wizard
}  // namespace shcore
