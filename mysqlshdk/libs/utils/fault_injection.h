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

#ifndef __MYSQLSHDK_LIBS_UTILS_FAULT_INJECTION_H
#define __MYSQLSHDK_LIBS_UTILS_FAULT_INJECTION_H

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace utils {

/**
 * Fault injection interface class.
 *
 * This allows test code to cause instrumented code to throw exceptions or
 * abort instead of following normal execution, making testing of error cases
 * simpler. It's similar to dbug, but is more flexible because the exceptions
 * being thrown and the exact cases where they're thrown can be customized
 * at runtime.
 *
 * There are 2 sides for that:
 *
 * - the tested code, that registers an injector and triggers them
 * - and the test code, that sets up traps that cause specific triggers to be
 * fired at specific points in execution
 *
 * An injector is a custom routine that executes a custom action, like throwing
 * an exception or calling abort(). That can be done with the FI_DEFINE()
 * macro, which is only enabled if DBUG_OFF is NOT defined.
 *
 * These injectors must then be invoked by the code to be tested (e.g. right
 * before SQL is executed, to simulate client or server DB errors) by using
 * FI_TRIGGER_TRAP(). This will go through traps defined by test code and
 * check if conditions set by the trap are true and if so, invoke the injector.
 *
 * Traps specify that a trigger must be executed when a given list of conditions
 * is met. The list of conditions can include simple logical expressions that
 * are evaluated against variables set by the tested code or the number of times
 * that a trap was tripped. That makes it possible for an error to be injected
 * only when a specific SQL statement is executed 3 times, for example.
 *
 * The trap can also define details about the exception being thrown, like error
 * codes, exception text and so on. These variables/options are injector
 * specific and just forwarded as trap options to the injector function.
 */
class FI {
 public:
  using Trigger_options = std::map<std::string, std::string>;
  using Trap_options = std::map<std::string, std::string>;

  using Type_id = size_t;

  /**
   * Arguments for the injector function, encapsulating trigger and trap
   * options. Trigger options are given by the tested code and trap options
   * are defined by the test code that sets up traps. Trap options will override
   * trigger options.
   */
  class Args {
   public:
    Args(const Trigger_options &trigger_options,
         const Trap_options &trap_options)
        : m_trigger_options(trigger_options), m_trap_options(trap_options) {}

    int get_int(const std::string &arg,
                mysqlshdk::utils::nullable<int> default_value = {}) const {
      auto o = m_trap_options.find(arg);
      if (o != m_trap_options.end()) return std::stoi(o->second);

      o = m_trigger_options.find(arg);
      if (o != m_trigger_options.end()) return std::stoi(o->second);

      if (default_value.is_null())
        throw std::invalid_argument("Invalid argument name '" + arg + "'");
      return *default_value;
    }

    std::string get_string(
        const std::string &arg,
        mysqlshdk::utils::nullable<std::string> default_value = {}) const {
      auto o = m_trap_options.find(arg);
      if (o != m_trap_options.end()) return o->second;

      o = m_trigger_options.find(arg);
      if (o != m_trigger_options.end()) return o->second;

      if (default_value.is_null())
        throw std::invalid_argument("Invalid argument name '" + arg + "'");
      return *default_value;
    }

   private:
    const Trigger_options &m_trigger_options;
    const Trap_options &m_trap_options;
  };

  struct Trap;

  /**
   * Trap condition list.
   *
   * Each trap has a condition list associated, which is evaluated every time
   * it's triggered. Only when all conditions are true the trap will be actually
   * fired. Evaluation stops at the first false condition. Empty lists are
   * allowed.
   *
   * Condition expressions can reference trigger options and built-in variables,
   * which currently is just match_counter. match_counter is an int incremented
   * every time a condition that references it is evaluated (so it behaves like
   * ++match_counter).
   */
  class Conditions {
   public:
    /** Parses and adds a condition represented as a string:
     *
     * opt == str
     * opt regex pattern
     * opt > int
     *
     * Special options:
     * ++match_counter == int
     * ++match_counter > int
     */
    Conditions &add(const std::string &str);

    Conditions &add_regex(const std::string &key, const std::string &value);

    Conditions &add_eq(const std::string &key, const std::string &value);

    Conditions &add_gt(const std::string &key, int value);

   public:
    bool match(Trap *trap, const Trigger_options &options) const;

   private:
    std::list<std::function<bool(Trap *trap, const Trigger_options &options)>>
        m_matchers;
  };

  /**
   * Trap definition.
   *
   * A trap has its condition list evaluated every time the tested code calls
   * FI_TRIGGER_TRAP(), but the trap is only fired if all conditions match.
   *
   * A trap can have a special option called `onetime`, which can be set to 1
   * to indicate that the trap should only be fired once and then deactivate.
   */
  struct Trap {
    Conditions conds;
    Trap_options options;
    bool active = true;
    int match_counter = 0;  // how many times the trap conditions before the one
                            // checking it have matched

    Trap(const Conditions &c, const Trap_options &o) : conds(c), options(o) {}
  };

 public:
  // Routines called by the tested code

  /** Adds a custom fault injector for a fault type and returns an injector
   * context, which should be used to trigger faults.
   *
   * Use the FI_DEFINE() macro instead of calling directly, which becomes
   * a no-op in DBUG_OFF builds.
   */
  static Type_id add_injector(const std::string &type,
                              const std::function<void(const Args &)> &handler);

  /** Calls a fault injector with the given input. A fault will be injected
   * if the injector matches the input.
   *
   * This function may not return.
   *
   * Use the FI_TRIGGER_TRAP() macro instead of calling directly, which becomes
   * a no-op in DBUG_OFF builds.
   */
  static void trigger_trap(Type_id type, const Trigger_options &input);

 public:
  // Routines called by test code

  /** Sets up a trap which will inject a failure of the given type when the trap
   * for it type is triggered and conditions match.
   *
   * options must include options supported by the injector. The built-in
   * `onetime` flag can also be included, which indicates the trap must be
   * deactivated after the 1st time it fires.
   */
  static void set_trap(const std::string &type, const Conditions &conds,
                       const Trap_options &options);

  // remove all traps of the given type
  static void clear_traps(const std::string &type = "");

  // reset state of all traps, which resets the onetime flag and match_counter
  static void reset_traps(const std::string &type = "");
};

#ifdef DBUG_OFF

#define FI_DEFINE(type, injector) struct dummy_fi_##type

#define FI_TRIGGER_TRAP(id, args) \
  do {                            \
  } while (0)

#else

#define FI_DEFINE(type, injector)                      \
  static mysqlshdk::utils::FI::Type_id g_##type##_fi = \
      mysqlshdk::utils::FI::add_injector(STRINGIFY(type), injector)

#define FI_TRIGGER_TRAP(type, args)                          \
  do {                                                       \
    mysqlshdk::utils::FI::trigger_trap(g_##type##_fi, args); \
  } while (0)

#endif

}  // namespace utils

}  // namespace mysqlshdk

#endif  // __MYSQLSHDK_LIBS_UTILS_FAULT_INJECTION_H
