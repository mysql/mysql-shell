/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/fault_injection.h"

#include <cassert>
#include <map>
#include <regex>
#include <vector>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace utils {

// ----------------------------------------------------------------------------
namespace {

bool debug_fi() {
  static bool is_enabled = getenv("TEST_DEBUG_FI") != nullptr;
  return is_enabled;
}

class Fault_injector {
 public:
  Fault_injector(std::string type_name,
                 std::function<void(const FI::Args &)> injector)
      : m_name(std::move(type_name)), m_inject(std::move(injector)) {}

  const std::string &name() const { return m_name; }

  void add_trap(const FI::Conditions &conds,
                const FI::Trap_options &trap_options) {
    if (debug_fi()) fprintf(stderr, "FI: Adding '%s' trap\n", m_name.c_str());
    m_traps.emplace_back(conds, trap_options);
  }

  void clear_traps() {
    if (debug_fi())
      fprintf(stderr, "FI: Clearing '%s' traps\n", m_name.c_str());
    m_traps.clear();
  }

  void trigger(const FI::Trigger_options &trigger_options) {
    auto is_onetime = [](const FI::Trigger_options &opts) {
      auto it = opts.find("onetime");
      return it != opts.end() && it->second != "0";
    };

    if (debug_fi()) fprintf(stderr, "FI: Trigger '%s' trap\n", m_name.c_str());

    if (m_paused) {
      if (debug_fi())
        fprintf(stderr, "FI: Trigger '%s' is paused\n", m_name.c_str());
      return;
    }

    for (auto &trap : m_traps) {
      if (trap.conds.match(&trap, trigger_options)) {
        const auto &trap_options = trap.options;

        if (debug_fi())
          fprintf(stderr, "FI: Trap '%s' matched%s\n", m_name.c_str(),
                  (trap.active ? "" : " but is not active"));

        if (trap.active) {
          if (is_onetime(trap_options)) trap.active = false;

          m_inject(FI::Args(trigger_options, trap_options));
        }
        break;
      } else {
        if (debug_fi())
          fprintf(stderr, "FI: Trap '%s' wasn't matched\n", m_name.c_str());
      }
    }
  }

  void reset() {
    for (auto &trap : m_traps) {
      trap.active = true;
      trap.match_counter = 0;
    }
  }

  void pause() {
    ++m_paused;
    if (debug_fi() && 1 == m_paused)
      fprintf(stderr, "FI: Trigger '%s' paused\n", m_name.c_str());
  }

  void resume() {
    assert(m_paused > 0);
    --m_paused;
    if (debug_fi() && 0 == m_paused)
      fprintf(stderr, "FI: Trigger '%s' resumed\n", m_name.c_str());
  }

 private:
  std::string m_name;
  std::function<void(const FI::Args &)> m_inject;

  std::list<FI::Trap> m_traps;
  static thread_local int m_paused;
};

thread_local int Fault_injector::m_paused = 0;

std::unique_ptr<std::vector<Fault_injector>> &fault_injectors() {
  static std::unique_ptr<std::vector<Fault_injector>> fault_injectors_ptr;
  return fault_injectors_ptr;
}

}  // namespace

// ----------------------------------------------------------------------------

FI::Conditions &FI::Conditions::add(const std::string &str) {
  auto tokens = shcore::str_split(str, " ", 2);
  if (tokens.size() != 3)
    throw std::runtime_error("Invalid condition string: " + str);

  if (tokens[1] == "==") {
    add_eq(tokens[0], tokens[2], false);
  } else if (tokens[1] == "!=") {
    add_eq(tokens[0], tokens[2], true);
  } else if (tokens[1] == ">") {
    add_gt(tokens[0], std::stoi(tokens[2]));
  } else if (tokens[1] == "regex") {
    add_regex(tokens[0], tokens[2], false);
  } else if (tokens[1] == "!regex") {
    add_regex(tokens[0], tokens[2], true);
  } else {
    throw std::runtime_error("Invalid condition operator: " + tokens[1]);
  }

  return *this;
}

FI::Conditions &FI::Conditions::add_regex(const std::string &key,
                                          const std::string &value,
                                          bool invert) {
  m_matchers.push_back(
      [key, value, invert](Trap *, const Trigger_options &options) {
        auto it = options.find(key);
        if (it == options.end()) return false;

        std::regex re(value, std::regex::icase);

        bool r = std::regex_search(it->second, re);
        if (debug_fi())
          fprintf(stderr, "FI: \"%s\" matches \"%s\" => %s\n",
                  it->second.c_str(), value.c_str(), r ? "true" : "false");

        return invert ? (!r) : r;
      });

  return *this;
}

FI::Conditions &FI::Conditions::add_eq(const std::string &key,
                                       const std::string &value, bool invert) {
  if (key == "++match_counter") {
    int ivalue = std::stoi(value);

    m_matchers.push_back(
        [key, ivalue, invert](Trap *trap, const Trigger_options &) {
          bool r = ivalue == ++trap->match_counter;
          if (debug_fi())
            fprintf(stderr, "FI: match_counter (%i) = %i => %s\n",
                    trap->match_counter, ivalue, r ? "true" : "false");

          return invert ? (!r) : r;
        });
  } else {
    m_matchers.push_back(
        [key, value, invert](Trap *, const Trigger_options &options) {
          auto it = options.find(key);
          if (it != options.end()) {
            bool r = value == it->second;
            if (debug_fi())
              fprintf(stderr, "FI: \"%s\" = \"%s\" => %s\n", it->second.c_str(),
                      value.c_str(), r ? "true" : "false");

            return invert ? (!r) : r;
          }
          return false;
        });
  }
  return *this;
}

FI::Conditions &FI::Conditions::add_gt(const std::string &key, int value) {
  if (key == "++match_counter") {
    m_matchers.push_back([key, value](Trap *trap, const Trigger_options &) {
      bool r = value < ++trap->match_counter;

      if (debug_fi())
        fprintf(stderr, "FI: match_counter (%i) > %i => %s\n",
                trap->match_counter, value, r ? "true" : "false");

      return r;
    });
  } else {
    m_matchers.push_back([key, value](Trap *, const Trigger_options &options) {
      auto it = options.find(key);
      if (it != options.end()) {
        bool r = value < std::stoi(it->second);

        if (debug_fi())
          fprintf(stderr, "FI: %s (%s) > %i => %s\n", key.c_str(),
                  it->second.c_str(), value, r ? "true" : "false");

        return r;
      }
      return false;
    });
  }

  return *this;
}

bool FI::Conditions::match(Trap *injector,
                           const Trigger_options &options) const {
  bool r = true;
  for (const auto &m : m_matchers) {
    if (false == m(injector, options)) {
      r = false;
      break;
    }
  }
  return r;
}

// ----------------------------------------------------------------------------

std::mutex FI::s_mutex;

FI::Type_id FI::add_injector(
    const std::string &type,
    const std::function<void(const Args &)> &injector) {
  const std::lock_guard<std::mutex> lock{s_mutex};

  if (debug_fi()) fprintf(stderr, "FI: Add handler for '%s'\n", type.c_str());

  if (!fault_injectors())
    fault_injectors() = std::make_unique<std::vector<Fault_injector>>();

  fault_injectors()->emplace_back(type, injector);

  return fault_injectors()->size();
}

void FI::trigger_trap(Type_id type, const Trap_options &input) {
  const std::lock_guard<std::mutex> lock{s_mutex};

  if (type > 0) {
    assert(fault_injectors() && type <= fault_injectors()->size());

    (*fault_injectors())[type - 1].trigger(input);
  }
}

void FI::set_trap(const std::string &type, const Conditions &conds,
                  const Trap_options &options) {
#ifdef NDEBUG
  throw std::logic_error("FI not enabled in this build");
#endif
  const std::lock_guard<std::mutex> lock{s_mutex};

  bool flag = false;

  for (auto &fi : *fault_injectors()) {
    if (fi.name() == type) {
      fi.add_trap(conds, options);
      flag = true;
    }
  }
  if (!flag) throw std::invalid_argument("No injection handler named " + type);
}

void FI::clear_traps(const std::string &type) {
  const std::lock_guard<std::mutex> lock{s_mutex};

  bool flag = false;

  for (auto &fi : *fault_injectors()) {
    if (type.empty() || fi.name() == type) {
      fi.clear_traps();
      flag = true;
    }
  }
  if (!flag) throw std::invalid_argument("No injection handler named " + type);
}

void FI::reset_traps(const std::string &type) {
  const std::lock_guard<std::mutex> lock{s_mutex};

  bool flag = false;

  for (auto &fi : *fault_injectors()) {
    if (type.empty() || fi.name() == type) {
      fi.reset();
      flag = true;
    }
  }
  if (!flag) throw std::invalid_argument("No injection handler named " + type);
}

void FI::pause_traps(const std::string &type) {
  const std::lock_guard<std::mutex> lock{s_mutex};

  bool flag = false;

  for (auto &fi : *fault_injectors()) {
    if (type.empty() || fi.name() == type) {
      fi.pause();
      flag = true;
    }
  }
  if (!flag) throw std::invalid_argument("No injection handler named " + type);
}

void FI::resume_traps(const std::string &type) {
  const std::lock_guard<std::mutex> lock{s_mutex};

  bool flag = false;

  for (auto &fi : *fault_injectors()) {
    if (type.empty() || fi.name() == type) {
      fi.resume();
      flag = true;
    }
  }
  if (!flag) throw std::invalid_argument("No injection handler named " + type);
}

}  // namespace utils
}  // namespace mysqlshdk
